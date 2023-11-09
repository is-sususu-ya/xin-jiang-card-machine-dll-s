#!/bin/sh
# sync time with rtc 
export TZ=CST-8
chmod +x /home/bin/rtcsync
/home/bin/rtcsync -l
sync
