#!/bin/sh

if [ $# = 0 ]; then
  echo Applies an attachment patch from the Nightingale bugzilla
  echo "usage: bugzilla-patch <attachment number>"
  exit 1
fi

wget -q -O -  http://bugzilla.getnightingale.com/attachment.cgi?id=$1 | patch $2
