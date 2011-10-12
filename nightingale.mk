#
#=BEGIN NIGHTINGALE GPL
#
# This file is part of the Nightingale web player.
#
# Copyright(c) 2005-2010 POTI, Inc.
# http://www.getnightingale.com
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
#=END NIGHTINGALE GPL
#

#
# I hate hardcoding this, but until we have more time that's
#   just how it's gonna be... This MUST match what's in
#   configure.ac or bad things will happen!
#
OBJDIRBASENAME  = compiled
DISTDIRNAME = dist
OBJDIR_DEPTH = ..

CWD := $(shell pwd)
ifeq "$(CWD)" "/"
  CWD := /.
endif

ROOTDIR   := $(shell dirname $(CWD))
TOPSRCDIR := $(CWD)
_AUTOCONF_TOOLS_DIR = $(TOPSRCDIR)/build/autoconf

CONFIGURE = $(TOPSRCDIR)/configure
ALLMAKEFILES = $(TOPSRCDIR)/allmakefiles.sh
CONFIGUREAC = $(TOPSRCDIR)/configure.ac

CLOBBER_TRASH = $(CONFIGURE) \
                $(TOPSRCDIR)/.nightingaleconfig.mk \
                $(TOPSRCDIR)/.nightingaleconfig.out \
                $(NULL)

CONFIGURE_ARGS = $(NULL)

ARCH = $(call uname -m)
SB_ARCH = -$(call uname -m)

####################################
# Load nightingaleconfig Options
#

NIGHTINGALECONFIG_MAKEFILE_LOADER := $(TOPSRCDIR)/build/autoconf/nightingaleconfig2client-mk
NIGHTINGALECONFIG_CONFIGURE_LOADER := $(TOPSRCDIR)/build/autoconf/nightingaleconfig2configure
NIGHTINGALECONFIG_FINDER := $(TOPSRCDIR)/build/autoconf/nightingaleconfig-find

run_for_make_options := \
  $(shell cd $(ROOTDIR); \
     $(NIGHTINGALECONFIG_MAKEFILE_LOADER) $(TOPSRCDIR) $(TOPSRCDIR)/.nightingaleconfig.mk > \
     $(TOPSRCDIR)/.nightingaleconfig.out)

include $(TOPSRCDIR)/.nightingaleconfig.mk

CONFIGURE_ARGS = $(NIGHTINGALECONFIG_CONFIGURE_OPTIONS) \
                 $(NULL)

ifneq (,$(findstring --enable-debug,$(CONFIGURE_ARGS)))
	DEBUG = 1
endif

# Global, debug/release build options
# building installer is off by default
#
# Setting these options via the environment is deprecated; use nightingale.config
# instead; see bug 6515
ifdef SB_ENABLE_INSTALLER
   CONFIGURE_ARGS += --enable-installer
   ENV_SETTING_WARN += --enable-installer
endif

ifdef NIGHTINGALE_OFFICIAL
   CONFIGURE_ARGS += --enable-official
   ENV_SETTING_WARN += --enable-official
endif

ifdef NIGHTINGALE_NIGHTLY
   CONFIGURE_ARGS += --enable-nightly
   ENV_SETTING_WARN += --enable-nightly
endif

ifdef SB_UPDATE_CHANNEL
   CONFIGURE_ARGS += --enable-update-channel=$(SB_UPDATE_CHANNEL)
   ENV_SETTING_WARN += --enable-update-channel=$(SB_UPDATE_CHANNEL)
endif

# debug build options
ifdef DEBUG
   # debug builds turn off jars by default, unless SB_ENABLE_JARS is set
   ifdef SB_ENABLE_JARS
      CONFIGURE_ARGS += --enable-jars
      ENV_SETTING_WARN += --enable-jars
   endif
   # turn off tests if you really want
   ifndef SB_DISABLE_TESTS
      CONFIGURE_ARGS += --enable-tests
   endif

   OBJDIRNAME = $(OBJDIRBASENAME)-debug$(SB_ARCH)
endif

# release build options
ifndef DEBUG
   # release builds have jars by default, unless SB_DISABLE_JARS is set
   ifdef SB_DISABLE_JARS
      CONFIGURE_ARGS += --disable-jars
      ENV_SETTING_WARN += --disable-jars
   endif
   # release builds don't have tests by default
   ifdef SB_ENABLE_TESTS
      CONFIGURE_ARGS += --enable-tests
      ENV_SETTING_WARN += --enable-tests
   endif
    
   OBJDIRNAME = $(OBJDIRBASENAME)-release$(SB_ARCH)
endif

# choose core wrappers to enable
ifdef SB_NO_MEDIA_CORE
   CONFIGURE_ARGS += --with-media-core=none
   ENV_SETTING_WARN += --with-media-core=none
endif #SB_NO_MEDIA_CORE

# breakpad support
ifdef SB_ENABLE_BREAKPAD
   CONFIGURE_ARGS += --enable-breakpad
   ENV_SETTING_WARN += --enable-breakpad
endif

# force installation of wmp core, so it's bundled with the application.
ifdef SB_FORCE_MEDIA_CORE_WMP
   CONFIGURE_ARGS += --with-media-core=windowsmedia \
                     --with-force-media-core=windowsmedia \
                     $(NULL)
   ENV_SETTING_WARN += --with-media-core=windowsmedia \
                       --with-force-media-core=windowsmedia \
                       $(NULL)
endif

# force installation of qt core, so it's bundled with the application.
ifdef SB_FORCE_MEDIA_CORE_QT
   CONFIGURE_ARGS += --with-media-core=qt \
                     --with-force-media-core=qt \
                     $(NULL)
   ENV_SETTING_WARN += --with-media-core=qt \
                       --with-force-media-core=qt \
                       $(NULL)
endif

# compiler environment checks
ifdef SB_DISABLE_COMPILER_ENVIRONMENT_CHECKS
   CONFIGURE_ARGS += --disable-compiler-environment-checks
   ENV_SETTING_WARN += --disable-compiler-environment-checks
endif

ifneq (,$(ENV_SETTING_WARN))
   $(warning WARNING: Setting build options via the environment is deprecated; add the following options to your nightingale.config: $(ENV_SETTING_WARN). Support for this will eventually go away.)
endif

OBJDIR = $(TOPSRCDIR)/$(OBJDIRNAME)
DISTDIR = $(OBJDIR)/$(DISTDIRNAME)
CONFIGSTATUS = $(OBJDIR)/config.status

#
# Set some needed commands; let configure figure out the rest
#

AUTOCONF ?= autoconf
MKDIR ?= mkdir -p
PERL ?= perl
RM ?= rm
SVN ?= svn

NIGHTINGALE_MESSAGE = Nightingale Build System

# When calling configure we need to use a relative path so that it will spit
# out relative paths for our makefiles.

CONFIGURE_PREREQS = $(ALLMAKEFILES) \
                    $(CONFIGUREAC) \
                    $(NULL)

all: nightingale_output build

debug release: all

$(CONFIGSTATUS): $(CONFIGURE) $(SB_DEP_PKG_LIST) $(OBJDIR) $(DISTDIR)
	cd $(OBJDIR) && \
   $(CONFIGURE) $(CONFIGURE_ARGS)

$(CONFIGURE): $(CONFIGURE_PREREQS)
	cd $(TOPSRCDIR) && \
    $(AUTOCONF) && \
    $(RM) -r $(TOPSRCDIR)/autom4te.cache/ 

nightingale_output:
	@echo $(NIGHTINGALE_MESSAGE)

run_autoconf:
	cd $(TOPSRCDIR) && \
    $(AUTOCONF) && \
    $(RM) -r $(TOPSRCDIR)/autom4te.cache/ 

$(OBJDIR) $(DISTDIR):
	$(MKDIR) $(OBJDIR) $(DISTDIR)

makefiles: $(OBJDIR) $(DISTDIR)
	@touch configure
	cd $(OBJDIR) && \
    $(CONFIGURE) $(CONFIGURE_ARGS)

run_configure: $(CONFIGSTATUS)
	cd $(OBJDIR) && \
   $(CONFIGURE) $(CONFIGURE_ARGS)

clean:
	$(MAKE) -C $(OBJDIR) clean

clobber:
	$(RM) $(CLOBBER_TRASH)
	$(RM) -r $(OBJDIR)

depclobber:
	$(RM) -r $(foreach p,$(SB_DEP_PKGS), $($(p)_DEP_DIR))

build : $(CONFIGSTATUS)
	$(MAKE) -C $(OBJDIR)

.PHONY : all debug nightingale_output run_autoconf run_configure clean clobber depclobber build 
