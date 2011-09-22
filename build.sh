# this depends on your system's gstreamer location
for dir in /usr/lib64 /usr/lib ; do
  if [ -f ${dir}/gstreamer-0.10/libgstcoreelements.so ] ; then
    export GST_PLUGIN_PATH=${dir}/gstreamer-0.10
    break
  fi
done

# hopefully we have python2 on this system
export = PYTHON="$(which python2 2>/dev/null || which python)"

# fixes a build error, let's add to the main makefiles later
export CXXFLAGS="-std=gnu++0x"

# use our own gstreamer libs
grep -sq gstreamer-system songbird.config || ( echo 'ac_add_options --with-media-core=gstreamer-system' >> songbird.config )

make -f songbird.mk clobber
make -f songbird.mk
