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
	stack->top = NULL;
}

void stackFree(Rand_Stack_t* stack)
{
	HASH_CLEAR(hh,HashItem);
	Item_t* temp;
	while(stack->top != NULL)
	{
		temp = stack->top->next;
		free((void*)stack->top);
		stack->top = temp;
	}

}


//create new item, attach it to the stack
//insert follow the case where address first time been referenced
void createItem(Rand_Stack_t* stack, uint64_t key)
{
	stack->totalKey++;

	Item_t* n_item = malloc(sizeof(Item_t));

	n_item->addrKey = key;

	n_item->next = NULL;

	HASH_ADD(hh, HashItem, addrKey, sizeof(uint64_t), n_item);
	stackUpdate(stack, n_item, COLDMISS);

}

//hash
long long findItem (Rand_Stack_t* stack, uint64_t key)
{
	long long sd = 1;
	Item_t* t_item, *c_item;

	c_item = stack->top;
	HASH_FIND(hh, HashItem, &key, sizeof(uint64_t), t_item);

	if (t_item != NULL)
	{
		while(c_item != t_item)
		{
			sd++;
			c_item = c_item -> next;
		}

		stackUpdate(stack, c_item, sd);
		return sd;
	}

	return COLDMISS;
}


double fact1(double n) 
{ 
    double res = 1; 
    for (double i = 2; i <= n; i++) 
        res = res * i; 
    return res; 
} 

void stackUpdate(Rand_Stack_t* stack, Item_t* item, long long sd)
{

	if (stack->top == NULL)
	{
		stack->top = item;
		return;
	}


	
	long long curr_index = 2;

	Item_t* yt = stack->top;
	Item_t* old_curr_item = stack->top->next;
	stack->top = item;
	Item_t* curr_item = stack->top;
	Item_t* rest_item = item->next;
	
	if(sd == COLDMISS) sd = stack->totalKey;
	double swap_pos = 1;


	while(curr_index < sd)
	{
		double tmp = ldexp(pcg64_random(), -64);
		// double tmp = (double)pcg64_boundedrand(1000000000);
		// fprintf(stderr, "%f\n", tmp);
		//double curr_prob = (curr_index-1)/(double)(curr_index+(stack->k)-1);
		// double curr_prob = (curr_index-1)/(double)(curr_index+(stack->k)-1)*1000000000;
		// double curr_prob =1.0-((stack->k)*pow((curr_index-1),(stack->k)-1)/pow(curr_index, stack->k));
		//double curr_prob = (curr_index-(stack->k))/(double)(curr_index);
		double curr_prob = pow((curr_index-1.0)/curr_index, stack->k);
		// double curr_prob = 1-pow((1.0)/curr_index, stack->k);
		// double curr_prob = pow(curr_index-1, (stack->k))*pow(stack->k,-0.0009765)/pow((curr_index), stack->k);

		// double sum = 0;

		// double num = 1;
		// double fact = 1;
		// for (double i = 1; i <= stack->k; i++){

		// 	// fact = fact*i;
		// 	// num = num*((stack->k)+1-i);
			
		// 	double kCr = fact1(stack->k) / (fact1(i) * fact1(stack->k - i)); 
		// 	sum += kCr*pow((curr_index-1),stack->k-i);
		// }

		// double curr_prob = 1.0-(sum/pow(curr_index,stack->k));


		
		if (tmp > curr_prob) {
			//swaps occurs
			curr_item->next = yt;
			yt = old_curr_item;
			swap_pos = curr_index;			
		}else {	
			curr_item->next = old_curr_item;
		}

		old_curr_item = old_curr_item->next;
		curr_item = curr_item->next;
		curr_index++;
	}

	curr_item->next = yt;
	yt->next = rest_item;
}



void stackUpdate_binaryprob(Rand_Stack_t* stack, Item_t* item, long long sd)
{

	if (stack->top == NULL)
	{
		stack->top = item;
		return;
	}


	
	long long curr_index = 2;

	Item_t* yt = stack->top;
	Item_t* old_curr_item = stack->top->next;
	stack->top = item;
	Item_t* curr_item = stack->top;
	Item_t* rest_item = item->next;
	
	if(sd == COLDMISS) sd = stack->totalKey;

	while(curr_index < sd)
	{
		float tmp = (rand()/(double)RAND_MAX);
		float curr_prob = pow((curr_index-1)/(float)curr_index, stack->k);

		if (tmp > curr_prob) {
			curr_item->next = yt;
			yt = old_curr_item;			
		}else {	
			curr_item->next = old_curr_item;
		}

		old_curr_item = old_curr_item->next;
		curr_item = curr_item->next;
		curr_index++;
	}

	curr_item->next = yt;
	yt->next = rest_item;
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
	hist -> sdHist = (int*)malloc(sizeof(int)*(slots+2));
	hist -> missRatio = (float*)malloc(sizeof(float)*(slots+2));
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
	ret=fgets(line, 256, rfd);
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

