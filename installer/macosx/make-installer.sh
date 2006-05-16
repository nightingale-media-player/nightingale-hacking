#!/bin/sh

DEPTH=../../..
CURRENT_DIR=`pwd`

rm -rf ${DEPTH}/_built_installer
mkdir ${DEPTH}/_built_installer

rm -rf ${DEPTH}/compiled/_packaging
mkdir ${DEPTH}/compiled/_packaging
mkdir ${DEPTH}/compiled/_packaging/Songbird.app
cp -RL ${DEPTH}/compiled/dist/* ${DEPTH}/compiled/_packaging/Songbird.app
cp -RL ${DEPTH}/installer/macosx/*.plist ${DEPTH}/compiled/_packaging/Songbird.app/Contents/
cp -RL ${DEPTH}/compiled/dist/Contents/Resources/* ${DEPTH}/compiled/_packaging/Songbird.app/Contents/Resources/
cp ${DEPTH}/installer/macosx//MacSongbirdInstall.icns ${DEPTH}/compiled/_packaging/Songbird.app/Contents/Resources/
cp ${DEPTH}/installer/macosx/GPL.txt ${DEPTH}/compiled/_packaging/Songbird.app/
cp ${DEPTH}/installer/macosx/LICENSE.txt ${DEPTH}/compiled/_packaging/Songbird.app/
cp ${DEPTH}/installer/macosx/TRADEMARK.txt ${DEPTH}/compiled/_packaging/Songbird.app/

${DEPTH}/installer/macosx/make-diskimage ${DEPTH}/_built_installer/Songbird.dmg ${DEPTH}/compiled/_packaging Songbird -null- ${DEPTH}/installer/macosx/songbird.dsstore

