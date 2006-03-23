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

DIST_DIR_NAME = dist

CWD := $(shell pwd)
ifeq "$(CWD)" "/"
CWD := /.
endif

ROOTDIR   := $(shell dirname $(CWD))
TOPSRCDIR := $(CWD)

CONFIGURE    = $(TOPSRCDIR)/configure
ALLMAKEFILES = $(TOPSRCDIR)/allmakefiles.sh
CONFIGUREAC  = $(TOPSRCDIR)/configure.ac
DISTDIR      = $(TOPSRCDIR)/$(DIST_DIR_NAME)
CONFIGSTATUS = $(DISTDIR)/config.status

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

CREATE_DIST_DIR_CMD = cd $(TOPSRCDIR) && \
                      $(MKDIR) -p $(DIST_DIR_NAME) \
                      $(NULL)

RUN_CONFIGURE_CMD = cd $(DISTDIR) && \
                    $(CONFIGURE) \
                    $(NULL)

CLEAN_CMD = cd $(TOPSRCDIR) && \
            rm -rf $(DISTDIR) && \
            rm -rf $(TOPSRCDIR)/autom4te.cache/ && \
            rm -f $(CONFIGURE) \
            $(NULL)

BUILD_CMD = cd $(DISTDIR) && \
            $(MAKE) \
            $(NULL)

CONFIGURE_PREREQS = $(ALLMAKEFILES) \
                    $(CONFIGUREAC) \
                    $(NULL) 

all : songbird_output build

$(CONFIGSTATUS) : $(CONFIGURE)
	$(CREATE_DIST_DIR_CMD) && $(RUN_CONFIGURE_CMD)

$(CONFIGURE) : $(CONFIGURE_PREREQS)
	$(RUN_AUTOCONF_CMD)

songbird_output:
	@echo $(SONGBIRD_MESSAGE)

run_autoconf :
	$(RUN_AUTOCONF_CMD)

create_dist_dir :
	$(CREATE_DIST_DIR_CMD)

run_configure : $(CONFIGURE_PREREQS)
	$(RUN_CONFIGURE_CMD)

clean :
	$(CLEAN_CMD)

build : $(CONFIGSTATUS)
	$(BUILD_CMD)

.PHONY : all songbird_output run_autoconf create_dist_dir run_configure clean build
