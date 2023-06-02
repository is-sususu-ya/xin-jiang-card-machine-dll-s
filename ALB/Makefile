SRC:=$(wildcard *.c)
SRC+=../utils/utils_net.c ../utils/longtime.c ../utils/utils_mtrace.c ../utils/utils_tty.c
OBJ=$(SRC:%.c=%.o)
LIBS_FLAGS:=-lpthread -lrt -lm
APP_FLAGS:= -g -Wall -fPIC -I../utils/
TARGET:=
DST:=
CC=gcc
TARGET:=libAlb.so
LIBS_FLAGS+=-shared

all: $(TARGET)
$(TARGET) : $(OBJ)
	$(CC) -shared -o $@ $^ $(LIBS_FLAGS)


$(OBJ) :%.o:%.c
	$(CC) $(APP_FLAGS) -c $^ -o $@

cc:
	rm $(OBJ)
