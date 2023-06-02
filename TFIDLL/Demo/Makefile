#include ../Makefile.param
#ifeq ($(SAMPLE_PARAM_FILE), )
#     SAMPLE_PARAM_FILE:=../Makefile.param
#     include $(SAMPLE_PARAM_FILE)
#endif
.PHONY: all lib clean

LIBSRC:=$(wildcard *.c)
LIBOBJ:=$(LIBSRC:%.c=%.o)
LIBS_FLAGS:=
APP_FLAGS:= -g -Wall -fPIC -I./include

LIB_RELY:= -L$(LANE_LIB_PATH)
CC=gcc

APP:=getqr

all: $(APP)
$(APP) :  $(LIBOBJ) 
	$(CC) $(LIBS_FLAGS) -o $@ $^ -L. -lTFI -lpthread -lrt

$(LIBOBJ) : %.o:%.c
	$(CC) $(APP_FLAGS) $(CLANE_FLAGS) -c $^ $(LIB_FLAGS)  -o $@

cc:
	@rm  -f $(LIBOBJ)
	rm -f $(APPOBJ)
	rm -f $(APP)
	rm -f $(MLIB)
	rm -f tags
