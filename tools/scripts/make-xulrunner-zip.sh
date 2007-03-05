#!/bin/sh

notice() {
  echo $* 1>&2
}

if [ $# != 3 ]; then
  notice "usage: make-xulrunner-zip.sh [mozilla-bin-dir] [songbird-zip-dest-dir] [target-zip-name]"
  exit 1
fi

srcdir="$1"
temp1=`dirname "$srcdir"`
temp2=`basename "$srcdir"`
bindir="`cd \"$temp1\" 2>/dev/null && pwd || echo \"$temp1\"`/$temp2"

destdir="$2"
temp1=`dirname "$destdir"`
temp2=`basename "$destdir"`
zipdir="`cd \"$temp1\" 2>/dev/null && pwd || echo \"$temp1\"`/$temp2"

zipname="$3"

zipball="$zipdir/$zipname"

echo cd "$bindir" 
cd "$bindir" 

rm -rf ../xulrunner
mkdir ../xulrunner
cp -a * ../xulrunner
cd ..

notice "creating zip in dest..."
zip -r -9 -v $zipball xulrunner 

notice "done."
