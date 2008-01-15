#!/bin/bash

if [ $# < 2 ]; then
  echo usage: make-installer architecture readme
  exit 1
fi

DEPTH=../../..
CURRENT_DATE=`date +%Y%m%d`
ARCH="$1"
README_FILE="$2"

mv ${DEPTH}/compiled/dist/${README_FILE} ${DEPTH}/compiled/dist/README.txt

cd ${DEPTH}/installer/windows
cmd /c "PrepareInstaller.bat ${CURRENT_DATE} cygwin ${ARCH}"

