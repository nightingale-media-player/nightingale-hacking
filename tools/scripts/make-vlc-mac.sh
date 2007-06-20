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
pluginplat=
platform=`uname -p`

case $platform in
  powerpc )
    pluginplat="macosx-ppc"
    ;;
  i386 )
    pluginplat="macosx-i686"
    ;;
esac

pluginroot="$base/dependencies/$pluginplat/plugins/vlc"
scripts="$base/tools/scripts"

echo "Building /builds/vlc and copying to $pluginroot/$1/"

cd /builds/vlc/

make clean
$scripts/configure-vlc-mac-$1.sh
make
$scripts/make-vlc-mac-tarball.sh $pluginroot/$1/

cd $pwd
