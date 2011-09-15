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

DEPTH = ../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/components/remoteapi/test
VPATH = /media/disk/Linux/makepkg/songbird/components/remoteapi/test

include $(DEPTH)/build/autodefs.mk

SONGBIRD_TEST_COMPONENT = remoteapi


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

SONGBIRD_PP_RESOURCES = $(srcdir)/test_batchCalls_page.html.in \
                        $(srcdir)/test_privileged_page.html.in \
                        $(srcdir)/test_remotelibrary_page.html.in \
                        $(srcdir)/media/remoteLibraryTest.m3u.in \
                        $(NULL)

# TODO: XXX Add these tests back in once they are fixed
# see bugs 23940-23945
#                        $(srcdir)/test_remotemedialist_add_page.html.in \
#                        $(srcdir)/test_remotedownload_page.html.in \

SONGBIRD_TESTS = $(srcdir)/head_remoteapi.js \
                 $(srcdir)/tail_remoteapi.js \
                 $(srcdir)/test_batchCalls.js \
                 $(DEPTH)/components/remoteapi/test/test_batchCalls_page.html \
                 $(srcdir)/test_crossLibAdd.js \
                 $(srcdir)/test_crossLibAdd_page.html \
                 $(srcdir)/test_empty.js \
                 $(srcdir)/test_empty_page.html \
                 $(srcdir)/test_remotelibrary.js \
                 $(DEPTH)/components/remoteapi/test/test_remotelibrary_page.html \
                 $(srcdir)/test_remotelibrary_duplicates.js \
                 $(srcdir)/test_remotelibrary_duplicates_page.html \
                 $(srcdir)/test_remotelibrary_getters.js \
                 $(srcdir)/test_remotelibrary_getters_page.html \
                 $(srcdir)/test_remotelibrary_enumerator.js \
                 $(srcdir)/test_remotelibrary_enumerator_page.html \
                 $(srcdir)/test_remotelibrary_playlists_enumerator_page.html \
                 $(srcdir)/test_remotelibrary_playlists_enumerator.js \
                 $(srcdir)/test_remotemediaitem.js \
                 $(srcdir)/test_remotemediaitem_page.html \
                 $(srcdir)/test_remotemedialist.js \
                 $(srcdir)/test_remotemedialist_page.html \
                 $(srcdir)/test_remotemedialist_add2.js \
                 $(srcdir)/test_remotemedialist_add2_page.html \
                 $(srcdir)/test_remoteplayer.js \
                 $(srcdir)/test_remoteplayer_page.html \
                 $(srcdir)/test_remoteplayerlisteners.js \
                 $(srcdir)/test_remoteplayerlisteners_page.html \
                 $(srcdir)/test_siteID.js \
                 $(srcdir)/test_siteID_page.html \
                 $(srcdir)/test_privileged.js \
                 $(DEPTH)/components/remoteapi/test/test_privileged_page.html \
                 $(srcdir)/test_properties.js \
                 $(srcdir)/test_properties_page.html \
                 $(srcdir)/test_properties_urls.js \
                 $(srcdir)/test_properties_urls_page.html \
                 $(srcdir)/media/test01.mp3 \
                 $(srcdir)/media/test02.mp3 \
                 $(srcdir)/media/test03.mp3 \
                 $(srcdir)/media/test04.mp3 \
                 $(srcdir)/media/test05.mp3 \
                 $(srcdir)/media/test06.mp3 \
                 $(srcdir)/media/test07.mp3 \
                 $(srcdir)/media/test08.mp3 \
                 $(srcdir)/media/test09.mp3 \
                 $(srcdir)/media/test10.mp3 \
                 $(srcdir)/media/test11.mp3 \
                 $(srcdir)/media/test12.mp3 \
                 $(srcdir)/media/test13.mp3 \
                 $(srcdir)/media/test14.mp3 \
                 $(srcdir)/media/test15.mp3 \
                 $(srcdir)/media/test16.mp3 \
                 $(srcdir)/media/DJBLUE_-_Hear_Me.mp3 \
                 $(srcdir)/media/jaspertine_-_Introvert.mp3 \
                 $(srcdir)/media/stefsax_-_I_like_it_like_Thats.thaens.mp3 \
                 $(srcdir)/media/Sawtooth_-_We_Are_Not_Evil_Down_To_Brass_Tacks_Remix.mp3 \
                 $(srcdir)/media/Vanchy_-_Black_Is_The_Night_Vanchy_s_Re-Fix.mp3 \
                 $(DEPTH)/components/remoteapi/test/remoteLibraryTest.m3u \
                 $(srcdir)/media/nyx_-_Undercover.mp3 \
                 $(NULL)

# TODO: XXX Add these tests back in once they are fixed
# see bugs 23940-23945
#                 $(DEPTH)/components/remoteapi/test/test_remotedownload_page.html \
#                 $(DEPTH)/components/remoteapi/test/test_remotemedialist_add_page.html \
#                 $(srcdir)/test_remotedownload.js \
#                 $(srcdir)/test_remotemedialist_add.js \

include $(topsrcdir)/build/rules.mk
