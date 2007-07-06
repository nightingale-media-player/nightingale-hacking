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

var Cr = Components.results;
var Ci = Components.interfaces;
var Cc = Components.classes;

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
  if (getPlatformString() == "Windows_NT")
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
  this._uri = null;
  this._paused = false;
  this._fakePosition = false;
  this._ignorePrefChange = false;

  this._mediaUrlExtensions = ["mp3", "ogg", "flac", "mpc", "wav", "aac", "mva",
                              "wma", "wmv", "asx", "asf", "avi",  "mov", "mpg",
                              "mp4", "mp2", "mpeg", "mpga", "mpega", "mkv",
                              "mka", "ogm", "mpe", "qt", "aiff", "aif", "m4a",
                              "m4v"];
  this._mediaUrlSchemes = ["mms", "rstp"];

  this._videoUrlExtensions = ["wmv", "asx", "asf", "avi", "mov", "qt", "mpg",
                              "m4v", "mp4", "mp2", "mpeg", "mpe", "mkv", "ogm"];

  this._unsupportedExtensions = [];

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

// apply custom preferences to vlc
CoreVLC.prototype._applyPreferences = function ()
{
  var log = this._object.log;
  var config = this._object.config;

  try {
    log.verbosity = 1;
  }
  catch(e) {
    this.LOG("failed to set log verbosity. log messages disabled.");
  }

  //Be more generous about file caching.
  try {
    //XXXAus: Read this value from prefs (bug #699)
    config.setConfigInt("access_file", "file-caching", 1000);
  }
  catch(e) {
    this.LOG("access_file module is missing, can't set config item.");
  }
  
  //Be more generous about http caching.
  try {
    //XXXAus: Read this value from prefs (bug #699)
    config.setConfigInt("access_http", "http-caching", 2000);
  }
  catch(e) {
    this.LOG("access_http module is missing, can't set config item.");
  }
  
  //Be more generous about ftp caching.
  try {
    //XXXAus: Read this value from prefs (bug #699)
    config.setConfigInt("access_ftp", "ftp-caching", 2000);
  }
  catch(e) {
    this.LOG("access_ftp module is missing, can't set config item.");
  }
  
  //Be more generous about smb caching.
  try {
    //XXXAus: Read this value from prefs (bug #699)
    config.setConfigInt("access_smb", "smb-caching", 2000);
  }
  catch(e) {
    this.LOG("access_smb module is missing, can't set config item.");
  }
  
  //Automatically reconnect if http connection lost.
  try {
    //XXXAus: Read this value from prefs (bug #699)
    config.setConfigBool("access_http", "http-reconnect", 1);
  }
  catch(e) {
    this.LOG("access_http module is missing, can't set config item.");
  }
  
  //Turn on volume normalization.
  try {
    //XXXAus: Read these values from prefs (bug #699)
    config.setConfigString("main", "audio-filter", "volnorm");
    config.setConfigInt("normvol", "norm-buff-size", 99);
    config.setConfigFloat("normvol", "norm-max-level", 5.123456789123);
  }
  catch(e) {
    this.LOG("normvol module is missing, can't set config item.");
  }
  
  //Set user agent, read from moz prefs.
  //config.setConfigString("access_http", "http-user-agent", "Songbird");

  //Be very flexible about SSL certificates. Typically self signed certs
  //are used by average users and services.
  try {
    //XXXAus: Read these values from prefs (bug #699)
    config.setConfigBool("gnutls", "tls-check-cert", 0);
    config.setConfigBool("gnutls", "tls-check-hostname", 0);
    config.setConfigInt("gnutls", "gnutls-cache-expiration", 12000);
    config.setConfigInt("gnutls", "gnutls-cache-size", 128);
  }
  catch(e) {
    this.LOG("gnutls module is missing, can't set config item.");
  }
  
  if(getPlatformString() == "Windows_NT") {
    try {
      var prefsService = Cc["@mozilla.org/preferences-service;1"]
                        .getService(Ci.nsIPrefService);
      
      var prefBranch = prefsService.QueryInterface(Ci.nsIPrefBranch2);
      
      prefBranch.addObserver("songbird.mediacore.audioOut", this, false);
      
      var audioOut = prefBranch.getCharPref("songbird.mediacore.audioOut");
      this._setAudioOutput(audioOut);      
    }
    catch(e) {
      this.LOG(e);
    }
  }
};

CoreVLC.prototype._setAudioOutput = function(aAudioOutputName)
{
  switch(aAudioOutputName) {
    case "directsound":
      this._setAudioOutputDirectSound();
      return true;
    break;
    
    case "waveout":
      this._setAudioOutputWaveOut();
      return true;
    break;
    
    default:
      ;
  }
  
  return false;
};

CoreVLC.prototype._setAudioOutputWaveOut = function()
{
  if(getPlatformString() == "Windows_NT") {
    this._object.config.setConfigString("main", "aout", "waveout");
  }
};

CoreVLC.prototype._setAudioOutputDirectSound = function()
{
  if(getPlatformString() == "Windows_NT") {
    this._object.config.setConfigString("main", "aout", "aout_directx");
  }
};

CoreVLC.prototype._setProxyForURI = function (aURI)
{
  if(aURI.scheme == "http") {
    
    var prefsService = Cc["@mozilla.org/preferences-service;1"]
                       .getService(Ci.nsIPrefService);
    
    var prefBranch = prefsService.getBranch("network.")
                        .QueryInterface(Ci.nsIPrefBranch);
    
    var prefValue;
    
    var hostName = "";
    var hostPort = 0;
    
    var prefHost = "";
    var prefPort = "";
    
    var hostScheme = "";
    
    switch(aURI.scheme) {
      case "http":
        prefHost = "proxy.http";
        prefPort = "proxy.http_port";
        hostScheme = "http://";
      break;
      
      //If you were to add another protocol, this is how you would do it.
      /*
      case "https":
        prefHost = "proxy.ssl";
        prefPort = "proxy.ssl_port";
        hostScheme = "https://";
      break;
      */
      
      default:
        return;
    }

    prefValue = prefBranch.getCharPref(prefHost);
    if(prefValue != "") {
      hostName = prefValue;
    }
    prefValue = prefBranch.getIntPref(prefPort);
    if(prefValue != 0) {
      hostPort = prefValue;
    }
    
    if(hostName != "" && hostPort != 0) {
      this._setProxyInfo(hostScheme + hostName, hostPort);
    }    
  }
};

CoreVLC.prototype._setProxyInfo = function (aProxyHost, aProxyPort, aProxyUser, aProxyPassword)
{
  if(!aProxyHost ||
     !aProxyHost.length) {
    return;
  }

  var actualHost = aProxyHost;
  var port = parseInt(aProxyPort);
  
  if(!isNaN(port)) {
    actualHost = actualHost + ":" + aProxyPort;
  }
  
  if(aProxyUser && aProxyUser.length && 
     aProxyPassword && aProxyPassword.length) {
    actualHost = aProxyUser + ":" + aProxyPassword + "@" + actualHost;
  }
  
  //Set proxy host.
  this._object.config.setConfigString("access_http", "http-proxy", actualHost);
};

CoreVLC.prototype._getLoginInfoForURI = function(aURI)
{
  var httpAuthManager = Cc["@mozilla.org/network/http-auth-manager;1"]
                      .getService(Ci.nsIHttpAuthManager);
  
  var userDomain = {};
  var userName = {};
  var userPassword = {};
  
  try {
    httpAuthManager.getAuthIdentity(aURI.scheme,
                                    aURI.host,
                                    aURI.port == -1 ? 80 : aURI.port,
                                    "basic",
                                    "",
                                    aURI.path,
                                    userDomain,
                                    userName,
                                    userPassword);
                                    
    return [userName.value, userPassword.value];
  }
  catch(e) {
    this.LOG(e);
  }

  //Failed to get the info using http auth manager. Prompt user for information.
  var windowWatcher = Cc["@mozilla.org/embedcomp/window-watcher;1"]
                        .getService(Ci.nsIWindowWatcher);
  
  var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"]
                        .getService(Ci.nsIPromptService);

  var checkState = {};
  var message = SBString("message.requireauth", 
    "Enter username and password to access the following location - ");
  
  message = message + "\n" + aURI.scheme + "://" + aURI.host + aURI.path;
    
  var hasInfo = promptService.promptUsernameAndPassword(
                                          windowWatcher.activeWindow,
                                          null,
                                          message,
                                          userName, 
                                          userPassword,
                                          null,
                                          checkState);
                                            
  return hasInfo ? [userName.value, userPassword.value] : [];
};

/**
 * \brief This function is meant to attempt to process
 * log messages received from VLC and interpret
 * their content with the intent of trying to recover
 * from errors during playURL attempts.
 */
CoreVLC.prototype._processLogMessagesForPlayURL = function()
{
  this._verifyObject();

  var messages = this._object.log.messages;
  
  if(messages.count < 1) {
    return true;
  }
 
  var messageIterator = messageIterator = messages.iterator();

  var message = null;  
  while(messageIterator.hasNext) {
    message = messageIterator.next();
    
    //Try and see if we got a 401 and bring up an auth box.  
    if(message.message.match("'HTTP' answer code 401")) {
      
      var loginInfos = this._getLoginInfoForURI(this._uri);
      
      //Found a username / password pair.
      if(loginInfos.length == 2) {
        var uriWithLogin = this._uri.clone();
      
        //Prevent PlaylistPlayback from going to the next track.
        this._fakePosition = true;

        uriWithLogin.username = loginInfos[0];
        uriWithLogin.password = loginInfos[1];
        
        this.playURL(uriWithLogin.spec);
      } 
    }
    
    message = null;
  }
  
  messages.clear();

  return true;
};

CoreVLC.prototype.observe = function (aSubject, aTopic, aData) 
{
  if(this._ignorePrefChange) {
    return;
  }

  var prefBranch = aSubject.QueryInterface(Ci.nsIPrefBranch);
  var audioOut = prefBranch.getCharPref("songbird.mediacore.audioOut");

  try {
    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
              .getService(Ci.nsIWindowMediator);
    var mainWin = wm.getMostRecentWindow("Songbird:Main");
    
    if(this._setAudioOutput(audioOut)) {
      if(mainWin) {
        mainWin.sbRestartBox_strings("message.mediacore", 
                                     "message.needrestart", 
                                     "Media Core", 
                                     "This setting will take effect after you restart Songbird");
      }
    }
    else {
      //Alert user.
      mainWin.alert("'directsound' or 'waveout' only.");

      //Ignore next callback
      this._ignorePrefChange = true;
      
      //Reset to default.
      prefBranch.clearUserPref("songbird.mediacore.audioOut");
      
      this._ignorePrefChange = false;
    }
  }
  catch(e) { 
    this.LOG(e);
  }

  return;
};

CoreVLC.prototype.destroyCoreVLC = function() 
{
  var prefsService = Cc["@mozilla.org/preferences-service;1"]
                  .getService(Ci.nsIPrefService);

  var prefBranch = prefsService.getBranch("songbird.mediacore.")
                    .QueryInterface(Ci.nsIPrefBranch2);

  prefBranch.removeObserver("audioOut", this);
};

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

  // Figure out if this URI can use a proxy, if so, try and get the proper
  // proxy information from the preferences and set them in VLC.
  // VLC only supports HTTP, HTTPS proxy through the access_http module.
  this._setProxyForURI(uri);

  this._uri = uri;
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
  
  if(this._fakePosition) {
    return true;
  }
  
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
  
  //This is here because our CoreWrapper interface doesn't have "Run" or "Step"
  //which would be useful to run a state machine that processes feedback from 
  //the playback core.
  this._processLogMessagesForPlayURL();

  if (this._object.playlist.itemCount <= 0) {
    return 0;
  }
		
	var currentPos = 0, currentPosTime = 0;
	
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

  if(!this._fakePosition) {
    if(currentPos < 1 && currentPosTime == 0) {
      
      // Sometimes the player loop will call this after we've stopped.
      if (!this._startTime)
        return 0;
      
      if(!this._paused) {
        var currentTime = new Date();
        var deltaTime = currentTime.getTime() - this._startTime;
        
        if( currentPos > 0  && 
            (currentTime.getTime() - this._lastCalcTime.getTime() > 5000)) {
          var posMul = 1 / currentPos;
          this._fileTime = posMul * deltaTime;
          this._lastCalcTime = currentTime;
        }
        
        currentPos = deltaTime;
      }
      else {
        currentPos = this._savedTime;
      }
    }
    else {
      this._fileTime = 0;
      currentPos = currentPosTime;
    }
  }
 
  if (this._fakePosition && 
      currentPos == 0) {
    return 1;
  }
  else {
    this._fakePosition = false;
  }

  return currentPos;
};

CoreVLC.prototype.setPosition = function(position) 
{
  this._verifyObject();

  if (this._object.playlist.itemCount <= 0 
      || this._object.input.state == CoreVLC.INPUT_STATES.IDLE)
    return;

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
  this._verifyObject();
  
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
};

CoreVLC.prototype.isVideoURL = function ( aURL )
{
  return this._videoUrlMatcher.match(aURL);
};

CoreVLC.prototype.getSupportedFileExtensions = function ()
{
  return new StringArrayEnumerator(this._mediaUrlExtensions);
};

CoreVLC.prototype.activate = function ()
{
  if (this._active)
    return;
  
  this._verifyObject();
  try {
    var videoElement =
      this._object.QueryInterface(Components.interfaces.nsIDOMElement);
    videoElement.removeAttribute("hidden");
  }
  catch (err) { }
  
  this._active = true;
};

CoreVLC.prototype.deactivate = function ()
{
  if (!this._active)
    return;
  
//  this.stop();
  
  var videoElement =
    this._object.QueryInterface(Components.interfaces.nsIDOMElement);
  videoElement.setAttribute("hidden", true);
  
  this._active = false;
};

CoreVLC.prototype.getSupportForFileExtension = function(aFileExtension)
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

CoreVLC.prototype.QueryInterface = function(iid) 
{
  if (!iid.equals(Ci.sbICoreWrapper) &&
      !iid.equals(Ci.nsIObserver) &&
      !iid.equals(Ci.nsISupports))
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
  window.addEventListener("unload", gCoreVLC.destroyCoreVLC, false);
}
catch (err) {
  dump("ERROR!!! coreVLC failed to create properly.\n");
}

/**
  * This is the function called from a document onload handler to bind everything as playback.
  * The <html:object>s won't have their scriptable APIs attached until the onload.
  */
function CoreVLCDocumentInit()
{
  try
  {
    var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                         .getService(Components.interfaces.sbIPlaylistPlayback);
    var theVLCInstance = document.getElementById( "core_vlc" );

    gCoreVLC.setId("VLC1");
    gCoreVLC.setObject(theVLCInstance);
    theVLCInstance.setAttribute("hidden", "true");
    
    // apply prefs to playback core
    gCoreVLC._applyPreferences();

    gPPS.addCore(gCoreVLC, true);
  }
  catch ( err )
  {
    dump( "\n!!! coreVLC failed to bind properly\n" + err );
  }
}

window.addEventListener("load", CoreVLCDocumentInit, false);
