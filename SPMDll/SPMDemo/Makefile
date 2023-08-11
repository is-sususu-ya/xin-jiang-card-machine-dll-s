CC=gcc
LFLAGS=-lpthread -lm -lrt
CFLAGS:=-g -I. 
AIM=spmDemo
SRC=$(wildcard ./*.c)
OBJ=$(SRC:%.c=%.o) 
 
all : $(AIM)
$(AIM) : $(OBJ)
	$(CC) -o $@ $^ $(LFLAGS) -ldl
	@cp -vf $@ /nfsroot/
	
$(OBJ) : %.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@
			
.PHONY : clean
cc:
	rm -f $(OBJ)
	rm -f $(AIM)
