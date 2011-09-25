#!/bin/sh
#./bootstrap && \
# Note:  can't compile with --disable-mkv
./configure \
  --disable-release \
  --disable-strip \
  --enable-mozilla \
  --with-mozilla-sdk-path=./extras/contrib/src/gecko-sdk \
  --enable-debug --enable-sdl --enable-mad \
  --enable-libdvbpsi --enable-a52 --disable-dvdplay \
  --disable-dvdnav --disable-dvdread --enable-ffmpeg \
  --enable-faad --enable-flac --enable-vorbis \
  --enable-speex --enable-theora --enable-ogg \
  --disable-shout --enable-cddb --disable-cddax \
  --enable-vcdx \
  --disable-skins --disable-skins2 --disable-wxwidgets \
  --enable-freetype --enable-fribidi --enable-caca \
  --enable-live555 --enable-dca --enable-goom \
  --enable-modplug --enable-gnutls --enable-daap \
  --enable-ncurses --enable-libtwolame --enable-x264 \
  --enable-png --disable-realrtsp --enable-lua --disable-libtool \
  --disable-x11 --disable-xvideo --disable-glx 
