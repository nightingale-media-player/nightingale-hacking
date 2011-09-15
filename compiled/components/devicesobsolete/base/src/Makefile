#
# BEGIN SONGBIRD GPL
# 
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2008 POTI, Inc.
# http://www.songbirdnest.com
# 
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 (the GPL).
# 
# Software distributed under the License is distributed 
# on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either 
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
srcdir = /media/disk/Linux/makepkg/songbird/components/devicesobsolete/base/src
VPATH = /media/disk/Linux/makepkg/songbird/components/devicesobsolete/base/src

include $(DEPTH)/build/autodefs.mk

STATIC_LIB = devicebase

CPP_SRCS = DeviceBase.cpp \
           $(NULL)

CPP_EXTRA_INCLUDES = $(DEPTH)/components/devicesobsolete/base/public \
                     $(DEPTH)/components/dbengine/public \
                     $(DEPTH)/components/library/base/public \
                     $(DEPTH)/components/library/localdatabase/public \
                     $(topsrcdir)/components/library/localdatabase/src \
                     $(DEPTH)/components/property/public \
                     $(topsrcdir)/components/property/src \
                     $(MOZSDK_INCLUDE_DIR)/docshell \
                     $(MOZSDK_INCLUDE_DIR)/rdf \
                     $(MOZSDK_INCLUDE_DIR)/rdfutil \
                     $(MOZSDK_INCLUDE_DIR)/necko \
                     $(MOZSDK_INCLUDE_DIR)/unicharutil \
                     $(MOZSDK_IDL_DIR) \
                     $(NULL)

include $(topsrcdir)/build/rules.mk
