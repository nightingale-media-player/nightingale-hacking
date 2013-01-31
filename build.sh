#!/bin/bash

# break on any error
set -e

# do a debug or a release build? YOU WANT RELEASE!
# if you do a debug build, add the appropriate options
# to your nightingale.config file!
build="release"
buildir="$(pwd)"
version=1.12

download() {
  if which wget &>/dev/null ; then
    wget "$1"
  elif which curl &>/dev/null ; then
    curl --location -o "${1##*/}" "$1"
  else
    echo "Failed to find downloader to fetch $1" >&2
    exit 1
  fi
}

md5_verify() {
  md5_fail() {
    echo "-------------------------------------------------------------------------------"
    echo "WARNING: MD5 checksum verification failed: $1"
    echo "It is a safety risk to continue unless you know exactly what you're doing."
    echo "Answer no to delete it and retry the build process, or yes to continue anyway."
    read -p "Continue? (y/n) [n]" ans
    case $ans in
      "y" | "Y")
        echo "Checksum ignored."
        ;;
      "n" | "N" | "")
        rm "$1"
        exit 1
        ;;
      *)
        echo "Invalid input."
        md5_fail $1
        ;;
    esac
  }

  if [ $depdirn = "macosx-i686" ] ; then
    md5 -r "$1"|grep -q -f "$1.md5" || md5_fail "$1"
  else
    md5sum -c --status "$1.md5" || md5_fail "$1"
  fi
}

# Check for the build deps for the system's architecture and OS
case $OSTYPE in
  linux*)
    case "$(uname -m)" in
      *64*) arch=x86_64 ;;
      *86)  arch=i686 ;;
      *) echo "Unknown arch" >&2 ; exit 1 ;;
    esac
    depdirn="linux-$arch"
    patch=1
    version=1.12
    #if you have a dep built on a differing date for either arch, just use a conditional to set this
    depdate=20130101
    export CXXFLAGS="-O2 -fomit-frame-pointer -pipe -fpermissive"

    echo "linux $arch"
    ( cd dependencies && {
		if [ ! -d "$depdirn" ] ; then
			if [ ! -f "$depdirn-$version-$depdate-release.tar.lzma" ] ; then
				download "http://downloads.sourceforge.net/project/ngale/$version-Build-Deps/$depdirn-$version-$depdate-release.tar.lzma"
				md5_verify "$depdirn-$version-$depdate-release.tar.lzma"
			fi
			tar xvf "$depdirn-$version-$depdate-release.tar.lzma"
		fi
	} ; )
    
    # use our own gstreamer libs
    for dir in /usr/lib /usr/lib64 /usr/lib/${arch}-linux-gnu ; do
      if [ -f ${dir}/gstreamer-0.10/libgstcoreelements.so ] ; then
        export GST_PLUGIN_PATH=${dir}/gstreamer-0.10
        break
      elif [ -f ${dir}/gstreamer0.10/libgstcoreelements.so ] ; then
        export GST_PLUGIN_PATH=${dir}/gstreamer0.10
        break
      fi
    done
    
    [ -f nightingale.config ] || touch nightingale.config
    grep -q -E 'ac_add_options\s+--with-media-core=gstreamer-system' nightingale.config || echo -e 'ac_add_options --with-media-core=gstreamer-system\n' >> nightingale.config
    ;;
  msys*)
    depdirn="windows-i686"
    # Nightingale version number and dependency version, change if the deps change.
    version=1.12
    depversion="20130121-release"
    
    # Ensure line endings, as git might have converted them
    tr -d '\r' < ./components/library/localdatabase/content/schema.sql > tmp.sql
    rm ./components/library/localdatabase/content/schema.sql
    mv tmp.sql ./components/library/localdatabase/content/schema.sql
    
    cd dependencies
    
    if [ ! -f "$depdirn-$version-$depversion.tar.lzma" ] ; then
      # We want the new deps instead of the old ones...
      rm -rf "$depdirn"
      download "http://downloads.sourceforge.net/project/ngale/$version-Build-Deps/$depdirn-$version-$depversion.tar.lzma"
    fi
    
    if [ ! -d "$depdirn" ] ; then
      md5_verify "$depdirn-$version-$depversion.tar.lzma"
      mkdir "$depdirn"
      tar --lzma -xvf "$depdirn-$version-$depversion.tar.lzma" -C "$depdirn"
    fi
    cd ../    
    ;;
  darwin*)
    depdirn="macosx-i686"
    depversion="20130130"
    arch_flags="-m32 -arch i386"
    export CFLAGS="$arch_flags" 
    export CXXFLAGS="$arch_flags" 
    export CPPFLAGS="$arch_flags"
    export LDFLAGS="$arch_flags" 
    export OBJCFLAGS="$arch_flags"
    export CC="gcc"
    export CXX="g++"

    echo 'ac_add_options --with-macosx-sdk=/Developer/SDKs/MacOSX10.5.sdk' > nightingale.config
    echo 'ac_add_options --enable-installer' >> nightingale.config
    echo 'ac_add_options --enable-official' >> nightingale.config
    echo 'ac_add_options --enable-compiler-environment-checks=no' >> nightingale.config
    
    cd dependencies

    if [ ! -f "$depdirn-$version-$depversion-$build.tar.bz2" ] ; then
      # We want the new deps instead of the old ones...
      rm -rf "$depdirn"
      download "http://downloads.sourceforge.net/project/ngale/$version-Build-Deps/$depdirn-$version-$depversion-$build.tar.bz2"
    fi
    
    if [ ! -d "$depdirn" ] ; then
      md5_verify "$depdirn-$version-$depversion-$build.tar.bz2"
      tar xvf "$depdirn-$version-$depversion-$build.tar.bz2"
    fi

    cd ../
    ;;
  *)
    echo "Can't find deps for $OSTYPE. You may need to build them yourself. Doublecheck the SVN's for \n
    Songbird and Nightingale trunks to be sure."
    ;;
esac

# get the vendor build deps...
cd dependencies

if [ ! -f "vendor-$version.zip" ] ; then
  #We want the new deps instead of the old ones...
  rm -rf "vendor"
  download "http://downloads.sourceforge.net/project/ngale/$version-Build-Deps/vendor-$version.zip"
  md5_verify "vendor-$version.zip"
fi

if [ ! -d "vendor" ] ; then
	rm -rf vendor &> /dev/null
	unzip "vendor-$version.zip"
fi

cd ../
cd $buildir

make -f nightingale.mk clobber
rm -rf compiled &> /dev/null #sometimes clobber doesn't nuke it all
make -f nightingale.mk || echo "Build Error!" && exit 1

echo "Build Succeeded!" && exit 0
