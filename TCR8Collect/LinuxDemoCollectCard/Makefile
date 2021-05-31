
LFLAGS=-lpthread -lm -lrt
CFLAGS:=-g -I. 
AIM=demo
SRC=$(wildcard ./*.c)
OBJ=$(SRC:%.c=%.o) 
AR=arm-hisiv100nptl-linux-ar
CC=arm-hisiv100nptl-linux-gcc
 
all : $(AIM)
$(AIM) : $(OBJ)
	$(CC) -o $@ $^ $(LFLAGS)
	@cp -vf $@ /nfsroot/work
	
$(OBJ) : %.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@
			
.PHONY : clean
cc:
	rm -f $(OBJ)
	rm -f $(AIM)
