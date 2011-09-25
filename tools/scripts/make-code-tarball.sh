#!/bin/sh

currentdir=`pwd`

notice() {
  echo $* 1>&2
}

if [ $# != 1 ]; then
  notice "usage: make-code-tarball.sh [code-tarball-destination-dir]"
  exit 1
fi

destdir="$1"
tempdir=${destdir}/.__temp
currenttime=`date +%Y%m%d`
tarballname=nightingale_snapshot_${currenttime}.tar.gz

notice "Creating destination directory: ${destdir}"
mkdir ${destdir}

notice "Creating temporary directory: ${tempdir}"
mkdir ${tempdir}

notice "Checking out nightingale/trunk from public svn to ${tempdir}/nightingale/trunk"
svn co svn://publicsvn.getnightingale.com/nightingale/client/trunk ${tempdir}/nightingale/trunk

notice "Creating gzipped tarball for nightingale/trunk here: ${destdir}/${tarballname}"
cd ${tempdir}
tar -f ${destdir}/${tarballname} -cz nightingale/trunk

notice "Deleting temporary directory...."
cd ${currentdir}
rm -rf ${tempdir}

notice "Done... you should go into ${destdir} to make sure your tarball '${tarballname}' was created properly."

