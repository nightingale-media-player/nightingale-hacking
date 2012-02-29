# Songbird official branding.

SB_BRAND_SHORT_NAME  = $(SB_APPNAME)
SB_BRAND_FULL_NAME   = Nightingale Media Player
SB_BRAND_VENDOR_NAME = The Nightingale Community
SB_BRAND_TRADEMARKS  = \
  Mozilla is a registered trademark of the Mozilla Foundation.\\\\r\\\\n\
  XULRunner is a trademark of the Mozilla Foundation.
SB_INSTALLER_ABOUT_URL=http://getnightingale.com/
SB_INSTALLER_UPDATE_URL=http://getnightingale.com/
SB_CRASHREPORT_SERVER_URL=
SB_APP_BUNDLE_BASENAME=com.getnightingale

SB_BRANDING_DEFINES += \
  -DSB_VENDOR="$(SB_BRAND_VENDOR_NAME)" \
  -DSB_TRADEMARKS="$(SB_BRAND_TRADEMARKS)" \
  -DSB_INSTALLER_ABOUT_URL="$(SB_INSTALLER_ABOUT_URL)" \
  -DSB_INSTALLER_UPDATE_URL="$(SB_INSTALLER_UPDATE_URL)" \
  -DSB_CRASHREPORT_SERVER_URL="$(SB_CRASHREPORT_SERVER_URL)" \
  -DSB_APP_BUNDLE_BASENAME="$(SB_APP_BUNDLE_BASENAME)" \
  $(NULL)
