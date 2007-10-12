/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et: */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Darin Fisher <darin@meer.net>
 *  Boris Zbarsky <bzbarsky@mit.edu>
 *  John Gaunt <redfive@songbirdnest.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*
 * Original file is mozilla code, pulled into the songbird tree with some slight
 * modification since it is being used in a different system (XULRunner) than the
 * original (xpcshell).
 */

// This file contains common code that is loaded with each test file.

/*
 * These get defined in sbTestHarness.js so we don't need to do it here.
 * const Ci = Components.interfaces;
 * const Cc = Components.classes;
 * const Cr = Components.results;
*/

// setup the main thread event queue
var _quit = false;
var _fail = false;
var _running_event_loop = false;
var _tests_pending = 0;
var _test_name = "sbTestHarness";
var _consoleService = null;

function _TimerCallback(expr) {
  this._expr = expr;
}
_TimerCallback.prototype = {
  _expr: "",
  QueryInterface: function(iid) {
    if (iid.Equals(Ci.nsITimerCallback) || iid.Equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },
  notify: function(timer) {
    eval(this._expr);
  }
};

function doTimeout(delay, expr) {
  var timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback(new _TimerCallback(expr), delay, timer.TYPE_ONE_SHOT);
}

function doMain() {
  if (_quit)
    return;

  _consoleService.logStringMessage("*** [" + _test_name + "] - running event loop");

  var tm = Cc["@mozilla.org/thread-manager;1"].getService(Ci.nsIThreadManager);
  var mainThread = tm.mainThread;

  while(!_quit) {
    mainThread.processNextEvent(true);
  }

}

function doQuit() {
  //log("*** [" + _test_name + "] - exiting\n");

  _quit = true;

}

function doThrow(text) {
  _fail = true;
  _tests_pending = 0;
  doQuit();
  log("*** [" + _test_name + "] - CHECK FAILED: " + text);
  var frame = Components.stack;
  while (frame != null) {
    log(frame);
    frame = frame.caller;
  }
  throw Cr.NS_ERROR_ABORT;
}

function assertFalse(aTest, aMessage) {
  if (aTest) {
    var msg = (aMessage != null) ? ( " : " +  aMessage ) : "";
    doThrow(aTest + msg);
  }
}

function assertTrue(aTest, aMessage) {
  assertFalse(!aTest, aMessage);
}

function assertEqual( aExpected, aActual, aMessage) {
  if (aExpected != aActual) {
    var msg = (aMessage != null) ? ( " : " +  aMessage ) : "";
    doThrow(aExpected + " != " + aActual + msg);
  }
}

function assertNotEqual( aExpected, aActual, aMessage) {
  if (aExpected == aActual) {
    var msg = (aMessage != null) ? ( " : " +  aMessage ) : "";
    doThrow(aExpected + " != " + aActual + msg);
  }
}

function fail(aMessage) {
  var msg = (aMessage != null) ? ( "FAIL : " +  aMessage ) : "FAIL";
  doThrow(msg);
}

function testPending() {
  log("*** [" + _test_name + "] - test pending\n");
  if ( _tests_pending == 0 ) {
    // start of tests, don't spin wait
    _tests_pending++;
    return;
  }

  let count = _tests_pending++;
  while ( count < _tests_pending ) {
    // sleep for awhile. testFinished will decrement the _tests_pending
    sleep(100, true);
  }
}

function testFinished() {
  log("*** [" + _test_name + "] - test finished\n");
  if (--_tests_pending == 0)
    doQuit();
}

function log(s) {
  _consoleService.logStringMessage(s);
}

function getPlatform() {
  var sysInfo = Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2);
  return sysInfo.getProperty("name");
}

function sleep(ms, suppressOutput) {
  var threadManager = Cc["@mozilla.org/thread-manager;1"].
                      getService(Ci.nsIThreadManager);
  var mainThread = threadManager.mainThread;

  if (!suppressOutput) {
    log("*** [" + _test_name + "] - waiting for " + ms + " milliseconds...");
  }
  var then = new Date().getTime(), now = then;
  for (; now - then < ms; now = new Date().getTime()) {
    mainThread.processNextEvent(true);
  }
  if (!suppressOutput) {
    log("*** [" + _test_name + "] - done waiting.");
  }
}

function getTestServerPortNumber() {
  var DEFAULT_PORT = 8080;
  const ENV_VAR_PORT = "SONGBIRD_TEST_SERVER_PORT";

  var environment =
    Components.classes["@mozilla.org/process/environment;1"]
              .getService(Components.interfaces.nsIEnvironment);

  var port = DEFAULT_PORT;
  if (environment.exists(ENV_VAR_PORT)) {
    var envPort = parseInt(environment.get(ENV_VAR_PORT));
    if (envPort) {
      port = envPort;
    }
  }

  log("*** [" + _test_name + "] - using test server port " + port);

  return port;
}

function mockCore () {
  this._metadata = {
    artist: "Van Halen",
    album: "Women and Children First",
    title: "Everybody wants some!"
  }

  this._videoUrlMatcher = new ExtensionSchemeMatcher(this._videoUrlExtensions, []);
  this._mediaUrlMatcher = new ExtensionSchemeMatcher(this._mediaUrlExtensions,
                                                     this._mediaUrlSchemes);
}

mockCore.prototype = {
  _name: "mockCore",
  _id: "mockCore",
  _mute: false,
  _playing: false,
  _paused: false,
  _video: false,
  _volume: 128,
  _length: 25000,
  _position: 0,
  _metadata: null,
  _active: false,
  _mediaUrlExtensions: ["mp3", "ogg", "flac", "mpc", "wav", "m4a", "m4v",
                        "wmv", "asf", "avi",  "mov", "mpg", "mp4", "ogm",
                        "mp2", "mka", "mkv"],
  _mediaUrlSchemes: ["mms", "rstp"],
  _videoUrlExtensions: ["wmv", "asf", "avi", "mov", "mpg", "m4v", "mp4", "mp2",
                        "mpeg", "mkv", "ogm"],

  QueryInterface: function(iid) {
    if (!iid.equals(Ci.sbICoreWrapper) &&
        !iid.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  },
  getPaused: function() { return this._paused; },
  getPlaying: function() { return this._playing; },
  getPlayingVideo: function() { return this._video; },
  getMute: function() { return this._mute; },
  setMute: function(mute) { this._mute = mute; },
  getVolume: function() { return this._volume; },
  setVolume: function(vol) { this._volume = vol },
  getLength: function() { return this._length; },
  getPosition: function() { return this._position; },
  setPosition: function(pos) { this._position = pos; },
  goFullscreen: function() { },
  getId: function() { return this.id; },
  getObject: function() { return null; },
  setObject: function(ele) { },
  playURL: function(url) { return true; },
  play: function() { return true; },
  stop: function() { return true; },
  pause: function() { return true; },
  prev: function() { return true; },
  next: function() { return true; },
  getMetadata: function(key) { return this._metadata[key]; },
  isMediaURL: function(url) { return this._mediaUrlMatcher.match(url); },
  isVideoURL: function(url) { return this._videoUrlMatcher.match(url); },
  getSupportedFileExtensions: function() { return new StringArrayEnumerator(this._mediaUrlExtensions); },
  activate: function() { this._active = true; },
  deactivate: function() { this._active = false; },
  getActive: function(url) { return this._active; },
  getSupportForFileExtension: function(extension) {
    if (extension == ".mp3")
      return 1;
    else
      return -1;
  }
};

function initMockCore() {
  var pps = Cc["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
              .getService(Ci.sbIPlaylistPlayback);
  var myCore = new mockCore();
  pps.addCore(myCore, true);
}

function StringArrayEnumerator(aArray) {
  this._array = aArray;
  this._current = 0;
}

StringArrayEnumerator.prototype.hasMore = function() {
  return this._current < this._array.length;
}

StringArrayEnumerator.prototype.getNext = function() {
  return this._array[this._current++];
}

StringArrayEnumerator.prototype.QueryInterface = function(iid) {
  if (!iid.equals(Components.interfaces.nsIStringEnumerator) &&
      !iid.equals(Components.interfaces.nsISupports))
    throw Components.results.NS_ERROR_NO_INTERFACE;
  return this;
};

function ExtensionSchemeMatcher(aExtensions, aSchemes) {
  this._extensions = aExtensions;
  this._schemes    = aSchemes;
}

ExtensionSchemeMatcher.prototype.match = function(aStr) {

  // Short circuit for the most common case
  if(aStr.lastIndexOf("." + this._extensions[0]) ==
     aStr.length - this._extensions[0].length + 1) {
    return true;
  }

  var extensionSep = aStr.lastIndexOf(".");
  if(extensionSep >= 0) {
    var extension = aStr.substring(extensionSep + 1);
    var rv = this._extensions.some(function(ext) {
      return extension == ext;
    });
    if(rv) {
      return true;
    }
  }

  var schemeSep = aStr.indexOf("://");
  if(schemeSep >= 0) {
    var scheme = aStr.substring(0, schemeSep);
    var rv = this._schemes.some(function(sch) {
      return scheme == sch;
    });
    if(rv) {
      return true;
    }
  }

  return false;
}

_consoleService = Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService);
