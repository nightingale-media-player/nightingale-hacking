#!/bin/bash

# break on any error
set -e

# do a debug or a release build? YOU WANT RELEASE!
# if you do a debug build, add the appropriate options
# to your nightingale.config file!
build="release"
buildir="$(pwd)"
version=1.11
# we'll use wget as default. OS without wget should override this.
DOWNLOADER="wget"

function md5_verify() {
  function md5_fail() {
    echo "-------------------------------------------------------------------------------"
    echo "WARNING: MD5 checksum verification failed: $1"
    echo "It is a safety risk to continue unless you know exactly what you're doing."
    echo "Answer yes to delete it and retry the build process, or no to continue anyway."
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
    arch=$(uname -m)
    depdirn="linux-$arch"
    patch=1
    
    cd dependencies
    
    if [ ! -f "$depdirn-$version.tar.lzma" ] ; then
      $DOWNLOADER "https://downloads.sourceforge.net/project/ngale/$version-Build-Deps/$arch/$depdirn-$version.tar.lzma"
      md5_verify "$depdirn-$version.tar.lzma"
      rm -rf "$depdirn" &> /dev/null
      tar xvf "$depdirn-$version.tar.lzma"
    fi
    cd ../
    
    # use our own gstreamer libs
    for dir in /usr/lib64 /usr/lib ; do
      if [ -f ${dir}/gstreamer-0.10/libgstcoreelements.so ] ; then
        export GST_PLUGIN_PATH=${dir}/gstreamer\-0.10
        break
      elif [ -f ${dir}/gstreamer0.10/libgstcoreelements.so ] ; then
        export GST_PLUIN_PATH=${dir}/gstreamer0.10
        break
      fi
    done
    
    # !!!! NOTICE: comment the below out if building on/for Windows or Mac or playback probably won't work !!!!
    if [ -f nightingale.config ] ; then
      rm nightingale.config
    fi
    echo 'ac_add_options --with-media-core=gstreamer-system' >> nightingale.config
    ;;
  msys*)
    depdirn="windows-i686"
    
    # Ensure line endings, as git might have converted them
    tr -d '\r' < ./components/library/localdatabase/content/schema.sql > tmp.sql
    rm ./components/library/localdatabase/content/schema.sql
    mv tmp.sql ./components/library/localdatabase/content/schema.sql
    
    cd dependencies
    
    if [ ! -f "$depdirn-$version.tar.lzma" ] ; then
      $DOWNLOADER "https://downloads.sourceforge.net/project/ngale/$version-Build-Deps/i686/$depdirn-$version.tar.lzma"
      md5_verify "$depdirn-$version.tar.lzma"
      rm -rf "$depdirn" &> /dev/null
      mkdir "$depdirn"
      tar --lzma -xvf "$depdirn-$version.tar.lzma" -C "$depdirn"
    fi
    cd ../
    ;;
  darwin*)
    # no wget on OSX, use curl
    DOWNLOADER="curl -L -O"
    depdirn="macosx-i686"
    arch_flags="-m32 -arch i386"
    export CFLAGS="$arch_flags" 
    export CXXFLAGS="$arch_flags" 
    export CPPFLAGS="$arch_flags"
    export LDFLAGS="$arch_flags" 
    export OBJCFLAGS="$arch_flags"

    echo 'ac_add_options --with-macosx-sdk=/Developer/SDKs/MacOSX10.4u.sdk' > nightingale.config
    
    cd dependencies
    
    if [ ! -d "$depdirn" ] ; then
      mkdir "$depdirn"
    fi
    
   if [ ! -f "$depdirn-$version.tar.bz2" ] ; then
      $DOWNLOADER "https://downloads.sourceforge.net/project/ngale/$version-Build-Deps/$depdirn/$depdirn-$version.tar.bz2"
      md5_verify "$depdirn-$version.tar.bz2"
      tar -xvf "$depdirn-$version.tar.bz2" -C "$depdirn"
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
  $DOWNLOADER "https://downloads.sourceforge.net/project/ngale/$version-Build-Deps/vendor-$version.zip"
  md5_verify "vendor-$version.zip"

  rm -rf vendor &> /dev/null
  unzip "vendor-$version.zip"
fi

cd ../
cd $buildir

# hopefully we have python2 on this system
export PYTHON="$(which python2 2>/dev/null || which python)"

make -f nightingale.mk clobber
make -f nightingale.mk

echo "Build finished!"
