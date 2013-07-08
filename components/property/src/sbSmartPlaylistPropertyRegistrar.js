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

  // it'd be nice to have currying as a generic operation
  function register(id, width, direction)
  {
    this.registerPropertyToContext(context, SBProperties[id], width, direction);
  }
  
  var props = [
     ["albumName",       220, "a"],
     ["albumArtistName", 220, "a"],
     ["artistName",      220, "a"],
     ["bitRate",          40, "d"],
     ["bpm",              40, "d"],
     ["comment",         120, "a"],
     ["composerName",    150, "a"],
     ["created",         122, "d"],
     ["updated",         122, "d"],
     ["discNumber",       40, "d"],
     ["contentURL",      220, "a"],
     ["genre",            70, "a"],
     ["lastPlayTime",    122, "d"],
     ["lastSkipTime",    122, "d"],
     ["playCount",        40, "d"],
     ["rating",           85, "d"],
     ["sampleRate",       75, "d"],
     ["contentLength",    50, "d"],
     ["skipCount",        40, "d"],
     ["originURL",       220, "a"],
     ["originPage",      220, "a"],
     ["originPageTitle", 150, "a"],
     ["duration",         55, "d"],
     ["trackName",       220, "a"],
     ["trackNumber",      40, "d"],
     ["year",             50, "d"],
     ["keywords",        220, "a"],
     ["description",     220, "a"],
     ["showName",        220, "a"],
     ["episodeNumber",    40, "a"],
     ["seasonNumber",     40, "a"],
     ["contentType",      40, "a"],
  ];

  for each (p in props)
  {
    register.apply(this, p);
  }
  
  this.registerPropertyToContext(context,
                                 "http://songbirdnest.com/dummy/smartmedialists/1.0#playlist",
                                 0,
                                 "");
}

SmartPlaylistPropertyRegistrar.prototype = {
  constructor     : SmartPlaylistPropertyRegistrar,
  classDescription: "Songbird SmartPlaylist Property Registrar Interface",
  classID         : Components.ID("{4b6a6c23-e247-419e-b629-3c2ef5f5876d}"),
  contractID      : "@songbirdnest.com/Songbird/SmartPlaylistPropertyRegistrar;1",
 
//  _xpcom_categories: [{
//	    category: "app-startup",
//	    service: true
//  }],

  /**
   * Returns an enumerator to all properties for a particular smart
   * playlist context
   */
  getPropertiesForContext: function 
    SmartPlaylistPropertyRegistrar_getPropertyForContext(aContextID) {
    return ArrayConverter.enumerator(this._maps[aContextID]);
  },

  /**
   * Register a property to a smart playlist context
   */  
  registerPropertyToContext: function
    SmartPlaylistPropertyRegistrar_registerPropertyToContext(aContextID, 
                                                             aPropertyID,
                                                             aColWidth,
                                                             aSortDir) {
    if (!(aContextID in this._maps)) {
      this._maps[aContextID] = [];
    }
    var smartPlaylistProperty = {
      propertyID           : aPropertyID,
      defaultColumnWidth   : aColWidth,
      defaultSortDirection : aSortDir,
      QueryInterface       : XPCOMUtils.generateQI([Ci.sbISmartPlaylistProperty])
    };
    this._maps[aContextID].push(smartPlaylistProperty);
  },

  /**
   * See nsISupports.idl
   */
  QueryInterface:
    XPCOMUtils.generateQI([Ci.sbISmartPlaylistPropertyRegistrar])
}; // SmartPlaylistPropertyRegistrar.prototype

//function postRegister(aCompMgr, aFileSpec, aLocation) {
//  XPCOMUtils.categoryManager
//            .addCategoryEntry('app-startup',
//                              'smartplaylist-property-registrar', 
//                              'service,@songbirdnest.com/Songbird/SmartPlaylistPropertyRegistrar;1',
//                              true, 
//                              true);
//}

var NSGetModule = 
  XPCOMUtils.generateNSGetFactory([SmartPlaylistPropertyRegistrar]);

