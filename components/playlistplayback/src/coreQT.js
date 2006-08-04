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

/**
 * \file coreQT.js
 * \brief The CoreWrapper implementation for the QuickTime Plugin
 * \sa sbICoreWrapper.idl coreBase.js
 */

/**
 * \class CoreQT
 * \brief The CoreWrapper for the QuickTime Plugin
 * \sa CoreBase
 */
function CoreQT()
{
  this._object = null;
  this._url = "";
  this._id = "";
  this._playing = false;
  this._paused  = false;
};

// inherit the prototype from CoreBase
CoreQT.prototype = new CoreBase();

// set the constructor so we use ours and not the one for CoreBase
CoreQT.prototype.constructor = CoreQT();

// Set the url and tell it to just play it.  Eventually this talks to the media library object to make a temp playlist.
CoreQT.prototype.playURL = function ( aURL )
{
  this._verifyObject();

  if (!aURL)
    throw Components.results.NS_ERROR_INVALID_ARG;

  this._url = this.sanitizeURL(aURL);

  this._paused = false;
  this._playing = true;

  if( aURL.search(/[A-Za-z].*\:\/\//g) < 0 )
  {
    aURL = "file://" + aURL;
  }
  
  if( aURL.search(/\\/g))
  {
    aURL.replace(/\\/, '/');
  }

  this._object.playURL( this._url );

  return true;
};

CoreQT.prototype.play = function ()
{
  this._verifyObject();
  
  this._paused = false;
  this._playing = true;

  try {
    this._object.playPlayer();
  } catch(e) {
    this.LOG(e);
  }
  
  return true;
};
  
CoreQT.prototype.pause = function ()
{
  this._verifyObject();

  if( this._paused )
    return this._paused;

  try {
    this._object.pausePlayer();
  } catch(e) {
    this.LOG(e);
  }

  this._paused = true;
  
  return this._paused;
};
  
CoreQT.prototype.stop = function ()
{
  this._verifyObject();
  
  if( !this._playing )
    return true;
    
  this._paused = false;
  this._playing = false;

  try {
    this._object.stopPlayer();
  } catch(e) {
    this.LOG(e);
  }

  return true;
};
  
CoreQT.prototype.getPlaying = function ()
{
  this._verifyObject();
  
  //XXXAus: I remember I did this for a good reason, but I don't remember why exactly.
  //Just don't change it for now =)
  if( this.getLength() == this.getPosition() )
  {
    this.stop();
  }
  
  return this._playing;
};
  
CoreQT.prototype.getPaused = function ()
{
  this._verifyObject();
  return this._paused;
};
  
CoreQT.prototype.getLength = function ()
{
  this._verifyObject();
  var playLength = 0;
  
  try {
    playLength = this._object.getPlayerLength();
  } catch(e) {
    this.LOG(e);
  }
  
  return playLength;
};
  
CoreQT.prototype.getPosition = function ()
{
  this._verifyObject();
  var curPos = 0;
  
  try {
    curPos = this._object.getPlayerPosition();
  } catch(e) {
    this.LOG(e);
  }
  
  return curPos;
};
  
this.setPosition = function ( pos )
{
  this._verifyObject();
  
  try {
    this._object.seekPlayer( pos );
  } catch(e) {
    this.LOG(e);
  }
};
  
CoreQT.prototype.getVolume = function ()
{
  this._verifyObject();
  var curVol = this._object.getPlayerVolume();
  
  return curVol;
};
  
CoreQT.prototype.setVolume = function ( vol )
{
  this._verifyObject();
  this._object.setPlayerVolume(vol);
};
  
CoreQT.prototype.getMute = function ()
{
  this._verifyObject();
  var isMuted = this._object.isMuted;
  
  return isMuted;
};
  
CoreQT.prototype.setMute = function ( mute )
{
  this._verifyObject();
  this._object.mutePlayer( mute );
};

CoreQT.prototype.getMetadata = function ( key )
{
  this._verifyObject();
  return "";
};

CoreQT.prototype.goFullscreen = function ()
{
  this._verifyObject();
  
  // QT has no fullscreen functionality exposed to its plugin :(
  return true;
};
  
/**
  * See nsISupports.idl
  */
CoreQT.prototype.QueryInterface = function(iid) 
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
  var gQTCore = new CoreQT();
}
catch (err) {
  dump("ERROR!!! coreQT failed to create properly.");
}

/**
  * This is the function called from a document onload handler to bind everything as playback.
  * The <html:object>s won't have their scriptable APIs attached until the onload.
  */
function CoreQTDocumentInit( id )
{
  try
  {
    var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
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
};
