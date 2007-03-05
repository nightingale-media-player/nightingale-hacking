#!/bin/sh

notice() {
  echo $* 1>&2
}

DEPTH=../..
CURRENT_DIR=`pwd`

export SONGBIRD_OFFICIAL=1
export SB_UPDATE_CHANNEL="nightly"
export SB_ENABLE_JARS=1
export SB_ENABLE_INSTALLER=1

cd ${DEPTH}
make -f songbird.mk clobber
make -f songbird.mk
make -C compiled/update complete

unset SONGBIRD_OFFICIAL
unset SB_UPDATE_CHANNEL
unset SB_ENABLE_JARS
unset SB_ENABLE_INSTALLER

notice
notice "Done. MAR and snippet are in compiled/update."
notice "Please copy these files to a safe location!"
notice "They are required to generate the update for the next build."
notice
