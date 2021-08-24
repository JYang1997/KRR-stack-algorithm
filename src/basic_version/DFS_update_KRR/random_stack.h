

#ifndef JY_STACK_RAND_H
#define JY_STACK_RAND_H

#include "uthash.h"
#include "utstack.h"
#include <stdint.h>
#include <sys/time.h>
#define COLDMISS -1234
#define INITIAL_CAPACITY (1024*1024)
#define INVALID -1
#ifndef K_EXP
	#define K_EXP 1.4
#endif

// int expect_cnt = 0;
// int expect_access = 0;

int TIME_FLAG=0;
double tt_time =0;

typedef struct Item_t
{
	uint64_t addrKey;
	int32_t index;
	UT_hash_handle hh;
} Item_t;

Item_t *HashItem = NULL;

typedef struct Rand_Stack_t {
	
	int32_t totalKey;
	int32_t totalIns;
	double k; //k for "k"-lru
	int32_t capacity;
	Item_t** item_array;

} Rand_Stack_t;

//Internal struct used for stack update
typedef struct Tuple {
	int32_t loc1; //start index of the interval
	int32_t loc2; //end index of the interval
	struct Tuple *next;
} Tuple;//inclusive

Tuple* StackHead = NULL;


typedef struct Hist_t {
	int32_t* sdHist; //contain first to last + coldmiss box
	int first;
	int size; 
	int interval;
	float* missRatio;
} Hist_t;



void stackInit(Rand_Stack_t* stack, int32_t k);
void stackFree(Rand_Stack_t* stack);
void stackResize(Rand_Stack_t* stack);

void* arrayResize(void* arry, size_t item_size, uint32_t *curr_cap); //internal use

void stackUpdate(Rand_Stack_t* stack, Item_t* item);
void createItem(Rand_Stack_t* stack, uint64_t key);
long long findItem(Rand_Stack_t* stack, uint64_t key);



void histInit(Hist_t* hist, int begin, int end, int interval);
void histFree(Hist_t* hist);
void addToHist(Hist_t* hist, long long sd);
void solveMRC(Hist_t* hist, Rand_Stack_t* stack);


void access(Rand_Stack_t* stack, Hist_t* hist, uint64_t key, float fixed_rate);





#endif