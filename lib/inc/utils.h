#ifndef __JY_UTILS_H__
#define __JY_UTILS_H__


typedef struct progress_bar_t {
	char bar[28];
	uint64_t total;
	uint64_t cnt;
	uint64_t star;
	int barIndex;
} progress_bar_t;

#define PROGRESS_BAR_INIT(total, barPtr)
do {
	#ifdef NO_PROGRESS_BAR
		break;
	#endif
	*barPtr = malloc(sizeof(progress_bar_t));
	*barPtr->bar[0] = '[';
	*barPtr->bar[26] = ']';
	*barPtr->bar[27]= '\0';
	for (i=1; i<=25; i++) *barPtr->bar[i] = ' ';
	*barPtr->total = total;
	*barPtr->cnt = 0;
	*barPtr->star = total/100;
	*barPtr->barIndex = 1;
} while (0)

#define PROGRESS_BAR_UPDATE(bar)
do {
	#ifdef NO_PROGRESS_BAR
		break;
	#endif
	bar->cnt++;
	if (bar->cnt % star == 0) {
		if (bar->cnt%(bar->star*4) == 0){
			bar->bar[bar->barIndex++] = '#';
		}
		fprintf(stdout,"\rProgress: %s%d%% %ld", bar->bar, (int)(bar->cnt/(double)bar->total*100)+1, bar->cnt);
		if(bar->cnt == total) fprintf(stdout,"\n");
		fflush(stdout);
	}
} while(0)



uint32_t random_bits();


void jy_64_srandom();


uint64_t jy_64_random();

void jy_32_srandom();


uint64_t jy_32_random();

#endif //__JY_UTILS_H__



