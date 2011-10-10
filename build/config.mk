#
# BEGIN NIGHTINGALE GPL
#
# This file is part of the Nightingale web player.
#
# Copyright(c) 2005-2008 POTI, Inc.
# http://www.getnightingale.com
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
# END NIGHTINGALE GPL
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

EXIT_ON_ERROR = set -e; # Shell loops continue past errors without this.

####
ifeq (,$(wildcard $(MOZBROWSER_DIR)))
   $(error Missing mozbrowser directory ($(MOZBROWSER_DIR)). Bailing...)
endif

BUILDINFO_FILE := $(DEPTH)/build/sbBuildInfo.mk

ifneq (,$(wildcard $(BUILDINFO_FILE)))
   include $(BUILDINFO_FILE)

   ifeq (,$(NIGHTINGALE_OFFICIAL)$(NIGHTINGALE_NIGHTLY))
      SB_BUILD_NUMBER = 0
   endif

   ifeq (,$(SB_MOZILLA_VERSION))
      $(error Could not derive SB_MOZILLA_VERSION)
   endif

   PPDEFINES += -DSB_APPNAME="$(SB_APPNAME)" \
                -DSB_BRANCHNAME="$(SB_BRANCHNAME)" \
                -DSB_BUILD_ID="$(SB_BUILD_ID)" \
                -DSB_BUILD_NUMBER="$(SB_BUILD_NUMBER)" \
                -DSB_MILESTONE="$(SB_MILESTONE)" \
                -DSB_MILESTONE_WINDOWS="$(SB_MILESTONE_WINDOWS)" \
                -DSB_PROFILE_VERSION="$(SB_PROFILE_VERSION)" \
                -DSB_MOZILLA_VERSION="$(SB_MOZILLA_VERSION)" \
                $(NULL)

endif

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

ifdef NIGHTINGALE_OFFICIAL
   PPDEFINES += -DNIGHTINGALE_OFFICIAL="$(NIGHTINGALE_OFFICIAL)"
endif

ifdef SB_ENABLE_TESTS
   PPDEFINES += -DSB_ENABLE_TESTS=1
endif

ifdef SB_ENABLE_TEST_HARNESS
   PPDEFINES += -DSB_ENABLE_TEST_HARNESS=1
endif

ifdef SB_ENABLE_BREAKPAD
   PPDEFINES += -DSB_ENABLE_BREAKPAD=1
endif

ifdef SB_USER_EULA_FILE
   PPDEFINES += -DSB_USER_EULA_FILE="$(SB_USER_EULA_FILE)"
endif

ifdef SB_USER_ABOUT_FILE
   PPDEFINES += -DSB_USER_ABOUT_FILE="$(SB_USER_ABOUT_FILE)"
endif

ifdef SB_USER_ABOUTCOLON_FILE
   PPDEFINES += -DSB_USER_ABOUTCOLON_FILE="$(SB_USER_ABOUTCOLON_FILE)"
endif

# core wrappers to enable
PPDEFINES += $(if $(MEDIA_CORE_WMP), -DMEDIA_CORE_WMP=1) \
             $(if $(MEDIA_CORE_GST), -DMEDIA_CORE_GST=1) \
             $(if $(MEDIA_CORE_GST_SYSTEM), -DMEDIA_CORE_GST_SYSTEM=1) \
             $(if $(MEDIA_CORE_QT),  -DMEDIA_CORE_QT=1 ) \
             $(NULL)

# support for different versions of MSVC
PPDEFINES += $(if $(_MSC_VER), -D_MSC_VER=$(_MSC_VER))

# define default extension architecture
ifndef EXTENSION_NO_BINARY_COMPONENTS
   ifeq (windows,$(SB_PLATFORM))
      EXTENSION_ARCH = WINNT_x86-msvc
   endif

   ifeq (macosx,$(SB_PLATFORM))
      ifeq (i686,$(SB_ARCH))
         EXTENSION_ARCH = Darwin_x86-gcc3
       else
         EXTENSION_ARCH = Darwin_$(SB_ARCH)-gcc3
      endif
   endif

   ifeq (linux,$(SB_PLATFORM))
      ifeq (i686,$(SB_ARCH))
         EXTENSION_ARCH = Linux_x86-gcc3
      else
         EXTENSION_ARCH = Linux_$(SB_ARCH)-gcc3
      endif
   endif
endif

THISMAKEFILE = $(CURDIR)/Makefile

#------------------------------------------------------------------------------
endif #CONFIG_MK_INCLUDED
#------------------------------------------------------------------------------
