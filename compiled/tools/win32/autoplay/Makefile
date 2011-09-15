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

DEPTH = ../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/tools/win32/autoplay
VPATH = /media/disk/Linux/makepkg/songbird/tools/win32/autoplay

include $(DEPTH)/build/autodefs.mk

# Ensure the program name is always the same (e.g., doesn't get a different name
# for debug builds), so that the installer can execute it.
DISABLE_IMPLICIT_NAMING = 1
SIMPLE_PROGRAM = sbAutoPlayUtil$(BIN_SUFFIX)
FINAL_TARGET = $(SONGBIRD_DISTDIR)

CPP_SRCS = sbAutoPlayUtil.cpp \
           $(NULL)

CPP_EXTRA_INCLUDES += \
 $(DEPS_DIR)/private/windows_ddk/$(SB_CONFIGURATION)/include \
 $(topsrcdir)/components/include \
 $(topsrcdir)/tools/lib/windows/static \
 $(NULL)

CPP_EXTRA_DEFS += -DSB_LOG_TO_WINDOWS_EVENT_LOG \
                  $(NULL)

SIMPLE_PROGRAM_EXTRA_IMPORTS = shell32 \
                               $(NULL)

SIMPLE_PROGRAM_STATIC_IMPORTS += \
 tools/lib/windows/static/sbWindowsUtils \
 $(NULL)

include $(topsrcdir)/build/rules.mk

