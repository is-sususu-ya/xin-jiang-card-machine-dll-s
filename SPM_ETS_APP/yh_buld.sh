#!/bin/bash

time make clean
sleep 1
sync

time make
sleep 4
sync

cp corelib/special /home/linux/Share_Smaba/01_Release/
cp mod_led/mod_led.so /home/linux/Share_Smaba/01_Release/
cp mod_scan/mod_qr.so /home/linux/Share_Smaba/01_Release/
sleep 1
sync
