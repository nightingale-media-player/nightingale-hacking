#!/bin/bash

notice() {
  echo $* 1>&2
}

DEPTH=../..
CURRENT_DIR=`pwd`

export SONGBIRD_OFFICIAL=1
export SB_DISABLE_JARS=1
export SB_ENABLE_INSTALLER=1

cd ${DEPTH}
make -f songbird.mk clobber
make -f songbird.mk

unset SONGBIRD_OFFICIAL
unset SB_DISABLE_JARS
unset SB_ENABLE_INSTALLER

