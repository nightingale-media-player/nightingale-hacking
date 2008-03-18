#!/bin/sh

if [[ $# < 2 ]]; then
  echo usage: make-installer architecture version readme
  exit 1
fi

DEPTH=../../..
CURRENT_DIR=`pwd`
CURRENT_DATE=`date +%Y%m%d`
SONGBIRD_ARCH="$1"
SONGBIRD_VERSION="$2"
README_FILE="$3"

if [ ${README_FILE} = README_OFFICIAL.txt ]; then
SONGBIRD_FILENAME=Songbird_${SONGBIRD_VERSION}_${SONGBIRD_ARCH}
else
SONGBIRD_FILENAME=Songbird_${SONGBIRD_VERSION}_${CURRENT_DATE}_${SONGBIRD_ARCH}
fi

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
