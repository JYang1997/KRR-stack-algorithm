#ifndef __SPATIAL_SAMPLING_H__
#define __SPATIAL_SAMPLING_H__



typedef int64_t (*access_func)(void*, uint64_t, uint32_t, char*);

/**
 * 
 * fixed rate spatial sampling from SHARDS 
 * 
 * @param rfd trace file descriptor.
 * 		the file should be in following format
 *      "key,valueSize,command"
 * 		if valueSize and command are not provided
 * 		this will simply assume it is default value and command
 * @param stack pointer of a stack struct
 * 
 * 
 * 
 * 
 * 
 * 
 **/

void fixed_rate_spatial_sampling (FILE* 	rfd,
								  void* 	stack,
								  access_func access,
							 	  uint32_t 	seed,
								  float 	sampling_rate,
								  Hist_t* 	hist,
								  double* 	timePtr);

void tw_fixed_rate_spatial_sampling(char* fileName,
                                  void*     stack,
                                  access_func access,
                                  uint32_t  seed,
                                  float     sampling_rate,
                                  Hist_t*   hist,
                                  double*   timePtr);
#endif //__SPATIAL_SAMPLING_H__