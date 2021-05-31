
LFLAGS=-lpthread -lm -lrt
CFLAGS:=-g -I. 
AIM=demo
SRC=$(wildcard ./*.c)
OBJ=$(SRC:%.c=%.o) 
CC=gcc
 
all : $(AIM)
$(AIM) : $(OBJ)
	$(CC) -o $@ $^ $(LFLAGS) -ldl
	@cp -vf $@ /nfsroot/work
	
$(OBJ) : %.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@
			
.PHONY : clean
cc:
	rm -f $(OBJ)
	rm -f $(AIM)
