#!/bin/bash

DEPTH=../../..
CURRENT_DATE=`date +%Y%m%d`
SONGBIRD_DIR=Songbird_${CURRENT_DATE}
SONGBIRD_ARCH=$1
SONGBIRD_FILENAME=${SONGBIRD_DIR}_${SONGBIRD_ARCH}

cd ${DEPTH}/compiled

# clean up any remaining installers & dirs and make fresh dirs
rm -rf ./_built_installer
mkdir ./_built_installer

rm -rf ./${SONGBIRD_DIR}
mkdir ./${SONGBIRD_DIR}

rm -f ./${SONGBIRD_FILENAME}.tar
rm -f ./${SONGBIRD_FILENAME}.tar.gz

# pull everything we want into the installer working dir
cp -RL ./dist/* ./${SONGBIRD_DIR}
cp ${DEPTH}/installer/linux/LICENSE.txt ./${SONGBIRD_DIR}
cp ${DEPTH}/installer/linux/TRADEMARK.txt ./${SONGBIRD_DIR}

# tar and gzip the goods
tar -c -f ./${SONGBIRD_FILENAME}.tar ./${SONGBIRD_DIR}
gzip ./${SONGBIRD_FILENAME}.tar

# copy tarball into final location and create md5sum
cd ./_built_installer
cp ../${SONGBIRD_FILENAME}.tar.gz .
md5sum ./${SONGBIRD_FILENAME}.tar.gz > ./${SONGBIRD_FILENAME}.tar.gz.md5

