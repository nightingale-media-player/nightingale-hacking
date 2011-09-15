#
#=BEGIN SONGBIRD GPL
#
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2010 POTI, Inc.
# http://www.songbirdnest.com
#
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 (the ``GPL'').
#
# Software distributed under the License is distributed
# on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
# express or implied. See the GPL for the specific language
# governing rights and limitations.
#
# You should have received a copy of the GPL along with this
# program. If not, go to http://www.gnu.org/licenses/gpl.html
# or write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
#=END SONGBIRD GPL
#

DEPTH = ../../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/components/integration/src/win32
VPATH = /media/disk/Linux/makepkg/songbird/components/integration/src/win32 \
        /media/disk/Linux/makepkg/songbird/components/integration/src/win32/..

include $(DEPTH)/build/autodefs.mk

DYNAMIC_LIB = sbIntegration

CPP_EXTRA_INCLUDES = $(DEPTH)/components/integration/public \
                     $(DEPTH)/components/dataremote/public \
                     $(DEPTH)/components/devicesobsolete/manager/public \
                     $(DEPTH)/components/devicesobsolete/base/public \
                     $(DEPTH)/components/devicesobsolete/cd/public \
                     $(DEPTH)/components/devicesobsolete/usb_mass_storage/public \
                     $(topsrcdir)/components/include \
                     $(topsrcdir)/components/integration/src/win32 \
                     $(topsrcdir)/components/devicesobsolete/base/src \
                     $(topsrcdir)/components/devicesobsolete/cd/src \
                     $(MOZSDK_INCLUDE_DIR)/dom \
                     $(MOZSDK_INCLUDE_DIR)/gfx \
                     $(MOZSDK_INCLUDE_DIR)/accessibility \
                     $(MOZSDK_INCLUDE_DIR)/intl \
                     $(MOZSDK_INCLUDE_DIR)/xultmpl \
                     $(MOZSDK_INCLUDE_DIR)/content \
                     $(MOZSDK_INCLUDE_DIR)/widget \
                     $(MOZSDK_INCLUDE_DIR)/docshell \
                     $(MOZSDK_INCLUDE_DIR)/layout \
                     $(MOZSDK_INCLUDE_DIR)/necko \
                     $(MOZSDK_INCLUDE_DIR)/webbrwsr \
                     $(MOZSDK_INCLUDE_DIR)/embedcomponents \
                     $(MOZSDK_INCLUDE_DIR)/xpcom \
                     $(MOZSDK_IDL_DIR) \
                     $(NULL)

DYNAMIC_LIB_EXTRA_IMPORTS = plds4  \
                            ole32 \
                            shell32 \
                            Uxtheme \
                            $(NULL)

CPP_SRCS = MultiMonitor.cpp \
           WindowMinMax.cpp \
           WindowMinMaxSubclass.cpp \
           WindowRegion.cpp \
           WindowResizeHook.cpp \
           GlobalHotkeys.cpp \
           sbKnownFolderManager.cpp \
           sbNativeWindowManager.cpp \
           sbScreenSaverSuppressor.cpp \
           sbWindowChromeService.cpp \
           sbWindowMoveService.cpp \
           $(NULL)

# From ..
CPP_SRCS += NativeWindowFromNode.cpp \
            WindowCloak.cpp \
            IntegrationComponent.cpp \
            sbBaseScreenSaverSuppressor.cpp \
            $(NULL)

IS_COMPONENT = 1

include $(topsrcdir)/build/rules.mk
