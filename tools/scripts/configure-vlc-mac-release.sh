#!/bin/sh
#./bootstrap && \
# Note:  can't compile with --disable-mkv
./configure \
  --enable-release \
  --enable-strip \
  --disable-debug \
  --enable-mozilla \
  --with-mozilla-sdk-path=./extras/contrib/src/gecko-sdk \
  --enable-sdl \
  --enable-visual \
  --enable-ffmpeg \
  --enable-mad --disable-a52 \
  --enable-faad --enable-flac --enable-vorbis --enable-ogg --enable-speex \
  --enable-freetype --enable-fribidi \
  --disable-activex \
  --disable-hal \
  --disable-gtk \
  --disable-wxwidgets --disable-x11 --disable-xvideo --disable-glx \
  --disable-cddax --disable-vcdx --disable-goom \
  --disable-twolame \
  --disable-libmpeg2 \
  --disable-skins2 --disable-skins \
  --disable-dvdread \
  --disable-dvdnav --disable-screen \
  --disable-bonjour --disable-joystick --disable-httpd \
  --disable-vlm --disable-gnutls --disable-dvbpsi \
  --disable-cdda --disable-libcddb --disable-png  \
  --disable-alsa  \
  --disable-cmml \
  --disable-mod --disable-mpc \
  --disable-dts --disable-x264 --disable-h264 --disable-real --disable-realrtsp  \
  --disable-libtool \
