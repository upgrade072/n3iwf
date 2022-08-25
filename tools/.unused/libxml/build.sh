#!/bin/bash

export ROOTDIR="$(pwd)/../../"
export PKG_CONFIG_PATH=${ROOTDIR}/build/lib/pkgconfig
LIBNAME="libxml2"

if [  -f ${ROOTDIR}/build/lib/${LIBNAME}.a ]; then
	echo "${LIBNAME} already exist in build/bin"
    exit
fi

VERSION="v2.9.11"
if [ ! -f ${LIBNAME}-${VERSION}.tar.gz ]; then
	wget https://gitlab.gnome.org/GNOME/libxml2/-/archive/${VERSION}/libxml2-${VERSION}.tar.gz --no-check-certificate
fi

rm -rf libxml2-${VERSION}
tar xvf libxml2-${VERSION}.tar.gz

cd libxml2-${VERSION}

./autogen.sh
./configure \
    --prefix=${ROOTDIR}/build \
    --enable-static 

make 
make install
