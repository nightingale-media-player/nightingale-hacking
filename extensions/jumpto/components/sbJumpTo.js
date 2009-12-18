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

/**
 * \file  sbJumpTo.js
 * \brief Javascript source for the jump to service component.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Jump to service component.
//
//   This component provides support for the jump to services.  It sets up jump
// to hotkey support.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Jump to service imported services.
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
Cu.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");


//------------------------------------------------------------------------------
//
// Jump to service configuration.
//
//------------------------------------------------------------------------------

//
// classDescription             Description of component class.
// classID                      Component class ID.
// contractID                   Component contract ID.
// ifList                       List of external component interfaces.
// _xpcom_categories            List of component categories.
//

var sbJumpToServiceCfg = {
  classDescription: "Jump To Service",
  classID: Components.ID("{41c2d712-1dd2-11b2-8a9f-9ece961c3675}"),
  contractID: "@songbirdnest.com/Songbird/JumpToService;1",
  ifList: [ Ci.nsIObserver ],

  version: 1,
  extensionUUID: "jumpto@songbirdnest.com"
};

sbJumpToServiceCfg._xpcom_categories = [
  {
    category: "app-startup",
    entry:    sbJumpToServiceCfg.classDescription,
    value:    "service," + sbJumpToServiceCfg.contractID
  }
];


//------------------------------------------------------------------------------
//
// Jump to service object.
//
//------------------------------------------------------------------------------

/**
 * Construct a jump to service object.
 */

function sbJumpToService() {
}

// Define the object.
sbJumpToService.prototype = {
  // Set the constructor.
  constructor: sbJumpToService,

  //
  // Jump to service component configuration fields.
  //
  //   classDescription         Description of component class.
  //   classID                  Component class ID.
  //   contractID               Component contract ID.
  //   _xpcom_categories        List of component categories.
  //

  classDescription: sbJumpToServiceCfg.classDescription,
  classID: sbJumpToServiceCfg.classID,
  contractID: sbJumpToServiceCfg.contractID,
  _xpcom_categories: sbJumpToServiceCfg._xpcom_categories,

  //
  // Jump to service fields.
  //
  //   _cfg                     Configuration settings.
  //   _observerSet             Set of observers.
  //   _hotkeyServiceReady      If true, hotkey service is ready.
  //   _uninstallOnShutdown     If true, uninstall jump to service on shutdown.
  //

  _cfg: sbJumpToServiceCfg,
  _observerSet: null,
  _hotkeyServiceReady: false,
  _uninstallOnShutdown: false,


  //----------------------------------------------------------------------------
  //
  // Jump to service nsIObserver services.
  //
  //----------------------------------------------------------------------------

  /**
   * Observe will be called when there is a notification for the
   * topic |aTopic|.  This assumes that the object implementing
   * this interface has been registered with an observer service
   * such as the nsIObserverService.
   *
   * If you expect multiple topics/subjects, the impl is
   * responsible for filtering.
   *
   * You should not modify, add, remove, or enumerate
   * notifications in the implemention of observe.
   *
   * @param aSubject : Notification specific interface pointer.
   * @param aTopic   : The notification topic or subject.
   * @param aData    : Notification specific wide string.
   */

  observe: function sbJumpToService_observe(aSubject, aTopic, aData) {
    // Dispatch processing of the event.
    switch (aTopic) {
      case "app-startup" :
        // Initialize the services.
        this._initialize();
        break;

      case "service-ready" :
        // Initialize the services with hotkey service ready.
        if (aData == "@songbirdnest.com/Songbird/HotkeyService;1") {
          this._hotkeyServiceReady = true;
          this._initialize();
        }
        break;

      case "before-service-shutdown" :
        // Shutdown the services if dependent services are shutting down.
        if (aData == "@songbirdnest.com/Songbird/HotkeyService;1")
          this._shutdown();
        break;

      case "quit-application" :
        // Shutdown the services.
        this._shutdown();
        break;

      case "em-action-requested" :
        this._handleEMActionRequested(aSubject, aData);
        break;

      default :
        break;
    }
  },


  /**
   * Handle the extension manager action requested event specified by aSubject
   * and aData.
   *
   * \param aSubject            nsIUpdateItem detailing the action item.
   * \param aData               String specifying the action.
   */

  _handleEMActionRequested:
    function sbJumpToService__handleEMActionRequested(aSubject, aData) {
    // Do nothing if jump to extension is not being updated.
    var updateItem = aSubject.QueryInterface(Ci.nsIUpdateItem);
    if (updateItem.id != this._cfg.extensionUUID)
      return;

    // If extension is being uninstalled, mark to uninstall on shutdown.  If
    // cancelling action, don't uninstall on shutdown.
    if (aData == "item-uninstalled")
      this._uninstallOnShutdown = true;
    else if (aData == "item-cancel-action")
      this._uninstallOnShutdown = false;
  },


  //----------------------------------------------------------------------------
  //
  // Jump to service nsISupports services.
  //
  //----------------------------------------------------------------------------

  QueryInterface: XPCOMUtils.generateQI(sbJumpToServiceCfg.ifList),


  //----------------------------------------------------------------------------
  //
  // Internal jump to services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the services.
   */

  _initialize: function sbJumpToService__initialize() {
    // Set up observers.
    if (!this._observerSet) {
      this._observerSet = new ObserverSet();
      this._observerSet.add(this, "service-ready", false, false);
      this._observerSet.add(this, "before-service-shutdown", false, false);
      this._observerSet.add(this, "quit-application", false, false);
      this._observerSet.add(this, "em-action-requested", false, false);
    }

    // Do nothing more until the hotkey service is ready.
    if (!this._hotkeyServiceReady)
      return;

    // Install services.
    this._installServices();
  },


  /**
   * Finalize the services.
   */

  _finalize: function sbJumpToService__finalize() {
    // Remove observers.
    if (this._observerSet) {
      this._observerSet.removeAll();
      this._observerSet = null;
    }
  },


  /**
   * Shut down the services.
   */

  _shutdown: function sbJumpToService__shutdown() {
    // Uninstall services if set to do so.
    if (this._uninstallOnShutdown) {
      this._uninstallServices();
      this._uninstallOnShutdown = false;
    }

    // Finalize the services.
    this._finalize();
  },


  /**
   * Install the jump to services.
   */

  _installServices: function sbJumpToService__installServices() {
    // Do nothing if jump to services are already installed.
    if (SBDataGetIntValue("jumpto.installed_version"))
      return;

    // Get the hotkey services.
    var hotkeyService = Cc["@songbirdnest.com/Songbird/HotkeyService;1"]
                          .getService(Ci.sbIHotkeyService);

    // Add the open jump to window hotkey.
    var hotkeyConfig = Cc["@songbirdnest.com/Songbird/HotkeyConfiguration;1"]
                         .createInstance(Ci.sbIHotkeyConfiguration);
    hotkeyConfig.key = SBString("jumptofile.hotkey.open.key");
    hotkeyConfig.keyReadable = SBString("jumptofile.hotkey.open.key.readable");
    hotkeyConfig.action = "jumpto.open";
    if (!hotkeyService.getHotkey(hotkeyConfig.key))
      hotkeyService.addHotkey(hotkeyConfig);

    // Set the jump to installed version.
    SBDataSetIntValue("jumpto.installed_version", this._cfg.version);
  },


  /**
   * Uninstall the jump to services.
   */

  _uninstallServices: function sbJumpToService__uninstallServices() {
    // Jump to services are no longer installed.
    SBDataDeleteBranch("jumpto.installed_version");

    // Remove all jump to hotkeys.
    var hotkeyService = Cc["@songbirdnest.com/Songbird/HotkeyService;1"]
                          .getService(Ci.sbIHotkeyService);
    hotkeyService.removeHotkeysByAction("jumpto.open");
  }
};


//------------------------------------------------------------------------------
//
// Jump to service component services.
//
//------------------------------------------------------------------------------

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbJumpToService]);
}

