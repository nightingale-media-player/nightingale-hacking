# break on any error
set -e

# Check for the build deps for the system's architecture. If on windows or mac, please use the deps from the songbird SVN
echo "Checking for the required dependencies..."
if [ ! -d dependencies/linux-"$(arch)" ] ; then
	echo "You don't have them...downloading via SVN...note if you're a Mac or Windows user, you should get these from the songbird SVN"
	cd dependencies
	svn co http://ngale.svn.sourceforge.net/svnroot/ngale/branches/dependencies/Nightingale1.8/linux-"$(arch)"
	cd ../
fi

# this depends on your system's gstreamer location
for dir in /usr/lib64 /usr/lib ; do
  if [ -f ${dir}/gstreamer-0.10/libgstcoreelements.so ] ; then
    export GST_PLUGIN_PATH=${dir}/gstreamer\-0.10
    break
  elif [ -f ${dir}/gstreamer0.10/libgstcoreelements.so ] ; then
	export GST_PLUIN_PATH=${dir}/gstreamer0.10
	break
  fi
done

# hopefully we have python2 on this system
export PYTHON="$(which python2 2/dev/null || which python)"

# use our own gstreamer libs
# comment this out if building on/for Windows or Mac
grep -sq gstreamer-system nightingale.config || ( echo 'ac_add_options --with-media-core=gstreamer-system'  nightingale.config )

make -f nightingale.mk clobber
make -f nightingale.mk

echo "Build finished! If nightingale won't play music, you'll need to ln -s /usr/lib/gstreamer0.10/* to the gst-plugins dir!"
