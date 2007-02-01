/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

//
// Formal constructors to get interfaces - all creations in js should use these unless
//   getting the object as a service, then you should use the proper getService call.
// alpha-sorted!!
//

const sbIDatabaseQuery = new Components.Constructor("@songbirdnest.com/Songbird/DatabaseQuery;1", "sbIDatabaseQuery");
const sbIDynamicPlaylist = new Components.Constructor("@songbirdnest.com/Songbird/DynamicPlaylist;1", "sbIDynamicPlaylist");
const sbIMediaLibrary = new Components.Constructor("@songbirdnest.com/Songbird/MediaLibrary;1", "sbIMediaLibrary");
const sbIMediaScan = new Components.Constructor("@songbirdnest.com/Songbird/MediaScan;1", "sbIMediaScan");
const sbIMediaScanQuery = new Components.Constructor("@songbirdnest.com/Songbird/MediaScanQuery;1", "sbIMediaScanQuery");
const sbIPlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");
const sbIPlaylist = new Components.Constructor("@songbirdnest.com/Songbird/Playlist;1", "sbIPlaylist");
const sbIPlaylistReaderListener = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistReaderListener;1", "sbIPlaylistReaderListener");
const sbIPlaylistReaderManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistReaderManager;1", "sbIPlaylistReaderManager");
const sbIPlaylistsource = new Components.Constructor("@mozilla.org/rdf/datasource;1?name=playlist", "sbIPlaylistsource");
const sbISimplePlaylist = new Components.Constructor("@songbirdnest.com/Songbird/SimplePlaylist;1", "sbISimplePlaylist");
const sbISmartPlaylist = new Components.Constructor("@songbirdnest.com/Songbird/SmartPlaylist;1", "sbISmartPlaylist");
const sbIServicesource = new Components.Constructor("@mozilla.org/rdf/datasource;1?name=Servicesource", "sbIServicesource");

// XXXredfive - this goes in the sbWindowUtils.js file when I get around to making it.
var gPrefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
var gConsole = Components.classes["@mozilla.org/consoleservice;1"]
               .getService(Components.interfaces.nsIConsoleService);
// log to JS console AND to the command line (in case of crashes)
function SB_LOG (scopeStr, msg) {
  msg = msg ? msg : ""; 
  //This works, but adds everything as an Error
  //Components.utils.reportError( scopeStr + " : " + msg);
  gConsole.logStringMessage( scopeStr + " : " + msg );
  dump( scopeStr + " : " + msg + "\n");
}
const PREFS_SERVICE_CONTRACTID = "@mozilla.org/preferences-service;1";
const nsIPrefBranch2 = Components.interfaces.nsIPrefBranch2;
/**
 * Adapted from nsUpdateService.js.in. Need to replace with dataremotes.
 */
function getPref(aFunc, aPreference, aDefaultValue) {
  var prefs = 
    Components.classes[PREFS_SERVICE_CONTRACTID].getService(nsIPrefBranch2);
  try {
    return prefs[aFunc](aPreference);
  }
  catch (e) { }
  return aDefaultValue;
}
function setPref(aFunc, aPreference, aValue) {
  var prefs = 
    Components.classes[PREFS_SERVICE_CONTRACTID].getService(nsIPrefBranch2);
  return prefs[aFunc](aPreference, aValue);
}



// Useful constants
var CORE_WINDOWTYPE = "Songbird:Core";
var PREF_BONES_SELECTED = "general.bones.selectedMainWinURL";
var PREF_FEATHERS_SELECTED = "general.skins.selectedSkin";
var BONES_DEFAULT_URL = "chrome://rubberducky/content/xul/mainwin.xul";
var FEATHERS_DEFAULT_NAME = "rubberducky";

// Lots of things assume the playlist playback service is a global
var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                      .getService(Components.interfaces.sbIPlaylistPlayback);
