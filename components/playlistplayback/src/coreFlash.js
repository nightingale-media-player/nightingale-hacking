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

// The Flash Plugin Wrapper
function CoreFlash()
{
  this._object = null;
  this._url = "";
  this._id = "";
  this._playing = false;
  this._paused  = false;
}

// inherit the prototype from BaseCore
CoreFlash.prototype = new BaseCore();

// set the constructor so we use ours and not the one for BaseCore
CoreFlash.prototype.constructor = CoreFlash;

CoreFlash.prototype.playUrl = function ( aURL )
{
  if (!aURL)
    throw Components.results.NS_ERROR_INVALID_ARG;

  try {
    this._verifyObject();

    // store original url, pre-cleanup
    this._url = aURL;

    // remove "\\" and add "file:" if neccessary   
    aURL = this.sanitizeURL(aURL);

    //this.LOG( "Trying to play url: " + aURL );
  
    this._object.LoadMovie( 0, aURL );
    this._object.Play();
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
  	
  //for ( var stuff in this._object )
  //  this.LOG("XXXredfive obj has: " + stuff + "\n");
  
  try {
    this._object.Play();
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
    this._object.StopPlay();
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
    this._object.StopPlay();
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
  return this._paused;
};

CoreFlash.prototype.getPlaying = function ()
{
  return this._playing;
};

CoreFlash.prototype.getMute = function ()
{
  return false;
};

CoreFlash.prototype.setMute = function (aMute)
{
  // XXXredfive - there is no volume aspect to the flash api
  return;
};


CoreFlash.prototype.getVolume = function ()
{
  // XXXredfive - there is no volume aspect to the flash api
  return 100;
};


CoreFlash.prototype.setVolume = function (aVolume)
{
  // XXXredfive - there is no volume aspect to the flash api
  return;
};

CoreFlash.prototype.getLength = function ()
{
  this._verifyObject();
  return this._object.TotalFrames();
};

CoreFlash.prototype.getPosition = function ()
{
  this._verifyObject();
  return true;
};

CoreFlash.prototype.setPosition = function (aPosition)
{
  this._verifyObject();
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
  return true;
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
  dump("ERROR!!! coreFLASH failed to create properly");
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
    gFlashCore.setObject(flashIFrame.contentWindow.document.getElementById("core_fp"));
    gFlashCore.setId("FLASH1");

    // add the core to the playback service
    var gPPS = Components.classes["@songbird.org/Songbird/PlaylistPlayback;1"]
                         .getService(Components.interfaces.sbIPlaylistPlayback);
    gPPS.addCore(gFlashCore, true);
  }
  catch ( err ) {
    dump( "\n!!! Flash PlaybackCore failed to bind properly\n" + err + "\n" );
  }
};
 
