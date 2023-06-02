
# 
.PHONY: all lib clean

LIBSRC:=$(wildcard *.c)
LIBSRC+=$(wildcard ../utils/*.c) 
LIBOBJ:=$(LIBSRC:%.c=%.o) 

APP_FLAGS:= -g -Wall -fPIC -I../utils -I./
 
CC=gcc
#CC=arm-hisiv400-linux-gcc
#CC=arm-hisiv100nptl-linux-gcc
#CC=arm-hisiv200-linux-gcc
 
APP:=libTFI.so
all: $(APP)
$(APP) :  $(LIBOBJ) 
	$(CC) -shared -o $@ $^ -lpthread -lrt

$(LIBOBJ) : %.o:%.c
	$(CC) $(APP_FLAGS) -c $^ -o $@

cc:
	@rm  -f $(LIBOBJ)  
	rm -f $(APP) 
	rm -f tags
