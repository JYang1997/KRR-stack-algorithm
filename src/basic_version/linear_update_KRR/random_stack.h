

#ifndef JY_STACK_RAND_H
#define JY_STACK_RAND_H

#include "uthash.h"
#include <stdint.h>
#include <sys/time.h>
#define COLDMISS -1234
#ifndef K_EXP
	#define K_EXP 1.4
#endif



int TIME_FLAG=0;
double tt_time =0;

typedef struct Item_t
{
	uint64_t addrKey;
	struct Item_t* next;
	UT_hash_handle hh;
} Item_t;

Item_t *HashItem = NULL;

typedef struct Rand_Stack_t {
	
	int32_t totalKey;
	int32_t totalIns;
	int32_t k; //k for "k"-lru
	struct Item_t* top;

} Rand_Stack_t;


typedef struct Hist_t {
	int32_t* sdHist; //contain first to last + coldmiss box
	int first;
	int size; 
	int interval;
	float* missRatio;
} Hist_t;



void stackInit(Rand_Stack_t* stack, int32_t k);
void stackFree(Rand_Stack_t* stack);

void stackUpdate(Rand_Stack_t* stack, Item_t* item, long long sd);
void createItem(Rand_Stack_t* stack, uint64_t key);
long long findItem(Rand_Stack_t* stack, uint64_t key);



void histInit(Hist_t* hist, int begin, int end, int interval);
void histFree(Hist_t* hist);
void addToHist(Hist_t* hist, long long sd);
void solveMRC(Hist_t* hist, Rand_Stack_t* stack);


void access(Rand_Stack_t* stack, Hist_t* hist, uint64_t key, float fixed_rate);




#endif