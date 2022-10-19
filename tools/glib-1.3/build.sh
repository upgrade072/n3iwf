#!/bin/bash

export ROOTDIR="$(pwd)/../../"
export PKG_CONFIG_PATH=${ROOTDIR}/build/lib/pkgconfig
LIBNAME="libglib-1.3"

if [  -f ${ROOTDIR}/build/lib/${LIBNAME}.a ]; then
	echo "${LIBNAME} already exist in build/lib"
    exit
fi

VERSION="1.3.15"
if [ ! -f glib-${VERSION}.tar.gz ]; then
	wget https://download.gnome.org/sources/glib/1.3/glib-${VERSION}.tar.gz --no-check-certificate
fi

rm -rf glib-${VERSION}
tar xvf glib-${VERSION}.tar.gz

cd glib-${VERSION}

./configure \
    --prefix=${ROOTDIR}/build \
    --enable-static \
    --disable-shared

make -j8
make install
cp ./glibconfig.h ${ROOTDIR}/build/include/glib-2.0
