#!/bin/sh
# 2022-07-26-YH

OBJ_APP_DIR="special"
OBJ_APP_NAME="special"

#如果没有目标app依赖库对应的目录，则创建目录
if [ ! -d "/home/lib" ]; then
    mkdir /home/lib
fi

if [ ! -d "/home/bin" ]; then
    mkdir /home/bin
fi

if [ ! -d "/home/etc" ]; then
    mkdir /home/etc
fi

# 拷贝目标app所需要依赖的库文件
if [ -d /home/lib ]; then
    rm -rf /home/lib/*
    cd /var/update #升级包解压后所在的临时目录
    cp ./app_lib/* /home/lib/
    # sleep 1
fi

# 拷贝目标home/bin下可自行文件
if [ -d /home/bin ]; then
    cd /var/update #升级包解压后所在的临时目录
    cp ./home_bin/* /home/bin/
    # sleep 1
fi

if [ -d /home/etc ]; then
    cd /var/update #升级包解压后所在的临时目录
    cp ./app_etc/rc.local /home/etc/

    # 若需要批量修改为相同 IP 地址，则可以修改 ip.cfg 文件后，打开本行
    # cp ./app_etc/ip.cfg /home/etc/
fi

sleep 1

# 删除当前可能存在的已开机自启动的其他app
# sed -i '/test/d' /home/etc/rc.local
# sed -i '/CommonBoard/d' /home/etc/rc.local
# sed -i '/tlctest/d' /home/etc/rc.local
del_str="# add app config"
has_test_app=$(cat /home/etc/rc.local | grep "${del_str}")
if [ "$has_test_app" == "$del_str" ]; then
    del_start_line=$(sed -n "/# add app config/=" /home/etc/rc.local)
    start_line_tmp=1
    del_start_line=$(($del_start_line + $start_line_tmp))
    del_end_line=$(sed -n "$=" /home/etc/rc.local)
    echo ${del_start_line}
    echo ${del_end_line}
    sed -i "${del_start_line},${del_end_line}d" /home/etc/rc.local
fi

has_uaer_app=$(cat /home/etc/rc.local | grep "${OBJ_APP_NAME}")
# 改写 /home/etc/rc.local 开机自启动目标app
if [ $has_uaer_app="" ]; then
    echo " " >>/home/etc/rc.local
    echo "# user app autorun " >>/home/etc/rc.local
    # echo "cd /home/${OBJ_APP_DIR}" >>/home/etc/rc.local
    # echo "chmod a+x /home/$OBJ_APP_DIR/${OBJ_APP_NAME}" >>/home/etc/rc.local
    # echo "/home/$OBJ_APP_DIR/${OBJ_APP_NAME} -r" >>/home/etc/rc.local
    echo "cd /home/${OBJ_APP_DIR}" >>/home/etc/rc.local
    echo "chmod a+x ./${OBJ_APP_NAME}" >>/home/etc/rc.local
    echo "./${OBJ_APP_NAME} -r" >>/home/etc/rc.local
    echo " " >>/home/etc/rc.local
fi

# 如果没有目标app运行目录，则创建目录
if [ ! -d "/home/$OBJ_APP_DIR" ]; then
    mkdir /home/$OBJ_APP_DIR
fi

# 关闭当前正在运行的目标app
if [ -d /home/$OBJ_APP_DIR ]; then

    if [ -f /home/$OBJ_APP_DIR/${OBJ_APP_NAME} ]; then
        cd /home/$OBJ_APP_DIR
        # /home/$OBJ_APP_DIR/${OBJ_APP_NAME} -x
        ./${OBJ_APP_NAME} -x
        usleep 50000
    fi
fi

# 拷贝新的固件到目标app运行目录下
cd /var/update #升级包解压后所在的临时目录
# cp ./app_bin/* /home/$OBJ_APP_DIR
cp ./app_bin/${OBJ_APP_NAME} /home/$OBJ_APP_DIR
cp ./app_bin/_gbk2uc.dat /home/$OBJ_APP_DIR
cp ./app_bin/_uc2gbk.dat /home/$OBJ_APP_DIR
cp ./app_bin/mod_led.so /home/$OBJ_APP_DIR
cp ./app_bin/mod_qr.so /home/$OBJ_APP_DIR
cp ./app_lib/libutils.so /home/$OBJ_APP_DIR

# 删除配置文件，让软件首次运行时自动生成
# rm /home/$OBJ_APP_DIR/app.cfg
# 直接拷贝升级包中的配置文件覆盖旧的配置文件
# cp ./app_bin/app.cfg /home/$OBJ_APP_DIR

usleep 50000

#拷贝其他需要的文件
cd /var/update #升级包解压后所在的临时目录
cp ./app_bin/set_data_to_hw.sh /home/$OBJ_APP_DIR
chmod a+x /home/$OBJ_APP_DIR/set_data_to_hw.sh

usleep 50000
reboot
# 进入目标app运行目录，添加运行权限，开启后台运行app
# cd /home/${OBJ_APP_DIR}
# chmod a+x /home/$OBJ_APP_DIR/${OBJ_APP_NAME}
# /home/$OBJ_APP_DIR/${OBJ_APP_NAME} -r
