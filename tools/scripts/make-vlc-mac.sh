#!/bin/sh

notice() {
  echo $* 1>&2
}

if [ $# != 1 ]; then
  notice "Configures and builds VLC from /builds/vlc/ and copies it into dependencies/"
  notice "Note:  Expects to be run from tools/scripts/"
  notice "Usage: make-vlc-mac.sh [debug|release]"
  exit 1
fi

pwd=`pwd`
cd ../../
base=`pwd`
pluginroot="$base/dependencies/macosx-i686/plugins/vlc"
scripts="$base/tools/scripts"

echo "Building /builds/vlc and copying to $pluginroot/$1/"

cd /builds/vlc/

make clean
$scripts/configure-vlc-mac-$1.sh
make
$scripts/make-vlc-mac-tarball.sh $pluginroot/$1/

cd $pwd
