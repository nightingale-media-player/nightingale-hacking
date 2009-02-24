/*
Copyright (c) 2008, Pioneers of the Inevitable, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of Pioneers of the Inevitable, Songbird, nor the names
    of its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// these constants make everything better
const Cc = Components.classes;
const CC = Components.Constructor;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

// our namespace
const NS = 'http://www.songbirdnest.com/lastfm#'
const SB_NS = 'http://songbirdnest.com/data/1.0#'
const SP_NS = 'http://songbirdnest.com/rdf/servicepane#'

// our annotations
const ANNOTATION_SCROBBLED = NS+'scrobbled';
const ANNOTATION_HIDDEN = NS+'hidden';
const ANNOTATION_LOVE = NS+'love';
const ANNOTATION_BAN = NS+'ban';

// our properties
const PROPERTY_TRACKID = NS+'trackid';

// their namespace
const LASTFM_NS = 'http://www.audioscrobbler.net/dtd/xspf-lastfm'
// their properties
const PROPERTY_TRACKAUTH = LASTFM_NS+'trackauth';
const PROPERTY_ALBUMID = LASTFM_NS+'albumid';
const PROPERTY_ARTISTID = LASTFM_NS+'artistid';

// Last.fm API key, secret and URL
const API_KEY = '4d5bce1e977549f10623b51dd0e10c5a';
const API_SECRET = '3ebb03d4561260686b98388037931f11'; // obviously not secret
const API_URL = 'http://ws.audioscrobbler.com/2.0/';

// handshake failure types
const HANDSHAKE_FAILURE_AUTH = true;
const HANDSHAKE_FAILURE_OTHER = false;

// how often should we try to scrobble again?
const TIMER_INTERVAL = (5*60*1000); // every five minutes sounds lovely

// what should we call the radio library
const RADIO_LIBRARY_FILENAME = 'lastfm-radio.db';

// import the XPCOM helper
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
// import the properites helper
Cu.import("resource://app/jsmodules/sbProperties.jsm");
// import the library utils
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
// import dataremote utils
Cu.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");
// import array utils
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");

// object to manage login state
var Logins = {
  loginManager: Cc["@mozilla.org/login-manager;1"]
      .getService(Ci.nsILoginManager),

  LOGIN_HOSTNAME: 'https://www.last.fm',
  LOGIN_FORMURL: 'https://www.last.fm',
  LOGIN_FIELD_USERNAME: 'username',
  LOGIN_FIELD_PASSWORD: 'password',

  get: function() {
    // username & password
    var username = '';
    var password = '';
    // lets ask the login manager
    var logins = this.loginManager.findLogins({}, this.LOGIN_HOSTNAME,
                                              this.LOGIN_FORMURL, null);
    for (var i = 0; i < logins.length; i++) {
      if (i==0) {
        // use the first username & password we find
        username = logins[i].username;
        password = logins[i].password;
      } else {
        // get rid of the rest
        this.loginManager.removeLogin(logins[i]);
      }
    }
    return {username: username, password: password};
  },

  set: function(username, password) {
    var logins = this.loginManager.findLogins({}, this.LOGIN_HOSTNAME,
                                              this.LOGIN_FORMURL, null);
    for (var i=0; i<logins.length; i++) {
      this.loginManager.removeLogin(logins[i]);
    }
    // set new login info
    var nsLoginInfo = new CC("@mozilla.org/login-manager/loginInfo;1",
      Ci.nsILoginInfo, "init");
    this.loginManager.addLogin(new nsLoginInfo(this.LOGIN_HOSTNAME,
        this.LOGIN_FORMURL, null, username, password,
        this.LOGIN_FIELD_USERNAME, this.LOGIN_FIELD_PASSWORD));
  }
}


// helper for enumerating enumerators. duh.
function enumerate(enumerator, func) {
  while(enumerator.hasMoreElements()) {
    try {
      func(enumerator.getNext());
    } catch(e) {
      Cu.reportError(e);
    }
  }
}

// create an nsIURI from a string
function newURI(spec) {
  var ioService = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  return ioService.newURI(spec, null, null);
}

// calculate a hex md5 digest thing
function md5(str) {
  var converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
      .createInstance(Ci.nsIScriptableUnicodeConverter);

  converter.charset = "UTF-8";
  // result is an out parameter,
  // result.value will contain the array length
  var result = {};
  // data is an array of bytes
  var data = converter.convertToByteArray(str, result);
  var ch = Cc["@mozilla.org/security/hash;1"].createInstance(Ci.nsICryptoHash);
  ch.init(ch.MD5);
  ch.update(data, data.length);
  var hash = ch.finish(false);

  // return the two-digit hexadecimal code for a byte
  function toHexString(charCode) {
    return ("0" + charCode.toString(16)).slice(-2);
  }

  // convert the binary hash data to a hex string.
  var s = [toHexString(hash.charCodeAt(i)) for (i in hash)].join("");
  return s;
}

// urlencode an object's keys & values
function urlencode(o) {
  var s = '';
  for (var k in o) {
    var v = o[k];
    if (s.length) { s += '&'; }
    s += encodeURIComponent(k) + '=' + encodeURIComponent(v);
  }
  return s;
}

// make an HTTP POST request
function POST(url, params, onload, onerror) {
  var xhr = null;
  try {
    // create the XMLHttpRequest object
    xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance();
    // don't tie it to a XUL window
    xhr.mozBackgroundRequest = true;
    // set up event handlers
    xhr.onload = function(event) { onload(xhr); }
    xhr.onerror = function(event) { onerror(xhr); }
    // open the connection to the url
    xhr.open('POST', url, true);
    // we're always sending url encoded parameters
    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    // send url encoded parameters
    xhr.send(urlencode(params));
    // pass the XHR back to the caller
  } catch(e) {
    Cu.reportError(e);
    onerror(xhr);
  }
  return xhr;
}

// An object to track listeners
function Listeners() {
  var listeners = [];
  this.add = function Listeners_add(aListener) {
    listeners.push(aListener);
  }
  this.remove = function Listeners_remove(aListener) {
    for(;;) {
      // find our listener in the array
      let i = listeners.indexOf(aListener);
      if (i >= 0) {
        // remove it
        listeners.splice(i, 1);
      } else {
        return;
      }
    }
  }
  this.each = function Listeners_each(aCallback) {
    for (var i=0; i<listeners.length; i++) {
      try {
        aCallback(listeners[i]);
      } catch(e) {
        Cu.reportError(e);
      }
    }
  }
}

// an object that represents a played track for sending to Last.fm
function PlayedTrack(mediaItem, timestamp, rating, source) {
  // apply defaults
  if (!rating) rating = ''; // no rating
  if (!source) source = 'P'; // user picked

  // copy properties out of the media item
  this.a = mediaItem.getProperty(SBProperties.artistName);
  this.t = mediaItem.getProperty(SBProperties.trackName);
  this.b = mediaItem.getProperty(SBProperties.albumName);
  this.n = mediaItem.getProperty(SBProperties.trackNumber);
  this.l = Math.round(parseInt(
      mediaItem.getProperty(SBProperties.duration))/1000000);
  // pull the musicbrainz id when we have a standard sb property for that...
  this.m = '';

  // attach info that was passed in
  this.r = rating;
  this.o = source;
  this.i = timestamp;
}

function findRadioNode(node, radioString) {
	if (node.isContainer && node.name != null &&
		((radioString && node.name == radioString)
		 || (node.getAttributeNS(SB_NS, "radioFolder") == 1)))
	{
		node.setAttributeNS(SB_NS, "radioFolder", 1);
		return node;
	}

	if (node.isContainer) {
		var children = node.childNodes;
		while (children.hasMoreElements()) {
			var child =
					children.getNext().QueryInterface(Ci.sbIServicePaneNode);
			var result = findRadioNode(child, radioString);
			if (result != null)
				return result;
		}
	}
	return null;
}

function sbLastFm() {
  // our interface is really lightweight - make the service available as a JS
  // object so we can avoid the IDL / XPConnect complexity.
  this.wrappedJSObject = this;

  // keep track of our listeners
  this.listeners = new Listeners();

  // get the username & password
  var login = Logins.get();
  this.username = login.username;
  this.password = login.password;

  // make the API key available
  this.apiKey = API_KEY;

  // session info
  this.session = null;
  this.nowplaying_url = null;
  this.submission_url = null;

  // love/ban state
  // the mediaitem guid that is being loved or banned
  this.loveTrack = null;
  // loved (true) or banned (false)
  this.love = false;

  // hard failure count
  this.hardFailures = 0;

  // the session key used by the last.fm "rest" web service
  this.sk = null;

  // the should-we-scrobble pref
  var prefsService = Cc['@mozilla.org/preferences-service;1']
      .getService(Ci.nsIPrefBranch);
  this.__defineGetter__('shouldScrobble', function() {
    return prefsService.getBoolPref('extensions.lastfm.scrobble');
  });
  this.__defineSetter__('shouldScrobble', function(val) {
    prefsService.setBoolPref('extensions.lastfm.scrobble', val);
    this.listeners.each(function(l) { l.onShouldScrobbleChanged(val); });
  });

  // user-logged-out pref
  this.__defineGetter__('userLoggedOut', function() {
    return prefsService.getBoolPref('extensions.lastfm.loggedOut');
  });
  this.__defineSetter__('userLoggedOut', function(val) {
    prefsService.setBoolPref('extensions.lastfm.loggedOut', val);
    this.listeners.each(function(l) { l.onUserLoggedOutChanged(val); });
  });

  // the error state
  this._error = null;
  this.__defineGetter__('error', function () { return this._error; });
  this.__defineSetter__('error', function(val) {
    this._error = val;
    this.listeners.each(function(l) {
      l.onErrorChanged(val); });
  });

  // the loggedIn state
  this._loggedIn = false;
  this.__defineGetter__('loggedIn', function() { return this._loggedIn; });
  this.__defineSetter__('loggedIn', function(aLoggedIn){
    this._loggedIn = aLoggedIn;
    this.listeners.each(function(l) { l.onLoggedInStateChanged(); });
  });

  // get the playback history service
  this._playbackHistory =
      Cc['@songbirdnest.com/Songbird/PlaybackHistoryService;1']
      .getService(Ci.sbIPlaybackHistoryService);
  // add ourselves as a playlist history listener
  this._playbackHistory.addListener(this);

  // a timer to periodically try to post to last.fm
  this._timer = Cc['@mozilla.org/timer;1'].createInstance(Ci.nsITimer);
  this._timer.init(this, TIMER_INTERVAL, Ci.nsITimer.TYPE_REPEATING_SLACK);

  // listen to the playlist playback service
  this._mediacoreManager = Cc['@songbirdnest.com/Songbird/Mediacore/Manager;1']
    .getService(Ci.sbIMediacoreManager);
  this._mediacoreManager.addListener(this);

  // set up the radio library and medialist
  var libraryFactory =
    Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/LibraryFactory;1"]
    .getService(Ci.sbILibraryFactory);
  var file = Cc["@mozilla.org/file/directory_service;1"]
    .getService(Ci.nsIProperties).get("ProfD", Ci.nsIFile);
  file.append("db");
  file.append(RADIO_LIBRARY_FILENAME);
  var bag = Cc["@mozilla.org/hash-property-bag;1"]
    .createInstance(Ci.nsIWritablePropertyBag2);
  bag.setPropertyAsInterface("databaseFile", file);
  var library = libraryFactory.createLibrary(bag);
  library.clear();
  this.radio_mediaList = library.createMediaList('simple');
  this.radio_mediaList.clear();

  // save a pointer to our string bundle
  this._strings = Cc["@mozilla.org/intl/stringbundle;1"]
	.getService(Ci.nsIStringBundleService)
	.createBundle("chrome://sb-lastfm/locale/overlay.properties");

  // create a service pane node for the radio tuner ui
  this._servicePaneService = Cc['@songbirdnest.com/servicepane/service;1']
    .getService(Ci.sbIServicePaneService);
  var BMS = Cc['@songbirdnest.com/servicepane/bookmarks;1']
	.getService(Ci.sbIBookmarks);
 
  // find a radio folder if it already exists
  var radioString = this._strings.GetStringFromName("lastfm.radio.label");
  var radioFolder = findRadioNode(this._servicePaneService.root, radioString);
  if (radioFolder)
	  radioFolder.name = radioString;
  else {
	  radioFolder = BMS.addFolder(radioString);
	  radioFolder.setAttributeNS(SB_NS, "radioFolder", 1);
  }
  radioFolder.editable = false;
  radioFolder.hidden = false;
  // Sort the radio folder node in the service pane
  radioFolder.setAttributeNS(SP_NS, "Weight", 1);
  this._servicePaneService.sortNode(radioFolder);
  
  this._servicePaneNode = BMS.addBookmarkAt(
		  "chrome://sb-lastfm/content/tuner2.xhtml", "Last.fm",
		  "chrome://sb-lastfm/skin/as.png", radioFolder, null);
  if (this._servicePaneNode) {
	  this._servicePaneNode.editable = false;
	  this._servicePaneNode.hidden = false;
  } else {
	  this._servicePaneNode = this._servicePaneService.getNodeForURL(
		  "chrome://sb-lastfm/content/tuner2.xhtml");
  }
  this._servicePaneNode.image = 'chrome://sb-lastfm/skin/as.png';

  //dump("HERE: " + this._servicePaneNode + "\n");
  //this._servicePaneService.removeNode(this._servicePaneNode);

  // track metrics
  this._metrics = Cc['@songbirdnest.com/Songbird/Metrics;1']
    .getService(Ci.sbIMetrics);

  this._jsonSvc = Cc["@mozilla.org/dom/json;1"]
	.createInstance(Ci.nsIJSON);

  // report errors using the console service
  this._console = Cc["@mozilla.org/consoleservice;1"]
    .getService(Ci.nsIConsoleService);

  var observerService = Cc["@mozilla.org/observer-service;1"]
	.getService(Ci.nsIObserverService);
  observerService.addObserver(this, "http-on-modify-request", false);
 
  // reset some data remotes
  SBDataSetStringValue("lastfm.radio.station", "");
  SBDataSetBoolValue("lastfm.radio.requesting", false);

  // Attach our listener to the ShowCurrentTrack event issue by the faceplate
  var faceplateManager =  Cc['@songbirdnest.com/faceplate/manager;1']
		.getService(Ci.sbIFaceplateManager);
  var pane = faceplateManager.getPane("songbird-dashboard");
  var mainWin = Cc["@mozilla.org/appshell/window-mediator;1"]
		.getService(Ci.nsIWindowMediator)
		.getMostRecentWindow("Songbird:Main");
  if (mainWin && mainWin.window) {
	var self = this;
    mainWin.window.addEventListener("ShowCurrentTrack", function(e) {
			self.showStation(e) }, true);
  }
		
  LastFmUninstallObserver.register();
}

// XPCOM Magic
sbLastFm.prototype.classDescription = 'Songbird Last.fm Service'
sbLastFm.prototype.contractID = '@songbirdnest.com/lastfm;1';
sbLastFm.prototype.classID =
    Components.ID('13bc0c9e-5c37-4528-bcf0-5fe37fcdc37a');
sbLastFm.prototype.QueryInterface =
    XPCOMUtils.generateQI([Ci.sbIPlaybackHistoryListener, Ci.nsIObserver,
        Ci.sbILastFmWebServices, Ci.sbIMediacoreEventListener,
		Ci.sbILastFmRadio, Ci.sbILastFm]);

// Error reporting
sbLastFm.prototype.log = function sbLastFm_log(message) {
  this._console.logStringMessage('[last-fm] '+message);
}

// failure handling
sbLastFm.prototype.hardFailure =
function sbLastFm_hardFailure(message) {
  this.error =
      'An error occurred while communicating with the Last.fm servers.';
  this.hardFailures++;

  if (this.hardFailures >= 3) {
    // after three hard failures, try to re-handshake
    this.log('Last.fm hard failure: '+message+'\nFalling back to handshake');
    this.hardFailures = 0;
    this.session = null;
    this.handshake();
  } else {
    // just count and log this
    this.log('Last.fm hard failure: ' + message);
  }
}

sbLastFm.prototype.badSession =
function sbLastFm_badSession() {
  // the server rejected the current session.
  // we need to re-handshake, but we don't care about the result
  this.session = null;
  this.handshake();
}

// should the user be automatically logged in?
sbLastFm.prototype.shouldAutoLogin =
function sbLastFm_shouldAutoLogin() {
  return this.username && this.password && !this.userLoggedOut;
}

// login functionality
sbLastFm.prototype.login =
function sbLastFm_login() {
  this.listeners.each(function(l) { l.onLoginBegins(); });

  // first step - handshake.
  var self = this;
  this.handshake(function success() {
    // increment metrics
    self._metrics.metricsInc('lastfm', 'login', null);

    // save the credentials
    Logins.set(self.username, self.password);

    // authenticate against the new Last.fm "rest" API
    self.apiAuth();

    // authenticate against the Last.fm radio API
    self.radioLogin(function() { }, function() { });

  }, function failure(aAuthFailed) {
    self.loggedIn = false;
    self.error = 'Login failed';
    self.listeners.each(function(l) { l.onLoginFailed(); });
  });
}
sbLastFm.prototype.cancelLogin =
function sbLastFm_cancelLogin() {
  if (this._handshake_xhr) {
    this._handshake_xhr.abort();
  }
  this.listeners.each(function(l) { l.onLoginCancelled(); });
}

// logout is pretty simple
sbLastFm.prototype.logout =
function sbLastFm_logout() {
  this.session = null;
  this.nowplaying_url = null;
  this.submission_url = null;
  this.loggedIn = false;
}


// do the handshake
sbLastFm.prototype.handshake =
function sbLastFm_handshake(success, failure) {
  // have default callbacks
  if (!success) success = function() { }
  if (!failure) failure = function(x) { }

  // make the url
  var timestamp = Math.round(Date.now()/1000).toString();
  var hs_url = 'http://post.audioscrobbler.com/?hs=true&p=1.2.1&c=sbd&v=0.1' +
    '&u=' + this.username + '&t=' + timestamp + '&a=' +
    md5(md5(this.password) + timestamp);

  function failureOccurred(aAuthFailure) {
    failure(aAuthFailure);
  }

  this._handshake_xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
    .createInstance();
  this._handshake_xhr.mozBackgroundRequest = true;
  var self = this;
  this._handshake_xhr.onload = function(event) {
    // loaded - check the HTTP status
    if (self._handshake_xhr.status != 200) {
      self.log('handshake got HTTP status: ' + self._handshake_xhr.status);
      failureOccurred(HANDSHAKE_FAILURE_OTHER);
      return;
    }

    // split the response text into lines and parse it
    var response_lines = self._handshake_xhr.responseText.split('\n');
    if (response_lines.length < 4) {
      self.log('handshake got not enough lines: ' + response_lines.toSource());
      failureOccurred(HANDSHAKE_FAILURE_OTHER);
      return;
    }
    if (response_lines[0] == 'BADAUTH') {
      self.log('handshake got authentication failure');
      failureOccurred(HANDSHAKE_FAILURE_AUTH);
      return;
    }
    if (response_lines[0] != 'OK') {
      self.log('handshake got unexpected status: '+response_lines[0]);
      failureOccurred(HANDSHAKE_FAILURE_OTHER);
      return;
    }
    // save the results of parsing out
    self.session = response_lines[1];
    self.nowplaying_url = response_lines[2];
    self.submission_url = response_lines[3];

    // download profile info
    self.updateProfile(function success() {
      self.loggedIn = true;
      self.error = null;
      self.listeners.each(function(l) { l.onLoginSucceeded(); });

      // we should try to scrobble
      self.scrobble();
    }, function failure() {
      self.loggedIn = false;
      self.error = null;
      self.listeners.each(function(l) { l.onLoginFailed(); });
    });

    // we've sucessfully handshook
    success();
  };
  this._handshake_xhr.onerror = function(event) {
    // faileded
    self.log('handshake got XMLHttpRequest error');
    failureOccurred(HANDSHAKE_FAILURE_OTHER);
  };
  this._handshake_xhr.open('GET', hs_url, true);
  this._handshake_xhr.send(null);
}

// update profile data
sbLastFm.prototype.updateProfile =
function sbLastFm_updateProfile(succeeded, failed) {
  var url = 'http://ws.audioscrobbler.com/1.0/user/' +
    encodeURIComponent(this.username) + '/profile.xml?' + Date.now();
  var self = this;
  this.getXML(url, function success(xml) {
    function text(tag) {
      var tags = xml.getElementsByTagName(tag);
      if (tags.length) {
        return tags[0].textContent;
      } else {
        return '';
      }
    }
    self.realname = text('realname');
    self.playcount = parseInt(text('playcount'));
    self.avatar = text('avatar');
    self.profileurl = text('url');
    self.listeners.each(function(l) { l.onProfileUpdated(); });
    succeeded();
  }, function failure(xhr) {
    failed();
  });
}


// set now playing for the current user
// the first argument is an array of object, each with one-letter keys
// corresponding to the audioscrobbler submission protocol keys - ie:
// a=artist, t=track, l=track-length, b=album, n=track-number
// the PlayedTrack object implements this
sbLastFm.prototype.nowPlaying =
function sbLastFm_nowPlaying(submission) {
  var url = this.nowplaying_url;
  var params = {s:this.session};
  var props = 'atblnm'; // the keys we send
  for (var j=0; j<props.length; j++) {
    params[props[j]] = submission[props[j]];
  }
  // post, but don't worry about failure or success, this is best-effort
  this.asPost(url, params, function() { }, function() { });
}

// the first argument is an array of object, each with one-letter keys
// corresponding to the audioscrobbler submission protocol keys - ie:
// a=artist, t=track, i=start-time, l=track-length, b=album, n=track-number
// the PlayedTrack object implements this
sbLastFm.prototype.submit =
function sbLastFm_submit(submissions, success) {
  // if we don't have a session, we need to handshake first
  if (!this.session) {
    this.handshake();
    return;
  }

  // build the submission
  var url = this.submission_url;
  var params = {s:this.session};
  var props = 'brainmolt'; // the keys we send
  for (var i=0; i<submissions.length; i++) {
    for (var j=0; j<props.length; j++) {
      params[props[j] + '[' + i + ']'] = submissions[i][props[j]];
    }
  }

  //post to AudioScrobbler
  var self = this;
  dump("*** submitted: " + url + " // params: " + params + "\n");
  this.asPost(url, params, success,
              function fail(msg) { self.hardFailure(msg); },
              function badsession() { self.badSession(); });
}

// get XML from an URL
sbLastFm.prototype.getXML =
function sbLastFm_getXML(url, success, failure) {
  var xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
    .createInstance();
  // run in the background, since we're in xpcomland
  xhr.mozBackgroundRequest = true;
  // force an xml response
  xhr.overrideMimeType('text/xml');
  xhr.onload = function(event) {
    if (xhr.responseXML) {
	  if (xhr.responseText == "No recs :(") {
		  dump("Failed to get XSPF");
		  failure(xhr);
	  } else {
		  success(xhr.responseXML);
	  }
    } else {
      failure(xhr);
    }
  };
  xhr.onerror = function(event) {
    failure(xhr);
  };
  xhr.open('GET', url, true);
  xhr.send(null);
  return xhr;
}

// get Last.fm radio name/value pairs from an URL
sbLastFm.prototype.getPairs =
function sbLastFm_getPairs(url, success, failure) {
  var xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
    .createInstance();
  // run in the background, since we're in xpcomland
  xhr.mozBackgroundRequest = true;
  xhr.onload = function(event) {
    try {
      var pairs = new Object();
      var lines = xhr.responseText.split('\n');
      for (var i=0; i<lines.length; i++) {
        var m = lines[i].match(/([^=]+)=(.*)/);
        if (m && m.length == 3) {
          pairs[m[1]] = m[2];
        }
      }
    } catch(e) {
      failure(xhr);
      return;
    }
    success(pairs);
  };
  xhr.onerror = function(event) {
    failure(xhr);
  };
  xhr.open('GET', url, true);
  xhr.send(null);
  return xhr;
}


// log in to the last.fm radio service based on the credentials stored in 
// the sbLastFm service.
sbLastFm.prototype.radioLogin =
function sbLastFm_radioLogin(success, failure) {
  var self = this; 
  this.getPairs('http://ws.audioscrobbler.com/radio/handshake.php?' + 
      urlencode({ username: this.username, passwordmd5: md5(this.password) }),
      function _success(pairs) {
        // have off useful handshake data in service instance variables
        // FIXME: make sure session != FAILED
        self.radio_session = pairs.session;
        self.radio_base_url = 'http://'+pairs.base_url+pairs.base_path;
        // FIXME: notify listeners that radio login completed?
        success();
      },
      function _failure(xhr) {
        // FIXME: error handling / reporting / retrying
		dump("Failed to authenticate: Please try logging in again\n");
        failure();
      });
}


// tune the current radio session into a particular lastfm:// uri
sbLastFm.prototype.radioStation =
function sbLastFm_radioStation(station, success, failure) {
  var self = this;
  this.getPairs(this.radio_base_url + '/adjust.php?' +
      urlencode({session: this.radio_session, url: station}),
      function _success(pairs) {
        // save off current station info
		if (pairs.url)
			self.radio_station_url = pairs.url;
		if (pairs.stationname)
			self.radio_station_name = pairs.stationname;
		else {
			self.radio_station_name = unescape(self.radio_station_url.replace(
				/^lastfm:\/\//, "").replace(/\//, " "));
		}
        success();
      }, function _failure(xhr) {
        failure();
      });
}

// get more tracks from Last.fm's servers into the radio playlist
sbLastFm.prototype.requestMoreRadio =
function sbLastFm_requestMoreRadio(success, failure) {
  var self = this; 
  this.getXML(this.radio_base_url + '/xspf.php?' + 
    urlencode({sk: this.radio_session, discovery: 0, desktop: '1.3.1.1'}),
    function _success(xspf) {
	  dump("**** SUCCESSFULLY loaded new XSPF data\n");
      var tracks = xspf.getElementsByTagName('track');
      function tagValue(trackNumber, tagName) {
        try { return tracks[i].getElementsByTagName(tagName)[0].textContent; } 
        catch(e) { return ''; }
      }
      function linkValue(trackNumber, linkRel) {
        try {
          var links = tracks[i].getElementsByTagName('link');
          for (var i=0; i<links.length; i++) {
            if (links[i].getAttribute('rel') == linkRel) {
              return links[i].textContent;
            }
          }
          return '';
        } catch(e) { return ''; }
      }
      for (var i=0; i<tracks.length; i++) {
        // create a media item from the XSPF
		dump("URL: " + tagValue(i, 'title') + " -- " + tagValue(i, 'location') + "\n");
        var mediaItem = self.radio_mediaList.library.createMediaItem(
          newURI(tagValue(i, 'location')));
        // set up standard track metadata
        mediaItem.setProperty(SBProperties.trackName, tagValue(i, 'title'));
        mediaItem.setProperty(SBProperties.artistName, tagValue(i, 'creator'));
        mediaItem.setProperty(SBProperties.albumName, tagValue(i, 'album'));
        mediaItem.setProperty(SBProperties.primaryImageURL, 
            tagValue(i, 'image'));
        mediaItem.setProperty(SBProperties.duration, 
            parseInt(tagValue(i, 'duration'))*1000);
        // set up some lastfm-specific metadata
        mediaItem.setProperty(PROPERTY_TRACKID, tagValue(i, 'id'));
        // get their properties out of the xspf
        // FIXME: register these properties first?
        var props = ['trackauth', 'albumid', 'artistid'];
        for (var j=0; j<props.length; j++) {
          var tags = tracks[i].getElementsByTagNameNS(LASTFM_NS, props[i]);
          if (tags && tags.length) {
            mediaItem.setProperty(LASTFM_NS + props[i], tags[0].textContent);
          }
        }

        self.radio_mediaList.add(mediaItem);
      }

      success();
    }, function _failure(xhr) {
      failure();
    });
}

sbLastFm.prototype.saveRecentStation =
function sbLastFm_saveRecentStation(name, stationUrl, url, sImageUrl,
		mImageUrl, lImageUrl)
{
	var thisStation = { name: name, stationUrl: stationUrl, url: url,
		sImageUrl: sImageUrl, mImageUrl: mImageUrl, lImageUrl: lImageUrl };
	var prevIdx = -1;

	var recentStations = this._jsonSvc.decode(Application.prefs.getValue(
				"extensions.lastfm.recent.stations", "[]"));
	for (var i=0; i<recentStations.length; i++) {
		if (recentStations[i].stationUrl == stationUrl) {
			prevIdx = i;
			break;
		}
	}
	if (prevIdx > -1) {
		recentStations.splice(prevIdx, 1);
	}
	LastfmTuner.recentStations.push(thisStation);
	Application.prefs.setValue("extensions.lastfm.recent.artists",
		this._jsonSvc.encode(recentStations));
}

// begin last.fm radio playback of a particular station
sbLastFm.prototype.radioPlay =
function sbLastFm_radioPlay(station) {
  // FIXME: make sure we've got a radio session active...
 
  // clear the playlist
  this.radio_mediaList.clear();

  // set the station name to the URI
  this.radio_station_url = station;
  this.radio_station_name = station;

  dump("******* radioplay: " + station + "\n");
  // manually set the buffering state so the user has some feedback that
  // Songbird is trying to do something
  // this is kinda hacky. sorry :(
  SBDataSetStringValue("lastfm.radio.station", "");
  SBDataSetStringValue("metadata.artist", "");
  SBDataSetStringValue("metadata.album", "");
  SBDataSetStringValue("metadata.title", "Loading Last.fm station...");
  SBDataSetBoolValue("faceplate.seenplaying", true);
  SBDataSetBoolValue("faceplate.buffering", true);
  SBDataSetBoolValue("lastfm.radio.requesting", false);
  /* what i *really* want is createPane. :-(
  var faceplateMgr =  Cc["@songbirdnest.com/faceplate/manager;1"]
    .getService(Ci.sbIFaceplateManager);
  var pane = faceplateMgr.createPane("songbird-lastfm", "Last.fm Radio",
    "chrome://songbird/content/bindings/facePlate.xml#progress-pane");
  pane.setData("label1", "Loading Last.fm Radio Playlist");
  pane.setData("progress-mode", "undetermined");
  pane.setData("label1-hidden", false);
  pane.setData("label2-hidden", true);
  pane.setData("progress-hidden", false);
  */

  // reset failure count
  this.radio_failure_count = 0;
  var self = this;
  this.radioStation(station,
      function radioStation_success() {
        // get the first bunch of tracks
        self.requestMoreRadio(
          function requestMoreRadio_success() {
		    // we have to make sure we're going sequentially. the radio
			// protocol says we can't play out of order from the XSPF
			self._mediacoreManager.sequencer.mode =
			  Ci.sbIMediacoreSequencer.MODE_FORWARD;
			self._mediacoreManager.sequencer.repeatMode =
			  Ci.sbIMediacoreSequencer.MODE_REPEAT_NONE;

            // time to start playing the radio medialist
            self._mediacoreManager.sequencer.playView(
				self.radio_mediaList.createView(), 0);
          },
          function requestMoreRadio_failure() {
		    dump("*** in requestMoreRadio_failure()\n");
			// increment the failure count
			self.radio_failure_count++;
			// if we haven't hit the failure retry limit, then keep retrying
			if (self.radio_failure_count < 5)
				radioStation_success();
			else {
				dump("Repeated failure trying to tune to Last.fm station: " +
					self.radio_station_url);
			}
          });
        
      },
      function radioStation_failure() {
		  dump("FAIL FAIL FAIL to authenticate.\n");
      });
}


// post to audioscrobbler (the old api)
sbLastFm.prototype.asPost =
function sbLastFm_asPost(url, params, success, hardfailure, badsession) {
  var self = this;
  POST(url, params, function(xhr) {
    /* loaded */
    if (xhr.status != 200) {
      hardfailure('HTTP status ' + xhr.status + ' posting to ' + url);
    } else if (xhr.responseText.match(/^OK\n/)) {
      success();
    } else if (xhr.responseText.match(/^BADSESSION\n/)) {
      self.log('Bad Session when posting to last.fm');
      badsession();
    } else {
      hardfailure(xhr.responseText);
    }
  }, function(xhr) {
    /* errored */
    hardfailure('XMLHttpRequest called onerror');
  });
}

// authenticate against the new Last.fm "rest" web service APIs
sbLastFm.prototype.apiAuth = function sbLastFm_apiAuth() {
  // clear our old session
  this.sk = null;

  // get a lastfm mobile session
  var self = this;
  this.apiCall('auth.getMobileSession', {
    username: this.username,
    authToken: md5(this.username + md5(this.password))
  }, function response(success, xml) {
    if (!success) return;
    var keys = xml.getElementsByTagName('key');
    if (keys.length == 1) {
      self.sk = keys[0].textContent;
      self.listeners.each(function(l) { l.onAuthorisationSuccess(); });
    }
  });
}


sbLastFm.prototype.apiCall =
function sbLastFm_apiCall(method, params, responseCallback) {
  // make a new Last.fm Web Service API call
  // see: http://www.last.fm/api/rest
  // note: this isn't really REST.

  function callback(success, xml) {
    if (typeof(responseCallback) == 'function') {
      responseCallback(success, xml);
    } else {
      responseCallback.responseReceived(success, xml);
    }
  }

  // create an object to hold the HTTP params
  var post_params = new Object();

  // load the params from the nsIPropertyBag
  if (params instanceof Ci.nsIPropertyBag) {
    enumerate(params.enumerator, function(item) {
      item.QueryInterface(Ci.nsIProperty);
      post_params[item.name] = item.value;
    })
  } else {
    // or from the object
    for (var k in params) { post_params[k] = params[k]; }
  }

  // set the method and API key
  post_params.method = method;
  post_params.api_key = API_KEY;
  // if we have an sk, use it
  if (this.sk) post_params.sk = this.sk;

  // calculate the signature...
  // put the key/value pairs in an array
  var sorted_params = new Array();
  for (var k in post_params) {
    sorted_params.push(k+post_params[k])
  }
  // sort them
  sorted_params.sort();
  // join them into a string
  sorted_params = sorted_params.join('');
  // hash them with the "secret" to get the API signature
  post_params.api_sig = md5(sorted_params+API_SECRET);

  // post the request
  var self = this;
  POST(API_URL, post_params, function (xhr) {
    if (!xhr.responseXML) {
      // we expect all API responses to have XML
      callback(false, null);
      return;
    }
    if (xhr.responseXML.documentElement.tagName != 'lfm' ||
        !xhr.responseXML.documentElement.hasAttribute('status')) {
      // we expect the root (document) element to be <lfm status="...">
      callback(false, xhr.responseXML);
      return;
    }
    if (xhr.responseXML.documentElement.getAttribute('status' == 'failed')) {
      // the server reported an error
      self.log('Last.fm Web Services Error: '+xhr.responseText);
      callback(false, xhr.responseXML);
      return;
    }
    // all should be good!
    callback(true, xhr.responseXML);
  }, function (xhr) {
    callback(false, null);
  });
}


// try to scrobble pending tracks from playback history
sbLastFm.prototype.scrobble =
function sbLastFm_scrobble(aEntries) {
  var entry_list = [];
  var entries;
  if (!aEntries) {
    entries = this._playbackHistory.entries;
  } else {
    entries = aEntries.enumerate();
  }
  enumerate(entries,
            function(e) {
              e.QueryInterface(Ci.sbIPlaybackHistoryEntry);
              if (!e.hasAnnotation(ANNOTATION_SCROBBLED) &&
                  !e.hasAnnotation(ANNOTATION_HIDDEN)) {
                entry_list.push(e);
              }
            });

  // create the scrobble list in the reverse order
  var scrobble_list = [];
  if (entry_list.length > 0) {
    // collect the playlist history entries into objects that can be sent
    // to last.fm's audioscrobbler api
    for (var i=entry_list.length-1; i>=0; i--) {
      var rating = '';
      if (entry_list[i].hasAnnotation(ANNOTATION_LOVE)) {
        rating = 'L';
      } else {
        // this isn't allowed except for radio
        // rating = 'B';
      }
      var source;
      var track_auth = entry_list[i].item.getProperty(PROPERTY_TRACKAUTH);
      if (track_auth) {
        source = 'L'+track_auth;
      } else {
        source = 'P';
      }
      scrobble_list.push(
          new PlayedTrack(entry_list[i].item,
					  Math.round(entry_list[i].timestamp/1000000), rating, source));
    }
    // submit to the last.fm audioscrobbler api
    var self = this;
    this.submit(scrobble_list,
      function success() {
        // on success mark all these as scrobbled, love & ban as appropriate
        for (i=0; i<entry_list.length; i++) {
          let entry = entry_list[i];
          entry.setAnnotation(ANNOTATION_SCROBBLED, 'true');
          if (entry.hasAnnotation(ANNOTATION_LOVE)) {
            self.apiCall('track.love', {
                track: entry.item.getProperty(SBProperties.trackName),
                artist: entry.item.getProperty(SBProperties.artistName)
              }, function response(success, xml) { });
          } else if (entry.hasAnnotation(ANNOTATION_BAN)) {
            self.apiCall('track.ban', {
                track: entry.item.getProperty(SBProperties.trackName),
                artist: entry.item.getProperty(SBProperties.artistName)
              }, function response(success, xml) { });
          }
        }
        self.playcount += entry_list.length;
        self.listeners.each(function(l) { l.onProfileUpdated(); });
        self.error = null;
        // increment metrics
        self._metrics.metricsAdd('lastfm', 'scrobble', null, entry_list.length);
      });
  }
}


// love and ban
sbLastFm.prototype.loveBan =
function sbLastFm_loveBan(aMediaItem, aLove) {
  this.loveTrack = aMediaItem;
  this.love = aLove;
  this.listeners.each(function(l) { l.onLoveBan(); });
}

// sbIPlaybackHistoryListener
sbLastFm.prototype.onEntriesAdded =
function sbLastFm_onEntriesAdded(aEntries) {
  if (this.shouldScrobble) {
    if (this.loveTrack) {
      // if the track was loved or banned, mark that in the playback history
      var self = this;
      enumerate(aEntries.enumerate(),
                function(item) {
                  item.QueryInterface(Ci.sbIPlaybackHistoryEntry);
                  if (item.item.guid == self.loveTrack) {
                    // love or ban the track
                    item.setAnnotation(self.love?ANNOTATION_LOVE:ANNOTATION_BAN,
                      'true');
                  }
                });
    }
    // let's try to scrobble all the unscrobbled playback history entries
    if (!this.userLoggedOut) {
      this.scrobble(aEntries);
    }
  } else {
    // scrobbling is disabled, let's mark the added entries as not
    // scrobbleable
    enumerate(aEntries.enumerate(),
              function(item) {
                item.QueryInterface(Ci.sbIPlaybackHistoryEntry);
                item.setAnnotation(ANNOTATION_HIDDEN, 'true');
              });
  }
}
sbLastFm.prototype.onEntriesUpdated =
function sbLastFm_onEntriesUpdated(aEntries) { }
sbLastFm.prototype.onEntriesRemoved =
function sbLastFm_onEntriesRemoved(aEntries) { }
sbLastFm.prototype.onEntriesCleared =
function sbLastFm_onEntriesCleared() { }


// nsIObserver - just used for a timer callback
sbLastFm.prototype.observe =
function sbLastFm_observe(subject, topic, data) {
  if (topic == 'timer-callback' && subject == this._timer &&
      this.shouldScrobble && !this.userLoggedOut) {
    // try to scrobble when the timer ticks
    this.scrobble();
  }
  else if (topic == "http-on-modify-request") {
	var channel = subject.QueryInterface(Ci.nsIHttpChannel);
	var uri = channel.URI;
	if (uri.host == "www.last.fm" || uri.host == "last.fm") {
		var r = uri.path.match(/^\/?listen\/(.*)/);
		if (r) {
			var radio = "lastfm://" + r[1];
			dump("TUNING INTO: " + radio + "\n");
			channel.cancel(Components.results.NS_BINDING_ABORTED);
			this.radioPlay(radio);
		}
	}
  }
}

// sbIMediacoreEventListener
sbLastFm.prototype.onMediacoreEvent = 
function sbLastFm_onMediacoreEvent(aEvent) {
  switch(aEvent.type) {
    case Ci.sbIMediacoreEvent.STREAM_STOP:
      this.onStop();
      break;
    case Ci.sbIMediacoreEvent.VIEW_CHANGE:
      this.radio_playing = (aEvent.data.mediaList == this.radio_mediaList);
	  dump("setting radio_playing: " + this.radio_playing + "\n");
      break;
    case Ci.sbIMediacoreEvent.BEFORE_TRACK_CHANGE:
      if (this.radio_playing) {
		  dump("setting station: " + this.radio_station_name + "\n");
        SBDataSetStringValue('lastfm.radio.station', this.radio_station_name);
      } else {
        SBDataSetStringValue('lastfm.radio.station', '');
      }
      break;
    case Ci.sbIMediacoreEvent.TRACK_CHANGE:
      // if we're playing the last track in a radio view time to get more!
      if (this.radio_playing && !this._mediacoreManager.sequencer.nextItem) {
        dump("OUT OF RADIO TRACKS... requesting more\n");
        SBDataSetBoolValue('lastfm.radio.requesting', true);
		SBDataSetStringValue("faceplate.status.text", "Requesting next set of tracks from Last.fm...");
        this.requestMoreRadio(function(){
			SBDataSetBoolValue('lastfm.radio.requesting', false);
			SBDataSetStringValue("faceplate.status.text", "Next tracks loaded successfully!");
			}, function(){});
      } else 
        SBDataSetBoolValue('lastfm.radio.requesting', false);
      if (this.radio_playing) {
        // let's remove all the tracks *before* this one from the medialist
        var view = this._mediacoreManager.sequencer.view
        var pos = this._mediacoreManager.sequencer.viewPosition;
        var removals = new Array();
        for (var i=0; i<pos && i<view.length; i++) {
          removals.push(view.getItemByIndex(i));
        }
        if (removals.length) {
          view.mediaList.removeSome(ArrayConverter.enumerator(removals));
        }
      }
      // we want to do some more stuff too...
      this.onTrackChange(aEvent.data);
      break;
    case Ci.sbIMediacoreEvent.STREAM_START:
      break;
    case Ci.sbIMediacoreEvent.STREAM_STOP:
      break;
    case Ci.sbIMediacoreEvent.STREAM_END:
      break;
    default:
      break;
  }
}

// invoked from onMediacoreEvent above
sbLastFm.prototype.onTrackChange =
function sbLastFm_onTrackChange(aItem) {
  // NOTE: This depends on the current assumption that onTrackChange will
  // be run before onEntriesAdded. Otherwise love/ban won't work.

  // reset the love/ban state
  this.loveBan(null, false);

  // send now playing info if appropriate
  if (this.shouldScrobble && this.loggedIn &&
      aItem.getProperty(SBProperties.excludeFromHistory) != '1') {
    this.nowPlaying(new PlayedTrack(aItem));
  }
}

sbLastFm.prototype.onStop = function sbLastFm_onStop() {
  // reset the love/ban state
  this.loveBan(null, false);
}

sbLastFm.prototype.showStation = function sbLastFm_showStation(e) {
	if (this.radio_playing) {
		var stationPage = this.radio_station_url.replace(/^lastfm:\/\//,
				"http://last.fm/");
		stationPage = stationPage.replace(/\/personal$/, "");
		var mainWin =
			Components.classes['@mozilla.org/appshell/window-mediator;1']
			.getService(Components.interfaces.nsIWindowMediator)
			.getMostRecentWindow('Songbird:Main');
		if (mainWin && mainWin.gBrowser)
			mainWin.gBrowser.loadOneTab(stationPage);
		e.preventDefault();
	}
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbLastFm]);
}

/* UNINSTALL OBSERVER */
var LastFmUninstallObserver = {
	_uninstall : false,
	_disable : false,
	_tabs : null,

	observe : function(subject, topic, data) {
		if (topic == "em-action-requested") {
			// Extension has been flagged to be uninstalled
			subject.QueryInterface(Ci.nsIUpdateItem);
			
			var SPS = Cc['@songbirdnest.com/servicepane/service;1'].
					getService(Ci.sbIServicePaneService);
			var radioNode = findRadioNode(SPS.root);
			var lastFmNode = SPS.getNodeForURL(
					"chrome://sb-lastfm/content/tuner2.xhtml");

			if (subject.id == "audioscrobbler@songbirdnest.com") {
				if (data == "item-uninstalled") {
					this._uninstall = true;
				} else if (data == "item-disabled") {
					this._disable = true;
					lastFmNode.hidden = true;
				} else if (data == "item-cancel-action") {
					if (this._uninstall)
						this._uninstall = false;
					if (this._disable)
						lastFmNode.hidden = false;
				}
			}
		} else if (topic == "quit-application-granted") {
			// We're shutting down, so check to see if we were flagged
			// for uninstall - if we were, then cleanup here
			if (this._uninstall) {
				// Things to cleanup:
				// Service pane node
				var SPS = Cc['@songbirdnest.com/servicepane/service;1'].
						getService(Ci.sbIServicePaneService);

				// Remove only the Last.fm node 
				var lastFmNode = SPS.getNodeForURL(
						"chrome://sb-lastfm/content/tuner2.xhtml");
				SPS.removeNode(lastFmNode);

				// Remove the "Radio" node if it's empty
				// XXX stevel - in a loop due to bugs with multiple Radio
				// nodes having been created
				var radioFolder = findRadioNode(SPS.root);
				var enum = radioFolder.childNodes;
				while (enum.hasMoreElements()) {
					var node = enum.getNext();
					dump(node.name + "--" + node.url + "--" + node.id + "\n");
				}

				while (radioFolder != null && !radioFolder.firstChild) {
					SPS.removeNode(radioFolder);
					radioFolder = findRadioNode(SPS.root);
				}
			}
			this.unregister();
		}
	},

	register : function() {
		var observerService = Cc["@mozilla.org/observer-service;1"]
			.getService(Ci.nsIObserverService);
		observerService.addObserver(this, "em-action-requested", false);
		observerService.addObserver(this, "quit-application-granted", false);
	},

	unregister : function() {
		var observerService = Cc["@mozilla.org/observer-service;1"]
			.getService(Ci.nsIObserverService);
		observerService.removeObserver(this, "em-action-requested");
		observerService.removeObserver(this, "quit-application-granted");
	}
}
