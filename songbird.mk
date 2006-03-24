#
# BEGIN SONGBIRD GPL
# 
# This file is part of the Songbird web player.
#
# Copyright© 2006 Pioneers of the Inevitable LLC
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

CWD := $(shell pwd)
ifeq "$(CWD)" "/"
CWD := /.
endif

ROOTDIR   := $(shell dirname $(CWD))
TOPSRCDIR := $(CWD)

CONFIGURE    = $(TOPSRCDIR)/configure
ALLMAKEFILES = $(TOPSRCDIR)/allmakefiles.sh
CONFIGUREAC  = $(TOPSRCDIR)/configure.ac
OBJDIR       = $(TOPSRCDIR)/$(OBJDIRNAME)
DISTDIR      = $(OBJDIR)/$(DISTDIRNAME)
CONFIGSTATUS = $(OBJDIR)/config.status

ifeq (debug,$(MAKECMDGOALS))
DEBUG = 1
endif

ifdef DEBUG
CONFIGURE_ARGS = --enable-debug
endif

ifndef AUTOCONF
AUTOCONF := autoconf
endif

ifndef MKDIR
MKDIR := mkdir
endif

ifndef MAKE
MAKE := gmake
endif

SONGBIRD_MESSAGE = Songbird Web Player v0.2

RUN_AUTOCONF_CMD = cd $(TOPSRCDIR) && \
                   $(AUTOCONF) && \
                   rm -rf $(TOPSRCDIR)/autom4te.cache/ \
                   $(NULL)

CREATE_OBJ_DIR_CMD = $(MKDIR) -p $(OBJDIR) $(DISTDIR) \
                     $(NULL)

RUN_CONFIGURE_CMD = cd $(OBJDIR) && \
                    $(CONFIGURE) $(CONFIGURE_ARGS) \
                    $(NULL)

CLEAN_CMD = $(MAKE) -C $(OBJDIR) clean \
            $(NULL)

CLOBBER_CMD = rm -rf $(CONFIGURE) && \
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
	$(CREATE_OBJ_DIR_CMD) && $(RUN_CONFIGURE_CMD)

$(CONFIGURE) : $(CONFIGURE_PREREQS)
	$(RUN_AUTOCONF_CMD)

songbird_output:
	@echo $(SONGBIRD_MESSAGE)

run_autoconf :
	$(RUN_AUTOCONF_CMD)

create_obj_dir :
	$(CREATE_OBJ_DIR_CMD)

run_configure : $(CONFIGSTATUS)

clean :
	$(CLEAN_CMD)

clobber:
	$(CLOBBER_CMD)

build : $(CONFIGSTATUS)
	$(BUILD_CMD)

.PHONY : all debug songbird_output run_autoconf create_dist_dir run_configure clean clobber build
