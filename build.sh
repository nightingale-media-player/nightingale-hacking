# this depends on your system's gstreamer location
for dir in /usr/lib64 /usr/lib ; do
  if [ -f ${dir}/gstreamer-0.10/libgstcoreelements.so ] ; then
    export GST_PLUGIN_PATH=${dir}/gstreamer-0.10
    break
  fi
done

# i think this is archlinux specific
export PYTHON=/usr/bin/python2

# this fixes a build error, let's toss it into the makefile(s) eventually
export CXXFLAGS=-std=gnu++0x

make -f songbird.mk clobber
make -f songbird.mk
