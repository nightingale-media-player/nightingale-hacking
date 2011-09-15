#!/bin/sh
#
#=BEGIN SONGBIRD GPL
#
# This file is part of the Songbird web player.
#
# Copyright(c) 2005-2010 POTI, Inc.
# http://www.songbirdnest.com
#
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 (the ``GPL'').
#
# Software distributed under the License is distributed
# on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
# express or implied. See the GPL for the specific language
# governing rights and limitations.
#
# You should have received a copy of the GPL along with this
# program. If not, go to http://www.gnu.org/licenses/gpl.html
# or write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
#=END SONGBIRD GPL
#

#filter substitution

#
# Songbird Windows application forward update test script.
#
# Options:
#
#   -clobber                    Perform a clobber and full rebuild.
#
#   This script may be used to test updating of the Songbird application on
# Windows.  This script tests updating of the locally built Songbird.
#   This script builds a Songbird mar file, sets up the update files on the
# specified web server, installs the locally built Songbird as the base
# Songbird, patches it to use the specified web server for updates, and launches
# the base Songbird.
#   To test, follow the instructions below.  When the script completes, the
# base, non-updated Songbird should be open and presenting an application update
# page in the first-run wizard for updating to the locally built Songbird.
#   IMPORTANT: When the base Songbird installation is complete, exit the
# installer without launching Songbird.  This is required because this script
# must change the default update server URL preference before Songbird is first
# launched.
#
# 1. Set up a web server that is accessible via a local file path.
#
#   The files "Songbird.mar" and "appupdate.xml" will be copied to the root of
# the specified web server path.  This script tries to ensure that their
# permissions are set to read-all.  If this doesn't happen, the base Songbird
# will not be able to update.  In this case, manually set the permissions and
# then manually update the base Songbird.
#
# 2. Configure the following environment variables:
#
#   export HTTP_SERVER_PATH=<HTTP server path>
#   export HTTP_SERVER_URL=<HTTP server URL>
#   export SHA1SUM=<path to sha1sum>
#
#   E.g.,
#
#   HTTP_SERVER_URL=http://www.updates.com/test_update/
#   HTTP_SERVER_PATH=/z/Updates/
#   SHA1SUM=/c/dev/cygwin/bin/sha1sum.exe
#
#   SHA1SUM must only be defined if sha1sum or gsha1sum are not in the system
# path.  Also, if it is defined, it must be defined before the Songbird build is
# configured.
#
# 3. Delete profile and uninstall Songbird.
# 4. Checkout songbird and cd into trunk.
# 5. Execute the following:
#
#   ./update/test/test_windows_forward_update.sh
#
# 6. When the base Songbird installer opens, install the base Songbird into
#    "c:\Program Files\@INSTALL_DIR@" (it should be the default).
# 7. IMPORTANT: Remember not to launch Songbird from the installer.
# 8. Wait for this script to launch the base Songbird.
# 9. Verify that the application update is presented in the first-run wizard.
#    If not, check the permisssions of the update files on the HTTP server and
#    try manually updating.
# 10. Perform update.
# 11. Verify that update worked.
#

# Only build if not running the test.
if [ "$1" != "-test" ]; then

  # Build update.
  if [ "$1" = "-clobber" ]; then
    echo "ac_add_options --enable-installer" > songbird.config
    make -f songbird.mk clobber && \
      make -f songbird.mk && \
      make -C compiled/update complete && \
      make -C compiled/update/test
  else
    make -C compiled && \
      make -C compiled/update complete && \
      make -C compiled/update/test
  fi

  # Test update.
  chmod a+x compiled/update/test/test_windows_forward_update.sh
  ./compiled/update/test/test_windows_forward_update.sh -test
  exit 0

fi

cp compiled/_built_installer/Songbird_*_windows-i686-msvc8.complete.mar \
   ${HTTP_SERVER_PATH}/Songbird.mar
chmod a+r ${HTTP_SERVER_PATH}/Songbird.mar
export MAR_HASHVALUE=`sed -n 4p compiled/_built_installer/Songbird_*_windows-i686-msvc8.complete.snippet`
export MAR_SIZE=`sed -n 5p compiled/_built_installer/Songbird_*_windows-i686-msvc8.complete.snippet`

rm -f appupdate.xml
echo "<updates>" >> appupdate.xml
echo "<update type=\"major\"  version=\"@UPDATE_VERSION@\" extensionVersion=\"@UPDATE_VERSION@\">" >> appupdate.xml
echo "<patch type=\"complete\" URL=\"${HTTP_SERVER_URL}/Songbird.mar\" hashFunction=\"SHA1\" hashValue=\"${MAR_HASHVALUE}\" size=\"${MAR_SIZE}\" />" >> appupdate.xml
echo "</update>" >> appupdate.xml
echo "</updates>" >> appupdate.xml
cp appupdate.xml ${HTTP_SERVER_PATH}/appupdate.xml
chmod a+r ${HTTP_SERVER_PATH}/appupdate.xml

# DON'T START SONGBIRD FROM INSTALLER
./compiled/_built_installer/@INSTALLER@
echo "pref(\"app.update.url\", \"${HTTP_SERVER_URL}/appupdate.xml\");" > "c:\Program Files\@INSTALL_DIR@\defaults\preferences\aaa-test-update.js"
echo "pref(\"songbird.firstrun.check.app-update\", true);" >> "c:\Program Files\@INSTALL_DIR@\defaults\preferences\aaa-test-update.js"
"c:\Program Files\@INSTALL_DIR@\songbird.exe"

