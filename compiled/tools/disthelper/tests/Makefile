# vim: set sw=3 noet syntax=Makefile :
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
srcdir = /media/disk/Linux/makepkg/songbird/tools/disthelper/tests
VPATH = /media/disk/Linux/makepkg/songbird/tools/disthelper/tests \
        /media/disk/Linux/makepkg/songbird/tools/disthelper/tests/../$(SB_PLATFORM) \
        /media/disk/Linux/makepkg/songbird/tools/disthelper/tests/.. \
        $(NULL)

include $(DEPTH)/build/autodefs.mk

CPP_SRCS = main.cpp \
           parser.cpp \
           debugtest.cpp \
           version.cpp \
           utils.cpp \
           environmenttest.cpp \
           exectest.cpp \
           $(NULL)

# things from ../$(SB_PLATFORM)
ifeq ($(SB_PLATFORM), macosx)
   CMM_SRCS += commands.mm \
               macutils.mm \
               $(NULL)
else #not macosx
   CPP_SRCS += commands.cpp
endif

# things from ../
CPP_SRCS += debug.cpp \
            readini.cpp \
            stringconvert.cpp \
            $(NULL)

# we force include mozilla-config.h/xpcom-config.h; otherwise this is not necessary
CPP_INCLUDES = $(MOZSDK_INCLUDE_DIR) \
               $(MOZSDK_INCLUDE_DIR)/xpcom \
               $(srcdir) \
               $(srcdir)/.. \
               $(NULL)

CMM_EXTRA_INCLUDES = $(srcdir) \
                     $(srcdir)/.. \
                     $(NULL)

SIMPLE_PROGRAM_IMPORT_PATHS = $(MOZSDK_LIB_DIR) \
                              $(NULL)

ifeq ($(SB_PLATFORM), windows)
   SIMPLE_PROGRAM_EXTRA_IMPORTS = shell32 \
                                  shlwapi \
                                  version \
                                  $(NULL)
endif #windows

ifeq ($(SB_PLATFORM), macosx)
   SIMPLE_PROGRAM_EXTRA_FLAGS = -Wl,-framework,Foundation \
                                -Wl,-framework,AppKit \
                                $(NULL)
endif

# Since this is debug-only, we don't need tests.exe coming out as tests_d.exe;
# turn that feature off.
DISABLE_IMPLICIT_NAMING = 1

SIMPLE_PROGRAM = test_disthelper$(BIN_SUFFIX)

NO_DIST_INSTALL = 1

SONGBIRD_TEST_COMPONENT = disthelper

SONGBIRD_TESTS = $(srcdir)/test_disthelper.js \
                 $(NULL)

include $(topsrcdir)/build/rules.mk
