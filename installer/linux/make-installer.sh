#!/bin/bash

CURRENT_DIR=`pwd`
CURRENT_DATE=`date +%Y%m%d`
SONGBIRD_DIR=Songbird_${CURRENT_DATE}

rm -rf ../../built_installer
mkdir ../../built_installer

cd ${CURRENT_DIR}/../../compiled/
rm -rf ${SONGBIRD_DIR}
mkdir ${SONGBIRD_DIR}

cp -RL dist/* ${SONGBIRD_DIR}
cp ${CURRENT_DIR}/GPL.txt ${SONGBIRD_DIR}
cp ${CURRENT_DIR}/LICENSE.txt ${SONGBIRD_DIR}
cp ${CURRENT_DIR}/TRADEMARK.txt ${SONGBIRD_DIR}

rm -f ${SONGBIRD_DIR}.tar
rm -f ${SONGBIRD_DIR}.tar.gz

tar -c -f ${SONGBIRD_DIR}.tar ${SONGBIRD_DIR}
gzip ${SONGBIRD_DIR}.tar

cp ${SONGBIRD_DIR}.tar.gz ${CURRENT_DIR}/../../_built_installer

