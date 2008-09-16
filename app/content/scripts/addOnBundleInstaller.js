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
 * \file  addOnBundleInstaller.js
 * \brief Javascript source for the add-on bundle installer widget services.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Add-on bundle installer widget services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Add-on bundle installer widget services defs.
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
// Add-on bundle installer widget services.
//
//------------------------------------------------------------------------------

/**
 * Construct an add-on bundle installer widget services object for the widget
 * specified by aWidget.
 *
 * \param aWidget               Add-on bundle installer widget.
 */

function addOnBundleInstallerSvc(aWidget) {
  this._widget = aWidget;
}

// Define the object.
addOnBundleInstallerSvc.prototype = {
  // Set the constructor.
  constructor: addOnBundleInstallerSvc,

  //
  // Public widget services fields.
  //
  //   restartRequired          True if add-on installation requires a restart.
  //

  restartRequired: false,


  //
  // Internal widget services fields.
  //
  //   _widget                  First-run wizard add-ons widget.
  //   _addOnBundle             Add-on bundle object.
  //   _addOnsInstallIndexList  List of add-ons to install.
  //   _nextAddOnToInstall      Index within _addOnsInstallIndexList of next
  //                            add-on to install.
  //   _addOnFileDownloader     File downloader for add-on.
  //   _addOnFileList           List of downloaded add-ons.
  //

  _widget: null,
  _addOnBundle: null,
  _addOnsInstallIndexList: null,
  _nextAddOnToInstall: 0,
  _addOnFileDownloader: null,
  _addOnFileList: null,


  //----------------------------------------------------------------------------
  //
  // Widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the widget services.
   */

  initialize: function addOnBundleInstallerSvc_initialize() {
  },


  /**
   * Finalize the widget services.
   */

  finalize: function addOnBundleInstallerSvc_finalize() {
    // Clear object fields.
    this._widget = null;
    this._addOnBundle = null;
    this._addOnFileList = null;
  },


  /**
   * Start installing the add-on bundle specified by aAddOnBundle.
   *
   * \param aAddOnBundle        Add-on bundle to install.
   */

  install: function addOnBundleInstallerSvc_install(aAddOnBundle) {
    // Set the add-on bundle to install.
    this._addOnBundle = aAddOnBundle;

    // Initialize add-on file list.
    this._addOnFileList = [];

    // Get the list of add-ons to install.
    this._addOnsInstallIndexList = [];
    var extensionCount = this._addOnBundle.bundleExtensionCount;
    for (var i = 0; i < extensionCount; i++) {
      if (this._addOnBundle.getExtensionInstallFlag(i))
        this._addOnsInstallIndexList.push(i);
    }

    // Start downloading the add-ons.
    this._downloadNextAddOnStart();
  },


  /**
   * Cancel the add-on bundle installation.
   */

  cancel: function addOnBundleInstallerSvc_cancel() {
    if (this._addOnFileDownloader) {
      this._addOnFileDownloader.listener = null;
      this._addOnFileDownloader.cancel();
      this._addOnFileDownloader = null;
    }
  },


  //----------------------------------------------------------------------------
  //
  // Widget nsIFileDownloaderListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Called when progress is made on file download.
   */

  onProgress: function addOnBundleInstallerSvc_onProgress() {
    // Update the UI.
    this._update();
  },


  /**
   * \brief Called when download has completed.
   */

  onComplete: function addOnBundleInstallerSvc_onComplete() {
    // Update the UI.
    this._update();

    // Complete downloading of next add-on.
    this._downloadNextAddOnComplete();
  },


  //----------------------------------------------------------------------------
  //
  // Internal widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the UI.
   */

  _update: function addOnBundleInstallerSvc__update() {
    // Get the current add-on index.
    var currentAddOnIndex =
          this._addOnsInstallIndexList[this._nextAddOnToInstall];

    // Set the current add-on name.
    var currentAddOnLabelElem = this._getElement("current_add_on_label");
    currentAddOnLabelElem.value =
      this._addOnBundle.getExtensionAttribute(currentAddOnIndex, "name");

    // Set the total progress label.
    var totalProgressLabelElem = this._getElement("total_progress_label");
    totalProgressLabelElem.value =
      SBFormattedString("add_on_bundle_installer.progress_all",
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
   * Start downloading the next addon.
   */

  _downloadNextAddOnStart:
    function addOnBundleInstallerSvc__downloadNextAddOnStart() {
    // If all add-ons have been downloaded, install them all.
    if (this._nextAddOnToInstall == this._addOnsInstallIndexList.length) {
      this._installAllAddOns();
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
      this._addOnBundle.getExtensionAttribute(nextAddOnIndex, "url");
    this._addOnFileDownloader.destinationFileExtension = "xpi";

    // Start downloading add-on.
    this._addOnFileDownloader.start();

    // Update the UI.
    this._update();
  },


  /**
   * Complete donwloading of the next addon.
   */

  _downloadNextAddOnComplete:
    function addOnBundleInstallerSvc__downloadNextAddOnComplete() {
    // Add downloaded file to the add-on file list.
    if (this._addOnFileDownloader.succeeded)
        this._addOnFileList.push(this._addOnFileDownloader.destinationFile);
    this._addOnFileDownloader.listener = null;
    this._addOnFileDownloader = null;

    // Start download of the next add-on.
    this._nextAddOnToInstall++;
    this._downloadNextAddOnStart();
  },


  /**
   * Install all downloaded add-ons.
   */

  _installAllAddOns: function addOnBundleInstallerSvc__installAddOns() {
    // Install all of the add-ons.
    for (var i = 0; i < this._addOnFileList.length; i++) {
      this._installAddOnFile(this._addOnFileList[i]);
    }

    // Post an install completion event.
    var event = document.createEvent("Events");
    event.initEvent("complete", true, true);
    this._widget.dispatchEvent(event);
  },


  /**
   * Install the add-on from the file specified by aAddOnFile.
   *
   * \param aAddOnFile          Add-on file to install.
   */

  _installAddOnFile:
    function addOnBundleInstallerSvc__installAddOnFile(aAddOnFile) {
    // Get the extension manager.
    var extensionManager = Cc["@mozilla.org/extensions/manager;1"]
                             .getService(Ci.nsIExtensionManager);

    // Install the add-on.
    try {
      extensionManager.installItemFromFile(aAddOnFile, "app-profile");
      this.restartRequired = true;
    } catch (ex) {
      Cu.reportError("Error installing add-on: " + ex);
    }

    // Remove the add-on file.
    aAddOnFile.remove(false);
  },


  /**
   * \brief Return the XUL element with the ID specified by aElementID.  Use the
   *        element "anonid" attribute as the ID.
   *
   * \param aElementID          ID of element to get.
   *
   * \return Element.
   */

  _getElement: function addOnBundleInstallerSvc__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "anonid",
                                                   aElementID);
  }
}

