#!/bin/bash

export ROOTDIR="$(pwd)/../../"
export PKG_CONFIG_PATH=${ROOTDIR}/build/lib/pkgconfig
LIBNAME="libjson-c"

if [  -f ${ROOTDIR}/build/lib/${LIBNAME}.a ]; then
	echo "${LIBNAME}.a already exist in build/lib"
    exit
fi

VERSION="json-c-0.13.1-20180305"
if [ ! -f ${VERSION}.tar.gz ]; then
    wget https://github.com/json-c/json-c/archive/${VERSION}.tar.gz
fi

rm -rf json-c-${VERSION}
tar xvf ${VERSION}.tar.gz

cd json-c-${VERSION}

autoreconf --force --install
./configure \
	--prefix=${ROOTDIR}/build \
	--with-gnu-ld \
	--enable-threading \
	--enable-static \
	--disable-shared

make
make install
