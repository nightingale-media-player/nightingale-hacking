#
# BEGIN SONGBIRD GPL
# 
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2007 POTI, Inc.
# http://www.songbirdnest.com
# 
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

##############################################################################
# config.mk
#
# This file is included in rules.mk and contains variables not set by
# autoconf. It's primary use is to set PPDEFINES, a list of definitions that
# are passed to the preprocessor.
###############################################################################

#------------------------------------------------------------------------------
# Only include this file once
ifndef CONFIG_MK_INCLUDED
CONFIG_MK_INCLUDED=1
#------------------------------------------------------------------------------

#
# Create a list of defines for the preprocessor. These are collected in the
# PPDEFINES variable. You need to prefix with -D.
#

#
# We want to pull this info out of the sbBuildInfo.ini file, but if we're just
# starting then that file may not have been generated yet. In that case these
# variables will be empty and the preprocessor will FAIL if one of them is
# referenced.
#
# SB_APPNAME
# SB_BRANCHNAME
# SB_BUILD_ID
# SB_MILESTONE
# SB_MILESTONE_WINDOWS
#

BUILDINFO_FILE := $(DEPTH)/build/sbBuildInfo.ini

ifneq (,$(wildcard $(BUILDINFO_FILE)))

CMD := $(PYTHON) $(MOZSDK_SCRIPTS_DIR)/printconfigsetting.py $(BUILDINFO_FILE) Build

SB_APPNAME := $(shell $(CMD) AppName)
SB_BRANCHNAME := $(shell $(CMD) Branch)
SB_BUILD_ID := $(shell $(CMD) BuildID)
SB_MILESTONE := $(shell $(CMD) Milestone)
SB_MILESTONE_WINDOWS := $(shell $(CMD) MilestoneWindows)
SB_PROFILE_VERSION := $(shell $(CMD) ProfileVersion)

PPDEFINES += -DSB_APPNAME="$(SB_APPNAME)" \
             -DSB_BRANCHNAME="$(SB_BRANCHNAME)" \
             -DSB_BUILD_ID="$(SB_BUILD_ID)" \
             -DSB_MILESTONE="$(SB_MILESTONE)" \
             -DSB_MILESTONE_WINDOWS="$(SB_MILESTONE_WINDOWS)" \
             -DSB_PROFILE_VERSION="$(SB_PROFILE_VERSION)" \
             $(NULL)

endif

#
# Pull this value out of mozilla-config.h
#
# SB_MOZILLA_VERSION
#

AWK_CMD = $(AWK) $(AWK_EXPR) < $(MOZSDK_INCLUDE_DIR)/mozilla-config.h
AWK_EXPR = '/\#define MOZILLA_VERSION_U/ { gsub(/"/, "", $$3); print $$3 }'
SB_MOZILLA_VERSION := $(shell $(AWK_CMD))

PPDEFINES += -DSB_MOZILLA_VERSION="$(SB_MOZILLA_VERSION)"

#
# Need some others from autodefs.mk (hence configure args)
#

PPDEFINES += -DSB_UPDATE_CHANNEL="$(SB_UPDATE_CHANNEL)"

ifdef FORCE_JARS
PPDEFINES += -DFORCE_JARS="$(FORCE_JARS)"
endif

ifdef PREVENT_JARS
PPDEFINES += -DFPREVENT_JARS="$(PREVENT_JARS)"
endif

ifdef MAKE_INSTALLER
PPDEFINES += -DMAKE_INSTALLER="$(MAKE_INSTALLER)"
endif

ifdef SONGBIRD_OFFICIAL
PPDEFINES += -DSONGBIRD_OFFICIAL="$(SONGBIRD_OFFICIAL)"
endif

ifdef SB_ENABLE_TESTS
PPDEFINES += -DSB_ENABLE_TESTS=1
endif

ifdef SB_ENABLE_BREAKPAD
PPDEFINES += -DSB_ENABLE_BREAKPAD=1
endif

# core wrappers to enable
PPDEFINES += $(if $(MEDIA_CORE_VLC), -DMEDIA_CORE_VLC=1) \
             $(if $(MEDIA_CORE_WMP), -DMEDIA_CORE_WMP=1) \
             $(if $(MEDIA_CORE_GST), -DMEDIA_CORE_GST=1) \
             $(if $(MEDIA_CORE_QT),  -DMEDIA_CORE_QT=1 ) \
             $(NULL)

# support for different versions of MSVC
PPDEFINES += $(if $(_MSC_VER), -D_MSC_VER=$(_MSC_VER))

#------------------------------------------------------------------------------
endif #CONFIG_MK_INCLUDED
#------------------------------------------------------------------------------
