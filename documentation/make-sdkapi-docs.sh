#!/bin/sh

notice() {
  echo $* 1>&2
}

if [ $# != 2 ]; then
  notice "usage: make-sdkapi-docs.sh [topsrcdir] [toolsdir]"
  exit 1
fi

# Working directory
workingdir=`pwd`

# Trunk top level directory
topsrcdir="$1"

# Tools directory
toolsdir="$2"

# Run Doxygen
$toolsdir/doxygen/bin/doxygen songbird.dox
