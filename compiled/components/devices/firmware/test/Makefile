#
# BEGIN SONGBIRD GPL
# 
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2009 POTI, Inc.
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
srcdir = /media/disk/Linux/makepkg/songbird/components/devices/firmware/test
VPATH = /media/disk/Linux/makepkg/songbird/components/devices/firmware/test \
        /media/disk/Linux/makepkg/songbird/components/devices/base/src \
        /media/disk/Linux/makepkg/songbird/components/devices/firmware/src

include $(DEPTH)/build/autodefs.mk

SONGBIRD_TEST_COMPONENT = devicefirmware

SONGBIRD_TESTS = $(srcdir)/test_device_firmware_handler.js \
                 $(NULL)

SUBDIRS = testfiles \
          $(NULL)

include $(topsrcdir)/build/rules.mk
