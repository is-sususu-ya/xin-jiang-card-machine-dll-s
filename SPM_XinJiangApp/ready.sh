#!/bin/bash 
rm -rf release 
mkdir release 
root_path=`pwd` 
strip_tool=arm-hisiv100nptl-strip
root_dir=$root_path"/release" 
# 测试时，可以加入 "TMPTEST"
# "TMP3520TEST"
# "TMPE42" "TMP55" "TMPE32" "TSP86" 
for list in "TSP86" 
do
	release_dir=$root_dir"/dst_"$list
	release_lib=$root_dir"/dst_"$list"/lib"
	release_app=$root_dir"/dst_"$list"/pay"
	release_dll=$root_dir"/dst_"$list"/dll"
	mkdir -p $release_dir $release_lib $release_app
	cd $root_path
	make clean
	make BOARD_TYPE=$list -j8
	mkdir $release_dir/lib
	cp ./corelib/pay $release_app
	cp ./mod_led/mod_led.so  $release_app
	cp ./mod_scan/mod_qr.so  $release_app
	cp ./mod_scan_tty/mod_qr_tty.so  $release_app
	cp ./utils/libutils.so $release_lib 
	if [ "$list" = "TMPE32" ];then
		cp -rvf ./lib/v100lib/*.so* $release_lib 
		arm-hisiv100nptl-linux-strip $release_lib/*.so*
	fi  
	if [  "$list" = "TMP55" ]; then 
		cp -rvf ./lib/v100lib/*.so* $release_lib 
		arm-hisiv100nptl-linux-strip $release_lib/*.so*
	fi   
	if [ "$list" = "TMPE42" ]; then 
		cp -rvf ./lib/v400lib/*.so* $release_lib 
		arm-hisiv400-linux-strip $release_lib/*.so*
	fi  
	if [ "$list" = "TMPTEST" ];then
		cp -rvf ./lib/v100lib/*.so* $release_lib 
		arm-hisiv100nptl-linux-strip $release_lib/*.so*
	fi
	if [ "$list" = "TSP86" ]; then
		cp -rvf ./lib/v100lib/*.so* $release_lib 
		cp corelib/pay /nfsroot/work/dst_TSP86/pay/
		arm-hisiv100nptl-linux-strip $release_lib/*.so*
	fi
	if [ "$list" = "TMP3520TEST" ]; then
		cp -rvf ./lib/v100lib/*.so* $release_lib 
		arm-hisiv100nptl-linux-strip $release_lib/*.so*
	fi
done
rm ./payTar.tar.gz
tar zcvf  payTar.tar.gz ./release/
#rm -rf release
