

#ifndef JY_STACK_RAND_H
#define JY_STACK_RAND_H

#include "uthash.h"
#include "utstack.h"
#include "util/hist.h"
#include "pqueue.h"
#include <stdint.h>
#include <sys/time.h>


#define INITIAL_CAPACITY 32

#ifndef K_EXP
	#define K_EXP 1.4
#endif

#ifndef SIZE_BASE
	#define SIZE_BASE 1.2
#endif



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
#ifdef VARSIZE
	int32_t size; //signed totSize_array
#endif
	UT_hash_handle hh;
} Item_t;


typedef struct KRR_Stack_t {

	uint64_t totalGet; //total get command
	uint64_t totalSet; //total set command
	uint64_t totalDel; //total delete command
	uint64_t totalMod; //total update command
	uint64_t totalIns; //total number of sampled instructions

	int64_t totalKey; //total sampled keys in the stack, including delete position holder
	double k; //k for "k"-lru, default k = input k^1.4
	uint64_t capacity; //size of the stack array initialize to 32, 
					   //double every time 

	Item_t** item_array; //array store pointer of hashed item
	Item_t *HashItem; //hash table head of stored objects

	pqueue_t* holder_heap;//the min heap use to hold the deleted nodes in the stack

#ifdef VARSIZE
	int64_t totalSize; //total size of stack //default in bytes (stat)
	int64_t* totSize_array; //array of partial sums, 1, size_base^1, size_base^2 ...
	uint32_t tsLen; //totalSize_array length, maxsize = O(logn)
#endif

} KRR_Stack_t;



//*************public Interface********************//

KRR_Stack_t* stackInit(uint32_t k);

void stackFree(KRR_Stack_t* stack);


/**
 * @param stack,
 * @param key, 
 * @param size, //if in uniform size mode, the size parameter is ignored
 * @param commandStr, "GET, SET, DELETE, UPDATE"
 * @return if command is GET, then return stack distance
 * 		   otherwise, return INVALID 
 **/
int64_t KRR_access(KRR_Stack_t* 	stack, 
				   uint64_t 		key, 
				   int32_t 			size, 
				   char*			commandStr);

/**
 * @param stack
 * @param key
 * @param size
 * @return stack distance
 * 		   only GET request incur miss penalty,  
 * 			other command might alter cache state but does not cause penalty
 **/
int64_t KRR_GET(KRR_Stack_t* 	stack,
				uint64_t 		key,
				int32_t 		size);

/**
 * @param stack
 * @param key
 * @param size 
 **/
void KRR_SET(KRR_Stack_t* 	stack,
			 uint64_t 		key,
			 int32_t 		size);

/**
 * @param stack
 * @param key 
 **/
void KRR_DELETE(KRR_Stack_t* 	stack, 
				uint64_t 		key);

/**
 * @param stack
 * @param key 
 * @param size
 **/
void KRR_UPDATE(KRR_Stack_t* stack,
			   	uint64_t key,
			    int32_t size);




//******************************************************************//





#endif