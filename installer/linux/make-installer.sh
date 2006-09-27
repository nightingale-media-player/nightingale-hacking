#!/bin/bash

CURRENT_DIR=`pwd`
CURRENT_DATE=`date +%Y%m%d`
SONGBIRD_DIR=Songbird_${CURRENT_DATE}
SONGBIRD_ARCH=$1

rm -rf ../../../_built_installer
mkdir ../../../_built_installer

rm -rf ../../../compiled/${SONGBIRD_DIR}
mkdir ../../../compiled/${SONGBIRD_DIR}

cp -RL ../../../compiled/dist/* ../../../compiled/${SONGBIRD_DIR}
cp ../../../installer/linux/LICENSE.txt ../../../compiled/${SONGBIRD_DIR}
cp ../../../installer/linux/TRADEMARK.txt ../../../compiled/${SONGBIRD_DIR}

rm -f ../../../compiled/${SONGBIRD_DIR}_${SONBIRD_ARCH}.tar
rm -f ../../../compiled/${SONGBIRD_DIR}_${SONBIRD_ARCH}.tar.gz

cd ../../../compiled
tar -c -f ${SONGBIRD_DIR}_${SONGBIRD_ARCH}.tar ${SONGBIRD_DIR}
gzip ${SONGBIRD_DIR}_${SONGBIRD_ARCH}.tar

cp ${SONGBIRD_DIR}_${SONGBIRD_ARCH}.tar.gz ../_built_installer

