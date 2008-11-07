#!/bin/bash

if [ $# < 3 ]; then
  echo usage: make-installer architecture readme buildid
  exit 1
fi

DEPTH=../../..
CURRENT_DATE=`date +%Y%m%d`
ARCH="$1"
README_FILE="$2"
BUILD_NUMBER="$3"

mv ${DEPTH}/compiled/dist/${README_FILE} ${DEPTH}/compiled/dist/README.txt

cd ${DEPTH}/installer/windows
cmd /c "PrepareInstaller.bat ${BUILD_NUMBER} ${CURRENT_DATE} ${ARCH}"

