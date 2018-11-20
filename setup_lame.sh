#!/bin/sh
set -e
set -x  #set +x
#set -v # set -+v
lame_version=3.100

#clean the directories
if [ $# -eq 1 ] ; then
    usr_opt=$1
    if [ ${usr_opt} = "clean" ] ; then
        rm -rf ext || true
        rm -rf lame-${lame_version} || true
        rm lame-${lame_version}.tar.gz || true
    fi
    exit 0
 fi
 
# lame : get tar, unzip, build and install 
rm -rf ext || true
mkdir -p ext
wget "http://sourceforge.net/projects/lame/files/lame/${lame_version}/lame-${lame_version}.tar.gz"
tar -xf lame-${lame_version}.tar.gz
cd lame-${lame_version}
rm -rf build || true
mkdir -p build
cd build
../configure --prefix=$PWD/../../ext/
make -j 4 
make install 
cd ../../
echo "Setup Done Sucessfully"
