#ifndef __JY_UTILS_H__
#define __JY_UTILS_H__

#define PROGRESS_BAR
typedef struct progress_bar_t {
    char bar[28];
    uint64_t total;
    uint64_t cnt;
    uint64_t star;
    int barIndex;
} progress_bar_t;



#ifdef PROGRESS_BAR

#define PROGRESS_BAR_INIT(total,barPtr)                                         \
do {                                                                            \
    *barPtr = malloc(sizeof(progress_bar_t));                                   \
    *barPtr->bar[0] = '[';                                                      \
    *barPtr->bar[26] = ']';                                                     \
    *barPtr->bar[27]= '\0';                                                     \
    for (int i=1; i<=25; i++) *barPtr->bar[i] = ' ';                                \
    *barPtr->total = total;                                                     \
    *barPtr->cnt = 0;                                                           \
    *barPtr->star = total/100;                                                  \
    *barPtr->barIndex = 1;                                                      \
} while (0)

#else 
#define PROGRESS_BAR_INIT(total,barPtr)
#endif 

#ifdef PROGRESS_BAR

#define PROGRESS_BAR_UPDATE(bar)                                                \
do {                                                                            \
    bar->cnt++;                                                                 \
    if (bar->cnt % bar->star == 0) {                                                 \
        if (bar->cnt%(bar->star*4) == 0){                                       \
            bar->bar[bar->barIndex++] = '#';                                    \
        }                                                                       \
        fprintf(stdout,"\rProgress: %s%d%% %ld",                                \
            bar->bar, (int)(bar->cnt/(double)bar->total*100)+1, bar->cnt);      \
        fflush(stdout);                                                         \
    }                                                                           \
     if(bar->cnt == total) fprintf(stdout,"\n");                                \
} while(0)

#else
#define PROGRESS_BAR_UPDATE(bar)
#endif


#define COUNT_FILE_LINE(fileName,totalPtr)                                      \
do {                                                                            \
    FILE* rfd;                                                                  \
    if((rfd = fopen(fileName,"r")) == NULL)                                     \
    { perror("COUNT_FILE_LINE: open error for read"); exit(-1); }               \
    *totalPtr = 0;                                                              \
    ssize_t read;                                                               \
    char* line = NULL;                                                          \
    size_t len = 0;                                                             \
    while ((read = getline(&line, &len, rfd)) != -1) {                          \
        (*totalPtr)++;                                                          \
    }                                                                           \
    free(line);                                                                 \
    fclose(rfd);                                                                \
} while(0)


uint32_t random_bits();


void jy_64_srandom();


uint64_t jy_64_random();

void jy_32_srandom();


uint64_t jy_32_random();

#endif //__JY_UTILS_H__



