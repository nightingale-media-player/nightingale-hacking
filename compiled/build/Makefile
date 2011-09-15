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

DEPTH = ..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/build
VPATH = /media/disk/Linux/makepkg/songbird/build

include $(DEPTH)/build/autodefs.mk

AWK_CMD = $(AWK) $(AWK_EXPR) < $(MOZSDK_INCLUDE_DIR)/mozilla-config.h
AWK_EXPR = '/\#define MOZILLA_VERSION_U/ { gsub(/"/, "", $$3); print $$3 }'

BUILD_INFO_MK_FILE = sbBuildInfo.mk

export:: $(BUILD_INFO_MK_FILE)

GARBAGE += $(BUILD_INFO_MK_FILE)

export SONGBIRD_OFFICIAL

$(BUILD_INFO_MK_FILE): FORCE
	$(RM) -f $@
	$(PYTHON) $(srcdir)/make-buildinfo.py -i $(srcdir)/$@.in -o $@ \
    -k SB_MOZILLA_VERSION=$(shell $(AWK_CMD)) 

include $(topsrcdir)/build/rules.mk
