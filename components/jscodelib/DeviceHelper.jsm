/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

EXPORTED_SYMBOLS = [ "SYNCSETTINGS_CHANGE",
                     "SYNCSETTINGS_APPLY", "SYNCSETTINGS_CANCEL",
                     "SYNCSETTINGS_SAVING", "SYNCSETTINGS_SAVED",
                     "SYNCSETTINGS_IMAGETAB" ];

// Possible values for detail parameter in sbDeviceSync-settings event
var SYNCSETTINGS_CHANGE = 0;
var SYNCSETTINGS_APPLY = 1;
var SYNCSETTINGS_CANCEL = 2;
var SYNCSETTINGS_SAVING = 3;
var SYNCSETTINGS_SAVED = 4;
var SYNCSETTINGS_IMAGETAB = 5;
