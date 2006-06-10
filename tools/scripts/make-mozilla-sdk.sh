#!/bin/bash

bin_files="regxpcom*
           xpidl*
           xpt_dump*
           xpt_link*
"

lib_files="*embed_base.*
           *nspr4.*
           *nspr4_s.*
           *plc4.*
           *plc4_s.*
           *plds4.*
           *plds4_s.*
           *unicharutil_s.*
           *xpcom.*
           *xpcomglue.*
           *xpcomglue_s.*
           *xul.*
           *XUL*
           *mozjs*
"

notice() {
  echo $* 1>&2
}

if [ $# != 2 ]; then
  notice "usage: make-mozilla-sdk.sh mozilla-build-dist-dir songbird-sdk-dest-dir"
  exit 1
fi

reldistdir="$1"
temp1=`dirname "$reldistdir"`
temp2=`basename "$reldistdir"`
distdir="`cd \"$temp1\" 2>/dev/null && pwd || echo \"$temp1\"`/$temp2"

relsdkdir="$2"
temp1=`dirname "$relsdkdir"`
temp2=`basename "$relsdkdir"`
sdkdir="`cd \"$temp1\" 2>/dev/null && pwd || echo \"$temp1\"`/$temp2"

mkdir -p "$sdkdir"

notice "copying binary files..."
cd "$sdkdir" && mkdir -p bin
cd "$distdir/bin" && cp -Lfp $bin_files "$sdkdir/bin"

notice "copying library files..."
cd "$sdkdir" && mkdir -p lib
# some os don't have all these files, so silence errors
cd "$distdir/lib" && cp -Lfp $lib_files "$sdkdir/lib" 2>/dev/null

notice "copying include files..."
cd "$sdkdir" && mkdir -p include
cd "$distdir/include" && cp -RLfp * "$sdkdir/include"

notice "copying idl files..."
cd "$sdkdir" && mkdir -p idl
cd "$distdir/idl" && cp -Lfp * "$sdkdir/idl"

notice "copying frozen sdk..."
cd "$sdkdir" && mkdir -p frozen
cd "$distdir/sdk" && cp -RLfp * "$sdkdir/frozen"

notice "done."
