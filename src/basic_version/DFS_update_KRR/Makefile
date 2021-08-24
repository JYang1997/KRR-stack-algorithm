IDIR = ../../../inc
CFLAGS=-I$(IDIR) 
CC = gcc
MAIN = random_stack_main.c
#SRC = ./src/hist.c ./src/priority.c ./src/RankCache.c 
DOTO = $(IDIR)/murmur3.o $(IDIR)/entropy.o $(IDIR)/libpcg_random.a


all: CACHE

CACHE: $(MAIN)
	$(CC) $(CFLAGS) $(MAIN) $(DOTO) -lm -o dfs_KRR

K_EXP: $(MAIN)
	$(CC) $(CFLAGS) -DK_EXP=$(K) $(MAIN) $(DOTO) -lm -o dfs_KRR
clean:
	rm -f dfs_KRR



