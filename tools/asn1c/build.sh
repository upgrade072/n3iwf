#!/bin/bash

export ROOTDIR="$(pwd)/../../"
export PKG_CONFIG_PATH=${ROOTDIR}/build/lib/pkgconfig
LIBNAME="asn1c"

if [  -f ${ROOTDIR}/build/bin/${LIBNAME} ]; then
	echo "${LIBNAME} already exist in build/bin"
    exit
fi

if [ ! -d ${LIBNAME} ]; then
    git clone https://github.com/mouse07410/asn1c.git
fi

cd asn1c
git reset --hard 8282f80bc89cc773f9432cde56398a36f2683511^

autoreconf --force --install
./configure \
    --prefix=${ROOTDIR}/build \
    --enable-static \
	--enable-ASN_DEBUG

make -j4
make install
