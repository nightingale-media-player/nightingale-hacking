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

  this._mediaUrlExtensions = ["mp3", "ogg", "flac", "mpc", "wav", "m4a", "m4v",
                              "wmv", "asf", "avi",  "mov", "mpg", "mp4", "ogm",
                              "mp2", "mka", "mkv"];
  this._mediaUrlSchemes = ["mms", "rstp"];

  this._videoUrlExtensions = ["wmv", "asf", "avi", "mov", "mpg", "m4v", "mp4",
                              "mp2", "mpeg", "mkv", "ogm"];

  this._unsupportedExtensions = [];

  this._mediaUrlMatcher = new ExtensionSchemeMatcher(this._mediaUrlExtensions,
                                                     this._mediaUrlSchemes);
  this._videoUrlMatcher = new ExtensionSchemeMatcher(this._videoUrlExtensions,
                                                     []);
  this._uriChecker = null;
  this._lastPlayStart = null;
};

// inherit the prototype from CoreBase
CoreGStreamerSimple.prototype = new CoreBase();

// set the constructor so we use ours and not the one for CoreBase
CoreGStreamerSimple.prototype.constructor = CoreGStreamerSimple();

CoreGStreamerSimple.prototype.playURL = function ( aURL )
{
  this._verifyObject();
  this._checkURL(aURL);

  if (!aURL) {
    throw Components.results.NS_ERROR_INVALID_ARG;
  }

  aURL = this.sanitizeURL(aURL);
  this._paused = false;

  try
  {
    if(this._object.isPlaying || this._object.isPaused)
    {
      this._object.stop();
    }

    if (window.fullScreen)
    {
      window.fullScreen = !window.fullScreen;
      if (this._object.fullscreen) {
        this._object.fullscreen = false;
      }
    }

    var ioService =
      Components.classes["@mozilla.org/network/io-service;1"]
                .getService(Components.interfaces.nsIIOService);

    var uri = ioService.newURI(aURL, null, null);

    // If this is a local file, just play it
    if (uri instanceof Components.interfaces.nsIFileURL) {
      this._object.uri = aURL;
      this._object.play();
      this._lastPlayStart = new Date();
      return true;
    }
    else {
      // Resolve the network URL
      return this._resolveRedirectsAndPlay(uri);
    }

  }
  catch(e)
  {
      this.LOG(e);
  }

  return true;
};

CoreGStreamerSimple.prototype._resolveRedirectsAndPlay = function(aURI) {

  // Cancel any old checkers
  if (this._uriChecker) {
    this._uriChecker = null;
  }

  // Make a new uriChecker and initialize it
  var uriChecker =
    Components.classes["@mozilla.org/network/urichecker;1"]
              .createInstance(Components.interfaces.nsIURIChecker);
  uriChecker.init(aURI);

  // Save it away so we can cancel if necessary. Can't do this until after the
  // init call.
  this._uriChecker = uriChecker;

  // And begin the check
  uriChecker.asyncCheck(this, null);

  return true;
};

CoreGStreamerSimple.prototype.play = function ()
{
  this._verifyObject();

  this._paused = false;

  try
  {
    this._object.play();
    this._lastPlayStart = new Date();
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

CoreGStreamerSimple.prototype.getPlayingVideo = function ()
{
  this._verifyObject();
  return this._object.isPlayingVideo;
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

  var position = -1;
  try {
    position = this._object.position;
  }
  catch (e) {
    if(e.result != Components.results.NS_ERROR_NOT_AVAILABLE) {
      Components.util.reportError(e);
    }
  }

  if(this._object.lastErrorCode > 0) {
    curPos = -1;
  }
  else if(this._object.isAtEndOfStream || !this.getPlaying()) {
    curPos = 0;
  }
  else if(position > 0) {
    curPos = Math.round(position / (1000 * 1000));
  }
  else {
    // If bufferingPercent is > 0 and < 100, we know we are trying to play
    // a remote stream that is buffering.  While this is happening,
    // subsitute playback position for the number of milliseconds since play
    // was requested.  This will make playlistplayback think that the file
    // is playing and will prevent it from thinking that there is a problem.
    if (this._object.bufferingPercent > 0 && 
        this._object.bufferingPercent < 100) {
      curPos = (new Date()) - this._lastPlayStart;
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
      case "url": {
      // Special case for URL... Need to make sure we hand back a complete URI.
       var ioService =
          Components.classes[IOSERVICE_CONTRACTID].getService(nsIIOService);
        var uri;
        try {
          // See if it is a file, first.
          var file =
            Components.classes["@mozilla.org/file/local;1"]
                      .createInstance(Components.interfaces.nsILocalFile);
          file.initWithPath(this._object.uri);
          var fileHandler =
            ioService.getProtocolHandler("file")
                     .QueryInterface(Components.interfaces.nsIFileProtocolHandler);
          var url = fileHandler.getURLSpecFromFile(file);
          uri = ioService.newURI(url, null, null);
        }
        catch (err) { }
        
        if (!uri) {
          try {
            // See if it is a regular URI
            uri = ioService.newURI(this._object.uri, null, null);
          }
          catch (err) { };
        }
        
        if (uri)
          rv = uri.spec;
      }
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
  window.fullScreen=!window.fullScreen;
  if (!this._object.fullscreen) 
    this._object.fullscreen = true;
  else
    this._object.fullscreen = false;
};

  
CoreGStreamerSimple.prototype.isMediaURL = function ( aURL )
{
  return this._mediaUrlMatcher.match(aURL);
}

CoreGStreamerSimple.prototype.isVideoURL = function ( aURL )
{
  return this._videoUrlMatcher.match(aURL);
}

CoreGStreamerSimple.prototype.getSupportedFileExtensions = function ()
{
  return new StringArrayEnumerator(this._mediaUrlExtensions);
}

CoreGStreamerSimple.prototype.getSupportForFileExtension = function(aFileExtension)
{
  // Strip the beginning '.' if it exists and make it lowercase
  var extension =
    aFileExtension.charAt(0) == "." ? aFileExtension.slice(1) : aFileExtension;
  extension = extension.toLowerCase();
  
  // TODO: do something smarter here
  if (this._mediaUrlExtensions.indexOf(extension) > -1)
    return 1;
  else if (this._unsupportedExtensions.indexOf(extension) > -1)
    return -1;
  
  return 0; // We are the default handler for whomever.
};

CoreGStreamerSimple.prototype.onStartRequest = function(request, context)
{
  // Nothing to do here.
};

CoreGStreamerSimple.prototype.onStopRequest = function(request, context, status)
{
  if (request != this._uriChecker) {
    return;
  }

  if (status == NS_BINDING_SUCCEEDED) {
    // Clear immediately so we can't try to cancel it anymore
    this._uriChecker = null;

    var uriChecker =
      request.QueryInterface(Components.interfaces.nsIURIChecker);
    var url = uriChecker.baseChannel.URI.spec;

    this._url = url;
    this._object.uri = url;
    this._object.play();
    this._lastPlayStart = new Date();
  }
};

/**
  * See nsISupports.idl
  */
CoreGStreamerSimple.prototype.QueryInterface = function(iid) {
  if (!iid.equals(Components.interfaces.sbICoreWrapper) &&
      !iid.equals(nsIRequestObserver) &&
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
  dump("ERROR!!! coreGStreamerSimple failed to create properly.");
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
    registeredCores.push(gGStreamerSimpleCore);
  }
  catch ( err )
  {
    dump( "\n!!! coreGStreamerSimple failed to bind properly\n" + err );
  }
};

