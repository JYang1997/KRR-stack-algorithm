#include <stdint.h>
#include <stdlib.h>
#include "pcg_variants.h"
#include "entropy.h"
#include "utils.h"



uint32_t random_bits() {
	uint32_t seed = 0;
	int i = 0;
	for (i = 0; i < 32; i++)
		if (pcg64_random()%100 > 50)
			seed = seed | (1 << i);

	return seed;
}



void jy_64_srandom() {
	volatile uint64_t seeds[2];
    entropy_getbytes((void*)seeds, sizeof(seeds));
    pcg64_srandom(seeds[0], seeds[1]);
}



uint64_t jy_64_random() {
	return pcg64_random();
}

void jy_32_srandom() {
	volatile uint64_t seeds[2];
    entropy_getbytes((void*)seeds, sizeof(seeds));
    pcg32_srandom(seeds[0], seeds[1]);
}



uint64_t jy_32_random() {
	return pcg32_random();
}
