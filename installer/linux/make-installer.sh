#!/bin/bash

DEPTH=../../..
CURRENT_DIR=`pwd`
CURRENT_DATE=`date +%Y%m%d`
SONGBIRD_DIR=Songbird_${CURRENT_DATE}
SONGBIRD_ARCH=$1

rm -rf ${DEPTH}/compiled/_built_installer
mkdir ${DEPTH}/compiled/_built_installer

rm -rf ${DEPTH}/compiled/${SONGBIRD_DIR}
mkdir ${DEPTH}/compiled/${SONGBIRD_DIR}

cp -RL ${DEPTH}/compiled/dist/* ${DEPTH}/compiled/${SONGBIRD_DIR}
cp ${DEPTH}/installer/linux/LICENSE.txt ${DEPTH}/compiled/${SONGBIRD_DIR}
cp ${DEPTH}/installer/linux/TRADEMARK.txt ${DEPTH}/compiled/${SONGBIRD_DIR}

rm -f ${DEPTH}/compiled/${SONGBIRD_DIR}_${SONBIRD_ARCH}.tar
rm -f ${DEPTH}/compiled/${SONGBIRD_DIR}_${SONBIRD_ARCH}.tar.gz

cd ${DEPTH}/compiled
tar -c -f ${SONGBIRD_DIR}_${SONGBIRD_ARCH}.tar ${SONGBIRD_DIR}
gzip ${SONGBIRD_DIR}_${SONGBIRD_ARCH}.tar

cp ${SONGBIRD_DIR}_${SONGBIRD_ARCH}.tar.gz _built_installer

