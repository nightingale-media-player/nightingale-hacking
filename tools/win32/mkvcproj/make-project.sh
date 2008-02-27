#!/bin/sh

#
# Run this script to create dummy (non-compilable) MSVC projects that reflect the source tree
#


if test "$1" == ""; then
  # no param was given, show commandline options
  echo "make-project [7|8|9]"
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

# if we're running from the root of the build tree, cd to our home dir,
# otherwise, assume we're already there

if [ -e app ]; then
  cd tools/win32/mkvcproj
fi

# see if the mkvcproj tool is in the current dir

if [ -f mkvcproj.exe ]; then

  # setup tool commandline options

  SBROOT=../../..
  EXCLUDED_EXTENSIONS=obj,idb,lib,pch,pdb,ilk,exe,dll,dsp,sln,vcproj,dsw,opt,plg,ncb,suo,user,log,m4,bak
  EXCLUDED_DIRECTORIES=compiled,dependencies
  EXCLUDED_DIRPARTS=.svn
  EXCLUDED_FILEMATCH=*.,.*
  PREFIX=..
  OUTPATH=$SBROOT/vcproj

  # create the vcproj directory if it does not exist

  if [ ! -e $OUTPATH ]; then
    echo "vcproj directory not found, creating it."
    mkdir $OUTPATH
  fi

  # create the dummy vc project

  ./mkvcproj.exe $SWITCH -outfiles:$PROJECTFILE -project:Songbird -root:$SBROOT -prefix:$PREFIX -outpath:$OUTPATH -e:$EXCLUDED_EXTENSIONS -d:$EXCLUDED_DIRECTORIES -p:$EXCLUDED_DIRPARTS -m:$EXCLUDED_FILEMATCH

else
  echo "mkvcproj.exe not found in the current directory"
fi

