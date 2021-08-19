#ifndef __HIST_H__
#define __HIST_H__

#include <math.h>
#include <stdio.h>
#include "util/JY_MACRO.h"

//histogram sd >= 0
#define LOG 1

#define FIXED 1


// #define COLDMISS -1324

// #define LOGA(x,a) ((log(x)/log(a)))



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

Hist_t* histInit(uint64_t histSize, double alpha) {
		

	Hist_t* hist = malloc(sizeof(Hist_t));

	hist->sdHist = malloc(sizeof(int64_t)*(histSize+2));

	hist->missRatio =  malloc(sizeof(float)*(histSize+2));
	hist->histSize = histSize+2; //cold miss are stored in index histSize+1
								//index histSize is empty pucket
	hist->totalCnt = 0;


	if (alpha <= 0 || histSize <= 0) {
		perror("histogram init error, parameter <= 0.\n");
		return NULL;
	}

#ifdef FIXED
	hist->interval = (uint64_t)alpha;
#elif LOG
	hist->base = alpha;
#endif
	memset(hist->sdHist, 0, sizeof(int64_t)*hist->histSize);

	return hist;
}

void histFree(Hist_t* hist) {
	free((void*)hist->sdHist);
	free((void*)hist->missRatio);
}

void addToHist(Hist_t* hist, long long sd) {
	
	hist->totalCnt++;

	if (sd == COLDMISS) {
		hist->sdHist[hist->histSize-1] += 1;  
		return;
	}

	if (sd < 0) {
		// perror("add hist error sd < 0");
		fprintf(stderr, "add hist error sd < 0; sd = %lld \n", sd);
		exit(-1);
		return;
	}

	if (sd > (hist->histSize-2)*(hist->interval)) {
		hist->sdHist[hist->histSize-1] += 1;  
		// printf("cnt: %ld sd:%lld\n",hist->totalCnt,sd);
	} else {
		//index 0 contain sd from {1 to ....interval-1}  
		uint64_t index; 
#ifdef FIXED 
		index = sd/hist->interval; 
#elif LOG
		//
		index = LOGA(sd,hist->base);
#endif
		hist->sdHist[index]++;
	}


}


void solveMRC(Hist_t* hist) {
	int64_t i;
	hist -> missRatio[hist->histSize-1] = (hist->sdHist[hist->histSize-1])/((double)(hist->totalCnt));
	hist -> missRatio[hist->histSize-2] = hist -> missRatio[hist->histSize-1];
	for(i = hist->histSize-3; i >= 0; i--)
	{
		hist -> missRatio[i] = ((hist -> sdHist[i+1])/((double)(hist->totalCnt))) + hist->missRatio[i+1];
	}
}


void printfMRC(FILE* fd, Hist_t* hist)
{
	int64_t i;
	for(i = 0; i < hist->histSize-2; i++)
	{
#ifdef FIXED
		fprintf(fd, "%ld, %f\n", (i+1)*(hist->interval), hist->missRatio[i]);
#elif LOG
		fprintf(fd, "%ld, %f\n", exp(hist->base, i), hist->missRatio[i]);
#endif
	}
}



void printfHist(FILE* fd, Hist_t* hist)
{
	int64_t i;
	fprintf(fd, "cold miss:%ld\ntotal instructions:%ld\n",
						hist->sdHist[hist->histSize-1], hist->totalCnt);
	
	for(i = 0; i < hist->histSize-2; i++)
	{
		#ifdef FIXED
			fprintf(fd, "%ld, %ld\n", (i+1)*(hist->interval), hist->sdHist[i]);
		#elif LOG
			fprintf(fd, "%ld, %ld\n", exp(hist->base, i), hist->sdHist[i]);
		#endif
	}
}


#endif /*__HIST_H__*/