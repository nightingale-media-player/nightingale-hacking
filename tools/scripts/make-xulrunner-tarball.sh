#!/bin/bash

notice() {
  echo $* 1>&2
}

if [ $# != 3 ]; then
  notice "usage: make-xulrunner-tarball.sh [mozilla-bin-dir] [songbird-tarball-dest-dir] [target-tarball-name]"
  exit 1
fi

srcdir="$1"
temp1=`dirname "$srcdir"`
temp2=`basename "$srcdir"`
bindir="`cd \"$temp1\" 2>/dev/null && pwd || echo \"$temp1\"`/$temp2"

destdir="$2"
temp1=`dirname "$destdir"`
temp2=`basename "$destdir"`
tarballdir="`cd \"$temp1\" 2>/dev/null && pwd || echo \"$temp1\"`/$temp2"

tarballname="$3"

tarball="$tarballdir/$tarballname"

echo cd "$bindir" 
cd "$bindir" 

notice "creating tarball in dest..."
tar czvhf $tarball *

notice "done."
