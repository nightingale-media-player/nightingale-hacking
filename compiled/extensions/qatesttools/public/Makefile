#
#=BEGIN SONGBIRD GPL
#
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2009 POTI, Inc.
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

DEPTH = ../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/extensions/qatesttools/public
VPATH = /media/disk/Linux/makepkg/songbird/extensions/qatesttools/public \
        $(MOZJSHTTPD_DIR)

include $(DEPTH)/build/autodefs.mk

IS_EXTENSION = 1

#
# Only package the js httpd when we are building without unit tests. Otherwise 
# there will be two copies of the interface and they are likely to conflict with 
# each other in weird an annoying ways.
#
ifeq (,$(SB_ENABLE_TESTS)$(SB_ENABLE_TEST_HARNESS))
XPIDL_SRCS = nsIHttpServer.idl \
             $(NULL)

XPIDL_MODULE = sbQATestTools.xpt
endif

include $(topsrcdir)/build/rules.mk

