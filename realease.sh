#!/bin/bash
root_dir=`pwd`
release_dir=$root_dir"linux_release"
rm -rf linux_release
mkdir $release_dir
for list in $root_dir/TCR8Collect/  $root_dir/TCR8SendCard/
do 
	cd $root_dir
	cd $list
	make 
	cp ./lib*.so $release_dir
	cp *.pdf $release_dir
	cp LinuxDemo* -rf $release_dir
	make cc
done 



