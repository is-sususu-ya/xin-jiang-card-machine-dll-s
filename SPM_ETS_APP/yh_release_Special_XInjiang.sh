#!/bin/bash

###################################################
mk_file_name="mk_Makefile_Special_XinJiang.mk"
projectname="special_XinJiang"
app_name="special"
update_src_dir="update_src_special_xinjiang"
###################################################

# 重新编译代码
time make clean
sleep 1
echo "make clean done!!!"
sleep 1
time make -f $mk_file_name
sleep 4
echo "make -f $mk_file_name done!!!"

# 制作update升级包
update_tmp="update_tmp"
root_path="$PWD"
date_verison=$(date "+%Y%m%d_%H%M%S")

release_dir="$root_path/release"
output_dir="/home/linux/Share_Smaba/01_Release/"

if [ ! -d "$release_dir" ]; then
    mkdir $release_dir
fi

# 清除并重建升级包缓存文件夹
rm -rf $update_tmp
mkdir $update_tmp

# 拷贝升级包主体模板
cp -rf $update_src_dir/* $update_tmp

# 拷贝最新编译的应用程序可执行文件
cp corelib/$app_name $update_tmp/app_bin/

# 拷贝程序以及其他文件
# cp corelib/special $update_tmp/app_bin/
cp mod_led/mod_led.so $update_tmp/app_bin/
cp mod_scan/mod_qr.so $update_tmp/app_bin/

sleep 1

# 打包程序
cd $update_tmp
zip -q -r update.zip *

cp -vf update.zip $release_dir/update_"$projectname"_"$date_verison".zip

cd $root_path
rm -rf $update_tmp

# sleep 1
# cp $release_dir/update_"$projectname"_"$date_verison".zip /mnt/hgfs/VM_Linux_Share/
# sleep 2
# echo "cp $release_dir/update_"$projectname"_"$date_verison".zip /mnt/hgfs/VM_Linux_Share/ done!!!"
# sync
sleep 1
mv $release_dir/update_"$projectname"_"$date_verison".zip $output_dir
sleep 2
echo "mv $release_dir/update_"$projectname"_"$date_verison".zip $output_dir done!!!"
sync
