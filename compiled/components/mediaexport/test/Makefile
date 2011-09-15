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

DEPTH = ../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/components/mediaexport/test
VPATH = /media/disk/Linux/makepkg/songbird/components/mediaexport/test

include $(DEPTH)/build/autodefs.mk

SONGBIRD_TEST_COMPONENT = mediaexportservice 

SONGBIRD_TESTS = $(srcdir)/head_mediaexportservice.js \
                 $(srcdir)/test_mediaexportservice.js \
                 $(NULL)

MEDIAEXPORT_TEST_FILES = $(srcdir)/files/one.mp3 \
                         $(srcdir)/files/two.mp3 \
                         $(srcdir)/files/three.mp3 \
                         $(srcdir)/files/four.mp3 \
                         $(srcdir)/files/five.mp3 \
                         $(srcdir)/files/six.mp3 \
                         $(NULL)

MEDIAEXPORT_TEST_FILES_DIR = $(SONGBIRD_TEST_COMPONENT_DIR)/files

ifeq (1,$(SB_ENABLE_TESTS))
   CREATEDIRS = $(MEDIAEXPORT_TEST_FILES_DIR)
endif

include $(topsrcdir)/build/rules.mk

export::
ifeq (1,$(SB_ENABLE_TESTS))
	$(INSTALL_FILE) $(MEDIAEXPORT_TEST_FILES) $(MEDIAEXPORT_TEST_FILES_DIR)
endif
