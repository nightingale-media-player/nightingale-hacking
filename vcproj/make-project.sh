#!/bin/bash

#
# Run this script to create dummy (non-compilable) MSVC projects that reflect the source tree
#


if test "$1" == ""; then
  # no param was given, show commandline options
  echo "make-project VCVERSION"
  echo "where VCVERSION is 7, 8 or 9"
  exit
fi

# handle vc project version

if test "$1" == "7"; then
    SWITCH=-vc7
    PROJECTFILE=SongbirdVC7
fi
if test "$1" == "8"; then
    SWITCH=-vc8
    PROJECTFILE=SongbirdVC8
fi
if test "$1" == "9"; then
    SWITCH=-vc9
    PROJECTFILE=SongbirdVC9
fi

# see if the mkvcproj tool is there

if [ -f mkvcproj/mkvcproj.exe ]; then

  # setup tool commandline options

  SBROOT=..
  EXCLUDED_EXTENSIONS=obj,idb,lib,pch,pdb,ilk,exe,dll,dsp,sln,vcproj,dsw,opt,plg,ncb,suo,user,log,m4,bak
  EXCLUDED_DIRECTORIES=compiled,dependencies
  EXCLUDED_DIRPARTS=.svn
  EXCLUDED_FILEMATCH=*.,.*
  PREFIX=..
  OUTPATH=$SBROOT/vcproj

  # create the dummy vc project

  ./mkvcproj/mkvcproj.exe $SWITCH -outfiles:$PROJECTFILE -project:Songbird -root:$SBROOT -prefix:$PREFIX -outpath:$OUTPATH -e:$EXCLUDED_EXTENSIONS -d:$EXCLUDED_DIRECTORIES -p:$EXCLUDED_DIRPARTS -m:$EXCLUDED_FILEMATCH

else
  echo "mkvcproj not found"
fi

