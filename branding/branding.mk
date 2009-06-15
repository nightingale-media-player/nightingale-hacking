# Songbird official branding.

SB_BRAND_SHORT_NAME  = $(SB_APPNAME)
SB_BRAND_FULL_NAME   = Songbird Web Player
SB_BRAND_VENDOR_NAME = POTI, Inc.
SB_BRAND_TRADEMARKS  = \
  Songbird and 'Play the Web' are registered trademarks of POTI, Inc.\\\\r\\\\n\
  Mozilla is a registered trademark of the Mozilla Foundation.\\\\r\\\\n\
  XULRunner is a trademark of the Mozilla Foundation.
SB_INSTALLER_ABOUT_URL=http://www.songbirdnest.com/
SB_INSTALLER_UPDATE_URL=http://getsongbird.com/
SB_CRASHREPORT_SERVER_URL=https://crashreports.songbirdnest.com/submit
SB_APP_BUNDLE_BASENAME=com.songbirdnest

SB_BRANDING_DEFINES += \
  -DSB_VENDOR="$(SB_BRAND_VENDOR_NAME)" \
  -DSB_TRADEMARKS="$(SB_BRAND_TRADEMARKS)" \
  -DSB_INSTALLER_ABOUT_URL="$(SB_INSTALLER_ABOUT_URL)" \
  -DSB_INSTALLER_UPDATE_URL="$(SB_INSTALLER_UPDATE_URL)" \
  -DSB_CRASHREPORT_SERVER_URL="$(SB_CRASHREPORT_SERVER_URL)" \
  -DSB_APP_BUNDLE_BASENAME="$(SB_APP_BUNDLE_BASENAME)" \
  $(NULL)
