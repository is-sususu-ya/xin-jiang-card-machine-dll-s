
include ../Makefile.param
#ifeq ($(SAMPLE_PARAM_FILE), )
#     SAMPLE_PARAM_FILE:=../Makefile.param
#     include $(SAMPLE_PARAM_FILE)
#endif
.PHONY: all lib clean

SRC:=$(wildcard *.c)
SRC+=$(wildcard ./mqtt/*.c)
SRC+=$(wildcard ../utils/*.c)
APP=special
OTHREDEP=$(CURL_DEP)
OBJ:=$(SRC:%.c=%.o)
 
 
all: $(APP)
$(APP) : $(OBJ)
	$(CC) -o $@  $^ $(SYS_FLAGS) $(CLANE_LFLAGS) -ldl $(OTHREDEP)

$(OBJ) : %.o:%.c
	$(CC) $(INCLUDE_PATH) -D$(BOARD_TYPE) $(CFLAGS) -I. -I./mqtt -c $^ -o $@

clean:
	rm -f $(OBJ)
	rm -f tags
