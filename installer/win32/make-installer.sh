#!/bin/bash

DEPTH=../../..
CURRENT_DATE=`date +%Y%m%d`

cd ${DEPTH}/installer/win32
PrepareInstaller.bat ${CURRENT_DATE} cygwin

