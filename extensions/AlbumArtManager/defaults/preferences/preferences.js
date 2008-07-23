/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// END SONGBIRD GPL
//
*/

/*******************************************************************************
 *******************************************************************************
 *
 * Default preferences file for the Album Art Manager.
 *
 *******************************************************************************
 ******************************************************************************/

pref("songbird.albumartmanager.fetcher.metadata.priority", 1);
pref("songbird.albumartmanager.fetcher.file.priority", 2);
pref("songbird.albumartmanager.fetcher.amazon.priority", 3);

pref("songbird.albumartmanager.fetcher.metadata.isEnabled", true);
pref("songbird.albumartmanager.fetcher.file.isEnabled", true);
pref("songbird.albumartmanager.fetcher.amazon.isEnabled", true);

pref("songbird.albumartmanager.display_track_notification", true);
pref("songbird.albumartmanager.cover.usealbumlocation", true);
pref("songbird.albumartmanager.search.includeArtist", true);
pref("songbird.albumartmanager.cover.format", "%artist% - %album%");
pref("songbird.albumartmanager.country", "com");
pref("songbird.albumartmanager.scan", true);

pref("songbird.albumartmanager.debug", false);
