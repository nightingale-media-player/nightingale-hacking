#
# BEGIN SONGBIRD GPL
# 
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2008 POTI, Inc.
# http://www.songbirdnest.com
# 
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 (the "GPL").
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
srcdir = /media/disk/Linux/makepkg/songbird/locales
VPATH = /media/disk/Linux/makepkg/songbird/locales

include $(DEPTH)/build/autodefs.mk

JAR_MANIFEST = jar.mn.in


ifeq (default,$(SB_SEARCHPLUGINS))
   SONGBIRD_SEARCHPLUGINS = \
    $(srcdir)/en-US/searchplugins/songbird-internal-search.xml \
    $(NULL)

else
   SONGBIRD_SEARCHPLUGINS = \
     $(addprefix $(srcdir)/en-US/searchplugins/,\
     $(subst $(COMMA),$(SPACE),$(SB_SEARCHPLUGINS)))
endif

include $(topsrcdir)/build/rules.mk
