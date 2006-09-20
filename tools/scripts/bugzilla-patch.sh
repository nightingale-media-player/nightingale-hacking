#!/bin/bash

if [ $# = 0 ]; then
  echo Applies an attachment patch from the Songbird bugzilla
  echo "usage: bugzilla-patch <attachment number>"
  exit 1
fi

wget -q -O -  http://bugzilla.songbirdnest.com/attachment.cgi?id=$1 | patch -p0
