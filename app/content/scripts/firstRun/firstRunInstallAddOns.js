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
 * \file  firstRunInstallAddOns.js
 * \brief Javascript source for the first-run wizard install add-ons widget
 *        services.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// First-run wizard install add-ons widget services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// First-run wizard install add-ons widget imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");


//------------------------------------------------------------------------------
//
// First-run wizard install add-ons widget services defs.
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
// First-run wizard install add-ons widget services.
//
//------------------------------------------------------------------------------

/**
 * Construct a first-run wizard install add-ons widget services object for the
 * widget specified by aWidget.
 *
 * \param aWidget               First-run wizard install add-ons widget.
 */

function firstRunInstallAddOnsSvc(aWidget) {
  this._widget = aWidget;
}

// Define the object.
firstRunInstallAddOnsSvc.prototype = {
  // Set the constructor.
  constructor: firstRunInstallAddOnsSvc,

  //
  // Widget services fields.
  //
  //   _widget                  First-run wizard install add-ons widget.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _wizardElem              First-run wizard element.
  //   _addOnsBundle            Add-ons bundle object.
  //   _addOnsInstallIndexList  List of add-ons to install.
  //   _nextAddOnToInstall      Index within _addOnsInstallIndexList of next
  //                            add-on to install.
  //   _addOnFileDownloader     File downloader for add-on.
  //

  _widget: null,
  _domEventListenerSet: null,
  _wizardElem: null,
  _addOnsBundle: null,
  _addOnsInstallIndexList: null,
  _nextAddOnToInstall: 0,
  _addOnFileDownloader: null,


  //----------------------------------------------------------------------------
  //
  // Widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the widget services.
   */

  initialize: function firstRunInstallAddOnsSvc_initialize() {
    var _this = this;
    var func;

    // Initialize the list of add-ons to install.
    this._addOnsInstallIndexList = [];

    // Create a DOM event listener set.
    this._domEventListenerSet = new DOMEventListenerSet();

    // Get the first-run wizard and wizard page elements.
    var wizardPageElem = this._widget.parentNode;
    this._wizardElem = wizardPageElem.parentNode;

    // Listen for page show and hide events.
    var func = function() { _this._doPageShow(); };
    this._domEventListenerSet.add(wizardPageElem,
                                  "pageshow",
                                  func,
                                  false);
    var func = function() { _this._doPageHide(); };
    this._domEventListenerSet.add(wizardPageElem,
                                  "pagehide",
                                  func,
                                  false);
  },


  /**
   * Finalize the widget services.
   */

  finalize: function firstRunInstallAddOnsSvc_finalize() {
    // Remove DOM event listeners.
    if (this._domEventListenerSet) {
      this._domEventListenerSet.removeAll();
    }
    this._domEventListenerSet = null;

    // Cancel any add-on download in progress.
    this._cancelAddOnDownload();

    // Clear object fields.
    this._widget = null;
    this._wizardElem = null;
    this._addOnsBundle = null;
    this._addOnsInstallIndexList = null;
  },


  //----------------------------------------------------------------------------
  //
  // Widget event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle a wizard page show event.
   */

  _doPageShow: function firstRunInstallAddOnsSvc__doPageShow() {
    // Get the add-ons bundle object.
    var addOnsID = this._widget.getAttribute("addonsid");
    var addOnsBundleProperty =
          this._widget.getAttribute("addonsbundleproperty");
    var addOnsElem = document.getElementById(addOnsID);
    this._addOnsBundle = addOnsElem[addOnsBundleProperty];

    // Get the list of add-ons to install.
    var extensionCount = this._addOnsBundle.bundleExtensionCount;
    for (var i = 0; i < extensionCount; i++) {
      if (this._addOnsBundle.getExtensionInstallFlag(i))
        this._addOnsInstallIndexList.push(i);
    }

    // Start installing the add-ons.
    this._installNextAddOnStart();
  },


  /**
   * Handle a wizard page hide event.
   */

  _doPageHide: function firstRunInstallAddOnsSvc__doPageHide() {
    // Cancel any add-on download in progress.
    this._cancelAddOnDownload();
  },


  //----------------------------------------------------------------------------
  //
  // Widget nsIFileDownloaderListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Called when progress is made on file download.
   */

  onProgress: function firstRunInstallAddOnsSvc_onProgress() {
    // Update the UI.
    this._update();
  },


  /**
   * \brief Called when download has completed.
   */

  onComplete: function firstRunInstallAddOnsSvc_onComplete() {
    // Complete installation of next add-on.
    this._installNextAddOnComplete();
  },


  //----------------------------------------------------------------------------
  //
  // Internal widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the UI.
   */

  _update: function firstRunInstallAddOnsSvc__update() {
    // Get the current add-on index.
    var currentAddOnIndex =
          this._addOnsInstallIndexList[this._nextAddOnToInstall];

    // Set the current add-on name.
    var currentAddOnLabelElem = this._getElement("current_add_on_label");
    currentAddOnLabelElem.value =
      this._addOnsBundle.getExtensionAttribute(currentAddOnIndex, "name");

    // Set the total progress label.
    var totalProgressLabelElem = this._getElement("total_progress_label");
    totalProgressLabelElem.value =
      SBFormattedString("first_run.install_add_ons.progress_all",
                        [ this._nextAddOnToInstall + 1,
                          this._addOnsInstallIndexList.length ]);

    // Set the current add-on download progress meter.
    var progressMeterElem = this._getElement("progressmeter");
    if (this._addOnFileDownloader)
      progressMeterElem.value = this._addOnFileDownloader.percentComplete;
    else
      progressMeterElem.value = 0;
  },


  /**
   * Start installation of the next addon.
   */

  _installNextAddOnStart:
    function firstRunInstallAddOnsSvc__installNextAddOnStart() {
    // Advance wizard when all add-on installations are complete.
    if (this._nextAddOnToInstall == this._addOnsInstallIndexList.length) {
      this._wizardElem.canAdvance = true;
      this._wizardElem.advance();
      return;
    }

    // Get the index of the next add-on to install.
    var nextAddOnIndex =
          this._addOnsInstallIndexList[this._nextAddOnToInstall];

    // Create an add-on file downloader.
    this._addOnFileDownloader =
           Cc["@songbirdnest.com/Songbird/FileDownloader;1"]
             .createInstance(Ci.sbIFileDownloader);
    this._addOnFileDownloader.listener = this;
    this._addOnFileDownloader.sourceURISpec =
      this._addOnsBundle.getExtensionAttribute(nextAddOnIndex, "url");
    this._addOnFileDownloader.destinationFileExtension = "xpi";

    // Start downloading add-on.
    this._addOnFileDownloader.start();

    // Update the UI.
    this._update();
  },


  /**
   * Complete installation of the next addon.
   */

  _installNextAddOnComplete:
    function firstRunInstallAddOnSvc__installNextAddOnComplete() {
    // Get the add-on file.
    var addOnFile = null;
    if (this._addOnFileDownloader.succeeded)
      addOnFile = this._addOnFileDownloader.destinationFile;

    // Install the add-on.
    var addOnInstalled = false;
    if (addOnFile) {
      // Get the extension manager.
      var extensionManager = Cc["@mozilla.org/extensions/manager;1"]
                               .getService(Ci.nsIExtensionManager);

      // Install the add-on.
      try {
        extensionManager.installItemFromFile(addOnFile, "app-profile");
        addOnInstalled = true;
      } catch (ex) {
        Cu.reportError("Error installing add-on: " + ex);
      }
    } else {
      Cu.reportError("Failed to download add-on file.");
    }

    // Mark first-run wizard for application restart.
    if (addOnInstalled)
      this._wizardElem.setAttribute("restartapp", "true");

    // Remove the add-on file and release the add-on file downloader.
    if (addOnFile)
      addOnFile.remove(false);
    this._addOnFileDownloader.listener = null;
    this._addOnFileDownloader = null;

    // Start installation of the next add-on.
    this._nextAddOnToInstall++;
    this._installNextAddOnStart();
  },


  /**
   * Cancel any add-on download in progress.
   */

  _cancelAddOnDownload:
    function firstRunInstallAddOnsSvc__cancelAddOnDownload() {
    if (this._addOnFileDownloader) {
      this._addOnFileDownloader.listener = null;
      this._addOnFileDownloader.cancel();
      this._addOnFileDownloader = null;
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

  _getElement: function firstRunInstallAddOnsSvc__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "anonid",
                                                   aElementID);
  }
}

