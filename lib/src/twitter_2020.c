
//********************************
// @Junyao Yang 12/25/2020
// methods for sanitize twitter traces
//**********************************
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "twitter_2020.h"
#include "murmur3.h"


void tw_trace_load(tw_iterator_t* itr) {

	ssize_t read;
	char* line = NULL;
	size_t len = 0;
	char* token;
	unsigned long i = 0;
	tw_ref_t* ref = NULL;
	while ( (i < itr->max_num) 
				&& 
			((read = getline(&line, &len, itr->rfd)) != -1)) {

		ref = itr->trace_buf+i;
		if (ref == NULL) {
			printf("nullll\n"); exit(-1);
		}
		token = strtok(line, ","); //remove newline from the end
		ref->timestamp = strtoul(token, NULL, 10);
		token = strtok(NULL, ",");
		free((void*)(ref->raw_key));
		ref->raw_key = strdup(token); //dup the key
		token = strtok(NULL, ",");
		ref->key_size = strtoul(token, NULL, 10);
		token = strtok(NULL, ",");
		ref->val_size = strtoul(token, NULL, 10);
		token = strtok(NULL, ",");
		ref->cli_id = strtoul(token, NULL, 10);
		token = strtok(NULL, ",");
		free((void*)(ref->op));
		ref->op = strdup(token);
		token = strtok(NULL, "\n");
		ref->ttl = strtoul(token, NULL, 10);

		MurmurHash3_x64_128(ref->raw_key,
							strlen(ref->raw_key),
							SEED,
							ref->murmur3_hashed_key);
		i++;
	}

	itr->max_num = i;
	itr->curr_num = 0;

	free((void*)line);
}


//this will return number of num specified by input
//if file contain less than num, returned max_num will be that num
tw_iterator_t* tw_trace_init (char* fileName, unsigned long num, int flag) {

	//open trace file

	//malloc iterator
	tw_iterator_t* itr = malloc(sizeof(tw_iterator_t));
	itr->curr_num = 0;
	itr->max_num = num;

	itr->flag = flag;
	FILE* rfd = fopen(fileName, "r");
	if (rfd == NULL) {
		free((void*)itr);
		perror("trace file open failed");
		exit(-1);
	}	

	itr->rfd = rfd;

	//read it into buf
	itr->trace_buf = malloc(sizeof(tw_ref_t)*num);
	for (unsigned long i = 0; i < itr->max_num; i++) {
		itr->trace_buf[i].raw_key = NULL;
		itr->trace_buf[i].op = NULL;
	}
	tw_trace_load(itr);

	if (flag == ONETIME) fclose(rfd);
	//return itr

	return itr;
}

//return next access
//return NULL if max_num reached
tw_ref_t* tw_trace_next(tw_iterator_t* itr) {
	unsigned long num = itr->curr_num;
	itr->curr_num++;
	if (tw_trace_finished(itr) && itr->flag == CONTINUE) {
		tw_trace_load(itr);

	}
	return itr->trace_buf+num;
}

int tw_trace_finished(tw_iterator_t* itr) {
	return itr->max_num == itr->curr_num;
}

void tw_trace_cleanUp(tw_iterator_t* itr) {
	unsigned long i;
	for (i = 0; i < itr->max_num; i++) {
		free((void*)(itr->trace_buf[i].raw_key));
		free((void*)(itr->trace_buf[i].op));
	}
	free((void*)(itr->trace_buf));
	free((void*)itr);
	if(itr->flag == CONTINUE) fclose(itr->rfd);
}


