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
 * \file coreVLC.js
 * \brief The CoreWrapper implementation for the VLC Plugin
 * \sa sbICoreWrapper.idl coreBase.js
 */

/**
 * ----------------------------------------------------------------------------
 * Core Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \class CoreVLC
 * \brief The CoreWrapper for the VLC Plugin
 * \sa CoreBase
 */
 
function CoreVLC()
{
  this._object = null;
  this._id = "";
  this._muted = false;
  this._url = "";
  this._paused = false;
  
};

// inherit the prototype from CoreBase
CoreVLC.prototype = new CoreBase();

// set the constructor so we use ours and not the one for CoreBase
CoreVLC.prototype.constructor = CoreVLC();

CoreVLC.prototype.playUrl = function (aURL)
{
  this._verifyObject();

  if (!aURL)
    throw Components.results.NS_ERROR_INVALID_ARG;
    
  this._url = aURL;
  this._object.playmrl(this._url);
  
  if (this._object.isplaying()) 
  {
    this._paused = false;
    this._lastPosition = 0;
    return true;
  }
  
  return false;
};
  
CoreVLC.prototype.play = function() 
{
  this._verifyObject();
  this._object.play();
  this._paused = false;

  return true;
};
  
CoreVLC.prototype.stop = function() 
{
  this._verifyObject();
  this._object.stop();
  this._paused = false;
  return false;
};
  
CoreVLC.prototype.pause = function()
{
  if (this._paused)
    return false;
    
  this._verifyObject();
  this._object.pause();
  
  if (this._object.isplaying())
    return false;
    
  this._paused = true;
  this.LOG("Pause!!!");
  
  return true;
};

CoreVLC.prototype.getPaused = function() 
{
  return this._paused;
};

CoreVLC.prototype.getPlaying = function() 
{
  this._verifyObject();
  return this._object.isplaying() || this._paused;
};

CoreVLC.prototype.getMute = function() 
{
  return this._muted;
};

CoreVLC.prototype.setMute = function(mute)
{
  this._verifyObject();

  if (this._muted != mute) 
  {
    this._muted = mute;
    this._object.mute();
  }
};

CoreVLC.prototype.getVolume = function() 
{
  this._verifyObject();

  /**
  * Valid volumes are from 0 to 255.
  * VLC uses a 0-200 scale, so volumes are adjusted accordingly.
  */
  var scaledVolume = this._object.get_volume();
  var retVolume = Math.round(scaledVolume / 175 * 255);
  
  return retVolume;
};

CoreVLC.prototype.setVolume = function(volume) 
{
  this._verifyObject();
  if ( (volume < 0) || (volume > 255) )
    throw Components.results.NS_ERROR_INVALID_ARG;
    
  var scaledVolume = Math.round(volume / 255 * 175);
  
  this._object.set_volume(scaledVolume);
};
  
CoreVLC.prototype.getLength = function() 
{
  this._verifyObject();
  return this._object.get_length() * 1000;
};

CoreVLC.prototype.getPosition = function() 
{
  this._verifyObject();
  return this._object.get_time() * 1000;
};

CoreVLC.prototype.setPosition = function(position) 
{
  this._verifyObject();
  this._object.seek(position / 1000, false);
};

CoreVLC.prototype.goFullscreen = function() 
{
  this._verifyObject();
  return this._object.fullscreen();
};

CoreVLC.prototype.getMetadata = function(key) 
{
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
};
  
CoreVLC.prototype.QueryInterface = function(iid) 
{
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
  var gCoreVLC = new CoreVLC();
}
catch (err) {
  dump("ERROR!!! coreFLASH failed to create properly.");
}

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
    dump( "\n!!! coreVLC failed to bind properly\n" + err );
  }
}; 
 