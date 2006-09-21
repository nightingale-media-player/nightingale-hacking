/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
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

// The MPLAYER Plugin Wrapper
function CoreMPlayer()
{
  this._object = null;
  this._url = "";
  this._id = "";
  this._playing = false;
  this._paused  = false;
};

// inherit the prototype from CoreBase
CoreMPlayer.prototype = new CoreBase();

// set the constructor so we use ours and not the one for CoreBase
CoreMPlayer.prototype.constructor = CoreMPlayer;


  
// Set the url and tell it to just play it.  Eventually this talks to the media library object to make a temp playlist.
CoreMPlayer.prototype.playURL = function ( aURL )
{
  if (!aURL)
    throw Components.results.NS_ERROR_INVALID_ARG;

  this._verifyObject();

  try {  
    this._url = aURL;

    aURL = this.sanitizeURL(aURL);

    for (var stuff in this._object)
      this.LOG("XXXredfive obj has: " + stuff + "\n");
    this.LOG("XXXredfive obj id:" + this._object.id + "\n");
    
    this._object.SetURL( aURL );
    this._object.Play();

    return true;
  }
  catch (err) {
    this.LOG("Error playing url: \n" + err + "\n");
    return false;
  }
}

CoreMPlayer.prototype.play = function ()
{
  this._verifyObject();
  	
  this._paused = false;
  this._playing = true;
  
  try {
    this._object.Play();
  }
  catch(err) {
    this.LOG(err);
  }
    
  return true;
}
  
CoreMPlayer.prototype.pause = function ()
{
  if( this._paused )
    return this._paused;

  this._verifyObject();

  try {
    this._object.Pause();
  }
  catch(err) {
    this.LOG(err);
  }

  this._paused = true;
    
  return this._paused;
}
  
CoreMPlayer.prototype.stop = function ()
{
  this._verifyObject();
    
  if( !this._playing )
    return true;
      
  this._paused = false;
  this._playing = false;

  try {
    this._object.Stop();
  }
  catch(err) {
    this.LOG(err);
  }

  return true;
}
  
CoreMPlayer.prototype.next = function ()
{
  this._verifyObject();
  return CNullStub();
}
  
CoreMPlayer.prototype.prev = function ()
{
  this._verifyObject();
  return CNullStub();
}
  
CoreMPlayer.prototype.getPlaying   = function ()
{
  this._verifyObject();
   
  if( this.getLength() == this.getPosition() )
  {
    this.Stop();
  }
    
  return this._object.isplaying();
}
 
CoreMPlayer.prototype.getPaused    = function ()
{
  this._verifyObject();
  return this._paused;
}
  
CoreMPlayer.prototype.getLength = function ()
{
  this._verifyObject();
  var playLength = 0;
    
  try {
    playLength = this._object.getDuration();
  }
  catch(err) {
    this.LOG(err);
  }
   
  return playLength;
}
  
CoreMPlayer.prototype.getPosition = function ()
{
  this._verifyObject();
  var curPos = 0;
  
  try {
    curPos = this._object.getTime();
  }
  catch(err) {
    this.LOG(err);
  }
   
  return curPos;
}
 
CoreMPlayer.prototype.setPosition = function ( pos )
{
  this._verifyObject();
  
  try {
    this._object.Seek( pos );
  }
  catch(err) {
    this.LOG(err);
  }
}
  
CoreMPlayer.prototype.getVolume   = function ()
{
  this._verifyObject();
  var curVol = 255;
  
  return curVol;
}

CoreMPlayer.prototype.setVolume   = function ( vol )
{
  this._verifyObject();
  //this._object.SetVolume( vol );

  this.LOG(this.getVolume());
}

CoreMPlayer.prototype.getMute     = function ()
{
  this._verifyObject();
  var isMuted = false;
  
  return isMuted;
}
  
CoreMPlayer.prototype.setMute     = function ( mute )
{
  this._verifyObject();
  //this._object.Mute( mute );
}

CoreMPlayer.prototype.getMetadata = function ( key )
{
  this._verifyObject();
  return "";
}

CoreMPlayer.prototype.isMediaURL = function(aURL) {
  if( ( aURL.indexOf ) && 
      (
        // Protocols at the beginning
        ( aURL.indexOf( "mms:" ) == 0 ) || 
        ( aURL.indexOf( "rtsp:" ) == 0 ) ||
        // File extensions at the end
        ( aURL.indexOf( ".mp3" ) == ( aURL.length - 4 ) ) ||
        ( aURL.indexOf( ".ogg" ) == ( aURL.length - 4 ) ) ||
        ( aURL.indexOf( ".flac" ) == ( aURL.length - 5 ) ) ||
        ( aURL.indexOf( ".wav" ) == ( aURL.length - 4 ) ) ||
        ( aURL.indexOf( ".m4a" ) == ( aURL.length - 4 ) ) ||
        ( aURL.indexOf( ".avi" ) == ( aURL.length - 4 ) ) ||
        ( aURL.indexOf( ".mpg" ) == ( aURL.length - 4 ) ) ||
        ( aURL.indexOf( ".mp4" ) == ( aURL.length - 4 ) )
      )
    )
  {
    return true;
  }
  return false;
}

CoreMPlayer.prototype.isVideoURL = function ( aURL )
{
  if ( ( aURL.indexOf ) && 
        (
          ( aURL.indexOf( ".avi" ) == ( aURL.length - 4 ) ) ||
          ( aURL.indexOf( ".mpg" ) == ( aURL.length - 4 ) ) ||
          ( aURL.indexOf( ".mp4" ) == ( aURL.length - 4 ) )
        )
      )
  {
    return true;
  }
  return false;
}

/**
 * See nsISupports.idl
 */
CoreMPlayer.prototype.QueryInterface = function(iid)
{
  if (!iid.equals(Components.interfaces.sbICoreWrapper) &&
      !iid.equals(Components.interfaces.nsISupports))
    throw Components.results.NS_ERROR_NO_INTERFACE;
  return this;
}

/**
 * ----------------------------------------------------------------------------
 * Global variables and autoinitialization.
 * ----------------------------------------------------------------------------
 */
var gMPlayerCore = new CoreMPlayer();

/**
  * This is the function called from a document onload handler to bind everything as playback.
  * The <html:object>s won't have their scriptable APIs attached until the onload.
  */
function InitPlaybackCoreMPlayer( iframeID )
{
  try
  {
    var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                         .getService(Components.interfaces.sbIPlaylistPlayback);
    var mplayerIFrame = document.getElementById( iframeID );
   
    gMPlayerCore.setId("MPLAYER1");
    gMPlayerCore.setObject(mplayerIFrame.contentWindow.document.getElementById("core_mp"));
    gPPS.addCore(gMPlayerCore, true);
  }
  catch ( err )
  {
    dump( "\n!!! coreMPLAYER failed to bind properly\n" + err );
  }
} 
 
