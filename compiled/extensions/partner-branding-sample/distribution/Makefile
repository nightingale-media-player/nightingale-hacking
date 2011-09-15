#
#=BEGIN SONGBIRD BSD-LIKE LICENSE
#
# Copyright (c) 2008-2010 Pioneers of the Inevitable, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#   * Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#   * Neither the name of Pioneers of the Inevitable, Songbird, nor the names
#     of its contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#=END SONGBIRD BSD-LIKE LICENSE
#

DEPTH = ../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/extensions/partner-branding-sample/distribution
VPATH = /media/disk/Linux/makepkg/songbird/extensions/partner-branding-sample/distribution

include $(DEPTH)/build/autodefs.mk

IS_EXTENSION = 1

SONGBIRD_PP_RESOURCES = $(srcdir)/distribution.ini.in \
                        $(srcdir)/updater.ini.in \
                        $(NULL)

PPDEFINES += $(SB_BRANDING_DEFINES) \
             $(DISTRIBUTION_DEFINES) \
             $(NULL)

DISTRIBUTION_DIR = $(OUR_EXTENSION_STAGE_DIR)/distribution

CREATEDIRS = $(DISTRIBUTION_DIR) \
             $(NULL)

SONGBIRD_BASE_INSTALLER_NAME ?= $(SB_INSTALLER_FINAL_DIR)/$(SB_INSTALLER_NAME).exe
SONGBIRD_BASE_INSTALLER_BASENAME = $(notdir $(SONGBIRD_BASE_INSTALLER_NAME))

UNPACKED_BASE_INSTALLER_DIR = $(CURDIR)/unpacked-base-installer

GARBAGE += $(UNPACKED_BASE_INSTALLER_DIR)

include $(topsrcdir)/build/rules.mk

ifeq (,$(wildcard $(SONGBIRD_BASE_INSTALLER_NAME)))
   $(error Couldn't find base songbird installer $(SONGBIRD_BASE_INSTALLER_NAME); either do a full build with --enable-installer or set SONGBIRD_BASE_INSTALLER_NAME)
endif

$(UNPACKED_BASE_INSTALLER_DIR): $(SONGBIRD_BASE_INSTALLER_NAME)	
	$(MKDIR) $(UNPACKED_BASE_INSTALLER_DIR)
	cmd /c "$(call normalizepath,$(SONGBIRD_BASE_INSTALLER_NAME)) /S /UNPACK /D=$(subst /,\,$(call normalizepath,$(UNPACKED_BASE_INSTALLER_DIR)))"

libs:: $(UNPACKED_BASE_INSTALLER_DIR)
	$(INSTALL_FILE) $(CURDIR)/distribution.ini $(DISTRIBUTION_DIR)
	$(INSTALL_FILE) $(CURDIR)/updater.ini $(DISTRIBUTION_DIR)
	$(SED) -e 's/^Profile=.*/Profile=$(DISTRIBUTION_PROFILE_NAME)/' < $(UNPACKED_BASE_INSTALLER_DIR)/application.ini > $(DISTRIBUTION_DIR)/application.ini
	$(INSTALL_FILE) $(srcdir)/alert.js $(DISTRIBUTION_DIR)
