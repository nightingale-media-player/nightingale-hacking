#!/bin/sh

# Ugh. So stupid.
# Mac OS 10.4, which is what's on the buildbots, uses bash 2.05,
# which doesn't have a builtin version of which. It calls /usr/bin/which, which
# will stupidly 1. Not use proper return values (so the "which foo && 
# do_something" construct fails), and 2. print everything to stderr, no matter
# what, so doing cute tricks like test -z `which gtar 2>/dev/null` don't work
# either.
#
# We know OS X has tar, so just ignore gtar detection on the mac... for now...
#

TAR=tar
if [ `uname` != "Darwin" ];  then
  which gtar 2>&1 >/dev/null && \
    TAR=gtar
fi

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
$TAR czvhf $tarball *

notice "done."
