/* 
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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
 * ----------------------------------------------------------------------------
 * Global Utility Functions (it's fun copying them everywhere...)
 * ----------------------------------------------------------------------------
 */

/**
* Makes a new URI from a url string
 */
function newURI(aURLString)
{
  // Must be a string here
  if (!(aURLString &&
        (aURLString instanceof String) || typeof(aURLString) == "string"))
    throw Components.results.NS_ERROR_INVALID_ARG;
  
  var ioService =
    Components.classes["@mozilla.org/network/io-service;1"]
    .getService(Components.interfaces.nsIIOService);
  
  try {
    return ioService.newURI(aURLString, null, null);
  }
  catch (e) { }
  
  return null;
}

/**
 * \class CoreWMP
 * \brief The CoreWrapper for the Windows Media Player plugin
 * \sa CoreBase
 */
function CoreWMP() {
  this._object = null;
  this._id = "";
  this._active = false;
}

CoreWMP.prototype = {
  
  /**
   * Boring stuff
   */
  constructor: CoreWMP,
  __proto__: CoreBase.prototype,

  playURL: function playURL(aURL) {
    this._verifyObject();
    
    var uri = newURI(aURL);
    this._object.currentURI = uri;
    
    if (this._object.playing || this._object.paused)
      this._object.stop();
    
    this._object.position = 0;
    this._object.play();
    
    return true;
  },

  play: function play() {
    this._verifyObject();
    this._object.play();
    return true;
  },

  pause: function pause() {
    this._verifyObject();
    this._object.pause();
    return this._object.paused;
  },
  
  stop: function stop() {
    this._verifyObject();
    this._object.stop();
    return !(this._object.playing && this._object.paused);
  },
  
  getPlaying: function getPlaying() {
    this._verifyObject();
    var playing = this._object.playing || this._object.paused;
    return playing;
  },
  
  getPaused: function getPaused() {
    this._verifyObject();
    return this._object.paused;
  },
  
  getLength: function getLength() {
    this._verifyObject();
    return this._object.length;
  },
  
  getPosition: function getPosition() {
    this._verifyObject();
    return this._object.position;
  },
  
  setPosition: function setPosition(aPos) {
    this._verifyObject();
    this._object.position = aPos;
  },
  
  getVolume: function getVolume() {
    this._verifyObject();
    return this._object.volume * 255 / 100;
  },
  
  setVolume: function setVolume(aVolume) {
    this._verifyObject();
    this._object.volume = aVolume / 255 * 100;
  },
  
  getMute: function getMute() {
    this._verifyObject();
    return this._object.muted;
  },
  
  setMute: function setMute(aMute) {
    this._verifyObject();
    this._object.muted = aMute;
  },
  
  getMetadata: function getMetadata(aKey) {
    this._verifyObject();
    return this._object.getMetadataForKey(aKey);
  },
  
  goFullscreen: function goFullscreen() {
    this._verifyObject();
  },
  
  isMediaURL: function isMediaURL(aURL) {
    this._verifyObject();
    var uri = newURI(aURL);
    if (!uri)
      return false;
    return this._object.isMediaURI(uri);
  },
  
  isVideoURL: function isVideoURL(aURL) {
    this._verifyObject();
    var uri = newURI(aURL);
    if (!uri)
      return false;
    return this._object.isVideoURI(uri);
  },
  
  getPlayingVideo: function getPlayingVideo() {
    this._verifyObject();
    return this._object.currentURIHasVideo;
  },
  
  getSupportedFileExtensions: function getSupportedFileExtensions() {
    this._verifyObject();
    return this._object.supportedFileTypes;
  },
    
  activate: function activate() {
    if (this._active)
      return;
    
    this._verifyObject();
    
    try {
      var videoElement = this._object.targetElement;
      videoElement.removeAttribute("hidden");
    } catch (e) { }
    
    this._active = true;
  },

  deactivate: function deactivate() {
    if (!this._active)
      return;

    this.stop();
    
    var videoElement = this._object.targetElement;
    videoElement.setAttribute("hidden", true);
    
    this._active = false;
  },

  /**
   * See nsISupports.idl
   */
  QueryInterface: function QueryInterface(iid) {
    if (!iid.equals(Components.interfaces.sbICoreWrapper) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
}

/**
 * ----------------------------------------------------------------------------
 * Autoinitialization.
 * ----------------------------------------------------------------------------
 */
 
/**
  * This is the function called from a document onload handler to bind everything as playback.
  * The <html:object>s won't have their scriptable APIs attached until the onload.
  */
function CoreWMPDocumentInit(aBoxId)
{
  var wmp = Components.classes["@songbirdnest.com/Songbird/Playback/WindowsMediaControl;1"]
                      .createInstance(Components.interfaces.sbIWindowsMediaControl);
  wmp.targetElement = document.getElementById(aBoxId);
  
  var coreWrapper = new CoreWMP();
  
  coreWrapper.setObject(wmp);
  coreWrapper.setId("WindowsMedia1");
  
  var pps = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                      .getService(Components.interfaces.sbIPlaylistPlayback);
  pps.addCore(coreWrapper, true);
  registeredCores.push(coreWrapper);
}
