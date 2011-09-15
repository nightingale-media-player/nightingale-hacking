#
# BEGIN SONGBIRD GPL
#
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2008 POTI, Inc.
# http://www.songbirdnest.com
#
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 (the GPL).
#
# Software distributed under the License is distributed
# on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either
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

DEPTH = ../../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/components/devicesobsolete/download/src
VPATH = /media/disk/Linux/makepkg/songbird/components/devicesobsolete/download/src

include $(DEPTH)/build/autodefs.mk

DYNAMIC_LIB = sbDownloadDevice

CPP_SRCS = DownloadDevice.cpp \
           DownloadDeviceComponent.cpp \
           $(NULL)

CPP_EXTRA_INCLUDES = $(topsrcdir)/components/devicesobsolete/base/src\
                     $(DEPTH)/components/devicesobsolete/base/public \
                     $(DEPTH)/components/devicesobsolete/download/public \
                     $(DEPTH)/components/dataremote/public \
                     $(DEPTH)/components/job/public \
                     $(DEPTH)/components/library/base/public \
                     $(DEPTH)/components/library/localdatabase/public \
                     $(DEPTH)/components/mediacore/metadata/manager/public \
                     $(DEPTH)/components/property/public \
                     $(topsrcdir)/components/include \
                     $(topsrcdir)/components/moz/strings/src \
                     $(topsrcdir)/components/moz/xpcom/src \
                     $(topsrcdir)/components/property/src \
                     $(MOZSDK_INCLUDE_DIR)/dom \
                     $(MOZSDK_INCLUDE_DIR)/intl \
                     $(MOZSDK_INCLUDE_DIR)/layout \
                     $(MOZSDK_INCLUDE_DIR)/necko \
                     $(MOZSDK_INCLUDE_DIR)/pref \
                     $(MOZSDK_INCLUDE_DIR)/uconv \
                     $(MOZSDK_INCLUDE_DIR)/unicharutil \
                     $(MOZSDK_INCLUDE_DIR)/uriloader \
                     $(MOZSDK_INCLUDE_DIR)/webbrowserpersist \
                     $(MOZSDK_INCLUDE_DIR)/windowwatcher \
                     $(MOZSDK_IDL_DIR) \
                     $(NULL)

DYNAMIC_LIB_STATIC_IMPORTS = \
 components/devicesobsolete/base/src/devicebase \
 components/moz/strings/src/sbMozStringUtils \
 $(NULL)

DYNAMIC_LIB_EXTRA_IMPORTS = plds4 \
                            $(NULL)

ifeq (macosx,$(SB_PLATFORM))
   DYNAMIC_LIB_EXTRA_IMPORTS += mozjs
endif

ifeq (,$(filter-out linux solaris,$(SB_PLATFORM)))
   DYNAMIC_LIB_STATIC_IMPORTS += \
    $(MOZSDK_LIB_DIR)/$(LIB_PREFIX)unicharutil_external_s$(LIB_SUFFIX) \
    $(NULL)
endif

ifeq (windows,$(SB_PLATFORM))
   DYNAMIC_LIB_EXTRA_IMPORTS += unicharutil_external_s \
                                $(NULL)
endif

JS_COMPONENTS = $(srcdir)/sbAutoDownloader.js \
                $(srcdir)/sbDownloadDeviceHelper.js \
                $(srcdir)/sbDownloadDeviceServicePaneModule.js \
                $(NULL)

SONGBIRD_COMPONENTS = $(JS_COMPONENTS)

IS_COMPONENT = 1

include $(topsrcdir)/build/rules.mk
