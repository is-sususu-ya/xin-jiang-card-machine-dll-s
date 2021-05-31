
LFLAGS=-lpthread -lm -lrt
CFLAGS:=-g -I. -DENABLE_TESTCODE -I. -I../utils

AIM=libTCR8ACM.so
SRC=$(wildcard ./*.c)
SRC+=../utils/utils_net.c ../utils/utils_tty.c ../TCR8lib/TCR8lib.c
OBJ=$(SRC:%.c=%.o) 
AR=arm-hisiv100nptl-linux-ar
CC=arm-hisiv100nptl-linux-gcc
 
all : $(AIM)
$(AIM) : $(OBJ)
	$(CC) -shared -o $@ $^ $(LFLAGS)
	$(AR) rv libTCR8.a  $^ 
	@cp -vf ./libTCRACM.so /nfsroot/work 
	
$(OBJ) : %.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@
			
.PHONY : clean
cc:
	rm -f $(OBJ)
	rm -f $(AIM)
