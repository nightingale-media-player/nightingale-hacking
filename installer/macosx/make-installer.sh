#!/bin/sh

CURRENT_DIR=`pwd`

cd ../..
rm -rf _built_installer
mkdir _built_installer

cd ${CURRENT_DIR}/../../compiled/
rm -rf _packaging
mkdir _packaging
mkdir _packaging/Songbird.app
cp -RL dist/* _packaging/Songbird.app
cp -RL ${CURRENT_DIR}/*.plist _packaging/Songbird.app/Contents/
cp -RL ${CURRENT_DIR}/Resources/* _packaging/Songbird.app/Contents/Resources/
cp ${CURRENT_DIR}/MacSongbirdInstall.icns _packaging/Songbird.app/Contents/Resources/
cp ${CURRENT_DIR}/GPL.txt _packaging/Songbird.app/
cp ${CURRENT_DIR}/LICENSE.txt _packaging/Songbird.app/
CP ${CURRENT_DIR}/TRADEMARK.txt _packaging/Songbird.app/

cd ${CURRENT_DIR}
./make-diskimage ${CURRENT_DIR}/../../_built_installer/Songbird.dmg ${CURRENT_DIR}/../../compiled/_packaging Songbird -null- ${CURRENT_DIR}/songbird.dsstore

