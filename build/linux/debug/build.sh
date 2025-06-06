#!/bin/sh

curdir=`pwd`

cd ../../..
autoreconf --force --install

cd ${curdir}

# To Profile w/ gprof:
# 1) Modify configure: ./configure --disable-shared CFLAGS="-pg"
# 2) Run the program. ./foo
# 3) Run gprof /libtool --mode=execute gprof ./foo

../../../configure --prefix=${curdir}  --enable-debug  --enable-websock \
--enable-alsa \
CFLAGS="-g -Wall" \
CXXFLAGS="-g -Wall" \
LIBS=


#make
#make install
