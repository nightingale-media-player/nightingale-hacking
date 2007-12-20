/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
 * \file coreQuickTime.js
 * \brief The CoreWrapper implementation for the GStreamer simple component
 * \sa sbICoreWrapper.idl coreBase.js
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
 * \class QuickTimeCore
 * \brief The CoreWrapper for the QuickTime component
 * \sa CoreBase
 */
function QuickTimeCore()
{
  this._object = null;
  this._id = "";
};

// inherit the prototype from CoreBase
QuickTimeCore.prototype = new CoreBase();

// set the constructor so we use ours and not the one for CoreBase
QuickTimeCore.prototype.constructor = QuickTimeCore();

QuickTimeCore.prototype.playURL = function (aURL) {
  this._verifyObject();

  var uri = newURI(aURL);
  this._object.currentURI = uri;

  if (this._object.playing || this._object.paused)
    this._object.stop();

  this._object.position = 0;
  this._object.play();

  return true;
};

QuickTimeCore.prototype.play = function () {
  this._verifyObject();
  this._object.play();
  return true;
};

QuickTimeCore.prototype.pause = function () {
  this._verifyObject();
  this._object.pause();
  return this._object.paused;
};

QuickTimeCore.prototype.stop = function () {
  this._verifyObject();
  this._object.stop();
  return !(this._object.playing && this._object.paused);
};

QuickTimeCore.prototype.getPlaying = function () {
  this._verifyObject();
  var playing = this._object.playing || this._object.paused;
  return playing;
};

QuickTimeCore.prototype.getPaused = function () {
  this._verifyObject();
  return this._object.paused;
};

QuickTimeCore.prototype.getLength = function () {
  this._verifyObject();
  return this._object.length;
};

QuickTimeCore.prototype.getPosition = function () {
  this._verifyObject();
  return this._object.position;
};

QuickTimeCore.prototype.setPosition = function (aPos) {
  this._verifyObject();
  this._object.position = aPos;
};

QuickTimeCore.prototype.getVolume = function () {
  this._verifyObject();
  return this._object.volume;
};

QuickTimeCore.prototype.setVolume = function (aVolume) {
  this._verifyObject();
  this._object.volume = aVolume;
};

QuickTimeCore.prototype.getMute = function () {
  this._verifyObject();
  return this._object.muted;
};

QuickTimeCore.prototype.setMute = function (aMute) {
  this._verifyObject();
  this._object.muted = aMute;
};

QuickTimeCore.prototype.getMetadata = function (aKey) {
  this._verifyObject();
  return this._object.getMetadataForKey(aKey);
};

QuickTimeCore.prototype.goFullscreen = function () {
  this._verifyObject();
};

QuickTimeCore.prototype.isMediaURL = function(aURL) {
  this._verifyObject();
  var uri = newURI(aURL);
  if (!uri)
    return false;
  return this._object.isMediaURI(uri);
};

QuickTimeCore.prototype.isVideoURL = function (aURL) {
  this._verifyObject();
  var uri = newURI(aURL);
  if (!uri)
    return false;
  return this._object.isVideoURI(uri);
};

QuickTimeCore.prototype.getPlayingVideo = function () {
  this._verifyObject();
  return this._object.currentURIHasVideo;
};

QuickTimeCore.prototype.getSupportedFileExtensions = function () {
  this._verifyObject();
  return this._object.supportedFileTypes;
};

/**
  * See nsISupports.idl
  */
QuickTimeCore.prototype.QueryInterface = function(iid) {
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

/**
 * This is the function called from a document onload handler to bind
 * everything as playback.
 */
function QuickTimeCoreInit(aId, aSelect)
{
  var quicktime =
    Components.classes["@songbirdnest.com/Songbird/Playback/QuickTime;1"]
              .createInstance(Components.interfaces.sbIQuickTime);    
  var videoElement = document.getElementById(aId);
  quicktime.targetElement = videoElement;
  
  var coreWrapper = new QuickTimeCore();
  
  coreWrapper.setObject(quicktime);
  coreWrapper.setId("QuickTime1");

  var pps = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                      .getService(Components.interfaces.sbIPlaylistPlayback);
  var select = (aSelect == true) ? true : false;
  pps.addCore(coreWrapper, select);
  registeredCores.push(coreWrapper);
};
