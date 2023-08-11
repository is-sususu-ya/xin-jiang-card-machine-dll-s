#!/bin/bash

git pull

make clean
make cc
make CC=/work/toolchain_R2_EABI/usr/bin/arm-none-linux-gnueabi-gcc APP=libSPM_arm.so

make clean
make cc
make CC=gcc APP=libSPM_x86.so
 
make cc