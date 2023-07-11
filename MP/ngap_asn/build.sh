#!/bin/bash

export ROOTDIR="${PWD}"
if [  -f ${ROOTDIR}/../build/lib/libngap.a ]; then
    echo "libngap.a exist"
    exit
fi

set -e

sudo cp ${OSSINFO}/ossasn1/linux-x86-64.trial/11.1.0.2/include/*.h ${ROOTDIR}/../build/include
sudo cp ${OSSINFO}/ossasn1/linux-x86-64.trial/11.1.0.2/lib/*.a ${ROOTDIR}/../build/lib

rm -rf BUILD_DIR
mkdir BUILD_DIR
cd BUILD_DIR
cp ../ASN_FILES/*.asn ./

asn1 ./*.asn  -nouniquepdu -debug -c -warningMessages -informatoryMessages -per -json -autoencdec
gcc -c ./NGAP-PDU-Descriptions.c -I${ROOTDIR}/../build/include -DOSSPRINT
ar rc libngap.a ./NGAP-PDU-Descriptions.o
cp ./libngap.a ${ROOTDIR}/../build/lib
cp ./*h ${ROOTDIR}/../build/include
