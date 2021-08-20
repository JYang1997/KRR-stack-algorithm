#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "hist.h"
#include "utils.h"
#include "JY_MACRO.h"
#include "murmur3.h"
#include "spatial_sampling.h"
#include "twitter_2020.h"




void fixed_rate_spatial_sampling (FILE* 	rfd,
								  void* 	stack,
								  access_func access,
							 	  uint32_t 	seed,
								  float 	sampling_rate,
								  Hist_t* 	hist,
								  double* 	timePtr){


	if (timePtr != NULL) *timePtr = 0;


	char *keyStr;
	char *sizeStr;
	char *commandStr = NULL;
	uint32_t size;
	uint64_t key;
	int64_t sd;
	char* ret;
	char   line[1024];

	if(seed == 0) seed = jy_32_random(); //defined in this file
	
	fprintf(stdout,"seed: %u\n",seed );


	ret = fgets(line, 256, rfd);
	keyStr = strtok(line, " ");
	keyStr = strtok(NULL, " ");
	uint64_t total = strtoull(keyStr, NULL, 10);

	progress_bar_t* bar;
	PROGRESS_BAR_INIT(total, &bar);


	uint64_t actualGetCnt = 0;
	uint64_t hash[2];     
	uint64_t P = 1;
	P = P << 24;
	uint64_t T = (uint64_t)(P * sampling_rate); //truncate instead of round
	
	while ((ret=fgets(line, 256, rfd)) != NULL)
	{

		
		keyStr = strtok(line, ",");
		key = strtoull(keyStr, NULL, 10);
		sizeStr = strtok(NULL, ",");
		size = (sizeStr != NULL) ? strtoul(sizeStr, NULL, 10) : 1;
		commandStr = strtok(NULL, ",");
		commandStr = (commandStr == NULL) ? "GET" : commandStr;

		if(strcmp(commandStr, "GET") == 0) actualGetCnt++;

		struct timeval  tv1, tv2;
		gettimeofday(&tv1, NULL);
		
		MurmurHash3_x64_128(keyStr, strlen(keyStr), seed, hash);
		if ((unsigned long long)(hash[1] & (P-1)) < T) {
			sd = access(stack, key, size, commandStr);

			if (sd != INVALID) { 
				sd = (sd != COLDMISS) ? sd / sampling_rate : sd;
			
				addToHist(hist,sd);
			}
		}

		gettimeofday(&tv2, NULL);

		if (timePtr != NULL) {
			*timePtr += (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
		}
		

		PROGRESS_BAR_UPDATE(bar);
		
	}



	//vertical shift correction

	int64_t diff = ((actualGetCnt*(double)sampling_rate) - hist->totalCnt);
	hist->sdHist[0] = hist -> sdHist[0] + diff;
	// stack->totRef = stack->totRef + diff; //shards should only make change to the hist
	hist->totalCnt = hist->totalCnt + diff;

}





void tw_fixed_rate_spatial_sampling(char* fileName,
								  void* 	stack,
								  access_func access,
							 	  uint32_t 	seed,
								  float 	sampling_rate,
								  Hist_t* 	hist,
								  double* 	timePtr){


	if (timePtr != NULL) *timePtr = 0;
	struct timeval  tv1, tv2;


	if(seed == 0) seed = jy_32_random(); //defined in this file
	
	fprintf(stderr,"seed: %u\n",seed );


	uint64_t total = 0;
	COUNT_FILE_LINE(fileName, &total);

	progress_bar_t* bar;
	PROGRESS_BAR_INIT(total, &bar);


	char* commandStr = NULL;


	tw_iterator_t* itr = tw_trace_init(fileName, 10000, CONTINUE);
	tw_ref_t* ref = NULL;

	uint32_t size;
	int64_t sd;
	uint64_t actualGetCnt = 0;
	uint64_t hash[2];     
	uint64_t P = 1;
	P = P << 24;
	uint64_t T = (uint64_t)(P * sampling_rate); //truncate instead of round
	
	while (!tw_trace_finished(itr))
	{
		tw_ref_t* ref = tw_trace_next(itr);

		// get/gets/set/add/replace/cas/append/prepend/delete/incr/decr
		if (strcmp(ref->op, "get") == 0 //get
		 || strcmp(ref->op, "get") == 0) {
			commandStr = "GET";
		} else if ( //set
			strcmp(ref->op, "set") == 0
		 || strcmp(ref->op, "replace") == 0) {
			commandStr = "SET";
		} else if ( //update
			strcmp(ref->op, "add") == 0
		 || strcmp(ref->op, "cas") == 0
		 || strcmp(ref->op, "append") == 0
		 || strcmp(ref->op, "prepend") == 0
		 || strcmp(ref->op, "incr") == 0
		 || strcmp(ref->op, "decr") == 0) {
			commandStr = "UPDATE";
		} else if ( //delete
			strcmp(ref->op, "delete") == 0) {
			commandStr = "DELETE";
		} else {
			printf("command not recognized: %s\n",ref->op);
			exit(-1);
		}

		if(strcmp(commandStr, "GET") == 0) actualGetCnt++;

		size = ref->val_size + ref->key_size;


		gettimeofday(&tv1, NULL);
		
		MurmurHash3_x64_128(ref->murmur3_hashed_key, 16, seed, hash);
		if ((unsigned long long)(hash[1] & (P-1)) < T) {
			sd = access(stack, ref->murmur3_hashed_key[1], size, commandStr);

			if (sd != INVALID) { 
				sd = (sd != COLDMISS) ? sd / sampling_rate : sd;
			
				addToHist(hist,sd);
			}
		}

		gettimeofday(&tv2, NULL);

		if (timePtr != NULL) {
			*timePtr += (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
		}
		

		PROGRESS_BAR_UPDATE(bar);
		
	}

	//vertical shift correction

	int64_t diff = ((actualGetCnt*(double)sampling_rate) - hist->totalCnt);
	hist->sdHist[0] = hist -> sdHist[0] + diff;
	// stack->totRef = stack->totRef + diff; //shards should only make change to the hist
	hist->totalCnt = hist->totalCnt + diff;

}

void ycsb_fixed_rate_spatial_sampling (FILE* 	rfd,
								  void* 	stack,
								  access_func access,
							 	  uint32_t 	seed,
								  float 	sampling_rate,
								  Hist_t* 	hist,
								  double* 	timePtr){


	if (timePtr != NULL) *timePtr = 0;


	char *keyStr;
	char *sizeStr;
	char *commandStr = NULL;
	uint32_t size;
	uint64_t key;
	int64_t sd;
	char* ret;
	char   line[1024];

	if(seed == 0) seed = jy_32_random(); //defined in this file
	
	fprintf(stdout,"seed: %u\n",seed );


	ret = fgets(line, 256, rfd);
	keyStr = strtok(line, " ");
	keyStr = strtok(NULL, " ");
	uint64_t total = strtoull(keyStr, NULL, 10);

	progress_bar_t* bar;
	PROGRESS_BAR_INIT(total, &bar);


	uint64_t actualGetCnt = 0;
	uint64_t hash[2];     
	uint64_t P = 1;
	P = P << 24;
	uint64_t T = (uint64_t)(P * sampling_rate); //truncate instead of round
	
	while ((ret=fgets(line, 256, rfd)) != NULL)
	{

		
		keyStr = strtok(line, ",");
		key = strtoull(keyStr, NULL, 10);
		sizeStr = strtok(NULL, ",");
		size = (sizeStr != NULL) ? strtoul(sizeStr, NULL, 10) : 1;
		commandStr = strtok(NULL, ",");
		commandStr = (commandStr == NULL) ? "GET" : commandStr;

		if(strcmp(commandStr, "GET") == 0) actualGetCnt++;

		struct timeval  tv1, tv2;
		gettimeofday(&tv1, NULL);
		
		MurmurHash3_x64_128(keyStr, strlen(keyStr), seed, hash);
		if ((unsigned long long)(hash[1] & (P-1)) < T) {
			sd = access(stack, key, size, commandStr);

			if (sd != INVALID) { 
				sd = (sd != COLDMISS) ? sd / sampling_rate : sd;
			
				addToHist(hist,sd);
			}
		}

		gettimeofday(&tv2, NULL);

		if (timePtr != NULL) {
			*timePtr += (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
		}
		

		PROGRESS_BAR_UPDATE(bar);
		
	}



	//vertical shift correction

	int64_t diff = ((actualGetCnt*(double)sampling_rate) - hist->totalCnt);
	hist->sdHist[0] = hist -> sdHist[0] + diff;
	// stack->totRef = stack->totRef + diff; //shards should only make change to the hist
	hist->totalCnt = hist->totalCnt + diff;

}

