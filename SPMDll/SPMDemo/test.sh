#!/bin/sh
cd ../SPMAPI
./build.sh
cd -
cp ../SPMAPI/libSPM_x86.so libSPM.so
./spmDemo 172.16.13.230
