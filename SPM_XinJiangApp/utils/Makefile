
include ../Makefile.param
#ifeq ($(SAMPLE_PARAM_FILE), )
#     SAMPLE_PARAM_FILE:=../Makefile.param
#     include $(SAMPLE_PARAM_FILE)
#endif
.PHONY: all lib clean

LIBSRC:=$(wildcard *.c)
LIBOBJ:=$(LIBSRC:%.c=%.o)
APP_FLAGS:= -lpthread -lrt -lm
COMPLIER_FLAGS:= -g -Wall -fPIC 


# this will be set for each module
MLIB:=libutils.so

lib: $(MLIB)
$(MLIB) : $(LIBOBJ) $(COMOBJ)
	$(CC) -shared -o $@  $^ $(APP_FLAGS)

$(LIBOBJ) : %.o:%.c
	$(CC) $(COMPLIER_FLAGS) -c $^

clean:
	rm -f $(COMOBJ)
	rm -f $(LIBOBJ)
	rm -f tags
