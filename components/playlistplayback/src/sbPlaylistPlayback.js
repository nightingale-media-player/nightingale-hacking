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

Components.utils.import("resource://app/components/sbProperties.jsm");

/**
 * ----------------------------------------------------------------------------
 * Constants
 * ----------------------------------------------------------------------------
 */
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

// XPCOM Registration
const SONGBIRD_PLAYLISTPLAYBACK_CONTRACTID = "@songbirdnest.com/Songbird/PlaylistPlayback;1";
const SONGBIRD_PLAYLISTPLAYBACK_CLASSNAME = "Songbird Playlist Playback Interface";
const SONGBIRD_PLAYLISTPLAYBACK_CID = Components.ID("{e1c06940-077d-4b66-b70d-71a720320e3a}");

// Songbird ContractID Stuff
const SONGBIRD_COREWRAPPER_CONTRACTID = "@songbirdnest.com/Songbird/CoreWrapper;1";
const SONGBIRD_DATAREMOTE_CONTRACTID = "@songbirdnest.com/Songbird/DataRemote;1";
const SONGBIRD_PLAYLISTREADERMANAGER_CONTRACTID = "@songbirdnest.com/Songbird/PlaylistReaderManager;1";

// String Bundles
const URI_SONGBIRD_PROPERTIES = "chrome://songbird/locale/songbird.properties";

// Other junk
const MINIMUM_FILE_SIZE = 64000;

// Interfaces
const sbICoreWrapper           = Components.interfaces.sbICoreWrapper;
const sbIDataRemote            = Components.interfaces.sbIDataRemote;
const sbIPlaylistPlayback      = Components.interfaces.sbIPlaylistPlayback;
const sbIPlaylistReaderManager = Components.interfaces.sbIPlaylistReaderManager;
const sbIMediaListView         = Components.interfaces.sbIMediaListView;

const DEBUG = false;

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
  if (DEBUG) {
    debug("***sbPlaylistPlayback*** " + string + "\n");
    if (gConsole)
      gConsole.logStringMessage(string);
  }
} // LOG

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
 * Makes a new URI from a url string
 */
function newURI(aURLString)
{
  // Must be a string here
  if (!(aURLString &&
       (aURLString instanceof String) || typeof(aURLString) == "string"))
    throw Components.results.NS_ERROR_INVALID_ARG;
  
  var ioService =
    Components.classes["@mozilla.org/network/io-service;1"]
    .getService(Components.interfaces.nsIIOService);
  
  try {
    return ioService.newURI(aURLString, null, null);
  }
  catch (e) { }
  
  return null;
}

/**
 * Returns the extension (without '.') from a URI object
 */
function getFileExtensionFromURI(aURI)
{
  if (!aURI.QueryInterface(Components.interfaces.nsIURI))
     throw Components.results.NS_ERROR_INVALID_ARG;
  
  var extension = null;
  try {
    var url = aURI.QueryInterface(Components.interfaces.nsIURL);
    extension = url.fileExtension;
  }
  catch (e) {
    var spec = aURI.spec;
    var result = spec.match(/\.([^\.\s]+)$/);
    if (result)
      extension = result[1];
  }
  return extension;
}

/**
 * Determines whether or not the given core supports the given extension.
 */
function coreSupportsExtension(aCore, aExtension)
{
  if (!aCore.QueryInterface(sbICoreWrapper))
    throw Components.results.NS_ERROR_INVALID_ARG;
  if (!((aExtension instanceof String) || (typeof(aExtension) == "string")))
    throw Components.results.NS_ERROR_INVALID_ARG;

  var extensionsEnum = aCore.getSupportedFileExtensions();
  while (extensionsEnum.hasMore()) {
    var supportedExtension = extensionsEnum.getNext();
    LOG("coreSupportsExtension (" + aCore.getId() + ") - " + supportedExtension);
    if (aExtension.toLowerCase() == supportedExtension.toLowerCase())
      return true;
  }
  return false;
}

/**
 * A wrapper to turn a JS array into an nsIStringEnumerator.
 */
function StringArrayEnumerator(aArray) {
  this._array = aArray;
  this._current = 0;
}

StringArrayEnumerator.prototype = {

  constructor: StringArrayEnumerator,
  
  _array: null,
  _current: 0,

  /**
   * See nsIStringEnumerator.idl
   */
  hasMore: function hasMore() {
    return this._current < this._array.length;
  },
  
  /**
   * See nsIStringEnumerator.idl
   */
  getNext: function getNext() {
    return this._array[this._current++];
  },

  /**
   * See nsISupports.idl
   */
  QueryInterface: function QueryInterface(iid) {
    if (!iid.equals(Components.interfaces.nsIStringEnumerator) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};

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
    gOS.addObserver(this, "profile-before-change", false);
  }

  var jsLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"]
                 .getService(Components.interfaces.mozIJSSubScriptLoader);
  jsLoader.loadSubScript("chrome://songbird/content/scripts/metrics.js", this);
  

} // PlaylistPlayback
PlaylistPlayback.prototype.constructor = PlaylistPlayback;

/**
 * Prototype
 */
PlaylistPlayback.prototype = {
  classDescription: SONGBIRD_PLAYLISTPLAYBACK_CLASSNAME,
  classID:          SONGBIRD_PLAYLISTPLAYBACK_CID,
  contractID:       SONGBIRD_PLAYLISTPLAYBACK_CONTRACTID,

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
  
  /**
   * Used to not hit the playback core for metadata too often.
   */
  _metadataPollCount: 0,
  
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
  _playURL:            null,
  _playing:            null,
  _paused:             null,
  _seenPlaying:        null,
  _playingVideo:       null,
  _lastVolume:         null,
  _lastPos:            null,
  _volume:             null,
  _muteData:           null,
  _playlistRef:        null,
  _repeat:             null,
  _shuffle:            null,
  _showRemaining:      null,
  _metadataTitle:      null,
  _metadataArtist:     null,
  _metadataAlbum:      null,
  _metadataGenre:      null,
  _metadataURL:        null,
  _metadataPos:        null,
  _metadataLen:        null,
  _metadataPosText:    null,
  _metadataLenText:    null,
  _statusText:         null,
  _statusStyle:        null,
  
  /**
   * For tracking change of playback control - used as event broadcaster
   */
  _controlTriggered:    null,
    
  _restartOnPlaybackEnd: null,
  _restartAppNow:        null,
  
  _requestedVolume:      -1,
  _calculatedVolume:     -1,

  _started:           false,
  _set_metadata:      false,
  _stopNextLoop:      false,
  _playlistReaderManager: null,

  
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
      // Attach all the sbDataRemote objects (via XPCOM!)
      this._attachDataRemotes();
      LOG("Songbird PlaylistPlayback Service loaded successfully");
    } catch( err ) {
      LOG( "!!! ERROR: sbPlaylistPlayback _init\n\n" + err + "\n" );
    }
  },
  
  _deinit: function() {
    this._releaseDataRemotes();
    
    var coreCount = this._cores.length;
    for (var index = 0; index < coreCount; index++) {
      var core = this._cores[0];
      this.removeCore(core);
    }
    
    LOG("Songbird PlaylistPlayback Service unloaded successfully");
  },

  _attachDataRemotes: function() {
    LOG("Attaching DataRemote objects");
    // This will create the component and call init with the args
    var createDataRemote = new Components.Constructor( SONGBIRD_DATAREMOTE_CONTRACTID, sbIDataRemote, "init");

    // HOLY SMOKES we use lots of data elements.
  
    // string current           
    this._playURL               = createDataRemote("faceplate.play.url", null);
    // whether the core is playing or not (if the core is paused, it is *also* playing, this remote indicates that the core is not stopped)
    this._playing               = createDataRemote("faceplate.playing", null);
    // whether the paused or not (if it is paused, it is also playing). 
    this._paused                = createDataRemote("faceplate.paused", null);
    
    // note:
    // an autoswitching play/pause button should decide upon its state in one of two ways :
    // show_play_button = !playing || paused; show_pause_button = !show_play_button;
    // or
    // show_pause_button = playing && !paused; show_play_button = !show_pause_button;
    
    // whether we are actually playing
    this._seenPlaying           = createDataRemote("faceplate.seenplaying", null);
    // whether we are playing a video file or an audio file
    this._playingVideo          = createDataRemote("faceplate.playingvideo", null);
    // current volume
    this._volume                = createDataRemote("faceplate.volume", null);
    //t/f                      
    this._muteData              = createDataRemote("faceplate.mute", null);
    this._playingRef            = createDataRemote("playing.ref", null);
    this._playlistRef           = createDataRemote("playlist.ref", null);
    this._playlistIndex         = createDataRemote("playlist.index", null);
    this._playingView           = null;
    this._playingViewItemUID    = null;
    this._repeat                = createDataRemote("playlist.repeat", null);
    this._shuffle               = createDataRemote("playlist.shuffle", null);
    this._showRemaining         = createDataRemote("faceplate.showremainingtime", null);
                               
    this._metadataTitle         = createDataRemote("metadata.title", null);
    this._metadataArtist        = createDataRemote("metadata.artist", null);
    this._metadataAlbum         = createDataRemote("metadata.album", null);
    this._metadataGenre         = createDataRemote("metadata.genre", null);
    this._metadataURL           = createDataRemote("metadata.url", null);
    this._metadataPos           = createDataRemote("metadata.position", null);
    this._metadataLen           = createDataRemote("metadata.length", null);
    this._metadataPosText       = createDataRemote("metadata.position.str", null);
    this._metadataLenText       = createDataRemote("metadata.length.str", null);
    this._restartOnPlaybackEnd  = createDataRemote("restart.onplaybackend", null);
    this._restartAppNow         = createDataRemote("restart.restartnow", null);

    this._statusText            = createDataRemote("faceplate.status.text", null );
    this._statusStyle           = createDataRemote("faceplate.status.style", null );

    // track the last time playback controls were used
    this._controlTriggered       = createDataRemote("playback.control.triggered", null);

    // Set startup defaults
    this._metadataPos.intValue = 0;
    this._metadataLen.intValue = 0;
    this._playing.boolValue = false;
    this._paused.boolValue = false;
    this._seenPlaying.boolValue = false;
    this._playingVideo.boolValue = false;
    this._metadataPosText.stringValue = "0:00";
    this._metadataLenText.stringValue = "0:00";
    this._showRemaining.boolValue = false;
    //this._muteData.boolValue = false;
    this._playlistRef.stringValue = "";
    this._playlistIndex.intValue = -1;
    this._restartOnPlaybackEnd.boolValue = false;
    this._restartAppNow.boolValue = false;
    this._metadataURL.stringValue = "";
    this._metadataTitle.stringValue = "";
    this._metadataArtist.stringValue = "";
    this._metadataAlbum.stringValue = "";
    this._metadataGenre.stringValue = "";
    this._statusText.stringValue = "";
    this._statusStyle.stringValue = "";
    this._playingRef.stringValue = "";
    this._playURL.stringValue = ""; 

    // if they have not been set they will be the empty string.
    if ( this._shuffle.stringValue == "")
      this._shuffle.boolValue = false; // start with no shuffle
    if ( this._repeat.stringValue == "" )
      this._repeat.intValue = sbIPlaylistPlayback.REPEAT_MODE_OFF; // start with no repeat
    if ( this._volume.stringValue == "" )
      this._volume.intValue = 128;
    this._requestedVolume = this._calculatedVolume = this._volume.intValue;
  },
  
  _releaseDataRemotes: function() {
    // And we have to let them go when we are done else all hell breaks loose.
    this._playURL.unbind();
    this._playing.unbind();
    this._seenPlaying.unbind();
    this._playingVideo.unbind();
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
    this._metadataGenre.unbind();
    this._metadataURL.unbind();
    this._metadataPos.unbind();
    this._metadataLen.unbind();
    this._metadataPosText.unbind();
    this._metadataLenText.unbind();
    this._restartOnPlaybackEnd.unbind();
    this._statusText.unbind();
    this._statusStyle.unbind();
  },
  
  /**
   * Tries to pick a valid core if one is removed.
   * Right now it just picks the first in the list (DUMB)
   */
  _chooseNewCore : function() {
    var newIndex = this._cores.length > 0 ? 0 : -1;
    this._swapCore(newIndex);
  },


  // --------------------------------------------------------------------------
  //  Public attribute (property) getters and setters
  // --------------------------------------------------------------------------

  get core() {
    if ((this._cores.length > 0) &&
        (this._currentCoreIndex > -1) &&
        (this._currentCoreIndex <= this._cores.length - 1)) {
      return this._cores[this._currentCoreIndex];
    }
    else {
      return null;
    }
  },

  set core(val) {
    LOG("set core: core.getId = " + val.id);
    this.selectCore(val);
  },

  get volume() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    var retval = core.getVolume();
    // hand back the requestedVolume if we have not changed to work around
    //   rounding vagaries from the core implementation - the core may
    //   internally track volume on a different scale.
    if ( retval == this._calculatedVolume ) {
      retval = this._requestedVolume;
    }
    return retval;
  },

  set volume(val) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    // after setting the value in the core, ask for the the value and store
    //   it so we can verify in the getter. Some cores may use a different
    //   scale for volume and we do not want rounding to change the volume.
    this._callMethodOnAllCores("setVolume", [val]);
    this._requestedVolume = val;
    this._calculatedVolume = core.getVolume();
    this._onPollVolume();
    return false;
  },
  
  get mute() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return core.getMute();
  },

  set mute(val) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    this._callMethodOnAllCores("setMute", [val]);
    // some cores set their volume to 0 on setMute(true), but do not restore
    // the volume when mute is turned off, this fixes the problem
    if (val == false) {
      this._callMethodOnAllCores("setVolume", [this._calculatedVolume]);
    }
    // if the core is not playing, the loop is not running, but we still want
    //     the new mute state (and possibly volume) to be routed to all the
    //     UI controls
    this._onPollMute(core);
    return true;
  },
  
  get length() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return core.getLength();
  },
  
  get position() {
    return this.core.getPosition();
  },

  set position(val) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    core.setPosition(val);
    this._controlTriggered.stringValue = Date.now();
    return true;
  },
  
  get repeat() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return this._repeat;
  },

  set repeat(val) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    this._repeat = val;
    return;
  },
  
  get shuffle() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return this._shuffle;
  },

  set shuffle(val) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    this._shuffle = val;      
    return;
  },
  
  get itemCount() {
    if (this._playingView) return this._playingView.length;
    return 0;
  },
  
  get currentIndex() {
    if (this.playing) return this._playlistIndex.intValue;
    return -1;
  },
  
  get currentGUID() {
    if (this.playing) return this._playingRef.stringValue;
    return null;
  },
  
  get currentURL() {
    return this._playURL.stringValue;
  },
  
  // ---------------------------------------------
  //  Public Methods
  // ---------------------------------------------

  // Add a core to the array. Optionally select it.
  addCore: function addCore(core, select) {
    LOG("addCore: core.id = " + core.getId() + "; select = " + select);
    
    // See if we've already added this core
    var coreCount = this._cores.length;
    for (var i = 0; i < coreCount; i++) {
      var alreadySeenCore = this._cores[i];
      if (alreadySeenCore == core) {
        if (select)
          this._swapCore(i);
        return;
      }
    }
    
    // This is a new core, so test and add it
    if (!core.QueryInterface(sbICoreWrapper))
      throw Components.results.NS_ERROR_NOINTERFACE;
    
    // Set initial data
    core.setMute(this._muteData.boolValue);
    
    var volume = this._volume.intValue;
    if (volume < 0)
      volume = 0;
    else if (volume > 255)
      volume = 255;
    
    core.setVolume(volume);
    
    // Add to the list
    this._cores.push(core);
    
    // Update index
    if (select)
      this._swapCore(coreCount);
  },
  
  removeCore: function removeCore(core) {
    LOG("removeCore with core.getId = " + core.getId());
    
    var coreCount = this._cores.length;
    for (var i = 0; i < coreCount; i++) {
      
      if (this._cores[i] == core) {
        // Remove the core from our list
        this._cores.splice(i, 1);
        
        if (i == this._currentCoreIndex) {
          // If that was our current core then we need a new one...
          this._chooseNewCore();          
        }
        else if (i < this._currentCoreIndex) {
          // Otherwise just decrement our index - no swapping necessary
          this._currentCoreIndex--;          
        }
        
        return;
      }
    }
    
    LOG("core wasn't in the list!");
  },

  selectCore: function selectCore(core) {
    // Cheat and rely on the behavior of the add method
    this.addCore(core, true);
  },
  
  _swapCore: function _swapCore(aNewCoreIndex) {
    
    var coreCount = this._cores.length;
    
    // If we have no cores left then there is nothing to do here.
    if (coreCount == 0)
      return;

    // Make sure the given index is valid before we do anything. -1 means we
    // have no cores left.
    if (aNewCoreIndex < 0 || aNewCoreIndex > coreCount - 1)
      throw Components.results.NS_ERROR_INVALID_ARG;
    
    if (this._currentCoreIndex > -1) {
      // Tell the old core that it is being swapped out
      var oldCore = this._cores[this._currentCoreIndex];
      oldCore.deactivate();
    }
    
    if (aNewCoreIndex > -1) {
      // Try to show the new video element
      var newCore = this._cores[aNewCoreIndex];
      newCore.activate();        
    }

    // Select the new one
    this._currentCoreIndex = aNewCoreIndex;
  },
  
  /**
   * Tries to find a core that can handle the given extension. Selects and
   * returns the core and sets the optional aCoreFound argument to true. If no
   * core is found then previously selected core will be returned and no change
   * in selection will occur. In such a case the aCoreFound variable will be set
   * to false.
   */
  _selectCoreForURI: function _selectCoreForURI(aURI, aCoreFound) {
    var selectedCore = this.core;
    if (!selectedCore)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    
    var extension = getFileExtensionFromURI(aURI);
    // HACK ALERT - some streams do not have file extensions but we need
    //              to pass an arg to comply with XPIDL. see bug 2565.
    if (! extension)
      extension = " ";   
    LOG("_selectCoreForURI: extension = " + extension);
    
    var coreSupport =
      this._callMethodOnAllCores("getSupportForFileExtension", [extension]);
    LOG("_selectCoreForURI: coreSupport = " + coreSupport);
    
    var newCoreIndex = -1;

    // Things vote so later things may outvote and supercede earlier things
    var highVal = -1;
    for ( var i = 0; i < coreSupport.length; i++ ) {
      if ( typeof( coreSupport[ i ] ) != 'undefined' && highVal < coreSupport[ i ] ) {
        highVal = coreSupport[ i ];
        newCoreIndex = i;
      }
    }
    
    LOG("_selectCoreForURI: newCoreIndex = " + newCoreIndex);

    // Set the error flag if we didn't find any cores that support the
    // extension.
    if (arguments.length > 1)
      aCoreFound = (newCoreIndex > -1);

    // Use our current core if we didn't find any cores that support the
    // extension. This is kinda silly and we may want to do something smarter.
    if (newCoreIndex == -1)
      newCoreIndex = this._currentCoreIndex;
    
    // Select the new core.
    var newCore = this._cores[newCoreIndex];
    this.selectCore(newCore);
    return newCore;
  },
  
  /**
   * Use this function to call the same method on all cores. Useful for setting
   * things like volume.
   *
   * \param aMethodName The name of the method to call on each core
   * \param aArgArray An array of arguments to pass to the method
   *
   * \return Returns an array of the return values from the method on each core
   */
  _callMethodOnAllCores: function _callMethodOnAllCores(aMethodName, aArgArray) {
    var returnVals = [];
    var coreCount = this._cores.length;

    //At this point there should always have one media core registered.
    if (coreCount == 0) {
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    }
    
    for (var index = 0; index < coreCount; index++) {
      var retval = null;
      var core = this._cores[index];
      var method = core[aMethodName];
      LOG("_callMethodOnAllCores: " + aMethodName + "(" + aArgArray + ")");
      try {
        retval = method.apply(core, aArgArray); // Ben likes to throw.
      } catch (e) {}
      returnVals[index] = retval;
    }
    return returnVals;    
  },
  
  play: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    // If we know what to play, just play it
    if (this._started ||
               (this._playingRef.stringValue != "" &&
                this._playlistIndex.stringValue != "")) 
    {
      // If paused, then continue where we left off.
      // Otherwise start the last played song.
      if (core.getPaused()) {
        core.play()
      } else {
        this.playView( this._playingView, this._playlistIndex.intValue );
      }
    }
    // Otherwise figure out the default action
    else {
      // play the current track, or the libary from position 0
      this._playDefault();
    }

    return true;
  },
  
  playView: function(aView, aIndex) {
    // XXXnewlib
    if (!(aView && aIndex >= 0))
      throw Components.results.NS_ERROR_INVALID_ARG;
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    this._playingView = aView;

    if (aIndex < 0) {
      aIndex = 0;
      // See if we should shuffle on it
      if ( this._shuffle.boolValue ) {
        var num_items = this._playingView.length;
        var rand = num_items * Math.random();
        aIndex = Math.floor( rand );
      }
    }

    // pull metadata and filters from aSourceRef
    this._updateCurrentInfoFromView(aView, aIndex);

    // Remember this item's view UID so we can find it if the view changes
    this._playingViewItemUID = aView.getViewItemUIDForIndex(aIndex);

    // Then play it
    var retval = this.playURL(this._playURL.stringValue);
 
    return retval;
  },

  playURL: function(aURL) {
    try  {
      var core = this.core;
      if (!core)
        throw Components.results.NS_ERROR_NOT_INITIALIZED;

      this._playURL.stringValue = "";
      this._metadataURL.stringValue = "";

      this.stop();
      
      var uri = newURI(aURL);
      
      var file;
      try {
        file = uri.QueryInterface(Components.interfaces.nsIFileURL).file;
      }
      catch (err) { }
      
      // See if this is a local file and do a basic integrity check if it is.
      if (file) {
        if (!file.exists())
          throw Components.results.NS_ERROR_FILE_NOT_FOUND;
        if (!file.isReadable())
          throw Components.results.NS_ERROR_FILE_ACCESS_DENIED;
        // If the size is too small we assume that the file is corrupted.
        if (file.fileSize < MINIMUM_FILE_SIZE)
          throw Components.results.NS_ERROR_FILE_CORRUPTED;
      }

      var coreCanPlayURI;
      core = this._selectCoreForURI(uri, coreCanPlayURI);

      var spec = uri.spec;
      
      this._metadataURL.stringValue = spec;
      this._playURL.stringValue = spec;
      this._controlTriggered.stringValue = Date.now();
      
      try {
        core.playURL(spec);      
      }
      catch(e) {
        // XXXAus: SB_EXTERNAL_VIDEO_LAUNCH.
        if(e.result == 0x80780001) {
          return true;
        }
      }

      LOG( "playURL() '" + core.getId() + "'(" + this.position + "/" +
           this.length + ") - playing: " + this.playing +
           " paused: " + this.paused
         );
      
      // Start the polling loop to feed the metadata dataremotes.
      this._startPlayerLoop();
      
      // metrics
      var ext = spec.substr( spec.lastIndexOf(".") + 1, spec.length );
      if ( ext.length < 1 || ext.length > 6 )
        ext = "???";  // Well, we certainly tried to play something.
      var protocol = spec.substr(0, spec.indexOf(":"));
      if (protocol.length < 1) 
        protocol = "???"
      this.metrics_inc("mediacore.play.attempt", core.getId(), ext + "." + protocol);
    } catch( err ) {
      debug( "playURL:\n" + err + "\n" );
      return false;
    }
    return true;
  },
  
  pause: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    core.pause();
    this._controlTriggered.stringValue = Date.now();
    return true;
  },

  stop: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
      
//    if ( ! this.playing )
//      return;
      
    // Ask the core very nicely to please stop.  Won't happen immediately.
    core.stop();
    
    // Wait a second or two to see if we see ourselves stop.
    var start = new Date().getTime();
    while ( this.playing && ( new Date().getTime() - start < 2000 ) )
      this._sleep( 100 );
    
    // Even if it fails here, it should be okay on the next loop around the block
    this._stopNextLoop = true;
    if (this.playing)
      dump("sbPlaylistPlayback::stop() - WHOA.  Playback core didn't actually stop when we asked it!\n");
/*
// Something in the _onPlayerLoop() kills us here.
// It looks like this is okay to ignore and just use the _stopNextLoop flag.
    else {
      // Call the loop immediately, here, so we clean out and shut the loop down.
      this._onPlayerLoop();
      this._stopPlayerLoop();
      this._stopNextLoop = false; // If we make it here, we don't need this
    }
*/    
    this._playingVideo.boolValue = false;
    this._controlTriggered.stringValue = Date.now();
    return true;
  },

  next: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    this._playNextPrev(1);
    this._controlTriggered.stringValue = Date.now();
    return this._playlistIndex;
  },

  previous: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    this._playNextPrev(-1);
    this._controlTriggered.stringValue = Date.now();
    return this._playlistIndex;
  },

  current: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return this._playlistIndex;
  },

  get paused() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return core.getPaused();
  },

  get playing() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return core.getPlaying();
  },

  get started() {
    return this._started;
  },
  
  get playingView() {
    return this._playingView;
  },

  goFullscreen: function() {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    core.goFullscreen();
  },
  
  getMetadataFields: function(/*out*/ fieldCount, /*out*/ metaFields) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    fieldCount = 0;
    metaFields = new Array();
    return;
  },

  getCurrentValue: function(field) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return "";
  },

  setCurrentValue: function(field, value) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return;
  },

  getCurrentValues: function(fieldCount, metaFields, /*out*/ valueCount, /*out*/ metaValues) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    valueCount = 0;
    metaValues = new Array();
    return;
  },

  setCurrentValues: function(fieldCount, metaFields, valueCount, metaValues) {
    var core = this.core;
    if (!core)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return;
  },

  isMediaURL: function(aURL) {
    var coreCount = this._cores.length;
    
    //At this point there should always have one media core registered.
    if (coreCount == 0) {
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    }
    
    for (var index = 0; index < coreCount; index++) {
      var core = this._cores[index];
      if (core.isMediaURL(aURL.toLowerCase()))
        return true;
    }
    return false;
  },

  isVideoURL: function (aURL) {
    var coreCount = this._cores.length;
    
    //At this point there should always have one media core registered.
    if (coreCount == 0) {
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    }

    for (var index = 0; index < coreCount; index++) {
      var core = this._cores[index];
      if (core.isVideoURL(aURL.toLowerCase()))
        return true;
    }
    return false;
  },

  isPlaylistURL: function(aURL) {
    aURL = aURL.toLowerCase();
    if( ( aURL.indexOf ) && 
        (
          // For now, still hardcode the playlist types.
          ( aURL.indexOf( ".pls" ) != -1 ) || 
          ( aURL.indexOf( "rss" ) != -1 ) || 
          ( aURL.indexOf( ".m3u" ) != -1 ) 
        )
      )
    {
      return true;
    }
    return false;

/*
    // GRRRRR.  USE THIS LATER.  ALL HTML IS "TRUE" FOR THIS.  
    if ( aURL.indexOf )
    {
      // Cache our manager, but not at construction!
      if ( this._playlistReaderManager == null )
        this._playlistReaderManager = Components.classes[SONGBIRD_PLAYLISTREADERMANAGER_CONTRACTID]
                                      .createInstance(sbIPlaylistReaderManager);
    
      // Tell it what filters to be using
      var filterlist = "";
      var extensionCount = new Object;
      var extensions = this._playlistReaderManager.supportedFileExtensions(extensionCount);

      // Cycle over the list of supported extensions looking for a match in the url
      for(var i = 0; i < extensions.length; i++)
      {
        var ext_list = extensions[i].split(",");
        for(var j = 0; j < ext_list.length; j++)
        {
          var ext = ext_list[j];
          if ( aURL.indexOf( "." + ext ) != -1 )
          {      
            return true;
          }
        }
      }
    }
    return false;
*/    
  },

  getSupportedFileExtensions: function () {
    var supportedExtensions = [];
    function appendExtensions(aElement, aIndex, aArray) {
      if ( aElement ) {
        while (aElement.hasMore())
          supportedExtensions.push(aElement.getNext());
      }
    }
    
    var enumerators =
      this._callMethodOnAllCores("getSupportedFileExtensions", []);
    enumerators.forEach(appendExtensions);

    return new StringArrayEnumerator(supportedExtensions);
  },

  stripHoursFromTimeString: function ( aTimeString )
  {
    if ( aTimeString == null )
      aTimeString = "";
    var retval = aTimeString;
    if ( ( aTimeString.length == 7 ) &&
         ( aTimeString[ 0 ] == "0" ) &&
         ( aTimeString[ 1 ] == ":" ) )
    {
      retval = aTimeString.substring( 2, aTimeString.length );
    }
    return retval;
  },

  emitSecondsToTimeString: function ( aSeconds )
  {
    if ( aSeconds < 0 )
      return "0:00";
    aSeconds = parseFloat( aSeconds );
    var minutes = parseInt( aSeconds / 60 );
    aSeconds = parseInt( aSeconds ) % 60;
    var hours = parseInt( minutes / 60 );
    if ( hours > 50 ) // lame
      return "Error";
    minutes = parseInt( minutes ) % 60;
    var text = ""
    if ( hours > 0 )
      text += hours + ":";
    if ( hours > 0 && minutes < 10 )
      text += "0";
    text += minutes + ":"; // always lead the minutes 0
    if ( aSeconds < 10 ) // always lead the seconds 0
      text += "0";
    text += aSeconds;
    return text;
  },
  
  convertURLToDisplayName: function( aURL )
  {
    var urlDisplay = "";
    
    try {
      urlDisplay = decodeURI( aURL );
    } catch(err) {
      dump("convertURLToDisplayName, oops! URI decode weirdness: " + err + "\n");
    }
    
    // Set the title display  
    if ( urlDisplay.lastIndexOf('/') != -1 )
    {
      urlDisplay = urlDisplay.substring( urlDisplay.lastIndexOf('/') + 1, urlDisplay.length );
    }
    else if ( aURL.lastIndexOf('\\') != -1 )
    {
      urlDisplay = aURL.substring( aURL.lastIndexOf('\\') + 1, aURL.length );
    }

    if ( ! urlDisplay.length )
    {
      urlDisplay = aURL;
    }
    
    return urlDisplay;
  },

  convertURLToFolder: function( aURL )
  {
    // Set the title display  
    aURL = decodeURI( aURL );
    var urlDisplay = "";
    if ( aURL.lastIndexOf('/') != -1 )
    {
      urlDisplay = aURL.substring( 0, aURL.lastIndexOf('/') );
    }
    else if ( aURL.lastIndexOf('\\') != -1 )
    {
      urlDisplay = aURL.substring( 0, aURL.lastIndexOf('\\') );
    }
    else
    {
      urlDisplay = aURL;
    }
    return urlDisplay;
  },
 
  // watch for XRE startup and shutdown messages 
  observe: function(subject, topic, data) {
    switch (topic) {
    case "profile-after-change":
      gOS.removeObserver(this, "profile-after-change");
      
      // Preferences are initialized, ready to start the service
      this._init();
      break;
    case "profile-before-change":
      gOS.removeObserver(this, "profile-before-change");
      this._deinit();
      
      // Release Services to avoid memory leaks
      gConsole  = null;
      gOS       = null;
      break;
    }
  },
  
  // --------------------------------------------------------------------------
  // Below here are local items used internally to do player loop handling.
  // --------------------------------------------------------------------------

  notify: function( timer ) { // nsITimerCallback
    this._onPlayerLoop();
  },
  
  _startPlayerLoop: function () {
    this._stopPlayerLoop();
    this._started = true;
    this._playing.boolValue = false;
    this._stopNextLoop = false;
    this._lookForPlayingCount = 0;
    this._timer = Components.classes[ "@mozilla.org/timer;1" ]
                  .createInstance( Components.interfaces.nsITimer );
    this._timer.initWithCallback( this, 250, 1 ) // TYPE_REPEATING_SLACK
  },
  
  _stopPlayerLoop: function () {
    if (this._timer) {
      this._timer.cancel();
      this._timer = null;
    }
    this._started = false;
  },
  
  // Poll function
  _onPlayerLoop: function () {
    try {
      var core = this.core;
      if (!core)
        throw Components.results.NS_ERROR_NOT_INITIALIZED;

      var len = this.length;
      var pos = this.position;
      
      if ( !this._once ) {
        LOG( "_onPlayerLoop '" + core.getId() +
             "'(" + this.position + "/" + this.length +
             ") - playing: " + this.playing +
             " paused: " + this.paused +
             " ref: " + this._playingRef + "\n" );
        this._once = true;
      }

      // If we miscalculated the length of the track too short, string it out.        
      if ( pos > len && len > 0 )
        len = pos;
      // When the length changes, always set metadata.
      if ((len > 0) &&
          (Math.round(this._metadataLen.intValue / 1000) != Math.round(len / 1000)))
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
      this._onPollStates( len, pos, core );
      this._onPollCompleted( len, pos, core );
      this._onPollVideo( core );
    }       
    catch ( err )  
    {
      dump( "!!! ERROR: sbPlaylistPlayback _onPlayerLoop\n\n" + err + "\n" );
    }
  },
  

  // Elapsed / Total display
 
  _onPollTimeText: function ( len, pos ) {
    this._metadataPosText.stringValue = this.emitSecondsToTimeString( pos / 1000.0 ) + " ";

    if ( len > 0 && this._showRemaining.boolValue )
      this._metadataLenText.stringValue = "-" + this.emitSecondsToTimeString( ( len - pos ) / 1000.0 );
    else
      this._metadataLenText.stringValue = " " + this.emitSecondsToTimeString( len / 1000.0 );
  },


  // Routes volume changes from the core to the volume ui data remote 
  _onPollVolume: function ( core ) {
      // do not ask the core directly for volume. special processing required!
      var v = this.volume;
      if ( v != this._volume.intValue ) {
        this._volume.intValue = v;
    }
  },

  // Routes mute changes to the mute data remote
  _onPollMute: function ( core ) {
      var mute = core.getMute();
      if ( mute != this._muteData.boolValue ) {
        this._muteData.boolValue = mute;
      }
  },

  // Routes core playback status changes to the playing/paused ui data remote
  _onPollStates: function ( len, pos, core ) {
    this._playing.boolValue = core.getPlaying();
    this._paused.boolValue = core.getPaused();
  },

  // Routes metadata (and their possible updates) to the metadata ui data remotes
  // Also updates the database if the core reads metadata that is different from
  // what we had so far
  _onPollMetadata: function ( len_ms, pos_ms, core )
  {
    //XXX lone> this test should probably be reworked, the first time the loop is hit on a track, pos_ms is already > 250,
    // and it's not obvious that we're actually taking advantage of that. Also, the comment that says "we skip 20 times" is
    // misleading, the first time the loop hits, that counter is 0, so we haven't skipped 20 times yet (and yes, we're also
    // taking advantage of that, if we weren't, we'd need to have a _set_metadata_once var or equivalent, to make it so that
    // the first time the metadata needs to be updated, it's done as soon as possible. as is, it always does it the first time
    // the loop is hit, and that suits us, but it's really not clear)

    // Wait a bit, and then only ask infrequently
    if ( 
      ( pos_ms > 250 ) && // If we've gone more than a quarter of a second, AND
      ( 
        ( ( this._metadataPollCount++ % 20 ) == 0 ) /*|| // We skip 20 times, OR
        ( this._set_metadata ) */// Someone says we're supposed to be setting metadata now.
      )
    ) {

      // Sometimes the core is a little slower than we are and it returns
      // metadata for the previous song after we are already playing the next.
      // Make sure we're in sync and bail if not.
      if (core.getMetadata("url") != this._playURL.stringValue)
        return;

      // Ask, and ye shall receive
      var title = "" + core.getMetadata("title");
      var album = "" + core.getMetadata("album");
      var artist = "" + core.getMetadata("artist");
      var genre = "" + core.getMetadata("genre");
      var length = "";
      if ( len_ms >= 0 )
        length = this.emitSecondsToTimeString( len_ms / 1000.0 );

      // Glaaaaaaah!        
      if ( title == "null" ) title = "";
      if ( album == "null" ) album = "";
      if ( artist == "null" ) artist = "";
      if ( genre == "null" ) genre = "";
      
      if ( length == "Error" ) {
        return;
      }

      // If the current title is the default title string (filename), 
      // it is okay to overwrite the title.
      var defaultTitle = unescape(this._playURL.stringValue);
      var lastSlash = defaultTitle.lastIndexOf("/");
      if (lastSlash >= 0)
        defaultTitle = defaultTitle.substr(lastSlash + 1);
      if (( title.length > 0 ) && 
          ( this._metadataTitle.stringValue != title ) &&
          ( ( this._metadataTitle.stringValue == "" ) ||
            ( this._metadataTitle.stringValue == defaultTitle )
          ))
        this._set_metadata = true; 
      else
        title = "";
        
      // Only if we have no known artist (or it is a Stream updating artist)
      if (( artist.length > 0 ) && 
          ( this._metadataArtist.stringValue != artist ) &&
          ( ( this._metadataArtist.stringValue == "" ) ||
            ( this._playURL.stringValue.toLowerCase().indexOf("http:") == 0 )
         ))
        this._set_metadata = true; 
      else
        artist = "";
        
      // Only if we have no known album.
      if ( album.length > 0 && this._metadataAlbum.stringValue == "" )
        this._set_metadata = true; 
      else
        album = "";
        
      // Only if we have no known genre.
      if ( genre.length > 0 && this._metadataGenre.stringValue == "" )
        this._set_metadata = true; 
      else
        genre = "";
        
      if ( this._set_metadata ) {
        //Get current item using current index and current playing view.
        var base   = "http://songbirdnest.com/data/1.0#";
        var cur_index = this._playlistIndex.intValue;
        var cur_item = this._playingView.getItemByIndex(cur_index);
        
        //Set metadata for this item
        this._setItemMetadata( cur_item, title, this._metadataLen.intValue, album, artist, genre );
        this._set_metadata = false;
      }

      // Send it out to the dataremotes
      if (title != "")
        this._metadataTitle.stringValue = title;
      if (artist != "")
        this._metadataArtist.stringValue = artist;
      if (album != "")
        this._metadataAlbum.stringValue = album;
      if (genre != "")
        this._metadataGenre.stringValue = genre;
    }
  },

  _onPollCompleted: function ( len, pos, core ) {
    
    LOG("_onPollCompleted(" + len + ", " + pos + ", " + core);
    
    var isPlaying = core.getPlaying();
    
    LOG("_onPollCompleted - isPlaying? " + isPlaying);
    LOG("_onPollCompleted - stop next loop? " + this._stopNextLoop);
    
    // Basically, this logic means that posLoop will run until the player first says it is playing, and then stops.
    if ( isPlaying && ( this._isFLAC() || len > 0.0 || pos > 0.0 ) ) {
      
      LOG("_onPollCompleted - is playing and len or pos > 0.0");
      
      // First time we see it playing, 
      if ( ! this._seenPlaying.boolValue ) {
        
        LOG("_onPollCompleted - first time seen playing.");
      
        this._metadataPollCount = 0; // start the count again.
        this._lookForPlayingCount = 0;
        this._stopNextLoop = false;
      }
      // OH OH!  If our position isn't moving, go to the next track!
      else if ( pos == this._lastPos && pos > 0.0 && ! this._isFLAC() && ! this.paused ) {
        
        LOG("_onPollCompleted - position is not moving.");
        
        // After 10 seconds, give up and go to the next one?
        if ( this._lookForPlayingCount++ > 80 )
          this.next();
      }
      else {
        
        LOG("_onPollCompleted - reset the counters.");
        
        // Boring, reset the counters.
        this._lastPos = pos;
        this._lookForPlayingCount = 0;
      }      
      // Then remember we saw it
      this._seenPlaying.boolValue = true;
    }
    // If we haven't seen ourselves playing, yet, we couldn't have stopped.
    else if ( (this._seenPlaying.boolValue || ( len < 0.0 )) &&
              ( this._lookForPlayingCount++ > 8 ) ) {
      
      LOG("_onPollCompleted - seen playing (" + this._seenPlaying.boolValue + ") OR len: " + len);
      
      // Oh, NOW you say we stopped, eh?
      this._seenPlaying.boolValue = false;
      this._stopPlayerLoop();
      
      LOG("_onPollCompleted - stop next loop? " + this._stopNextLoop);
      
      // Don't go on to the next track if we see that we're stopping.
      if ( ! this._stopNextLoop )
        this._playNextPrev(1);
        
      this._stopNextLoop = false;
    }
    else {
      
      LOG("_onPollCompleted - lookForPlayingCount: " + this._lookForPlayingCount);
      
      // After 10 seconds or fatal error, give up and go to the next one?
      if ( ( this._lookForPlayingCount++ > 80 ) || ( len < -1 ) ) {
      
      LOG("_onPollCompleted - lookForPlayingCount > 80 or ERROR.");
      
        if ( ! this._stopNextLoop )
          this._playNextPrev(1);
        this._stopNextLoop = false;
      }
    }
  },

  // If the stream has video, show the video window
  _onPollVideo: function ( core ) {
    try {
      this._playingVideo.boolValue = core.getPlayingVideo();
    }
    catch(e) {
      // Not all cores support this method.  Fall back to isVideoURL
      this._playingVideo.boolValue = core.isVideoURL(this._playURL.stringValue);
    }
  },

  _playNextPrev: function ( incr ) {
    
    LOG("_playNextPrev - increment by " + incr);
    
    // "FIXME" -- mig sez: I think the reason it was broke is because you
    // basically made it restart on playLIST end.  And that would get confusing,
    // I assume.
    if (this._restartOnPlaybackEnd.boolValue)
    { 
      this._restartApp();
      return;
    }

    var cur_ref = this._playingRef.stringValue;
    var cur_url = this._playURL.stringValue;

    // If we haven't played anything yet, do the default
    if (cur_ref == "") {
      this.play();
      return;
    }

    // Get the index of the item that just finished from the view using the
    // stored view item UID.  This is so we can determine the "next" track
    // based on the previous track's current position in the view, not its
    // position when playback started
    var cur_index;
    try {
      cur_index =
        this._playingView.getIndexForViewItemUID(this._playingViewItemUID);
    }
    catch(e if e.result == Components.results.NS_ERROR_NOT_AVAILABLE) {
      // The item that just finished has vanished from the current view.  We
      // have no context to find the next track so just stop
      this.stop();
      return;
    }

    LOG( "current index: " + cur_index );
    
    if ( cur_index > -1 ) {
      // Play the next playlist entry tree index (or whatever, based upon state.)
      // XXXnewlib
      var num_items = this._playingView.length
      LOG( num_items + " items in the current playlist" );
      
      var next_index = -1;
      // Are we confused?
      if ( cur_index != -1 ) {
        // Are we REPEAT ONE?
        if ( this._repeat.intValue == sbIPlaylistPlayback.REPEAT_MODE_ONE ) {
          next_index = cur_index;
        }
        // Are we SHUFFLE?
        else if ( this._shuffle.boolValue ) {
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
            if ( this._repeat.intValue == sbIPlaylistPlayback.REPEAT_MODE_ALL )
              next_index = 0; // Start over
            else
              next_index = -1; // Give up
        }
      }
      
      // If we think we want to play a track, do so.
      LOG( "next index: " + next_index );
      if ( next_index != -1 && this._playingView) {
        this.playView( this._playingView, next_index );
      } else {
        this.stop();
      }        
    }
  },
  
  _playDefault: function () 
  {
    var libraryManager = Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                                   .getService(Components.interfaces.sbILibraryManager);
    var view = libraryManager.mainLibrary.createView();
    // Filter the view as if it is being shown in the UI to determine
    // if this library actually has anything playable in it
    view.setFilters(SBProperties.createArray([
      [SBProperties.isList, "0"],
      [SBProperties.hidden, "0"]
    ]));
    if (view.length > 0) {
      this.playView(view, 0);
    }
  },
  
  _setItemMetadata: function( aItem, aTitle, aLength, aAlbum, aArtist, aGenre ) {
    var base   = "http://songbirdnest.com/data/1.0#";
    
    if ( !aItem ) {
      return;
    }
      
    if ( aTitle && aTitle.length ) {
      aItem.setProperty(base + "trackName", aTitle);
    }

    if ( aLength && aLength != 0 ) {
      aLength = aLength * 1000;
      aItem.setProperty(base + "duration", aLength);
    }

    if ( aAlbum && aAlbum.length ) {
      aItem.setProperty(base + "albumName", aAlbum);
    }

    if ( aArtist && aArtist.length ) {
      aItem.setProperty(base + "artistName", aArtist);
    }

    if ( aGenre && aGenre.length ) {
      aItem.setProperty(base + "genre", aGenre);
    }
  },
  
  _updateCurrentInfoFromView: function(aView, aIndex)
  {
    this._playingRef.stringValue = aView.mediaList.guid;
    this._playlistIndex.intValue = aIndex;

    var item   = aView.getItemByIndex(aIndex);
    var base   = "http://songbirdnest.com/data/1.0#";
    var url    = item.contentSrc.spec;
    var title  = item.getProperty(base + "trackName");
    var artist = item.getProperty(base + "artistName");
    var album  = item.getProperty(base + "albumName");
    var genre  = item.getProperty(base + "genre");
    var duration = item.getProperty(base + "duration");

    // Clear the data remotes
    this._playURL.stringValue = "";
    this._metadataURL.stringValue = "";
    this._metadataTitle.stringValue = "";
    this._metadataArtist.stringValue = "";
    this._metadataAlbum.stringValue = "";
    this._metadataGenre.stringValue = "";
    this._metadataLen.intValue = 0;
    
    // Set the data remotes to indicate what is about to play
    this._playURL.stringValue = url;
    this._metadataURL.stringValue = url;
    this._metadataTitle.stringValue = title;
    this._metadataArtist.stringValue = artist;
    this._metadataAlbum.stringValue = album;
    this._metadataGenre.stringValue = genre;
    this._metadataLen.intValue = parseInt(duration, 10) / 1000;
  },

  _restartApp: function() {
    // this is being watched by the main document in order to implement it
    this._restartAppNow.boolValue = true;
  },
  
  // Stooooopid bug.
  _isFLAC: function() {
    var url = this._playURL.stringValue.toLowerCase();
    return ( url.indexOf( ".flac" ) != -1 );
  },
  
  
  _sleep: function( ms ) {
    var thread = Components.classes["@mozilla.org/thread-manager;1"]
                  .getService().currentThread;
    //thread.sleep(ms);  
  },
  
  /**
   * QueryInterface is always last, it has no trailing comma.
   */
  QueryInterface: function(iid) {
    if (!iid.equals(sbIPlaylistPlayback) &&
        !iid.equals(Components.interfaces.nsIObserver) && 
        !iid.equals(Components.interfaces.nsITimerCallback) && 
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
  
}; // PlaylistPlayback.prototype


function NSGetModule(compMgr, fileSpec) {

  return XPCOMUtils.generateModule([
    PlaylistPlayback,
  ],
  function(aCompMgr, aFileSpec, aLocation) {
    XPCOMUtils.categoryManager.addCategoryEntry(
      "app-startup",
      PlaylistPlayback.prototype.classDescription,
      "service," + PlaylistPlayback.prototype.contractID,
      true,
      true);
  });
}

