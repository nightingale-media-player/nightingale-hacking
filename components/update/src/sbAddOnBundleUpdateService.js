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
Components.utils.import("resource://app/jsmodules/ObserverUtils.jsm");
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
// defaultUpdateEnabled         Default update enabled preference value.
//

var sbAddOnBundleUpdateServiceCfg = {
  className: "Songbird Add-on Bundle Update Service",
  cid: Components.ID("{927d9849-8565-4bc4-805a-f3a6ad1b25ec}"),
  contractID: "@songbirdnest.com/AddOnBundleUpdateService;1",
  ifList: [ Ci.nsIObserver ],

  defaultUpdateEnabled: false
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
  //

  classDescription: sbAddOnBundleUpdateServiceCfg.className,
  classID: sbAddOnBundleUpdateServiceCfg.cid,
  contractID: sbAddOnBundleUpdateServiceCfg.contractID,
  _xpcom_categories: sbAddOnBundleUpdateServiceCfg.categoryList,

  _cfg: sbAddOnBundleUpdateServiceCfg,
  _isInitialized: false,
  _observerSet: null,
  _prefsAvailable: false,


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

  QueryInterface: XPCOMUtils.generateQI(sbAddOnBundleUpdateServiceCfg.ifList),


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
   * Handle application quit events.
   */

  _handleAppQuit: function sbAddOnBundleUpdateService__handleAppQuit() {
    // Finalize the services.
    this._finalize();
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
    }

    // Wait until preferences are available.
    if (!this._prefsAvailable)
      return;

    // Initialization is now complete.
    this._isInitialized = true;

    // Load and present new add-ons.
    var Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);
    var updateEnabled =
          Application.prefs.getValue("recommended_addons.update.enabled",
                                     this._cfg.defaultUpdateEnabled);
    if (updateEnabled)
      this._loadAndPresentNewAddOns();
  },


  /**
   * Finalize the services.
   */

  _finalize: function sbAddOnBundleUpdateService__finalize() {
    // Remove observers.
    if (this._observerSet) {
      this._observerSet.removeAll();
      this._observerSet = null;
    }
  },


  /**
   * Load and present the new add-ons.
   */

  _loadAndPresentNewAddOns:
    function sbAddOnBundleUpdateService__loadAndPresentNewAddOns() {
    // Trace execution.
    dump("1: loadAndPresentNewAddOns\n");
  }
};


//------------------------------------------------------------------------------
//
// Add-on bundle update service component services.
//
//------------------------------------------------------------------------------

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbAddOnBundleUpdateService]);
}

