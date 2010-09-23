# Partner sample branding.

SB_BRAND_SHORT_NAME  = $(SB_APPNAME)
SB_BRAND_FULL_NAME   = Partner Media Player
SB_BRAND_VENDOR_NAME = PartnerCo, Inc.
SB_BRAND_TRADEMARKS  = \
  Songbird and 'Play the Web' are registered trademarks of POTI, Inc.\\\\r\\\\n\
  Mozilla is a registered trademark of the Mozilla Foundation.\\\\r\\\\n\
  XULRunner is a trademark of the Mozilla Foundation. Any other trademarks:\
  put them here.

SB_INSTALLER_ABOUT_URL=http://www.example.com/
SB_INSTALLER_UPDATE_URL=http://example.com/
SB_CRASHREPORT_SERVER_URL=https://crashreports.songbirdnest.com/submit
SB_APP_BUNDLE_BASENAME=com.example

SB_BRANDING_DEFINES += \
  -DSB_VENDOR="$(SB_BRAND_VENDOR_NAME)" \
  -DSB_TRADEMARKS="$(SB_BRAND_TRADEMARKS)" \
  -DSB_INSTALLER_ABOUT_URL="$(SB_INSTALLER_ABOUT_URL)" \
  -DSB_INSTALLER_UPDATE_URL="$(SB_INSTALLER_UPDATE_URL)" \
  -DSB_CRASHREPORT_SERVER_URL="$(SB_CRASHREPORT_SERVER_URL)" \
  -DSB_APP_BUNDLE_BASENAME="$(SB_APP_BUNDLE_BASENAME)" \
  $(NULL)
