#include "random_stack.h"
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include "murmur3.h"
#include "pcg_variants.h"
#include "entropy.h"
#include <math.h>


void stackUpdate(KRR_Stack_t* stack, Item_t* item);
Item_t* createItem(KRR_Stack_t* stack, uint64_t key, int32_t size);
Item_t* findItem(KRR_Stack_t* stack, uint64_t key);
int64_t stackDistance(KRR_Stack_t* stack, Item_t* item);
void stackResize(KRR_Stack_t* stack);
void* arrayResize(void* arry, size_t item_size, uint64_t *curr_cap); 


#ifdef VARSIZE

/**
 * @param stack
 * @param st_loc stack position, (st_loc >= 1)
 * @return fine-grained stack distance 
 **/
int64_t variableSizeDistance(KRR_Stack_t* stack, int64_t st_loc) {

	int64_t sd;
	//p = 0, when st_loc = 1
	uint32_t p = LOGA(st_loc, SIZE_BASE); //find the highest log that is at most st_loc
	
	// int64_t ave_size = stack->totSize_array[p]/pow(SIZE_BASE, p);
	// sd = st_loc*ave_size;

	int64_t exact_num = pow(SIZE_BASE, p);
	int64_t second_portion = 0;
	if (st_loc > exact_num) {
		second_portion = (stack->totSize_array[p+1]-stack->totSize_array[p])
							*((st_loc-exact_num)/(double)(pow(SIZE_BASE, p+1)-exact_num)); 
	}

	sd = stack->totSize_array[p] + second_portion;
	return sd;
}


/**
 * @param low: the smaller position of the adjacent swap positions
 * @param high: the higher position of the adjacent swap positions
 * @param delta: the net size flow from these two positions
 * position >= 1 
 **/
static void updateTotSizeArray(KRR_Stack_t* stack, uint64_t low, uint64_t high, int64_t delta) {

	if (delta == 0) return;
	//sn_index is the node that stores the total size up till the expbase^(low)
	//the update does not include sn_upbound because the net size flow out of high position is zero
	uint32_t sn_index = ceil(LOGA(low, SIZE_BASE));
 	uint32_t sn_upbound = ceil(LOGA(high, SIZE_BASE));
 	while(sn_index < sn_upbound) {

 		stack->totSize_array[sn_index] += delta;
 		sn_index++;
 	}

}

#endif



/**
 * @param k, sampling size K
 * @return  KRR_stack
 **/
KRR_Stack_t* stackInit(uint32_t k) {
	KRR_Stack_t* stack = malloc(sizeof(KRR_Stack_t));
	
	
	stack->totalGet = 0;
	stack->totalSet = 0;
	stack->totalDel = 0;
	stack->totalMod = 0;
	stack->totalIns = 0;
	
	stack->totalKey = 0;
	stack->k = pow(k,K_EXP);
	stack->capacity = INITIAL_CAPACITY;

	stack->item_array = malloc((stack->capacity)*sizeof(Item_t*));
	stack->HashItem = NULL; //hash table pointer

	stack->holder_heap = pqueue_init(512, cmp_pri, get_pri, set_pri, get_pos, set_pos);


	#ifdef VARSIZE

	stack->tsLen = ((uint32_t)ceil(LOGA(stack->capacity, SIZE_BASE)));
	stack->totSize_array = malloc(stack->tsLen*sizeof(int64_t));
	stack->totalSize = 0;
	memset(stack->totSize_array, 0, sizeof(int64_t)*stack->tsLen);
	
	#endif

	return stack;

}


/**
 * @param stack
 * @return void 
 **/
void stackFree(KRR_Stack_t* stack) {

	HASH_CLEAR(hh,stack->HashItem);

	for (int64_t i=0; i<stack->totalKey; i++){
		free((void*)(stack->item_array[i])); 
	}

	free((void*)stack->item_array);

	#ifdef VARSIZE
	free((void*)stack->totSize_array);
	#endif

	pqueue_free(stack->holder_heap);

	free(stack);
}

//helper for stackResize
//increase the array size by *2
void* arrayResize(void* arry, size_t item_size, uint64_t* curr_cap) {
	uint32_t new_cap = *curr_cap;
	*curr_cap = *curr_cap*2;
		
	void* temp = realloc(arry, (*curr_cap)*item_size);
	if(temp == NULL){
		perror("realloc error arrayresize!");
		exit(-1);
	}else{
		arry = temp;
	}
	return arry; 
}


//stack resize for non-fixed size stack
void stackResize(KRR_Stack_t* stack)
{

	uint64_t prev_cap = stack->capacity;
	if (stack->totalKey >= stack->capacity-1) {
		stack->item_array =(Item_t**)arrayResize(stack->item_array, sizeof(Item_t*), &(stack->capacity));

		#ifdef VARSIZE
		int newLen = ceil(LOGA(stack->capacity,SIZE_BASE));
		if(newLen > stack->tsLen) {
			stack->totSize_array = realloc(stack->totSize_array, (newLen)*sizeof(int64_t));
			
			memset(stack->totSize_array+stack->tsLen, 0, sizeof(int64_t)*(newLen-stack->tsLen));
			updateTotSizeArray(stack, prev_cap, stack->capacity, stack->totalSize);
			stack->tsLen = newLen;
		}
		#endif
	}
}



/**
 * @param stack
 * @param key
 * @param size
 * @return new item
 * 1.init new item, and store the item to hash table
 * 2.the item's pointer is also assigned to the last position of the stack array
 **/
//create new item, attach it to the stack
//insert follow the case where address first time been referenced
Item_t* createItem(KRR_Stack_t* stack, uint64_t key, int32_t size) {

	Item_t* n_item = malloc(sizeof(Item_t));

	n_item->addrKey = key;

	n_item->index = stack->totalKey;

	stack->totalKey++;
	
	#ifdef VARSIZE
	stack->totalSize += size;
	n_item->size = size;
	#endif

	stack->item_array[n_item->index] = n_item;
	
	HASH_ADD(hh, stack->HashItem, addrKey, sizeof(uint64_t), n_item);

	return n_item;
}


/**
 * @param stack
 * @param item -Item_t
 * @return stack distance
 * 			stack distance is equvalent to item's index+1 for uniform mode
 * 			under variable size mode, fine grained sd is returned
 **/
int64_t stackDistance(KRR_Stack_t* stack, Item_t* item) {
	int64_t sd; 

	if (item == NULL ) {
		return COLDMISS;
	}

	sd = item->index+1;
	
	#ifdef VARSIZE
	sd = variableSizeDistance(stack, sd);
	#endif
	
	assert(sd > 0);

	return sd;
}


/**
 * @param stack
 * @param key
 * @return item Item_t
 **/
Item_t* findItem (KRR_Stack_t* stack, uint64_t key) {

	Item_t* t_item = NULL;
	HASH_FIND(hh, stack->HashItem, &key, sizeof(uint64_t), t_item);

	return t_item;
}



//to add delete operation
//		1. check smallest delete position
//      2. calculate swap position, start swap when smaller than smallest delete position
//      3. if no delete node or first delete node is larger than sd
//      4. then perform normal swaps
//      5. if delete node is in the interval
//      6.    then perform swaps, but do not actually swap,
//      7.    wait until current position smaller or equal to delete position
//      8.    then change sd position to a delete node
//      9.    then check wether sd position is the last position
//      10.   if it is last position, delete it and check upward delete all adjacent delete node.
void stackUpdate(KRR_Stack_t* stack, Item_t* item) {



	uint64_t j = item->index;
	uint64_t ddddd = j;


	double x;
	uint64_t tx;

	double exp = 1.0/(stack->k); 

		
	node_t* first_del = pqueue_peek(stack->holder_heap);

	

	Item_t* del_item = (first_del == NULL) ? NULL : stack->item_array[first_del->pri];

	int32_t delta1, delta2;
	uint64_t mid_pos;

	while (j > 0){
		x = ldexp(pcg64_random(), -64); //PRNG [0,1)
		
	 	x = pow(x, exp)*(j-1); //inverse function, pick a index from 0 to j-1
	 	x = round(x);
	 	tx = x;
	 	#ifdef VARSIZE
	 	delta1 = (item->size)-(stack->item_array[tx]->size);
	 	delta2 = 0;
	 	#endif
	 	mid_pos = j;

	 	if (first_del != NULL && tx < first_del->pri && j >= first_del->pri) {
	 		#ifdef VARSIZE
 			delta2 = (item->size)-(del_item->size);
 			#endif
 			mid_pos = first_del->pri;	

		} else if (first_del != NULL && first_del->pri < item->index 
					&& tx >= first_del->pri){
	 		#ifdef VARSIZE
	 		delta1 = 0;
	 		delta2 = (item->size)-(del_item->size);
	 		#endif
	 		mid_pos = tx;
		}
		
		stack->item_array[mid_pos] = stack->item_array[tx];
		stack->item_array[mid_pos]->index = mid_pos;
		
		#ifdef VARSIZE	 					
		updateTotSizeArray(stack, tx+1, mid_pos+1, delta1);
		updateTotSizeArray(stack, mid_pos+1, j+1, delta2);
		#endif
		j=x;
	
	}


	if (first_del != NULL && first_del->pri < item->index) {
		
		stack->item_array[item->index] = del_item;
		stack->item_array[item->index]->index = item->index;

		pqueue_remove_bykey(stack->holder_heap, first_del->pri);
		pqueue_insert_bykey(stack->holder_heap, item->index);
			
	}


	//check item's position if it is the last one, then check
	//wether last one is deleted node, if it is then remove it
	//and repeatedly check adjacent position, until it is not deleted node
	while (stack->totalKey-1 >= 0 
			&& stack->item_array[stack->totalKey-1]->addrKey == INVALID) {
		
		#ifdef VARSIZE
		int32_t del_size = stack->item_array[stack->totalKey-1]->size;

		updateTotSizeArray(stack, stack->totalKey, stack->capacity, 0-del_size);		
	
		stack->totalSize -= del_size;
	
		#endif

		pqueue_remove_bykey(stack->holder_heap, stack->totalKey-1);

		free(stack->item_array[stack->totalKey-1]);
		stack->item_array[stack->totalKey-1] = NULL;

		stack->totalKey--;

	}

	//replace the first pointer with referenced item, full cyclic swap complete
	stack->item_array[0] = item;
	stack->item_array[0]->index = 0;

	stackResize(stack);//stackResize for non-fixed size stack

}


//return stack distance
int64_t KRR_GET(KRR_Stack_t* 	stack,
				uint64_t 		key,
				int32_t 		size) {

	stack->totalIns++;
	stack->totalGet++;

	long long sd = COLDMISS;
	Item_t* item = findItem(stack, key);

	if (item == NULL) {
		item = createItem(stack, key, size);
		#ifdef VARSIZE	//special case when item is newly add to the stack
					//need to update all partial sum node that is beyond item's index.
		updateTotSizeArray(stack, item->index+1, stack->capacity, item->size);
	
		#endif
	} else {
		sd = stackDistance(stack, item);
	}

	stackUpdate(stack, item);

	return sd;
}


// SET operation
// if the item is new
// then set operation acts exactly like a GET operation
// except that set does not report stack distance
// if the item is not new,
//    1. for uniform mode, still act like GET operation
//    2. for varSize mode, 
//      1. if item size is not same as previous
//         we need to adjust tail stack sizeNodes

void KRR_SET(KRR_Stack_t* 	stack,
			 uint64_t 		key,
			 int32_t 		size) {
	stack->totalIns++;
	stack->totalSet++;

	Item_t* item = findItem(stack, key);

	if (item == NULL) {
		item = createItem(stack, key, size);
		#ifdef VARSIZE	
		updateTotSizeArray(stack, item->index+1, stack->capacity, item->size);
		#endif
	} else {
		
		#ifdef VARSIZE	
		if (item->size != size) {
			updateTotSizeArray(stack, item->index+1, stack->capacity, size-(item->size));
	
			item->size = size; //change item's size to new size
							   //the stackupdate will carry by new size
		}
		#endif
	}

	stackUpdate(stack, item);


}


//if key is not in the stack ignore it
//if key is in the stack, place a holder
void KRR_DELETE(KRR_Stack_t* stack, uint64_t key) {
	stack->totalIns++;
	stack->totalDel++;

	Item_t* item = findItem(stack, key);
	if (item == NULL) return;

	//remove item from hash table so that on next reference
	//it would be a miss
	HASH_DELETE(hh, stack->HashItem, item);

	item->addrKey = INVALID; //erase the key,

	pqueue_insert_bykey(stack->holder_heap, item->index);

	while (stack->totalKey >= 1 && 
				stack->item_array[stack->totalKey-1]->addrKey == INVALID ) {
		
		#ifdef VARSIZE
		int32_t del_size = stack->item_array[stack->totalKey-1]->size;
	
		updateTotSizeArray(stack, stack->totalKey, stack->capacity, 0-del_size);
		stack->totalSize -= del_size;
		#endif

		pqueue_remove_bykey(stack->holder_heap, stack->totalKey-1);

		
		free(stack->item_array[stack->totalKey-1]);
		stack->item_array[stack->totalKey-1] = NULL;

		stack->totalKey--;

	}

}


//equivalent to stack_set
void KRR_UPDATE(KRR_Stack_t* 	stack,
			   	uint64_t 		key,
			    int32_t 		size) {
	stack->totalIns++;
	stack->totalMod++;

	KRR_SET(stack, key, size);
}





/*******************************************
 * 
 * 
 *
 *******************************************/
int64_t KRR_access(KRR_Stack_t*		stack, 
				   uint64_t 		key, 
				   int32_t 			size, 
				   char*			commandStr) {

	long long sd = INVALID;

	if (strcmp(commandStr, "GET") == 0) {
		sd = KRR_GET(stack, key, size);
	} else if (strcmp(commandStr, "SET") == 0) {
		KRR_SET(stack, key, size);
	} else if (strcmp(commandStr, "DELETE") == 0) {
		KRR_DELETE(stack, key);
	} else if (strcmp(commandStr, "UPDATE") == 0) {
		KRR_UPDATE(stack, key, size);
	} else {
		fprintf(stderr,"Stack instrution number: %ld, Invalid command %s\n",
			stack->totalIns, commandStr);
		exit(-1);
	}

	return sd;
	
}
