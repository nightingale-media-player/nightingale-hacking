/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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
 * \file  sbWindowsAutoPlayService.js
 * \brief Songbird Windows AutoPlay Service Source.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Windows AutoPlay Service component.
//
//   This component provides various Windows AutoPlay services.  This component
// handles Windows AutoPlay flags in the Songbird command line.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Windows AutoPlay service imported services.
//
//------------------------------------------------------------------------------

// Component manager defs.
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

// Songbird imports.
Cu.import("resource://app/jsmodules/ObserverUtils.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");


//------------------------------------------------------------------------------
//
// Windows AutoPlay service configuration.
//
//------------------------------------------------------------------------------

//
// classDescription             Description of component class.
// classID                      Component class ID.
// contractID                   Component contract ID.
// ifList                       List of external component interfaces.
// _xpcom_categories            List of component categories.
//

var sbWindowsAutoPlayServiceCfg = {
  classDescription: "Songbird Windows AutoPlay Service",
  classID: Components.ID("{3124ec90-1dd2-11b2-8059-c4e994415c12}"),
  contractID: "@songbirdnest.com/Songbird/WindowsAutoPlayService;1",
  ifList: [ Ci.sbIWindowsAutoPlayService,
            Ci.sbICommandLineFlagHandler,
            Ci.nsIObserver ]
};

sbWindowsAutoPlayServiceCfg._xpcom_categories = [
  {
    category: "app-startup",
    entry:    sbWindowsAutoPlayServiceCfg.className,
    value:    "service," + sbWindowsAutoPlayServiceCfg.contractID
  }
];


//------------------------------------------------------------------------------
//
// Windows AutoPlay service object.
//
//------------------------------------------------------------------------------

/**
 * Construct a Windows AutoPlay service object.
 */

function sbWindowsAutoPlayService() {
  // Initialize.
  this._initialize();
}

// Define the object prototype.
sbWindowsAutoPlayService.prototype = {
  // Set the constructor.
  constructor: sbWindowsAutoPlayService,

  //
  // Windows AutoPlay service fields.
  //
  //   classDescription         Description of component class.
  //   classID                  Component class ID.
  //   contractID               Component contract ID.
  //   _xpcom_categories        List of component categories.
  //
  //   _cfg                     Configuration settings.
  //   _isInitialized           True if the services have been initialized.
  //   _isProcessingEvents      True if processing Windows AutoPlay events.
  //   _observerSet             Set of observers.
  //   _actionHandlerTable      Table mapping AutoPlay actions to lists of
  //                            action handlers.
  //

  classDescription: sbWindowsAutoPlayServiceCfg.classDescription,
  classID: sbWindowsAutoPlayServiceCfg.classID,
  contractID: sbWindowsAutoPlayServiceCfg.contractID,
  _xpcom_categories: sbWindowsAutoPlayServiceCfg._xpcom_categories,

  _cfg: sbWindowsAutoPlayServiceCfg,
  _isInitialized: false,
  _isProcessingEvents: false,
  _observerSet: null,
  _actionHandlerTable: null,


  //----------------------------------------------------------------------------
  //
  // Windows AutoPlay sbIWindowsAutoPlayService services.
  //
  //----------------------------------------------------------------------------

  /**
   * Add the action handler specified by aHandler as a handler for the AutoPlay
   * action specified by aAction.
   *
   * \param aHandler            AutoPlay action handler.
   * \param aAction             Action to handle.
   */

  addActionHandler:
    function sbWindowsAutoPlayService_addActionHandler(aHandler, aAction) {
    // Get the action handler list, creating one if needed.
    var actionHandlerList;
    if (aAction in this._actionHandlerTable) {
      actionHandlerList = this._actionHandlerTable[aAction];
    } else {
      actionHandlerList = [];
      this._actionHandlerTable[aAction] = actionHandlerList;
    }

    // Add the handler if it's not already in the list.
    if (actionHandlerList.indexOf(aHandler) < 0)
      actionHandlerList.push(aHandler);
  },


  /**
   * Remove the action handler specified by aHandler as a handler for the
   * AutoPlay action specified by aAction.
   *
   * \param aHandler            AutoPlay action handler.
   * \param aAction             Action to handle.
   */

  removeActionHandler:
    function sbWindowsAutoPlayService_removeActionHandler(aHandler, aAction) {
    // Do nothing if not initialized.  This allows clients to call this function
    // if they're finalized after the Windows AutoPlay service is finalized.
    if (!this._isInitialized)
      return;

    // Get the action handler list.  Do nothing if no handlers for the action.
    if (!(aAction in this._actionHandlerTable))
      return;
    var actionHandlerList = this._actionHandlerTable[aAction];

    // Remove handler from the list.
    var handlerIndex = actionHandlerList.indexOf(aHandler);
    if (handlerIndex >= 0)
      actionHandlerList.splice(handlerIndex, 1);

    // Remove the action handler list if it's empty.
    if (!actionHandlerList.length)
      delete this._actionHandlerTable[aAction];
  },


  //----------------------------------------------------------------------------
  //
  // Windows AutoPlay nsIObserver services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the observed event specified by aSubject, aTopic, and aData.
   *
   * \param aSubject            Event subject.
   * \param aTopic              Event topic.
   * \param aData               Event data.
   */

  observe: function sbWindowsAutoPlayService_observe(aSubject, aTopic, aData) {
    // Dispatch processing of the event.
    switch (aTopic) {
      case "app-startup" :
        this._handleAppStartup();
        break;

      case "songbird-main-window-presented" :
        this._handleSBMainWindowPresented();
        break;

      case "quit-application-granted" :
        this._handleQuitApplicationGranted();
        break;

      default :
        break;
    }
  },


  //----------------------------------------------------------------------------
  //
  // Windows AutoPlay sbICommandLineFlagHandler services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Handle a command line flag
   * \param aFlag The command line flag
   * \param aParam The parameter for the flag
   * \return Return true if the handler processed this item. Items that have
   *         not been processed are passed on to the next command line item
   *         handler
   */

  handleFlag: function sbWindowsAutoPlayService_handleFlag(aFlag, aParam) {
    // Dispatch processing of flag.  Return that flag was not processed if it is
    // not recognized.
    switch (aFlag)
    {
      case "autoplay-manage-volume-device" :
        this._handleAutoPlayManageVolumeDevice();
        break;
        
      case "autoplay-cd-rip" :
        this._handleAutoPlayCDRip();
        break;
        
      default :
        return false;
    }

    return true;
  },


  //----------------------------------------------------------------------------
  //
  // Windows AutoPlay nsISupports services.
  //
  //----------------------------------------------------------------------------

  QueryInterface: XPCOMUtils.generateQI(sbWindowsAutoPlayServiceCfg.ifList),


  //----------------------------------------------------------------------------
  //
  // Windows AutoPlay action services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle a Windows AutoPlay manage volume device action.
   */

  _handleAutoPlayManageVolumeDevice:
    function sbWindowsAutoPlayService__handleAutoPlayManageVolumeDevice() {
    // Handle the action.
    var actionHandled =
          this._handleAction
                 (Ci.sbIWindowsAutoPlayService.ACTION_MANAGE_VOLUME_DEVICE,
                  null);

    // If the action was not handled, alert the user that the device cannot be
    // managed.  Wait until a Songbird main window is available so it can be
    // used as the parent of the alert.
    if (!actionHandled) {
      var windowWatcher = Cc["@songbirdnest.com/Songbird/window-watcher;1"]
                            .getService(Ci.sbIWindowWatcher);
      var _this = this;
      var func = function(aWindow)
                   { _this._alertUserCannotManageDevice(aWindow); };
      windowWatcher.callWithWindow("Songbird:Main", func);
    }
  },


  /**
   * Handle a CD Rip action
   */
   
  _handleAutoPlayCDRip:
    function sbWindowsAutoPlayService__handleAutoPlayCDRip() {
    // Handle the action.
    var actionHandled =
          this._handleAction
                 (Ci.sbIWindowsAutoPlayService.ACTION_CD_RIP,
                  null);

    // If the action was not handled, alert the user that the device cannot be
    // managed.  Wait until a Songbird main window is available so it can be
    // used as the parent of the alert.
    if (!actionHandled) {
      var windowWatcher = Cc["@songbirdnest.com/Songbird/window-watcher;1"]
                            .getService(Ci.sbIWindowWatcher);
      var _this = this;
      var func = function(aWindow)
                   { _this._alertUserCannotCDRip(aWindow); };
      windowWatcher.callWithWindow("Songbird:Main", func);
    }
  },


  /**
   * Handle the Windows AutoPlay action specified by aAction with the argument
   * specified by aActionArg.  Return true if the action is handled.
   *
   * \param aAction             AutoPlay action to handle.
   * \param aActionArg          Action argument.
   *
   * \return true               Action was handled.
   */

  _handleAction: function sbWindowsAutoPlayService__handleAction(aAction,
                                                                 aActionArg) {
    // Get the action handler list.  Do nothing if no handlers for action.
    if (!(aAction in this._actionHandlerTable))
      return false;
    var actionHandlerList = this._actionHandlerTable[aAction];

    // Dispatch action to the handlers until it's handled.
    var actionHandled = false;
    for (var i = 0; i < actionHandlerList.length; i++) {
      var actionHandler = actionHandlerList[i];
      try {
        actionHandled = actionHandler.handleAction(aAction, aActionArg);
      } catch (ex) {
        Cu.reportError(ex);
      }
      if (actionHandled)
        break;
    }

    return actionHandled;
  },


  /**
   * Alert the user that a device cannot be managed.  Use the window specified
   * by aWindow for the parent window of the alert.
   *
   * \param aWindow             Parent window of alert.
   */

  _alertUserCannotManageDevice:
    function sbWindowsAutoPlayService__alertUserCannotManageDevice(aWindow) {
    // Get the application services.
    var Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);

    // Determine which dialog strings to use.
    var stringNamePrefix = "windows.autoplay.cannot_manage_device.dialog.";
    if (Application.extensions.has("msc@songbirdnest.com")) {
      if (!Application.extensions.get("msc@songbirdnest.com").enabled) {
        stringNamePrefix =
          "windows.autoplay.cannot_manage_device.msc_not_enabled.dialog.";
      }
    } else {
      stringNamePrefix =
        "windows.autoplay.cannot_manage_device.msc_not_installed.dialog.";
    }

    // Alert user.
    var prompter = Cc["@songbirdnest.com/Songbird/Prompter;1"]
                     .createInstance(Ci.sbIPrompter);
    var title = SBString(stringNamePrefix + "title");
    var msg = SBString(stringNamePrefix + "msg");
    prompter.alert(aWindow, title, msg);
  },


  /**
   * Alert the user that the CD cannot be ripped. Use the window specified
   * by aWindow for the parent window of the alert.
   *
   * \param aWindow             Parent window of the alert.
   */
   
  _alertUserCannotCDRip:
    function sbWindowsAutoPlayService__alertUserCannotCDRip(aWindow) {
    var prefix = "windows.autoplay.cannot_cd_rip.dialog.";
    var prompter = Cc["@songbirdnest.com/Songbird/Prompter;1"]
                     .createInstance(Ci.sbIPrompter);
    var title = SBString(prefix + "title");
    var msg = SBString(prefix + "msg");
    prompter.alert(aWindow, title, msg);
  },
  
  //----------------------------------------------------------------------------
  //
  // Windows AutoPlay event handler services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle application startup events.
   */

  _handleAppStartup: function sbWindowsAutoPlayService__handleAppStartup() {
    // Initialize the services.
    this._initialize();
  },


  /**
   * Handle Songbird main window presented events.
   */

  _handleSBMainWindowPresented:
    function sbWindowsAutoPlayService__handleSBMainWindowPresented() {
    // Start processing Windows AutoPlay events.
    // Wait until the maing window is presented so that all clients can register
    // AutoPlay handlers.
    this._start();
  },


  /**
   * Handle quit application granted events.
   */

  _handleQuitApplicationGranted:
    function sbWindowsAutoPlayService__handleQuitApplicationGranted() {
    // Finalize the services.
    this._finalize();
  },


  //----------------------------------------------------------------------------
  //
  // Windows AutoPlay command line services.
  //
  //----------------------------------------------------------------------------

  /**
   * Start Windows AutoPlay command line event processing.
   */

  _startCommandLine: function sbWindowsAutoPlayService__startCommandLine() {
    // Get the Songbird command line manager.
    var commandLineManager =
      Cc["@songbirdnest.com/commandlinehandler/general-startup;1?type=songbird"]
        .getService(Ci.sbICommandLineManager);

    // Add Windows AutoPlay command line flag handlers.
    commandLineManager.addFlagHandler(this, "autoplay-manage-volume-device");
    
    // Add Windows CD AutoPlay rip command line flag handlers
    commandLineManager.addFlagHandler(this, "autoplay-cd-rip");
  },


  /**
   * Stop Windows AutoPlay command line event processing.
   */

  _stopCommandLine: function sbWindowsAutoPlayService__stopCommandLine() {
    // Get the Songbird command line manager.
    var commandLineManager =
      Cc["@songbirdnest.com/commandlinehandler/general-startup;1?type=songbird"]
        .getService(Ci.sbICommandLineManager);

    // Remove Windows AutoPlay command line flag handlers.
    commandLineManager.removeFlagHandler(this, "autoplay-manage-volume-device");
    
    // Remove Windows CD AutoPlay rip command line flag handlers
    commandLineManager.removeFlagHandler(this, "autoplay-cd-rip");
  },


  //----------------------------------------------------------------------------
  //
  // Internal Windows AutoPlay services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the services.
   */

  _initialize: function sbWindowsAutoPlayService__initialize() {
    // Do nothing if already initialized.
    if (this._isInitialized)
      return;

    // Initialize the action handler table.
    this._actionHandlerTable = {};

    // Set up observers.
    if (!this._observerSet) {
      this._observerSet = new ObserverSet();
      this._observerSet.add(this,
                            "songbird-main-window-presented",
                            false,
                            false);
      this._observerSet.add(this, "quit-application-granted", false, false);
    }

    // Initialization is now complete.
    this._isInitialized = true;
  },


  /**
   * Finalize the services.
   */

  _finalize: function sbWindowsAutoPlayService__finalize() {
    // Remove observers.
    if (this._observerSet) {
      this._observerSet.removeAll();
      this._observerSet = null;
    }

    // Stop processing Windows AutoPlay events.
    this._stop();

    // Remove object references.
    this._actionHandlerTable = null;

    // No longer initialized.
    this._isInitialized = false;
  },


  /**
   * Start processing AutoPlay events.
   */

  _start: function sbWindowsAutoPlayService__start() {
    // Do nothing if already processing events.  Immediately indicate that
    // Windows AutoPlay events are being processed to avoid re-entrancy issues.
    if (this._isProcessingEvents)
      return;
    this._isProcessingEvents = true;

    // Start command line event processing.
    this._startCommandLine();
  },


  /**
   * Stop processing AutoPlay events.
   */

  _stop: function sbWindowsAutoPlayService__stop() {
    // Do nothing if not processing events.
    if (!this._isProcessingEvents)
      return;

    // Stop command line event processing.
    this._stopCommandLine();

    // Indicate that Windows AutoPlay events are no longer being processed.
    this._isProcessingEvents = false;
  }
};


//------------------------------------------------------------------------------
//
// Windows AutoPlay service component services.
//
//------------------------------------------------------------------------------

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbWindowsAutoPlayService]);
}


