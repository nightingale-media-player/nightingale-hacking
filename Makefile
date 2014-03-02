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

#
# I hate hardcoding this, but until we have more time that's
#   just how it's gonna be... This MUST match what's in
#   configure.ac or bad things will happen!
#
SHELL = /bin/sh

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

CONFIGURE = $(TOPSRCDIR)/configure
ALLMAKEFILES = $(TOPSRCDIR)/allmakefiles.sh
CONFIGUREAC = $(TOPSRCDIR)/configure.ac
OBJDIR = $(TOPSRCDIR)/$(OBJDIRNAME)
DISTDIR = $(OBJDIR)/$(DISTDIRNAME)
CONFIGSTATUS = $(OBJDIR)/config.status

CLOBBER_TRASH = $(CONFIGURE) \
                $(TOPSRCDIR)/.songbirdconfig.mk \
                $(TOPSRCDIR)/.songbirdconfig.out \
                $(NULL)

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
#    We don't support this anymore, because it was always broken in the
#    rebuild case (bug 17128)
ifneq (,$(filter debug release,$(MAKECMDGOALS)))
   $(error make -f songbird.mk [debug|release] is no longer supported; add your desired options to songbird.config)
endif

# These are all of the old environment variables that used to control
# various builds options.
ifneq (,$(SB_ENABLE_INSTALLER)$(SONGBIRD_NIGHTLY)$(SONGBIRD_OFFICIAL)$(SB_UPDATE_CHANNEL)$(SB_ENABLE_JARS)$(SB_DISABLE_JARS)$(SB_DISABLE_TESTS)$(SB_ENABLE_TESTS)$(SB_FORCE_MEDIA_CORE_WMP)$(SB_FORCE_MEDIA_CORE_QT)$(SB_DISABLE_COMPILER_ENVIRONMENT_CHECKS))
   $(error Setting build options via the environment is no longer supported; create/edit a nightingale.config file.)
endif

#
# Set some needed commands; let configure figure out the rest
#

AUTOCONF ?= autoconf
MKDIR ?= mkdir -p
PERL ?= perl
RM ?= rm
CP ?= cp
LN ?= ln
INSTALL ?= install

SONGBIRD_MESSAGE = Nightingale Build System

# When calling configure we need to use a relative path so that it will spit
# out relative paths for our makefiles.

CONFIGURE_PREREQS = $(ALLMAKEFILES) \
                    $(CONFIGUREAC) \
                    $(NULL)

#
# Prepare tests command
#
ifeq (,$(filter --enable-tests, $(CONFIGURE_ARGS)))
    TEST_COMMAND = $(error Not a Build with enabled Tests. Please set --enable-tests.)
else
    TEST_COMMAND = $(DISTDIR)/nightingale -test
endif

#
# Prepare install
# For best practices see: https://www.gnu.org/prep/standards/html_node/Makefile-Conventions.html#Makefile-Conventions
# Should use the install command to install Nightingale, however since it's not recursive that's non-trivial.
#
UNAME_S := $(shell uname -s)
prefix = /usr
exec_prefix = $(prefix)
libdir ?= $(exec_prefix)/lib
bindir ?= $(exec_prefix)/bin
datarootdir ?= $(prefix)/share
mandir ?= $(datarootdir)/man
man1dir ?= $(mandir)/man1
manext ?= .1
man1ext ?= $(manext)
docdir ?= $(datarootdir)/doc/nightingale
htmldir ?= $(docdir)
dvidir ?= $(docdir)
pdfdir ?= $(docdir)
psdir ?= $(docdir)
infodir ?= $(datarootdir)/info
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = ${INSTALL} -m 644

ifneq (Windows_NT,$(OS))
    ifeq (Darwin,$(UNAME_S))
        INSTALL_CMD = @echo Please use the .dmg file in compiled/dist.
    endif
    ifeq (Linux, $(UNAME_S))
        INSTALL_CMD = $(MAKE) install-linux

        UNINSTALL_CMD = $(RM) -r $(DESTDIR)$(libdir)/nightingale &&\
                        $(RM) $(DESTDIR)$(bindir)/nightingale &&\
                        $(RM) $(DESTDIR)$(man1dir)/nightingale$(man1ext).gz
        ifndef DESTDIR
            POST_INSTALL_CMD = xdg-icon-resource install --novendor --size 512 $(DISTDIR)/chrome/icons/default/default.xpm nightingale &&\
                               xdg-desktop-menu install --novendor $(TOPSRCDIR)/nightingale.desktop

            POST_UNINSTALL_CMD = xdg-icon-resource uninstall --size 512 nightingale &&\
                                 xdg-desktop-menu uninstall $(TOPSRCDIR)/debian/nightingale.desktop
        endif
    endif
else
    INSTALL_CMD = @echo Please use the installer located in compiled/dist.
endif

INSTALL_CMD ?= $(error Installing using make is currently not supported on your operating system.)
UNINSTALL_CMD ?= $(error Uninstalling using make is currently not supported on your operating system.)

all: songbird_output build

run_configure $(CONFIGSTATUS): $(CONFIGURE) $(SB_DEP_PKG_LIST) $(OBJDIR) $(DISTDIR)
	cd $(OBJDIR) && \
    $(CONFIGURE) $(CONFIGURE_ARGS)

$(CONFIGURE): $(CONFIGURE_PREREQS)
	cd $(TOPSRCDIR) && \
    $(AUTOCONF) && \
    $(RM) -r $(TOPSRCDIR)/autom4te.cache/ 

songbird_output:
	@echo $(SONGBIRD_MESSAGE)

run_autoconf:
	cd $(TOPSRCDIR) && \
    $(AUTOCONF) && \
    $(RM) -r $(TOPSRCDIR)/autom4te.cache/ 

$(OBJDIR) $(DISTDIR):
	$(MKDIR) $(OBJDIR) $(DISTDIR)

makefiles: $(OBJDIR) $(DISTDIR) run_configure

clean:
	$(RM) -rf $(OBJDIR)

clobber:
	$(RM) $(CLOBBER_TRASH)
	$(RM) -r $(OBJDIR)

depclobber:
	$(RM) -r $(foreach p,$(SB_DEP_PKGS), $($(p)_DEP_DIR))

build : $(CONFIGSTATUS)
	$(MAKE) -C $(OBJDIR)

test:
	$(TEST_COMMAND)

install:
	$(PRE_INSTALL)
	$(NORMAL_INSTALL) # Normal commands follow
	$(INSTALL_CMD)
	$(POST_INSTALL) # Post-install commands follow
	$(POST_INSTALL_CMD)

uninstall:
	$(PRE_UNINSTALL)
	$(NORMAL_UNINSTALL) # Normal commands follow
	$(UNINSTALL_CMD)
	$(POST_UNINSTALL) # Post-uninstallation commands follow
	$(POST_UNINSTALL_CMD)

installdirs:
	$(MKDIR) $(DESTDIR)$(bindir)
	$(MKDIR) $(DESTDIR)$(libdir)/nightingale

install-linux:
	$(MAKE) installdirs
	$(CP) -r $(DISTDIR)/* $(DESTDIR)$(libdir)/nightingale
	$(LN) -s $(DESTDIR)$(libdir)/nightingale/nightingale $(DESTDIR)$(bindir)/nightingale
	-$(INSTALL_DATA) $(OBJDIR)/documentation/manpage/nightingale$(man1ext).gz $(DESTDIR)$(man1dir)

.PHONY : all debug songbird_output run_autoconf run_configure clean clobber depclobber build test install uninstall installdirs install-linux
