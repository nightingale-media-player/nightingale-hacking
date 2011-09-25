/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
 */

/**
 * \file  firstRunAddons.js
 * \brief Javascript source for the first-run wizard add-ons widget services.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// First-run wizard add-ons widget services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// First-run wizard add-ons widget imported services.
//
//------------------------------------------------------------------------------

// Nightingale imports.
Components.utils.import("resource://app/jsmodules/AddOnUtils.jsm");
Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");


//------------------------------------------------------------------------------
//
// First-run wizard add-ons widget services defs.
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


//------------------------------------------------------------------------------
//
// First-run wizard add-ons widget services.
//
//------------------------------------------------------------------------------

/**
 * Construct a first-run wizard add-ons widget services object for the widget
 * specified by aWidget.
 *
 * \param aWidget               First-run wizard add-ons widget.
 */

function firstRunAddOnsSvc(aWidget) {
  this._widget = aWidget;
}

// Define the object.
firstRunAddOnsSvc.prototype = {
  // Set the constructor.
  constructor: firstRunAddOnsSvc,

  //
  // Public widget services fields.
  //
  //   addOnBundle              Add-on bundle object.
  //

  addOnBundle: null,


  //
  // Internal widget services fields.
  //
  //   _widget                  First-run wizard add-ons widget.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _addOnBundleRunning      True if the add-on bundle services are running.
  //   _addOnBundleLoader       Add-on bundle loader object.
  //

  _widget: null,
  _domEventListenerSet: null,
  _addOnBundleRunning: false,
  _addOnBundleLoader: null,


  //----------------------------------------------------------------------------
  //
  // Widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the widget services.
   */

  initialize: function firstRunAddOnsSvc_initialize() {
    var _this = this;
    var func;

    // Get the first-run wizard page element.
    var wizardPageElem = this._widget.parentNode;

    // Create a DOM event listener set.
    this._domEventListenerSet = new DOMEventListenerSet();

    // Initialize the add-on bundle services.
    this._addOnBundleInitialize();

    // Listen for page show events.
    func = function() { return _this._doPageShow(); };
    this._domEventListenerSet.add(wizardPageElem,
                                  "pageshow",
                                  func,
                                  false);

    // Listen for first-run wizard connection reset events.
    func = function() { return _this._doConnectionReset(); };
    this._domEventListenerSet.add(firstRunWizard.wizardElem,
                                  "firstRunConnectionReset",
                                  func,
                                  false);

    // Update the UI.
    this._update();
  },


  /**
   * Finalize the widget services.
   */

  finalize: function firstRunAddOnsSvc_finalize() {
    // Remove DOM event listeners.
    if (this._domEventListenerSet) {
      this._domEventListenerSet.removeAll();
    }
    this._domEventListenerSet = null;

    // Finalize the add-on bundle services.
    this._addOnBundleFinalize();

    // Clear object fields.
    this._widget = null;
    this.addOnBundle = null;
  },


  /**
   * Save the user settings in the first run wizard page.
   */

  saveSettings: function firstRunAddOnsSvc_saveSettings() {
    // Add all add-ons in bundle to recommended add-ons blacklist.
    if (this.addOnBundle) {
      var extensionCount = this.addOnBundle.bundleExtensionCount;
      for (var i = 0; i < extensionCount; i++) {
        // Get the bundle add-on ID.
        var addOnID = this.addOnBundle.getExtensionAttribute(i, "id");

        // Add add-on to blacklist.
        AddOnBundleLoader.addAddOnToBlacklist(addOnID);
      }
    }
  },


  //----------------------------------------------------------------------------
  //
  // Widget event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle a wizard page show event.
   */

  _doPageShow: function firstRunAddOnsSvc__doPageShow() {
    // Start the add-on bundle services.
    this._addOnBundleStart();

    // Update the UI.
    this._update();
  },


  /**
   * Handle a wizard connection reset event.
   */

  _doConnectionReset: function firstRunAddOnsSvc__doConnectionReset() {
    // Do nothing if the add-on bundle successfully loaded.
    if (this.addOnBundle)
      return;

    // Re-initialize the add-on bundle services.
    this._addOnBundleFinalize();
    this._addOnBundleInitialize();

    // Continue the add-on bundle services.  They shouldn't start running until
    // after the first time the first-run add-ons page is shown.
    this._addOnBundleContinue();
  },


  //----------------------------------------------------------------------------
  //
  // Widget add-on bundle services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the add-on bundle services.
   */

  _addOnBundleInitialize:
    function firstRunAddOnSvc__addOnBundleInitialize() {
    // Initialize the add-on bundle fields.
    this.addOnBundle = null;
    this._addOnBundleLoader = null;
  },


  /**
   * Finalize the add-on bundle services.
   */

  _addOnBundleFinalize:
    function firstRunAddOnSvc__addOnBundleFinalize() {
    // Cancel the add-on bundle loader.
    if (this._addOnBundleLoader) {
      this._addOnBundleLoader.cancel();
      this._addOnBundleLoader = null;
    }

    // Clear add-on bundle object fields.
    this.addOnBundle = null;
  },


  /**
   * Start running the add-on bundle services.
   */

  _addOnBundleStart: function firstRunAddOnSvc__addOnBundleStart() {
    // Mark the add-on bundle services running and continue.
    this._addOnBundleRunning = true;
    this._addOnBundleContinue();
  },


  /**
   * Continue running the add-on bundle services.
   */

  _addOnBundleContinue: function firstRunAddOnSvc__addOnBundleContinue() {
    // Do nothing if not running.
    if (!this._addOnBundleRunning)
      return;

    // Get the add-ons.
    this._getAddOns();
  },


  /**
   * Get the list of first-run add-ons.
   */

  _getAddOns: function firstRunAddOnsSvc__getAddOns() {
    // Start loading the add-on bundle.
    if (!this._addOnBundleLoader) {
      // Add all installed add-ons to the blacklist.  This prevents an add-on
      // from being presented if it was previously installed and then
      // uninstalled.
      AddOnBundleLoader.addInstalledAddOnsToBlacklist();

      // Create an add-on bundle loader that filters out installed and
      // blacklisted add-ons.
      this._addOnBundleLoader = new AddOnBundleLoader();
      this._addOnBundleLoader.filterInstalledAddOns = true;
      this._addOnBundleLoader.filterBlacklistedAddOns = true;

      // Start the add-on bundle loader.
      var _this = this;
      var func = function() { _this._getAddOns(); };
      this._addOnBundleLoader.start(func);
    }

    // Get the add-on bundle.
    if (this._addOnBundleLoader.complete &&
        Components.isSuccessCode(this._addOnBundleLoader.result)) {
      // Set add-on bundle property for the first-run add-on bundle widget
      this.addOnBundle = this._addOnBundleLoader.addOnBundle;

      // Set the add-on bundle object for the add-on bundle element.
      var addOnBundleElem = this._getElement("add_on_bundle");
      addOnBundleElem.addOnBundle = this._addOnBundleLoader.addOnBundle;
    }

    // Update the UI.
    this._update();
  },


  //----------------------------------------------------------------------------
  //
  // Internal widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the UI.
   */

  _update: function firstRunAddOnsSvc__update() {
    // If not loading the add-on bundle, select the no status panel.
    var selectedPanel;
    if (!this._addOnBundleLoader) {
      selectedPanel = this._getElement("no_status");
    }
    // Otherwise, if the add-on bundle loading has not completed, select the
    // loading status panel.
    else if (!this._addOnBundleLoader.complete) {
      selectedPanel = this._getElement("add_ons_loading_status");
    }
    // Otherwise, if the add-on bundle loading completed with success, select
    // the add-on bundle panel.
    else if (Components.isSuccessCode(this._addOnBundleLoader.result)) {
      selectedPanel = this._getElement("add_on_bundle");
    }
    // Otherwise, select the load failed status panel.
    else {
      selectedPanel = this._getElement("add_ons_load_failed_status");
    }

    // Select the panel.
    var statusDeckElem = this._getElement("status_deck");
    statusDeckElem.selectedPanel = selectedPanel;

    // Handle any connection errors.
    //XXXeps ideally, we wouldn't handle non-connection errors as connection
    //XXXeps errors.
    if (this._addOnBundleLoader &&
        this._addOnBundleLoader.complete &&
        !Components.isSuccessCode(this._addOnBundleLoader.result)) {
      firstRunWizard.handleConnectionError();
    }
  },


  /**
   * \brief Return the XUL element with the ID specified by aElementID.  Use the
   *        element "anonid" attribute as the ID.
   *
   * \param aElementID          ID of element to get.
   *
   * \return Element.
   */

  _getElement: function firstRunAddOnsSvc__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "anonid",
                                                   aElementID);
  }
}

