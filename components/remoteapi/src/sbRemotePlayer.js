/**
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

/**
 * \file sbRemotePlayer.js
 * \brief Remote API object
 */

var Cr = Components.results;
var Ci = Components.interfaces;
var Cc = Components.classes;
var gConsole = Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService);

// match constant in nsIScriptNameSpaceManager.idl
var JAVASCRIPT_GLOBAL_PROPERTY_CATEGORY="JavaScript global property";

const REMOTEPLAYER_CONTRACTID = "@songbirdnest.com/Songbird/RemotePlayer;1";
const REMOTEPLAYER_CLASSNAME = "Songbird Remote Mediaplayer";
const REMOTEPLAYER_CID = Components.ID("{f041ec15-aa9f-45d1-ba2f-787d6c17f62c}");
const REMOTEPLAYER_IID = Ci.sbIRemotePlayer;
const ARRAY_CONTRACTID = "@mozilla.org/array;1";

function RemotePlayer() {
}

// Define the prototype block before adding to the prototype object or the
//   additions will get blown away (at least the setters/getters were).
RemotePlayer.prototype = {
  name: "Songbird",
  _initialized: false,
  _playlists: null,
  _webPlaylist: null,
  _currentArtist: null,
  _currentAlbum: null,
  _currentTrack: null,

  // list of allowed state keys to which pages can bind:
  _publicKeys: [ "metadata.artist",
                 "metadata.title",
                 "metadata.album",
                 "metadata.genre",
                 "metadata.url",
                 "metadata.position",
                 "metadata.length",
                 "metadata.position.str",
                 "metadata.length.str",
                 "playlist.shuffle",
                 "playlist.repeat",
                 "faceplate.playing",
                 "faceplate.paused"
               ],

  // sbISecurityAggregator passes these to the mixin
  _publicWProps: [ "" ],
  _publicRProps: [ "name",
                   "playlists" ,
                   "webplaylist",
                   "currentArtist",
                   "currentAlbum",
                   "currentTrack",
                   "classDescription",
                   "contractID",
                   "classID",
                   "implementationLanguage",
                   "flags"],
  _publicMethods: [ "play",
                    "stop",
                    "pause",
                    "previous",
                    "next",
                    "playURL",
                    "unbind",
                    "bindElementAttribute",
                    "bindElementProperty",
                    "bindObserver" ],
  _publicInterfaces: [ Ci.nsISupports,
                       Ci.nsIClassInfo,
                       Ci.nsISecurityCheckedComponent,
                       REMOTEPLAYER_IID ],
  _securityMixin: null, 
  _gPPS: null,
  _DataRemoteCtor: null,

  // Set up some things we'll want on hand a lot. Sure would be nice to be
  //   able to set up the component stuff to call this without it interfering
  //   with registration.
  init: function() {
    this._gPPS = Cc["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                               .getService(Ci.sbIPlaylistPlayback);
    this._DataRemoteCtor = new Components.Constructor
                               ("@songbirdnest.com/Songbird/DataRemote;1",
                                "sbIDataRemote",
                                "init");
    this._currentArtist = this._DataRemoteCtor("metadata.artist", null);
    this._currentAlbum = this._DataRemoteCtor("metadata.album", null);
    this._currentTrack = this._DataRemoteCtor("metadata.title", null);
    this._initialized = true;

    // XXXredfive - need to investigate the playlist api before exposing these
    //this._playlists = Cc[ARRAY_CONTRACTID].createInstance(Ci.nsIArray);
    //this._webplaylist = Cc[webplaylist_contractid].createInstance(Ci.sbIMedialist);

    this._securityMixin = Cc["@songbirdnest.com/songbird/securitymixin;1"]
                         .createInstance(Ci.nsISecurityCheckedComponent);

    // initialize the security mixin with the cleared methods and props
    this._securityMixin
             .init(this, this._publicInterfaces, this._publicInterfaces.length,
                         this._publicMethods, this._publicMethods.length,
                         this._publicRProps, this._publicRProps.length,
                         this._publicWProps, this._publicWProps.length);
  },

  // helper function to check for allowed dataremote key
  checkKey: function(aKey)  {
    for ( var stuff in this._publicKeys )
      if ( this._publicKeys[stuff] == aKey )
        return true;
    return false;
  },

  // sbIRemotePlayer
  get currentArtist() {
    if (! this._initialized)
      this.init();
    return this._currentArtist.stringValue;
  },
  
  get currentAlbum() {
    if (! this._initialized)
      this.init();
    return this._currentAlbum.stringValue;
  },

  get currentTrack() {
    if (! this._initialized)
      this.init();
    return this._currentTrack.stringValue;
  },

  // XXXredfive - need to investigate the playlist api before exposing these
  get playlists() {
    if (! this._initialized)
      this.init();
    //return this._playlists;
    return null;
  },

  // XXXredfive - need to investigate the playlist api before exposing these
  get webplaylist() {
    if (! this._initialized)
      this.init();
    //return this._webPlaylist;
    return null;
  },

  // XXXredfive - pulled straight from sbDataRemoteUtils.js, can this
  //              just be used from there? subscript loader?
  //              Fix the API!!!
  bindElementProperty: function  ( aKey, aElement, aProperty, aIsBool, aIsNot, aEvalString ) {
    var retval = null;
    if (! this._initialized)
      this.init();
    if ( ! this.checkKey(aKey) )
      return null;
    try {
      var obj = ( typeof(aElement) == "object" ) ?  aElement : document.getElementById( aElement );
      if ( obj ) {
        retval = this._DataRemoteCtor( aKey, null );
        retval.bindProperty( obj, aProperty, aIsBool, aIsNot, aEvalString );
      }
      else {
        Components.utils.reportError( "bindElementProperty: Can't find " + aElement );
      }
    }
    catch ( err ) {
      Components.utils.reportError( err );
    }
    return retval;
  },

  // XXXredfive - pulled straight from sbDataRemoteUtils.js, can this
  //              just be used from there? subscript loader?
  //              Fix the API!!!
  bindElementAttribute: function ( aKey, aElement, aAttribute, aIsBool, aIsNot, aEvalString ) {
    if (! this._initialized)
      this.init();
    var retval = null;
    if ( ! this.checkKey(aKey) )
      return null;
    try {
      var obj = ( typeof(aElement) == "object" ) ?  aElement : document.getElementById( aElement );
      if ( obj ) {
        retval = this._DataRemoteCtor( aKey, null );
        retval.bindAttribute( obj, aAttribute, aIsBool, aIsNot, aEvalString );
      }
      else {
        Components.utils.reportError( "bindElementAttribute: Can't find " + aElement );
      }
    }
    catch ( err ) {
      Components.utils.reportError( "ERROR! Binding attribute did not succeed.\n" + err + "\n" );
    }
    return retval;
  },

  // XXXredfive - pulled straight from sbDataRemoteUtils.js, can this
  //              just be used from there? subscript loader?
  //              Fix the API!!!
  bindObserver: function ( aKey, aObserver, aSupressFirst ) {
    var retval = null;
    if (! this._initialized)
      this.init();
    if ( ! this.checkKey(aKey) ) {
      return null;
    }
    try {
      retval = this._DataRemoteCtor( aKey, null );
      retval.bindObserver( aObserver, aSupressFirst );
    }
    catch ( err ) {
      Components.utils.reportError( "ERROR! Binding attribute did not succeed.\n" + err + "\n" );
    }
    return retval;
  },

  // XXXredfive - this should be automaticly linked to the webpage unload
  //              so that we remove the listeners without being told to. Right
  //              now if the webpage doesn't unbind it is likely that the js
  //              will be in a closure and continue to run - BAD!
  unbind: function( aDataremote ) {
    if ( aDataremote != null )
      aDataremote.unbind();
  },

  play: function() {
    if (! this._initialized)
      this.init();
    return this._gPPS.play();
  },

  playURL: function(aURL) {
    if (! this._initialized)
      this.init();
    return this._gPPS.playURL(aURL);
  },

  stop: function() {
    if (! this._initialized)
      this.init();
    return this._gPPS.stop();
  },

  pause: function() {
    if (! this._initialized)
      this.init();
    return this._gPPS.pause();
  },

  next: function() {
    if (! this._initialized)
      this.init();
    return this._gPPS.next();
  },

  previous: function() {
    if (! this._initialized)
      this.init();
    return this._gPPS.previous();
  },

  // nsIClassInfo
  getInterfaces: function( count ) {
    var ifaces = [ REMOTEPLAYER_IID,
                   Ci.sbISecurityAggregator,
                   Ci.nsIClassInfo,
                   Ci.nsISecurityCheckedComponent ];
     count.value = ifaces.length;
     return ifaces;
  },
 
  get classDescription() {
      return REMOTEPLAYER_CLASSNAME;
  },

  get contractID() {
      return REMOTEPLAYER_CONTRACTID;
  },

  get classID() {
      return REMOTEPLAYER_CID;
  },

  getHelperForLanguage: function( language ) { return null; },

  implementationLanguage: Ci.nsIProgrammingLanguage.JAVASCRIPT,

  // Dont use DOM_OBJECT, it shortcircuits all the checks we want to happen
  flags: 0,

  // nsISecurityCheckedComponent -- implemented by the security mixin
  canCreateWrapper: function(iid) {
    if (! this._initialized)
      this.init();
    return this._securityMixin.canCreateWrapper(iid);
  },
  canCallMethod: function(iid, methodName) {
    if (! this._initialized)
      this.init();
    return this._securityMixin.canCallMethod(iid, methodName);
  },
  canGetProperty: function(iid, propertyName) {
    if (! this._initialized)
      this.init();
    return this._securityMixin.canGetProperty(iid, propertyName);
  },
  canSetProperty: function(iid, propertyName) {
    if (! this._initialized)
      this.init();
    return this._securityMixin.canSetProperty(iid, propertyName);
  },

  // nsISupports
  QueryInterface: function(iid) {
    if (!iid.equals(REMOTEPLAYER_IID) &&
             !iid.equals(Ci.nsIClassInfo) && 
             !iid.equals(Ci.sbISecurityAggregator) && 
             !iid.equals(Ci.nsISecurityCheckedComponent) && 
             !iid.equals(Ci.nsISupports)) {
      throw Cr.NS_ERROR_NO_INTERFACE;
    }

    return this;
  }
}; // RemotePlayer.prototype

// be specific about the constructor - may not be strictly necessary
RemotePlayer.prototype.constructor = RemotePlayer;

// ----------------------------------------------------------------------------
//
//                          XPCOM Module Registration
//
// ----------------------------------------------------------------------------

const gRemotePlayerModule = {
  registerSelf: function(compMgr, fileSpec, location, type) {
    compMgr = compMgr.QueryInterface(Ci.nsIComponentRegistrar);
    compMgr.registerFactoryLocation(REMOTEPLAYER_CID,
                                    REMOTEPLAYER_CLASSNAME,
                                    REMOTEPLAYER_CONTRACTID,
                                    fileSpec,
                                    location,
                                    type);

    var catman = Cc["@mozilla.org/categorymanager;1"].getService(Ci.nsICategoryManager);
    catman.addCategoryEntry(JAVASCRIPT_GLOBAL_PROPERTY_CATEGORY,
                            "songbird", /* use this name as the constructor */
                            REMOTEPLAYER_CONTRACTID,
                            true,  /* Persist this entry */
                            true); /* Replace existing entry */
  },

  unregisterSelf : function (compMgr, location, type) {
    compMgr.QueryInterface(Ci.nsIComponentRegistrar);
    compMgr.unregisterFactoryLocation(REMOTEPLAYER_CID, location);

    var catMan = Cc["@mozilla.org/categorymanager;1"].getService(Ci.nsICategoryManager);
    catman.deleteCategoryEntry(JAVASCRIPT_GLOBAL_PROPERTY_CATEGORY,
                               "songbird");
  },

  getClassObject : function (compMgr, cid, iid) {
    if (!cid.equals(REMOTEPLAYER_CID))
      throw Cr.NS_ERROR_NO_INTERFACE;

    if (!iid.equals(Ci.nsIFactory))
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;

    return this.mFactory;
  },

  mFactory : {
    createInstance : function (outer, iid) {
      if (outer != null)
        throw Cr.NS_ERROR_NO_AGGREGATION;
      return (new RemotePlayer()).QueryInterface(iid);
    }
  },

  canUnload: function(compMgr) { 
    return true; 
  },

  QueryInterface : function (iid) {
    if ( !iid.equals(Ci.nsIModule) ||
         !iid.equals(Ci.nsISupports) )
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }

}; // gRemotePlayerModule

function NSGetModule(comMgr, fileSpec) {
  return gRemotePlayerModule;
} // NSGetModule

