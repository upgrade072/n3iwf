#!/bin/bash

export ROOTDIR="$(pwd)/../../"
export PKG_CONFIG_PATH=${ROOTDIR}/build/lib/pkgconfig
LIBNAME="libconfig"

if [  -f ${ROOTDIR}/build/lib/${LIBNAME}.a ]; then
	echo "${LIBNAME} already exist in build/lib"
    exit
fi

VERSION="1.7.3"
if [ ! -f libconfig-${VERSION}.tar.gz ]; then
	wget https://hyperrealm.github.io/libconfig/dist/libconfig-${VERSION}.tar.gz
fi

rm -rf libconfig-${VERSION}
tar xvf libconfig-${VERSION}.tar.gz

cd libconfig-${VERSION}

./configure \
    --prefix=${ROOTDIR}/build \
    --enable-static \
    --disable-shared

make
make install
