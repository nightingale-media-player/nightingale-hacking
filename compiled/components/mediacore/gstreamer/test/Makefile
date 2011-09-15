#
# BEGIN SONGBIRD GPL
#
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2008 POTI, Inc.
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

DEPTH = ../../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/components/mediacore/gstreamer/test
VPATH = /media/disk/Linux/makepkg/songbird/components/mediacore/gstreamer/test

include $(DEPTH)/build/autodefs.mk

SONGBIRD_TEST_COMPONENT = gstreamer

SONGBIRD_TESTS = $(srcdir)/head_gstreamer.js \
                 $(srcdir)/test_plugins.js \
                 $(srcdir)/test_transcode_profiles.js \
                 $(srcdir)/test_gst_transcode_configurator.js \
                 $(srcdir)/test_audio_processing.js \
                 $(NULL)

GSTREAMER_TEST_FILES = $(srcdir)/files/simple.ogg \
                      $(srcdir)/files/surround51.ogg \
                      $(srcdir)/files/video.ogg \
                      $(srcdir)/files/corrupt-in-middle.ogg
                      $(NULL)

GSTREAMER_TEST_FILES_DIR = $(SONGBIRD_TEST_COMPONENT_DIR)/files

ifeq (1,$(SB_ENABLE_TESTS))
   CREATEDIRS = $(GSTREAMER_TEST_FILES_DIR)
endif

include $(topsrcdir)/build/rules.mk

export::
ifeq (1,$(SB_ENABLE_TESTS))
	$(INSTALL_FILE) $(GSTREAMER_TEST_FILES) $(GSTREAMER_TEST_FILES_DIR)
endif

