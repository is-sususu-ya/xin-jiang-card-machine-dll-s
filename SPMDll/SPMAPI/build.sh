#!/bin/bash

git pull
  
make clean
make cc
make CC=gcc APP=libSPM_x86.so
 
# make clean
# make cc
# make CC=arm-hisiv100nptl-linux-gcc APP=libSPM_hisi.so