# this depends on your system's gstreamer location
export GST_PLUGIN_PATH=/usr/lib/gstreamer-0.10

# i think this is archlinux specific
export PYTHON=/usr/bin/python2

# this fixes a build error, let's toss it into the makefile(s) eventually
export CXXFLAGS=-std=gnu++0x

make -f songbird.mk clobber
make -f songbird.mk
