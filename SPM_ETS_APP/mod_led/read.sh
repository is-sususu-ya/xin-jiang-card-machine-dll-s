#!/bin/bash
arm-hisiv100nptl-linux-gcc -DLED_TEST led.c ../utils/utils_ptrlist.c  ../utils/utils_tty.c -I../utils -lpthread -o led_test
cp -vf ./led_test /nfsroot/work
#make
#cp ./mod_led.so /nfsroot/work 
