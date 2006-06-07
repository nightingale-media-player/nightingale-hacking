#! /bin/sh
#
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 1999
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

# allmakefiles.sh - List of all makefiles. 
#   Appends the list of makefiles to the variable, MAKEFILES.
#   There is no need to rerun autoconf after adding makefiles.
#   You only need to run configure.
#
#   Please keep the modules in this file in sync with those in
#   mozilla/build/unix/modules.mk
#

MAKEFILES=""

# add_makefiles - Shell function to add makefiles to MAKEFILES
add_makefiles() {
    MAKEFILES="$MAKEFILES $*"
}

if [ "$srcdir" = "" ]; then
    srcdir=.
fi

#
# App makefiles
#

MAKEFILES_app="
Makefile
app/Makefile
app/content/Makefile
app/content/bindings/Makefile
app/content/browser/Makefile
app/content/browser/content/Makefile
app/content/browser/locale/Makefile
app/content/browser/skin/Makefile
app/content/html/Makefile
app/content/scripts/Makefile
app/content/xul/Makefile
app/icons/Makefile
app/preferences/Makefile
"

MAKEFILES_build="
build/autodefs.mk
"

MAKEFILES_components="
components/Makefile
"

MAKEFILES_dependencies="
dependencies/Makefile
"

MAKEFILES_feathers="
feathers/Makefile
feathers/cardinal/Makefile
feathers/dove/Makefile
feathers/rubberducky/Makefile
"

MAKEFILES_installer="
installer/Makefile
installer/linux/Makefile
installer/macosx/Makefile
installer/win32/Makefile
"

MAKEFILES_locales="
locales/Makefile
locales/branding/Makefile
locales/en-US/Makefile
"

#
# Component makefiles
#
MAKEFILES_bundle="
components/bundle/Makefile
components/bundle/public/Makefile
components/bundle/src/Makefile
"

MAKEFILES_dataremote="
components/dataremote/Makefile
components/dataremote/public/Makefile
components/dataremote/src/Makefile
"

MAKEFILES_devices="
components/devices/Makefile
components/devices/base/Makefile
components/devices/base/public/Makefile
components/devices/base/src/Makefile
"

MAKEFILES_devicemanager="
components/devices/manager/Makefile
components/devices/manager/public/Makefile
components/devices/manager/src/Makefile
"

MAKEFILES_cddevice="
components/devices/cd/Makefile
components/devices/cd/public/Makefile
components/devices/cd/src/Makefile
"

MAKEFILES_downloaddevice="
components/devices/download/Makefile
components/devices/download/public/Makefile
components/devices/download/src/Makefile
"

MAKEFILES_wmdevice="
components/devices/wm/Makefile
components/devices/wm/public/Makefile
components/devices/wm/src/Makefile
"

MAKEFILES_usb_mass_storagedevice="
components/devices/usb_mass_storage/Makefile
components/devices/usb_mass_storage/public/Makefile
components/devices/usb_mass_storage/src/Makefile
"

MAKEFILES_integration="
components/integration/Makefile
components/integration/public/Makefile
components/integration/src/Makefile
"

MAKEFILES_mediacore="
components/mediacore/Makefile
components/mediacore/metadata/Makefile
components/mediacore/metadata/handler/Makefile
components/mediacore/metadata/handler/id3/Makefile
components/mediacore/metadata/handler/id3/public/Makefile
components/mediacore/metadata/handler/id3/src/Makefile
components/mediacore/metadata/manager/Makefile
components/mediacore/metadata/manager/public/Makefile
components/mediacore/metadata/manager/src/Makefile
components/mediacore/transcoding/Makefile
components/mediacore/transcoding/public/Makefile
components/mediacore/transcoding/src/Makefile
"

MAKEFILES_medialibrary="
components/medialibrary/Makefile
components/medialibrary/public/Makefile
components/medialibrary/src/Makefile
"

MAKEFILES_metrics="
components/metrics/Makefile
components/metrics/public/Makefile
components/metrics/src/Makefile
"

MAKEFILES_playlistplayback="
components/playlistplayback/Makefile
components/playlistplayback/src/Makefile
components/playlistplayback/public/Makefile
"

MAKEFILES_playlistreader="
components/playlistreader/Makefile
components/playlistreader/m3u/Makefile
components/playlistreader/m3u/public/Makefile
components/playlistreader/m3u/src/Makefile
components/playlistreader/pls/Makefile
components/playlistreader/pls/public/Makefile
components/playlistreader/pls/src/Makefile
"

MAKEFILES_playlistsource="
components/playlistsource/Makefile
components/playlistsource/public/Makefile
components/playlistsource/src/Makefile
"

MAKEFILES_servicesource="
components/servicesource/Makefile
components/servicesource/public/Makefile
components/servicesource/src/Makefile
"

#
# Put it all together
#

add_makefiles "
$MAKEFILES_app
$MAKEFILES_build
$MAKEFILES_components
$MAKEFILES_dependencies
$MAKEFILES_feathers
$MAKEFILES_installer
$MAKEFILES_locales
$MAKEFILES_bundle
$MAKEFILES_dataremote
$MAKEFILES_devices
$MAKEFILES_devicemanager
$MAKEFILES_cddevice
$MAKEFILES_downloaddevice
$MAKEFILES_wmdevice
$MAKEFILES_usb_mass_storagedevice
$MAKEFILES_integration
$MAKEFILES_mediacore
$MAKEFILES_medialibrary
$MAKEFILES_metrics
$MAKEFILES_playlistplayback
$MAKEFILES_playlistreader
$MAKEFILES_playlistsource
$MAKEFILES_servicesource
"
