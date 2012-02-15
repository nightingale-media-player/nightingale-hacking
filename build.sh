#!/bin/bash

# break on any error
set -e

# do a debug or a release build? YOU WANT RELEASE!
# if you do a debug build, add the appropriate options
# to your nightingale.config file!
build="release"
buildir="$(pwd)"
version=1.11

# Check for the build deps for the system's architecture and OS
case $OSTYPE in
	linux*)
		arch=$(uname -m)
		depdirn="linux-$arch"
		patch=1
				
		cd dependencies
		rm -rf "$depdirn" &> /dev/null
		
		if [ -f "$depdirn-$version.tar.lzma" ] ; then
			tar xvf "$depdirn-$version.tar.lzma"
		else
			wget "https://downloads.sourceforge.net/project/ngale/$version-Build-Deps/$arch/$depdirn-$version.tar.lzma"
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

		tr -d '\r' < ./components/library/localdatabase/content/schema.sql > tmp.sql
		rm ./components/library/localdatabase/content/schema.sql
		mv tmp.sql ./components/library/localdatabase/content/schema.sql
		
		cd dependencies
		
		if [ ! -d "$depdirn" ] ; then
			mkdir "$depdirn"
		fi

		if [ -f "$depdirn-$version.tar.lzma" ] ; then
			tar --lzma -xvf "$depdirn-$version.tar.lzma" -C "$depdirn"
		else
			wget "https://downloads.sourceforge.net/project/ngale/$version-Build-Deps/i686/$depdirn-$version.tar.lzma"
			tar --lzma -xvf "$depdirn-$version.tar.lzma" -C "$depdirn"
		fi
		cd ../
		;;
	darwin*)
		depdirn="macosx-i686"
        echo 'ac_add_options  --with-macosx-sdk=/Developer/SDKs/MacOSX10.6.sdk' >> nightingale.config

        cd dependencies

        if [ ! -d "$depdirn" ] ; then
            mkdir "$depdirn"
        fi

        if [ -f "$depdirn-$version.tar.bz2" ] ; then
            tar --lzma -xvf "$depdirn-$version.tar.bz2" -C "$depdirn"
        else
            curl -L -O "https://downloads.sourceforge.net/project/ngale/$version-Build-Deps/osx/$depdirn-$version.tar.bz2"
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
rm -rf vendor &> /dev/null

if [ -f "vendor-$version.zip" ] ; then
	unzip "vendor-$version.zip"
else
	curl -L -O "https://downloads.sourceforge.net/project/ngale/$version-Build-Deps/vendor-$version.zip"
	unzip "vendor-$version.zip"
fi
cd ../

cd $buildir

# hopefully we have python2 on this system
export PYTHON="$(which python2 2>/dev/null || which python)"

make -f nightingale.mk clobber
make -f nightingale.mk

echo "Build finished!"
