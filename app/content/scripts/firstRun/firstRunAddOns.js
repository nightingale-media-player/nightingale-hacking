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

// Songbird imports.
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
// First-run wizard add-ons widget services configuration.
//
//------------------------------------------------------------------------------

/*
 * addOnsBundleDataLoadTimeout  Timeout in milliseconds for loading the add-ons
 *                              bundle data.
 */

var firstRunAddOnsSvcCfg = {
  addOnsBundleDataLoadTimeout: 15000,
  addOnsBundleURLPref: "songbird.url.firstrun"
};


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
  //   addOnsBundle             Add-ons bundle object.
  //

  addOnsBundle: null,


  //
  // Internal widget services fields.
  //
  //   _cfg                     Widget services configuration.
  //   _widget                  First-run wizard add-ons widget.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _addOnsBundleRunning     True if the add-ons bundle services are running.
  //   _addOnsBundleLoading     True if an add-ons bundle is being loaded.
  //   _addOnsBundleDataLoadComplete
  //                            True if loading of add-ons bundle data is
  //                            complete.
  //   _addOnsBundleDataLoadSucceeded
  //                            True if loading of add-ons bundle data
  //                            succeeded.
  //   _addOnsTable             Table of add-ons.
  //

  _cfg: firstRunAddOnsSvcCfg,
  _widget: null,
  _domEventListenerSet: null,
  _addOnsBundleRunning: false,
  _addOnsBundleLoading: false,
  _addOnsBundleDataLoadComplete: false,
  _addOnsBundleDataLoadSucceeded: false,
  _addOnsTable: null,


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

    // Initialize the add-ons bundle services.
    this._addOnsBundleInitialize();

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

    // Finalize the add-ons bundle services.
    this._addOnsBundleFinalize();

    // Clear object fields.
    this._widget = null;
    this.addOnsBundle = null;
    this._addOnsTable = null;
  },


  /**
   * Save the user settings in the first run wizard page.
   */

  saveSettings: function firstRunAddOnsSvc_saveSettings() {
    // Set install flag for each add-on.
    for (var addOnID in this._addOnsTable) {
      // Get the add-on info.
      var addOnInfo = this._addOnsTable[addOnID];
      var index = addOnInfo.index;
      var addOnItemElem = addOnInfo.addOnItemElem;

      // Set the install flag according to the add-on item element setting.
      this.addOnsBundle.setExtensionInstallFlag(index, addOnItemElem.install);
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
    // Start the add-ons bundle services.
    this._addOnsBundleStart();

    // Update the UI.
    this._update();
  },


  /**
   * Handle a wizard connection reset event.
   */

  _doConnectionReset: function firstRunAddOnsSvc__doConnectionReset() {
    // Re-initialize the add-ons bundle services.
    this._addOnsBundleFinalize();
    this._addOnsBundleInitialize();

    // Continue the add-ons bundle services.  They shouldn't start running until
    // after the first time the first-run add-ons page is shown.
    this._addOnsBundleContinue();
  },


  //----------------------------------------------------------------------------
  //
  // sbIBundleDataListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Bundle download completion callback
   * This method is called upon completion of the bundle data download
   * \param bundle An interface to the bundle manager that triggered the event
   * \sa onError, sbIBundle
   */

  onDownloadComplete: function firstRunAddOnsSvc__onDownloadComplete(aBundle) {
    this._addOnsBundleLoading = false;
    this._addOnsBundleDataLoadComplete = true;
    this._addOnsBundleDataLoadSucceeded = true;
    this._getAddOns();
  },


  /**
   * \brief Bundle download error callback
   * This method is called upon error while downloading the bundle data
   * \param bundle An interface to the bundle manager that triggered the event
   * \sa onDownloadComplete, sbIBundle
   */

  onError: function firstRunAddOnsSvc__onError(aBundle) {
    this._addOnsBundleLoading = false;
    this._addOnsBundleDataLoadComplete = true;
    this._addOnsBundleDataLoadSucceeded = false;
    this._getAddOns();
  },


  //----------------------------------------------------------------------------
  //
  // Widget add-ons bundle services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the add-ons bundle services.
   */

  _addOnsBundleInitialize:
    function firstRunAddOnsSvc__addOnsBundleInitialize() {
    // Initialize the add-ons bundle fields.
    this.addOnsBundle = null;
    this._addOnsTable = {};
    this._addOnsBundleLoading = false;
    this._addOnsBundleDataLoadComplete = false;
    this._addOnsBundleDataLoadSucceeded = false;
  },


  /**
   * Finalize the add-ons bundle services.
   */

  _addOnsBundleFinalize:
    function firstRunAddOnsSvc__addOnsBundleFinalize() {
    // Finalize add-ons bundle.
    //XXXeps need way to cancel it
    if (this.addOnsBundle)
      this.addOnsBundle.removeBundleDataListener(this);

    // Reset the add-ons bundle fields.
    this._addOnsBundleLoading = false;
    this._addOnsBundleDataLoadComplete = false;
    this._addOnsBundleDataLoadSucceeded = false;

    // Clear add-ons bundle object fields.
    this.addOnsBundle = null;
    this._addOnsTable = null;
  },


  /**
   * Start running the add-ons bundle services.
   */

  _addOnsBundleStart: function firstRunAddOnsSvc__addOnsBundleStart() {
    // Mark the add-ons bundle services running and continue.
    this._addOnsBundleRunning = true;
    this._addOnsBundleContinue();
  },


  /**
   * Continue running the add-ons bundle services.
   */

  _addOnsBundleContinue: function firstRunAddOnsSvc__addOnsBundleContinue() {
    // Do nothing if not running.
    if (!this._addOnsBundleRunning)
      return;

    // Get the add-ons.
    this._getAddOns();
  },


  /**
   * Get the list of first-run add-ons.
   */

  _getAddOns: function firstRunAddOnsSvc__getAddOns() {
    // Start loading the add-ons bundle data.
    if (!this._addOnsBundleDataLoadComplete && !this._addOnsBundleLoading) {
      // Set up the add-ons bundle for loading.
      this.addOnsBundle = Cc["@songbirdnest.com/Songbird/Bundle;1"]
                            .createInstance(Ci.sbIBundle);
      this.addOnsBundle.bundleId = "firstrun";
      this.addOnsBundle.bundleURL = Application.prefs.getValue
                                      (this._cfg.addOnsBundleURLPref,
                                       "default");
      this.addOnsBundle.addBundleDataListener(this);

      // Start loading the add-ons bundle data.
      try {
        this.addOnsBundle.retrieveBundleData
                            (this._cfg.addOnsBundleDataLoadTimeout);
        this._addOnsBundleLoading = true;
      } catch (ex) {
        // Report the exception as an error.
        Components.utils.reportError(ex);

        // Indicate that the add-ons bundle loading failed.
        this._addOnsBundleLoading = false;
        this._addOnsBundleDataLoadComplete = true;
        this._addOnsBundleDataLoadSucceeded = false;
      }
    }

    // Add loaded add-ons.
    if (this._addOnsBundleDataLoadComplete &&
        this._addOnsBundleDataLoadSucceeded) {
      var extensionCount = this.addOnsBundle.bundleExtensionCount;
      for (var i = 0; i < extensionCount; i++) {
        this._addAddOn(i);
      }
    }

    // Update the UI.
    this._update();
  },


  /**
   * Add the add-on from the add-ons bundle with the index specified by aIndex.
   *
   * \param aIndex              Index within the add-ons bundle of the add-on to
   *                            add.
   */

  _addAddOn: function firstRunAddOnsSvc__addAddOn(aIndex) {
    // Get the add-on ID.  Use the name as an ID if the ID is not available.
    var addOnID = this.addOnsBundle.getExtensionAttribute(aIndex, "id");
    if (!addOnID)
      addOnID = this.addOnsBundle.getExtensionAttribute(aIndex, "name");

    // Do nothing if add-on already added.
    if (this._addOnsTable[addOnID])
      return;

    // Get the add-on info.
    var addOnInfo = {};
    addOnInfo.index = aIndex;
    addOnInfo.installFlag = this.addOnsBundle.getExtensionInstallFlag(aIndex);
    addOnInfo.addOnID = this.addOnsBundle.getExtensionAttribute(aIndex, "id");
    addOnInfo.addOnURL = this.addOnsBundle.getExtensionAttribute(aIndex,
                                                                  "url");
    addOnInfo.name = this.addOnsBundle.getExtensionAttribute(aIndex, "name");
    addOnInfo.description =
          this.addOnsBundle.getExtensionAttribute(aIndex, "description");
    addOnInfo.iconURL = this.addOnsBundle.getExtensionAttribute(aIndex, "icon");

    // Add the add-on element to the add-on list element.
    addOnInfo.addOnItemElem = this._addAddOnElement(addOnInfo);

    // Add the add-on info to the add-ons table.
    this._addOnsTable[addOnID] = addOnInfo;
  },


  /**
   * Add the add-on element to the add-on list element for the add-on specified
   * by aAddOnInfo.  Return the added add-on element.
   *
   * \param aAddOnInfo          Add-on information.
   *
   * \return                    Add-on item element.
   */

  _addAddOnElement: function firstRunAddOnsSvc__addAddOnElement(aAddOnInfo) {
    // Get the add-on list item template.
    var listItemTemplateElem = this._getElement("list_item_template");

    // Create an add-on list item from the template.
    var listItemElem = listItemTemplateElem.cloneNode(true);
    listItemElem.hidden = false;

    // Set up the add-on item element.
    var itemElem = DOMUtils.getElementsByAttribute(listItemElem,
                                                   "templateid",
                                                   "item")[0];
    itemElem.setAttribute("defaultinstall", aAddOnInfo.installFlag);
    itemElem.setAttribute("addonid", aAddOnInfo.addOnID);
    itemElem.setAttribute("addonurl", aAddOnInfo.addOnURL);
    itemElem.setAttribute("name", aAddOnInfo.name);
    itemElem.setAttribute("description", aAddOnInfo.description);
    itemElem.setAttribute("icon", aAddOnInfo.iconURL);

    // Reclone element to force construction with new settings.
    listItemElem = listItemElem.cloneNode(true);

    // Add the add-on list item to the add-on list.
    var itemListElem = this._getElement("add_ons_list");
    itemListElem.appendChild(listItemElem);

    // Get the add-on item element after it's been appended to get a fully
    // functional element object.
    itemElem = DOMUtils.getElementsByAttribute(listItemElem,
                                               "templateid",
                                               "item")[0];

    return itemElem;
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
    // Determine the panel to select in the status deck.  Default to the no
    // status panel.
    var selectedPanel = this._getElement("no_status");

    // If loading the add-ons bundle, select the loading status panel.
    if (this._addOnsBundleLoading) {
      selectedPanel = this._getElement("add_ons_loading_status");
    }
    // Otherwise, if the add-ons bundle loading completed with success, select
    // the add-ons list panel.
    else if (this._addOnsBundleDataLoadComplete &&
             this._addOnsBundleDataLoadSucceeded) {
      selectedPanel = this._getElement("add_ons_list");
    }
    // Otherwise, if the add-ons bundle loading completed with failure, select
    // the load failed status panel.
    else if (this._addOnsBundleDataLoadComplete &&
             !this._addOnsBundleDataLoadSucceeded) {
      selectedPanel = this._getElement("add_ons_load_failed_status");
    }

    // Select the panel.
    var statusDeckElem = this._getElement("status_deck");
    statusDeckElem.selectedPanel = selectedPanel;

    // Handle any connection errors.
    //XXXeps ideally, we wouldn't handle non-connection errors as connection
    //XXXeps errors.
    if (this._addOnsBundleDataLoadComplete &&
        !this._addOnsBundleDataLoadSucceeded) {
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

