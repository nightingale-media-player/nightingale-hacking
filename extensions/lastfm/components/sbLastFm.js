// these constants make everything better
const Cc = Components.classes;
const CC = Components.Constructor;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

// our annotations
const ANNOTATION_SCROBBLED = 'http://www.songbirdnest.com/lastfm#scrobbled';
const ANNOTATION_HIDDEN = 'http://www.songbirdnest.com/lastfm#hidden';

// Last.fm API key, secret and URL
const API_KEY = '4d5bce1e977549f10623b51dd0e10c5a';
const API_SECRET = '3ebb03d4561260686b98388037931f11'; // obviously not secret
const API_URL = 'http://ws.audioscrobbler.com/2.0/';

// how often should we try to scrobble again?
const TIMER_INTERVAL = (5*60*1000); // every five minutes sounds lovely

// import the XPCOM helper
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

// import the properites helper
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

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
  s = '';
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

  // session info
  this.session = null;
  this.nowplaying_url = null;
  this.submission_url = null;

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

  // the online state
  this._online = false;
  this.__defineGetter__('online', function() { return this._online; });
  this.__defineSetter__('online', function(aOnline) {
    this._online = aOnline;
    this.listeners.each(function(l) { aOnline ? l.onOnline() : l.onOffline() });
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
  var pps = Cc['@songbirdnest.com/Songbird/PlaylistPlayback;1']
    .getService(Ci.sbIPlaylistPlayback);
  pps.addListener(this);

  // track metrics
  this._metrics = Cc['@songbirdnest.com/Songbird/Metrics;1']
    .getService(Ci.sbIMetrics);
}
// XPCOM Magic
sbLastFm.prototype.classDescription = 'Songbird Last.fm Service'
sbLastFm.prototype.contractID = '@songbirdnest.com/lastfm;1';
sbLastFm.prototype.classID =
    Components.ID('13bc0c9e-5c37-4528-bcf0-5fe37fcdc37a');
sbLastFm.prototype.QueryInterface =
    XPCOMUtils.generateQI([Ci.sbIPlaybackHistoryListener, Ci.nsIObserver,
        Ci.sbIPlaylistPlaybackListener]);

// failure handling
sbLastFm.prototype.hardFailure =
function sbLastFm_hardFailure(message) {
  this.error =
      'An error occurred while communicating with the Last.fm servers.';
  this.hardFailures++;

  if (this.hardFailures >= 3) {
    // after three hard failures, try to re-handshake
    Cu.reportError('Last.fm hard failure: ' + message +
                   '\nFalling back to handshake');
    this.hardFailures = 0;
    this.session = null;
    this.handshake(function () { }, function() { });
  } else {
    // just count and log this
    Cu.reportError('Last.fm hard failure: ' + message);
  }
}

sbLastFm.prototype.badSession =
function sbLastFm_badSession() {
  dump('badSession\n');
  // the server rejected the current session.
  // we need to re-handshake, but we don't care about the result
  this.session = null;
  this.handshake(function () { }, function() { });
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


  }, function failure(aAuthFailed) {
    self.loggedIn = false;
    self.error = 'Login failed';
    self.listeners.each(function(l) { l.onLoginFailed(); });
    self.online = false;
  }, function auth_failure() {
    self.loggedIn = false;
    self.error = 'Login failed';
    self.listeners.each(function(l) { l.onLoginFailed(); });
    self.online = false;
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
  // make the url
  var timestamp = Math.round(Date.now()/1000).toString();
  var hs_url = 'http://post.audioscrobbler.com/?hs=true&p=1.2&c=sbd&v=0.1' +
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
    /* loaded */
    if (self._handshake_xhr.status != 200) {
      Cu.reportError('status: '+self._handshake_xhr.status);
      failureOccurred(false);
      return;
    }
    var response_lines = self._handshake_xhr.responseText.split('\n');
    if (response_lines.length < 4) {
      Cu.reportError('not enough lines: '+response_lines.toSource());
      failureOccurred(false);
      return;
    }
    if (response_lines[0] == 'BADAUTH') {
      Cu.reportError('auth failed');
      failureOccurred(true);
      return;
    }
    if (response_lines[0] != 'OK') {
      Cu.reportError('handshake failure: '+response_lines[0]);
      failureOccurred(false);
      return;
    }
    self.session = response_lines[1];
    self.nowplaying_url = response_lines[2];
    self.submission_url = response_lines[3];

    // download profile info
    self.updateProfile(function success() {
      self.loggedIn = true;
      self.error = null;
      self.listeners.each(function(l) { l.onLoginSucceeded(); });
      self.online = true;

      // we should try to scrobble
      self.scrobble();
    }, function failure() {
      self.loggedIn = false;
      self.error = null;
      self.listeners.each(function(l) { l.onLoginFailed(); });
      self.online = false;
    });

    success();
  };
  this._handshake_xhr.onerror = function(event) {
    /* faileded */
    Cu.reportError('handshake error');
    failureOccurred(false);
  };
  this._handshake_xhr.open('GET', hs_url, true);
  this._handshake_xhr.send(null);
}

// update profile data
sbLastFm.prototype.updateProfile =
function sbLastFm_updateProfile(succeeded, failed) {
  var url = 'http://ws.audioscrobbler.com/1.0/user/' +
    encodeURIComponent(this.username) + '/profile.xml';
  self = this;
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
function sbLastFm_submit(submissions, success, failure) {
  // if we don't have a session, we need to handshake first
  if (!this.session) {
    this.handshake(success, failure);
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
      success(xhr.responseXML);
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


// post to audioscrobbler (the old api)
sbLastFm.prototype.asPost =
function sbLastFm_asPost(url, params, success, hardfailure, badsession) {
  POST(url, params, function(xhr) {
    /* loaded */
    if (xhr.status != 200) {
      hardfailure('HTTP status ' + xhr.status + ' posting to ' + url);
    } else if (xhr.responseText.match(/^OK\n/)) {
      success();
    } else if (xhr.responseText.match(/^BADSESSION\n/)) {
      Cu.reportError('Bad Session when posting to last.fm');
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
  // get a lastfm mobile session
  var self = this;
  this.apiCall('auth.getMobileSession', {
    username: this.username,
    authToken: md5(this.username + md5(this.password))
  }, function success(xml) {
    dump('apiAuth success FTW!!!\n');
    var keys = xml.getElementsByTagName('key');
    if (keys.length == 1) {
      self.sk = keys[0].textContent;
    }
  }, function failure(xhr) {
    dump('apiAuth failuring\n');
  });
}


sbLastFm.prototype.apiCall =
function sbLastFm_apiCall(method, params, success, failure) {
  // make a new Last.fm Web Service API call
  // see: http://www.last.fm/api/rest
  // note: this isn't really REST.

  // create an object to hold the HTTP params
  var post_params = new Object();

  // copy in our params
  for (var k in params) { post_params[k] = params[k]; }

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
  sorted_params.join('');
  // hash them with the "secret" to get the API signature
  post_params.api_sig = md5(sorted_params+API_SECRET);

  // post the request
  POST(API_URL, post_params, function (xhr) {
    dump('apiCall POST success\n');
    dump(xhr.responseText+'\n');
    if (!xhr.responseXML) {
      // we expect all API responses to have XML
      failure(xhr);
      return;
    }
    if (xhr.responseXML.documentElement.tagName != 'lfm' ||
        !xhr.responseXML.documentElement.hasAttribute('status')) {
      // we expect the root (document) element to be <lfm status="...">
      failure(xhr);
      return;
    }
    if (xhr.responseXML.documentElement.getAttribute('status' == 'failed')) {
      // the server reported an error
      Cu.reportError('Last.fm Web Services Error: '+xhr.responseXML);
      failure(xhr);
      return;
    }
    // all should be good!
    success(xhr.responseXML);
  }, function (xhr) {
    dump('apiCall POST failure\n');
    failure(xhr);
  });
}


// try to scrobble pending tracks from playback history
sbLastFm.prototype.scrobble =
function sbLastFm_scrobble() {
  var entry_list = [];
  enumerate(this._playbackHistory.entries,
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
      scrobble_list.push(
          new PlayedTrack(entry_list[i].item,
                          Math.round(entry_list[i].timestamp/1000)));
    }
    // submit to the last.fm audioscrobbler api
    this.submit(scrobble_list,
      function success() {
        // on success mark all these as scrobbled
        for (i=0; i<entry_list.length; i++) {
          entry_list[i].setAnnotation(ANNOTATION_SCROBBLED, 'true');
        }
        self.playcount += entry_list.length;
        self.listeners.each(function(l) { l.onProfileUpdated(); });
        self.error = null;
        // increment metrics
        self._metrics.metricsAdd('lastfm', 'scrobble', null, entry_list.length);
      },
      function failure() {
        // failure happens - we'll try again later anyway
      });
  }
}


// sbIPlaybackHistoryListener
sbLastFm.prototype.onEntriesAdded =
function sbLastFm_onEntriesAdded(aEntries) {
  if (this.shouldScrobble) {
    this.scrobble();
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
  if (topic == 'timer-callback' && subject == this._timer) {
    // try to scrobble when the timer ticks
    this.scrobble();
  }
}


// sbIPlaylistPlaybackListener
sbLastFm.prototype.onTrackChange =
function sbLastFm_onTrackChange(aItem, aView, aIndex) {
  if (this.shouldScrobble && this.loggedIn) {
    this.nowPlaying(new PlayedTrack(aItem));
  }
}

sbLastFm.prototype.onStop = function sbLastFm_onStop() { }
sbLastFm.prototype.onBeforeTrackChange =
function sbLastFm_onBeforeTrackChange(aItem, aView, aIndex) { }
sbLastFm.prototype.onTrackIndexChange =
function sbLastFm_onTrackIndexChange(aItem, aView, aIndex) { }


function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbLastFm]);
}
