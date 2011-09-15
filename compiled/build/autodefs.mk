#
# BEGIN SONGBIRD GPL
#
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2008 POTI, Inc.
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
# END SONGBIRD GPL
#

##############################################################################
# autodefs.mk
#
# This file gets included in every makefile for easy access to important
# definitions and tools
###############################################################################

#------------------------------------------------------------------------------
# Only include this file once
ifndef AUTODEFS_MK_INCLUDED
AUTODEFS_MK_INCLUDED=1
#------------------------------------------------------------------------------

# These technically should be in rules.mk, but autodefs.mk gets included at
# the top of Makefiles throughout the tree, whereas rules.mk gets included at
# the bottom. :`( Since we need these defines in the target definitions in
# between these two includes, we define them here.

COMMA ?= ,
EMPTY ?=
SPACE ?= $(EMPTY) $(EMPTY)

SB_PLATFORM = linux
SB_ARCH = x86_64
SB_CONFIGURATION = release

MACOSX_APPBUNDLE = 

ACDEFINES = -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DNDEBUG=1 -DXP_UNIX=1 -D_REENTRANT=1

BIN_SUFFIX = 
OBJ_SUFFIX = .o
LIB_PREFIX = lib
LIB_SUFFIX = .a
DLL_SUFFIX = .so

USING_RANLIB = 1

#
# Variables set by passing configure arguments
#

DEBUG = 
FORCE_JARS = 
PREVENT_JARS = 
MAKE_INSTALLER = 
SONGBIRD_OFFICIAL = 
SONGBIRD_NIGHTLY = 
SB_UPDATE_CHANNEL = default
SB_ENABLE_TESTS = 
SB_ENABLE_TEST_HARNESS = 
SB_ENABLE_BREAKPAD = 
SB_ENABLE_STATIC = 
SB_SEARCHPLUGINS = default
SB_BIRD_EXTENSIONS = default
SB_EXTENSIONS = default
SB_USE_JEMALLOC = 1
SB_SQLITE_DEBUG = 
SB_WITH_MSVC_EXPRESS = 
SB_WITH_DEADLY_WARNINGS = all

SB_ENABLE_LIBRARY_PERF = 

# cores to enable
MEDIA_CORE_WMP = 
MEDIA_CORE_GST = 1
MEDIA_CORE_GST_SYSTEM = 
MEDIA_CORE_QT = 

# cores to force install and enable
FORCE_MEDIA_CORE_WMP = 
FORCE_MEDIA_CORE_QT = 

#
# Autodetected variables set by configure
#

HAS_EXTRAS = 
HAVE_FLUENDO_MP3 = 

#
# Commonly used directories
#

DEPS_DIR = $(topsrcdir)/dependencies/linux-x86_64
XULRUNNER_DIR = /usr/lib/xulrunner-6.0.1
SCRIPTS_DIR = $(topsrcdir)/tools/scripts

#
# Where o' where have my lil' SDKs gone...
#

SB_MACOSX_SDK_10_4 = 
SB_MACOSX_SDK_10_5 = 
SB_MACOSX_SDK = 

MOZSDK_DIR = /usr/lib/xulrunner-devel-6.0.1
MOZSDK_INCLUDE_DIR = $(MOZSDK_DIR)/include
MOZSDK_LIB_DIR = $(MOZSDK_DIR)/lib
MOZSDK_BIN_DIR = $(MOZSDK_DIR)/bin
MOZSDK_IDL_DIR = $(MOZSDK_DIR)/idl
MOZSDK_SCRIPTS_DIR = /media/disk/Linux/makepkg/songbird/dependencies/linux-i686/mozilla-1.9.2/release/scripts

# Boo for hardcoding this; this appears in songbird.mk as well; so make
# changes there, too.
MOZBROWSER_DIR = $(topsrcdir)/dependencies/vendor/mozbrowser
MOZJSHTTPD_DIR = $(topsrcdir)/dependencies/vendor/mozjshttpd

PLUGINS_DIR = $(DEPS_DIR)/plugins

OBJDIRNAME = compiled
DISTDIRNAME = dist

DISTDIR_MANIFEST = manifest.$(SB_PLATFORM)-$(SB_ARCH)

SONGBIRD_OBJDIR = $(topsrcdir)/compiled
SONGBIRD_DISTDIR = $(topsrcdir)/compiled/dist

SONGBIRD_BRANDING_DIR = branding
SONGBIRD_CHROMEDIR = $(SONGBIRD_DISTDIR)/chrome
SONGBIRD_COMPONENTSDIR = $(SONGBIRD_DISTDIR)/components
SONGBIRD_CONTENTSDIR = $(topsrcdir)/compiled/dist
SONGBIRD_DEFAULTSDIR = $(SONGBIRD_DISTDIR)/defaults
SONGBIRD_DRIVERSDIR = $(SONGBIRD_DISTDIR)/drivers
SONGBIRD_EXTENSIONSDIR = $(SONGBIRD_DISTDIR)/extensions
SONGBIRD_DOCUMENTATIONDIR = $(topsrcdir)/compiled/documentation
SONGBIRD_GSTPLUGINSDIR = $(SONGBIRD_DISTDIR)/gst-plugins
SONGBIRD_JSMODULESDIR = $(SONGBIRD_DISTDIR)/jsmodules
SONGBIRD_LIBDIR = $(SONGBIRD_DISTDIR)/lib
SONGBIRD_MACOS = $(topsrcdir)/compiled/dist/MacOS
SONGBIRD_PLUGINSDIR = $(SONGBIRD_DISTDIR)/plugins
SONGBIRD_PREFERENCESDIR = $(SONGBIRD_DISTDIR)/defaults/preferences
SONGBIRD_PROFILEDIR = $(SONGBIRD_DISTDIR)/defaults/profile
SONGBIRD_SCRIPTSDIR = $(SONGBIRD_DISTDIR)/scripts
SONGBIRD_SEARCHPLUGINSDIR = $(SONGBIRD_DISTDIR)/searchplugins
SONGBIRD_TESTSDIR = $(SONGBIRD_DISTDIR)/testharness
SONGBIRD_XULRUNNERDIR = /usr/lib/xulrunner-6.0.1

# Directories assumed to exist when building any part of the application
APP_DIST_DIRS = $(SONGBIRD_CHROMEDIR) \
                $(SONGBIRD_COMPONENTSDIR) \
                $(SONGBIRD_DEFAULTSDIR) \
                $(SONGBIRD_EXTENSIONSDIR) \
                $(SONGBIRD_GSTPLUGINSDIR) \
                $(SONGBIRD_JSMODULESDIR) \
                $(SONGBIRD_LIBDIR) \
                $(SONGBIRD_PLUGINSDIR) \
                $(SONGBIRD_PREFERENCESDIR) \
                $(SONGBIRD_PROFILEDIR) \
                $(SONGBIRD_SCRIPTSDIR) \
                $(SONGBIRD_SEARCHPLUGINSDIR) \
                $(SONGBIRD_XULRUNNERDIR) \
                $(NULL)

ifeq (macosx,$(SB_PLATFORM))
  APP_DIST_DIRS += $(SONGBIRD_MACOS)
endif

ifdef SB_ENABLE_TESTS
   APP_DIST_DIRS += $(SONGBIRD_TESTSDIR)
endif

#
# Files
#
SB_LICENSE_FILE = $(topsrcdir)/compiled/installer/LICENSE.html

#
# Installer-related stuff
#
SB_INSTALLER_FINAL_DIR = $(SONGBIRD_OBJDIR)/_built_installer
SB_INSTALLER_NAME = $(SB_APPNAME)_$(SB_MILESTONE)-$(SB_BUILD_NUMBER)_$(SB_PLATFORM)-$(SB_ARCH)
SB_INSTALLER_SUFFIXES = tar.gz

#
# Tools
#

AR = ar
AWK = gawk
CC = gcc
CHMOD = chmod
CP = cp
CXX = g++
DIFF = diff
DOXYGEN = 
FIND = find
GREP = grep
GUNZIP = gunzip
GZIP = gzip
INSTALL = install -c
INSTALL_FILE = $(INSTALL) -m 0644
INSTALL_PROG = $(INSTALL) -m 0755
LD = g++
LN = ln
MACPKGMAKER = false
MD5SUM = md5sum
MIDL = 
MKDIR = mkdir -p
# Use MKDIR_APP to create a directory that is intended to be shipped; we use
# intstall -d here because mkdir -p -m N only applies the -m to the _last_
# directory part, not any of the other directories it creates along the way;
# lame, Lame, LAME; hats off to Mook for suggesting this.
MKDIR_APP = $(INSTALL) -d -m 755
MSMANIFEST_TOOL = 
MV = mv
PERL = perl
PYTHON = python2
RANLIB = ranlib
RC = 
REBASE = 
RM = rm -f
SED = sed
SHA1SUM = sha1sum
SORT = sort
STRIP = strip
SYSINSTALL = echo
TAR = tar
TOUCH = touch
UNZIP = unzip
ZIP = zip

XPIDL = $(MOZSDK_DIR)/bin/xpidl
XPTLINK = $(MOZSDK_DIR)/sdk/bin/xpt.py link
MAR = $(MOZSDK_DIR)/bin/mar
MBSDIFF = $(MOZSDK_DIR)/bin/mbsdiff

#
# Tool flags
#

CFLAGS =  -c -fPIC -fshort-wchar -fexceptions -fnon-call-exceptions -funwind-tables -fasynchronous-unwind-tables -fno-rtti -fno-strict-aliasing -Wall -Wno-conversion -Wno-attributes -Wpointer-arith -Wcast-align -Wno-long-long -pipe -pthread -gstabs+ -Os -include "mozilla-config.h"
CXXFLAGS =  -c -fPIC -fshort-wchar -fexceptions -fnon-call-exceptions -funwind-tables -fasynchronous-unwind-tables -fno-rtti -fno-strict-aliasing -Wall -Wno-conversion -Wno-attributes -Wpointer-arith -Wcast-align -Wno-long-long -pipe -pthread -gstabs+ -Os -include "mozilla-config.h" -Woverloaded-virtual -Wsynth -Wno-ctor-dtor-privacy -Wno-non-virtual-dtor
CMMFLAGS = 
CFLAGS_INCLUDE_PREFIX = -I
CFLAGS_INCLUDE_SUFFIX = 
CFLAGS_STATIC_LIBC = 
CFLAGS_PREPROCESS = -E
CFLAGS_ASSEMBLER = -S
CFLAGS_WARNING_IS_ERROR = -Werror
_MSC_VER = 
DEFAULT_MIDL_FLAGS = 
LDFLAGS =  -Wl,-z,defs -Wl,-rpath-link,$(topsrcdir)/compiled/dist/xulrunner
LDFLAGS_DLL = -shared
LDFLAGS_BIN = 
LDFLAGS_LIB = -static
LDFLAGS_OUT_PREFIX = -o 
LDFLAGS_OUT_SUFFIX = 
LDFLAGS_PATH_PREFIX = -L
LDFLAGS_PATH_SUFFIX = 
LDFLAGS_IMPORT_PREFIX = -l
LDFLAGS_IMPORT_SUFFIX = 
DEFAULT_LIBS = 
ARFLAGS = cr
ARFLAGS_OUT_PREFIX = 
ARFLAGS_OUT_SUFFIX = 
ARFLAGS_PATH_PREFIX = 
ARFLAGS_PATH_SUFFIX = 
ARFLAGS_LIB = 
LNFLAGS = -f --symbolic
UNZIPFLAGS = -u -o -q -X
UNZIPFLAGS_EXTRACT = -d
STRIP_FLAGS = -v

COMPILER_GARBAGE = 

PTHREAD_LIBS = -lpthread 

GSTREAMER_CFLAGS = 
GSTREAMER_LIBS = 
GTK_CFLAGS = -pthread -I/usr/include/gtk-2.0 -I/usr/lib/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/pango-1.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/libpng14  
GTK_LIBS =   -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo -lpango-1.0 -lfreetype -lfontconfig -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0  
GLIB_CFLAGS = -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include  
GLIB_LIBS =   -lglib-2.0  

DBUS_CFLAGS = -pthread -I/usr/include/dbus-1.0 -I/usr/lib/dbus-1.0/include -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include  
DBUS_LIBS =   -ldbus-glib-1 -ldbus-1 -lpthread -lgobject-2.0 -lgthread-2.0 -lrt -lglib-2.0  

# hal is deprecated
#HAL_CFLAGS = @HAL_CFLAGS@
#HAL_LIBS = @HAL_LIBS@

NSPR_CFLAGS = -I/usr/include/nspr  
NSPR_LIBS =   -lplds4 -lplc4 -lnspr4 -lpthread -ldl  
TAGLIB_CFLAGS = -I/usr/include/taglib  
TAGLIB_LIBS =   -ltag  

# Version numbers for various packages
GST_VERSION = 0.10
GLIB_VERSION = 2.0

#------------------------------------------------------------------------------
endif #AUTODEFS_MK_INCLUDED
#------------------------------------------------------------------------------
