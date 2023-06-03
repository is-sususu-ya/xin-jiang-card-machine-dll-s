include ../Makefile.param

LFLAGS=-lpthread -lm -lrt
CFLAGS:=-g -I. -DENABLE_TESTCODE -I. -I../utils -fPIC

AIM=libTCR8ACC$(TAIL).so
SRC=$(wildcard ./*.c)
SRC+=../utils/utils_net.c ../utils/utils_tty.c ../TCR8lib/TCR8lib.c
OBJ=$(SRC:%.c=%.o) 
 
all : $(AIM)
$(AIM) : $(OBJ)
	$(CC) -shared -o $@ $^ $(LFLAGS)

$(OBJ) : %.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@
			
.PHONY : clean
cc:
	rm -f $(OBJ)
	rm -f $(AIM)
