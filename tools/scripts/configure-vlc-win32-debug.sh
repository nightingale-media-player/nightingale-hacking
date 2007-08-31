#!/bin/sh
#./bootstrap && \
  CPPFLAGS="-I/usr/win32/include -I/usr/win32/include/ebml" \
  LDFLAGS=-L/usr/win32/lib \
  PKG_CONFIG_PATH=/usr/win32/lib/pkgconfig \
  CC="gcc -mno-cygwin" CXX="g++ -mno-cygwin" \
  ./configure \
      --enable-sdl --with-sdl-config-path=/usr/win32/bin --disable-gtk \
      --disable-activex \
      --enable-nls \
      --enable-ffmpeg --with-ffmpeg-mp3lame --with-ffmpeg-faac \
      --with-ffmpeg-zlib --enable-faad --enable-flac --enable-theora \
      --with-wx-config-path=/usr/win32/bin \
      --with-freetype-config-path=/usr/win32/bin \
      --with-fribidi-config-path=/usr/win32/bin \
      --enable-live555 --with-live555-tree=/usr/win32/live.com \
      --enable-caca --with-caca-config-path=/usr/win32/bin \
      --with-xml2-config-path=/usr/win32/bin \
      --with-dvdnav-config-path=/usr/win32/bin \
      --disable-cddax --disable-vcdx --enable-goom \
      --enable-twolame --disable-dvdread \
      --disable-gnomevfs \
      --disable-dts \
      --enable-mozilla \
      --with-mozilla-sdk-path=/usr/win32/gecko-sdk \
      --enable-debug \
      --disable-release \
