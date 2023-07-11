#!/bin/bash

export ROOTDIR="$(pwd)/../../"
export PKG_CONFIG_PATH=${ROOTDIR}/build/lib/pkgconfig
LIBNAME="libsctp"

if [  -f ${ROOTDIR}/build/lib/${LIBNAME}.a ]; then
	echo "${LIBNAME} already exist in build/lib"
    exit
fi

VERSION="1.0.19"
if [ ! -f lksctp-tools-${VERSION}.tar.gz ]; then
	wget https://github.com/sctp/lksctp-tools/archive/refs/tags/lksctp-tools-${VERSION}.tar.gz
fi

rm -rf lksctp-tools-lksctp-tools-${VERSION}
tar xvf lksctp-tools-${VERSION}.tar.gz

cd lksctp-tools-lksctp-tools-${VERSION}

./bootstrap
./configure \
    --prefix=${ROOTDIR}/build \
    --enable-static \
    --disable-shared

make
make install
