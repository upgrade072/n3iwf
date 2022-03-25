#!/bin/bash

export ROOTDIR="$(pwd)/../../"
export PKG_CONFIG_PATH=${ROOTDIR}/build/lib/pkgconfig
LIBNAME="asn1c"

if [  -f ${ROOTDIR}/build/bin/${LIBNAME} ]; then
	echo "${LIBNAME} already exist in build/bin"
    exit
fi

tar xvf ./asn1c.tar

cd asn1c

autoreconf --force --install
./configure \
    --prefix=${ROOTDIR}/build \
    --enable-static \
	--enable-ASN_DEBUG

make -j4
make install
