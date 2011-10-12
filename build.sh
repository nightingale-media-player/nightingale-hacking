# break on any error
set -e

# do a debug or a release build? YOU WANT RELEASE!
# if you do a debug build, add the appropriate options
# to your nightingale.config file!
build="release"

# Check for the build deps for the system's architecture and OS
case $OSTYPE in
	linux*)
		depdirn="linux-$arch"
		svnroot="http://ngale.svn.sourceforge.net/svnroot/ngale/branches/dependencies/Nightingale1.8/$depdirn"
	    arch=`uname -m`
	    
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
		echo "You should comment out line 29 in this build.sh script and rerun it to make sure things work properly!"
		depdirn="windows-i686-msvc8"
		svnroot="http://publicsvn.songbirdnest.com/vendor-binaries/tags/Songbird1.8.1/$depdirn"
		;;
	darwin*)
		echo "You should comment out line 29 in this build.sh script and rerun it to make sure things work properly!"
		depdirn="macosx-i686"
		svnroot="http://publicsvn.songbirdnest.com/vendor-binaries/tags/Songbird1.8.1/$depdirn"
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
	mkdir -p "$depdirn"/{xulrunner/release,mozilla/release,taglib/release,sqlite/release}
		
	cd "$depdirn/mozilla/release"
	svn co "$svnroot/mozilla/$build" ./
	
	cd ../../../
	
	cd "$depdirn/xulrunner/release"
	svn co "$svnroot/xulrunner/$build" ./
	
	cd ../../../
	
	cd "$depdirn/taglib/release"
	svn co "$svnroot/taglib/$build" ./
	
	cd ../../../
	
	cd "$depdirn/sqlite/release"
	svn co "$svnroot/sqlite/$build" ./
	
	cd ../../../../
fi

# hopefully we have python2 on this system
export PYTHON="$(which python2 2>/dev/null || which python)"

make -f nightingale.mk clobber
make -f nightingale.mk

# insert a copy of the above code to locate gstreamer libs on ngale launch so we don't have to symlink anymore
if [ $OSTYPE="linux-gnu" ] ; then
	patch -Np0 -i add_search_for_gst_libs.patch "compiled-$build-$arch/dist/nightingale"
fi

echo "Build finished!"
