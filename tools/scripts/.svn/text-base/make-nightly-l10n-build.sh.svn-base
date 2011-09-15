#!/bin/sh

notice() {
  echo $* 1>&2
}

DEPTH=../..
CURRENT_DIR=`pwd`

export SB_ENABLE_BREAKPAD==1
export SB_DISABLE_JARS=1
export SB_ENABLE_INSTALLER=1

cd ${DEPTH}
make -f songbird.mk clobber
make -f songbird.mk

unset SB_ENABLE_BREAKPAD
unset SB_DISABLE_JARS
unset SB_ENABLE_INSTALLER

