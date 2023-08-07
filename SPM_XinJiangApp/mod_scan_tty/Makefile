
include ../Makefile.param
#ifeq ($(SAMPLE_PARAM_FILE), )
#     SAMPLE_PARAM_FILE:=../Makefile.param
#     include $(SAMPLE_PARAM_FILE)
#endif
.PHONY: all lib clean

LIBSRC:=$(wildcard *.c)
LIBOBJ:=$(LIBSRC:%.c=%.o)
APP_FLAGS:= -lpthread -lrt -lm

# this will be set for each module
MLIB:=mod_qr_tty.so

lib: $(MLIB)
$(MLIB) : $(LIBOBJ)
	$(CC) -shared -o $@  $^ $(SYS_FLAGS) $(CLANE_LFLAGS)

$(LIBOBJ) : %.o:%.c
	$(CC) $(INCLUDE_PATH) -c $^ -o $@

clean:
	rm -f $(LIBOBJ)
	rm -f $(MLIB)
	rm -f tags
