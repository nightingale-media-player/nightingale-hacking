#!/bin/sh
#./bootstrap && \
CPPFLAGS="-I/mingw/include -I/_win32/include -I/_win32/include/ebml" \
LDFLAGS="-L/mingw/lib -L/_win32/lib" \
./configure \
  --enable-release \
  --enable-strip \
  --enable-mozilla \
  --with-mozilla-sdk-path=/usr/_win32/mozilla-sdk \
  --enable-ffmpeg --with-ffmpeg-faac --with-ffmpeg-mp3lame --with-ffmpeg-zlib \
  --enable-mad --disable-a52 \
  --enable-faad --enable-flac \
  --disable-activex \
  --disable-hal \
  --disable-sdl --disable-gtk \
  --disable-cddax --disable-vcdx --disable-goom \
  --disable-twolame \
  --disable-wxwidgets --disable-skins2 \
  --disable-debug --disable-dvdread \
  --disable-dvdnav --disable-screen --disable-visual \
  --disable-bonjour --disable-joystick --disable-httpd \
  --disable-vlm --disable-gnutls --disable-disable-dvbpsi \
  --disable-cdda --disable-libcddb --disable-png --disable-x11 --disable-xvideo --disable-glx --disable-opengl \
  --disable-alsa  --disable-mkv --disable-cmml --disable-freetype --disable-mod --disable-mpc \
  --disable-dts --disable-x264 --disable-h264 --disable-real --disable-realrtsp --disable-libmpeg2 \
  --with-xml2-config-path=/usr/_win32/bin \
  --with-freetype-config-path=/usr/_win32/bin \
  --with-fribidi-config-path=/usr/_win32/bin \
  --with-libintl-prefix=/usr/_win32
