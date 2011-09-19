# this depends on your system's gstreamer location
for dir in /usr/lib64 /usr/lib ; do
  if [ -f ${dir}/gstreamer-0.10/libgstcoreelements.so ] ; then
    export GST_PLUGIN_PATH=${dir}/gstreamer-0.10
    break
  fi
done

# this will only build with python2! maybe we'll fix that some day

if [ -f /usr/bin/python2 ] ; then
	export PYTHON=/usr/bin/python2
fi
# otherwise, we'll use whatever version is at /usr/bin/python
# cross your fingers!

# this fixes a build error, let's toss it into the makefiles eventually
export CXXFLAGS=-std=gnu++0x

make -f songbird.mk clobber
make -f songbird.mk
