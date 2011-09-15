#
# BEGIN SONGBIRD GPL
# 
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2008 POTI, Inc.
# http://www.songbirdnest.com
# 
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 (the “GPL”).
# 
# Software distributed under the License is distributed 
# on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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

DEPTH = ../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/components/dataremote/src
VPATH = /media/disk/Linux/makepkg/songbird/components/dataremote/src

include $(DEPTH)/build/autodefs.mk

JAR_MANIFEST = jar.mn.in

DYNAMIC_LIB = sbdataremote

CPP_SRCS = sbDataRemoteWrapper.cpp \
           sbDataRemoteModule.cpp \
           $(NULL)

CPP_EXTRA_INCLUDES = $(DEPTH)/components/dataremote/public \
                     $(DEPTH)/components/remoteapi/public \
                     $(MOZSDK_INCLUDE_DIR)/unicharutil \
                     $(MOZSDK_IDL_DIR) \
                     $(NULL)

# build a component (ie: a dynamically loadable module)
IS_COMPONENT = 1

DYNAMIC_LIB_EXTRA_IMPORTS = plds4 \
                            $(NULL)

SONGBIRD_COMPONENTS = $(topsrcdir)/components/dataremote/src/sbDataRemote.js \
                      $(NULL)

include $(topsrcdir)/build/rules.mk
