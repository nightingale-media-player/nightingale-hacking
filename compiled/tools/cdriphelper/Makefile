#
# BEGIN SONGBIRD GPL
#
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2008 POTI, Inc.
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

DEPTH = ../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/tools/cdriphelper
VPATH = /media/disk/Linux/makepkg/songbird/tools/cdriphelper

include $(DEPTH)/build/autodefs.mk

CPP_SRCS = winmain.cpp \
           reghandlers.cpp \
           stringconvert.cpp \
           debug.cpp \
           regparse.cpp \
           toolslib.cpp \
           $(NULL)

#
# Uncomment this definition to use the testing keys as opposed to
# the "live ammunition" keys that refer to in-play drivers...
#
## CPP_EXTRA_DEFS += -DRH_REGKEY_ENGAGE_SAFETY=1
#

# we force include mozilla-config.h/xpcom-config.h; otherwise this is not necessary
CPP_INCLUDES = $(MOZSDK_INCLUDE_DIR) \
               $(MOZSDK_INCLUDE_DIR)/xpcom \
               $(srcdir)
               $(NULL)

SIMPLE_PROGRAM_IMPORT_PATHS = $(NULL)

SIMPLE_PROGRAM_EXTRA_IMPORTS = shell32 \
                               shlwapi \
                               version \
                               $(NULL)

# This executable gets called by the updater and installer, so having a _d in 
# the debug version will make it impossible to use a debug reghelper 
# end-to-end; so, disable naming.

DISABLE_IMPLICIT_NAMING = 1

SIMPLE_PROGRAM = cdriphelper$(BIN_SUFFIX)

FINAL_TARGET = $(SONGBIRD_DISTDIR)

ifeq (windows_1,$(SB_PLATFORM)_$(HAS_EXTRAS))
   INSTALL_GEARWORKS_DRIVERS = 1
endif

GEARWORKS_SYSTEM_LIBS = \
 $(DEPS_DIR)/private/gearworks-sdk/Drivers/x86/GearAspi.dll \
 $(DEPS_DIR)/private/gearworks-sdk/Drivers/x86/GearAspiWDM.sys \
 $(NULL)

CREATEDIRS = $(SONGBIRD_DRIVERSDIR) \
             $(NULL)

include $(topsrcdir)/build/rules.mk

libs::
ifeq (1,$(INSTALL_GEARWORKS_DRIVERS))
	$(INSTALL_PROG) $(GEARWORKS_SYSTEM_LIBS) $(SONGBIRD_DRIVERSDIR)
endif
