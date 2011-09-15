#
# BEGIN SONGBIRD GPL
#
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2008 POTI, Inc.
# http://www.songbirdnest.com
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 (the "GPL").
#
# Software distributed under the License is distributed
# on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
# express or implied. See the GPL for the specific language
# governing rights and limitations.
#
# You should have received a copy of the GPL along with this
# program. If not, go to http://www.gnu.org/licenses/gpl.html
# or write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# END SONGBIRD GPL
#

DEPTH = ../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/installer/windows/partner
VPATH = /media/disk/Linux/makepkg/songbird/installer/windows/partner

include $(DEPTH)/build/autodefs.mk

SONGBIRD_BRANDING_DIR = extensions/partner-branding-sample

SONGBIRD_PP_RESOURCES = $(srcdir)/defines.nsi.in \
                        $(NULL)

# Base Songbird installer settings; can be overridden
SONGBIRD_BASE_INSTALLER_NAME ?= $(SB_INSTALLER_FINAL_DIR)/$(SB_INSTALLER_NAME).exe
SONGBIRD_BASE_INSTALLER_BASENAME = $(notdir $(SONGBIRD_BASE_INSTALLER_NAME))
UNPACKED_BASE_INSTALLER_DIR = $(CURDIR)/unpacked-base-installer
GARBAGE += $(UNPACKED_BASE_INSTALLER_DIR)

# Taken from installer/windows/defines.nsi.in
SONGBIRD_BASE_UNINSTALLER_BASENAME = Songbird-Uninstall.exe

# Partner extension settings; can be overridden
BRANDING_EXTENSION ?= $(SONGBIRD_OBJDIR)/xpi-stage/partner-branding-sample/partner-branding-sample-1.0.0.xpi
BRANDING_EXTENSION_BASENAME = $(notdir $(DISTRIBUTION_EXTENSION)
UNPACKED_BRANDING_EXTENSION_DIR = $(CURDIR)/unpacked-branding-extension
GARBAGE += $(UNPACKED_BRANDING_EXTENSION_DIR)

PARTNER_INSTALLER_DIST_DIR = $(CURDIR)/dist
GARBAGE += $(PARTNER_INSTALLER_DIST_DIR)

# Win32 only supports a single installer suffix, so the pluralized suffixes
# is misleading, but also fine 
#FULL_INSTALLER_NAME = $(SB_INSTALLER_NAME).$(SB_INSTALLER_SUFFIXES)

PPDEFINES += $(SB_BRANDING_DEFINES) \
             $(DISTRIBUTION_DEFINES) \
             -DSB_BASE_INSTALLER_NAME="$(SONGBIRD_BASE_INSTALLER_NAME)" \
             -DSB_BASE_INSTALLER_BASENAME="$(SONGBIRD_BASE_INSTALLER_BASENAME)" \
             -DSB_BASE_UNINSTALLER_BASENAME="$(SONGBIRD_BASE_UNINSTALLER_BASENAME)" \
             -DPARTNER_INSTALLER_NAME="$(PARTNER_INSTALLER_NAME)" \
             -DCUR_OBJDIR="$(CURDIR)" \
             $(NULL)

CREATEDIRS += $(SB_INSTALLER_FINAL_DIR)

include $(topsrcdir)/build/rules.mk

# We $(warn) instead of error, because there are a number of targets that
# we can still reasonably run without these files; we explicitely set them
# to null so their usage will error out.
ifeq (,$(wildcard $(SONGBIRD_BASE_INSTALLER_NAME)))
   $(warn Couldn't find base songbird installer $(SONGBIRD_BASE_INSTALLER_NAME); either do a full build with --enable-installer or set SONGBIRD_BASE_INSTALLER_NAME)
   SONGBIRD_BASE_INSTALLER_NAME = NULL
endif
ifeq (,$(wildcard $(BRANDING_EXTENSION)))
   $(warn Couldn't find partner branding extension $(BRANDING_EXTENSION); either do a full build with --enable-extensions=partner-branding-sample or set BRANDING_EXTENSION)
   BRANDING_EXTENSION = NULL
endif

$(UNPACKED_BASE_INSTALLER_DIR): $(SONGBIRD_BASE_INSTALLER_NAME)	
	$(RM) -r $(UNPACKED_BASE_INSTALLER_DIR)
	$(MKDIR) $(UNPACKED_BASE_INSTALLER_DIR)
	cmd /c "$(call normalizepath,$(SONGBIRD_BASE_INSTALLER_NAME)) /S /UNPACK /D=$(subst /,\,$(call normalizepath,$(UNPACKED_BASE_INSTALLER_DIR)))"

$(UNPACKED_BRANDING_EXTENSION_DIR): $(BRANDING_EXTENSION)
	$(RM) -r $(UNPACKED_BRANDING_EXTENSION_DIR)
	$(MKDIR) $(UNPACKED_BRANDING_EXTENSION_DIR)
	cd $(UNPACKED_BRANDING_EXTENSION_DIR) && $(UNZIP) $(BRANDING_EXTENSION)

$(PARTNER_INSTALLER_DIST_DIR): $(UNPACKED_BASE_INSTALLER_DIR) $(UNPACKED_BRANDING_EXTENSION_DIR)
	$(MKDIR) $(PARTNER_INSTALLER_DIST_DIR)
	$(MKDIR) $(PARTNER_INSTALLER_DIST_DIR)/partnerdist
	$(INSTALL_FILE) $(UNPACKED_BRANDING_EXTENSION_DIR)/distribution/application.ini $(PARTNER_INSTALLER_DIST_DIR)/partnerdist
	$(INSTALL_FILE) $(UNPACKED_BRANDING_EXTENSION_DIR)/distribution/distribution.ini $(PARTNER_INSTALLER_DIST_DIR)/partnerdist
	$(INSTALL_FILE) $(UNPACKED_BRANDING_EXTENSION_DIR)/distribution/updater.ini $(PARTNER_INSTALLER_DIST_DIR)/partnerdist
	$(INSTALL_FILE) $(srcdir)/partner.ico $(PARTNER_INSTALLER_DIST_DIR)/
	$(INSTALL_FILE) $(srcdir)/partner.ico $(PARTNER_INSTALLER_DIST_DIR)/partnerdist
	$(INSTALL_FILE) $(BRANDING_EXTENSION) $(PARTNER_INSTALLER_DIST_DIR)/partnerdist
	$(INSTALL_PROG) $(SONGBIRD_BASE_INSTALLER_NAME) $(PARTNER_INSTALLER_DIST_DIR)

GARBAGE += $(SB_INSTALLER_FINAL_DIR)/$(DISTRIBUTION_INSTALLER_NAME)

libs:: $(PARTNER_INSTALLER_DIST_DIR)
	cd $(PARTNER_INSTALLER_DIST_DIR) && $(DEPS_DIR)/nsis-2.45/makensis -NOCD -V3 $(srcdir)/PartnerPlayer.nsi
	$(MV) $(CURDIR)/dist/$(DISTRIBUTION_INSTALLER_NAME) $(SB_INSTALLER_FINAL_DIR)/$(DISTRIBUTION_INSTALLER_NAME)
