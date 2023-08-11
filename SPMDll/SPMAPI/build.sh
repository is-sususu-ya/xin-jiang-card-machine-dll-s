#!/bin/bash
make clean
make 2>err.log
cat err.log | grep error | head -n 20 
rm err.log
