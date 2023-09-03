#!/bin/bash
cd ../
make cc
make 
cd -
make cc
make 
cp ../libTCR8ACM_x86.so .
./sdemo
cp -vf libTCR8ACM_x86.so /nfsroot/
cp -vf sdemo /nfsroot/