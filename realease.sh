#!/bin/bash
host=`hostname`
if [ "$host" = "571f5f8200ca" ]; then
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
	cd $root_dir
	cp ./utils/wintype.h $release_dir
else
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
		make SET=pc
		cp ./lib*.so $release_dir
		cp ./*.h $release_dir
		cp *.pdf $release_dir
		cp LinuxDemo* -rf $release_dir
		make cc
	done
	cd $root_dir
	cp ./utils/wintype.h $release_dir
fi
