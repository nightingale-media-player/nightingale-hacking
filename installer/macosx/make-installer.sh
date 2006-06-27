#!/bin/sh

DEPTH=../../..
CURRENT_DIR=`pwd`

rm -rf ${DEPTH}/_built_installer
mkdir ${DEPTH}/_built_installer

cp ${DEPTH}/installer/macosx/GPL.txt ${DEPTH}/compiled/dist/Songbird.app/
cp ${DEPTH}/installer/macosx/LICENSE.txt ${DEPTH}/compiled/dist/Songbird.app/
cp ${DEPTH}/installer/macosx/TRADEMARK.txt ${DEPTH}/compiled/dist/Songbird.app/

${DEPTH}/installer/macosx/make-diskimage ${DEPTH}/_built_installer/Songbird.dmg ${DEPTH}/compiled/dist Songbird -null- ${DEPTH}/installer/macosx/songbird.dsstore
