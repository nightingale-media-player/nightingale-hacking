#
# BEGIN SONGBIRD GPL
# 
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2008 POTI, Inc.
# http://www.songbirdnest.com
# 
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 ("the GPL").
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

DEPTH = ../../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/components/moz/strings/src
VPATH = /media/disk/Linux/makepkg/songbird/components/moz/strings/src

include $(DEPTH)/build/autodefs.mk

STATIC_LIB = sbMozStringUtils

CPP_SRCS = sbTArrayStringEnumerator.cpp \
           sbStringBundle.cpp \
           sbStringUtils.cpp \
           $(NULL)

CPP_EXTRA_INCLUDES = $(DEPTH)/components/moz/strings/components/public \
                     $(topsrcdir)/components/include \
                     $(topsrcdir)/components/moz/strings/src \
                     $(MOZSDK_INCLUDE_DIR)/intl \
                     $(MOZSDK_INCLUDE_DIR)/uconv \
                     $(NULL)

# preedTODO: fix this hack
libs::
ifeq (macosx,$(SB_PLATFORM))
	$(LN) -sf $(OUR_STATIC_LIB) lib$(OUR_STATIC_LIB)
endif

GARBAGE += lib$(OUR_STATIC_LIB) 

include $(topsrcdir)/build/rules.mk

