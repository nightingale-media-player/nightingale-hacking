#!/bin/bash

DEPTH=../../..
CURRENT_DATE=`date +%Y%m%d`
ARCH="$1"

cd ${DEPTH}/installer/windows
cmd /c "PrepareInstaller.bat ${CURRENT_DATE} cygwin ${ARCH}"

