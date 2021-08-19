#include <assert.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include "pcg_variants.h"
#include "entropy.h"
// #include "spatial_sampling.h"
#include "random_stack_shards.c"

typedef int64_t (*access_func)(void*, uint64_t, uint32_t, char*);
uint32_t random_bits() {
	uint32_t seed = 0;
	int i = 0;
	for (i = 0; i < 32; i++)
		if (pcg64_random()%100 > 50)
			seed = seed | (1 << i);

	return seed;
}

void fixed_rate_spatial_sampling (FILE* 	rfd,
								  void* 	stack,
								  access_func access,
							 	  uint32_t 	seed,
								  float 	sampling_rate,
								  Hist_t* 	hist,
								  double* 	timePtr){

	volatile uint64_t seeds[2];
    entropy_getbytes((void*)seeds, sizeof(seeds));
    pcg64_srandom(seeds[0], seeds[1]);

	if (timePtr != NULL) *timePtr = 0;


	char *keyStr;
	char *sizeStr;
	char *commandStr = NULL;
	uint32_t size;
	uint64_t key;
	int64_t sd;
	char* ret;
	char   line[1024];
	char* tempLine;

	if(seed == 0) seed = random_bits(); //defined in this file
	
	fprintf(stderr,"seed: %u\n",seed );
	///////////////////progress bar////////////////////////////
	char bar[28];
	int i;
	//first line contain total reference number
	bar[0] = '[';
	bar[26] = ']';
	bar[27]='\0';
	for (i=1; i<=25; i++) bar[i] = ' ';
	// ret = fgets(line, 256, rfd);
	// keyStr = strtok(line, " ");
	// keyStr = strtok(NULL, " ");
	uint64_t total = 10000000;
	uint64_t star = total/100;
	i=1;
	///////////////////////////////////////////////////////////
	



	uint64_t d = 0;
	uint64_t actualGetCnt = 0;
	uint64_t hash[2];     
	uint64_t P = 1;
	P = P << 24;
	uint64_t T = (uint64_t)(P * sampling_rate); //truncate instead of round
	
	int tempc = 0;
	while ((ret=fgets(line, 1024, rfd)) != NULL)
	{

		tempLine = line;

		keyStr = strsep(&tempLine, ","); 

		if (strcmp(keyStr, "[OVERALL]") == 0) break;
        if (++tempc < 23) continue;
        
		key = strtoull(keyStr, NULL, 10);
		sizeStr = strsep(&tempLine, ","); 
		size = (sizeStr != NULL) ? strtoul(sizeStr, NULL, 10) : 1;
		commandStr = strsep(&tempLine, "\n"); 
		commandStr = (commandStr == NULL) ? "GET" : commandStr;
		// printf("line: %s command:%s\n", keyStr, commandStr);
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
		
		d++;
		
		if(d % star == 0)
		{	
			if( d%(star*4) == 0)
				bar[i++] = '#';
			fprintf(stderr,"\rProgress: %s%d%% %ld", bar, (int)(d/(double)total*100)+1, d);
			fflush(stdout);
		}
		
	}
	fprintf(stderr,"\n");


	//vertical shift correction

	int64_t diff = ((actualGetCnt*(double)sampling_rate) - hist->totalCnt);
	hist->sdHist[0] = hist -> sdHist[0] + diff;
	// stack->totRef = stack->totRef + diff; //shards should only make change to the hist
	hist->totalCnt = hist->totalCnt + diff;

}

//MSR shards script
/*input: arg1: input file name
 *		 arg2: init rd
 *       arg3: final rd
 *       arg4: step rd
 */
int main(int argc, char const *argv[])
{	



	int TIME_FLAG=0;
	double tt_time =0;
	char buf[1024];


	char* input_format = "arg1: input filename\n \
arg2: output file(target dir + prefix, none = \"\")\n \
arg3: hist # bins\n \
arg4: hist bin size (for log version enter the base number)\n \
arg5: k (k-lru)\n \
arg6: shards_rate\n \
arg7: timer flag (1 yes 0 no)\n\
arg8: seed (optional)\n";

	if(argc < 8) { 
		printf("%s", input_format);
		return 0;
	}
	//open file for read key value
	FILE*       rfd = NULL;
	FILE*       wfd = NULL;
	FILE*       histfd = NULL;

	if(atoi(argv[7]) == 1) TIME_FLAG = 1;
	else if(atoi(argv[7]) == 0) TIME_FLAG = 0;
	else {
		printf("%s", input_format);
		exit(-1);
	}

	uint32_t seed = argc > 8 ? strtoul(argv[8], NULL, 10) : 0;
	uint32_t k = strtoul(argv[5], NULL, 10); 

	if((rfd = fopen(argv[1],"r")) == NULL)
	{ perror("open error for read"); return -1; }
	
	const char* file_prefix = argv[2];
	char* input_path = strdup(argv[1]);


	sprintf(buf, "%s%s_back-stack_%d-lru_R%s.mrc", argv[2], basename(input_path), k, argv[6]);
	if((wfd = fopen(buf ,"w")) == NULL)
	{ perror("open error for write"); return -1; }

	buf[0] = '\0';
	sprintf(buf, "%s%s_back-stack_%d-lru_R%s.hist", argv[2], basename(input_path), k, argv[6]);
	if((histfd = fopen(buf ,"w")) == NULL)
	{ perror("open error for write"); return -1; }
	



	struct timeval  tv1, tv2;
	gettimeofday(&tv1, NULL);

	//cache init
	KRR_Stack_t* stack = stackInit(k);
	Hist_t* hist = histInit(strtoul(argv[3], NULL, 10),atof(argv[4]));

	gettimeofday(&tv2, NULL);
	tt_time += (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);

	double stack_process_time = 0; 
	fixed_rate_spatial_sampling(rfd, 
								stack,
								(access_func)KRR_access,
								seed, 
								atof(argv[6]), 
								hist,
								&stack_process_time);

	
	tt_time += stack_process_time;

	gettimeofday(&tv1, NULL);

	solveMRC(hist);
	
	gettimeofday(&tv2, NULL);
	tt_time += (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);


	printfMRC(wfd, hist);
	printfHist(histfd, hist);
	
	if(TIME_FLAG == 1)
		fprintf(stdout, "back-KRR k= %s rate= %s file= %s TotalTime = %f seconds\n",argv[5],argv[6], basename(input_path) ,tt_time);

	stackFree(stack);
	histFree(hist);
	fclose(wfd);
	fclose(rfd);
	fclose(histfd);
	return 0;
}