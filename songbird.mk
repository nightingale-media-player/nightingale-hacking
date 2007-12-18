#
# BEGIN SONGBIRD GPL
#
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2007 POTI, Inc.
# http://www.songbirdnest.com
#
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 (the GPL).
#
# Software distributed under the License is distributed
# on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either
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

#
# I hate hardcoding this, but until we have more time that's
#   just how it's gonna be... This MUST match what's in
#   configure.ac or bad things will happen!
#
OBJDIRNAME  = compiled
DISTDIRNAME = dist
OBJDIR_DEPTH = ..

CWD := $(shell pwd)
ifeq "$(CWD)" "/"
CWD := /.
endif

ROOTDIR   := $(shell dirname $(CWD))
TOPSRCDIR := $(CWD)
_AUTOCONF_TOOLS_DIR = $(TOPSRCDIR)/build/autoconf

CONFIGURE    = $(TOPSRCDIR)/configure
ALLMAKEFILES = $(TOPSRCDIR)/allmakefiles.sh
CONFIGUREAC  = $(TOPSRCDIR)/configure.ac
OBJDIR       = $(TOPSRCDIR)/$(OBJDIRNAME)
DISTDIR      = $(OBJDIR)/$(DISTDIRNAME)
CONFIGSTATUS = $(OBJDIR)/config.status

CONFIGURE_ARGS = $(NULL)

####################################
# Load songbirdconfig Options
#

SONGBIRDCONFIG_MAKEFILE_LOADER := $(TOPSRCDIR)/build/autoconf/songbirdconfig2client-mk
SONGBIRDCONFIG_CONFIGURE_LOADER := $(TOPSRCDIR)/build/autoconf/songbirdconfig2configure
SONGBIRDCONFIG_FINDER := $(TOPSRCDIR)/build/autoconf/songbirdconfig-find
run_for_make_options := \
  $(shell cd $(ROOTDIR); \
     $(SONGBIRDCONFIG_MAKEFILE_LOADER) $(TOPSRCDIR) $(TOPSRCDIR)/.songbirdconfig.mk > \
     $(TOPSRCDIR)/.songbirdconfig.out)

include $(TOPSRCDIR)/.songbirdconfig.mk

CONFIGURE_ARGS = $(SONGBIRDCONFIG_CONFIGURE_OPTIONS) \
                 $(NULL)

# MAKECMDGOALS contains the targets passed on the command line:
#    example:  make -f songbird.mk debug
#    - $(MAKECMDGOALS) would contain debug
ifeq (debug,$(MAKECMDGOALS))
DEBUG = 1
endif

# Global, debug/release build options
# building installer is off by default
ifdef SB_ENABLE_INSTALLER
CONFIGURE_ARGS += --enable-installer
endif

ifdef SONGBIRD_OFFICIAL
CONFIGURE_ARGS += --enable-official
endif

ifdef SB_UPDATE_CHANNEL
CONFIGURE_ARGS += --enable-update-channel=$(SB_UPDATE_CHANNEL)
endif

# debug build options
ifdef DEBUG
CONFIGURE_ARGS += --enable-debug
# debug builds turn off jars by default, unless SB_ENABLE_JARS is set
ifdef SB_ENABLE_JARS
CONFIGURE_ARGS += --enable-jars
endif
# turn off tests if you really want
ifndef SB_DISABLE_TESTS
CONFIGURE_ARGS += --enable-tests
endif
endif  # ifdef DEBUG

# release build options
ifndef DEBUG
# release builds have jars by default, unless SB_DISABLE_JARS is set
ifdef SB_DISABLE_JARS
CONFIGURE_ARGS += --disable-jars
endif
# release builds don't have tests by default
ifdef SB_ENABLE_TESTS
CONFIGURE_ARGS += --enable-tests
endif
endif #ifndef DEBUG

# choose core wrappers to enable
ifdef SB_NO_MEDIA_CORE
CONFIGURE_ARGS += --with-media-core=none
endif #SB_NO_MEDIA_CORE

# breakpad support
ifdef SB_ENABLE_BREAKPAD
CONFIGURE_ARGS += --enable-breakpad
endif

# force installation of wmp core, so it's bundled with the application.
ifdef SB_FORCE_MEDIA_CORE_WMP
CONFIGURE_ARGS += --with-media-core=windowsmedia \
                  --with-force-media-core=windowsmedia \
                  $(NULL)
endif #SB_FORCE_MEDIA_CORE_WMP

# force installation of qt core, so it's bundled with the application.
ifdef SB_FORCE_MEDIA_CORE_QT
CONFIGURE_ARGS += --with-media-core=qt \
                  --with-force-media-core=qt \
                  $(NULL)
endif #SB_FORCE_MEDIA_CORE_QT

#
# Set some needed commands
#

ifndef AUTOCONF
AUTOCONF := autoconf
endif

ifndef MKDIR
MKDIR := mkdir
endif

ifndef MAKE
MAKE := gmake
endif

SONGBIRD_MESSAGE = Songbird Build System

RUN_AUTOCONF_CMD = cd $(TOPSRCDIR) && \
                   $(AUTOCONF) && \
                   rm -rf $(TOPSRCDIR)/autom4te.cache/ \
                   $(NULL)

CREATE_OBJ_DIR_CMD = $(MKDIR) -p $(OBJDIR) $(DISTDIR) \
                     $(NULL)

# When calling configure we need to use a relative path so that it will spit
# out relative paths for our makefiles.
RUN_CONFIGURE_CMD = cd $(OBJDIR) && \
                    $(CONFIGURE) $(CONFIGURE_ARGS) \
                    $(NULL)

CLEAN_CMD = $(MAKE) -C $(OBJDIR) clean \
            $(NULL)

CLOBBER_CMD = rm -f $(CONFIGURE) && \
              rm -rf $(OBJDIR) \
              $(NULL)

BUILD_CMD = $(MAKE) -C $(OBJDIR) \
            $(NULL)

CONFIGURE_PREREQS = $(ALLMAKEFILES) \
                    $(CONFIGUREAC) \
                    $(NULL)

all : songbird_output build

debug : all

$(CONFIGSTATUS) : $(CONFIGURE)
	$(CREATE_OBJ_DIR_CMD)
	$(RUN_CONFIGURE_CMD)

$(CONFIGURE) : $(CONFIGURE_PREREQS)
	$(RUN_AUTOCONF_CMD)

songbird_output:
	@echo $(SONGBIRD_MESSAGE)

run_autoconf :
	$(RUN_AUTOCONF_CMD)

create_obj_dir :
	$(CREATE_OBJ_DIR_CMD)

makefiles:
	@touch configure
	$(CREATE_OBJ_DIR_CMD)
	$(RUN_CONFIGURE_CMD)

run_configure : $(CONFIGSTATUS)

clean :
	$(CLEAN_CMD)

clobber:
	$(CLOBBER_CMD)

build : $(CONFIGSTATUS)
	$(BUILD_CMD)

.PHONY : all debug songbird_output run_autoconf create_dist_dir run_configure clean clobber build
