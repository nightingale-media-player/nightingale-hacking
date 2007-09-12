#!/bin/sh

DEPTH=../../..
CURRENT_DIR=`pwd`
CURRENT_DATE=`date +%Y%m%d`
ARCH="$1"

rm -rf ${DEPTH}/compiled/_built_installer
mkdir ${DEPTH}/compiled/_built_installer

cp ${DEPTH}/compiled/dist/Songbird.app/README.txt ${DEPTH}/compiled/dist/README.txt
ln -s /Applications/ ${DEPTH}/compiled/dist/Applications

${DEPTH}/installer/macosx/make-diskimage ${DEPTH}/compiled/_built_installer/Songbird_${CURRENT_DATE}_${ARCH}.dmg ${DEPTH}/compiled/dist Songbird -null- ${DEPTH}/installer/macosx/songbird.dsstore ${DEPTH}/installer/macosx/background.tiff ${DEPTH}/installer/macosx/MacSongbirdDiskImage.icns 
md5 ${DEPTH}/compiled/_built_installer/Songbird_${CURRENT_DATE}_${ARCH}.dmg > ${DEPTH}/compiled/_built_installer/Songbird_${CURRENT_DATE}_${ARCH}.dmg.md5
