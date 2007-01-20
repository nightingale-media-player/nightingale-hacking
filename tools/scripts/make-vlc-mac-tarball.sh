#!/bin/sh

notice() {
  echo $* 1>&2
}

if [ $# != 1 ]; then
  notice "Tars VLC Plugin.plugin from /builds/vlc/ and copies it to the given folder"
  notice "usage: make-vlc-mac-tarball.sh [songbird-dir]"
  exit 1
fi

$pwd = `pwd`
cd /builds/vlc/mozilla
rm VLC\ Plugin.tar.gz
tar cvfz VLC\ Plugin.tar.gz VLC\ Plugin.plugin --exclude=*.svn*
cp VLC\ Plugin.tar.gz $1
cd $pwd
