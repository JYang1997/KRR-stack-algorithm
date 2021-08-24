#include "random_stack.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include "murmur3.h"
#include "pcg_variants.h"
#include "entropy.h"
#include <math.h>



void stackInit(Rand_Stack_t* stack, int32_t k)
{
	stack->totalKey = 0;
	stack->totalIns = 0;
	stack->k = pow(k,K_EXP);
	stack->capacity = INITIAL_CAPACITY;
	
	stack->item_array = malloc((stack->capacity)*sizeof(Item_t*));

}


void stackFree(Rand_Stack_t* stack)
{
	HASH_CLEAR(hh,HashItem);
	for (int i=0; i<stack->totalKey; i++)
		free((void*)(stack->item_array[i])); 
	free((void*)stack->item_array);

}

//internal helper used in
//increase the array size by *2
void* arrayResize(void* arry, size_t item_size, uint32_t* curr_cap)
{
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



void stackResize(Rand_Stack_t* stack)
{
	if (stack->totalKey >= stack->capacity-1) {
		stack->item_array =(Item_t**)arrayResize(stack->item_array, sizeof(Item_t*), &(stack->capacity));
	}
}




//create new item, attach it to the stack
//insert follow the case where address first time been referenced
void createItem(Rand_Stack_t* stack, uint64_t key)
{

	Item_t* n_item = malloc(sizeof(Item_t));

	n_item->addrKey = key;

	n_item->index = stack->totalKey;

	stack->totalKey++;

	stack->item_array[n_item->index] = n_item;
	

	HASH_ADD(hh, HashItem, addrKey, sizeof(uint64_t), n_item);
	stackUpdate(stack, n_item);

}

//hash
long long findItem (Rand_Stack_t* stack, uint64_t key)
{
	long long sd = 1;
	Item_t* t_item;
	HASH_FIND(hh, HashItem, &key, sizeof(uint64_t), t_item);

	if (t_item != NULL)
	{

		sd += t_item->index;
	
		stackUpdate(stack, t_item);
		return sd;

	}

	return COLDMISS;
}



void update_TupleStack(int32_t loc1, int32_t loc2){

	Tuple* tmp_tpl = (Tuple*)malloc(sizeof(Tuple));
	tmp_tpl->loc1 = loc1;
	tmp_tpl->loc2 = loc2;
	STACK_PUSH(StackHead, tmp_tpl);
}


//stack is maintained as array in this version
//in NR with small K only few place need swap during update
//
//update logic:
//	1. use tree top down traversal, locate swap locations
//  2. do a linear swap to such locations
//  3. must memorize all wait for swap locations
//  5. instead of meorize the item, we want to memorize the index of swap location
//  6. then iterate through the index for fast swap.
//  4. position 1, and position sd must be swap, when cold miss occur
//  sd should be the current stacksize

void swap(Rand_Stack_t* stack, int index, Item_t** yt) {
	Item_t* temp = stack->item_array[index];
	stack->item_array[index]=*yt;
	stack->item_array[index]->index = index;
	*yt = temp;
}

//optimization note:
// use static temp array
// reduce elt->wght to one instructions
// remove utstack, replace with a array 
void stackUpdate(Rand_Stack_t* stack, Item_t* item)
{
	// int tmp_cnt = 0;

	//edge case
	if (item->index == 0) return;

	//start DFS
	//interval is here 
	//initial inter is from 2 to sd-1
	//where sd-1 >= 3 
	//then we perform a DFS, traverse from left to right
	stackResize(stack);

	int32_t phi = item->index+1;
	Item_t* yt = item;
	swap(stack, 0, &yt);

	//determine whether there is swaps from 2 to sd -1
	if(ldexp(pcg64_random(), -64) > pow((1.0)/phi, stack->k)) {
		
		if(phi == 2){
			swap(stack, 1, &yt);
		} else
			update_TupleStack(2, phi);//initial Tuple
		
		
		
		Tuple* elt;
		double nsw1, nsw2, sw1, sw2, ntvl1, ntvl2, wght, tmp;

		while(!STACK_EMPTY(StackHead))
		{
			//for every poped tuple we gurantee there is swap inside the interval
			STACK_POP(StackHead, elt);
			// tmp_cnt++;

			int32_t mid = ceil(((elt->loc2)+(elt->loc1))/2.0);
			// mid = floor(((elt->loc2)+mid)/2.0);

			
			//first interval is loc1 to mid-1
			//second interval is mid to loc2

			//probability where only second pop with normalization
			nsw1 = pow(((elt->loc1)-1.0)/(mid-1.0), stack->k);
			nsw2 = pow((mid-1.0)/(elt->loc2), stack->k);
			sw1 = 1.0 - nsw1;
			sw2 = 1.0 - nsw2;

			//multiplication faster than division
			ntvl1 = (sw1*nsw2);
			ntvl2 = (nsw1*sw2);
			wght = (ntvl1+ntvl2+(sw1*sw2));//normalize by total swap prob

			tmp = ldexp(pcg64_random(), -64);


			if (tmp < (ntvl1/wght)) { //only swps in first interval
				if (elt->loc1 == mid-1) {
					swap(stack, elt->loc1-1, &yt);
				} else
					update_TupleStack(elt->loc1, mid-1);	
			} else if(tmp < (ntvl1+ntvl2)/wght) { // only swps on second interval
				if (elt->loc2 == mid) {
					swap(stack, elt->loc2-1, &yt);
				} else
					update_TupleStack(mid, elt->loc2);
			}else {// swps in both interval
				//second
				if (elt->loc2 != mid) update_TupleStack(mid, elt->loc2);
				
				//first
				if (elt->loc1 != mid-1) update_TupleStack(elt->loc1, mid-1);

				//first
				if (elt->loc1 == mid-1) swap(stack, elt->loc1-1, &yt);
				//second
				if (elt->loc2 == mid) swap(stack, elt->loc2-1, &yt);
								
			}

			free((void*)elt);

		}
	}

	swap(stack, phi-1, &yt);

}




void access(Rand_Stack_t* stack, Hist_t* hist, uint64_t key, float fixed_rate)
{
	stack->totalIns++;

	long long sd = findItem(stack, key);

	if (sd == COLDMISS) {createItem(stack, key);}
	else{
		
		sd--;
		sd = sd / fixed_rate;

	}

	addToHist(hist,sd);
	
	//add sd to sdHis

}


/***************Histogram Section  Start*************************/

/***************Histogram Section**************************
 Functions below are independent from Cache eviction policy
 Mainly used for generate MRC based on cache model



 ********************************************************/


void histInit(Hist_t* hist, int begin, int end, int interval)
{
	//consider edge case later
	int slots = ((end - begin)/interval);
	hist -> first = begin;
	hist -> size = slots+2;
	hist -> interval = interval;
	hist -> sdHist = malloc(sizeof(int)*(slots+2));
	hist -> missRatio = malloc(sizeof(float)*(slots+2));
	int i;
	for(i = 0; i < slots+2; i++)
	{
		hist -> sdHist[i] = 0;
	}
}


void histFree(Hist_t* hist)
{
	free((void*)hist->sdHist);
	free((void*)hist->missRatio);
}


void addToHist(Hist_t* hist, long long ird)
{
	if(ird == COLDMISS || ird > (((hist->size)-2)*(hist->interval)+(hist->first))){

	// if (ird == COLDMISS){
		hist -> sdHist[hist->size-1]++; //cold miss bin++
	}else{
		if(ird - hist->first >= 0)
		{
			long long index = ((ird - hist->first) / (hist->interval)) + 1;
			hist->sdHist[index]++;
		}else{
			hist->sdHist[0]++;
		}
	}
}


// normal MRC

void solveMRC(Hist_t* hist, Rand_Stack_t* stack)
{
	long long i;
	hist -> missRatio[hist->size-1] = (hist -> sdHist[hist->size-1])/((double)(stack -> totalIns));
	//hist -> missRatio[hist->size-1] = .56575;
	hist -> missRatio[hist->size-2] = hist -> missRatio[hist->size-1];
	for(i = hist->size-3; i >= 0; i--)
	{
		hist -> missRatio[i] = ((hist -> sdHist[i+1])/((double)(stack -> totalIns))) + hist->missRatio[i+1];
	}
}



void printfMRC(FILE* fd, Hist_t* hist)
{
	int i;
	for(i = 0; i < hist->size-1; i++)
		fprintf(fd, "%d, %f\n", hist->first+i*(hist->interval), hist->missRatio[i]);
}

void printfHist(FILE* fd, Hist_t* hist, Rand_Stack_t* stack)
{
	int i;
	fprintf(fd, "cold miss:%d\ntotal instructions:%d\n",stack->totalKey, stack->totalIns);
	for(i = 0; i < hist->size-1; i++)
	 	fprintf(fd, "%d, %d\n",hist->first+i*(hist->interval), hist->sdHist[i]);
}

/***************Histogram Section  END*************************/
/***************Histogram Section  END*************************/

uint32_t random_bits()
{
	// srand(time(0));
	uint32_t seed = 0;
	int i = 0;
	for (i = 0; i < 32; i++)
		if (pcg64_random()%100 > 50)
			seed = seed | (1 << i);

	return seed;
}

void random_stack_shards(FILE* rfd, uint32_t seed, float rate, Hist_t* hist, Rand_Stack_t* stack )
{
	
	// srand(time(0));
	uint64_t seeds[2];
    entropy_getbytes((void*)seeds, sizeof(seeds));
    pcg64_srandom(seeds[0], seeds[1]);


	uint64_t d = 0;
	uint64_t hash[2];     
	uint64_t P = 1;
	P = P << 24;
	uint64_t T = (uint64_t)(P * rate); //truncate instead of round


	char *keyStr;
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
		key = strtoull(line, NULL, 10);
		keyStr = strtok(line, "\n");
		


		struct timeval  tv1, tv2;
		gettimeofday(&tv1, NULL);
		
		MurmurHash3_x64_128(keyStr, strlen(keyStr), seed, hash);
		if ((unsigned long long)(hash[1] & (P-1)) < T)
			access(stack, hist, key, rate);

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

	stack->totalIns = stack->totalIns + abs((int)(d*rate) - stack->totalIns);
	hist->sdHist[0] = hist -> sdHist[0] + abs((int)(d*rate) - stack->totalIns);

}

