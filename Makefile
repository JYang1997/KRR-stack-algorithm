LIBIDIR = ./lib/inc/
LIBSDIR = ./lib/src/
SRCIDIR = ./inc/
IDIR1 = ../../../inc/libpqueue/src/
CFLAGS=-I$(IDIR) -I$(IDIR1)
CC = gcc
MAIN = random_stack_main.c 
#SRC = ./src/hist.c ./src/priority.c ./src/RankCache.c 
DOTO = $(LIBSDIR)/murmur3.o $(LIBSDIR)/entropy.o $(LIBSDIR)/pqueue.o $(LIBSDIR)/libpcg_random.a


all: CACHE

CACHE: $(MAIN)
	$(CC) $(CFLAGS) $(MAIN) $(DOTO) -g -lm -o back_KRR_uniform

VAR:
	$(CC) $(CFLAGS) -DVARSIZE $(MAIN) $(DOTO) -O2 -lm -o back_KRR_variable

K_EXP: $(MAIN)
	$(CC) $(CFLAGS) -DK_EXP=$(K) $(MAIN) $(DOTO) -O2 -lm -o back_KRR_uniform_K$(K)

VAR_K_EXP: $(MAIN)
	$(CC) $(CFLAGS) -DVARSIZE -DK_EXP=$(K) $(MAIN) $(DOTO) -O2 -lm -o back_KRR_variable_K$(K)




#KRR.o
#spatial.o pqueue.o murmur3.o utils.o

KRR:


UTIL:
	$(CC) -c $(LIBSDIR)utils.c


MURMUR3:

SPATIAL:

PQUEUE:




clean:
	rm -f back_KRR*
