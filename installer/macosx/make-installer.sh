#!/bin/sh

CURRENT_DIR=`pwd`

rm -rf ../../../_built_installer
mkdir ../../../_built_installer

rm -rf _packaging
mkdir _packaging
mkdir _packaging/Songbird.app
cp -RL dist/* _packaging/Songbird.app
cp -RL ../../../installer/macosx/*.plist ../../_packaging/Songbird.app/Contents/
cp -RL ../../../installer/macosx/Resources/* ../../_packaging/Songbird.app/Contents/Resources/
cp ../../../installer/macosx//MacSongbirdInstall.icns ../../_packaging/Songbird.app/Contents/Resources/
cp ../../../installer/macosx/GPL.txt ../../_packaging/Songbird.app/
cp ../../../installer/macosx/LICENSE.txt ../../_packaging/Songbird.app/
cp ../../../installer/macosx/TRADEMARK.txt ../../_packaging/Songbird.app/

../../../installer/macosx/make-diskimage ../../../_built_installer/Songbird.dmg ../../../compiled/_packaging Songbird -null- ../../../installer/macosx/songbird.dsstore

