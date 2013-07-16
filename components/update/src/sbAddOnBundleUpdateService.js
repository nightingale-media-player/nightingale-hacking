/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
 * \file  sbUpdateAddOnBundleService.js
 * \brief Javascript source for the add-on bundle update service component.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Add-on bundle update service component.
//
//   This component periodically checks for updates to the add-on bundle and
// presents new add-ons for installation to the user.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Add-on bundle update service imported services.
//
//------------------------------------------------------------------------------

// Songbird services.
Components.utils.import("resource://app/jsmodules/AddOnUtils.jsm");
Components.utils.import("resource://app/jsmodules/ObserverUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBUtils.jsm");
Components.utils.import("resource://app/jsmodules/WindowUtils.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


//------------------------------------------------------------------------------
//
// Add-on bundle update service defs.
//
//------------------------------------------------------------------------------

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;


//------------------------------------------------------------------------------
//
// Add-on bundle update service configuration.
//
//------------------------------------------------------------------------------

//
// className                    Name of component class.
// cid                          Component CID.
// contractID                   Component contract ID.
// ifList                       List of external component interfaces.
// categoryList                 List of component categories.
//
// updateEnabledPref            Preference for enabling add-on bundle updates.
// updateIntervalPref           Preference for setting interval between add-on
//                              bundle update checks.
// updatePrevAppVersionPref     Preference containing the version of the
//                              Application when the add-on bundle update
//                              service previously checked for updates.
// defaultUpdateEnabled         Default update enabled preference value.
// defaultUpdateInterval        Default add-on bundle update interval in
//                              seconds.
//

var sbAddOnBundleUpdateServiceCfg = {
  classDescription: "Songbird Add-on Bundle Update Service",
  className: "AddonBundleUpdateService",
  cid: Components.ID("{927d9849-8565-4bc4-805a-f3a6ad1b25ec}"),
  contractID: "@songbirdnest.com/AddOnBundleUpdateService;1",
  ifList: [ Ci.sbIAddOnBundleUpdateService, Ci.nsIObserver ],

  updateEnabledPref: "songbird.recommended_addons.update.enabled",
  updateIntervalPref: "songbird.recommended_addons.update.interval",
  updatePrevAppVersionPref:
    "songbird.recommended_addons.update.prev_app_version",
  defaultUpdateEnabled: false,
  defaultUpdateInterval: 86400
};

sbAddOnBundleUpdateServiceCfg.categoryList = [
  {
    category: "app-startup",
    entry:    sbAddOnBundleUpdateServiceCfg.className,
    value:    "service," + sbAddOnBundleUpdateServiceCfg.contractID
  }
];


//------------------------------------------------------------------------------
//
// Add-on bundle update service.
//
//------------------------------------------------------------------------------

/**
 * Construct an add-on bundle update service object.
 */

function sbAddOnBundleUpdateService() {
}

// Define the object.
sbAddOnBundleUpdateService.prototype = {
  // Set the constructor.
  constructor: sbAddOnBundleUpdateService,

  //
  // Add-on bundle update service fields.
  //
  //   classDescription         Description of component class.
  //   classID                  Component class ID.
  //   contractID               Component contract ID.
  //   _xpcom_categories        List of component categories.
  //
  //   _cfg                     Configuration settings.
  //   _isInitialized           True if the add-on bundle update service has
  //                            been initialized.
  //   _observerSet             Set of observers.
  //   _prefsAvailable          True if preferences are available.
  //   _networkAvailable        True if the network is available.
  //   _updateEnabled           True if add-on bundle update is enabled.
  //   _checkedFirstRunHasCompleted
  //                            True if a check has been made for first-run
  //                            completion.
  //   _firstRunHasCompleted    True if the first-run has been completed.
  //   _addOnBundleLoader       Add-on bundle loader object.
  //

  classDescription: sbAddOnBundleUpdateServiceCfg.classDescription,
  className: sbAddOnBundleUpdateServiceCfg.className,
  classID: sbAddOnBundleUpdateServiceCfg.cid,
  contractID: sbAddOnBundleUpdateServiceCfg.contractID,
  _xpcom_categories: sbAddOnBundleUpdateServiceCfg.categoryList,

  _cfg: sbAddOnBundleUpdateServiceCfg,
  _isInitialized: false,
  _observerSet: null,
  _prefsAvailable: false,
  _networkAvailable: false,
  _updateEnabled: false,
  _checkedFirstRunHasCompleted: false,
  _firstRunHasCompleted: false,
  _addOnBundleLoader: null,


  //----------------------------------------------------------------------------
  //
  // Add-on bundle update service sbIAddOnBundleUpdateService services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief True if a restart is required.
   */

  restartRequired: false,


  /**
   * \brief Check for updates to the add-on bundle and present any to user.
   */

  checkForUpdates: function sbAddOnBundleUpdateService_checkForUpdates() {
    // Ensure the services are initialized.
    this._initialize();

    // Present any new add-ons if update is enabled.
    if (this._updateEnabled) {
      // Update the add-on bundle cache synchronously if the application was
      // updated.
      if (this._getApplicationWasUpdated())
        this._updateAddOnBundleCache(true);

      // Present any new add-ons.
      this._presentNewAddOns();
    }
  },


  //----------------------------------------------------------------------------
  //
  // Add-on bundle update service nsIObserver services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the observed event specified by aSubject, aTopic, and aData.
   *
   * \param aSubject            Event subject.
   * \param aTopic              Event topic.
   * \param aData               Event data.
   */

  observe: function sbAddOnBundleUpdateService_observe(aSubject,
                                                       aTopic,
                                                       aData) {
    // Dispatch processing of the event.
    switch (aTopic) {
      case "app-startup" :
        this._handleAppStartup();
        break;

      case "profile-after-change" :
        this._handleProfileAfterChange();
        break;

      case "final-ui-startup" :
        this._handleFinalUIStartup();
        break;

      case "quit-application" :
        this._handleAppQuit();
        break;

      default :
        break;
    }
  },


  //----------------------------------------------------------------------------
  //
  // Add-on bundle update service nsISupports services.
  //
  //----------------------------------------------------------------------------

  QueryInterface: XPCOMUtils.generateQI([
    Ci.sbIAddOnBundleUpdateService, 
    Ci.nsIObserver
  ]),


  //----------------------------------------------------------------------------
  //
  // Add-on bundle update service event handler services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle application startup events.
   */

  _handleAppStartup: function sbAddOnBundleUpdateService__handleAppStartup() {
    // Initialize the services.
    this._initialize();
  },


  /**
   * Handle profile after change events.
   */

  _handleProfileAfterChange:
    function sbAddOnBundleUpdateService__handleProfileAfterChange() {
    // Preferences are now available.
    this._prefsAvailable = true;

    // Initialize the services.
    this._initialize();
  },


  /**
   * Handle final UI startup events.
   */

  _handleFinalUIStartup:
    function sbAddOnBundleUpdateService__handleFinalUIStartup() {
    // The network is now available.
    //XXXeps trying to load the add-on bundle too early will result in a hang
    //XXXeps during EM restart.  Not sure how to fix this better.
    this._networkAvailable = true;

    // Initialize the services.
    this._initialize();
  },


  /**
   * Handle application quit events.
   */

  _handleAppQuit: function sbAddOnBundleUpdateService__handleAppQuit() {
    // Finalize the services.
    this._finalize();
  },


  /**
   * Handle add-on update timer events.
   */

  _handleAddOnUpdateTimer:
    function sbAddOnBundleUpdateService__handleAddOnUpdateTimer(aTimer) {
    // Update the add-on bundle cache.
    this._updateAddOnBundleCache(false);
  },


  //----------------------------------------------------------------------------
  //
  // Internal add-on bundle update services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the services.
   */

  _initialize: function sbAddOnBundleUpdateService__initialize() {
    // Do nothing if already initialized.
    if (this._isInitialized)
      return;

    // Set up observers.
    if (!this._observerSet) {
      this._observerSet = new ObserverSet();
      this._observerSet.add(this, "quit-application", false, false);
      this._observerSet.add(this, "profile-after-change", false, true);
      this._observerSet.add(this, "final-ui-startup", false, true);
    }

    // Wait until preferences are available.
    if (!this._prefsAvailable)
      return;

    // Initialize the previous application version if first-run has not
    // completed.  This needs to be done as soon as possible so it occurs before
    // the first-run.
    if (!this._checkedFirstRunHasCompleted) {
      this._firstRunHasCompleted = SBUtils.hasFirstRunCompleted();
      if (!this._firstRunHasCompleted)
        this._updatePrevAppVersion();
      this._checkedFirstRunHasCompleted = true;
    }

    // Wait until the network is available.
    if (!this._networkAvailable)
      return;

    // Initialization is now complete.
    this._isInitialized = true;

    // Don't check for add-on bundle updates if the first-run has not completed.
    if (!this._firstRunHasCompleted)
      return;

    // Get the application services.
    var Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);

    // Check if add-on bundle update is enabled.  Do nothing more if not.
    this._updateEnabled =
           Application.prefs.getValue(this._cfg.updateEnabledPref,
                                      this._cfg.defaultUpdateEnabled);
    if (!this._updateEnabled)
      return;

    // Get the add-on bundle update period.
    var updateInterval =
          Application.prefs.getValue(this._cfg.updateIntervalPref,
                                     this._cfg.defaultUpdateInterval);

    // Register an add-on bundle update timer.
    /*XXXeps no way to unregister. */
    var updateTimerMgr = Cc["@mozilla.org/updates/timer-manager;1"]
                           .createInstance(Ci.nsIUpdateTimerManager);
    var _this = this;
    var func = function(aTimer) { _this._handleAddOnUpdateTimer(aTimer); };
    updateTimerMgr.registerTimer("add-on-bundle-update-timer",
                                 func,
                                 updateInterval);
  },


  /**
   * Finalize the services.
   */

  _finalize: function sbAddOnBundleUpdateService__finalize() {
    // Cancel the add-on bundle loader.
    if (this._addOnBundleLoader) {
      this._addOnBundleLoader.cancel();
      this._addOnBundleLoader = null;
    }

    // Remove observers.
    if (this._observerSet) {
      this._observerSet.removeAll();
      this._observerSet = null;
    }
  },


  /**
   * Present the new add-ons to the user.
   */

  _presentNewAddOns:
    function sbAddOnBundleUpdateService__presentNewAddOns() {
    // Load the add-on bundle.
    var addOnBundle = this._loadNewAddOns();

    // Do nothing if no add-ons.
    if (!addOnBundle || (addOnBundle.bundleExtensionCount == 0))
      return;

    // Present the new add-ons.
    var restartRequired = {};
    WindowUtils.openModalDialog
                  (null,
                   "chrome://songbird/content/xul/recommendedAddOnsWizard.xul",
                   "",
                   "chrome,modal=yes,centerscreen",
                   [ addOnBundle ],
                   [ restartRequired ]);
    this.restartRequired = (restartRequired.value == "true");
  },


  /**
   * Load the new add-on bundle.
   *
   * \return                    New add-on bundle.
   */

  _loadNewAddOns: function sbAddOnBundleUpdateService__loadNewAddOns() {
    // Create an add-on bundle loader.
    var addOnBundleLoader = new AddOnBundleLoader();

    // Add all installed add-ons to the blacklist.  This prevents an add-on
    // from being presented if it was previously installed and then uninstalled.
    AddOnBundleLoader.addInstalledAddOnsToBlacklist();

    // Load the add-on bundle from cache.
    addOnBundleLoader.filterInstalledAddOns = true;
    addOnBundleLoader.filterBlacklistedAddOns = true;
    addOnBundleLoader.readFromCache = true;
    addOnBundleLoader.start(null);

    // Check for add-on bundle loading errors.
    if (!addOnBundleLoader.complete ||
        !Components.isSuccessCode(addOnBundleLoader.result))
      return null;

    return addOnBundleLoader.addOnBundle;
  },


  /**
   * Update the add-on bundle cache.  If aSync is true, wait until the cache has
   * been updated.
   *
   * \param aSync               If true, operate synchronously.
   */

  _updateAddOnBundleCache:
    function sbAddOnBundleUpdateService__updateAddOnBundleCache(aSync) {
    // Start loading the add-on bundle into the cache.
    if (!this._addOnBundleLoader) {
      // Create an add-on bundle loader.
      this._addOnBundleLoader = new AddOnBundleLoader();

      // Start loading the add-on bundle into the cache.
      var _this = this;
      var func = function() { _this._updateAddOnBundleCacheContinue(); }
      this._addOnBundleLoader.start(func);
    }

    // Wait for update to complete if operating synchronously.
    if (aSync) {
      // Get the current thread.
      var threadManager = Cc["@mozilla.org/thread-manager;1"]
                            .getService(Ci.nsIThreadManager);
      currentThread = threadManager.currentThread;

      // Process events until update completes.
      while (this._addOnBundleLoader && !this._addOnBundleLoader.complete) {
        currentThread.processNextEvent(true);
      }
    }
  },


  /**
   * Continue the add-on bundle cache update operation.
   */

  _updateAddOnBundleCacheContinue:
    function sbAddOnBundleUpdateService__updateAddOnBundleCacheContinue() {
    // Clear the add-on bundle loader upon completion.
    if (this._addOnBundleLoader.complete)
      this._addOnBundleLoader = null;
  },


  /**
   * Return true if the application was updated.
   *
   * \return                    True if application was updated.
   */

  _getApplicationWasUpdated:
    function sbAddOnBundleUpdateService__getApplicationWasUpdated() {
    var updated = false;

    // Get the application services.
    var Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);

    // Get the current application version.
    var appInfo = Cc["@mozilla.org/xre/app-info;1"]
                    .getService(Ci.nsIXULAppInfo);
    var appVersion = appInfo.version;

    // Get the previous application version.
    var prevAppVersion =
          Application.prefs.getValue(this._cfg.updatePrevAppVersionPref, "");

    // Check for application update.  If the previous application version
    // preference has not been set, the application must have been updated from
    // a version of the application before the add-on bundle update support was
    // added.
    if (!prevAppVersion || (prevAppVersion != appVersion))
      updated = true;

    // Update the previous application version.
    Application.prefs.setValue(this._cfg.updatePrevAppVersionPref, appVersion);

    return updated;
  },


  /**
   * Update the previous application version preference with the current
   * application version.
   */

  _updatePrevAppVersion:
    function sbAddOnBundleUpdateService__updatePrevAppVersion() {
    // Get the current application version.
    var appInfo = Cc["@mozilla.org/xre/app-info;1"]
                    .getService(Ci.nsIXULAppInfo);
    var appVersion = appInfo.version;

    // Update the previous application version.
    var Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);
    Application.prefs.setValue(this._cfg.updatePrevAppVersionPref, appVersion);
  }
};


//------------------------------------------------------------------------------
//
// Add-on bundle update service component services.
//
//------------------------------------------------------------------------------

var NSGetFactory = XPCOMUtils.generateNSGetFactory([sbAddOnBundleUpdateService]);
