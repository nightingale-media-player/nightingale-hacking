#!/bin/zsh

# break on any error
set -e

# do a debug or a release build? YOU WANT RELEASE!
# if you do a debug build, add the appropriate options
# to your nightingale.config file! 
#...and build a debug xulrunner version
build="release"
buildir="$(pwd)"

# use our own gstreamer libs
for dir in /usr/lib64 /usr/lib /usr/lib/i386-linux-gnu /usr/lib/x86_64-linux-gnu ; do
  if [ -f ${dir}/gstreamer-0.10/libgstcoreelements.so ] ; then
    export GST_PLUGIN_PATH=${dir}/gstreamer\-0.10
    break
  elif [ -f ${dir}/gstreamer0.10/libgstcoreelements.so ] ; then
    export GST_PLUGIN_PATH=${dir}/gstreamer0.10
    break
  fi
done

# hopefully we have python2 on this system
# we can add this bit to the configure.ac as well
# and locate in order of preference
# python2.x python2.x python2 pyton
set +e
PYTHON="$(which python2)"
if [[ $? == 0 ]]; then
 export PYTHON=PYTHON
else
 export PYTHON="$(which python)"
fi
set -e

# fixes a build error
# add to one of the build files...not sure if it's linux specific or not
export CXXFLAGS="-std=gnu++0x -fpermissive"

# use our own gstreamer libs
# this is always necessary for linux builds using system libs
# unless we make it default, then we'll want to make an inverse rule
# and remove this one
grep -sq gstreamer-system songbird.config || ( echo 'ac_add_options --with-media-core=gstreamer-system' >> songbird.config )

make -f songbird.mk clobber
make -f songbird.mk
