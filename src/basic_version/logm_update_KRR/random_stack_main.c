#include <assert.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>


#include "random_stack_shards.c"


//MSR shards script
/*input: arg1: input file name
 *		 arg2: init rd
 *       arg3: final rd
 *       arg4: step rd
 */
int main(int argc, char const *argv[])
{
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

	fixed_rate_shards(rfd, seed, atof(argv[6]), hist,  stack );

	
	gettimeofday(&tv1, NULL);

	solveMRC(hist);
	
	gettimeofday(&tv2, NULL);
	tt_time += (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);


	printfMRC(wfd, hist);
	printfHist(histfd, hist);
	
	if(TIME_FLAG == 1)
		fprintf(stderr, "back-KRR k= %s rate= %s file= %s TotalTime = %f seconds\n",argv[5],argv[6], basename(input_path) ,tt_time);

	stackFree(stack);
	histFree(hist);
	fclose(wfd);
	fclose(rfd);
	fclose(histfd);
	return 0;
}