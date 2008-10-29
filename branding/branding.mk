# Songbird official branding.

SB_BRAND_SHORT_NAME  = $(SB_APPNAME)
SB_BRAND_FULL_NAME   = Songbird Web Player
SB_BRAND_VENDOR_NAME = POTI, Inc.
SB_BRAND_TRADEMARKS  = \
  Songbird is a registered trademark of POTI, Inc.\\n\
  Mozilla is a registered trademark and XULRunner is a trademark of the Mozilla Foundation.

SB_BRANDING_DEFINES += \
  -DSB_VENDOR="$(SB_BRAND_VENDOR_NAME)" \
  -DSB_TRADEMARKS="$(SB_BRAND_TRADEMARKS)" \
  $(NULL)
