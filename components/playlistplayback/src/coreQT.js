/*
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
  dump("***CoreQT*** " + string + "\n");
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

// The QT Plugin Wrapper
function CoreQT()
{
  this._object = null;
  this._url = "";
  this._id = "";
  this._playing = false;
  this._paused  = false;

  /**
   *
   */
  this._verifyObject = function() {
    if ( (this._object == undefined) || ( ! this._object ) /*|| ( ! this._object instanceof Components.interfaces.nsIQTScriptablePlugin )*/ )
    {
      LOG("VERIFY OBJECT FAILED.  OBJECT DOES NOT HAVE PROPER QT API.")
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    }
  }  
  
  // When another core wrapper takes over, stop.
  this.onSwappedOut = function onSwappedOut()
  {
    this._verifyObject();
    try
    {
      this._object.Stop();
    }
    catch(e)
    {
      LOG(e);
    }
  } 
  
  // Set the url and tell it to just play it.  Eventually this talks to the media library object to make a temp playlist.
  this.playUrl = function ( url )
  {
    this._verifyObject();

    if (!url)
      throw Components.results.NS_ERROR_INVALID_ARG;

    this._url = url;

    this._paused = false;
    this._playing = true;

    if( url.search(/[A-Za-z].*\:\/\//g) < 0 )
    {
      url = "file://" + url;
    }
    
    if( url.search(/\\/g))
    {
      url.replace(/\\/, '/');
    }

    LOG( "Trying to play url: " + url );
    
    this._object.playURL( url );

    return true;
  }

  this.play      = function ()
  {
    this._verifyObject();
  	
    this._paused = false;
    this._playing = true;
  
    try {
      this._object.playPlayer();
    } catch(e) {
      LOG(e);
    }
    
    return true;
  }
  
  this.pause     = function ()
  {
    if( this._paused )
      return this._paused;
  
    this._verifyObject();

    try {
      this._object.pausePlayer();
    } catch(e) {
      LOG(e);
    }

    this._paused = true;
    
    return this._paused;
  }
  
  this.stop = function ()
  {
    this._verifyObject();
    
    if( !this._playing )
      return true;
      
    this._paused = false;
    this._playing = false;

    try {
      this._object.stopPlayer();
    } catch(e) {
      LOG(e);
    }

    return true;
  }
  
  this.next = function ()
  {
    this._verifyObject();
    return CNullStub();
  }
  
  this.prev = function ()
  {
    this._verifyObject();
    return CNullStub();
  }
  
  this.getPlaying   = function ()
  {
    this._verifyObject();
    
    if( this.getLength() == this.getPosition() )
    {
      this.stop();
    }
    
    return this._playing;
  }
  
  this.getPaused    = function ()
  {
    this._verifyObject();
    return this._paused;
  }
  
  this.getLength = function ()
  {
    this._verifyObject();
    var playLength = 0;
    
    try {
      playLength = this._object.getPlayerLength();
    } catch(e) {
      LOG(e);
    }
    
    return playLength;
  }
  
  this.getPosition = function ()
  {
    this._verifyObject();
    var curPos = 0;
    
    try {
      curPos = this._object.getPlayerPosition();
    } catch(e) {
      LOG(e);
    }
    
    return curPos;
  }
  
  this.setPosition = function ( pos )
  {
    this._verifyObject();
    
    try {
      this._object.seekPlayer( pos );
    } catch(e) {
      LOG(e);
    }
  }
  
  this.getVolume   = function ()
  {
    this._verifyObject();
    var curVol = this._object.getPlayerVolume();
    
    return curVol;
  }
  
  this.setVolume   = function ( vol )
  {
    this._verifyObject();
    this._object.setPlayerVolume(vol);

    LOG(this.getVolume());
  }
  
  this.getMute     = function ()
  {
    this._verifyObject();
    var isMuted = this._object.isMuted;
    
    return isMuted;
  }
  
  this.setMute     = function ( mute )
  {
    this._verifyObject();
    this._object.mutePlayer( mute );
  }

  this.getMetadata = function ( key )
  {
    this._verifyObject();
    return "";
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
   * See nsISupports.idl
   */
  this.QueryInterface = function(iid) {
    if (!iid.equals(Components.interfaces.sbICoreWrapper) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }

}

/**
 * ----------------------------------------------------------------------------
 * Global variables and autoinitialization.
 * ----------------------------------------------------------------------------
 */
var gQTCore = new CoreQT();

/**
  * This is the function called from a document onload handler to bind everything as playback.
  * The <html:object>s won't have their scriptable APIs attached until the onload.
  */
function CoreQTDocumentInit( id )
{
  try
  {
    var gPPS = Components.classes["@songbird.org/Songbird/PlaylistPlayback;1"]
                         .getService(Components.interfaces.sbIPlaylistPlayback);
    var theDocumentQTInstance = document.getElementById( id );
   
    gQTCore.setId("QT1");
    gQTCore.setObject(theDocumentQTInstance.contentWindow);
    gPPS.addCore(gQTCore, true);
 }
  catch ( err )
  {
    LOG( "\n!!! coreQT failed to bind properly\n" + err );
  }
} 
 
}
catch ( err )
{
  LOG( "\n!!! coreQT failed to load properly\n" + err );
}
