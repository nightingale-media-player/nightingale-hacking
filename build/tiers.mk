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

##############################################################################
# tiers.mk
#
# Define the various tiers of our application.
###############################################################################

#------------------------------------------------------------------------------
# Only include this file once
ifndef TIERS_MK_INCLUDED
TIERS_MK_INCLUDED=1
#------------------------------------------------------------------------------

# List of available tiers... SYNC THIS
# preedTODO: SYNC THIS UP and do a verification on it; investigate using auto
# rules for this
tiers = \
   base \
   deps \
   components \
   $(NULL)

tier_base_dirs = \
   build \
   tools \
   $(NULL)

tier_componentsbase_dirs = \
   components/moz \
   components/intl \
   components/property \
   components/library \
   components/mediacore \
   components/albumart \
   components/dbengine \
   components/sqlbuilder \
   components/job \
   components/metrics \
   components/dataremote \
   components/mediaimport \
   components/filesystemevents \
   components/watchfolder \
   components/controller \
   components/devices \
   components/devicesobsolete \
   components/mediamanager \
   $(NULL)

tier_componentsall_dirs = \
   components/addonmetadata \
   components/bundle \
   components/commandline \
   components/contenthandling \
   components/displaypanes \
   components/draganddrop \
   components/faceplate \
   components/feathers \
   components/integration \
   components/jscodelib \
   components/mediapages \
   components/parsererror \
   components/playlistcommands \
   components/protocol \
   components/remoteapi \
   components/search \
   components/servicepane \
   components/update \
   components/webservices \
   components/mediaexport \
   components/appstartup \
   components/networkproxy \
   $(NULL)

tier_appbase_dirs = \
   app \
   $(NULL)

tier_deps_dirs = \
   dependencies \
   $(NULL)

tier_ui_dirs = \
   bindings \
   feathers \
   locales \
   $(NULL)

tier_extensions_dirs = \
   extensions \
   $(NULL)

ifdef HAS_EXTRAS
   tier_extensions_dirs += extras/extensions
endif

tier_branding_dirs = \
   $(SONGBIRD_BRANDING_DIR) \
   $(NULL)

tier_unittests_dirs = \
   components/testharness \
   $(NULL)

tier_update_dirs = \
   update \
   $(NULL)

tier_installer_dirs = \
   installer \
   $(NULL)

#------------------------------------------------------------------------------
endif #TIERS_MK_INCLUDED
#------------------------------------------------------------------------------
