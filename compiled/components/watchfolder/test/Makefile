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

DEPTH = ../../..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/components/watchfolder/test
VPATH = /media/disk/Linux/makepkg/songbird/components/watchfolder/test

include $(DEPTH)/build/autodefs.mk

SONGBIRD_TEST_COMPONENT = watchfolders

SONGBIRD_TESTS = $(srcdir)/head_watchfolders.js \
                 $(srcdir)/tail_watchfolders.js \
                 $(srcdir)/test_moverename.js \
                 $(NULL)


WATCHFOLDER_TESTDIR = $(SONGBIRD_TESTSDIR)/watchfolders/files

WATCHFOLDER_TEST_FILE_ROOT = $(srcdir)/files

WATCHFOLDER_TEST_FILES := $(shell cd $(WATCHFOLDER_TEST_FILE_ROOT) && \
 $(FIND) . -type f \
 ! -path '*.svn*' \
 ! -name 'Makefile.in')

CREATEDIRS = $(WATCHFOLDER_TESTDIR)

include $(topsrcdir)/build/rules.mk

export::
	cd $(WATCHFOLDER_TEST_FILE_ROOT) && $(CP) -P -f -p --parents $(WATCHFOLDER_TEST_FILES) $(WATCHFOLDER_TESTDIR)
