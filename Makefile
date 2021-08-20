LIBIDIR := ./lib/inc
LIBSDIR := ./lib/src
SRCIDIR := ./inc
SRCSDIR := ./src
PCGDIR := ./lib/src/libpcg_src

CFLAGS := -I$(LIBIDIR) -I$(SRCIDIR)
CC := gcc

ifeq ($(DEBUG),yes)
 	OPTIMIZE_FLAG := -ggdb3 -DDEBUG
else ifeq ($(NOOP),yes)
 	OPTIMIZE_FLAG :=  
else
	OPTIMIZE_FLAG := -O2
endif
VAR := mult-ops-variable-KRR
UNI := mult-ops-uniform-KRR
LIBSRC := $(wildcard $(LIBSDIR)/*.c)
LIBOBJ := $(LIBSRC:$(LIBSDIR)/%.c=$(LIBSDIR)/%.o)
SRC := $(SRCSDIR)/KRR_mult_ops.c
OBJ := $(SRCSDIR)/KRR_mult_ops.o

PCGLIB := $(LIBSDIR)/libpcg_random.a


# all: CACHE

# CACHE: $(MAIN)
# 	$(CC) $(CFLAGS) $(MAIN) $(DOTO) -g -lm -o back_KRR_uniform

# VAR:
# 	$(CC) $(CFLAGS) -DVARSIZE $(MAIN) $(DOTO) -O2 -lm -o back_KRR_variable

# K_EXP: $(MAIN)
# 	$(CC) $(CFLAGS) -DK_EXP=$(K) $(MAIN) $(DOTO) -O2 -lm -o back_KRR_uniform_K$(K)

# VAR_K_EXP: $(MAIN)
# 	$(CC) $(CFLAGS) -DVARSIZE -DK_EXP=$(K) $(MAIN) $(DOTO) -O2 -lm -o back_KRR_variable_K$(K)

all: $(PCGDIR) $(VAR)

$(VAR): $(LIBOBJ) $(OBJ) $(SRCSDIR)/KRR_main.c 
	$(CC) $(CFLAGS) $(OPTIMIZE_FLAG) -DVARSIZE $^ $(PCGLIB) -lm  -o $@


$(OBJ): $(SRC)
	$(CC) $(CFLAGS) $(OPTIMIZE_FLAG) -c $< -o $@

$(LIBSDIR)/%.o: $(LIBSDIR)/%.c
	$(CC) $(CFLAGS) $(OPTIMIZE_FLAG) -c $< -o $@

$(PCGDIR):
	$(MAKE) -C $@




#KRR.o
#spatial.o pqueue.o murmur3.o utils.o


clean:
	rm -f $(LIBSDIR)/*.o $(LIBSDIR)/*.a $(SRCSDIR)/*.o
	$(MAKE) -C $(PCGDIR) clean



.PHONY: all $(PCGDIR)