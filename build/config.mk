#
# BEGIN SONGBIRD GPL
# 
# This file is part of the Songbird web player.
#
# Copyright 2006 Pioneers of the Inevitable LLC
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
# autoconf.
###############################################################################

#------------------------------------------------------------------------------
# Only include this file once
ifndef CONFIG_MK_INCLUDED
CONFIG_MK_INCLUDED=1
#------------------------------------------------------------------------------

#
# We want to pull this info out of the sbBuildIDs.h file, but if we're just
# starting then that file may not have been generated yet. In that case these
# variables will be empty.
#
# SB_BUILD_ID
# SB_MILESTONE
# SB_MOZILLA_TAG
#

BUILD_ID_DEFS = $(DEPTH)/build/sbBuildIDs.h

TEST_CMD := if test -f $(BUILD_ID_DEFS); then \
              echo 1; \
            fi;

BUILD_ID_DEFS_EXISTS := $(shell $(TEST_CMD))

ifneq (,$(BUILD_ID_DEFS_EXISTS))

AWK_CMD = $(AWK) $(AWK_EXPR) < $(BUILD_ID_DEFS)

AWK_EXPR = '/\#define SB_BUILD_ID/ { gsub(/"/, "", $$3); print $$3 }'
SB_BUILD_ID := $(shell $(AWK_CMD))

AWK_EXPR = '/\#define SB_MILESTONE/ { gsub(/"/, "", $$3); print $$3 }'
SB_MILESTONE := $(shell $(AWK_CMD))

AWK_EXPR = '/\#define SB_MOZILLA_TAG/ { gsub(/"/, "", $$3); print $$3 }'
SB_MOZILLA_TAG := $(shell $(AWK_CMD))

endif

#
# Pull this value out of mozilla-config.h
#
# SB_MOZILLA_VERSION
#

AWK_CMD = $(AWK) $(AWK_EXPR) < $(MOZSDK_INCLUDE_DIR)/mozilla-config.h
AWK_EXPR = '/\#define MOZILLA_VERSION_U/ { gsub(/"/, "", $$3); print $$3 }'
SB_MOZILLA_VERSION := $(shell $(AWK_CMD))

#------------------------------------------------------------------------------
endif #CONFIG_MK_INCLUDED
#------------------------------------------------------------------------------
