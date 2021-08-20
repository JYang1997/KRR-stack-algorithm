#ifndef JY_TW_2020_H
#define JY_TW_2020_H
//junyaoy 12/27/2020

#define SEED 429 // seed for murmur3
#define CONTINUE 20010205
#define ONETIME 50201002

// enum Op_t { 
// 	get=0, 
// 	gets=1, 
// 	set=2, 
// 	add=3, 
// 	replace=4, 
// 	cas=5, 
// 	append=6, 
// 	prepend=7, 
// 	delete=8, 
// 	incr=9, 
// 	decr=10
// 	};



typedef struct _tw_ref_t {
	unsigned int timestamp; // in sec
	char* raw_key; // anonymized key
	unsigned int key_size; // in bytes
	unsigned int val_size; // inbytes
	unsigned int cli_id; // client id, represent who sends the request
	char* op;
	unsigned int ttl; //the time-to-live i.e. expiration time

	unsigned long long murmur3_hashed_key[2]; //128 bits hashed key

} tw_ref_t;

typedef struct _tw_iterator_t { 
	tw_ref_t* trace_buf;
	FILE* rfd;
	int flag; //continue means continue until end of file
	unsigned long max_num;
	unsigned long curr_num;
} tw_iterator_t;

tw_iterator_t* tw_trace_init (char* fileName, unsigned long num, int flag);
tw_ref_t* tw_trace_next(tw_iterator_t* itr);
int tw_trace_finished(tw_iterator_t* itr);
void tw_trace_cleanUp(tw_iterator_t* itr);

#endif