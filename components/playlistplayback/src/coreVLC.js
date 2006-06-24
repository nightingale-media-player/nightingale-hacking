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

try
{

/**
 * ----------------------------------------------------------------------------
 * Global Utility Functions (it's fun copying them everywhere...)
 * ----------------------------------------------------------------------------
 */

/**
 * Logs a string to the error console. 
 * @param   string
 *          The string to write to the error console..
 */  
function LOG(string) {
  dump("***CoreVLC*** " + string + "\n");
  gConsole = Components.classes["@mozilla.org/consoleservice;1"].createInstance(Components.interfaces.nsIConsoleService);
  if (gConsole)
    gConsole.logStringMessage(string);
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
 * ----------------------------------------------------------------------------
 * Core Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * Constructor
 */
function CoreVLC() {
  /**
   * The instance to the VLC <object>
   */
  this._object = null;
  
  /**
   * A generic 'id' tag
   */
  this._id = "";

  /**
   * Whether or not the player is currently muted
   */
  this._muted = false;

  /**
   * The url of the currently loaded media
   */
  this._url = "";

  /**
   *
   */
  this._paused = false;
  
  /**
   *
   */
  this._verifyObject = function() {
    // Like, actually verify that we think it's a vlc object.  Not just that it's defined.
    if ( (this._object === undefined) || ( ! this._object ) || ( ! this._object.playmrl ) )
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
  }
  
  /**
   *
   */
  this.getPaused = function() {
    return this._paused;
  }

  /**
   *
   */
  this.getPlaying = function() {
    this._verifyObject();
    return this._object.isplaying() || this._paused;
  }

  /**
   *
   */
  this.getMute = function() {
    return this._muted;
  }
  this.setMute = function(mute) {
    this._verifyObject();
    if (this._muted != mute) {
      this._muted = mute;
      this._object.mute();
    }
  }

  /**
   * Valid volumes are from 0 to 255.
   * VLC uses a 0-200 scale, so volumes are adjusted accordingly.
   */
  this.getVolume = function() {
    this._verifyObject();
    var scaledVolume = this._object.get_volume();
    var retVolume = Math.round(scaledVolume / 200 * 255);
    return retVolume;
  }
  this.setVolume = function(volume) {
    this._verifyObject();
    if ((volume < 0) || (volume > 255))
      throw Components.results.NS_ERROR_INVALID_ARG;
    var scaledVolume = Math.round(volume / 255 * 200); 
    this._object.set_volume(scaledVolume);
  }
  
  /**
   *
   */
  this.getLength = function() {
    this._verifyObject();
    return this._object.get_length() * 1000;
  }

  /**
   *
   */
  this.getPosition = function() {
    this._verifyObject();
    return this._object.get_time() * 1000;
  }
  this.setPosition = function(position) {
    this._verifyObject();
    this._object.seek(position / 1000, false);
  }

  /**
   *
   */
  this.goFullscreen = function() {
    this._verifyObject();
    return this._object.fullscreen();
  }

  /**
   *
   */
  this.getId = function() {
    return this._id;
  }
  this.setId = function(id) {
    if (this._id != id)
      this._id = id;
  }

  /**
   *
   */
  this.getObject = function() {
    return this._object;
  }
  this.setObject = function(object) {
   if (this._object != object) {
      if (this._object)
        this.swapCore();
      this._object = object;
    }
  }

  /**
   *
   */
  this.playUrl = function(url) {
    this._verifyObject();
    if (!url)
      throw Components.results.NS_ERROR_INVALID_ARG;
    LOG( "Trying to play url(" + url.length + "): " + url );
    var text = ""; for (var i = 0; i < url.length; i++) { text += (url[i]) + " "; if (i%8==0) text += "\n"; } LOG( text );
    this._url = url;
    this._object.playmrl(this._url);
    if (this._object.isplaying()) {
      this._paused = false;
      this._lastPosition = 0;
      LOG( "SUCCESS playing url: " + url );
      return true;
    }
    LOG( "FAILURE playing url: " + url );
    return false;
  }
  
  /**
   *
   */
  this.play = function() {
    this._verifyObject();
    this._object.play();
    this._paused = false;
    return false;
  }
  
  /**
   *
   */
  this.stop = function() {
    this._verifyObject();
    this._object.stop();
    this._paused = false;
    return false;
  }
  
  /**
   *
   */
  this.pause = function() {
    if (this._paused)
      return false;
    this._verifyObject();
    this._object.pause();
    if (this._object.isplaying())
      return false;
    this._paused = true;
    LOG("Pause!!!");
    return true;
  }

  /**
   *
   */
  this.getMetadata = function(key) {
    var retval = "";
    switch ( key ) {
      case "album":
        retval += this._object.get_metadata_str( "Meta-information", "Album/movie/show title" );
      break;
      
      case "artist":
        retval += this._object.get_metadata_str( "Meta-information", "Artist" );
      break;

      case "genre":
        retval += this._object.get_metadata_str( "Meta-information", "Genre" );
      break;
      
      case "length":
        retval += this._object.get_metadata_str( "General", "Duration" );
      break;
      
      case "title":
        retval += this._object.get_metadata_str( "Meta-information", "Title" );
      break;
      
      case "url":
        retval += this._object.get_metadata_str( "Meta-information", "URL" );
      break;
    }
    return retval;
  }
  
  /**
   *
   */
  this.onSwapCore = function() {
    this.stop();
  }
  
  /**
   * See nsISupports.idl
   */
  this.QueryInterface = function(iid) {
    if (!iid.equals(Components.interfaces.sbICoreWrapper) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
} // CoreVLC


/**
 * ----------------------------------------------------------------------------
 * Global variables and autoinitialization.
 * ----------------------------------------------------------------------------
 */
var gCoreVLC = new CoreVLC();


/**
  * This is the function called from a document onload handler to bind everything as playback.
  * The <html:object>s won't have their scriptable APIs attached until the onload.
  */
function CoreVLCDocumentInit( id )
{
  try
  {
    var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                         .getService(Components.interfaces.sbIPlaylistPlayback);
    var theVLCInstance = document.getElementById( id );

    gCoreVLC.setId("VLC1");
    gCoreVLC.setObject(theVLCInstance);
    gPPS.addCore(gCoreVLC, true);
  }
  catch ( err )
  {
    LOG( "\n!!! coreVLC failed to bind properly\n" + err );
  }
} 
 
}
catch ( err )
{
  LOG( "\n!!! coreVLC failed to load properly\n" + err );
}

