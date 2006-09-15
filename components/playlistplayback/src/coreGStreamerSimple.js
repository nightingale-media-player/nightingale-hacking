/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright� 2006 POTI, Inc.
// http://songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the �GPL�).
//
// Software distributed under the License is distributed
// on an �AS IS� basis, WITHOUT WARRANTY OF ANY KIND, either
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
 * \file coreGStreamerSimple.js
 * \brief The CoreWrapper implementation for the GStreamer simple component
 * \sa sbICoreWrapper.idl coreBase.js
 */

/**
 * \class CoreGStreamerSimple
 * \brief The CoreWrapper for the simple GStreamer Simple component
 * \sa CoreBase
 */
function CoreGStreamerSimple()
{
  this._object = null;
  this._url = "";
  this._id = "";
  this._paused  = false;
  this._oldVolume = 0;
  this._muted = false;
};

// inherit the prototype from CoreBase
CoreGStreamerSimple.prototype = new CoreBase();

// set the constructor so we use ours and not the one for CoreBase
CoreGStreamerSimple.prototype.constructor = CoreGStreamerSimple();

CoreGStreamerSimple.prototype.playURL = function ( aURL )
{
  this._verifyObject();

  if (!aURL) {
    throw Components.results.NS_ERROR_INVALID_ARG;
  }

  this._url = this.sanitizeURL(aURL);
  this._paused = false;

  try
  {
    if(this._object.isPlaying || this._object.isPaused)
    {
      this._object.stop();
    }

    this._object.uri = this._url;
    this._object.play();
  }
  catch(e)
  {
      this.LOG(e);
  }

  return true;
};

CoreGStreamerSimple.prototype.play = function ()
{
  this._verifyObject();

  this._paused = false;

  try
  {
    this._object.play();
  }
  catch(e)
  {
    this.LOG(e);
  }

  return true;
};

CoreGStreamerSimple.prototype.pause = function ()
{
  if( this._paused )
    return this._paused;

  this._verifyObject();

  try
  {
    this._object.pause();
  }
  catch(e)
  {
    this.LOG(e);
  }

  this._paused = true;

  return this._paused;
};

CoreGStreamerSimple.prototype.stop = function ()
{
  this._verifyObject();

  if( !this._object.isPlaying )
    return true;

  try
  {
    this._object.stop();
    this._paused = false;
  }
  catch(e)
  {
    this.LOG(e);
  }

  return true;
};

CoreGStreamerSimple.prototype.getPlaying = function ()
{
  this._verifyObject();
  var playing = (this._object.isPlaying || this._paused) && (!this._object.isAtEndOfStream);
  return playing;
};

CoreGStreamerSimple.prototype.getPaused = function ()
{
  this._verifyObject();
  return this._paused;
};

CoreGStreamerSimple.prototype.getLength = function ()
{
  this._verifyObject();
  var playLength = 0;

  try
  {
    if(this._object.lastErrorCode > 0) {
      return -1;
    }
    else if(!this.getPlaying()) {
      playLength = 0;
    }
    else {
      playLength = this._object.streamLength / (1000 * 1000);
    }
  }
  catch(e)
  {
    if(e.result == Components.results.NS_ERROR_NOT_AVAILABLE)
    {
      return -1;
    }
    else
    {
      this.LOG(e);
    }
  }

  return playLength;
};

CoreGStreamerSimple.prototype.getPosition = function ()
{
  this._verifyObject();
  var curPos = 0;

  try
  {
    if(this._object.lastErrorCode > 0) {
      return -1;
    }
    else if(this._object.isAtEndOfStream || !this.getPlaying()) {
      curPos = 0;
    }
    else {
      curPos = Math.round(this._object.position / (1000 * 1000));
    }
  }
  catch(e)
  {
    if(e.result == Components.results.NS_ERROR_NOT_AVAILABLE)
    {
      return -1;
    }
    else
    {
      this.LOG(e);
    }
  }

  return curPos;
};

CoreGStreamerSimple.prototype.setPosition = function ( pos )
{
  this._verifyObject();

  try
  {
    this._object.seek( pos * (1000 * 1000) );
  }
  catch(e)
  {
    this.LOG(e);
  }
};

CoreGStreamerSimple.prototype.getVolume = function ()
{
  this._verifyObject();
  return Math.round(this._object.volume * 255);
};

CoreGStreamerSimple.prototype.setVolume = function ( volume )
{
  this._verifyObject();

  if ((volume < 0) || (volume > 255))
    throw Components.results.NS_ERROR_INVALID_ARG;

  this._object.volume = volume / 255;
};

CoreGStreamerSimple.prototype.getMute = function ()
{
  this._verifyObject();
  return this._muted;
};

CoreGStreamerSimple.prototype.setMute = function ( mute )
{
  this._verifyObject();

  if(mute)
  {
    this._oldVolume = this.getVolume();
    this._muted = true;
  }
  else
  {
    this.setVolume(this._oldVolume);
    this._muted = false;
  }
};

CoreGStreamerSimple.prototype.getMetadata = function ( key )
{
  this._verifyObject();
  var rv;

  switch(key) {
      case "title":
        rv = this._object.title;
      break;
      case "album":
        rv = this._object.album;
      break;
      case "artist":
        rv = this._object.artist;
      break;
      case "genre":
        rv = this._object.genre;
      break;
      default:
        rv = "";
      break;
  }

  return rv;
};

CoreGStreamerSimple.prototype.goFullscreen = function ()
{
  this._verifyObject();
};

  
CoreGStreamerSimple.prototype.isMediaURL = function(aURL) {
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
        ( aURL.indexOf( ".wma" ) == ( aURL.length - 4 ) ) ||
        ( aURL.indexOf( ".wmv" ) == ( aURL.length - 4 ) ) ||
        ( aURL.indexOf( ".asx" ) == ( aURL.length - 4 ) ) ||
        ( aURL.indexOf( ".asf" ) == ( aURL.length - 4 ) ) ||
        ( aURL.indexOf( ".avi" ) == ( aURL.length - 4 ) ) ||
        ( aURL.indexOf( ".mov" ) == ( aURL.length - 4 ) ) ||
        ( aURL.indexOf( ".mpg" ) == ( aURL.length - 4 ) ) ||
        ( aURL.indexOf( ".mp4" ) == ( aURL.length - 4 ) )
      )
    )
  {
    return true;
  }
  return false;
}

CoreGStreamerSimple.prototype.isVideoURL = function ( aURL )
{
  if ( ( aURL.indexOf ) && 
        (
          ( aURL.indexOf( ".wmv" ) == ( aURL.length - 4 ) ) ||
          
          // A better solution is needed, as asx files are not always video..
          // The following hack brought to you by Nivi:
          ( aURL.indexOf( ".asx" ) == ( aURL.length - 4 ) && aURL.indexOf( "allmusic.com" ) == -1 ) ||
          
          ( aURL.indexOf( ".asf" ) == ( aURL.length - 4 ) ) ||
          ( aURL.indexOf( ".avi" ) == ( aURL.length - 4 ) ) ||
          ( aURL.indexOf( ".mov" ) == ( aURL.length - 4 ) ) ||
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
CoreGStreamerSimple.prototype.QueryInterface = function(iid) {
  if (!iid.equals(Components.interfaces.sbICoreWrapper) &&
      !iid.equals(Components.interfaces.nsISupports))
    throw Components.results.NS_ERROR_NO_INTERFACE;
  return this;
};

/**
 * ----------------------------------------------------------------------------
 * Global variables and autoinitialization.
 * ----------------------------------------------------------------------------
 */
try {
  var gGStreamerSimpleCore = new CoreGStreamerSimple();
}
catch(err) {
  dump("ERROR!!! coreFLASH failed to create properly.");
}

/**
  * This is the function called from a document onload handler to bind everything as playback.
  */
function CoreGStreamerSimpleDocumentInit( id )
{
  try
  {
    var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                         .getService(Components.interfaces.sbIPlaylistPlayback);
    var videoElement = document.getElementById( id );
    gGStreamerSimpleCore.setId("GStreamerSimple1");
    var gstSimple = Components.classes["@songbirdnest.com/Songbird/Playback/GStreamer/Simple;1"]
                              .createInstance(Components.interfaces.sbIGStreamerSimple);
    gstSimple.init(videoElement);
    gGStreamerSimpleCore.setObject(gstSimple);
    gPPS.addCore(gGStreamerSimpleCore, true);
  }
  catch ( err )
  {
    dump( "\n!!! coreGStreamerSimple failed to bind properly\n" + err );
  }
};

