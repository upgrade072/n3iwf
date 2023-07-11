#!/bin/bash

export ROOTDIR="$(pwd)/../../"
export PKG_CONFIG_PATH=${ROOTDIR}/build/lib/pkgconfig
LIBNAME="libfort"

if [  -f ${ROOTDIR}/build/lib/${LIBNAME}.a ]; then
	echo "${LIBNAME} already exist in build/lib"
    exit
fi

VERSION="0.4.2"
if [ ! -f v${VERSION}.tar.gz ]; then
	wget https://github.com/seleznevae/${LIBNAME}/archive/refs/tags/v${VERSION}.tar.gz
fi

rm -rf ${LIBNAME}-${VERSION}
tar xvf v${VERSION}.tar.gz

cd ${LIBNAME}-${VERSION}

cc ./lib/fort.c  -c -o ./libfort.o
ar -rc ./libfort.a ./libfort.o 

cp ./libfort.a ${ROOTDIR}/build/lib
cp ./lib/fort.h ${ROOTDIR}/build/include
