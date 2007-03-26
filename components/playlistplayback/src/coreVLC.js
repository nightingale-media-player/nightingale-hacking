/**
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
 * 
 */
function getPlatformString() {
  try {
    var sysInfo =
      Components.classes["@mozilla.org/system-info;1"]
                .getService(Components.interfaces.nsIPropertyBag2);
    return sysInfo.getProperty("name");
  }
  catch (e) { }
  
  var userAgent = navigator.userAgent;
  if (userAgent.indexOf("Windows") != -1)
    return "Windows_NT";
  
  return userAgent;
}

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
 * Hacks apart a URI to make it acceptable for VLC
 */
function getVLCURLFromURI(aURI) {
  if (!aURI)
    return null;
  
  // Non-file URIs are probably fine as they are.
  if (!(aURI instanceof Components.interfaces.nsIFileURL))
    return aURI.spec;

  var fileURL = aURI.QueryInterface(Components.interfaces.nsIFileURL);
  
  // Special samba case. Mozilla gives us "file://///" and VLC only understands
  // "smb://".
  var smbPath = fileURL.path;
  if (smbPath.slice(0, 3) ==  "///") {
    var newSpec = "smb:" + smbPath.slice(1);

    // Validate the new spec
    var uri = newURI(newSpec);
    return uri.spec;
  }
  
  var file = fileURL.file;
  file.normalize();
  
  // Normal file for Windows. Mozilla gives us "file:///" and VLC wants a
  // filesystem path like "C:\fun.mp3".
  if (getPlatformString == "Windows_NT")
    return file.path;
  
  // Default case. Rebuild the uri to avoid "file://localhost/" messes.
  var ioService =
    Components.classes["@mozilla.org/network/io-service;1"]
              .getService(Components.interfaces.nsIIOService);
  var fileHandler =
    ioService.getProtocolHandler("file")
             .QueryInterface(Components.interfaces.nsIFileProtocolHandler);
  return fileHandler.getURLSpecFromFile(file);
}

/**
 * \class CoreVLC
 * \brief The CoreWrapper for the VLC Plugin
 * \sa CoreBase
 */
 
function CoreVLC()
{
  this._startTime = 0;
  this._lastCalcTime = 0;
  this._fileTime = 0;
  this._savedTime = 0;
  
  this._object = null;
  this._id = "";
  this._muted = false;
  this._url = "";
  this._paused = false;

  this._mediaUrlExtensions = ["mp3", "ogg", "flac", "mpc", "wav", "aac", "m4a", "m4v",
                              "wma", "wmv", "asx", "asf", "avi",  "mov", "mpg",
                              "mp4", "mp2", "mpeg", "mpga", "mpega", "mkv",
                              "mka", "ogm", "mpe", "qt", "aiff", "aif"];
  this._mediaUrlSchemes = ["mms", "rstp"];

  this._videoUrlExtensions = ["wmv", "asx", "asf", "avi", "mov", "qt", "mpg",
                              "m4v", "mp4", "mp2", "mpeg", "mpe", "mkv", "ogm"];

  this._mediaUrlMatcher = new ExtensionSchemeMatcher(this._mediaUrlExtensions,
                                                     this._mediaUrlSchemes);
  this._videoUrlMatcher = new ExtensionSchemeMatcher(this._videoUrlExtensions,
                                                     []);
};

// Enumerate vlc.input.state options
CoreVLC.INPUT_STATES = {
  IDLE:       0,
  OPENING:    1,
  BUFFERING:  2,
  PLAYING:    3,
  PAUSED:     4,
  STOPPING:   5
};

// inherit the prototype from CoreBase
CoreVLC.prototype = new CoreBase();

// set the constructor so we use ours and not the one for CoreBase
CoreVLC.prototype.constructor = CoreVLC();

CoreVLC.prototype.playURL = function (aURL)
{
  this._verifyObject();

  this._fileTime = 0;
  this._lastCalcTime = 0;
  this._startTime = 0;

  if (!aURL)
    throw Components.results.NS_ERROR_INVALID_ARG;
  this.LOG("theURL: " + aURL);
  
  var uri = newURI(aURL);
  if (!uri)
    return false;
  
  this._url = getVLCURLFromURI(uri);
  
  //Encode + signs since VLC will try and decode those as spaces. 
  //Even though they are *VALID* characters for a filename as per URI specifications. :(
  this._url = this._url.replace(/\+/g, '%2b');

  this._object.playlist.clear();
  var item = this._object.playlist.add(this._url);
  this._object.playlist.playItem(item);
  
  this._lastCalcTime = new Date();
  this._startTime = this._lastCalcTime.getTime();
  
  dump("\ncoreVLC playing " + this._url + " as item " + item + "\n");
  
  if (this._object.playlist.isPlaying) 
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

  if (this._object.playlist.itemCount <= 0) 
    return false;

  this._object.playlist.play();

  if(this._paused && this._savedTime)
  {
    var currentTime = new Date();
    
    this._startTime = currentTime.getTime() -  this._savedTime;
    this._savedTime = 0;
  }

  this._paused = false;

  return true;
};
  
CoreVLC.prototype.stop = function() 
{
  this._verifyObject();

  if (this._object.playlist.itemCount > 0) 
    this._object.playlist.stop();

  this._paused = false;
  this._fileTime = 0;
  this._lastCalcTime = 0;
  this._startTime = 0;
  this._savedTime = 0;

  return this._object.playlist.isPlaying == false;
};
  
CoreVLC.prototype.pause = function()
{
  if (this._paused)
    return false;
    
  this._verifyObject();
  this._object.playlist.togglePause();
  
  if (this._object.playlist.isPlaying)
    return false;
    
  this._paused = true;

  if(this._fileTime)
  {
    var currentTime = new Date();
    this._savedTime = currentTime.getTime() - this._startTime;
  }

  return true;
};

CoreVLC.prototype.getPaused = function() 
{
  return this._paused;
};

CoreVLC.prototype.getPlaying = function() 
{
  this._verifyObject();
  return this._object.playlist.isPlaying || this._paused;
};

CoreVLC.prototype.getPlayingVideo = function ()
{
  this._verifyObject();
  var hasVout = false;

  try {
    hasVout = this._object.input.hasVout;
  } catch(err) {}

  return hasVout;
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
    this._object.audio.mute = mute;
  }
};

CoreVLC.prototype.getVolume = function() 
{
  this._verifyObject();

  /**
  * Valid volumes are from 0 to 255.
  * VLC uses a 0-49 scale, so volumes are adjusted accordingly.
  * If you going beyond 49 VLC will amplify the signal.
  * And it does so poorly, without clipping or compressing the signal.
  */
  var scaledVolume = this._object.audio.volume;
  var retVolume = Math.round(scaledVolume / 49 * 255);
  
  return retVolume;
};

CoreVLC.prototype.setVolume = function(volume) 
{
  this._verifyObject();
  if ( (volume < 0) || (volume > 255) )
    throw Components.results.NS_ERROR_INVALID_ARG;
    
  var scaledVolume = Math.round(volume / 255 * 49);
  
  this._object.audio.volume = scaledVolume;
};
  
CoreVLC.prototype.getLength = function() 
{
  this._verifyObject();

  if (this._object.playlist.itemCount <= 0 
	  || this._object.input.state == CoreVLC.INPUT_STATES.IDLE)
    return null;

  if(this._fileTime)
    return this._fileTime;
    
  return this._object.input.length;
};

CoreVLC.prototype.getPosition = function() 
{
  this._verifyObject();

  if (this._object.playlist.itemCount <= 0)
    return 0; 
		
	var currentPos, currentPosTime;
	
	// VLC will throw an exception if there is no active input. Catch this and
	// just return 0.
	try {
	  var input = this._object.input;
	  if (input.state == CoreVLC.INPUT_STATES.IDLE)
	    return 0;

    currentPos = input.position;
    currentPosTime = input.time;
	}
	catch (err) {
	  return 0;
	}
	
  if(currentPos < 1 && currentPosTime == 0)
  {
    // Sometimes the player loop will call this after we've stopped.
    if (!this._startTime)
      return 0;
    
    if(!this._paused)
    {
      var currentTime = new Date();
      var deltaTime = currentTime.getTime() - this._startTime;
      
      if( currentPos > 0  && (currentTime.getTime() - this._lastCalcTime.getTime() > 5000))
      {
        var posMul = 1 / currentPos;
        this._fileTime = posMul * deltaTime;
        this._lastCalcTime = currentTime;
      }
      
      currentPos = deltaTime;
    }
    else
    {
      currentPos = this._savedTime;
    }
  }
  else
  {
    this._fileTime = 0;
    currentPos = currentPosTime;
  }
  
  return currentPos;
};

CoreVLC.prototype.setPosition = function(position) 
{
  this._verifyObject();

  if (this._object.playlist.itemCount <= 0 
      || this._object.input.state == CoreVLC.INPUT_STATES.IDLE)
    return null;

  if (this._object.playlist.itemCount > 0)
    this._object.input.time = position; 
};

CoreVLC.prototype.goFullscreen = function() 
{
  this._verifyObject();
  return this._object.video.fullscreen = true;
};

CoreVLC.prototype.getMetadata = function(key)
{
  var retval = "";

  var metaInfoCat = "Meta-information";
  var genInfoCat = "General";

  // Make sure that the there are items in the playlist,
  // and the player is either playing or paused
  if (this._object.playlist.itemCount <= 0  
      ||  ((this._object.input.state != CoreVLC.INPUT_STATES.PLAYING) 
           && (this._object.input.state != CoreVLC.INPUT_STATES.PAUSED)))
    return retval;

  try {
    switch ( key ) {
      case "album":
        if(this._verifyCategoryKey(metaInfoCat, "Album/movie/show title"))
          retval += this._object.metadata.get( metaInfoCat,
                           "Album/movie/show title" );
      break;

      case "artist":
        if(this._verifyCategoryKey(metaInfoCat, "Now Playing"))
        {
          retval += this._object.metadata.get( metaInfoCat, "Now Playing" );
        }
        else if(this._verifyCategoryKey(metaInfoCat, "Artist"))
        {
          retval += this._object.metadata.get( metaInfoCat, "Artist" );
        }
      break;

      case "genre":
        if(this._verifyCategoryKey(metaInfoCat, "Genre"))
          retval += this._object.metadata.get( metaInfoCat, "Genre" );
      break;

      case "length":
        if(this._verifyCategoryKey(genInfoCat, "Duration"))
          retval += this._object.metadata.get( genInfoCat, "Duration" );
      break;

      case "title":
        if(this._verifyCategoryKey(metaInfoCat, "Title"))
          retval += this._object.metadata.get( metaInfoCat, "Title" );
      break;

      case "url":
        var ioService =
          Components.classes[IOSERVICE_CONTRACTID].getService(nsIIOService);
        var uri;
        try {
          // See if it is a file, first.
          var file =
            Components.classes["@mozilla.org/file/local;1"]
                      .createInstance(Components.interfaces.nsILocalFile);
          file.initWithPath(this._url);
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
            uri = ioService.newURI(this._url, null, null);
          }
          catch (err) { };
        }
        
        if (uri)
          retval += uri.spec;
      break;
    } 
  } catch (e) {
    dump("\ncoreVLC.getMetadata Error: " + e + "\n");
  }
  return retval;
};

CoreVLC.prototype._verifyCategoryKey = function (cat, key)
{
  var validKey = false;

  var categories = this._object.metadata.categories;
  var categoryCount = categories.length;
  var curCat = 0;

  for(; curCat < categoryCount; curCat++)
  {
    if(categories[curCat] == cat)
    {
      var categoryKeys = this._object.metadata.getCategoryKeys(cat);
      var catKeyCount = categoryKeys.length;
      var curKey = 0;

      for(; curKey < catKeyCount; curKey++)
      {
        if(categoryKeys[curKey] == key)
        {
          validKey = true;
          break;
        }
      }
    }
  }

  return validKey;
};

CoreVLC.prototype.isMediaURL = function( aURL )
{
  return this._mediaUrlMatcher.match(aURL);
}

CoreVLC.prototype.isVideoURL = function ( aURL )
{
  return this._videoUrlMatcher.match(aURL);
}

CoreVLC.prototype.getSupportedFileExtensions = function ()
{
  return new StringArrayEnumerator(this._mediaUrlExtensions);
}

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
  dump("ERROR!!! coreVLC failed to create properly.");
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
 
