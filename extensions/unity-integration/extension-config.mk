#
# BEGIN NIGHTINGALE GPL
#
# This file is part of the Nightingale media player.
#
# Copyright(c) 2013
# http://www.getnightingale.com
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
# END NIGHTINGALE GPL
#

EXTENSION_NAME = unity-integration
EXTENSION_UUID = $(EXTENSION_NAME)@lookingman.com

EXTENSION_VER = 1.1.12.1
EXTENSION_MIN_VER = $(SB_JSONLY_EXTENSION_MIN_VER)
EXTENSION_MAX_VER = $(SB_JSONLY_EXTENSION_MAX_VER)

# Overriding EXTENSION_ARCH does not work anymore:
# See r14481 note in:
# http://developer.songbirdnest.com/builds/trunk/1214/changes.txt

# Override to create dynamic libary in /components instead of platform
#EXTENSION_ARCH = $(NULL)

# This will force it to create UnityProxy.so in /components
EXTENSION_NO_BINARY_COMPONENTS = 1

# This is needed to build since targetPlatform can't be set install.rdf
EXTENSION_SUPPORTED_PLATFORMS = linux
