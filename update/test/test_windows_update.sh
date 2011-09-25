#!/bin/sh
#
#=BEGIN NIGHTINGALE GPL
#
# This file is part of the Nightingale web player.
#
# Copyright(c) 2005-2010 POTI, Inc.
# http://www.getnightingale.com
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
#=END NIGHTINGALE GPL
#

#
# Nightingale Windows application update test script.
#
# Options:
#
#   -clobber                    Perform a clobber and full rebuild.
#
#   This script may be used to test updating of the Nightingale application on
# Windows.  It builds a Nightingale mar file, sets up the update files on the
# specified web server, downloads and installs a base Nightingale, patches it to
# use the specified web server for updates, and launches the base Nightingale.
#   A base version of Nightingale is downloaded and installed so that the update
# can be tested.  The built Nightingale installer is not used because it would
# perform all of the installation steps that the update would do, eliminating
# the utility of testing the update.
#   To test, follow the instructions below.  When the script completes, the
# base, non-updated Nightingale should be open and presenting an application update
# dialog for updating to the locally built Nightingale.
#   IMPORTANT: When the base Nightingale installation is complete, exit the
# installer without launching Nightingale.  This is required because this script
# must change the default update server URL preference before Nightingale is first
# launched.
#
# 1. Set up a web server that is accessible via a local file path.
#
#   The files "Nightingale.mar" and "appupdate.xml" will be copied to the root of
# the specified web server path.  This script tries to ensure that their
# permissions are set to read-all.  If this doesn't happen, the base Nightingale
# will not be able to update.  In this case, manually set the permissions and
# then manually update the base Nightingale.
#
# 2. Configure the following environment variables:
#
#   export HTTP_SERVER_PATH=<HTTP server path>
#   export HTTP_SERVER_URL=<HTTP server URL>
#   export SHA1SUM=<path to sha1sum>
#
#   SHA1SUM must only be defined if sha1sum or gsha1sum are not in the system
# path.  Also, if it is defined, it must be defined before the Nightingale build is
# configured.
#
# 3. Delete profile and uninstall Nightingale.
# 4. Checkout nightingale and cd into trunk.
# 5. Execute the following:
#
#   ./update/test/test_update.sh
#
# 6. When the base Nightingale installer opens, install the base Nightingale into
#    "c:\Program Files\Nightingale".
# 7. IMPORTANT: Remember not to launch Nightingale from the installer.
# 8. Wait for this script to launch the base Nightingale.
# 9. Verify that the application update dialog is presented.  If not, check
#    the permisssions of the update files on the HTTP server and try manually
#    updating.
# 10. Perform update.
# 11. Verify that update worked.
#

if [ "$1" = "-clobber" ]; then
  echo "ac_add_options --enable-installer" > nightingale.config
  make -f nightingale.mk clobber && \
    make -f nightingale.mk && \
    make -C compiled/update complete
else
  make -C compiled && make -C compiled/update complete
fi

cp compiled/_built_installer/Nightingale_*_windows-i686-msvc8.complete.mar \
   ${HTTP_SERVER_PATH}/Nightingale.mar
chmod a+r ${HTTP_SERVER_PATH}/Nightingale.mar
export MAR_HASHVALUE=`sed -n 4p compiled/_built_installer/Nightingale_*_windows-i686-msvc8.complete.snippet`
export MAR_SIZE=`sed -n 5p compiled/_built_installer/Nightingale_*_windows-i686-msvc8.complete.snippet`

rm -f appupdate.xml
echo "<updates>" >> appupdate.xml
echo "<update type=\"major\">" >> appupdate.xml
echo "<patch type=\"complete\" URL=\"${HTTP_SERVER_URL}/Nightingale.mar\" hashFunction=\"SHA1\" hashValue=\"${MAR_HASHVALUE}\" size=\"${MAR_SIZE}\" />" >> appupdate.xml
echo "</update>" >> appupdate.xml
echo "</updates>" >> appupdate.xml
cp appupdate.xml ${HTTP_SERVER_PATH}/appupdate.xml
chmod a+r ${HTTP_SERVER_PATH}/appupdate.xml

wget http://download.getnightingale.com/installer/windows/i686-msvc8/Nightingale_1.4.3-1438_windows-i686-msvc8.exe
# DON'T START NIGHTINGALE FROM INSTALLER
Nightingale_1.4.3-1438_windows-i686-msvc8.exe
echo "pref(\"app.update.url\", \"${HTTP_SERVER_URL}/appupdate.xml\");" > "c:\Program Files\Nightingale\defaults\preferences\aaa-test-update.js"
"c:\Program Files\Nightingale\nightingale.exe"

