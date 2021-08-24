#include "random_stack.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include "murmur3.h"
#include "pcg_variants.h"
#include "entropy.h"
#include <math.h>



KRR_Stack_t* stackInit(uint32_t k) {
	KRR_Stack_t* stack = malloc(sizeof(KRR_Stack_t));
	stack->totalKey = 0;
	stack->totalIns = 0;
	stack->k = pow(k,K_EXP);
	stack->capacity = INITIAL_CAPACITY;

	stack->item_array = malloc((stack->capacity)*sizeof(Item_t*));
	stack->HashItem = NULL; //hash table pointer

	return stack;

}


void stackFree(KRR_Stack_t* stack) {
	HASH_CLEAR(hh,stack->HashItem);
	for (int i=0; i<stack->totalKey; i++){
		free((void*)(stack->item_array[i])); 
	}
	free((void*)stack->item_array);


}

//helper for stackResize
//increase the array size by *2
void* arrayResize(void* arry, size_t item_size, uint64_t* curr_cap) {
	uint32_t new_cap = *curr_cap;
	*curr_cap = *curr_cap*2;
		
	void* temp = realloc(arry, (*curr_cap)*item_size);
	if(temp == NULL){
		perror("realloc error arrayresize!");
		exit(-1);
	}else
		arry = temp;

	return arry; 
}


//stack resize for non-fixed size stack
void stackResize(KRR_Stack_t* stack)
{
	if (stack->totalKey >= stack->capacity-1) {
		stack->item_array =(Item_t**)arrayResize(stack->item_array, sizeof(Item_t*), &(stack->capacity));

	}
}




//create new item, attach it to the stack
//insert follow the case where address first time been referenced
void createItem(KRR_Stack_t* stack, uint64_t key, uint32_t size) {

	Item_t* n_item = malloc(sizeof(Item_t));

	n_item->addrKey = key;

	n_item->index = stack->totalKey;


	stack->totalKey++;

	stack->item_array[n_item->index] = n_item;
	

	HASH_ADD(hh, stack->HashItem, addrKey, sizeof(uint64_t), n_item);
	stackUpdate(stack, n_item, NEW_ITEM);

}




//Return "stack distance". (sd start from 1)
//for variable size version, return the approximated min size of cache
// see variableSizeDistance()
long long findItem (KRR_Stack_t* stack, uint64_t key) {
	long long sd;
	Item_t* t_item;
	HASH_FIND(hh, stack->HashItem, &key, sizeof(uint64_t), t_item);

	if (t_item != NULL)
	{
		sd = t_item->index+1;

		stackUpdate(stack, t_item, OLD_ITEM);

		return sd;

	}

	return COLDMISS;
}


//logn stack update method
//logn update partial sum array for variable size version
void stackUpdate(KRR_Stack_t* stack, Item_t* item, uint8_t flag) {
	//edge case
	// if (item->index == 0) return;


	uint64_t j = item->index;
	double x;
	uint64_t tx;

	double exp = 1.0/(stack->k); 



	while (j > 0){
		x = ldexp(pcg64_random(), -64); //PRNG [0,1)
		
	 	x = pow(x, exp)*(j-1); //inverse function, pick a index from 0 to j-1
	 	x = round(x);
	 	tx = x;


		
		//swap items on the fly
		stack->item_array[j] = stack->item_array[tx];
		stack->item_array[j]->index = j;
		j=x;
		
	}


	//replace the first pointer with referenced item, full cyclic swap complete
	stack->item_array[0] = item;
	stack->item_array[0]->index = 0;


	
	stackResize(stack);//stackResize for non-fixed size stack

}



/*******************************************
 * 
 * 
 *
 *******************************************/
void access(KRR_Stack_t*	stack, 
			Hist_t*			hist, 
			uint64_t 		key, 
			uint32_t 		size, 
			double			fixed_rate) {
	stack->totalIns++;

	long long sd = findItem(stack, key);

	if (sd == COLDMISS) {
		createItem(stack, key, size);
	} else {
		sd = sd / fixed_rate;
	}

	addToHist(hist,sd);
	
}



uint32_t random_bits() {
	uint32_t seed = 0;
	int i = 0;
	for (i = 0; i < 32; i++)
		if (pcg64_random()%100 > 50)
			seed = seed | (1 << i);

	return seed;
}

void fixed_rate_shards(FILE*			rfd, 
						 uint32_t		seed, 
						 float 			rate, 
						 Hist_t* 		hist, 
						 KRR_Stack_t*	stack){
	
	// srand(time(0));
	volatile uint64_t seeds[2];
    entropy_getbytes((void*)seeds, sizeof(seeds));
    pcg64_srandom(seeds[0], seeds[1]);


	uint64_t d = 0;
	uint64_t hash[2];     
	uint64_t P = 1;
	P = P << 24;
	uint64_t T = (uint64_t)(P * rate); //truncate instead of round


	char *keyStr;
	char *sizeStr;
	uint32_t size;
	uint64_t key;
	char* ret;
	char   line[256];



	if(seed == 0) seed = random_bits(); //defined in this file
	
	printf("seed: %u\n",seed );
	///////////////////progress bar////////////////////////////
	char bar[28];
	int i;
	//first line contain total reference number
	bar[0] = '[';
	bar[26] = ']';
	bar[27]='\0';
	for (i=1; i<=25; i++) bar[i] = ' ';
	ret = fgets(line, 256, rfd);
	keyStr = strtok(line, " ");
	keyStr = strtok(NULL, " ");
	uint64_t total = strtoull(keyStr, NULL, 10);
	uint64_t star = total/100;
	///////////////////////////////////////////////////////////
	i=1;
	
	while ((ret=fgets(line, 256, rfd)) != NULL)
	{
		// key = strtoull(line, NULL, 10);
		// keyStr = strtok(line, "\n");
		
		keyStr = strtok(line, ",");
		key = strtoull(keyStr, NULL, 10);
		sizeStr = strtok(NULL, ",");
		if (sizeStr != NULL)	
			size = strtoul(sizeStr, NULL, 10);
		else
			size = 1;


		struct timeval  tv1, tv2;
		gettimeofday(&tv1, NULL);
		
		MurmurHash3_x64_128(keyStr, strlen(keyStr), seed, hash);
		if ((unsigned long long)(hash[1] & (P-1)) < T)
			access(stack, hist, key, size, rate);

		/* stuff to do! */
		gettimeofday(&tv2, NULL);
		tt_time += (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
		d++;
		
		if(d % star == 0)
		{	
			if( d%(star*4) == 0)
				bar[i++] = '#';
			printf("\rProgress: %s%d%% %ld", bar, (int)(d/(double)total*100)+1, d);
			fflush(stdout);
		}
		
	}
	printf("\n");

	int64_t diff = ((d*(double)rate) - stack->totalIns);
	hist->sdHist[0] = hist -> sdHist[0] + diff;
	stack->totalIns = stack->totalIns + diff;
	hist->totalCnt = hist->totalCnt + diff;
}

