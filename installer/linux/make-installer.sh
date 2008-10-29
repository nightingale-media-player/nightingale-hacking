#!/bin/bash

if [ $# -lt 4 ]; then
  echo usage: make-installer architecture version build-number readme
  exit 1
fi

DEPTH=../../..
SONGBIRD_ARCH=$1
SONGBIRD_VERSION=$2
SONGBIRD_BUILD_NUMBER=$3
README_FILE=$4

SONGBIRD_DIR=Songbird
SONGBIRD_FILENAME=Songbird_${SONGBIRD_VERSION}-$(SONGBIRD_BUILD_NUMBER}_${SONGBIRD_ARCH}

cd ${DEPTH}/compiled

# clean up any remaining installers & dirs and make fresh dirs
rm -rf ./_built_installer
mkdir ./_built_installer

rm -rf ./${SONGBIRD_DIR}
mkdir ./${SONGBIRD_DIR}

rm -f ./${SONGBIRD_FILENAME}.tar
rm -f ./${SONGBIRD_FILENAME}.tar.gz

mv ./dist/${README_FILE} ./dist/README.txt

# pull everything we want into the installer working dir
cp -RL ./dist/* ./${SONGBIRD_DIR}

# tar and gzip the goods
tar -c -f ./${SONGBIRD_FILENAME}.tar ./${SONGBIRD_DIR}
gzip ./${SONGBIRD_FILENAME}.tar

# copy tarball into final location and create md5sum
cd ./_built_installer
cp ../${SONGBIRD_FILENAME}.tar.gz .
md5sum ./${SONGBIRD_FILENAME}.tar.gz > ./${SONGBIRD_FILENAME}.tar.gz.md5

