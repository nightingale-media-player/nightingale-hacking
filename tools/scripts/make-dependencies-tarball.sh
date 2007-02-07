#!/bin/sh

currentdir=`pwd`

notice() {
  echo $* 1>&2
}

if [ $# != 1 ]; then
  notice "usage: make-dependencies-tarball.sh [deps-tarball-destination-dir]"
  exit 1
fi

destdir="$1"
tempdir=${destdir}/.__temp
currenttime=`date +%Y%m%d`
tarballbasename=dependencies_snapshot
tarballname_windows=${tarballbasename}-windows-${currenttime}.tar.gz
tarballname_linux=${tarballbasename}-linux-${currenttime}.tar.gz
tarballname_macosx=${tarballbasename}-macosx-${currenttime}.tar.gz
tarballname_macosxintel=${tarballbasename}-macosx.intel-${currenttime}.tar.gz

notice "Creating destination directory: ${destdir}"
mkdir ${destdir}

notice "Creating temporary directory: ${tempdir}"
mkdir ${tempdir}

notice "Checking out vendor-binaries/windows-i686 from public svn to ${tempdir}/windows-i686"
svn co svn://publicsvn.songbirdnest.com/songbird/client/vendor-binaries/windows-i686 ${tempdir}/windows-i686

notice "Creating gzipped tarball for windows-i686 here: ${destdir}/${tarballname_windows}"
cd ${tempdir}
tar -f ${destdir}/${tarballname_windows} -cz windows-i686
rm -rf windows-i686

notice "Checking out vendor-binaries/linux from public svn to ${tempdir}/linux"
svn co svn://publicsvn.songbirdnest.com/songbird/client/vendor-binaries/linux ${tempdir}/linux

notice "Creating gzipped tarball for linux here: ${destdir}/${tarballname_linux}"
cd ${tempdir}
tar -f ${destdir}/${tarballname_linux} -cz linux
rm -rf linux

notice "Checking out vendor-binaries/macosx from public svn to ${tempdir}/macosx"
svn co svn://publicsvn.songbirdnest.com/songbird/client/vendor-binaries/macosx ${tempdir}/macosx

notice "Creating gzipped tarball for macosx here: ${destdir}/${tarballname_macosx}"
cd ${tempdir}
tar -f ${destdir}/${tarballname_macosx} -cz macosx
rm -rf macosx

notice "Checking out vendor-binaries/macosx.intel from public svn to ${tempdir}/macosx"
svn co svn://publicsvn.songbirdnest.com/songbird/client/vendor-binaries/macosx.intel ${tempdir}/macosx

notice "Creating gzipped tarball for macosx.intel here: ${destdir}/${tarballname_macosxintel}"
cd ${tempdir}
tar -f ${destdir}/${tarballname_macosxintel} -cz macosx
rm -rf macosx

notice "Deleting temporary directory...."
cd ${currentdir}
rm -rf ${tempdir}

notice "Done... you should go into ${destdir} to make sure all the tarballs were created properly."

