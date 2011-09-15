#
# BEGIN SONGBIRD GPL
#
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2010 POTI, Inc.
# http://songbirdnest.com
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

DEPTH = ../../../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/components/devices/firmware/test/testfiles
VPATH = /media/disk/Linux/makepkg/songbird/components/devices/firmware/test/testfiles

include $(DEPTH)/build/autodefs.mk

#
# According to head.songbird.js - |getTestServerPortNumber()|, the default port
# number is 8180, but can be set via the enviroment variable
# |SONGBIRD_TEST_SERVER_PORT|.
#
ifndef SONGBIRD_TEST_SERVER_PORT
   PP_TEST_SERVER_PORT = 8180
else
   PP_TEST_SERVER_PORT = $(SONGBIRD_TEST_SERVER_PORT)
endif

PPDEFINES = -DSONGBIRD_TEST_SERVER_PORT="$(PP_TEST_SERVER_PORT)" \
            $(NULL)

SONGBIRD_PP_RESOURCES = $(srcdir)/firmware.xml.in \
                        $(NULL)

SONGBIRD_TEST_COMPONENT = devicefirmware/files

TEST_FILE_EXTENSIONS = xml \
                       html \
                       bin \
                       $(NULL)

SONGBIRD_TESTS = \
   $(foreach e,$(TEST_FILE_EXTENSIONS),$(wildcard $(srcdir)/*.$e)) \
   $(foreach e,$(TEST_FILE_EXTENSIONS),$(wildcard $(DEPTH)/components/devices/firmware/test/testfiles/*.$e)) \
   $(NULL)

include $(topsrcdir)/build/rules.mk


