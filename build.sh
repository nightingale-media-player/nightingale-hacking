#!/bin/bash

# break on any error
set -e

# do a debug or a release build? YOU WANT RELEASE!
# if you do a debug build, add the appropriate options
# to your nightingale.config file! 
#...and build a debug xulrunner version
build="release"
buildir="$(pwd)"

# The xulrunner version we're using
export XUL=6.0.2

# this depends on your system's gstreamer location
# this should be added to configure.ac and we should
# make system gstreamer default on linux
for dir in /usr/lib64 /usr/lib ; do
	if [ -f ${dir}/gstreamer-0.10/libgstcoreelements.so ] ; then
		export GST_PLUGIN_PATH=${dir}/gstreamer-0.10
		break
	elif [ -f ${dir}/gstreamer0.10/libgstcoreelements.so ] ; then
		export GST_PLUIN_PATH=${dir}/gstreamer0.10
		break
	fi
done

# hopefully we have python2 on this system
# we can add this bit to the configure.ac as well
# and locate in order of preference
# python2.x python2.x python2 pyton
export PYTHON="$(which python2 2>/dev/null || which python)"

# fixes a build error
# add to one of the build files...not sure if it's linux specific or not
export CXXFLAGS="-std=gnu++0x"

# use our own gstreamer libs
# this is always necessary for linux builds using system libs
# unless we make it default, then we'll want to make an inverse rule
# and remove this one
grep -sq gstreamer-system songbird.config || ( echo 'ac_add_options --with-media-core=gstreamer-system' >> songbird.config )

make -f songbird.mk clobber
make -f songbird.mk
