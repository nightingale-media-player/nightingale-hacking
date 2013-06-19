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

const SB_HOTKEY_SERVICE_CLASSNAME  = "sbHotkeyService"
const SB_HOTKEY_SERVICE_DESC       = "Songbird Global Hotkey Service";
const SB_HOTKEY_SERVICE_CONTRACTID = "@songbirdnest.com/Songbird/HotkeyService;1";
const SB_HOTKEY_SERVICE_CID        = "{8264dc94-1dd2-11b2-ab4a-8b3dfb1072fb}";

const SB_DEFGLOBALHKACTIONS_CLASSNAME  = "sbDefaultGlobalHotkeyActions";
const SB_DEFGLOBALHKACTIONS_DESC       = "Songbird Default Global Hotkey Actions";
const SB_DEFGLOBALHKACTIONS_CONTRACTID = "@songbirdnest.com/Songbird/DefaultGlobalHotkeyActions;1";
const SB_DEFGLOBALHKACTIONS_CID        = "{76512c09-7f02-48f5-86c0-c3a8670ead3f}";

const SB_HOTKEY_CONFIGURATION_CLASSNAME  = "sbHotkeyConfiguration";
const SB_HOTKEY_CONFIGURATION_DESC       = "Songbird Hotkey Configuration";
const SB_HOTKEY_CONFIGURATION_CONTRACTID = "@songbirdnest.com/Songbird/HotkeyConfiguration;1";
const SB_HOTKEY_CONFIGURATION_CID        = "{5c6b204c-1dd2-11b2-8005-83725e03a0d0}";

const SB_HOTKEYMANAGER_CONTRACTID = "@songbirdnest.com/Songbird/GlobalHotkeys;1";
const SB_HOTKEYACTIONS_CONTRACTID = "@songbirdnest.com/Songbird/HotkeyActions;1";
const SB_COMMANDLINE_CONTRACTID   = "@songbirdnest.com/commandlinehandler/general-startup;1?type=songbird";

const STARTUP_TOPIC  = "final-ui-startup";
const SHUTDOWN_TOPIC = "quit-application";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");
Cu.import("resource://app/jsmodules/SBTimer.jsm");

/**
 * Global getter for the Global Hotkey Manager
 */
__defineGetter__("HotkeyManager", function() {
  delete HotkeyManager;

  if (SB_HOTKEYMANAGER_CONTRACTID in Cc) {
    HotkeyManager = Cc[SB_HOTKEYMANAGER_CONTRACTID]
                      .getService(Ci.sbIGlobalHotkeys);
  } else {
    HotkeyManager = null;
  }

  return HotkeyManager;
});

/**
 * Global getter for Global Hotkey Actions Service
 */
__defineGetter__("HotkeyActions", function() {
  delete HotkeyActions;

  if (SB_HOTKEYACTIONS_CONTRACTID in Cc) {
    HotkeyActions = Cc[SB_HOTKEYACTIONS_CONTRACTID]
                      .getService(Ci.sbIHotkeyActions);
  } else {
    HotkeyActions = null;
  }

  return HotkeyActions;
});

/**
 * Global getter for the Command Line Service
 */
 __defineGetter__("CommandLine", function() {
   delete CommandLine;

   if (SB_COMMANDLINE_CONTRACTID in Cc) {
    CommandLine = Cc[SB_COMMANDLINE_CONTRACTID]
                    .getService(Ci.nsICommandLineHandler)
                    .QueryInterface(Ci.sbICommandLineManager);
   } else {
    CommandLine = null;
   }

   return CommandLine;
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



function sbHotkeyService() {
  ObserverService.addObserver(this, STARTUP_TOPIC, false);
}

sbHotkeyService.prototype =
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
  // sbIHotkeyService
  ///////////////////////////////////////////////////////////////////
  getHotkeys: function() {
    var count = SBDataGetIntValue("globalhotkeys.count");
    var hotkeyConfigList = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                             .createInstance(Ci.nsIMutableArray);
    for (var i = 0; i < count; i++) {
      var hotkeyConfig = Cc[SB_HOTKEY_CONFIGURATION_CONTRACTID]
                           .createInstance(Ci.sbIHotkeyConfiguration);

      var keyBase = "globalhotkey." + i + ".";
      hotkeyConfig.key = SBDataGetStringValue(keyBase + "key");
      hotkeyConfig.keyReadable = SBDataGetStringValue(keyBase + "key.readable");
      hotkeyConfig.action = SBDataGetStringValue(keyBase + "action");

      if (!this._configListContainsKey(hotkeyConfigList, hotkeyConfig.key))
        hotkeyConfigList.appendElement(hotkeyConfig, false);
    }

    return hotkeyConfigList;
  },

  getHotkey: function(aKey) {
    var hotkeyConfigList = this.getHotkeys();
    for (var i = 0; i < hotkeyConfigList.length; i++) {
      var hotkeyConfig =
            hotkeyConfigList.queryElementAt(i, Ci.sbIHotkeyConfiguration);
      if (hotkeyConfig.key == aKey)
        return hotkeyConfig;
    }

    return null;
  },

  setHotkeys: function(aHotkeyConfigList) {
    SBDataDeleteBranch("globalhotkey");
    for (var i = 0; i < aHotkeyConfigList.length; i++) {
      var hotkeyConfig =
            aHotkeyConfigList.queryElementAt(i, Ci.sbIHotkeyConfiguration);

      var keyBase = "globalhotkey." + i + ".";
      SBDataSetStringValue(keyBase + "key", hotkeyConfig.key);
      SBDataSetStringValue(keyBase + "key.readable", hotkeyConfig.keyReadable);
      SBDataSetStringValue(keyBase + "action", hotkeyConfig.action);
    }
    SBDataSetIntValue("globalhotkeys.count", aHotkeyConfigList.length);
    this._loadHotkeysFromPrefs();
  },

  addHotkey: function(aHotkeyConfig) {
    var hotkeyConfigList = this.getHotkeys();
    this._removeHotkeyFromConfigList(hotkeyConfigList, aHotkeyConfig.key);
    hotkeyConfigList.appendElement(aHotkeyConfig, false);
    this.setHotkeys(hotkeyConfigList);
  },

  removeHotkeyByKey: function(aKey) {
    var hotkeyConfigList = this.getHotkeys();
    this._removeHotkeyFromConfigList(hotkeyConfigList, aKey);
    this.setHotkeys(hotkeyConfigList);
  },

  removeHotkeysByAction: function(aAction) {
    var hotkeyConfigList = this.getHotkeys();
    for (var i = hotkeyConfigList.length - 1; i >= 0; i--) {
      var hotkeyConfig =
            hotkeyConfigList.queryElementAt(i, Ci.sbIHotkeyConfiguration);
      if (hotkeyConfig.action == aAction)
        hotkeyConfigList.removeElementAt(i);
    }
    this.setHotkeys(hotkeyConfigList);
  },

  get hotkeysEnabled() {
    return SBDataGetBoolValue("globalhotkeys.enabled");
  },

  set hotkeysEnabled(aEnabled) {
    SBDataSetBoolValue("globalhotkeys.enabled", aEnabled);
    this._loadHotkeysFromPrefs();
  },

  get hotkeysEnabledDRKey() {
    return "globalhotkeys.enabled";
  },

  ///////////////////////////////////////////////////////////////////
  // Internal Methods
  ///////////////////////////////////////////////////////////////////
  _init: function() {
    if(!HotkeyManager) {
      Cu.reportError("Global Hotkey Manager is not available.");
      return;
    }

    this._initPlatform();
    this._initMetaKeyString();
    this._initHotkeyHandler();

    this._loadDefaultHotkeys();

    this._loadHotkeysFromPrefs();

    if(!HotkeyActions) {
      Cu.reportError("Global Hotkey Actions Service is not available.");
      return;
    }

    // Register the default set of actions.
    var defaultActions = Cc[SB_DEFGLOBALHKACTIONS_CONTRACTID]
                           .createInstance(Ci.sbIHotkeyActionBundle);
    HotkeyActions.registerHotkeyActionBundle(defaultActions);

    // Register observer for application shutdown to clean up.
    ObserverService.addObserver(this, SHUTDOWN_TOPIC, false);

    // Indicate that the service is ready.
    var serviceManager = Cc["@songbirdnest.com/Songbird/ServiceManager;1"]
                           .getService(Ci.sbIServiceManager);
    serviceManager.setServiceReady(SB_HOTKEY_SERVICE_CONTRACTID, true);
  },

  _shutdown: function() {
    // Indicate that the service is no longer ready.
    var serviceManager = Cc["@songbirdnest.com/Songbird/ServiceManager;1"]
                           .getService(Ci.sbIServiceManager);
    serviceManager.setServiceReady(SB_HOTKEY_SERVICE_CONTRACTID, false);

    if (CommandLine) {
      CommandLine.removeFlagHandler(this._hotkeyHandler, "hotkey");
    }

    this._hotkeyHandler = null;

    if (HotkeyManager) {
      HotkeyManager.removeAllHotkeys();
    }
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
                                                Ci.nsISupportsWeakReference])
      };
    }

    CommandLine.addFlagHandler(this._hotkeyHandler, "hotkey");
  },

  /**
   * \brief Load the hotkeys saved in the users prefs
   */
  _loadHotkeysFromPrefs: function() {
    this._removeHotkeyBindings();

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
        HotkeyManager.addHotkey(keyCode,
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

    // Hot key list
    var metaKey = this._metaKeyString;
    var hotkeyConfigList =
    [
      // Media keys
      { key:         "$176",
        keyReadable: "nexttrack",
        action:      "playback.nexttrack" },

      { key:         "$177",
        keyReadable: "prevtrack",
        action:      "playback.previoustrack" },

      { key:         "$179",
        keyReadable: "playpause",
        action:      "playback.playpause" },

      // Regular keys
      { key:         "meta-$190",
        keyReadable: metaKey + " - .",
        action:      "playback.volumeup" },

      { key:         "meta-$188",
        keyReadable: metaKey + " - ,",
        action:      "playback.volumedown" },

      { key:         "meta-$221",
        keyReadable: metaKey + " - ]",
        action:      "playback.nexttrack" },

      { key:         "meta-$219",
        keyReadable: metaKey + " - [",
        action:      "playback.previoustrack" },

      { key:         "meta-$220",
        keyReadable: metaKey + " - \\",
        action:      "playback.playpause" },

      { key:         "$178",
        keyReadable: "stop",
        action:      "playback.stop" }
    ];

    // Build an array of hot key configurations
    var hotkeyConfigArray = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                              .createInstance(Ci.nsIMutableArray);
    for (var i = 0; i < hotkeyConfigList.length; i++) {
      var hotkeyConfiguration = Cc[SB_HOTKEY_CONFIGURATION_CONTRACTID]
                                  .createInstance(Ci.sbIHotkeyConfiguration);
      hotkeyConfiguration.key =         hotkeyConfigList[i].key;
      hotkeyConfiguration.keyReadable = hotkeyConfigList[i].keyReadable;
      hotkeyConfiguration.action =      hotkeyConfigList[i].action;
      hotkeyConfigArray.appendElement(hotkeyConfiguration, false);
    }

    // Set the default hot keys
    this.setHotkeys(hotkeyConfigArray);
  },

  /**
   * \brief Remove all hotkeys currently bound
   */
  _removeHotkeyBindings: function() {
    if (HotkeyManager) {
      HotkeyManager.removeAllHotkeys();
    }
  },

  _stringToKeyCode: function(aStr) {
    if (aStr.slice(0, 1) == '$') {
      return aStr.slice(1);
    }

    return 0;
  },

  _configListContainsKey: function(aHotkeyConfigList, aKey) {
    for (var i = 0; i < aHotkeyConfigList.length; i++) {
      var hotkeyConfig =
            aHotkeyConfigList.queryElementAt(i, Ci.sbIHotkeyConfiguration);
      if (hotkeyConfig.key == aKey)
        return true;
    }

    return false;
  },

  _removeHotkeyFromConfigList: function(aHotkeyConfigList, aKey) {
    for (var i = aHotkeyConfigList.length - 1; i >= 0; i--) {
      var hotkeyConfig =
            aHotkeyConfigList.queryElementAt(i, Ci.sbIHotkeyConfiguration);
      if (hotkeyConfig.key == aKey)
        aHotkeyConfigList.removeElementAt(i);
    }
  },

  className: SB_HOTKEY_SERVICE_CLASSNAME,
  classDescription: SB_HOTKEY_SERVICE_DESC,
  classID: Components.ID(SB_HOTKEY_SERVICE_CID),
  contractID: SB_HOTKEY_SERVICE_CONTRACTID,
  _xpcom_categories: [
    {
      category: "app-startup",
      entry:    SB_HOTKEY_SERVICE_DESC,
      value:    "service," + SB_HOTKEY_SERVICE_CONTRACTID
      service:  true
    }
  ],
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.sbIHotkeyService])
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


function sbHotkeyConfiguration() {
}

sbHotkeyConfiguration.prototype =
{
  key: null,
  keyReadable: null,
  action: null,

  className: SB_HOTKEY_CONFIGURATION_CLASSNAME,
  classDescription: SB_HOTKEY_CONFIGURATION_DESC,
  classID: Components.ID(SB_HOTKEY_CONFIGURATION_CID),
  contractID: SB_HOTKEY_CONFIGURATION_CONTRACTID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.sbIHotkeyConfiguration])
};


//------------------------------------------------------------------------------
// XPCOM Registration

// function NSGetModule(compMgr, fileSpec)
// {
//   return XPCOMUtils.generateModule([sbHotkeyService,
//                                     sbDefaultGlobalHotkeyActions,
//                                     sbHotkeyConfiguration]);
// }

var NSGetModule = XPCOMUtils.generateNSGetFactory([sbHotkeyService]);
