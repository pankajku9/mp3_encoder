#!/bin/sh
set -e
set -x  #set +x
#set -v # set -+v
lame_version=3.100
log_file='lame_setup.log'

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
wget "http://sourceforge.net/projects/lame/files/lame/${lame_version}/lame-${lame_version}.tar.gz"
tar -xf lame-${lame_version}.tar.gz
cp -f Makefile.unix lame-${lame_version}/Makefile.unix
cd lame-${lame_version}
cp configMS.h config.h
make -f Makefile.unix UNAME=MSDOS
cd ..
rm -rf ext || true
mkdir -p ext
cd ext
mkdir -p lib
mkdir -p include
mkdir -p include/lame
cd ..
cp lame-${lame_version}/libmp3lame/libmp3lame.a ext/lib
cp lame-${lame_version}/include/* ext/include/lame

echo "Setup Done Sucessfully"
