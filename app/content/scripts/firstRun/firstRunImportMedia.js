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
 * \file  firstRunImportMedia.js
 * \brief Javascript source for the first-run wizard import media widget
 *        services.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// First-run wizard import media widget services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// First-run wizard import media widget imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/FolderUtils.jsm");


//------------------------------------------------------------------------------
//
// First-run wizard import media widget services defs.
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
// First-run wizard import media widget services.
//
//------------------------------------------------------------------------------

/**
 * Construct a first-run wizard import media widget services object for the
 * widget specified by aWidget.
 *
 * \param aWidget               First-run wizard import media widget.
 */

function firstRunImportMediaSvc(aWidget) {
  this._widget = aWidget;
}

// Define the object.
firstRunImportMediaSvc.prototype = {
  // Set the constructor.
  constructor: firstRunImportMediaSvc,

  //
  // Widget services fields.
  //
  //   _widget                  First-run wizard import media widget.
  //   _watchFolderAvailable    True if watch folder services are available.
  //

  _widget: null,
  _watchFolderAvailable: false,


  //----------------------------------------------------------------------------
  //
  // Widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the widget services.
   */

  initialize: function firstRunImportMediaSvc_initialize() {
    // Select the default scan directory.
    this._selectDefaultScanDirectory();

    // Determine whether the watch folder services are available.
    var watchFolderService = Cc["@songbirdnest.com/watch-folder-service;1"]
                               .getService(Ci.sbIWatchFolderService);
    this._watchFolderAvailable = watchFolderService.isSupported;

    // Update the UI.
    this._update();
  },


  /**
   * Finalize the widget services.
   */

  finalize: function firstRunImportMediaSvc_finalize() {
    // Clear object fields.
    this._widget = null;
  },


  /**
   * Save the user settings in the first run wizard page.
   */

  saveSettings: function firstRunImportMediaSvc_saveSettings() {
    // Dispatch processing of the import settings radio group.
    var importRadioGroupElem = this._getElement("import_radiogroup");
    var metricsImportType;
    switch (importRadioGroupElem.value) {
      case "scan_directories" :
        this._saveScanDirectoriesSettings();
        metricsImportType = "filescan";
        break;

      default :
        metricsImportType = "none";
        break;
    }

#ifdef METRICS_ENABLED
    var metrics = Cc["@songbirdnest.com/Songbird/Metrics;1"]
                    .createInstance(Ci.sbIMetrics);
    metrics.metricsInc("firstrun", "mediaimport", metricsImportType);
#endif
  },

  //----------------------------------------------------------------------------
  //
  // Widget event handler services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the command event specified by aEvent.
   *
   * \param aEvent              Command event.
   */

  doCommand: function firstRunImportMediaSvc_doCommand(aEvent) {
    // Dispatch processing of command.
    var action = aEvent.target.getAttribute("action");
    switch (action) {
      case "browse_scan_directory" :
        this._doBrowseScanDirectory();
        break;

      default :
        break;
    }

    // Update the UI.
    this._update();
  },


  /**
   * Browse for the scan directory.
   */

  _doBrowseScanDirectory: function
                            firstRunImportMediaSvc__browseScanDirectory() {
    // Get the currently selected scan directory.
    var scanDirectoryTextBox = this._getElement("scan_directory_textbox");
    var scanPath = scanDirectoryTextBox.value;
    var scanDir = null;
    if (scanPath) {
      scanDir = Cc["@mozilla.org/file/local;1"]
                  .createInstance(Ci.nsILocalFile);
      scanDir.initWithPath(scanPath);
    }

    // Set up a file picker for browsing.
    var filePicker = Cc["@mozilla.org/filepicker;1"]
                       .createInstance(Ci.nsIFilePicker);
    filePicker.init(window,
                    SBString("first_run.media_scan_browser.title"),
                    Ci.nsIFilePicker.modeGetFolder);
    if (scanDir && scanDir.exists())
      filePicker.displayDirectory = scanDir;

    // Show the file picker.
    var result = filePicker.show();

    // Update the scan directory path.
    if (result == Ci.nsIFilePicker.returnOK)
      scanDirectoryTextBox.value = filePicker.file.path;
  },


  //----------------------------------------------------------------------------
  //
  // Internal widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the UI.
   */

  _update: function firstRunImportMediaSvc__update() {
    // Get the import radio group element.
    var importRadioGroupElem = this._getElement("import_radiogroup");

    // Check which import options should be enabled.
    this._checkImportEnabled();

    // Select the default radio if none selected.
    if (importRadioGroupElem.selectedIndex < 0)
      this._selectDefaultImport();

    // Get the import radio group element and the selected radio.
    var selectedRadioElem = importRadioGroupElem.selectedItem;
    var selectedRadioElemID = selectedRadioElem.getAttribute("anonid");

    // Get all radio group child elements with a "parentradio" attribute.
    var radioChildList = DOMUtils.getElementsByAttribute(importRadioGroupElem,
                                                         "parentradio");

    // Disable all elements that are children of unselected radio elements and
    // enable all other radio children.
    for (var i = 0; i < radioChildList.length; i++) {
      // If child's radio is selected, enable the child; otherwise, disable it.
      var child = radioChildList[i];
      if (child.getAttribute("parentradio") == selectedRadioElemID)
        child.removeAttribute("disabled");
      else
        child.setAttribute("disabled", "true");
    }
  },


  /**
   * Check which import options should be enabled.
   */

  _checkImportEnabled: function firstRunImportMediaSvc__checkImportEnabled() {
    // If the watch folder services are not available, hide the watch folder
    // options.
    if (!this._watchFolderAvailable) {
      var scanDirectoriesWatchCheckBox =
            this._getElement("scan_directories_watch_checkbox");
      scanDirectoriesWatchCheckBox.hidden = true;
    }
  },


  /**
   * Save the scan directories user settings.
   */

  _saveScanDirectoriesSettings:
    function firstRunImportMediaSvc__savScanDirectoriesSettings() {
    // Get the scan directories settings.
    var scanDirectoryTextBox = this._getElement("scan_directory_textbox");
    var scanDirectoryPath = scanDirectoryTextBox.value;

    // Save the scan directories settings.
    Application.prefs.setValue("songbird.firstrun.scan_directory_path",
                               scanDirectoryPath);
    Application.prefs.setValue("songbird.firstrun.do_scan_directory", true);

    // If the watch folder services are available, save the watch folder
    // settings.
    if (this._watchFolderAvailable) {
      // Get the watch folder settings.
      var scanDirectoriesWatchCheckBox =
            this._getElement("scan_directories_watch_checkbox");
      var enableWatchFolder = scanDirectoriesWatchCheckBox.checked;

      // Save the watch folder settings.
      Application.prefs.setValue("songbird.watch_folder.enable",
                                 enableWatchFolder);
      if (enableWatchFolder) {
        Application.prefs.setValue("songbird.watch_folder.path",
                                   scanDirectoryPath);
      }
    }
  },


  /**
   * Select the default import option.
   */

  _selectDefaultImport: function firstRunImportMediaSvc__selectDefaultImport() {
    // Get the import radio group element.
    var importRadioGroupElem = this._getElement("import_radiogroup");

    // Select the scan directories import option.
    var scanDirectoriesElem = this._getElement("scan_directories_radio");
    importRadioGroupElem.selectedItem = scanDirectoriesElem;
  },


  /**
   * Select the default scan directory.
   */

  _selectDefaultScanDirectory:
    function firstRunImportMediaSvc__selectDefaultScanDirectory() {
    // Set the scan directory textbox value.
    var scanDirectoryTextBox = this._getElement("scan_directory_textbox");
    scanDirectoryTextBox.value = FolderUtils.musicFolder.path;
  },


  /**
   * \brief Return the XUL element with the ID specified by aElementID.  Use the
   *        element "anonid" attribute as the ID.
   *
   * \param aElementID          ID of element to get.
   *
   * \return Element.
   */

  _getElement: function firstRunImportMediaSvc__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "anonid",
                                                   aElementID);
  }
};

