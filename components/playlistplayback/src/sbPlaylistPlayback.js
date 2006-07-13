/**
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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
 * ----------------------------------------------------------------------------
 * Constants
 * ----------------------------------------------------------------------------
 */

// XPCOM Registration
const SONGBIRD_PLAYLISTPLAYBACK_CONTRACTID = "@songbirdnest.com/Songbird/PlaylistPlayback;1";
const SONGBIRD_PLAYLISTPLAYBACK_CLASSNAME = "Songbird Playlist Playback Interface";
const SONGBIRD_PLAYLISTPLAYBACK_CID = Components.ID("{000e2465-58b7-4922-bdfb-9ab1492c6037}");
const SONGBIRD_PLAYLISTPLAYBACK_IID = Components.interfaces.sbIPlaylistPlayback;

// Songbird ContractID Stuff
const SONGBIRD_COREWRAPPER_CONTRACTID = "@songbirdnest.com/Songbird/CoreWrapper;1";
const SONGBIRD_COREWRAPPER_IID = Components.interfaces.sbICoreWrapper;

const SONGBIRD_DATAREMOTE_CONTRACTID = "@songbirdnest.com/Songbird/DataRemote;1";
const SONGBIRD_DATAREMOTE_IID = Components.interfaces.sbIDataRemote;

const SONGBIRD_DATABASEQUERY_CONTRACTID = "@songbirdnest.com/Songbird/DatabaseQuery;1";
const SONGBIRD_DATABASEQUERY_IID = Components.interfaces.sbIDatabaseQuery;

const SONGBIRD_MEDIALIBRARY_CONTRACTID = "@songbirdnest.com/Songbird/MediaLibrary;1";
const SONGBIRD_MEDIALIBRARY_IID = Components.interfaces.sbIMediaLibrary;

// String Bundles
const URI_SONGBIRD_PROPERTIES    = "chrome://songbird/locale/songbird.properties";

// Database GUIDs
const DB_TEST_GUID = "testdb-0000";

// Other XPCOM Stuff
const sbIDatabaseQuery = new Components.Constructor("@songbirdnest.com/Songbird/DatabaseQuery;1", "sbIDatabaseQuery");
/*
const sbIPlaylistsource = new Components.Constructor("@mozilla.org/rdf/datasource;1?name=playlist", "sbIPlaylistsource");
const sbIMediaLibrary = new Components.Constructor("@songbirdnest.com/Songbird/MediaLibrary;1", "sbIMediaLibrary");
const sbIPlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");
const sbIPlaylist = new Components.Constructor("@songbirdnest.com/Songbird/Playlist;1", "sbIPlaylist");
const sbISimplePlaylist = new Components.Constructor("@songbirdnest.com/Songbird/SimplePlaylist;1", "sbISimplePlaylist");
const sbIDynamicPlaylist = new Components.Constructor("@songbirdnest.com/Songbird/DynamicPlaylist;1", "sbIDynamicPlaylist");
const sbISmartPlaylist = new Components.Constructor("@songbirdnest.com/Songbird/SmartPlaylist;1", "sbISmartPlaylist");
const sbIPlaylistReaderManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistReaderManager;1", "sbIPlaylistReaderManager");
const sbIPlaylistReaderListener = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistReaderListener;1", "sbIPlaylistReaderListener");
const sbIMediaScan = new Components.Constructor("@songbirdnest.com/Songbird/MediaScan;1", "sbIMediaScan");
const sbIMediaScanQuery = new Components.Constructor("@songbirdnest.com/Songbird/MediaScanQuery;1", "sbIMediaScanQuery");
*/

// Regular const's
const PLAYBUTTON_STATE_PLAY  = 0;
const PLAYBUTTON_STATE_PAUSE = 1;

const REPEAT_MODE_OFF = 1;
const REPEAT_MODE_ALL = 2;
const REPEAT_MODE_ONE = 3;

/**
 * ----------------------------------------------------------------------------
 * Global variables
 * ----------------------------------------------------------------------------
 */

var gConsole    = null;
var gOS         = null;

/**
 * ----------------------------------------------------------------------------
 * Global Utility Functions
 * ----------------------------------------------------------------------------
 */

/**
 * Logs a string to the error console. 
 * @param   string
 *          The string to write to the error console..
 */  
function LOG(string) {
    dump("***sbPlaylistPlayback*** " + string + "\n");
    if (gConsole)
      gConsole.logStringMessage(string);
} // LOG

function ConvertUrlToDisplayName(url)
{
  url = decodeURI( url );
  
  var ret = "";
  
  if (url.lastIndexOf('/') != -1)
    ret = url.substring(url.lastIndexOf('/') + 1, url.length);
  else if (url.lastIndexOf('\\') != -1)
    ret = url.substring(url.lastIndexOf('\\') + 1, url.length);
  else
    ret = url;
    
  var percent = ret.indexOf('%');
  if (percent != -1) {
    var remainder = ret;
    ret = "";
    while (percent != -1) {
      ret += remainder.substring(0, percent);
      remainder = remainder.substring(percent + 3, url.length);
      percent = remainder.indexOf('%');
      ret += " ";
      if (percent == -1)
        ret += remainder;
    }
  }
  if (ret.length == 0)
    ret = url;

  return ret;
}

// Just a useful function to parse down some seconds.
function EmitSecondsToTimeString( seconds )
{
  if ( seconds < 0 )
    return "00:00";
  seconds = parseFloat( seconds );
  var minutes = parseInt( seconds / 60 );
  seconds = parseInt( seconds ) % 60;
  var hours = parseInt( minutes / 60 );
  if ( hours > 50 ) // lame
    return "Error";
  minutes = parseInt( minutes ) % 60;
  var text = ""
  if ( hours > 0 )
    text += hours + ":";
  if ( minutes < 10 )
    text += "0";
  text += minutes + ":";
  if ( seconds < 10 )
    text += "0";
  text += seconds;
  return text;
}

/**
 * Dumps an object's properties to the console
 * @param   obj
 *          The object to dump
 * @param   objName
 *          A string containing the name of obj
 */  
function listProperties(obj, objName) {
  var columns = 3;
  var count = 0;
  var result = "";
  for (var i in obj) {
      result += objName + "." + i + " = " + obj[i] + "\t\t\t";
      count = ++count % columns;
      if ( count == columns - 1 )
        result += "\n";
  }
  LOG("listProperties");
  dump(result + "\n");
}

/**
 * ----------------------------------------------------------------------------
 * The PlaylistPlayback Component
 * ----------------------------------------------------------------------------
 */

/**
 * Constructor for the PlaylistPlayback object used to set up global variables.
 * All service initialization is handled in _init() after prefs are available.
 */
function PlaylistPlayback() {  
  gConsole = Components.classes["@mozilla.org/consoleservice;1"]
                       .getService(Components.interfaces.nsIConsoleService);
  gOS      = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
  
  if (gOS.addObserver) {
    // We should wait until the profile has been loaded to start
    gOS.addObserver(this, "profile-after-change", false);
    // We need to unhook things on shutdown
    gOS.addObserver(this, "xpcom-shutdown", false);
  }

  // Playlistsource provides the interface for requesting playlist info.  
  this._source = Components.classes[ "@mozilla.org/rdf/datasource;1?name=playlist" ].getService( Components.interfaces.sbIPlaylistsource );
  this._timer = Components.classes[ "@mozilla.org/timer;1" ].createInstance( Components.interfaces.nsITimer );

  var jsLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"].getService(Components.interfaces.mozIJSSubScriptLoader);
  jsLoader.loadSubScript("chrome://songbird/content/scripts/metrics.js", this);

} // PlaylistPlayback
PlaylistPlayback.prototype.constructor = PlaylistPlayback;

/**
 * Prototype
 */
PlaylistPlayback.prototype = {
  /**
   * ---------------------------------------------
   * Private Variables
   * ---------------------------------------------
   */

  /**
   * The player loop timer
   */
  _timer: null,
  
  /**
   * The Playlistsource
   */
  _source: null,
  
  /**
   * Used to not hit the playback core for metadata too often.
   */
  _metadataPollCount: 0,
  
  /**
   *
   */
  _repeatMode: REPEAT_MODE_OFF,

  /**
   *
   */
  _shuffle: false,

  /**
   * An array of cores to use
   */
  _cores: [],

  /**
   * Keeps track of the currently selected core
   */
  _currentCoreIndex: -1,

  /**
   * This dataremote we use as our "current index value"
   */
  _playlistIndex:      null,

  /**
   * This dataremote we use as our "current playing playlist ref"
   */
  _playingRef:      null,

  /**
   * All the data remotes we probably mess with
   */
  _playButton:         null,
  _playUrl:            null,
  _seenPlaying:        null,
  _started:            0,
  _lastVolume:         null,
  _volume:             null,
  _muteData:           null,
  _playlistRef:        null,
  _repeat:             null,
  _shuffle:            null,
  _showRemaining:      null,
  _metadataTitle:      null,
  _metadataArtist:     null,
  _metadataAlbum:      null,
  _metadataUrl:        null,
  _metadataPos:        null,
  _metadataLen:        null,
  _metadataPosText:    null,
  _metadataLenText:    null,
  _statusText:         null,
  _statusStyle:        null,
    
  _faceplateState:       null,
  _restartOnPlaybackEnd: null,
  _resetSearchData:      null,
  
  _requestedVolume:      -1,
  _calculatedVolume:     -1,

  _set_metadata:      false,
  
  /**
   * ---------------------------------------------
   * Private Methods
   * ---------------------------------------------
   */

  /**
   * Initializes the PlaylistPlayback object
   */
  _init: function() {
    try {
      LOG("Songbird PlaylistPlayback Service loaded successfully");
      
      // Attach all the sbDataRemote objects (via XPCOM!)
      LOG("Attaching DataRemote objects");
      this._attachDataRemotes();
    } catch( err ) {
      LOG( "!!! ERROR: sbPlaylistPlayback _init\n\n" + err + "\n" );
    }
  },
  
  _deinit: function() {
    this._releaseDataRemotes();
  },

  _attachDataRemotes: function() {
    // This will create the component and call init with the args
    var createDataRemote = new Components.Constructor( SONGBIRD_DATAREMOTE_CONTRACTID, SONGBIRD_DATAREMOTE_IID, "init");

    // HOLY SMOKES we use lots of data elements.
  
    // Play/Pause image toggle
    this._playButton            = createDataRemote("faceplate.play", null);
    //string current           
    this._playUrl               = createDataRemote("faceplate.play.url", null);
    //actually playing         
    this._seenPlaying           = createDataRemote("faceplate.seenplaying", null);
    this._volume                = createDataRemote("faceplate.volume", null);
    //t/f                      
    this._muteData              = createDataRemote("faceplate.mute", null);
    this._playingRef            = createDataRemote("playing.ref", null);
    this._playlistRef           = createDataRemote("playlist.ref", null);
    this._playlistIndex         = createDataRemote("playlist.index", null);
    this._repeat                = createDataRemote("playlist.repeat", null);
    this._shuffle               = createDataRemote("playlist.shuffle", null);
    this._showRemaining         = createDataRemote("faceplate.showremainingtime", null);
                               
    this._metadataTitle         = createDataRemote("metadata.title", null);
    this._metadataArtist        = createDataRemote("metadata.artist", null);
    this._metadataAlbum         = createDataRemote("metadata.album", null);
    this._metadataUrl           = createDataRemote("metadata.url", null);
    this._metadataPos           = createDataRemote("metadata.position", null);
    this._metadataLen           = createDataRemote("metadata.length", null);
    this._resetSearchData       = createDataRemote("faceplate.search.reset", null);
    this._metadataPosText       = createDataRemote("metadata.position.str", null);
    this._metadataLenText       = createDataRemote("metadata.length.str", null);
    this._faceplateState        = createDataRemote("faceplate.state", null);
    this._restartOnPlaybackEnd  = createDataRemote("restart.onplaybackend", null);

    this._statusText            = createDataRemote("faceplate.status.text", null );
    this._statusStyle           = createDataRemote("faceplate.status.style", null );

// Set startup defaults
    this._metadataPos.intValue = 0;
    this._metadataLen.intValue = 0;
    this._seenPlaying.boolValue = false;
    this._metadataPosText.stringValue = "0:00:00";
    this._metadataLenText.stringValue = "0:00:00";
    this._showRemaining.boolValue = false;
    //this._muteData.boolValue = false;
    this._playlistRef.stringValue = "";
    this._playlistIndex.intValue = -1;
    this._faceplateState.boolValue = false;
    this._restartOnPlaybackEnd.boolValue = false;
    this._metadataUrl.stringValue = "";
    this._metadataTitle.stringValue = "";
    this._metadataArtist.stringValue = "";
    this._metadataAlbum.stringValue = "";
    this._statusText.stringValue = "";
    this._statusStyle.stringValue = "";
    this._playingRef.stringValue = "";
    this._playUrl.stringValue = ""; 
    this._playButton.boolValue = true; // Start on.

    // if they have not been set they will be the empty string.
    if (this._repeat.stringValue == '') this._repeat.intValue = 0; // start with no shuffle
    if (this._shuffle.stringValue == '') this._shuffle.boolValue = false; // start with no shuffle
    if (this._volume.stringValue == '') this._volume.intValue = 128;
    this._requestedVolume = this._calculatedVolume = this._volume.intValue;
  },
  
  _releaseDataRemotes: function() {
    // And we have to let them go when we're done else all hell breaks loose.
    this._playButton.unbind();
    this._playUrl.unbind();
    this._seenPlaying.unbind();
    this._volume.unbind();
    this._muteData.unbind();
    this._playingRef.unbind();
    this._playlistRef.unbind();
    this._playlistIndex.unbind();
    this._repeat.unbind();
    this._shuffle.unbind();
    this._showRemaining.unbind();
    this._metadataTitle.unbind();
    this._metadataArtist.unbind();
    this._metadataAlbum.unbind();
    this._metadataUrl.unbind();
    this._metadataPos.unbind();
    this._metadataLen.unbind();
    this._resetSearchData.unbind();
    this._metadataPosText.unbind();
    this._metadataLenText.unbind();
    this._faceplateState.unbind();
    this._restartOnPlaybackEnd.unbind();
    this._statusText.unbind();
    this._statusStyle.unbind();
  },
  
  /**
   * Tries to pick a valid core if one is removed.
   * Right now it just picks the first in the list (DUMB)
   */
  _chooseCore : function() {
    if (this._cores.length > 0)
      this._currentCoreIndex = 0;
    else
      this._currentCoreIndex = -1;
  },

  /**
   * ---------------------------------------------
   * Public get/set Property Accessors
   * ---------------------------------------------
   */

  /**
   * Access to the currently selected core object
   */
  get core() {
//    LOG("get core: _cores.length = " + this._cores.length);
//    LOG("get core: _currentCoreIndex = " + this._currentCoreIndex);
    
    if ((this._cores.length > 0) &&
        (this._currentCoreIndex > -1) &&
        (this._currentCoreIndex <= this._cores.length - 1)) {
//      LOG("get core: returning, core.getId = " + this._cores[this._currentCoreIndex].getId());
      return this._cores[this._currentCoreIndex];
    }
    else {
//      LOG("get core: returning null");
      return null;
    }
  },
  set core(val) {
    LOG("set core: core.getId = " + val.id);
    this.selectCore(val);
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  get volume() {
    return this.getVolume();
  },
  set volume(val) {
    this.setVolume(val);
  },
  
  /**
   * See sbIPlaylistPlayback.idl
   */
  get mute() {
    return this.getMute();
  },
  set mute(val) {
    this.setMute(val);
  },
  
  /**
   * See sbIPlaylistPlayback.idl
   */
  get length() {
    return this.getLength();
  },
  
  /**
   * See sbIPlaylistPlayback.idl
   */
  get position() {
    return this.getPosition();
  },
  set position(val) {
    this.setPosition(val);
  },
  
  /**
   * See sbIPlaylistPlayback.idl
   */
  get repeat() {
    return this.getRepeat();
  },
  set repeat(val) {
    this.setRepeat(val);
  },
  
  /**
   * See sbIPlaylistPlayback.idl
   */
  get shuffle() {
    return this.getShuffle();
  },
  set shuffle(val) {
    this.setShuffle(val);
  },
  
  /**
   * See sbIPlaylistPlayback.idl
   */
  get currentIndex() {
    return 0;
  },
  set currentIndex(val) {
    
  },
  
  /**
   * See sbIPlaylistPlayback.idl
   */
  get itemCount() {
    return 0;
  },
  set itemCount(val) {
    
  },
  
  /**
   * See sbIPlaylistPlayback.idl
   */
  get currentGUID() {
    return "";
  },
  
  /**
   * See sbIPlaylistPlayback.idl
   */
  get currentURL() {
    return "";
  },
  
  /**
   * ---------------------------------------------
   * Public Methods
   * ---------------------------------------------
   */

  /**
   * Add a core to the array. Optionally select it.
   */
  addCore: function(core, select) {
    LOG("addCore: core.id = " + core.getId());
    for (var i = 0; i < this._cores.length; i++) {
      if (this._cores[i] == core)
        if (select)
          this._currentCoreIndex = i;
        return;
    }
    LOG("addCore: new core");
    var testCore = core;
    if (testCore.QueryInterface(SONGBIRD_COREWRAPPER_IID)) {
      LOG("addCore: new core has valid interface");
      this._cores.push(core);
      if (select) {
        LOG("addCore: selecting new core");
        this._currentCoreIndex = 0;
      }
      core.setMute(this._muteData.boolValue);
      core.setVolume(this._volume.intValue);
    }
    else
      throw Components.results.NS_ERROR_INVALID_ARG;
 },
  
  /**
   *
   */
  removeCore: function(core) {
    LOG("removeCore with core.getId = " + core.getId());
    for (var i = 0; i < this._cores.length; i++) {
      if (this._cores[i] == core) {
        this._cores.splice(i, 1);
        if (i == this._currentCoreIndex)
          this._chooseCore();
        else if (i < this._currentCoreIndex)
          this._currentCoreIndex--;
        return;
      }
    }
  },

  /**
   *
   */
  selectCore: function(core) {
    LOG("selectCore with core.getId = " + core.getId());
    this.addCore(core, true);
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  play: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    if (this._started)
    { 
      if (core.getPaused() == false) core.pause();
      else core.play();
    }
    else
    {
      this._playDefault();
    }
    // Hide the intro box and show the normal faceplate box
    this._faceplateState.boolValue = true;
    return true;
  },
  
  /**
   * See sbIPlaylistPlayback.idl
   */
  playRef: function(source_ref, index) {
    LOG("source = " + source_ref + " index = " + index);
    if (!source_ref || (index == null) || (index < 0))
      throw Components.results.NS_ERROR_INVALID_ARG;
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    this._updateCurrentInfo(source_ref, index);

    // Then play it
    this.playUrl(this._playUrl.stringValue);

    // Hide the intro box and show the normal faceplate box
    this._faceplateState.boolValue = true;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  playRefByID: function(source_ref, row_id) {
    LOG("source = " + source_ref + " row_id = " + row_id);
    if (!source_ref || (row_id == null) || (row_id < 0))
      throw Components.results.NS_ERROR_INVALID_ARG;
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
  
    var index = this._source.getRefRowByColumnValue(source_ref, "id", row_id);
    
    return this.playRef(source_ref, index);
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  playRefByUUID: function(source_ref, media_uuid) {
    LOG("source = " + source_ref + " media_uuid = " + media_uuid);
    if (!source_ref || (media_uuid == ""))
      throw Components.results.NS_ERROR_INVALID_ARG;
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    var index = this._source.getRefRowByColumnValue(source_ref, "uuid", media_uuid);
    return this.playRef(source_ref, index);
  },
  
  /**
   * See sbIPlaylistPlayback.idl
   */
  playRefByURL: function(source_ref, url) {
    LOG("source = " + source_ref + " url = " + url);
    if (!source_ref || (url == null) || (url == ""))
      throw Components.results.NS_ERROR_INVALID_ARG;
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    this.playUrl(url);
    setTimeout('var index = this._source.getRefRowByColumnValue(' + source_ref + ', "url",' + url + '); this._updateCurrentInfo(' + source_ref + ', index )', 250);
   
    return;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  playTable: function(dbGUID, table, index) {
    LOG("playTable - db: " + dbGUID + "\ntable: " + table + "\nindex: " + index);
    
    if ( !dbGUID || !table || (index == null) )
      throw Components.results.NS_ERROR_INVALID_ARG;
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    // Create a ref different from what playlist.xml uses as a ref.
    var theRef = "sbPlaylistPlayback.js_" + dbGUID + "_" + table;
    
    // Tell playlistsource to set up that ref to the requested playlist
    this._source.feedPlaylist( theRef, dbGUID, table );
    this._source.executeFeed( theRef );
    
    // Synchronous call!  Woo hoo!
    while( this._source.isQueryExecuting( theRef ) );

    // After the call is done, force GetTargets
    this._source.forceGetTargets( theRef );
    
    // And then use the source to play that ref.
    return this.playRef( theRef, index );
  },

  playTableByUrl: function(dbGUID, table, url) {
    var index = this._source.getRefRowByColumnValue(source_ref, "url", url);
    this.playTable(dbGUID, table, index);
  },
  
  playTableByID: function(dbGUID, table, row_id) {
    var index = this._source.getRefRowByColumnValue(source_ref, "id", row_id);
    this.playTable(dbGUID, table, index);
  },
  
  /**
   * See sbIPlaylistPlayback.idl
   */
  playObject: function(playlist, index) {
    if (!playlist || !index)
      throw Components.results.NS_ERROR_INVALID_ARG;
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
      
    if ( playlist.ref )
      return this.playRef(playlist.ref, index);
    else
      throw Components.results.NS_ERROR_INVALID_ARG;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  playUrl: function(url) {
    try  {
      var core = this.core;
      if (!core)
        throw Components.results.NS_ERROR_NOT_INITIALIZED;
        
      this._playUrl.stringValue = url;
      this._metadataUrl.stringValue = url;
      
      core.stop();
      core.playUrl( url );
      
      LOG( "Playing '" + core.getId() + "'(" + this.getLength() + "/" + this.getPosition() + ") - playing: " + this.getPlaying() + " paused: " + this.getPaused() );
      
      // Start the polling loop to feed the metadata dataremotes.
      this._startPlayerLoop();
      
      // Hide the intro box and show the normal faceplate box
      this._faceplateState.boolValue = true;

      // metrics
      var s = url.split(".");
      if (s.length > 1)
      {
        var ext = s[s.length-1];
        this.metrics_inc("play.attempt", ext, null);
      }
      this.metrics_inc("play.attempt", core.getId(), null);
    } catch( err ) {
      LOG( "playUrl:\n" + err );
      return false;
    }
    return true;
  },
  
  playAndImportUrl: function(url) {
    try  {
      var row = this._importUrlInLibrary(url);
      this.playRefByURL("NC:songbird_library", url);
    } catch( err ) {
      LOG( "playAndImportUrl:\n" + err );
      return false;
    }
    return true;
  },
  
  importUrl: function(url) {
    return this._importUrlInLibrary(url);
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  pause: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    core.pause();
    return true;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  stop: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    core.stop();
    this._stopPlayerLoop(); // oh VERY important!
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  next: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    this._playNextPrev(1);
    return 0;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  previous: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    this._playNextPrev(-1);
    return 0;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  current: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return 0;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  getPaused: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return core.getPaused();
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  getPlaying: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return core.getPlaying();
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  getStarted: function() {
    return this._started;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  getVolume: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    var retval = core.getVolume();
    if ( retval == this._calculatedVolume ) {
      retval = this._requestedVolume;
    }
//    dump( "GetVolume -- req: " + this._requestedVolume + " calc: " + this._calculatedVolume + " retval: " + retval + "\n" );
    return retval;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  setVolume: function(volume) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    core.setVolume(volume);
    this._requestedVolume = volume;
    this._calculatedVolume = core.getVolume();
//    dump( "SetVolume -- req: " + this._requestedVolume + " calc: " + this._calculatedVolume + "\n" );
    this._onPollVolume();
    return false;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  getMute: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return core.getMute();
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  setMute: function(mute) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    core.setMute(mute);
    // some cores set their volume to 0 on setMute(true), but do not restore
    // the volume when mute is turned off, this fixes the problem
    if (mute == false) {
      core.setVolume(this._calculatedVolume);
    }
    this._onPollMute(core); // if the core is not playing, the loop is not running, but we still want the new mute state (and possibly volume) to be routed to all the UI controls
    return true;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  getLength: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return core.getLength();
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  getPosition: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return core.getPosition();
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  setPosition: function(position) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    core.setPosition(position);
    return true;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  setRepeat: function(repeatMode) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  getRepeat: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return REPEAT_MODE_OFF;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  setShuffle: function(shuffle) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
      
    var shuffleVal = shuffle ? 1 : 0;


    // XXX what is this.m_strName?
    var query = ""; //"UPDATE " + this._table + ' SET shuffle = "' + shuffleVal + '" WHERE name = "' + /*this.m_strName +*/ '"');
/*    

      // Uhhhhhh..... why are we handling shuffle all funny?  Shuffle isn't per table.


    this._db.resetQuery();
    this._db.addQuery(query);
    this._db.execute();
    this._db.waitForCompletion();
*/    
    return;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  getShuffle: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return false;
  },
  
  /**
   * See sbIPlaylistPlayback.idl
   */
  goFullscreen: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    core.goFullscreen();
  },
  

  /**
   * See sbIPlaylistPlayback.idl
   */
  getMetadataFields: function(/*out*/ fieldCount, /*out*/ metaFields) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    fieldCount = 0;
    metaFields = new Array();
    return;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  getCurrentValue: function(field) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return "";
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  setCurrentValue: function(field, value) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  getCurrentValues: function(fieldCount, metaFields, /*out*/ valueCount, /*out*/ metaValues) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    valueCount = 0;
    metaValues = new Array();
    return;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  setCurrentValues: function(fieldCount, metaFields, valueCount, metaValues) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return;
  },

  isMediaUrl: function(the_url) {
    if ( ( the_url.indexOf ) && 
          (
            // Known playlist type?
            ( this.isPlaylistUrl( the_url ) ) ||
            // Protocols at the beginning
            ( the_url.indexOf( "mms:" ) == 0 ) || 
            ( the_url.indexOf( "rtsp:" ) == 0 ) || 
            // File extensions at the end
            ( the_url.indexOf( ".pls" ) != -1 ) || 
            ( the_url.indexOf( "rss" ) != -1 ) || 
            ( the_url.indexOf( ".m3u" ) == ( the_url.length - 4 ) ) || 
  //          ( the_url.indexOf( ".rm" ) == ( the_url.length - 3 ) ) || 
  //          ( the_url.indexOf( ".ram" ) == ( the_url.length - 4 ) ) || 
  //          ( the_url.indexOf( ".smil" ) == ( the_url.length - 5 ) ) || 
            ( the_url.indexOf( ".mp3" ) == ( the_url.length - 4 ) ) ||
            ( the_url.indexOf( ".ogg" ) == ( the_url.length - 4 ) ) ||
            ( the_url.indexOf( ".flac" ) == ( the_url.length - 5 ) ) ||
            ( the_url.indexOf( ".wav" ) == ( the_url.length - 4 ) ) ||
            ( the_url.indexOf( ".m4a" ) == ( the_url.length - 4 ) ) ||
            ( the_url.indexOf( ".wma" ) == ( the_url.length - 4 ) ) ||
            ( the_url.indexOf( ".wmv" ) == ( the_url.length - 4 ) ) ||
            ( the_url.indexOf( ".asx" ) == ( the_url.length - 4 ) ) ||
            ( the_url.indexOf( ".asf" ) == ( the_url.length - 4 ) ) ||
            ( the_url.indexOf( ".avi" ) == ( the_url.length - 4 ) ) ||
            ( the_url.indexOf( ".mov" ) == ( the_url.length - 4 ) ) ||
            ( the_url.indexOf( ".mpg" ) == ( the_url.length - 4 ) ) ||
            ( the_url.indexOf( ".mp4" ) == ( the_url.length - 4 ) )
          )
        )
    {
      return true;
    }
    return false;
  },

  isPlaylistUrl: function(the_url) {
    try
    {
      if ( the_url.indexOf )
      {
        // Make the playlist reader manager.
        const PlaylistReaderManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistReaderManager;1", "sbIPlaylistReaderManager");
        var aPlaylistReaderManager = (new PlaylistReaderManager()).QueryInterface(Components.interfaces.sbIPlaylistReaderManager);
        
        // Tell it what filters to be using
        var filterlist = "";
        var extensionCount = new Object;
        var extensions = aPlaylistReaderManager.supportedFileExtensions(extensionCount);

        for(var i = 0; i < extensions.length; i++)
        {
          if ( the_url.indexOf( "." + extensions[i] ) != -1 )
          {      
            return true;
          }
        }
      }
    }
    catch ( err )
    {
      alert( "IsPlaylistUrl - " + err );
    }
    return false;
  },

  /**
   * Handle Observer Service notifications
   */
  observe: function(subject, topic, data) {
    switch (topic) {
    case "profile-after-change":
      gOS.removeObserver(this, "profile-after-change");
      
      // Preferences are initialized, ready to start the service
      this._init();
      break;
    case "xpcom-shutdown":
      gOS.removeObserver(this, "xpcom-shutdown");
      this._deinit();
      
      // Release Services to avoid memory leaks
      gConsole  = null;
      gOS       = null;
      break;
    }
  },
  
//
// Below here are local items used internally to do player loop handling.
//

  notify: function( timer ) { // nsITimerCallback
    this._onPlayerLoop();
  },
  
  _startPlayerLoop: function () {
    this._stopPlayerLoop();
    this._started = 1;
    this._seenPlaying.boolValue = false;
    this._lookForPlayingCount = 0;
    this._timer.initWithCallback( this, 250, 1 ) // TYPE_REPEATING_SLACK
  },
  
  _stopPlayerLoop: function () {
    this._timer.cancel();
    this._started = 0;
  },
  
  // Poll function
  _onPlayerLoop: function () {
    try {
      var core = this.core;
      if (!core)
        throw Components.results.NS_ERROR_NOT_INITIALIZED;

      var len = this.getLength();
      var pos = this.getPosition();
      
      if ( !this._once ) {
        LOG( "_onPlayerLoop '" + core.getId() + "'(" + this.getLength() + "/" + this.getPosition() + ") - playing: " + this.getPlaying() + " paused: " + this.getPaused() );
        this._once = true;
      }
        
      // When the length changes, always set metadata.
      if ( this._metadataLen.intValue != len ) 
        this._set_metadata = true; 
      this._metadataLen.intValue = len;
      this._metadataPos.intValue = pos;
      
      // Ignore metadata when paused.
      if ( core.getPlaying() && ! core.getPaused() ) {
        this._onPollMetadata( len, pos, core );
      }
      
      // Call all the wacky poll functions!  yeehaw!
      this._onPollVolume( core );
      this._onPollMute( core );
      this._onPollTimeText( len, pos );
      this._onPollButtons( len, core );
      this._onPollCompleted( len, pos, core );
    }       
    catch ( err )  
    {
      LOG( "!!! ERROR: sbPlaylistPlayback _onPlayerLoop\n\n" + err + "\n" );
    }
  },
  

  // Elapsed / Total display
 
  _onPollTimeText: function ( len, pos ) {
    this._metadataPosText.stringValue = EmitSecondsToTimeString( pos / 1000.0 ) + " ";

    if ( len > 0 && this._showRemaining.boolValue )
      this._metadataLenText.stringValue = "-" + EmitSecondsToTimeString( ( len - pos ) / 1000.0 );
    else
      this._metadataLenText.stringValue = " " + EmitSecondsToTimeString( len / 1000.0 );
  },


  // Routes volume changes from the core to the volume ui data remote 
  
  _onPollVolume: function ( core ) {
      var v = this.getVolume();
      if ( v != this._volume.intValue ) {
        this._volume.intValue = v;
    }
  },

  _onPollMute: function ( core ) {
      var mute = core.getMute();
      if ( mute != this._muteData.boolValue ) {
        this._muteData.boolValue = mute;
      }
  },

  // Routes core playback status changes to the play/pause button ui data remote

  _onPollButtons: function ( len, core ) {
    if ( core.getPlaying() && ( ! core.getPaused() ) && ( len > 0 ) )
      this._playButton.boolValue = false;
    else
      this._playButton.boolValue = true;
  },

  // Routes metadata (and their possible updates) to the metadata ui data remotes
  // Also updates the database if the core reads metadata that is different from
  // what we had so far
  _onPollMetadata: function ( len_ms, pos_ms, core )
  {
    // Wait a bit, and then only ask infrequently
    if ( ( pos_ms > 250 ) && ( ( this._metadataPollCount++ % 20 ) == 0 ) ) {
      // Ask, and ye shall receive
      var title = "" + core.getMetadata("title");
      var album = "" + core.getMetadata("album");
      var artist = "" + core.getMetadata("artist");
      var genre = "" + core.getMetadata("genre");
      var length = "";
      if ( len_ms >= 0 )
        length = EmitSecondsToTimeString( len_ms / 1000.0 );

      // Glaaaaaaah!        
      if ( title == "null" ) title = "";
      if ( album == "null" ) album = "";
      if ( artist == "null" ) artist = "";
      if ( genre == "null" ) genre = "";
      
      if ( length == "Error" ) {
        return;
      }
      
      // If the current title is part of the url, it is okay to overwrite the title.
      if ( title.length && ( 
          ( this._metadataTitle.stringValue != title ) &&
          ( this._playUrl.stringValue.indexOf( this._metadataTitle.stringValue ) != -1 )
         ) )
        this._set_metadata = true; 
      else
        title = "";
      // Only if we have no known artist.
      if ( artist.length && ( this._metadataArtist.stringValue == "" ) )
        this._set_metadata = true; 
      else
        artist = "";
      // Only if we have no known album.
      if ( album.length && ( this._metadataAlbum.stringValue == "" ) )
        this._set_metadata = true; 
      else
        album = "";

      if ( this._set_metadata ) {
        // Set the metadata into the database table
        this._setURLMetadata( this._playUrl.stringValue, title, length, album, artist, genre, true );
        // Tell the search popup the metadata has changed
        this._resetSearchData.intValue++;
        this._set_metadata = false;
      }

      if (title != "")
        this._metadataTitle.stringValue = title;
      if (artist != "")
        this._metadataArtist.stringValue = artist;
      if (album != "")
        this._metadataAlbum.stringValue = album;
    }
  },

  _onPollCompleted: function ( len, pos, core ) {
    // Basically, this logic means that posLoop will run until the player first says it is playing, and then stops.
    if ( core.getPlaying() && ( len > 0.0 ) ) {
      // First time we see it playing, 
      if ( ! this._seenPlaying.boolValue ) {
        // Clear the search popup
        this._resetSearchData.intValue++;
        this._metadataPollCount = 0; // start the count again.
      }
      // Then remember we saw it
      this._seenPlaying.boolValue = true;
    }
    // If we haven't seen ourselves playing, yet, we couldn't have stopped.
    else if ( this._seenPlaying.boolValue || ( len < 0.0 ) ) {
      // Oh, NOW you say we've stopped, eh?
      this._seenPlaying.boolValue = false;
      this._stopPlayerLoop();
      
      this._playNextPrev(1);
    }
    else {
      // After 10 seconds or fatal error, give up and go to the next one?
      if ( ( this._lookForPlayingCount++ > 40 ) || ( len < -1 ) )
        this.next();
    }
  },
  
  _playNextPrev: function ( incr ) {
    // "FIXME" -- mig sez: I think the reason it was broke is because you
    // basically made it restart on playLIST end.  And that would get confusing,
    // I assume.
    if (this._restartOnPlaybackEnd.boolValue)
    { 
      restartApp();
      return;
    }
                                                        // GRRRRRR!
    var cur_index = this._playlistIndex.intValue; // this._findCurrentIndex;
    var cur_ref = this._playingRef.stringValue;
    var cur_url = this._playUrl.stringValue;
  
    // Doublecheck that filters match what they were when we last played something.  
    var panic = false;
    var num_filters = this._source.getNumFilters( cur_ref );
    if (num_filters > 0)
    {
      var i;
      for (i = 0; i < num_filters; i++)
      {
        // If they're not the same, just return from this.  No more play for you!
        var filter = this._source.getFilter( cur_ref, i );
        if (this.filters[i] != filter)
          panic = true; // WHOA, now what?  Try to use the current url!
      };
    };
    
    if (panic)
    {
      // So, we're in panic mode.  The filters have changed.
      var index = this._source.getRefRowByColumnValue(cur_ref, "url", cur_url);
      // If we can find the current url in the current list, use that as the index
      if (index != -1)
        cur_index = index;
      else
        return; // Otherwise, stop playback.
    };
    
    // XXXredfive - this looks redundant. Do we do this for data remote reasons?
    this._playlistIndex.intValue = cur_index;

    LOG( "current index: " + cur_index );
    
    if ( cur_index > -1 ) {
      // Play the next playlist entry tree index (or whatever, based upon state.)
      var num_items = this._source.getRefRowCount( cur_ref );
      LOG( num_items + " items in the current playlist" );
      var next_index = -1;
      // Are we confused?
      if ( cur_index != -1 ) {
        // Are we REPEAT ONE?
        if ( this._repeat.intValue == 1 ) {
          next_index = cur_index;
        }
        // Are we SHUFFLE?
        else if ( this._shuffle.boolValue ) {
          //Does shuffle look like it's supposed to be FUCKING RANDOM. Could we *PLEASE* have a real shuffle. *PLEASE*.
          //Thanks. --aus
          var rand = num_items * Math.random();
          next_index = Math.floor( rand );
          LOG( "shuffle: " + next_index );
        }
        else {
          // Increment / Decrement
          next_index = parseInt( cur_index ) + parseInt( incr );
          LOG( "increment: " + next_index );
          // Are we at the end?
          if ( next_index >= num_items ) 
            // Are we REPEAT ALL?
            if ( this._repeat.intValue == 2 )
              next_index = 0; // Start over
            else
              next_index = -1; // Give up
        }
      }
      
      // If we think we want to play a track, do so.
      LOG( "next index: " + next_index );
      if ( next_index != -1 ) 
        this.playRef( cur_ref, next_index );
        
    }
  },
  
  _findCurrentIndex: function () {
    var retval = -1;
    var ref = this._playingRef.stringValue;
    if ( this._playingRef.stringValue.length > 0 ) {
      retval = this._source.getRefRowByColumnValue( ref, "url", this._playUrl.stringValue );
    }
    return retval;
  },
  
  _playDefault: function () 
  {
    if ( this._playingRef && this._playingRef.stringValue.length ) {
      // if there is a song to play, play it
      this.playRef(this._playingRef.stringValue, 0);
    } 
    else {
      // otherwise play the library
      this.playRef("NC:songbird_library", 0);
    }
  },
  
  _importUrlInLibrary: function( the_url )
  {
    const MediaLibrary = new Components.Constructor("@songbirdnest.com/Songbird/MediaLibrary;1", "sbIMediaLibrary");
    var library = (new MediaLibrary()).QueryInterface(Components.interfaces.sbIMediaLibrary);
    var queryObj = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].createInstance(Components.interfaces.sbIDatabaseQuery);
    queryObj.setDatabaseGUID("songbird");
    library.setQueryObject(queryObj);
    var keys = new Array( "title" );
    var values = new Array();
    values.push( ConvertUrlToDisplayName( the_url ) );
    var guid = library.addMedia( the_url, keys.length, keys, values.length, values, false, false );
    LOG("add media = " + guid);
    var row = library.getValueByGUID(guid, "id");
    LOG("findbyguid = " + row);
    return row;
  },
  
/*  
*/
  // Updates the database with metadata changes

  _setURLMetadata: function( aURL, aTitle, aLength, aAlbum, aArtist, aGenre, boolSync, aDBQuery, execute ) {
    if ( aDBQuery == null ) {
      aDBQuery = new sbIDatabaseQuery();
      aDBQuery.setAsyncQuery(true);
      aDBQuery.setDatabaseGUID("songbird");
    }
    if ( execute == null ) {
      execute = true;
    }
      
    if ( aTitle && aTitle.length ) {
      var q = 'update library set title="'   + aTitle  + '" where url="' + aURL + '"';
      aDBQuery.addQuery( q );
    }
    if ( aLength && aLength.length ) {
      var q = 'update library set length="'    + aLength + '" where url="' + aURL + '"';
      aDBQuery.addQuery( q );
    }
    if ( aAlbum && aAlbum.length ) {
      var q = 'update library set album="'  + aAlbum  + '" where url="' + aURL + '"';
      aDBQuery.addQuery( q );
    }
    if ( aArtist && aArtist.length ) {
      var q = 'update library set artist="' + aArtist + '" where url="' + aURL + '"';
      aDBQuery.addQuery( q );
    }
    if ( aGenre && aGenre.length ) {
      var q = 'update library set genre="'  + aGenre  + '" where url="' + aURL + '"';
      aDBQuery.addQuery( q );
    }
    
    if ( execute ) {
      var ret = aDBQuery.execute();
    }
      
    return aDBQuery; // So whomever calls this can keep track if they want.
  },
  
  
  _updateCurrentInfo: function(source_ref, index)
  {
    // These define what is _actually_ playing
    this._playingRef.stringValue = source_ref;
    this._playlistIndex.intValue = index;
    
    // And from those, we ask the Playlistsource to tell us who to play
    var url = this._source.getRefRowCellByColumn( source_ref, index, "url" );
    var title = this._source.getRefRowCellByColumn( source_ref, index, "title" );
    var artist = this._source.getRefRowCellByColumn( source_ref, index, "artist" );
    var album = this._source.getRefRowCellByColumn( source_ref, index, "album" );
    
    // Set the data remotes to indicate what's about to play
    this._playUrl.stringValue = url;
    this._metadataUrl.stringValue = url;
    this._metadataTitle.stringValue = title;
    this._metadataArtist.stringValue = artist;
    this._metadataAlbum.stringValue = album;
    
    // Record, internally, what the filters on the ref are.
    // If the filters are different when we are asked to play next/prev, fail.
    this.num_filters = this._source.getNumFilters( source_ref );
    this.filters = new Array();
    var i;
    for (i = 0; i < this.num_filters; i++)
    {
      this.filters.push( this._source.getFilter( source_ref, i ) );
    };
  },
  
  /**
   * QueryInterface is always last, it has no trailing comma.
   */
  QueryInterface: function(iid) {
    if (!iid.equals(SONGBIRD_PLAYLISTPLAYBACK_IID) &&
        !iid.equals(Components.interfaces.nsIObserver) && 
        !iid.equals(Components.interfaces.nsITimerCallback) && 
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
  
}; // PlaylistPlayback.prototype




















/**
 * ----------------------------------------------------------------------------
 * Registration for XPCOM
 * ----------------------------------------------------------------------------
 * Adapted from nsUpdateService.js
 */
var gModule = {
  registerSelf: function(componentManager, fileSpec, location, type) {
    componentManager = componentManager.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    for (var key in this._objects) {
      var obj = this._objects[key];
      componentManager.registerFactoryLocation(obj.CID, obj.className, obj.contractID,
                                               fileSpec, location, type);
    }
    var categoryManager = Components.classes["@mozilla.org/categorymanager;1"]
                                    .getService(Components.interfaces.nsICategoryManager);
    categoryManager.addCategoryEntry("app-startup", this._objects.playback.className,
                                     "service," + this._objects.playback.contractID, 
                                     true, true, null);
  },

  getClassObject: function(componentManager, cid, iid) {
    if (!iid.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    for (var key in this._objects) {
      if (cid.equals(this._objects[key].CID))
        return this._objects[key].factory;
    }
    
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  _makeFactory: #1= function(ctor) {
    function ci(outer, iid) {
      if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;
      return (new ctor()).QueryInterface(iid);
    } 
    return { createInstance: ci };
  },
  
  _objects: {
    // The PlaylistPlayback Component
    playback: { CID        : SONGBIRD_PLAYLISTPLAYBACK_CID,
                contractID : SONGBIRD_PLAYLISTPLAYBACK_CONTRACTID,
                className  : SONGBIRD_PLAYLISTPLAYBACK_CLASSNAME,
                factory    : #1#(PlaylistPlayback)
              },
  },

  canUnload: function(componentManager) { 
    return true; 
  },
  
  unregisterSelf: function(componentManager, fileSpec, location, type) {
    var categoryManager = Components.classes["@mozilla.org/categorymanager;1"]
                                    .getService(Components.interfaces.nsICategoryManager);
    categoryManager.deleteCategoryEntry("app-startup",
                                        this._objects.playback.contractID, true);
  }
}; // gModule

function NSGetModule(comMgr, fileSpec) {
  return gModule;
} // NSGetModule
