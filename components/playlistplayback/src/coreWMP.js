/* 
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
  dump("***CoreWMP*** " + string + "\n");
} // LOG

/**
 * Dumps an object's properties to the console
 * @param   obj
 *          The object to dump
 * @param   objName
 *          A string containing the name of obj
 */  
function listProperties(obj, objName) {
  var columns = 1;
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
 * Converts seconds to a time string
 * @param   seconds
 *          The number of seconds to convert
 * @return  A string containing the converted time
 */  
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
  {
    text += hours + ":";
  }
  if ( minutes < 10 )
  {
    text += "0";
  }
  text += minutes + ":";
  if ( seconds < 10 )
  {
    text += "0";
  }
  text += seconds;
  return text;
}

/**
 * ----------------------------------------------------------------------------
 * Core Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * Constructor
 */
function CoreWMP() {
  /**
   * The instance to the WMP <object>
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
    // Like, actually verify that we think it's a wmp object.  Not just that it's defined.
    if ( (this._object === undefined) || ( ! this._object ) || ( ! this._object.uiMode ) )
    {
      LOG("VERIFY OBJECT FAILED.  OBJECT DOES NOT HAVE PROPER WMP API.")
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    }
  }
  
  /**
   *
   */
  this.getPaused = function() {
    return ( this._object.playState == 2 );
  }

  /**
   *
   */
  this.getPlaying = function() {
    this._verifyObject();
    return ( ( this._object.playState == 3 ) || this.getPaused() );
  }

  /**
   *
   */
  this.getMute = function() {
    return this._muted;
  }
  this.setMute = function(mute) {
    this._verifyObject();
    if (this._muted != mute)
      this._object.settings.mute = this._muted = mute;
  }

  /**
   * Valid volumes are from 0 to 255.
   * WMP uses a 0-100 scale, so volumes are adjusted accordingly.
   */
  this.getVolume = function() {
    this._verifyObject();
    var scaledVolume = this._object.settings.volume;
    var retVolume = Math.round(scaledVolume / 100 * 255);
    return retVolume;
  }
  this.setVolume = function(volume) {
    this._verifyObject();
    if ((volume < 0) || (volume > 255))
      throw Components.results.NS_ERROR_INVALID_ARG;
    var scaledVolume = Math.round(volume / 255 * 100); 
    this._object.settings.volume = scaledVolume;
  }
  
  /**
   *
   */
  this.getLength = function() {
    this._verifyObject();
    return this._object.currentMedia.duration * 1000;
  }

  /**
   *
   */
  this.getPosition = function() {
    this._verifyObject();
    return this._object.controls.currentPosition * 1000;
  }
  this.setPosition = function(position) {
    this._verifyObject();
    this._object.controls.currentPosition = position / 1000;
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
    this._object.controls.stop();
    this._object.URL = url;
    this._object.controls.play();
    if (this.getPlaying()) {
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
    this._object.controls.play();
    this._paused = false;
    return false;
  }
  
  /**
   *
   */
  this.stop = function() {
    this._verifyObject();
    this._object.controls.stop();
    this._paused = false;
    return false;
  }
  
  /**
   *
   */
  this.pause = function() {
    this._verifyObject();
    if ( this.getPaused() )
      this._object.controls.play();
    else
      this._object.controls.pause();
    this._paused = this.getPaused();
    LOG("Pause - " + this._paused);
    return this._paused;
  }

  /**
   *
   */
  this.getMetadata = function(key) {
    var retval = "";
    switch ( key ) {
      case "album":
        retval += this._object.getItemInfo( "WM/AlbumTitle" );
      break;
      
      case "artist":
        retval += this._object.getItemInfo( "WM/AlbumArtist" );
      break;

      case "genre":
        retval += this._object.getItemInfo( "WM/Genre" );
      break;
      
      case "length":
        retval += EmitSecondsToTimeString( this._object.getItemInfo( "Duration" ) );
      break;
      
      case "title":
        retval += this._object.getItemInfo( "Title" );
      break;
      
      case "url":
        retval += this._object.getItemInfo( "SourceURL" );
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
} // CoreWMP


/**
 * ----------------------------------------------------------------------------
 * Global variables and autoinitialization.
 * ----------------------------------------------------------------------------
 */
var gCoreWMP = new CoreWMP();


/**
  * This is the function called from a document onload handler to bind everything as playback.
  * The <html:object>s won't have their scriptable APIs attached until the onload.
  */
function CoreWMPDocumentInit( id )
{
  try
  {
    var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                         .getService(Components.interfaces.sbIPlaylistPlayback);
    var theWMPInstance = document.getElementById( id );

   
    alert( theWMPInstance.controls );
    
    gCoreWMP.setId("WMP1");
    gCoreWMP.setObject(theWMPInstance);
    gPPS.addCore(gCoreWMP, true);
    
  }
  catch ( err )
  {
    LOG( "\n!!! coreWMP failed to bind properly\n" + err );
  }
} 
 
}
catch ( err )
{
  LOG( "\n!!! coreWMP failed to load properly\n" + err );
}

