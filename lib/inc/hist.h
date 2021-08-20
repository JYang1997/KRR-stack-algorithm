#ifndef __HIST_H__
#define __HIST_H__

#include <stdio.h>
#include "JY_MACRO.h"
#include <stdint.h>

//histogram sd >= 0
#define LOG 1

#define FIXED 1



typedef struct Hist_t {
	int64_t* sdHist; //contain first to last + coldmiss box
	uint64_t histSize; //number of bins 

#ifdef FIXED
	uint64_t interval;
#elif LOG
	float base; //default as 2
#endif
	uint64_t totalCnt;
	float* missRatio; //
} Hist_t;



Hist_t* histInit(uint64_t histSize, double alpha);
void histFree(Hist_t* hist);
void addToHist(Hist_t* hist, long long sd);
void solveMRC(Hist_t* hist);
void printfHist(FILE* fd, Hist_t* hist);
void printfMRC(FILE* fd, Hist_t* hist);


#endif /*__HIST_H__*/