#!/bin/bash

# bin_files are relative to $objdir/dist/bin/
bin_files="regxpcom*
           xpidl*
           xpt_dump*
           xpt_link*
"

# lib_files are relative to $objdir/dist/lib/
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

# update_bin_files are relative to $objdir/dist/host/bin/
update_bin_files="*mar*
                  *bsdiff*
"

# update_script_files are relative to $srcdir/tools/update-packaging/
update_script_files="common.sh
                     make_full_update.sh
                     make_incremental_update.sh
                     unwrap_full_update.sh
                     unwrap_full_update.pl
"

# build_scripts are relative to $srcdir
build_script_files="build/cygwin-wrapper
                    build/autoconf/acoutput-fast.pl
                    build/autoconf/make-makefile
                    build/macosx/universal/fix-buildconfig
                    build/macosx/universal/unify
                    build/package/mac_osx/make-diskimage
                    build/package/mac_osx/mozilla.dsstore
                    config/make-jars.pl
                    config/mozLock.pm
                    config/preprocessor.pl
"

notice() {
  echo $* 1>&2
}

if [ $# != 3 ]; then
  notice "usage: make-mozilla-sdk.sh [mozilla-src-dir] [mozilla-obj-dir] [songbird-sdk-dir]"
  exit 1
fi

relsrcdir="$1"
temp1=`dirname "$relsrcdir"`
temp2=`basename "$relsrcdir"`
srcdir="`cd \"$temp1\" 2>/dev/null && pwd || echo \"$temp1\"`/$temp2"

relobjdir="$2"
temp1=`dirname "$relobjdir"`
temp2=`basename "$relobjdir"`
objdir="`cd \"$temp1\" 2>/dev/null && pwd || echo \"$temp1\"`/$temp2"

distdir="$objdir/dist"

relsdkdir="$3"
temp1=`dirname "$relsdkdir"`
temp2=`basename "$relsdkdir"`
sdkdir="`cd \"$temp1\" 2>/dev/null && pwd || echo \"$temp1\"`/$temp2"

mkdir -p "$sdkdir"

notice "copying binary files..."
cd "$sdkdir" && mkdir -p bin
cd "$distdir/bin" && cp -Lfp $bin_files "$sdkdir/bin"
cd "$distdir/host/bin" && cp -Lfp $update_bin_files "$sdkdir/bin"

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

notice "copying scripts..."
cd "$sdkdir" && mkdir -p scripts
cd "$srcdir" && cp -Lfp $build_script_files "$sdkdir/scripts"
cd "$srcdir/tools/update-packaging" && cp -Lfp $update_script_files "$sdkdir/scripts"

notice "done."
