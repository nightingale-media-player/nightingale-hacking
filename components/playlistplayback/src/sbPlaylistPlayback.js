/**
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
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
const SONGBIRD_PLAYLISTPLAYBACK_CONTRACTID = "@songbird.org/Songbird/PlaylistPlayback;1";
const SONGBIRD_PLAYLISTPLAYBACK_CLASSNAME = "Songbird Playlist Playback Interface";
const SONGBIRD_PLAYLISTPLAYBACK_CID = Components.ID("{190b1e87-8769-43b4-a362-4065eb6730e5}");
const SONGBIRD_PLAYLISTPLAYBACK_IID = Components.interfaces.sbIPlaylistPlayback;

// Songbird ContractID Stuff
const SONGBIRD_COREWRAPPER_CONTRACTID = "@songbird.org/Songbird/CoreWrapper;1";
const SONGBIRD_COREWRAPPER_IID = Components.interfaces.sbICoreWrapper;

const SONGBIRD_DATAREMOTE_CONTRACTID = "@songbird.org/Songbird/DataRemote;1";
const SONGBIRD_DATAREMOTE_IID = Components.interfaces.sbIDataRemote;

const SONGBIRD_DATABASEQUERY_CONTRACTID = "@songbird.org/Songbird/DatabaseQuery;1";
const SONGBIRD_DATABASEQUERY_IID = Components.interfaces.sbIDatabaseQuery;

const SONGBIRD_MEDIALIBRARY_CONTRACTID = "@songbird.org/Songbird/MediaLibrary;1";
const SONGBIRD_MEDIALIBRARY_IID = Components.interfaces.sbIMediaLibrary;

// String Bundles
const URI_SONGBIRD_PROPERTIES    = "chrome://songbird/locale/songbird.properties";

// Database GUIDs
const DB_TEST_GUID = "testdb-0000";

// Other XPCOM Stuff
/*
const sbIDatabaseQuery = new Components.Constructor("@songbird.org/Songbird/DatabaseQuery;1", "sbIDatabaseQuery");
const sbIPlaylistsource = new Components.Constructor("@mozilla.org/rdf/datasource;1?name=playlist", "sbIPlaylistsource");
const sbIMediaLibrary = new Components.Constructor("@songbird.org/Songbird/MediaLibrary;1", "sbIMediaLibrary");
const sbIPlaylistManager = new Components.Constructor("@songbird.org/Songbird/PlaylistManager;1", "sbIPlaylistManager");
const sbIPlaylist = new Components.Constructor("@songbird.org/Songbird/Playlist;1", "sbIPlaylist");
const sbISimplePlaylist = new Components.Constructor("@songbird.org/Songbird/SimplePlaylist;1", "sbISimplePlaylist");
const sbIDynamicPlaylist = new Components.Constructor("@songbird.org/Songbird/DynamicPlaylist;1", "sbIDynamicPlaylist");
const sbISmartPlaylist = new Components.Constructor("@songbird.org/Songbird/SmartPlaylist;1", "sbISmartPlaylist");
const sbIPlaylistReaderManager = new Components.Constructor("@songbird.org/Songbird/PlaylistReaderManager;1", "sbIPlaylistReaderManager");
const sbIPlaylistReaderListener = new Components.Constructor("@songbird.org/Songbird/PlaylistReaderListener;1", "sbIPlaylistReaderListener");
const sbIMediaScan = new Components.Constructor("@songbird.org/Songbird/MediaScan;1", "sbIMediaScan");
const sbIMediaScanQuery = new Components.Constructor("@songbird.org/Songbird/MediaScanQuery;1", "sbIMediaScanQuery");
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


// Quick tool to make this process more friendly
function createAndInitDataRemote(key, root) {
  var newDataRemote = Components.classes[SONGBIRD_DATAREMOTE_CONTRACTID]
                              .createInstance(SONGBIRD_DATAREMOTE_IID);
  if (!newDataRemote)
    throw Components.results.NS_ERROR_FAILURE;
    
  if (root)
    newDataRemote.init(key, root);
  else
    newDataRemote.init(key, null);
  
  return newDataRemote;
} // createAndInitDataRemote


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
  _lastVolume:         null,
  _volumeLastReadback: null,
  _trackerVolume:      null,
  _volume:             null,
  _muteData:           null,
  _seekbarTracker:     null,
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
    
  _faceplateState:       null,
  _restartOnPlaybackEnd: null,
  _seekbarTracker:       null,
  _resetSearchData:      null,

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
    // HOLY SMOKES we use lots of data elements.
  
    // Play/Pause image toggle
    this._playButton            = createAndInitDataRemote("faceplate.play");
    //string current           
    this._playUrl               = createAndInitDataRemote("faceplate.play.url");
    //actually playing         
    this._seenPlaying           = createAndInitDataRemote("faceplate.seenplaying");
    this._lastVolume            = createAndInitDataRemote("faceplate.volume.last");
    this._volumeLastReadback    = createAndInitDataRemote("faceplate.volume.lastreadback");
    this._trackerVolume         = createAndInitDataRemote("faceplate.volume.tracker");
    this._volume                = createAndInitDataRemote("faceplate.volume");
    //t/f                      
    this._muteData              = createAndInitDataRemote("faceplate.mute");
    this._playingRef            = createAndInitDataRemote("playing.ref");
    this._playlistRef           = createAndInitDataRemote("playlist.ref");
    this._playlistIndex         = createAndInitDataRemote("playlist.index");
    this._repeat                = createAndInitDataRemote("playlist.repeat");
    this._shuffle               = createAndInitDataRemote("playlist.shuffle");
    this._showRemaining         = createAndInitDataRemote("faceplate.showremainingtime");
                               
    this._metadataTitle         = createAndInitDataRemote("metadata.title");
    this._metadataArtist        = createAndInitDataRemote("metadata.artist");
    this._metadataAlbum         = createAndInitDataRemote("metadata.album");
    this._metadataUrl           = createAndInitDataRemote("metadata.url");
    this._metadataPos           = createAndInitDataRemote("metadata.position");
    this._metadataLen           = createAndInitDataRemote("metadata.length");
    this._seekbarTracker        = createAndInitDataRemote("faceplate.seek.tracker");
    this._resetSearchData       = createAndInitDataRemote("faceplate.search.reset");
    this._metadataPosText       = createAndInitDataRemote("metadata.position.str");
    this._metadataLenText       = createAndInitDataRemote("metadata.length.str");
    this._faceplateState        = createAndInitDataRemote("faceplate.state");
    this._restartOnPlaybackEnd  = createAndInitDataRemote("restart.onplaybackend");

// Set startup defaults
    this._metadataPos.setValue( 0 );
    this._metadataLen.setValue( 0 );
    this._seenPlaying.setBoolValue(false);
    this._metadataPosText.setValue( "0:00:00" );
    this._metadataLenText.setValue( "0:00:00" );
    this._showRemaining.setBoolValue(false);
    this._trackerVolume.setBoolValue(false);
    this._volumeLastReadback.setValue( -1 );
    this._muteData.setBoolValue( false );
    this._playlistRef.setValue( "" );
    this._playlistIndex.setValue( -1 );
    this._faceplateState.setValue( 0 );
    this._restartOnPlaybackEnd.setBoolValue( false );
//    this._metadataTitle.setValue( "" );
//    this._metadataArtist.setValue( "" );
//    this._metadataAlbum.setValue( "" );
    this._playingRef.setValue( "" );
    this._playUrl.setValue( "" ); 
    this._playButton.setValue( 1 ); // Start on.
    this._repeat.setValue( 0 ); // start with no shuffle
    this._shuffle.setBoolValue( false ); // start with no shuffle
  },
  
  _releaseDataRemotes: function() {
    // And we have to let them go when we're done else all hell breaks loose.
    this._playButton.unbind();
    this._playUrl.unbind();
    this._seenPlaying.unbind();
    this._lastVolume.unbind();
    this._volumeLastReadback.unbind();
    this._trackerVolume.unbind();
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
    this._seekbarTracker.unbind();
    this._resetSearchData.unbind();
    this._metadataPosText.unbind();
    this._metadataLenText.unbind();
    this._faceplateState.unbind();
    this._restartOnPlaybackEnd.unbind();
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
    if ((this._seenPlaying.getBoolValue()) &&
        (core.getPaused() == false))
      core.pause();
    else
      core.play();
    return true;
  },
  
  /**
   * See sbIPlaylistPlayback.idl
   */
  playRef: function(source_ref, index) {
    if (!source_ref || (index == null) || (index < 0))
      throw Components.results.NS_ERROR_INVALID_ARG;
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    // These define what is _actually_ playing
    this._playingRef.setValue( source_ref );
    this._playlistIndex.setValue( index )
    
    // And from those, we ask the Playlistsource to tell us who to play
    var url = this._source.GetRefRowCellByColumn( source_ref, index, "url" );
    var title = this._source.GetRefRowCellByColumn( source_ref, index, "title" );
    var artist = this._source.GetRefRowCellByColumn( source_ref, index, "artist" );
    var album = this._source.GetRefRowCellByColumn( source_ref, index, "album" );
    
    // Set the data remotes to indicate what's about to play
    this._playUrl.setValue(url);
    this._metadataUrl.setValue(url);
    this._metadataTitle.setValue(title);
    this._metadataArtist.setValue(artist);
    this._metadataAlbum.setValue(album);
    
    // Then play it
    this.playUrl(url);
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  playTable: function(dbGUID, table, index) {
    if (!dbGUID || !table || !index)
      throw Components.results.NS_ERROR_INVALID_ARG;
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    // Create a ref different from what playlist.xml uses as a ref.
    var theRef = "sbPlaylistPlayback.js_" + dbGUID + "_" + table;

    // Tell playlistsource to set up that ref to the requested playlist
    this._source.FeedPlaylist( theRef, dbGUID, table );
    this._source.FeedFilters( theRef );
    
    // Synchronous call!  Woo hoo!
    while( this._source.IsQueryExecuting( theRef ) )
      ;
      
    // After the call is done, force GetTargets
    this._source.ForceGetTargets( theRef );
    
    // And then use the source to play that ref.
    return this.playRef( theRef, index );
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
        
      this._playUrl.setValue(url);
      this._metadataUrl.setValue(url);
      
      LOG("playUrl: " + url);
      
      core.stop();
      core.playUrl( url );
      
      LOG( "Playing '" + core.getId() + "'(" + this.getLength() + "/" + this.getPosition() + ") - playing: " + this.getPlaying() + " paused: " + this.getPaused() );
      
      // Start the polling loop to feed the metadata dataremotes.
      this._startPlayerLoop();
      
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
    return 0;
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  previous: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
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
  getVolume: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return core.getVolume();
  },

  /**
   * See sbIPlaylistPlayback.idl
   */
  setVolume: function(volume) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    core.setVolume(volume);
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


    this._db.ResetQuery();
    this._db.AddQuery(query);
    this._db.Execute();
    this._db.WaitForCompletion();
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
        const PlaylistReaderManager = new Components.Constructor("@songbird.org/Songbird/PlaylistReaderManager;1", "sbIPlaylistReaderManager");
        var aPlaylistReaderManager = (new PlaylistReaderManager()).QueryInterface(Components.interfaces.sbIPlaylistReaderManager);
        
        // Tell it what filters to be using
        var filterlist = "";
        var extensionCount = new Object;
        var extensions = aPlaylistReaderManager.SupportedFileExtensions(extensionCount);

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
    this._seenPlaying.setBoolValue(false);
    this._lookForPlayingCount = 0;
    this._timer.initWithCallback( this, 250, 1 ) // TYPE_REPEATING_SLACK
  },
  
  _stopPlayerLoop: function () {
    this._timer.cancel();
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
        
      // Don't update the data if we're currently tracking the seekbar.
      if ( ! this._seekbarTracker.getBoolValue() ) {
        this._metadataLen.setValue( len );
        this._metadataPos.setValue( pos );
      }
      
      // Ignore metadata when paused.
      if ( core.getPlaying() && ! core.getPaused() ) {
        this._onPollMetadata( len, pos, core );
      }
      
      // Call all the wacky poll functions!  yeehaw!
      this._onPollTimeText( len, pos );
      this._onPollVolume( core );
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
    this._metadataPosText.setValue( EmitSecondsToTimeString( pos / 1000.0 ) + " " );

    if ( len > 0 && this._showRemaining.getBoolValue() )
      this._metadataLenText.setValue( "-" + EmitSecondsToTimeString( ( len - pos ) / 1000.0 ) );
    else
      this._metadataLenText.setValue( " " + EmitSecondsToTimeString( len / 1000.0 ) );
  },


  // Routes volume changes from the core to the volume ui data remote 
  // (faceplate sets "faceplate.volume.tracker" to true for bypass while user is changing the volume)
  
  _onPollVolume: function ( core ) {
    if ( ! this._trackerVolume.getBoolValue() ) {
      var v = core.getVolume();
      if ( v != this._volumeLastReadback.getIntValue())  {
        this._volumeLastReadback.setValue( -1 );
        this._volume.setValue(core.getVolume());
      }
    }
  },
  

  // Routes core playback status changes to the play/pause button ui data remote

  _onPollButtons: function ( len, core ) {
    if ( core.getPlaying() && ( ! core.getPaused() ) && ( len > 0 ) )
      this._playButton.setValue( 0 );
    else
      this._playButton.setValue( 1 );
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
      
      if ( length == "Error" )
        return;
      
      var set_metadata = false;
      if ( title.length && ( this._metadataTitle.getValue() != title ) ) {
        this._metadataTitle.setValue( title );
        set_metadata = true; 
      }
/*
      if ( set_metadata ) {
        // Set the metadata into the database table
        SetURLMetadata( this._playUrl.getValue(), title, length, album, artist, genre, true );
        // Tell the search popup the metadata has changed
        this._resetSearchData.setValue( this._resetSearchData.getIntValue() + 1 );
      }
*/      
      this._metadataArtist.setValue( artist );
      this._metadataAlbum.setValue( album );
    }
  },

  _onPollCompleted: function ( len, pos, core )
  {
    // Basically, this logic means that posLoop will run until the player first says it is playing, and then stops.
    if ( core.getPlaying() && ( len > 0.0 ) ) {
      // First time we see it playing, 
      if ( ! this._seenPlaying.getBoolValue() ) {
        // Clear the search popup
        this._resetSearchData.setValue( this._resetSearchData.getIntValue() + 1 );
        this._metadataPollCount = 0; // start the count again.
      }
      // Then remember we saw it
      this._seenPlaying.setBoolValue(true);
    }
    // If we haven't seen ourselves playing, yet, we couldn't have stopped.
    else if ( this._seenPlaying.getBoolValue() ) {
      // Oh, NOW you say we've stopped, eh?
      this._seenPlaying.setBoolValue(false);
      this._stopPlayerLoop();
                                                         // GRRRRRR!
      var cur_index = this._playlistIndex.getIntValue(); // this._findCurrentIndex;
      var cur_ref = this._playingRef.getValue();
      this._playlistIndex.setValue( cur_index );

      LOG( "current index: " + cur_index );
      
      if ( cur_index > -1 ) {
        // Play the next playlist entry tree index (or whatever, based upon state.)
        var num_items = this._source.GetRefRowCount( cur_ref );
        LOG( num_items + " items in the current playlist" );
        var next_index = -1;
        // Are we confused?
        if ( cur_index != -1 ) {
          // Are we REPEAT ONE?
          if ( this._repeat.getIntValue() == 1 ) {
            next_index = cur_index;
          }
          // Are we SHUFFLE?
          else if ( this._shuffle.getBoolValue() ) {
            var rand = num_items * Math.random();
            next_index = Math.floor( rand );
            LOG( "shuffle: " + next_index );
          }
          else {
            // Increment
            next_index = parseInt( cur_index ) + 1;
            LOG( "increment: " + next_index );
            // Are we at the end?
            if ( next_index >= num_items ) 
              // Are we REPEAT ALL?
              if ( this._repeat.getIntValue() == 2 )
                next_index = 0; // Start over
              else
                next_index = -1; // Give up
          }
        }
        
        // If we think we want to play a track, do so.
        LOG( "next index: " + next_index );
        if ( next_index != -1 ) 
          this.playRef( cur_ref, next_index );
          
        // "FIXME" -- mig sez: I think the reason it was broke is because you
        // basically made it restart on playLIST end.  And that would get confusing,
        // I assume.
        if (this._restartOnPlaybackEnd.getBoolValue()) 
          restartApp();
      }
    }
    else {
      // After 10 seconds or fatal error, give up and go to the next one?
      if ( ( this._lookForPlayingCount++ > 40 ) || ( len < -1 ) )
        this.next();
    }
    
  },
  
  _findCurrentIndex: function () {
    var retval = -1;
    var ref = this._playingRef.GetValue();
    if ( this._playingRef.GetValue().length > 0 ) {
      retval = this._source.GetRefRowByColumnValue( ref, "url", playerControls_playURL.GetValue() );
    }
    return retval;
  },
  
/*  
  // Updates the database with metadata changes

  function SetURLMetadata( aURL, aTitle, aLength, aAlbum, aArtist, aGenre, boolSync, aDBQuery, execute )
  {
    if ( aDBQuery == null )
    {
      aDBQuery = new sbIDatabaseQuery();
      aDBQuery.SetAsyncQuery(true);
      aDBQuery.SetDatabaseGUID("songbird");
    }
    if ( execute == null )
    {
      execute = true;
    }
      
    if ( aTitle && aTitle.length )
    {
      var q = 'update library set title="'   + aTitle  + '" where url="' + aURL + '"';
      aDBQuery.AddQuery( q );
    }
    if ( aLength && aLength.length )
    {
      var q = 'update library set length="'    + aLength + '" where url="' + aURL + '"';
      aDBQuery.AddQuery( q );
    }
    if ( aAlbum && aAlbum.length )
    {
      var q = 'update library set album="'  + aAlbum  + '" where url="' + aURL + '"';
      aDBQuery.AddQuery( q );
    }
    if ( aArtist && aArtist.length )
    {
      var q = 'update library set artist="' + aArtist + '" where url="' + aURL + '"';
      aDBQuery.AddQuery( q );
    }
    if ( aGenre && aGenre.length )
    {
      var q = 'update library set genre="'  + aGenre  + '" where url="' + aURL + '"';
      aDBQuery.AddQuery( q );
    }
    
    if ( execute )
    {
      var ret = aDBQuery.Execute();
    }
      
    return aDBQuery; // So whomever calls this can keep track if they want.
  }
  
*/

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