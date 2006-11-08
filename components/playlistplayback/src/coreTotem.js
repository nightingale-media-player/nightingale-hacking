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

/**
 * \file coreTotem.js
 * \brief The CoreWrapper implementation for the Totem Plugin
 * \sa sbICoreWrapper.idl coreBase.js
 */
 
/**
 * \class CoreTotem
 * \brief The CoreWrapper for the VLC Plugin
 * \sa CoreBase
 */
function CoreTotem()
{
  this._object = null;
  this._url = "";
  this._id = "";
  this._paused  = false;
  this._oldVolume = 0;
  this._muted = false;

  this._mediaUrlExtensions = ["mp3", "ogg", "flac", "wav", "m4a", "avi", 
                              "mov", "mpg", "mp4"];
  this._mediaUrlSchemes = ["mms", "rstp"];

  this._videoUrlExtensions = ["avi", "mpg", "mp4"];

  this._mediaUrlMatcher = new ExtensionSchemeMatcher(this._mediaUrlExtensions,
                                                     this._mediaUrlSchemes);
  this._videoUrlMatcher = new ExtensionSchemeMatcher(this._videoUrlExtensions,
                                                     []);
};

// inherit the prototype from CoreBase
CoreTotem.prototype = new CoreBase();

// set the constructor so we use ours and not the one for CoreBase
CoreTotem.prototype.constructor = CoreTotem();

CoreTotem.prototype.playURL = function ( aURL )
{
  this._verifyObject();

  if (!aURL)
    throw Components.results.NS_ERROR_INVALID_ARG;

  this._url = this.sanitizeURL(aURL);
  this._paused = false;

  try
  {
    if(this._object.IsPlaying) 
    {
      this._object.Close();
    }
    
    this._object.OpenUrl( this._url );
    this._object.Play();
  }
  catch(e) 
  {
      this.LOG(e);
  }

  return true;
};

CoreTotem.prototype.play = function ()
{
  this._verifyObject();

  this._paused = false;

  try
  {
    this._object.Play();
  }
  catch(e) 
  {
    this.LOG(e);
  }

  return true;
};

CoreTotem.prototype.pause = function ()
{
  if( this._paused )
    return this._paused;

  this._verifyObject();

  try 
  {
    this._object.Stop();
  } 
  catch(e) 
  {
    this.LOG(e);
  }

  this._paused = true;

  return this._paused;
};

CoreTotem.prototype.stop = function ()
{
  this._verifyObject();

  if( !this._object.IsPlaying )
    return true;

  try 
  {
    this._object.Rewind();
    this._object.Close();
    this._paused = false;
  } 
  catch(e) 
  {
    this.LOG(e);
  }

  return true;
};

CoreTotem.prototype.getPlaying = function ()
{
  this._verifyObject();
  return this._object.IsPlaying || this._paused;
};

CoreTotem.prototype.getPlayingVideo = function ()
{
  throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
};

CoreTotem.prototype.getPaused = function ()
{
  this._verifyObject();
  return this._paused;
};

CoreTotem.prototype.getLength = function ()
{
  this._verifyObject();
  var playLength = 0;

  try 
  {
    playLength = this._object.StreamLength;
  } 
  catch(e) 
  {
    this.LOG(e);
  }

  return playLength;
};

CoreTotem.prototype.getPosition = function ()
{
  this._verifyObject();
  var curPos = 0;

  try 
  {
    curPos = this._object.CurrentTime;
  } 
  catch(e) 
  {
    this.LOG(e);
  }

  return curPos;
};

CoreTotem.prototype.setPosition = function ( pos )
{
  this._verifyObject();

  try 
  {
    this._object.SeekTime( pos );
  }
  catch(e) 
  {
    this.LOG(e);
  }
};

CoreTotem.prototype.getVolume = function ()
{
  this._verifyObject();
  return Math.round(this._object.Volume / 100 * 255);
};

CoreTotem.prototype.setVolume = function ( volume )
{
  this._verifyObject();
  
  if ((volume < 0) || (volume > 255))
    throw Components.results.NS_ERROR_INVALID_ARG;
    
  this._object.Volume = Math.round(volume / 255 * 100); 
};

CoreTotem.prototype.getMute = function ()
{
  this._verifyObject();
  return this._muted;
};

CoreTotem.prototype.setMute = function ( mute )
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

CoreTotem.prototype.getMetadata = function ( key )
{
  this._verifyObject();
  return "";
};

CoreTotem.prototype.goFullscreen = function ()
{
  this._verifyObject();
  // Can totem do this?
};

CoreTotem.prototype.isMediaURL = function ( aURL )
{
  return this._mediaUrlMatcher.match(aURL);
}

CoreTotem.prototype.isVideoURL = function ( aURL )
{
  return this._videoUrlMatcher.match(aURL);
}

/**
  * See nsISupports.idl
  */
CoreTotem.prototype.QueryInterface = function(iid) {
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
  var gTotemCore = new CoreTotem();
}
catch(err) {
  dump("ERROR!!! coreFLASH failed to create properly.");
}

/**
  * This is the function called from a document onload handler to bind everything as playback.
  * The <html:object>s won't have their scriptable APIs attached until the onload.
  */
function CoreTotemDocumentInit( id )
{
  try
  {
    var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                         .getService(Components.interfaces.sbIPlaylistPlayback);
    var totemIframe = document.getElementById( id );
    listProperties(totemIframe.contentWindow.document.core_totem, "totem");
    gTotemCore.setId("Totem1");
    gTotemCore.setObject(totemIframe.contentWindow.document.core_totem);
    gPPS.addCore(gTotemCore, true);
  }
  catch ( err )
  {
    dump( "\n!!! coreTotem failed to bind properly\n" + err );
  }
};
