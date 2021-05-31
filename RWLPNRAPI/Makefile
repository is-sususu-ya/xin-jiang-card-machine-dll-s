SRC:=$(wildcard *.c)
SRC+=../utils/utils_net.c ../utils/longtime.c
OBJ=$(SRC:%.c=%.o)
LIBS_FLAGS:=-lpthread -lrt -lm
APP_FLAGS:= -g -Wall -fPIC -I../utils/
TARGET:=
DST:=
CC=gcc
TARGET:=libRWLPNRAPI.so
LIBS_FLAGS+=-shared

all: $(TARGET)
$(TARGET) : $(OBJ)
	$(CC) $(LIBS_FLAGS) -o $@ $^

$(OBJ) :%.o:%.c
	$(CC) $(APP_FLAGS) -c $^ -o $@

cc:
	rm $(OBJ)
