#!/bin/sh

if [[ $# < 4 ]]; then
  echo usage: make-installer architecture version build-number readme
  exit 1
fi

DEPTH=../../..
CURRENT_DIR=`pwd`
SONGBIRD_ARCH="$1"
SONGBIRD_VERSION="$2"
SONGBIRD_BUILD_NUMBER="$3"
README_FILE="$4"

SONGBIRD_FILENAME=Songbird_${SONGBIRD_VERSION}-${SONGBIRD_BUILD_NUMBER}_${SONGBIRD_ARCH}

rm -rf ${DEPTH}/compiled/_built_installer
mkdir ${DEPTH}/compiled/_built_installer

cp ${DEPTH}/compiled/dist/Songbird.app/Contents/Resources/${README_FILE} \
   ${DEPTH}/compiled/dist/README.txt
mv ${DEPTH}/compiled/dist/Songbird.app/Contents/Resources/${README_FILE} \
   ${DEPTH}/compiled/dist/Songbird.app/Contents/Resources/README.txt
ln -s /Applications/ ${DEPTH}/compiled/dist/Applications

${DEPTH}/installer/macosx/make-diskimage \
  ${DEPTH}/compiled/_built_installer/${SONGBIRD_FILENAME}.dmg \
  ${DEPTH}/compiled/dist \
  Songbird \
  -null- \
  ${DEPTH}/installer/macosx/songbird.dsstore \
  ${DEPTH}/installer/macosx/background.tiff \
  ${DEPTH}/installer/macosx/MacSongbirdDiskImage.icns 

md5 ${DEPTH}/compiled/_built_installer/${SONGBIRD_FILENAME}.dmg > \
    ${DEPTH}/compiled/_built_installer/${SONGBIRD_FILENAME}.dmg.md5
