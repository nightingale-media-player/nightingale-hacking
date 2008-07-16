/**
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
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ce = Components.Exception;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");

function SmartPlaylistPropertyRegistrar() {
  this._maps = {};

  var context = "default";
  
  for each (var prop in 
    ["albumName", "albumArtistName", "artistName", "bitRate", "bpm",
     "comment", "composerName", "created", "updated", "discNumber",
     "contentURL", "genre", "lastPlayTime", "lastSkipTime", "playCount",
     "rating", "sampleRate", "contentLength", "skipCount", "originURL",
     "originPage", "originPageTitle", "duration", "trackName", "trackNumber",
     "year"]) {
    this.registerPropertyToContext(context, SBProperties[prop]);
  }
}

SmartPlaylistPropertyRegistrar.prototype = {
  constructor     : SmartPlaylistPropertyRegistrar,
  classDescription: "Songbird SmartPlaylist Property Registrar Interface",
  classID         : Components.ID("{4b6a6c23-e247-419e-b629-3c2ef5f5876d}"),
  contractID      : "@songbirdnest.com/Songbird/SmartPlaylistPropertyRegistrar;1",

  /**
   * Returns an enumerator to all properties for a particular smart
   * playlist context
   */
  getPropertiesForContext: function 
    SmartPlaylistPropertyRegistrar_getPropertyForContext(aContextID) {
    return ArrayConverter.stringEnumerator(this._maps[aContextID]);
  },

  /**
   * Register a property to a smart playlist context
   */  
  registerPropertyToContext: function
    SmartPlaylistPropertyRegistrar_registerPropertyToContext(aContextID, 
                                                             aProperty) {
    if (!(aContextID in this._maps)) {
      this._maps[aContextID] = [];
    }
    this._maps[aContextID].push(aProperty);
  },

  /**
   * See nsISupports.idl
   */
  QueryInterface:
    XPCOMUtils.generateQI([Ci.sbISmartPlaylistPropertyRegistrar])
}; // SmartPlaylistPropertyRegistrar.prototype

function postRegister(aCompMgr, aFileSpec, aLocation) {
  XPCOMUtils.categoryManager
            .addCategoryEntry('app-startup',
                              'smartplaylist-property-registrar', 
                              'service,@songbirdnest.com/Songbird/SmartPlaylistPropertyRegistrar;1',
                              true, 
                              true);
}

var NSGetModule = 
  XPCOMUtils.generateNSGetModule([SmartPlaylistPropertyRegistrar], 
                                 postRegister);

