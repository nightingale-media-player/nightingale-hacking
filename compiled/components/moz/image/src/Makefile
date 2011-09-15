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

DEPTH = ../../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/components/moz/image/src
VPATH = /media/disk/Linux/makepkg/songbird/components/moz/image/src

include $(DEPTH)/build/autodefs.mk

CPP_SRCS = sbImageParser.cpp \
           sbImageParserModule.cpp \
           $(NULL)

CPP_EXTRA_INCLUDES = $(DEPTH)/components/moz/image/public \
                     $(topsrcdir)/components/include \
                     $(MOZSDK_INCLUDE_DIR)/necko \
                     $(NULL)

DYNAMIC_LIB_EXTRA_IMPORTS = plds4 \
                            plc4 \
                            $(NULL)
DYNAMIC_LIB = sbImageParser

IS_COMPONENT = 1

include $(topsrcdir)/build/rules.mk
