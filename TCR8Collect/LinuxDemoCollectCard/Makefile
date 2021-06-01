include ../../Makefile.param
LFLAGS=-lpthread -lm -lrt
CFLAGS:=-g -I. 
AIM=cdemo
SRC=$(wildcard ./*.c)
OBJ=$(SRC:%.c=%.o) 
 
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
