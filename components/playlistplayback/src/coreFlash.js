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
 * \file coreFlash.js
 * \brief The CoreWrapper implementation for the Flash Plugin
 * \sa sbICoreWrapper.idl coreBase.js
 */
// The Flash Plugin Wrapper

/**
 * \class CoreFlash
 * \brief The CoreWrapper for the Flash Plugin
 * \sa CoreBase
 */
function CoreFlash()
{
  this._object = null;
  this._url = "";
  this._id = "";
  this._playing = false;
  this._paused  = false;
}

// inherit the prototype from CoreBase
CoreFlash.prototype = new CoreBase();

// set the constructor so we use ours and not the one for CoreBase
CoreFlash.prototype.constructor = CoreFlash;

CoreFlash.prototype.playURL = function ( aURL )
{
  if (!aURL)
    throw Components.results.NS_ERROR_INVALID_ARG;

  try {
    this._verifyObject();

    // store original url, pre-cleanup
    this._url = aURL;

    // remove "\\" and add "file:" if neccessary   
    aURL = this.sanitizeURL(aURL);

    this._object.SetVariable("url", aURL);

    this._paused = false;
    this._playing = true;

    return true;
  }
  catch (err) {
    this.LOG("Error playing url: \n" + err + "\n");
    return false;
  }
};

CoreFlash.prototype.play = function ()
{
  this._verifyObject();
  	
  try {
    this._object.SetVariable("action", "play");
    this._paused = false;
    this._playing = true;
  }
  catch(err) {
    this.LOG("Error playing url: \n" + err + "\n");
  }
    
  return true;
};
  
CoreFlash.prototype.pause = function ()
{
  if( this._paused )
    return this._paused;
  
  this._verifyObject();

  try {
    this._object.SetVariable("action", "pause");
  }
  catch(err) {
    this.LOG(err);
  }

  this._paused = true;
    
  return this._paused;
};
  
CoreFlash.prototype.stop = function ()
{
  if( !this._playing )
    return true;
      
  this._verifyObject();
    
  try {
    this._object.SetVariable("action", "stop");
    this._paused = false;
    this._playing = false;
  }
  catch(err) {
    this.LOG(err);
    return false;
  }
  return true;
};
 
CoreFlash.prototype.getPaused = function ()
{
  this._verifyObject();
      
  try {
    this._paused = this._object.GetVariable("isPaused") == "1";
  }
  catch(err) {
    this.LOG(err);
    return false;
  }

  return this._paused;
};

CoreFlash.prototype.getPlaying = function ()
{
  try {
    this._playing = ( this._object.GetVariable("isPlaying") == "1" ||
                      this._object.GetVariable("isPaused") == "1" );
  }
  catch(err) {
    this.LOG(err);
    return false;
  }
  
  return this._playing;
};

CoreFlash.prototype.getMute = function ()
{
  return this._object.GetVariable("volume") == "0";
};

CoreFlash.prototype.setMute = function (aMute)
{
  this._object.SetVariable("volume", "0");
  return;
};


CoreFlash.prototype.getVolume = function ()
{
  return parseInt(this._object.GetVariable("volume"));
};


CoreFlash.prototype.setVolume = function (aVolume)
{
  this._object.SetVariable("volume", "" + aVolume)
};

CoreFlash.prototype.getLength = function ()
{
  this._verifyObject();
  return parseInt(this._object.GetVariable("duration"));
};

CoreFlash.prototype.getPosition = function ()
{
  this._verifyObject();
  return parseInt(this._object.GetVariable("position"));
};

CoreFlash.prototype.setPosition = function (aPosition)
{
  this._verifyObject();
  this._object.SetVariable("position", "" + aPosition)
  return true;
};

CoreFlash.prototype.goFullscreen = function ()
{
  this._verifyObject();
  return true;
};

CoreFlash.prototype.getMetadata = function (aKey)
{
  this._verifyObject();
  return null;
};

/**
 * See nsISupports.idl
 */
CoreFlash.prototype.QueryInterface = function(iid)
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
  var gFlashCore = new CoreFlash();
}
catch (err) {
  dump("ERROR!!! coreFLASH failed to create properly.");
}

/**
  * This is the function called from a document onload handler to bind everything as playback.
  * The <html:object>s won't have their scriptable APIs attached until the onload.
  */
function InitPlaybackCoreFlash ( aElementId )
{
  try {
    // get the IFrame that contains the embedded flash plugin
    var flashIFrame = document.getElementById( aElementId );

    // get the nsHTMLDOMObjectElement
    gFlashCore.setObject(flashIFrame.contentWindow.document.core_fp);
    gFlashCore.setId("FLASH1");

    // add the core to the playback service
    var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                         .getService(Components.interfaces.sbIPlaylistPlayback);
    gPPS.addCore(gFlashCore, true);
  }
  catch ( err ) {
    dump( "\n!!! Flash PlaybackCore failed to bind properly\n" + err + "\n" );
  }
};
