#!/bin/bash

notice() {
  echo $* 1>&2
}

if [ $# != 2 ]; then
  notice "usage: make-mozilla-sdk.sh mozilla-build-dist-dir songbird-sdk-dest-dir"
  exit 1
fi

bin_files="regxpcom* \
           xpidl* \
           xpt_dump* \
           xpt_link* \
"

lib_files="*embed_base.* \
           *nspr4.* \
           *nspr4_s.* \
           *plc4.* \
           *plc4_s.* \
           *plds4.* \
           *plds4_s.* \
           *unicharutil_s.* \
           *xpcom.* \
           *xpcomglue.* \
           *xpcomglue_s.* \
           *xul.* \
           *XUL* \
"

notice "copying binary files..."
cd "$sdkdir" && mkdir -p bin
cd "$distdir"/bin && cp -Lfp "$bin_files" "$sdkdir"/bin

notice "copying library files..."
cd "$sdkdir" && mkdir -p lib
cd "$distdir"/lib && cp -Lfp "$lib_files" "$sdkdir"/lib

notice "copying include files..."
cd "$sdkdir" && mkdir -p include
cd "$distdir"/include && cp -RLfp * "$sdkdir"/indlude

notice "copying binary files..."
cd "$sdkdir" && mkdir -p idl
cd "$distdir"/idl && cp -RLfp * "$sdkdir"/idl

notice "copying frozen sdk..."
cd "$sdkdir" && mkdir -p frozen
cd "$distdir"/sdk && cp -RLfp * "$sdkdir"/frozen
