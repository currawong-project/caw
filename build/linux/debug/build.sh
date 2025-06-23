#!/bin/sh

curdir=`pwd`

cd ../../..
autoreconf --force --install

cd ${curdir}

# To Profile w/ gprof:
# 1) Modify configure: ./configure --disable-shared CFLAGS="-pg"
# 2) Run the program. ./foo
# 3) Run gprof /libtool --mode=execute gprof ./foo

MKL="/opt/intel/oneapi/2025.1"

../../../configure --prefix=${curdir}  --enable-debug  --enable-websock --enable-mkl \
--enable-alsa \
CFLAGS="-g -Wall" \
CXXFLAGS="-g -Wall" \
CPPFLAGS="-I${MKL}/include" \
LDFLAGS="-L${MKL}/lib -Wl,-rpath,${MKL}/lib" \
LIBS=


#make
#make install
