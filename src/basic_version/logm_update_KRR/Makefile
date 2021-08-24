IDIR = ../../../inc
CFLAGS=-I$(IDIR) 
CC = gcc
MAIN = random_stack_main.c
#SRC = ./src/hist.c ./src/priority.c ./src/RankCache.c 
DOTO = $(IDIR)/murmur3.o $(IDIR)/entropy.o $(IDIR)/libpcg_random.a


all: CACHE

CACHE: $(MAIN)
	$(CC) $(CFLAGS) $(MAIN) $(DOTO) -O2 -lm -o back_KRR_uniform

VAR:
	$(CC) $(CFLAGS) -DVARSIZE $(MAIN) $(DOTO) -O2 -lm -o back_KRR_variable

K_EXP: $(MAIN)
	$(CC) $(CFLAGS) -DK_EXP=$(K) $(MAIN) $(DOTO) -O2 -lm -o back_KRR_uniform_K$(K)

VAR_K_EXP: $(MAIN)
	$(CC) $(CFLAGS) -DVARSIZE -DK_EXP=$(K) $(MAIN) $(DOTO) -O2 -lm -o back_KRR_variable_K$(K)


clean:
	rm -f back_KRR*




