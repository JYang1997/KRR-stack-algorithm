

#ifndef JY_STACK_RAND_H
#define JY_STACK_RAND_H

#include "uthash.h"
#include "utstack.h"
#include "util/hist.h"
#include <stdint.h>
#include <sys/time.h>

#ifndef COLDMISS
	#define COLDMISS -1234
#endif

#define INITIAL_CAPACITY 32
#define INVALID -1
#ifndef K_EXP
	#define K_EXP 1.4
#endif


#define SIZE_BASE 1.2
#define NEW_ITEM 1
#define OLD_ITEM 0



int TIME_FLAG=0;
double tt_time =0;

// we don't track item's recency
// under KRR policy, item's rank is exactly its stack position
// (we use item's rank to probabilistically determine relative priority between objects)
// note that under KLRU, item's rank is exactly its relative recency ranking

// KLRU is equivalent to KRR iff stack ordering == recency ordering
// note this is only true if policy is LRU 
typedef struct Item_t
{
	uint64_t addrKey;
	uint64_t index; //item's stack position. 

	UT_hash_handle hh;
} Item_t;


typedef struct KRR_Stack_t {
	
	uint64_t totalKey; //total sampled keys in the stack
	uint64_t totalIns; //total number of sampled instructions

	double k; //k for "k"-lru, default k = input k^1.4
	uint64_t capacity; //size of the stack array initialize to 32, 
					   //doubling every time when not enough slots

	Item_t** item_array; //array store pointer of hashed item
	Item_t *HashItem; //hash table head of stored objects



} KRR_Stack_t;



//*************public api********************//

KRR_Stack_t* stackInit(uint32_t k);

void stackFree(KRR_Stack_t* stack);

//if in uniform size mode, the size parameter is ignored
void access(KRR_Stack_t* 	stack, 
			Hist_t* 		hist, 
			uint64_t 		key, 
			uint32_t 		size, 
			double 			fixed_rate);




//******************************************************************//
void stackUpdate(KRR_Stack_t* stack, Item_t* item, uint8_t flag);
void createItem(KRR_Stack_t* stack, uint64_t key, uint32_t size);
long long findItem(KRR_Stack_t* stack, uint64_t key);
void stackResize(KRR_Stack_t* stack);
void* arrayResize(void* arry, size_t item_size, uint64_t *curr_cap); 




#endif