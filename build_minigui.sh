root_dir=`pwd`
release_dir=$root_dir/"linux_release"
rm -rf $release_dir
mkdir $release_dir
for list in $root_dir/TCR8Collect/  $root_dir/TCR8SendCard/
do
    cd $root_dir
    cd $list
    rm lib*.so
    make cc
    make SET=xinjiang
    make cc
    make SET=pc
    cp ./lib*.so $release_dir
    cp ./*.h $release_dir
    cp *.pdf $release_dir
    cp LinuxDemo* -rf $release_dir
    make cc
done
cd  $root_dir/SPMDll/SPMAPI
sh ./build_minigui.sh
cp ./libSPM_arm.so $release_dir
cp ./libSPM_x86.so $release_dir
cp ./SPM.h $release_dir
cp ../SPMDemo -rf $release_dir
cd $root_dir
cp ./utils/wintype.h $release_dir