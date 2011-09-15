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

DEPTH = ..
topsrcdir = /media/disk/Linux/makepkg/songbird
srcdir = /media/disk/Linux/makepkg/songbird/branding
VPATH = /media/disk/Linux/makepkg/songbird/branding

include $(DEPTH)/build/autodefs.mk

# preferences are loaded alphabetically, we want to make sure this one loads
# after songbird-prefs.js so use -zzz
SONGBIRD_PP_RESOURCES = $(srcdir)/songbird-zzz-branding.js.in
SONGBIRD_PP_DIR = $(SONGBIRD_PREFERENCESDIR)

include $(topsrcdir)/build/rules.mk
