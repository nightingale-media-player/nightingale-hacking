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
 
 // This needs to become an api so plugins can register new actions and callbacks for them

var hotkey_service;
var hotkeyactions_service;
var cmdline_mgr;

var platform;
try {
  var sysInfo =
    Components.classes["@mozilla.org/system-info;1"]
              .getService(Components.interfaces.nsIPropertyBag2);
  platform = sysInfo.getProperty("name");                                          
}
catch (e) {
  dump("System-info not available, trying the user agent string.\n");
  var user_agent = navigator.userAgent;
  if (user_agent.indexOf("Windows") != -1)
    platform = "Windows_NT";
  else if (user_agent.indexOf("Mac OS X") != -1)
    platform = "Darwin";
  else if (user_agent.indexOf("Linux") != -1)
    platform = "Linux";
}

var meta_key_str;
if (platform == "Windows_NT")
  meta_key_str = "win";
else if (platform == "Darwin")
  meta_key_str = "command";
else if (platform == "Linux")
  meta_key_str = "meta";
else
  meta_key_str = "meta";

function initGlobalHotkeys() {
  // Access global hotkeys component (if it exists)
  var globalHotkeys = Components.classes["@songbirdnest.com/Songbird/GlobalHotkeys;1"];
  if (globalHotkeys) {
    hotkey_service = globalHotkeys.getService(Components.interfaces.sbIGlobalHotkeys);
    loadHotkeysFromPrefs()
  }
  var hotkeyActions = Components.classes["@songbirdnest.com/Songbird/HotkeyActions;1"];
  if (hotkeyActions) {
    hotkeyactions_service = hotkeyActions.getService(Components.interfaces.sbIHotkeyActions);
  }

  var cmdline = Components.classes["@songbirdnest.com/commandlinehandler/general-startup;1?type=songbird"];
  if (cmdline) {
    var cmdline_service = cmdline.getService(Components.interfaces.nsICommandLineHandler);
    if (cmdline_service) {
      cmdline_mgr = cmdline_service.QueryInterface(Components.interfaces.sbICommandLineManager);
      if (cmdline_mgr) {
        cmdline_mgr.addFlagHandler(hotkeyHandler, "hotkey");
      }
    }
  }
}

function resetGlobalHotkeys() {
  if (cmdline_mgr) cmdline_mgr.removeFlagHandler(hotkeyHandler, "hotkey");
  removeHotkeyBindings();
  stopWatchingHotkeyRemotes();
}

function removeHotkeyBindings() {
  // Remove all hotkey bindings
  if (hotkey_service) hotkey_service.removeAllHotkeys();
}

// Hotkey handler, this gets called back when the user presses a hotkey that has been registered in loadHotkeysFromPrefs
var hotkeyHandler = 
{
    onHotkey: function( hotkeyid )
    {
      hotkeyid = hotkeyid.toLowerCase();
      // look through the action bundles to find the right action to trigger
      for (var i=0;i<hotkeyactions_service.bundleCount;i++)
      {
        var bundle = hotkeyactions_service.enumBundle(i);
        for (var j=0;j<bundle.actionCount;j++)
        {
          var id = bundle.enumActionID(j);
          if (id.toLowerCase() == hotkeyid) {
            bundle.onAction(j);
            return;
          }
        }
      }
      //alert("Unknown hotkey action '" + hotkeyid + "'"); 
    },
    
    handleFlag: function(aFlag, aParam)
    {
      var ids = aParam.split(",");
      for (var i=0;i<ids.length;i++) {
        setTimeout(function(id) { hotkeyHandler.onHotkey(id); }, i*10, ids[i]);
      }
      return true;
    },

    QueryInterface : function(aIID)
    {
      if (!aIID.equals(Components.interfaces.sbIGlobalHotkeyCallback) &&
          !aIID.equals(Components.interfaces.sbICommandLineFlagHandler) &&
          !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
          !aIID.equals(Components.interfaces.nsISupports)) 
      {
        throw Components.results.NS_ERROR_NO_INTERFACE;
      }
      
      return this;
    }
}

// Load hotkeys
function loadHotkeysFromPrefs() {
  if (!hotkey_service) return;
  if (!songbird_hotkeys_event1) beginWatchingHotkeyRemotes();
  setDefaultGlobalHotkeys();
  var enabled = SBDataGetBoolValue("globalhotkeys.enabled");
  //log("(re)init - hotkeys are " + (enabled ? "enabled" : "disabled"));
  if (!enabled) return;
  var count = SBDataGetIntValue("globalhotkeys.count");
  for (var i=0;i<count;i++) {
    // Read hotkey binding from user preferences
    var root = "globalhotkey." + i + ".";
    var keycombo = SBDataGetStringValue(root + "key");
    var action = SBDataGetStringValue(root + "action");
    // Split key combination string
    var keys = keycombo.split("-");
    // Parse its components
    var alt = false;
    var ctrl = false;
    var shift = false;
    var meta = false;
    var keyCode = 0;
    for (var j=0;j<keys.length;j++) {
      keys[j] = keys[j].toLowerCase();
      if (keys[j] == "alt") alt = true;
      else if (keys[j] == "shift") shift = true;
      else if (keys[j] == "ctrl") ctrl = true;
      else if (keys[j] == "meta") meta = true;
      else keyCode = stringToKeyCode(keys[j]);
    }
    // If we had a key code (and possibly modifiers), register the corresponding action for it
    if (keyCode != 0) hotkey_service.addHotkey(keyCode, alt, ctrl, shift, meta, action, hotkeyHandler);
  }
}

// Sets the default hotkey settings
function setDefaultGlobalHotkeys() {
  if (!SBDataGetBoolValue("globalhotkeys.changed")) {
    SBDataSetBoolValue("globalhotkeys.changed", true);
    SBDataSetBoolValue("globalhotkeys.enabled", true);

    SBDataSetIntValue("globalhotkeys.count", 12);

    // media keyboard keys :

    SBDataSetStringValue("globalhotkey.0.key",             "$175");
    SBDataSetStringValue("globalhotkey.0.key.readable",    "volumeup");
    SBDataSetStringValue("globalhotkey.0.action",          "playback.volumeup");
    
    SBDataSetStringValue("globalhotkey.1.key",             "$174");
    SBDataSetStringValue("globalhotkey.1.key.readable",    "volumedown");
    SBDataSetStringValue("globalhotkey.1.action",          "playback.volumedown");
    
    SBDataSetStringValue("globalhotkey.2.key",             "$176");
    SBDataSetStringValue("globalhotkey.2.key.readable",    "nexttrack");
    SBDataSetStringValue("globalhotkey.2.action",          "playback.nexttrack");
    
    SBDataSetStringValue("globalhotkey.3.key",             "$177");
    SBDataSetStringValue("globalhotkey.3.key.readable",    "prevtrack");
    SBDataSetStringValue("globalhotkey.3.action",          "playback.previoustrack");

    SBDataSetStringValue("globalhotkey.4.key",             "$179");
    SBDataSetStringValue("globalhotkey.4.key.readable",    "playpause");
    SBDataSetStringValue("globalhotkey.4.action",          "playback.playpause");
    
    // non media keyboard keys :
    
    SBDataSetStringValue("globalhotkey.5.key",             "meta-$38");
    SBDataSetStringValue("globalhotkey.5.key.readable",    meta_key_str + "-up");
    SBDataSetStringValue("globalhotkey.5.action",          "playback.volumeup");
    
    SBDataSetStringValue("globalhotkey.6.key",             "meta-$40");
    SBDataSetStringValue("globalhotkey.6.key.readable",    meta_key_str + "-down");
    SBDataSetStringValue("globalhotkey.6.action",          "playback.volumedown");
    
    SBDataSetStringValue("globalhotkey.7.key",             "meta-$39");
    SBDataSetStringValue("globalhotkey.7.key.readable",    meta_key_str + "-right");
    SBDataSetStringValue("globalhotkey.7.action",          "playback.nexttrack");
    
    SBDataSetStringValue("globalhotkey.8.key",             "meta-$37");
    SBDataSetStringValue("globalhotkey.8.key.readable",    meta_key_str + "-left");
    SBDataSetStringValue("globalhotkey.8.action",          "playback.previoustrack");
    
    SBDataSetStringValue("globalhotkey.9.key",             "meta-$96");
    SBDataSetStringValue("globalhotkey.9.key.readable",    meta_key_str + "-numpad0");
    SBDataSetStringValue("globalhotkey.9.action",          "playback.playpause");

    SBDataSetStringValue("globalhotkey.10.key",             "$178");
    SBDataSetStringValue("globalhotkey.10.key.readable",    "stop");
    SBDataSetStringValue("globalhotkey.10.action",          "playback.stop");

    SBDataSetStringValue("globalhotkey.11.key",            "meta-$74");
    SBDataSetStringValue("globalhotkey.11.key.readable",   meta_key_str + "-J");
    SBDataSetStringValue("globalhotkey.11.action",         "jumpto.open");
  }
}

// Returns the key code for a given key string
function stringToKeyCode( str ) {
  if (str.slice(0, 1) == '$') return str.slice(1);
  return 0;
}

var songbird_hotkeys_event1;
var songbird_hotkeys_event2;

const global_hotkeys_changed = {
  observe : function ( aSubject, aTopic, aData ) { globalHotkeysChanged(); }
}

function beginWatchingHotkeyRemotes() {
  songbird_hotkeys_event1 = SB_NewDataRemote ("globalhotkeys.changed", null);
  songbird_hotkeys_event1.bindObserver( global_hotkeys_changed, true )
  songbird_hotkeys_event2 = SB_NewDataRemote("globalhotkeys.enabled", null);
  songbird_hotkeys_event2.bindObserver( global_hotkeys_changed, true )
}

function stopWatchingHotkeyRemotes() {
  if (songbird_hotkeys_event1) {
    songbird_hotkeys_event1.unbind();
    songbird_hotkeys_event1 = null;
  }
  if (songbird_hotkeys_event2) {
    songbird_hotkeys_event2.unbind();
    songbird_hotkeys_event2 = null;
  }
}

function globalHotkeysChanged(v) {
  removeHotkeyBindings();
  loadHotkeysFromPrefs();
}


/*
function log(str) {
  var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                          .getService(Components.interfaces.nsIConsoleService);
  consoleService.logStringMessage(str);
}
*/

// -------------------------------------------------------------------------------------------------------------------------------------------------------------
// Playback Hotkey Actions
// -------------------------------------------------------------------------------------------------------------------------------------------------------------

// A Few internal hotkey actions. The same method can be used by extensions or other parts of the app to register their own hotkey actions

var playbackHotkeyActions = {
 
  // you should change these to match your own actions, strings, and stringbundle (ie, you will need to ship your own translations with your extension
  // since the description string you use will probably not be in the standard songbird string bundle)
  _packagename: "playback",
  _actions: [ "volumeup", "volumedown", "nexttrack", "previoustrack", "playpause", "pause", "stop" ],
  // the string bundle to use to get the localized strings (ie, hotkeys.actions.playback, hotkeys.actions.playback.volumeup, hotkeys.actions.playback.volumedown, etc)
  _stringbundle: "chrome://songbird/locale/songbird.properties", 

  // this enumerates the actions, their localized display strings, and their internal ids, so that the hotkey action manager may list them 
  // in the htokeys preference pane
  get_actionCount: function() { return this._actions.length; },
  enumActionLocaleDescription: function (idx) { return this._getLocalizedPackageName() + ": " + this._getLocalizedActionName(this._actions[idx]); },
  enumActionID: function(idx) { return this._packagename + "." + this._actions[idx]; },

  // this is called when an action has been triggered
  onAction: function(idx) {
    switch (idx) {
      case 0: this._hotkey_volumeUp(); break;
      case 1: this._hotkey_volumeDown(); break;
      case 2: this._hotkey_nextTrack(); break;
      case 3: this._hotkey_previousTrack(); break;
      case 4: this._hotkey_playPause(); break;
      case 5: this._hotkey_pause(); break;
      case 6: this._hotkey_stop(); break;
    }
  },
  
  // -------------------------
  // actions implementation

  _gPPS: Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback),
  
  _hotkey_volumeUp: function() {
    var volume = this._gPPS.volume + 8;
    if (volume > 255) volume = 255;
    this._gPPS.volume = volume;
  },

  _hotkey_volumeDown: function() {
    var volume = this._gPPS.volume - 8;
    if (volume < 0) volume = 0;
    this._gPPS.volume = volume;
  },

  _hotkey_nextTrack: function() {
    this._gPPS.next();
  },

  _hotkey_previousTrack: function() {
    this._gPPS.previous();
  },

  _hotkey_playPause: function() {
    // If paused or stopped, then start playing
    if (this._gPPS.paused == this._gPPS.playing) {
      this._gPPS.play(); 
    // Otherwise pause
    } else {
      this._gPPS.pause();  
    }
  },

  _hotkey_pause: function() {
    this._gPPS.pause();
  },

  _hotkey_stop: function() {
    this._gPPS.stop();
  },

  // -------------------------
  // convenience functions

  _localpackagename: null,
  _sbs: null,
  _songbirdStrings: null,
  
  _getLocalizedString: function(str, defaultstr) {
    var r = defaultstr;
    if (!this._sbs) {
      this._sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
      this._songbirdStrings = this._sbs.createBundle(this._stringbundle);
    }
    try {
      r = this._songbirdStrings.GetStringFromName(str);
    } catch (err) { /* we have default */ }
    return r;
  },
  
  // the local package name is taken from the specified string bundle, using the key "hotkeys.actions.package", where package is
  // the internal name of the package, as specified in this._packagename
  
  _getLocalizedPackageName: function() {
    if (!this._localpackage) 
      this._localpackage = this._getLocalizedString("hotkeys.actions." + this._packagename, this._packagename); 
    return this._localpackage;
  },
  
  // the local action name is taken from the specified string bundle, using the key "hotkeys.actions.package.action", where package
  // is the internal name of the package, as specified in this._packagename, and action is the internal name of the action, as 
  // specified in the this._actions array.
  
  _getLocalizedActionName: function(action) {
    return this._getLocalizedString("hotkeys.actions." + this._packagename + "." + action, action);
  },

  // -------------------------
  // interface code

  QueryInterface : function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbIHotkeyActionBundle) &&
        !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    
    return this;
  }
};

playbackHotkeyActions.__defineGetter__( "actionCount", playbackHotkeyActions.get_actionCount);

// register the above object as a hotkey action bundle (sbIHotkeyActionBundle)

// not using the global hotkeyactions_service var here since it hasn't been initialized yet
var hotkeyActionsComponent = Components.classes["@songbirdnest.com/Songbird/HotkeyActions;1"];
if (hotkeyActionsComponent) {
  var hotkeyactionsService = hotkeyActionsComponent.getService(Components.interfaces.sbIHotkeyActions);
  if (hotkeyactionsService) hotkeyactionsService.registerHotkeyActionBundle(playbackHotkeyActions);
}


