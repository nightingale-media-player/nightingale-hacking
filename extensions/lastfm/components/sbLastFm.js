/*
Copyright (c) 2008-2010, Pioneers of the Inevitable, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of Pioneers of the Inevitable, Nightingale, nor the names
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

var Application = Cc["@mozilla.org/fuel/application;1"]
                    .getService(Ci.fuelIApplication);

// our namespace
const NS = 'http://www.getnightingale.com/lastfm#'
const SB_NS = 'http://getnightingale.com/data/1.0#'
const SP_NS = 'http://getnightingale.com/rdf/servicepane#'

// our annotations
const ANNOTATION_SCROBBLED = NS+'scrobbled';
const ANNOTATION_HIDDEN = NS+'hidden';
const ANNOTATION_LOVE = NS+'love';
const ANNOTATION_BAN = NS+'ban';

// our properties
const PROPERTY_TRACKID = NS+'trackid';
const PROPERTY_STATION = NS+'station';
const PROPERTY_ARTISTPAGE = NS+'artistPage';

// their namespace
const LASTFM_NS = 'http://www.audioscrobbler.net/dtd/xspf-lastfm'
// their properties
const PROPERTY_TRACKAUTH = LASTFM_NS+'trackauth';
const PROPERTY_ALBUMID = LASTFM_NS+'albumid';
const PROPERTY_ARTISTID = LASTFM_NS+'artistid';

// Last.fm API key, secret and URL
const API_KEY = 'ad68d3b69dee88a912b193a35d235a5b';
const API_SECRET = '5cb0c1f1cceb3bff561a62b718702175'; // obviously not secret
var API_URL = 'http://ws.audioscrobbler.com/2.0/';
var AUTH_URL = 'http://post.audioscrobbler.com/';

// handshake failure types
const HANDSHAKE_FAILURE_AUTH = true;
const HANDSHAKE_FAILURE_OTHER = false;

// different phases of Last.fm login
const AUTH_PHASE_HANDSHAKE = 1;
const AUTH_PHASE_TOKEN_REQUEST = 2;
const AUTH_PHASE_WEB_LOGIN_GEO = 3;
const AUTH_PHASE_WEB_LOGIN = 4;
const AUTH_PHASE_APP_APPROVE = 5;
const AUTH_PHASE_SESSION_REQUEST = 6;
const AUTH_PHASE_USER_INFO = 7;

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
// import observer utils
Cu.import("resource://app/jsmodules/ObserverUtils.jsm");

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
    /*
    dump("POST:\n");
    dump("\tdata: " + urlencode(params) + "\n");
    dump("\turl: " + url + "\n");
    */
    xhr.send(urlencode(params));
    // pass the XHR back to the caller
  } catch(e) {
    Cu.reportError(e);
    onerror(xhr);
  }
  return xhr;
}

function logStringToFile(path, text, comment) {
  dump("Logging " + comment + " to " + path + "\n");
  var file = Cc["@mozilla.org/file/local;1"]
                  .createInstance(Ci.nsILocalFile);
  file.initWithPath(path);
  var foStream = Cc["@mozilla.org/network/file-output-stream;1"]
                  .createInstance(Ci.nsIFileOutputStream);

  // use 0x02 | 0x10 to open file for appending.
  foStream.init(file, 0x02 | 0x08 | 0x20, 0666, 0);
  // write, create, truncate
  // In a c file operation, we have no need to set file mode with or operation,
  // directly using "r" or "w" usually.

  // if you are sure there will never ever be any non-ascii text in data you
  // can also call foStream.writeData directly
  var converter = Cc["@mozilla.org/intl/converter-output-stream;1"]
                    .createInstance(Ci.nsIConverterOutputStream);
  converter.init(foStream, "UTF-8", 0, 0);
  converter.writeString(text);
  converter.close(); // this closes foStream
  dump("Done logging\n");
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

function sbLastFm() {
  // our interface is really lightweight - make the service available as a JS
  // object so we can avoid the IDL / XPConnect complexity.
  this.wrappedJSObject = this;

  // observe before library manager shutdown events to trigger shutdown
  this._observerSet = new ObserverSet();
  this._observerSet.add(this,
                        "nightingale-library-manager-before-shutdown",
                        false,
                        true);

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
  this._login_phase = null;

  this.__defineGetter__('login_phase', function() {
    return this._login_phase;
  });
  this.__defineSetter__('login_phase', function(val) {
    this._login_phase = val;
    var phase = "not actively logging in";
    switch (val) {
      case AUTH_PHASE_HANDSHAKE:
        phase = "handshake";
        break;
      case AUTH_PHASE_TOKEN_REQUEST:
        phase = "authorisation token request";
        break;
      case AUTH_PHASE_WEB_LOGIN_GEO:
        phase = "last.fm geo-detection for weblogin";
        break;
      case AUTH_PHASE_WEB_LOGIN:
        phase = "last.fm website login";
        break;
      case AUTH_PHASE_APP_APPROVE:
        phase = "application authorisation";
        break;
      case AUTH_PHASE_SESSION_REQUEST:
        phase = "session request";
        break;
      case AUTH_PHASE_USER_INFO:
        phase = "user info/profile download";
        break;
    }
    dump("LOGIN PHASE: " + phase + "\n");
  });

  // the should-we-scrobble pref
  var prefsService = Cc['@mozilla.org/preferences-service;1']
      .getService(Ci.nsIPrefBranch);
  AUTH_URL = prefsService.getCharPref('extensions.lastfm.auth_url');
  API_URL = prefsService.getCharPref('extensions.lastfm.api_url');

  this.__defineGetter__('shouldScrobble', function() {
    return prefsService.getBoolPref('extensions.lastfm.scrobble');
  });
  this.__defineSetter__('shouldScrobble', function(val) {
    prefsService.setBoolPref('extensions.lastfm.scrobble', val);
    this.listeners.each(function(l) { l.onShouldScrobbleChanged(val); });
  });

  // the enable-radio-node pref
  this.__defineGetter__('displayRadioNode', function() {
    return prefsService.getBoolPref('extensions.lastfm.show_radio_node');
  });
  this.__defineSetter__('displayRadioNode', function(val) {
    prefsService.setBoolPref('extensions.lastfm.show_radio_node', val);
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

  // whether the user is a subscriber or not
  this._subscriber = false;
  if (Application.prefs.getValue("extensions.lastfm.subscriber_override",false))
    this._subscriber = true;
  this.__defineGetter__('subscriber', function() { return this._subscriber; });

  // the loggedIn state
  this._loggedIn = false;
  this.__defineGetter__('loggedIn', function() { return this._loggedIn; });
  this.__defineSetter__('loggedIn', function(aLoggedIn){
    this._loggedIn = aLoggedIn;
    this.listeners.each(function(l) { l.onLoggedInStateChanged(); });
  });

  // get the playback history service
  this._playbackHistory =
      Cc['@getnightingale.com/Nightingale/PlaybackHistoryService;1']
      .getService(Ci.sbIPlaybackHistoryService);
  // add ourselves as a playlist history listener
  this._playbackHistory.addListener(this);

  // a timer to periodically try to post to last.fm
  this._timer = Cc['@mozilla.org/timer;1'].createInstance(Ci.nsITimer);
  this._timer.init(this, TIMER_INTERVAL, Ci.nsITimer.TYPE_REPEATING_SLACK);

  // listen to the playlist playback service
  this._mediacoreManager = Cc['@getnightingale.com/Nightingale/Mediacore/Manager;1']
    .getService(Ci.sbIMediacoreManager);
  this._mediacoreManager.addListener(this);

  // save the previous shuffle and repeat modes
  this.prevShuffleMode = this._mediacoreManager.sequencer.mode;
  this.prevRepeatMode = this._mediacoreManager.sequencer.repeatMode;

  // set up the radio library and medialist
  var libraryFactory =
    Cc["@getnightingale.com/Nightingale/Library/LocalDatabase/LibraryFactory;1"]
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
  this._strings =
    Cc["@mozilla.org/intl/stringbundle;1"]
      .getService(Ci.nsIStringBundleService)
      .createBundle("chrome://sb-lastfm/locale/overlay.properties");

  // create a service pane node for the radio tuner ui
  this._servicePaneService = Cc['@getnightingale.com/servicepane/service;1']
    .getService(Ci.sbIServicePaneService);

  // find a radio folder if it already exists
  var radioFolder = this._servicePaneService.getNode("SB:RadioStations");
  if (!radioFolder) {
    radioFolder = this._servicePaneService.createNode();
    radioFolder.id = "SB:RadioStations";
    radioFolder.className = "folder radio";
    radioFolder.name = this._strings.GetStringFromName("lastfm.radio.label");
    radioFolder.setAttributeNS(SB_NS, "radioFolder", 1); // for backward-compat
    radioFolder.setAttributeNS(SP_NS, "Weight", 2);
    this._servicePaneService.root.appendChild(radioFolder);
  }
  radioFolder.editable = false;

  this._servicePaneNode = this._servicePaneService.createNode();
  this._servicePaneNode.url = "chrome://sb-lastfm/content/tuner2.xhtml";
  this._servicePaneNode.id = "SB:RadioStations:LastFM";
  this._servicePaneNode.name = "Last.fm";
  this._servicePaneNode.image = 'chrome://sb-lastfm/skin/as.png';
  this._servicePaneNode.editable = false;
  this._servicePaneNode.hidden = true;
  radioFolder.appendChild(this._servicePaneNode);

  this.updateServicePaneNodes();

  // track metrics
  this._metrics = Cc['@getnightingale.com/Nightingale/Metrics;1']
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
}

// XPCOM Magic
sbLastFm.prototype.classDescription = 'Nightingale Last.fm Service'
sbLastFm.prototype.contractID = '@getnightingale.com/lastfm;1';
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

sbLastFm.prototype.updateServicePaneNodes = function updateSPNodes() {
  // only show the Last.fm radio node if the user is a subscriber or if they've
  // specifically requested to have it be shown
  var radioFolder = this._servicePaneService.getNode("SB:RadioStations");
  if (this.loggedIn && (this._subscriber || this.displayRadioNode))
    this._servicePaneNode.hidden = false;
  else
    this._servicePaneNode.hidden = true;

  // Hide the "Radio" node if it's empty
  var enum = radioFolder.childNodes;
  var visibleFlag = false;

  // Iterate through elements and check if one is visible
  while (enum.hasMoreElements()) {
    var node = enum.getNext();
    if (!node.hidden) {
      visibleFlag = true;
      break;
    }
  }
  radioFolder.hidden = !visibleFlag;
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

sbLastFm.prototype.postHandshake =
function sbLastFm_postHandshake() {
  var self = this;

  // download profile info
  this.updateProfile(function success() {
    try {
      self.loggedIn = true;
      self.error = null;
      self.updateServicePaneNodes();
      self.listeners.each(function(l) { l.onLoginSucceeded(); });

      self.login_phase = null;
      // we should try to scrobble
      self.scrobble();
    } catch (e) {
      dump("Exception thrown: " + e + "\n");
    }
  }, function failure() {
    self.loggedIn = false;
    self.error = null;
    self.updateServicePaneNodes();
    self.listeners.each(function(l) { l.onLoginFailed(); });
    self.login_phase = null;
  });

  // get loved tracks information
  this.getLovedTracks();
}

sbLastFm.prototype.login = function sbLastFm_login(clearSession) {
  var self = this;

  this.listeners.each(function(l) { l.onLoginBegins(); });

  if (clearSession) {
    dump("clearing session\n");
    Application.prefs.setValue("extensions.lastfm.session_key", "")
  }

  // grab the previously saved session key
  self.sk = Application.prefs.getValue("extensions.lastfm.session_key", "")
  if (self.sk == "") {
    // no previously saved session
    self.authorise(function authorisationSuccess() {
                     self.postHandshake();
                   }, function authorisationFailure() {
                   });
    return;
  } else {
    self.handshake(function hsSuccess() {
                     self.postHandshake();
                   }, function hsFailure() {
                     self.login_phase = null;
                     dump("handshake with existing sk FAILURE\n");
                     dump("resetting saved key and retrying\n");
                     Application.prefs.setValue("extensions.lastfm.session_key",
                                                "");
                     self.sk = null;
                     self.login();
                   });
  }
}

// authenticate against the web services authentication process
sbLastFm.prototype.authorise =
function sbLastFm_authorise(onSuccess, onFailure) {
  var self = this;
  this.apiAuth(function apiSuccess() {
                 dump("Last.fm Web Services Authentication process COMPLETE\n");

                 self.listeners.each(function(l) { l.onLoginBegins(); });
                 self.handshake(function hsSuccess() {
                                  dump("handshake SUCCESS\n");

                                  if (typeof(onSuccess) == "function")
                                    onSuccess();
                                }, function hsFailure() {
                                     dump("handshake FAILURE\n");
                                     if (typeof(onFailure) == "function")
                                       onFailure();
                                });
               }, function apiFailure() {
                    dump("Last.fm Web Services " +
                         "Authentication process FAILED\n");
               });
}

sbLastFm.prototype.cancelLogin =
function sbLastFm_cancelLogin() {
  var phase;
  if (!this.login_phase) {
    dump("No active login to cancel\n");
    return;
  }
  switch (this.login_phase) {
    case AUTH_PHASE_HANDSHAKE:
      phase = "handshake";
      if (this._handshake_xhr)
        this._handshake_xhr.abort();
      break;
    case AUTH_PHASE_TOKEN_REQUEST:
      phase = "authorisation token request";
      if (this._token_xhr)
        this._token_xhr.abort();
      break;
    case AUTH_PHASE_WEB_LOGIN_GEO:
      phase = "last.fm geo-specific website login";
      if (this._weblogin_xhr)
        this._weblogin_xhr.abort();
      break;
    case AUTH_PHASE_WEB_LOGIN:
      phase = "last.fm website login";
      if (this._weblogin_xhr)
        this._weblogin_xhr.abort();
      break;
    case AUTH_PHASE_APP_APPROVE:
      phase = "application authorisation";
      if (this._appauth_xhr)
        this._appauth_xhr.abort();
      break;
    case AUTH_PHASE_SESSION_REQUEST:
      phase = "session request";
      if (this._session_xhr)
        this._session_xhr.abort();
      break;
    case AUTH_PHASE_USER_INFO:
      phase = "user info/profile download";
      if (this._info_xhr)
        this._info_xhr.abort();
      break;
  }
  dump("CANCEL: " + phase + "\n");
  this.listeners.each(function(l) { l.onLoginCancelled(); });
  this.logout();
}

// logout is pretty simple
sbLastFm.prototype.logout =
function sbLastFm_logout() {
  this.session = null;
  this.sk = null;
  this.nowplaying_url = null;
  this.submission_url = null;
  this.loggedIn = false;
  this.updateServicePaneNodes();
}


// do the handshake
sbLastFm.prototype.handshake =
function sbLastFm_handshake(success, failure) {
  // have default callbacks
  if (!success) success = function() { }
  if (!failure) failure = function(x) { }

  // make the url
  var timestamp = Math.round(Date.now()/1000).toString();
  this.login_phase = AUTH_PHASE_HANDSHAKE;
  dump("handshake session key: " + this.sk + "\n");
  var hs_url = AUTH_URL + '?hs=true&p=1.2.1&c=sbd&v=1.2' +
    '&u=' + this.username + '&t=' + timestamp + '&a=' +
    md5(API_SECRET + timestamp) + '&api_key=' + API_KEY + '&sk=' + this.sk;

  function failureOccurred(aAuthFailure, responseText) {
    dump("handshake FAILED with code: " + aAuthFailure + "\n");
    dump("Response text: " + responseText + "\n\n");
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
      failureOccurred(HANDSHAKE_FAILURE_OTHER, self._handshake_xhr.responseText);
      return;
    }
    if (response_lines[0] == 'BADAUTH') {
      self.log('handshake got authentication failure');
      failureOccurred(HANDSHAKE_FAILURE_AUTH, self._handshake_xhr.responseText);
      return;
    }
    if (response_lines[0] != 'OK') {
      self.log('handshake got unexpected status: '+response_lines[0]);
      failureOccurred(HANDSHAKE_FAILURE_OTHER, self._handshake_xhr.responseText);
      return;
    }
    // save the results of parsing out
    self.session = response_lines[1];
    self.nowplaying_url = response_lines[2];
    self.submission_url = response_lines[3];

    // increment metrics
    self._metrics.metricsInc('lastfm', 'login', null);

    // save the credentials
    Logins.set(self.username, self.password);

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
function sbLastFm_updateProfile(onSuccess, onFailure) {
  var self = this;
  this.login_phase = AUTH_PHASE_USER_INFO;
  this._info_xhr = this.apiCall("user.getInfo", {},
    function response(success, xml, xmlText) {
      if (!success) {
        dump("user.getInfo FAILED\n");
        if (typeof(onFailure) == "function")
          onFailure();
        return;
      }

      function text(tag) {
        var tags = xml.getElementsByTagName(tag);
        if (tags.length) {
          return tags[0].textContent;
        } else {
          return '';
        }
      }

      self._subscriber = (text('subscriber') == 1)
      if (Application.prefs.getValue("extensions.lastfm.subscriber_override",
                                     false))
        self._subscriber = true;
      self.realname = text('realname');
      self.playcount = parseInt(text('playcount'));
      self.avatar = text('image');
      self.profileurl = text('url');
      self.listeners.each(function(l) { l.onProfileUpdated(); });

      self.updateServicePaneNodes();

      if (typeof(onSuccess) == "function")
        onSuccess();
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

  if (submissions.length == 0)
    return;

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
  //dump("*** submitted: " + url + " // params: " + uneval(params) + "\n");
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

// get the list of tracks the user has loved
sbLastFm.prototype.getLovedTracks =
function sbLastFm_getLovedTracks() {
  var self = this;
  var lovedLimit = Application.prefs.getValue("extensions.lastfm.loved_limit",
                                              100);
  this.apiCall('user.getLovedTracks',
               { user: self.username,
                 limit: lovedLimit },
               function response(success, xml, xmlText) {
                xmlText = xmlText.replace(
                  /<\?xml version="1.0" encoding="[uU][tT][fF]-8"\?>/, "");
                var x = new XML(xmlText);

                self.lovedTracks = new Object();
                var i = 0;
                for each (var track in x..track) {
                  var trackName = track.name.toString().toLowerCase();
                  var artistName = track.artist.name.toString().toLowerCase();
                  self.lovedTracks[trackName + "@@" + artistName] = true;
                }
               });
}

// log in to the last.fm website so that if we get directed over to web links
// from radio links we'll already have a logged in state
sbLastFm.prototype.webLogin =
function sbLastFm_webLogin(success, failure) {
  var self = this;
  var postdata = new Object;
  postdata["username"] = this.username;
  postdata["password"] = this.password;

  // Need to determine which Last.fm geo site they get redirected to
  // Hit up www.last.fm/login and let's see where we get redirected to
  this.login_phase = AUTH_PHASE_WEB_LOGIN_GEO;
  this._weblogin_xhr = POST("http://last.fm/login", null,
      function(xhr) {
        var loginURL = decodeURI(xhr.channel.URI.spec);
        dump("Geo login URL: " + loginURL + "\n");

        if (loginURL.match(/backto/)) {
          // if the user isn't already web authenticated against the
          // geo-specific last.fm homepage, then they'll get directed to
          // last.fm/login/... backto=... where they'll login against the
          // main US last.fm site and get redirected to a successCallback
          // on the geo-specific site
          // so parse the geo base domain out of that successCallback
          self.geoBaseDomain = loginURL.replace(/^.*backto=http%3A%2F%2F/, "")
                                       .replace(/%2F.*$/, "");
        } else {
          // if the user is already authenticated, then they just immediately
          // get redirected over to that geo-specific homepage
          // (www.lastfm.fr/home for example), so just parse the TLD out of
          // that
          var loginURI = newURI(loginURL);
          var eTLDService = Cc["@mozilla.org/network/effective-tld-service;1"]
                              .getService(Ci.nsIEffectiveTLDService);
          self.geoBaseDomain = "www." + eTLDService.getBaseDomain(loginURI);
        }
        dump("Geo base domain: " + self.geoBaseDomain + "\n");

        this.login_phase = AUTH_PHASE_WEB_LOGIN;
        self._weblogin_xhr = POST(loginURL, postdata,
          function(xhr) {
            if (typeof(success) == "function")
              success();
          },
          function(xhr) {
            if (typeof(failure) == "function")
              failure();
          });
      },
      function(xhr) {
      });
}

// tune the current radio session into a particular lastfm:// uri
sbLastFm.prototype.tuneStation =
function sbLastFm_tuneStation(station, onSuccess, onFailure) {
  var self = this;
  dump("radio.Tune, tuning to: " + station + "\n");
  this.apiCall('radio.Tune', { station: station },
    function response(success, xml, xmlText) {
      if (!success) {
        dump("Failed to tune station: " + station + "\n");
        if (typeof(onFailure) == "function")
          onFailure();
        return;
      }

      function text(key) {
        tags = xml.getElementsByTagName(key);
        if (tags.length) {
          return tags[0].textContent;
        } else {
          return '';
        }
      }
      self.station_radio_url = station;
      self.station_webpage_url = text('url');
      self.station_name = text('name');
      if (typeof(onSuccess) == "function")
        onSuccess();
    });
}

// get more tracks from Last.fm's servers into the radio playlist
sbLastFm.prototype.requestMoreRadio =
function sbLastFm_requestMoreRadio(onSuccess, onFailure) {
  var self = this;
  /*
  this.getXML(this.radio_base_url + '/xspf.php?' +
    urlencode({sk: this.radio_session, discovery: 0, desktop: '1.3.1.1'}),
  */
  this.apiCall('radio.getPlaylist', { discovery: true, rtp: true },
    function response(success, xspf, xmlText) {
      if (!success) {
        dump("radio.getPlaylist FAILED\n");
        if (typeof(onFailure) == "function")
          onFailure();
        return;
      }

      if (xspf.getElementsByTagName('error').length > 0) {
        dump("Failed to tune.  User might not be a subscriber?\n");
        dump(xmlText + "\n\n");
        if (typeof(onFailure) == "function")
          onFailure();
        return;
      }

      dump("**** SUCCESSFULLY loaded new XSPF data\n");
      var tracks = xspf.getElementsByTagName('track');
      function tagValue(trackNumber, tagName) {
        try {
          return tracks[i].getElementsByTagName(tagName)[0].textContent;
        }
        catch(e) { return ''; }
      }

      for (var i=0; i<tracks.length; i++) {
        // create a media item from the XSPF
        var location = tagValue(i, 'location');
        dump("URL: " + tagValue(i, 'title') + " -- " + location + "\n");
        var mediaItem =
          self.radio_mediaList.library.createMediaItem(newURI(location));
        // set up standard track metadata
        mediaItem.setProperty(SBProperties.trackName, tagValue(i, 'title'));
        mediaItem.setProperty(SBProperties.artistName, tagValue(i, 'creator'));
        mediaItem.setProperty(SBProperties.albumName, tagValue(i, 'album'));
        mediaItem.setProperty(SBProperties.primaryImageURL,
                              tagValue(i, 'image'));
        mediaItem.setProperty(SBProperties.duration,
                              parseInt(tagValue(i, 'duration'))*1000);
        // set up some lastfm-specific metadata
        mediaItem.setProperty(PROPERTY_TRACKID, tagValue(i, 'identifier'));
        var extensions = tracks[i].getElementsByTagName("extension")[0];
        mediaItem.setProperty
          (PROPERTY_ARTISTPAGE,
           extensions.getElementsByTagName("artistpage")[0].textContent);
        mediaItem.setProperty
          (PROPERTY_TRACKAUTH,
           extensions.getElementsByTagName("trackauth")[0].textContent);

        self.radio_mediaList.add(mediaItem);
      }

      if (typeof(onSuccess) == "function")
        onSuccess();
    });
}

sbLastFm.prototype.saveRecentStation =
function sbLastFm_saveRecentStation(name, stationUrl, url, sImageUrl,
                                    mImageUrl, lImageUrl)
{
  var thisStation = { name: name, stationUrl: stationUrl, url: url,
                      sImageUrl: sImageUrl, mImageUrl: mImageUrl,
                      lImageUrl: lImageUrl };
  var prevIdx = -1;

  var recentStations = this._jsonSvc.decode(
    Application.prefs.getValue("extensions.lastfm.recent.stations", "[]"));
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
  // Ensure we have a session authenticated, and that the user is a
  // subscriber
  if (!this.sk) {
    // not logged in
    dump("not logged in\n");
    return false;
  }
  if (!this._subscriber) {
    // not a subscriber
    dump("not a subscriber\n");
    return false;
  }

  // See if we're already playing a station.  If not, save the shuffle/repeat
  // settings since we reset them later on in the success handler
  if (SBDataGetStringValue("lastfm.radio.station") == "" &&
      !SBDataGetBoolValue("lastfm.radio.requesting"))
  {
    dump("saving previous shuffle/repeat modes\n");
    this.prevShuffleMode = this._mediacoreManager.sequencer.mode;
    this.prevRepeatMode = this._mediacoreManager.sequencer.repeatMode;
  }

  // clear the playlist
  this.radio_mediaList.clear();

  // set the station name to the URI
  this.station_radio_url = station;
  this.station_name = station;

  dump("Attempting to play: " + station + "\n");

  // manually set the buffering state so the user has some feedback that
  // Nightingale is trying to do something
  // this is kinda hacky. sorry :(
  SBDataSetStringValue("lastfm.radio.station", "");
  SBDataSetStringValue("metadata.artist", "");
  SBDataSetStringValue("metadata.album", "");
  SBDataSetStringValue("metadata.title", "Loading Last.fm station...");
  SBDataSetBoolValue("faceplate.seenplaying", true);
  SBDataSetBoolValue("faceplate.buffering", true);
  SBDataSetBoolValue("lastfm.radio.requesting", false);
  /* what i *really* want is createPane. :-(
  var faceplateMgr =  Cc["@getnightingale.com/faceplate/manager;1"]
    .getService(Ci.sbIFaceplateManager);
  var pane = faceplateMgr.createPane("nightingale-lastfm", "Last.fm Radio",
    "chrome://nightingale/content/bindings/facePlate.xml#progress-pane");
  pane.setData("label1", "Loading Last.fm Radio Playlist");
  pane.setData("progress-mode", "undetermined");
  pane.setData("label1-hidden", false);
  pane.setData("label2-hidden", true);
  pane.setData("progress-hidden", false);
  */

  // reset failure count
  this.radio_failure_count = 0;
  var self = this;
  this.tuneStation(station,
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
                   self.station_radio_url);
              SBDataSetStringValue("lastfm.radio.station", "");
              SBDataSetStringValue("metadata.artist", "");
              SBDataSetStringValue("metadata.album", "");
              SBDataSetStringValue("metadata.title", "");
              SBDataSetBoolValue("faceplate.seenplaying", false);
              SBDataSetBoolValue("faceplate.buffering", false);
              SBDataSetBoolValue("lastfm.radio.requesting", false);
            }
          });

      },
      function radioStation_failure() {
        dump("FAIL FAIL FAIL to authenticate.\n");
      });
  return true;
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
sbLastFm.prototype.apiAuth = function sbLastFm_apiAuth(onSuccess, onFailure) {
  // clear our old session
  this.sk = null;
  Application.prefs.setValue("extensions.lastfm.session_key", "");

  // clear any web cookies we may have already
  dump("web cookies cleared\n");
  var cookieMgr = Cc["@mozilla.org/cookiemanager;1"]
                    .getService(Ci.nsICookieManager);
  cookieMgr.remove(".last.fm", "Session", "/", false);
  cookieMgr.remove(".last.fm", "s_cc", "/", false);
  cookieMgr.remove(".last.fm", "s_sq", "/", false);
  cookieMgr.remove(".last.fm", "wwwlang", "/", false);
  cookieMgr.remove(".last.fm", "__qcb", "/", false);
  cookieMgr.remove(".last.fm", "TREA", "/", false);
  cookieMgr.remove(".last.fm", "s_nr", "/", false);
  cookieMgr.remove(".last.fm", "__qca", "/", false);
  cookieMgr.remove(".last.fm", "s_lastvisit", "/", false);
  cookieMgr.remove(".last.fm", "LastUser", "/", false);
  cookieMgr.remove(".last.fm", "AnonTrack", "/", false);
  cookieMgr.remove(".last.fm", "fastq", "/", false);

  // get a lastfm desktop session
  var self = this;
  this.webLogin(function success() {
    dump("webLogin SUCCESS\n");

    self.login_phase = AUTH_PHASE_TOKEN_REQUEST;
    self._token_xhr = self.apiCall('auth.getToken', { },
      function response(success, xml, xmlText) {
        if (!success) {
          dump("auth.getToken: FAILED TO AUTHENTICATE: " + xmlText + "\n\n");
          return;
        }

        var authtoken = xml.getElementsByTagName('token');
        if (authtoken.length != 1) {
          dump("auth.getToken: FAILED TO FIND TOKEN: " + xmlText + "\n\n");
          return;
        }
        authtoken = authtoken[0].textContent;
        dump("auth.getToken SUCCESS: " + authtoken + "\n");

        var window = Cc['@mozilla.org/appshell/window-mediator;1']
                       .getService(Ci.nsIWindowMediator)
                       .getMostRecentWindow('Nightingale:Main');
        if (!window) {
          self.listeners.each(function(listener) {
            listener.onLoginFailed();
          });
          return;
        }
        var gBrowser = window.gBrowser;

        function removeAuthListeners() {
          gBrowser.removeEventListener("DOMContentLoaded",
                                       self._authListener, false);
          gBrowser.removeEventListener("unload", removeAuthListeners, false);
          authTab.removeEventListener("TabClose",
                                      self._authTabCloseListener, false);
        }

        // Create a listener for last.fm's authorization grant page.
        self._authListener = function (e) {

          // Ensure we are on the right tab.
          if (gBrowser.getBrowserForDocument(e.target) !=
              gBrowser.getBrowserForTab(authTab)) {
            return;
          }
  
          // We're listening for the LastFM "Permissions Granted" page. It will
          // have pathname "/api/grantaccess" on the last.fm domain or a
          // localized version such as lastfm.fr
          var loc = e.target.location;
          if (!/last\.?fm/.test(loc.host)) {
            // If we get here, it implies that the user navigated away from
            // LastFM without authorizing.
            removeAuthListeners();
            self.listeners.each(function(listener) {
              listener.onLoginFailed();
            });
            return;
          }

          if (loc.pathname.search(/grantaccess/i) == -1) {
            // Ignore LastFM pages that aren't the "Permissions Granted" page.
            return;
          }

          // We should be on the grantaccess page now, so remove the listeners
          // and try to grab a session key.
          removeAuthListeners();

          self.login_phase = AUTH_PHASE_SESSION_REQUEST;
          self._session_xhr = self.apiCall('auth.getSession', {
              token: authtoken
            },
            function response(success, xml, xmlText) {
              if (!success) {
                dump("auth.getSession: FAILED TO AUTHENTICATE: " +
                  xmlText + "\n\n");
                return;
              }
              var keys = xml.getElementsByTagName("key");
              if (keys.length != 1) {
                dump("auth.getSession: FAILED TO AUTH. TOKEN: " +
                  xmlText + "\n\n");
                return;
              }
              self.sk = keys[0].textContent;
              dump("auth.getSession: AUTHENTICATED\n");
              dump("session key: " + self.sk + "\n");
              Application.prefs.setValue('extensions.lastfm.session_key',
                self.sk);
              var subscribers = xml.getElementsByTagName("subscriber");
              if (subscribers.length == 1)
                self._subscriber = (subscribers[0].textContent == "1");
              if (Application.prefs.getValue(
                    "extensions.lastfm.subscriber_override", false))
                self._subscriber = true;
              dump("subscriber: " + self._subscriber + "\n");
              self.listeners.each(function(l) {
                l.onAuthorisationSuccess();
              });

              if (typeof(onSuccess) == "function")
                onSuccess();
          });
        } 

        // Load the user authorization page.
        var authURL = "http://" + self.geoBaseDomain + "/api/auth?api_key=" +
                      API_KEY + "&token=" + authtoken;

        gBrowser.addEventListener("DOMContentLoaded", self._authListener, false);
        // Make sure we don't leak the listeners if the user takes no action.
        gBrowser.addEventListener("unload", removeAuthListeners, false); 

        var authTab = gBrowser.loadOneTab(authURL, null, null, null, false);
      
        // The user could close the auth page tab without granting permission.
        self._authTabCloseListener = function(e) {
          removeAuthListeners();
          self.listeners.each(function(listener) {
            listener.onLoginFailed();
          });
        }
        
        authTab.addEventListener("TabClose", self._authTabCloseListener, false);

    }, function failure() {   // auth.getToken failure
      dump("webLogin FAILED\n");
      self.listeners.each(function(listener) {
        listener.onLoginFailed();
      });
    }); // auth.getToken api call
  }, function() {
    dump("weblogin FAILURE\n");
    self.listeners.each(function(listener) {
      listener.onLoginFailed();
    });
  }); // weblogin
}


sbLastFm.prototype.apiCall =
function sbLastFm_apiCall(method, params, responseCallback) {
  // make a new Last.fm Web Service API call
  // see: http://www.last.fm/api/rest
  // note: this isn't really REST.

  function callback(success, xml, xmlText) {
    if (typeof(responseCallback) == 'function') {
      responseCallback(success, xml, xmlText);
    } else {
      responseCallback.responseReceived(success, xml, xmlText);
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
  return POST(API_URL, post_params, function (xhr) {
    if (!xhr.responseXML) {
      // we expect all API responses to have XML
      callback(false, null, null);
      return;
    }
    if (xhr.responseXML.documentElement.tagName != 'lfm' ||
        !xhr.responseXML.documentElement.hasAttribute('status')) {
      // we expect the root (document) element to be <lfm status="...">
      callback(false, xhr.responseXML, xhr.responseText);
      return;
    }
    if (xhr.responseXML.documentElement.getAttribute('status' == 'failed')) {
      // the server reported an error
      self.log('Last.fm Web Services Error: '+xhr.responseText);
      callback(false, xhr.responseXML, xhr.responseText);
      return;
    }
    // all should be good!
    callback(true, xhr.responseXML, xhr.responseText);
  }, function (xhr) {
    callback(false, null, null);
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
    var banCount = 0;

    // collect the playlist history entries into objects that can be sent
    // to last.fm's audioscrobbler api
    for (var i=entry_list.length-1; i>=0; i--) {
      // don't scrobble non-audio tracks or tracks w/ missing metadata
      // scrobbled anyway so we don't try again in the future
      if (entry_list[i].item.getProperty(SBProperties.contentType) != "audio"
          || entry_list[i].item.getProperty(SBProperties.artistName) == null
          || entry_list[i].item.getProperty(SBProperties.trackName) == null)
      {
        continue;
      }

      var rating = '';
      if (entry_list[i].hasAnnotation(ANNOTATION_LOVE)) {
        rating = 'L';
      } else if (entry_list[i].hasAnnotation(ANNOTATION_BAN)){
        rating = 'B';
        banCount++;
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
                          Math.round(entry_list[i].timestamp/1000000),
                          rating,
                          source));
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
            // Do nothing, this is handled in trackBan()
          }
        }
        // Only non-banned songs are counted in overall play count
        self.playcount += (entry_list.length-banCount);
        self.listeners.each(function(l) { l.onProfileUpdated(); });
        self.error = null;
        // increment metrics
        self._metrics.metricsAdd('lastfm', 'scrobble', null, entry_list.length);
      });
  }
}

sbLastFm.prototype.addTags =
function sbLastFm_addTags(aMediaItem, tagString, success, failure) {
  var self = this;
  this.apiCall('track.addTags', {
    track: aMediaItem.getProperty(SBProperties.trackName),
    artist: aMediaItem.getProperty(SBProperties.artistName),
    tags: tagString,
  }, function() {
      dump("Successfully added tags: " + tagString + "\n");
    self.listeners.each(function(l) {
      if (typeof(l.onItemTagsAdded) == "function") {
        var tags = tagString.split(",");
        for (var i in tags)
          tags[i] = tags[i].replace(/^\s*/, "").replace(/\s*$/, "");
        l.onItemTagsAdded(aMediaItem, tags);
      }
    });
    if (typeof(success) == "function")
      success();
  }, function() {
    dump("Failed to add tags: " + tagString + "\n");
    if (typeof(failure) == "function")
      failure();
  });
}

sbLastFm.prototype.removeTag =
function sbLastFm_removeTag(aMediaItem, aTagName) {
  var self = this;
  this.apiCall('track.removeTag', {
    track: aMediaItem.getProperty(SBProperties.trackName),
    artist: aMediaItem.getProperty(SBProperties.artistName),
    tag: aTagName,
  }, function() {
    dump("Successfully removed tag: " + aTagName + "\n");
    self.listeners.each(function(l) {
    if (typeof(l.onItemTagRemoved) == "function")
      l.onItemTagRemoved(aMediaItem, aTagName);
    });
    delete this.userTags[aTagName];
  }, function() {
    dump("Failed to remove tag: " + aTagName + "\n");
  });
}

// Note: this web services call will eventually be deprecated and rely
// strictly on the scrobble (see http://www.last.fm/api/submissions)
sbLastFm.prototype.trackBan =
function sbLastFm_trackBan(aMediaItem) {
  if (aMediaItem.getProperty(SBProperties.artistName) != null &&
      aMediaItem.getProperty(SBProperties.trackName) != null)
  {
    // Only trigger web services call, scrobble requires
    // additional minimum requirements and is handled in
    // scrobble()
    this.apiCall('track.ban', {
        track: aMediaItem.getProperty(SBProperties.trackName),
        artist: aMediaItem.getProperty(SBProperties.artistName)
    }, function response(success, xml) { });
  }
}

// love and ban
sbLastFm.prototype.loveBan =
function sbLastFm_loveBan(aMediaItem, aLove) {
  this.loveTrack = aMediaItem;
  this.love = aLove;
  var existing = false;
  if (aMediaItem) {
    var trackName = aMediaItem.getProperty(SBProperties.trackName).toLowerCase();
    var artistName = aMediaItem.getProperty(SBProperties.artistName).toLowerCase();

    if (this.lovedTracks[trackName + "@@" + artistName])
      existing = true;
    if (aLove) {
      dump("loving this track\n");
      this.lovedTracks[trackName + "@@" + artistName] = true;
    } else {
      dump("deleting this track\n");
      delete this.lovedTracks[trackName + "@@" + artistName];
      this.trackBan(aMediaItem);
    }
  }
  this.listeners.each(function(l) {
    l.onLoveBan(aMediaItem, aLove, existing);
  });
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
                  if (item.item.guid == self.loveTrack.guid) {
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
      if (r && this._subscriber && this.sk) {
        var radio = "lastfm://" + r[1];
        channel.cancel(Components.results.NS_BINDING_ABORTED);
        this.radioPlay(radio);
      }
    }
  }
  else if (topic == "nightingale-library-manager-before-shutdown") {
    this.shutdown()
  }
}

// sbIMediacoreEventListener
sbLastFm.prototype.onMediacoreEvent =
function sbLastFm_onMediacoreEvent(aEvent) {
  switch(aEvent.type) {
    case Ci.sbIMediacoreEvent.STREAM_END:
    case Ci.sbIMediacoreEvent.STREAM_STOP:
      SBDataSetStringValue('lastfm.radio.station', '');
      this.onStop();

      // reset the shuffle and repeat modes to the previous setting
      if (this.radio_playing) {
        this._mediacoreManager.sequencer.mode = this.prevShuffleMode;
        this._mediacoreManager.sequencer.repeatMode = this.prevRepeatMode;
      }
      break;
    case Ci.sbIMediacoreEvent.VIEW_CHANGE:
      if (this.radio_playing) {
        this._mediacoreManager.sequencer.mode = this.prevShuffleMode;
        this._mediacoreManager.sequencer.repeatMode = this.prevRepeatMode;
      }
      this.radio_playing = (aEvent.data.mediaList == this.radio_mediaList);
      break;
    case Ci.sbIMediacoreEvent.BEFORE_TRACK_CHANGE:
      if (this.radio_playing) {
        SBDataSetStringValue('lastfm.radio.station', this.station_name);
      } else {
        SBDataSetStringValue('lastfm.radio.station', '');
      }
      break;
    case Ci.sbIMediacoreEvent.TRACK_CHANGE:
      // if we're playing the last track in a radio view time to get more!
      if (this.radio_playing && !this._mediacoreManager.sequencer.nextItem) {
        dump("OUT OF RADIO TRACKS... requesting more\n");
        SBDataSetBoolValue('lastfm.radio.requesting', true);
        SBDataSetStringValue("faceplate.status.text",
                             "Requesting next set of tracks from Last.fm...");
        this.requestMoreRadio(function(){
          SBDataSetBoolValue('lastfm.radio.requesting', false);
          SBDataSetStringValue("faceplate.status.text",
                               "Next tracks loaded successfully!");
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
    default:
      break;
  }
}

// invoked from onMediacoreEvent above
sbLastFm.prototype.onTrackChange =
function sbLastFm_onTrackChange(aItem) {
  // NOTE: This depends on the current assumption that onTrackChange will
  // be run before onEntriesAdded. Otherwise love/ban won't work.

  if (aItem.getProperty(SBProperties.artistName) == null ||
      aItem.getProperty(SBProperties.trackName) == null)
  {
    return;
  }

  // reset the love/ban state
  this.loveBan(null, false);
  var trackName = aItem.getProperty(SBProperties.trackName).toLowerCase();
  var artistName = aItem.getProperty(SBProperties.artistName).toLowerCase();
  if (this.lovedTracks && this.lovedTracks[trackName + "@@" + artistName]) {
    this.loveBan(aItem, true);
  }

  // send now playing info if appropriate
  if (this.shouldScrobble && this.loggedIn &&
      aItem.getProperty(SBProperties.excludeFromHistory) != '1') {
    this.nowPlaying(new PlayedTrack(aItem));
  }

  this.userTags = new Object();
  this.globalTags = new Object();
  self = this;

  // update the metaverse's tags for this track
  this.apiCall('track.getInfo', {
    track: aItem.getProperty(SBProperties.trackName),
    artist: aItem.getProperty(SBProperties.artistName)
  }, function response(success, xml) {
    // update the tagPanel with the tags for this track
    var tagElements = xml.getElementsByTagName('tag');
    for (var i=0; i<tagElements.length; i++) {
      var tag = tagElements[i].childNodes[1].textContent;
      self.globalTags[tag] = false;
    }
  });

  // update the personal tags for this track
  this.apiCall('track.getTags', {
    track: aItem.getProperty(SBProperties.trackName),
    artist: aItem.getProperty(SBProperties.artistName)
  }, function response(success, xml) {
    // update the tagPanel with the tags for this track
    var tagElements = xml.getElementsByTagName('tag');
    for (var i=0; i<tagElements.length; i++) {
      var tag = tagElements[i].childNodes[1].textContent;
      self.userTags[tag] = true;
    }
  });
}

sbLastFm.prototype.onStop = function sbLastFm_onStop() {
  // reset the love/ban state
  this.loveBan(null, false);
}

sbLastFm.prototype.shutdown = function sbLastFm_shutdown() {
  // remove all observers
  this._observerSet.removeAll();
  this._observerSet = null;

  // stop listening to the playlist playback service
  this._mediacoreManager.removeListener(this);

  // remove ourselves as a playlist history listener
  this._playbackHistory.removeListener(this);
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbLastFm]);
}
