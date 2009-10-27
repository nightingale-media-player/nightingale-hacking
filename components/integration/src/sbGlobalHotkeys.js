/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */
 
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const SB_DEFGLOBALHKSVC_CLASSNAME  = "sbDefaultGlobalHotkeys";
const SB_DEFGLOBALHKSVC_DESC       = "Songbird Default Global Hotkeys Service";
const SB_DEFGLOBALHKSVC_CONTRACTID = "@songbirdnest.com/Songbird/DefaultGlobalHotkeysService;1";
const SB_DEFGLOBALHKSVC_CID        = "{3020e071-7199-4d1b-9d92-55bfb4d34801}";

const SB_DEFGLOBALHKACTIONS_CLASSNAME  = "sbDefaultGlobalHotkeyActions";
const SB_DEFGLOBALHKACTIONS_DESC       = "Songbird Default Global Hotkey Actions";
const SB_DEFGLOBALHKACTIONS_CONTRACTID = "@songbirdnest.com/Songbird/DefaultGlobalHotkeyActions;1";
const SB_DEFGLOBALHKACTIONS_CID        = "{76512c09-7f02-48f5-86c0-c3a8670ead3f}";

const SB_HOTKEYSERVICE_CONTRACTID = "@songbirdnest.com/Songbird/GlobalHotkeys;1";
const SB_HOTKEYACTIONS_CONTRACTID = "@songbirdnest.com/Songbird/HotkeyActions;1";
const SB_COMMANDLINE_CONTRACTID   = "@songbirdnest.com/commandlinehandler/general-startup;1?type=songbird";

const STARTUP_TOPIC  = "final-ui-startup";
const SHUTDOWN_TOPIC = "quit-application";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");
Cu.import("resource://app/jsmodules/SBTimer.jsm");

/**
 * Global getter for the Global Hotkey Service
 */
__defineGetter__("HotkeyService", function() {
  delete HotkeyService;
  
  if (Cc[SB_HOTKEYSERVICE_CONTRACTID]) {
    HotkeyService = Cc[SB_HOTKEYSERVICE_CONTRACTID]
                    .getService(Ci.sbIGlobalHotkeys);
    return HotkeyService;
  }
  
  return null;  
});

/**
 * Global getter for Global Hotkey Actions Service
 */
__defineGetter__("HotkeyActions", function() {
  delete HotkeyActions;
  if (Cc[SB_HOTKEYACTIONS_CONTRACTID]) {
    HotkeyActions = Cc[SB_HOTKEYACTIONS_CONTRACTID]
                        .getService(Ci.sbIHotkeyActions);
    return HotkeyActions;
  }
  
  return null;
});

/**
 * Global getter for the Command Line Service
 */
 __defineGetter__("CommandLine", function() {
   delete CommandLine;
   
   if (Cc[SB_COMMANDLINE_CONTRACTID]) {
    CommandLine = Cc[SB_COMMANDLINE_CONTRACTID]
                    .getService(Ci.nsICommandLineHandler)
                    .QueryInterface(Ci.sbICommandLineManager);
    return CommandLine;
   }
   
   return null;
});

/**
 * Global getter for Observer Service
 */
__defineGetter__("ObserverService", function() {
  delete ObserverService;
  ObserverService = Cc["@mozilla.org/observer-service;1"]
                      .getService(Ci.nsIObserverService);
  return ObserverService;
});



function sbDefaultGlobalHotkeyService() {
  ObserverService.addObserver(this, STARTUP_TOPIC, false);
}

sbDefaultGlobalHotkeyService.prototype = 
{
  _dataRemoteHotkeysChanged: null,
  _dataRemoteHotkeysEnabled: null,
  _dataRemoteObserver: null,
  _platform: null,
  _metaKeyString: null,
  _hotkeyHandler: null,
  
  ///////////////////////////////////////////////////////////////////
  // nsIObserver
  ///////////////////////////////////////////////////////////////////
  observe: function(aSubject, aTopic, aData) {
    switch(aTopic) {
      case STARTUP_TOPIC: {
        ObserverService.removeObserver(this, STARTUP_TOPIC);
        this._init();
      }
      break;
      case SHUTDOWN_TOPIC: {
        ObserverService.removeObserver(this, SHUTDOWN_TOPIC);
        this._shutdown();
      }
      break;
    }
  },
  
  ///////////////////////////////////////////////////////////////////
  // Internal Methods
  ///////////////////////////////////////////////////////////////////
  _init: function() {
    if(!HotkeyService) {
      Cu.reportError("Global Hotkey Service is not available.");
      return;
    }
    
    this._initPlatform();
    this._initMetaKeyString();
    this._initHotkeyHandler();
        
    this._startWatchingHotkeyDataRemotes();
    this._loadHotkeysFromPrefs();
    
    if(!HotkeyActions) {
      Cu.reportError("Global Hotkey Actions Service is not available.");
      return;
    }
    
    this._initHotkeyHandler();
    
    // Register the default set of actions.
    var defaultActions = Cc[SB_DEFGLOBALHKACTIONS_CONTRACTID]
                           .createInstance(Ci.sbIHotkeyActionBundle);
    HotkeyActions.registerHotkeyActionBundle(defaultActions);
    
    // Register observer for application shutdown to clean up.
    ObserverService.addObserver(this, SHUTDOWN_TOPIC, false);
  },
  
  _shutdown: function() {
    if (CommandLine) {
      CommandLine.removeFlagHandler(this._hotkeyHandler, "hotkey");
    }
    
    this._hotkeyHandler = null;
    
    if (HotkeyService) {
      HotkeyService.removeAllHotkeys();
    }
    
    this._stopWatchingHotkeyDataRemotes();
  },
  
  _initPlatform: function() {
    try {
      var sysInfo =
        Components.classes["@mozilla.org/system-info;1"]
                  .getService(Components.interfaces.nsIPropertyBag2);
      this._platform = sysInfo.getProperty("name");                                          
    }
    catch (e) {
      Cu.reportError(e);
      
      var user_agent = navigator.userAgent;
      if (user_agent.indexOf("Windows") != -1)
        this._platform = "Windows_NT";
      else if (user_agent.indexOf("Mac OS X") != -1)
        this._platform = "Darwin";
      else if (user_agent.indexOf("Linux") != -1)
        this._platform = "Linux";
      else if (user_agent.indexOf("SunOS") != -1)
        this._platform = "SunOS";
    }
  },
  
  _initMetaKeyString: function() {
    switch(this._platform) {
      case "Windows_NT": this._metaKeyString = "win"; break;
      case "Darwin": this._metaKeyString = "command"; break;
      case "Linux": this._metaKeyString = "meta"; break;
      default: this._metaKeyString = "meta";
    }
  },
  
  _initHotkeyHandler: function() {
    if (!CommandLine) {
      Cu.reportError("Command Line Handler is not available.");
      return;
    }

    if (!this._hotkeyHandler) {
      /**
       * The default hotkey handler
       */
      this._hotkeyHandler = {
        onHotkey: function (aHotkeyId) {
          aHotkeyId = aHotkeyId.toLowerCase();
          // look through the action bundles to find the right action to trigger
          for (var i = 0;i < HotkeyActions.bundleCount; i++)
          {
            var bundle = HotkeyActions.enumBundle(i);
            for (var j=0;j<bundle.actionCount;j++)
            {
              var id = bundle.enumActionID(j);
              if (id.toLowerCase() == aHotkeyId) {
                bundle.onAction(j);
                return;
              }
            }
          }
        },
        
        handleFlag: function(aFlag, aParam)
        {
          var ids = aParam.split(",");
          for (var i = 0; i < ids.length; i++) {
            this.onHotkey(ids[i]); 
          }
          
          return true;
        },

        QueryInterface : XPCOMUtils.generateQI([Ci.sbIGlobalHotkeyCallback,
                                                Ci.sbICommandLineFlagHandler,
                                                Ci.nsISupportsWeakReference,
                                                Ci.nsISupports])
      };
    }

    CommandLine.addFlagHandler(this._hotkeyHandler, "hotkey");
  },
  
  /**
   * \brief Load the hotkeys saved in the users prefs
   */
  _loadHotkeysFromPrefs: function() {
    this._loadDefaultHotkeys();
    
    var enabled = SBDataGetBoolValue("globalhotkeys.enabled");
    
    if (!enabled) {
      return;
    }
    
    var count = SBDataGetIntValue("globalhotkeys.count");
    for (var i = 0;i < count; i++) {
      // Read hotkey binding from user preferences
      let root = "globalhotkey." + i + ".";
      let keycombo = SBDataGetStringValue(root + "key");
      let action = SBDataGetStringValue(root + "action");
      
      // Split key combination string
      let keys = keycombo.split("-");
      
      // Parse its components
      let alt = false;
      let ctrl = false;
      let shift = false;
      let meta = false;
      let keyCode = 0;
      
      for (var j = 0; j < keys.length; j++) {
        keys[j] = keys[j].toLowerCase();
        switch (keys[j]) {
          case "alt": alt = true; break;
          case "shift": shift = true; break;
          case "ctrl": ctrl = true; break;
          case "meta": meta = true; break;
          default: keyCode = this._stringToKeyCode(keys[j]);
        }
      }
      
      // If we had a key code (and possibly modifiers), register the corresponding action for it
      if (keyCode != 0) {
        HotkeyService.addHotkey(keyCode, 
                                alt, 
                                ctrl, 
                                shift, 
                                meta, 
                                action, 
                                this._hotkeyHandler);
      }
    }

  },
  
  /**
   * \brief Load the default set of hotkeys
   */
  _loadDefaultHotkeys: function() {
    // Global Hotkeys have already changed, don't load the default set.
    if (SBDataGetBoolValue("globalhotkeys.changed")) {
      return;
    }
    
    // Indicate that the global hotkeys have changed and are enabled
    SBDataSetBoolValue("globalhotkeys.changed", true);
    SBDataSetBoolValue("globalhotkeys.enabled", true);

    // 10 hotkeys by default
    SBDataSetIntValue("globalhotkeys.count", 10);

    // For convenience
    var metaKey = this._metaKeyString;
    
    // Media keys  
    SBDataSetStringValue("globalhotkey.0.key",             "$176");
    SBDataSetStringValue("globalhotkey.0.key.readable",    "nexttrack");
    SBDataSetStringValue("globalhotkey.0.action",          "playback.nexttrack");
    
    SBDataSetStringValue("globalhotkey.1.key",             "$177");
    SBDataSetStringValue("globalhotkey.1.key.readable",    "prevtrack");
    SBDataSetStringValue("globalhotkey.1.action",          "playback.previoustrack");

    SBDataSetStringValue("globalhotkey.2.key",             "$179");
    SBDataSetStringValue("globalhotkey.2.key.readable",    "playpause");
    SBDataSetStringValue("globalhotkey.2.action",          "playback.playpause");
    
    // Regular keys
    SBDataSetStringValue("globalhotkey.3.key",             "meta-$38");
    SBDataSetStringValue("globalhotkey.3.key.readable",    metaKey + "-up");
    SBDataSetStringValue("globalhotkey.3.action",          "playback.volumeup");
    
    SBDataSetStringValue("globalhotkey.4.key",             "meta-$40");
    SBDataSetStringValue("globalhotkey.4.key.readable",    metaKey + "-down");
    SBDataSetStringValue("globalhotkey.4.action",          "playback.volumedown");
    
    SBDataSetStringValue("globalhotkey.5.key",             "meta-$39");
    SBDataSetStringValue("globalhotkey.5.key.readable",    metaKey + "-right");
    SBDataSetStringValue("globalhotkey.5.action",          "playback.nexttrack");
    
    SBDataSetStringValue("globalhotkey.6.key",             "meta-$37");
    SBDataSetStringValue("globalhotkey.6.key.readable",    metaKey + "-left");
    SBDataSetStringValue("globalhotkey.6.action",          "playback.previoustrack");
    
    SBDataSetStringValue("globalhotkey.7.key",             "meta-$96");
    SBDataSetStringValue("globalhotkey.7.key.readable",    metaKey + "-numpad0");
    SBDataSetStringValue("globalhotkey.7.action",          "playback.playpause");

    SBDataSetStringValue("globalhotkey.8.key",             "$178");
    SBDataSetStringValue("globalhotkey.8.key.readable",    "stop");
    SBDataSetStringValue("globalhotkey.8.action",          "playback.stop");

    SBDataSetStringValue("globalhotkey.9.key",            "meta-$74");
    SBDataSetStringValue("globalhotkey.9.key.readable",   metaKey + "-J");
    SBDataSetStringValue("globalhotkey.9.action",         "jumpto.open");
  },
  
  /**
   * \brief Remove all hotkeys currently bound
   */
  _removeHotkeyBindings: function() {
    if (HotkeyService) {
      HotkeyService.removeAllHotkeys();
    }
  },
  
  _startWatchingHotkeyDataRemotes: function() {
    if (!this._dataRemoteObserver) {
      var self = this;
      this._dataRemoteObserver = {
        observe: function(aSubject, aTopic, aData) {
          self._removeHotkeyBindings();
          self._loadHotkeysFromPrefs();
        }
      };
    }
    
    this._dataRemoteHotkeysChanged = 
      SB_NewDataRemote("globalhotkeys.changed", null);
    this._dataRemoteHotkeysChanged.bindObserver(this._dataRemoteObserver, true);

    this._dataRemoteHotkeysEnabled = 
      SB_NewDataRemote("globalhotkeys.enabled", null);
    this._dataRemoteHotkeysEnabled.bindObserver(this._dataRemoteObserver, true);
  },
  
  _stopWatchingHotkeyDataRemotes: function() {
    if (this._dataRemoteHotkeysChanged) {
      this._dataRemoteHotkeysChanged.unbind();
      this._dataRemoteHotkeysChanged = null;
    }
    
    if (this._dataRemoteHotkeysEnabled) {
      this._dataRemoteHotkeysEnabled.unbind();
      this._dataRemoteHotkeysEnabled = null;
    }
  },
  
  _stringToKeyCode: function(aStr) {
    if (aStr.slice(0, 1) == '$') {
      return aStr.slice(1);
    }
    
    return 0;
  },
  
  className: SB_DEFGLOBALHKSVC_CLASSNAME,
  classDescription: SB_DEFGLOBALHKSVC_DESC,
  classID: Components.ID(SB_DEFGLOBALHKSVC_CID),
  contractID: SB_DEFGLOBALHKSVC_CONTRACTID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports])
};



function sbDefaultGlobalHotkeyActions() {
}

sbDefaultGlobalHotkeyActions.prototype = 
{
  /**
   * You should change these to match your own actions, strings, and 
   * stringbundle (ie, you will need to ship your own translations with 
   * your extension since the description string you use will probably 
   * not be in the standard songbird string bundle)
   */
  _packagename: "playback",
  
  _actions: [ "volumeup", 
              "volumedown", 
              "nexttrack", 
              "previoustrack", 
              "playpause", 
              "pause", 
              "stop" ],
  /**
   * The string bundle to use to get the localized strings 
   * (ie, hotkeys.actions.playback, hotkeys.actions.playback.volumeup, 
   * hotkeys.actions.playback.volumedown, etc).
   */
  _stringbundle: "chrome://songbird/locale/songbird.properties", 

  /**
   * Internal data
   */
  _localpackagename: null,
  _sbs: null,
  _songbirdStrings: null,

  /**
   * This enumerates the actions, their localized display strings, 
   * and their internal ids, so that the hotkey action manager may list them
   * in the hotkeys preference pane.
   */
  get actionCount() {
    return this._actions.length;
  },
  
  enumActionLocaleDescription: function (aIndex) { 
    return this._getLocalizedPackageName() + 
           ": " + 
           this._getLocalizedActionName(this._actions[aIndex]); 
  },
  
  enumActionID: function(aIndex) { 
    return this._packagename + "." + this._actions[aIndex]; 
  },

  /**
   * This is called when an action has been triggered
   */
  onAction: function(aIndex) {
    switch (aIndex) {
      case 0: this._hotkey_volumeUp(); break;
      case 1: this._hotkey_volumeDown(); break;
      case 2: this._hotkey_nextTrack(); break;
      case 3: this._hotkey_previousTrack(); break;
      case 4: this._hotkey_playPause(); break;
      case 5: this._hotkey_pause(); break;
      case 6: this._hotkey_stop(); break;
    }
  },

  get _mm() {
    return Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
             .getService(Ci.sbIMediacoreManager);
  },
  
  _hotkey_volumeUp: function() {
    var volume = this._mm.volumeControl.volume + 0.03;
    if (volume > 1) volume = 1;
    this._mm.volumeControl.volume = volume;
  },

  _hotkey_volumeDown: function() {
    var volume = this._mm.volumeControl.volume - 0.03;
    if (volume < 0) volume = 0;
    this._mm.volumeControl.volume = volume;
  },

  _hotkey_nextTrack: function() {
    this._mm.sequencer.next();
  },

  _hotkey_previousTrack: function() {
    this._mm.sequencer.previous();
  },

  _hotkey_playPause: function() {
    try {
      // If we are already playing something just pause/unpause playback
      if (this._mm.status.state == Ci.sbIMediacoreStatus.STATUS_PLAYING || 
          this._mm.status.state == Ci.sbIMediacoreStatus.STATUS_BUFFERING) {
          this._mm.playbackControl.pause();
      } 
      else if(this._mm.status.state == Ci.sbIMediacoreStatus.STATUS_PAUSED) {
          this._mm.playbackControl.play();
      }
      // Otherwise just have the root application controller figure it out
      else {
        // If we have no context, initiate playback
        // via the root application controller
        var app = Cc["@songbirdnest.com/Songbird/ApplicationController;1"]
                    .getService(Ci.sbIApplicationController);
        app.playDefault();
      } 
    } catch (e) {
      Cu.reportError(e);
    }
  },

  _hotkey_pause: function() {
    this._mm.playbackControl.pause();
  },

  _hotkey_stop: function() {
    this._mm.playbackControl.stop();
  },

  _getLocalizedString: function(str, defaultstr) {
    var r = defaultstr;
    if (!this._sbs) {
      this._sbs = Cc["@mozilla.org/intl/stringbundle;1"]
                    .getService(Ci.nsIStringBundleService);
      this._songbirdStrings = this._sbs.createBundle(this._stringbundle);
    }
    try {
      r = this._songbirdStrings.GetStringFromName(str);
    } catch (e) { /* we have a default */ }

    return r;
  },
  
  /**
   * The local package name is taken from the specified string bundle, 
   * using the key "hotkeys.actions.package", where package is
   * the internal name of the package, as specified in this._packagename.
   */
  _getLocalizedPackageName: function() {
    if (!this._localpackage) 
      this._localpackage = this._getLocalizedString("hotkeys.actions." + 
                                                    this._packagename, 
                                                    this._packagename); 
    return this._localpackage;
  },
  
  /**
   * the local action name is taken from the specified string bundle, using 
   * the key "hotkeys.actions.package.action", where package
   * is the internal name of the package, as specified in this._packagename, 
   * and action is the internal name of the action, as specified in the 
   * this._actions array.
   */
  _getLocalizedActionName: function(action) {
    return this._getLocalizedString("hotkeys.actions." + 
                                    this._packagename + 
                                    "." + 
                                    action, 
                                    action);
  },

  className: SB_DEFGLOBALHKACTIONS_CLASSNAME,
  classDescription: SB_DEFGLOBALHKACTIONS_DESC,
  classID: Components.ID(SB_DEFGLOBALHKACTIONS_CID),
  contractID: SB_DEFGLOBALHKACTIONS_CONTRACTID,
  QueryInterface: XPCOMUtils.generateQI([Ci.sbIHotkeyActionBundle,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsISupports])
};

//------------------------------------------------------------------------------
// XPCOM Registration

function NSGetModule(compMgr, fileSpec)
{
  return XPCOMUtils.generateModule([sbDefaultGlobalHotkeyService, 
                                    sbDefaultGlobalHotkeyActions],
    function(aCompMgr, aFileSpec, aLocation) {
      XPCOMUtils.categoryManager.addCategoryEntry("app-startup",
                                                  SB_DEFGLOBALHKSVC_DESC,
                                                  "service," +
                                                  SB_DEFGLOBALHKSVC_CONTRACTID,
                                                  true,
                                                  true);
    }
  );
}
