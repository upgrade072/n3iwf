#!/bin/bash

export ROOTDIR="${PWD}"
if [  -f ${ROOTDIR}/../build/lib/libasncodec.a ]; then
    echo "libasncodec exist"
    exit
fi

set -e
#export ASN1C_PREFIX=NGAP_ // impact to structure & header file name
export ASN1C_PREFIX=

rm -rf BUILD_DIR
mkdir BUILD_DIR
cd BUILD_DIR
mkdir code

../../build/bin/asn1c \
    -pdu=all \
    -fcompound-names \
    -fno-include-deps \
    -findirect-choice \
	-gen-autotools \
	-gen-APER \
    -D code \
    ../ASN_FILES/*.asn

#-fwide-types \

autoreconf --force --install

./configure \
	--prefix=${ROOTDIR}/../build \
	--enable-static

make -j8
make install

mkdir -p $(pwd)/../../build/include
cp code/*h $(pwd)/../../build/include
