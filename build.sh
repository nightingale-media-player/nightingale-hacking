#!/bin/bash

# break on any error
set -e

# do a debug or a release build? YOU WANT RELEASE!
# if you do a debug build, add the appropriate options
# to your nightingale.config file!
build="release"
buildir="$(pwd)"

# Check for the build deps for the system's architecture and OS
case $OSTYPE in
	linux*)
		arch=`uname -m`
		depdirn="linux-$arch"
		svnroot="http://ngale.svn.sourceforge.net/svnroot/ngale/branches/dependencies/Nightingale1.8/$depdirn"
		# msys error workaround
		patch=1
        deps=(  mozilla
                sqlite
                taglib
                xulrunner)
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
		grep -sq gstreamer-system nightingale.config || ( echo 'ac_add_options --with-media-core=gstreamer-system' >> nightingale.config )
		;;
	msys*)
		echo "You should comment out line 31 in this build.sh script and rerun it to make sure things work properly!"
		tr -d '\r' < ./components/library/localdatabase/content/schema.sql > tmp.sql
		rm ./components/library/localdatabase/content/schema.sql
		mv tmp.sql ./components/library/localdatabase/content/schema.sql
		depdirn="windows-i686-msvc8"
		svnroot="http://publicsvn.songbirdnest.com/vendor-binaries/tags/Songbird1.8.1/$depdirn"
        # I'm not sure we need all these, but I haven't delved into trimming down the deps we download on windows
        deps=(  flac
                gettext 
                glib 
                gst-plugins-bad 
                gst-plugins-base 
                gst-plugins-good 
                gst-plugins-ugly 
                gstreamer 
                libIDL 
                libgpod 
                libjpeg 
                libogg 
                libtheora 
                libvorbis  
                mozilla
                mozilla-1.9.2
                sqlite
                taglib
                xulrunner
                xulrunner-1.9.2 )
		;;
	darwin*)
		echo "You should comment out line 31 in this build.sh script and rerun it to make sure things work properly!"
		depdirn="macosx-i686"
        # I'm not sure we need all these, but I haven't delved into trimming down the deps we download on osx
		svnroot="http://publicsvn.songbirdnest.com/vendor-binaries/tags/Songbird1.8.1/$depdirn"
        deps=(  flac
                gettext 
                glib 
                gst-plugins-bad 
                gst-plugins-base 
                gst-plugins-good 
                gst-plugins-ugly 
                gstreamer 
                libIDL 
                libgpod 
                libjpeg 
                libogg 
                libtheora 
                libvorbis  
                mozilla
                mozilla-1.9.2
                sqlite
                taglib
                xulrunner
                xulrunner-1.9.2 )
		;;
	*)
		echo "Can't find deps for $OSTYPE. You may need to build them yourself. Doublecheck the SVN's for \n
		Songbird and Nightingale 1.8 to be sure."
		;;
esac

echo "Checking for the required dependencies..."
if [ ! -d "dependencies/$depdirn" ] ; then
	echo "You don't have them...downloading via SVN..."
	
	cd dependencies
    mkdir $depdirn
    cd $depdirn
            
    for dep in ${deps[@]}; do
        mkdir -p "$dep/$build"
        cd "$dep/$build"
        svn co "$svnroot/$dep/$build" ./
        cd ../../
    done
fi

cd $buildir

# hopefully we have python2 on this system
export PYTHON="$(which python2 2>/dev/null || which python)"

make -f songbird.mk clobber
make -f songbird.mk

# insert a copy of the above code to locate gstreamer libs on ngale launch so we don't have to symlink anymore
if [ $patch = 1 ] ; then
	patch -Np0 -i add_search_for_gst_libs.patch "compiled/dist/songbird"
fi

echo "Build finished!"
