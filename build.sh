#!/bin/bash

# break on any error
set -e

# do a debug or a release build? YOU WANT RELEASE!
# if you do a debug build, add the appropriate options
# to your nightingale.config file!
build="release"
buildir="$(pwd)"
version=1.12

# This variable is used to look into the mozilla dependencies to see
# which versions (build/release) have been extracted.
mozdepver="mozilla-1.9.2"

count=`grep '\-\-enable\-debug' nightingale.config|wc -l`
if [ $count -ne 0 ] ; then
    build="debug"
fi

echo "You are building a $build build"

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
        md5 -r "$1" | grep -q -f "$1.md5" || md5_fail "$1"
    else
        md5sum -c --status "$1.md5" || md5_fail "$1"
    fi
}

# Check for the build deps for the system's architecture and OS
case $OSTYPE in
    linux*)
        case "$(uname -m)" in
            *64*) arch=x86_64 ;;
            *86)    arch=i686 ;;
            *) echo "Unknown arch" >&2 ; exit 1 ;;
        esac
        depdirn="linux-$arch"
        patch=1
        version=1.12
        # Nightingale dependency build date, change if the linux deps change.
        depdate="20130316"
        fname="$depdirn-$version-$depdate-$build-final.tar.lzma"

        export CXXFLAGS="-O2 -fomit-frame-pointer -pipe -fpermissive"

        echo "linux $arch"
        ( cd dependencies && {
            if [ ! -f "$fname" ] ; then
                # We want the new deps instead of the old ones...
                rm -rf "$depdirn"
                download "http://downloads.sourceforge.net/project/ngale/$version-Build-Deps/$fname"
            fi
            if [ ! -d "$depdirn/$mozdepver/$build" ] ; then
                if [ -f "$fname.md5" ] ; then
                    md5_verify "$fname"
                fi
                echo "Need to extract $fname"
                tar xvf "$fname"
            fi
        } ; )

        # https://en.wikipedia.org/wiki/List_of_Linux_distributions#Debian-based
        debianbased="buntu|Debian|LMDE|Mint|gNewSense|Fuduntu|Solus|CrunchBang|Peppermint|Deepin|Kali|Trisquel|elementary|Knoppix"
        # the below needs to be nested...in my testing it won't work otherwise
        if [[ $(grep -i -E $debianbased /etc/issue) ||
              $(grep -i -E $debianbased /etc/lsb-release) ||
              $(grep -i -E $debianbased /etc/os-release) ]] ; then
            grep -q -E 'taglib' nightingale.config || \
            echo -e 'ac_add_options --with-taglib-source=packaged\n' >> nightingale.config
        fi
        ;;
    msys*)
        depdirn="windows-i686"
        # Nightingale dependency build date, change if the msys deps change.
        depdate="20130121"

        # Ensure line endings, as git might have converted them
        tr -d '\r' < ./components/library/localdatabase/content/schema.sql > tmp.sql
        rm ./components/library/localdatabase/content/schema.sql
        mv tmp.sql ./components/library/localdatabase/content/schema.sql

        cd dependencies

        fname="$depdirn-$version-$depdate-$build.tar.lzma"

        if [ ! -f "$fname" ] ; then
            # We want the new deps instead of the old ones...
            rm -rf "$depdirn"
            download "http://downloads.sourceforge.net/project/ngale/$version-Build-Deps/$fname"
        fi

        if [ ! -d "$depdirn/$mozdepver/$build" ] ; then
            if [ -f "$fname.md5" ] ; then
                md5_verify "$fname"
            fi
            mkdir "$depdirn"
            tar --lzma -xvf "$fname" -C "$depdirn"
        fi
        cd ../
        ;;
    darwin*)
        depdirn="macosx-i686"
        # Nightingale dependency build date, change if the darwin deps change.
        depdate="20140419"
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

        # NOTE: The version is 1.12.1, hence the ".1" appendage here
        fname="$depdirn-$version.1-$depdate.tar.bz2"

        if [ ! -f "$fname" ] ; then
            # We want the new deps instead of the old ones...
            rm -rf "$depdirn"
            download "http://downloads.sourceforge.net/project/ngale/$version-Build-Deps/$fname"
        fi

        if [ ! -d "$depdirn/$mozdepver/$build" ] ; then
            if [ -f "$fname.md5" ] ; then
                md5_verify "$fname"
            fi
            tar xvf "$fname"
        fi

        cd ../
        ;;
    *)
        echo "Can't find deps for $OSTYPE. You may need to build them yourself. Doublecheck the SVN's for \n
        Nightingale trunk to be sure."
        ;;
esac

cd $buildir

make clobber
rm -rf compiled &> /dev/null #sometimes clobber doesn't nuke it all
make

sed -i 's/0\.10/1\.0/g' nightingale

echo "Build Succeeded!"
