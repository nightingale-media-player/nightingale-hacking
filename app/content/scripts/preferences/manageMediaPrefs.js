/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * \file  manageMediaPref.js
 * \brief JavaScript source for the media management preferences.
 */

//------------------------------------------------------------------------------
//
// imported JavaScript modules
//
//------------------------------------------------------------------------------

Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm"); 
Components.utils.import("resource://app/jsmodules/SBJobUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");
Components.utils.import("resource://app/jsmodules/WindowUtils.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

//------------------------------------------------------------------------------
//
// Global constants
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

const MMPREF_WRITE_TEST_FILE = "test.txt";
const PERMS_FILE = 0644;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Manage media preference pane services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

var manageMediaPrefsPane = {
  //----------------------------------------------------------------------------
  //
  // Event handling services.
  //
  //----------------------------------------------------------------------------

  _prefs: [], // Original values for the preferences

  /**
   * Handles the preference pane load event.
   */

  doPaneLoad: function manageMediaPrefsPane_doPaneLoad() {
    var self = this;
    function forceCheck(event) {
      const instantApply =
        Application.prefs.getValue("browser.preferences.instantApply", true);
      if (event.type == "dialogcancel" && !instantApply) {
        // cancel dialog, don't do anything
        return true;
      }

      if (!self._checkForValidPref(false)) {
        self._updateUI();
        event.preventDefault();
        return false;
      }

      // dialog is closing, apply any preferences that should never instant-apply
      var mediaMgmtSvc = Cc["@songbirdnest.com/Songbird/media-manager-service;1"]
                           .getService(Ci.sbIMediaManagementService);
      var enablePrefElem =
        document.getElementById("manage_media_pref_library_enable");

      var startMgtProcess = self._doUpdateCheck(mediaMgmtSvc.isEnabled, enablePrefElem.value);
      mediaMgmtSvc.isEnabled = enablePrefElem.value;

      if (startMgtProcess) {
        // Disable then enable the mediaMgmtSvc to force a scan
        mediaMgmtSvc.isEnabled = false;
        mediaMgmtSvc.isEnabled = true;
      }

      return true;
    }
    window.addEventListener('dialogaccept', forceCheck, false);
    window.addEventListener('dialogcancel', forceCheck, false);
    
    document.getElementById("manage_media_pref_library_folder")
            .addEventListener("change", this, false);
    
    // fake a change event to update some display
    var event = document.createEvent("Events");
    event.initEvent("change", true, true);
    document.getElementById("manage_media_pref_library_folder")
            .dispatchEvent(event);

    this._checkForValidPref(true);
    this._updateUI();
    this._saveCurrentPrefs();
  },

  /**
   * Handle the Preview command event
   */
  doDisplayPreview: function() {
    if (!this._checkForValidPref(false)) {
      return false;
    }

    var dirFormat = document.getElementById("manage_media_format_dir_formatter")
                            .value;
    var fileFormat = document.getElementById("manage_media_format_file_formatter")
                             .value;

    // show the preview
    WindowUtils.openModalDialog(window,
                                "chrome://songbird/content/xul/manageMediaPreview.xul",
                                "manage_media_preview_dialog",
                                "chrome,centerscreen",
                                [LibraryUtils.mainLibrary,
                                 this._libraryFolder,
                                 fileFormat,
                                 dirFormat],
                                null);
  },

  /**
   * Handle the browse command event specified by aEvent.
   *
   * \param aEvent              Browse command event.
   */

  doBrowseCommand: function manageMediaPrefsPane_doBrowseCommand(aEvent) {
    const title = SBString("prefs.media_management.browse.title");
    var folderPicker = Cc["@mozilla.org/filepicker;1"]
                         .createInstance(Ci.nsIFilePicker);
    folderPicker.init(window, title, Ci.nsIFilePicker.modeGetFolder);
    folderPicker.displayDirectory = this._libraryFolder;

    if (folderPicker.show() == Ci.nsIFilePicker.returnOK) {
      var selectedFolder = folderPicker.file;
      if (false) {
        selectedFolder = null; // use default
      }
      this._libraryFolder = selectedFolder;
    }
  },
  
  /**
   * Handle the toggle management mode on/off command
   */
  doEnableCommand: function manageMediaPrefsPane_doEnableCommand(aEvent) {
    var prefElem = document.getElementById(aEvent.target.getAttribute("preference"));
    if (prefElem) {
      prefElem.value = !prefElem.value;
    }
    this._updateUI();
  },
  
  /**
   * Handle DOM events (mostly, pref change)
   */
  handleEvent: function manageMediaPrefsPane_handleEvent(aEvent) {
    switch (String(aEvent.type)) {
      case "change":
        switch (aEvent.target.id) {
          case "manage_media_pref_library_folder":
            let prefElem = aEvent.target;
            let fileField = document.getElementById("manage_media_library_file");
            fileField.file = prefElem.value || manageMediaPrefsPane._libraryFolder;
            // we want to use the full path, unless that ends up being empty
            fileField.label = fileField.file.path;
            break;
        }
        break;
    }
  },

  //----------------------------------------------------------------------------
  //
  // Internal services.
  //
  //----------------------------------------------------------------------------
  
  /**
   * The library folder to dump things in
   */

  get _defaultLibraryFolder() {
    var baseFile = Cc["@songbirdnest.com/Songbird/DownloadDeviceHelper;1"]
                     .getService(Ci.sbIDownloadDeviceHelper)
                     .getDefaultMusicFolder();
    var file = baseFile.clone();
    file.append(SBBrandedString("mediamanager.music_dir",
                                SBStringBrandShortName()));
    if (!file.exists()) {
      // On Mac we have to have a valid folder in order to display it.
      try {
        var permissions = baseFile.permissions;
        file.create(Ci.nsIFile.DIRECTORY_TYPE, permissions);
      } catch (e) {
        Cu.reportError("Unable to create folder: " + file.path +
                       ", defaulting to " + baseFile.path +
                       " - Error: " + e);
        file = baseFile;
      }
    }
    return file;
  },

  get _libraryFolder() {
    try {
      var prefValue = document.getElementById("manage_media_pref_library_folder")
                              .value;
      if (prefValue) {
        return prefValue;
      }
    } catch (ex) {
      /* invalid file? boo. use default. */
    }
    document.getElementById("manage_media_pref_library_folder").value =
      this._defaultLibraryFolder;
    return this._defaultLibraryFolder;
  },
  
  set _libraryFolder(aValue) {
    if (!aValue || !(aValue instanceof Ci.nsIFile)) {
      aValue = null;
    }
    if (aValue) {
      document.getElementById("manage_media_pref_library_folder").value = aValue;
    } else {
      // set to default folder
      aValue = this._defaultLibraryFolder;
      document.getElementById("manage_media_pref_library_folder").value = aValue;
    }
  },
 
  /**
   * Shows a notification message after removing any other ones of the same class.
   */
  _showErrorNotification: function(aMsg, aLevel) {
    // focus this pref pane and this tab
    var pane = document.getElementById("paneManageMedia");
    document.documentElement.showPane(pane);
    
    var notifBox = document.getElementById("media_manage_notification_box");

    // show the notification, hiding any other ones of this class
    var oldNotif;
    while ((oldNotif = notifBox.getNotificationWithValue("media_manage_error"))) {
      notifBox.removeNotification(oldNotif);
    }
    var level = aLevel || "PRIORITY_CRITICAL_LOW";
    notifBox.appendNotification(aMsg,
                                "media_manage_error",
                                null,
                                notifBox[level],
                                []);
  },

  /**
   * Check if the current value of the preference is valid; pops up a
   * notification if it is not.
   * @return true if valid, false if not.
   */
  _checkForValidPref: function manageMediaPrefsPane__checkForValidPref(aSilent) {
    var self = this;
    function showErrorNotification(aMsg) {
      if (aSilent) {
        return;
      }
      self._showErrorNotification(aMsg); 
    }

    // need to manually set the dir pref if it's not user set, because it has
    // no default value (needs platform-specific path separator)
    var dirPrefElem = document.getElementById("manage_media_pref_library_format_dir");
    var dirFormatElem = document.getElementById("manage_media_format_dir_formatter");
    if (!dirPrefElem.hasUserValue) {
      dirPrefElem.valueFromPreferences = dirFormatElem.value;
    }

    var button = document.getElementById("manage_media_global_cmd");
    var prefElem = document.getElementById(button.getAttribute("preference"));
    var enabled = prefElem.value;

    if (enabled) {
      // Need to check if the user chose a usable folder for the managed folder.
      var managedFolder = document.getElementById("manage_media_library_file");
      if (!managedFolder || !managedFolder.file) {
        showErrorNotification(SBString("prefs.media_management.error.no_path"));
        return false;
      }
      var file = managedFolder.file;
      var missingDefault = false;
      if (!file.exists()) {
        if (file != this._defaultLibraryFolder) {
          missingDefault = true;
        } else {
          showErrorNotification(
              SBFormattedString("prefs.media_management.error.not_exist",
                                [file.path]));
          return false;
        }
      }
      if (!missingDefault && !file.isDirectory()) {
        showErrorNotification(
            SBFormattedString("prefs.media_management.error.not_directory",
                              [file.path]));
        return false;
      }
      if (!missingDefault && !file.isReadable()) {
        showErrorNotification(
            SBFormattedString("prefs.media_management.error.not_readable",
                              [file.path]));
        return false;
      }

      // Try to create a test file so we can be sure it can be written to
      try {
        var testFile = file.clone();
        testFile.append(MMPREF_WRITE_TEST_FILE);
        if (!testFile.exists()) {
          testFile.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, PERMS_FILE);
          testFile.remove(false);
        }
      } catch (ex) {
        showErrorNotification(
            SBFormattedString("prefs.media_management.error.not_writable",
                              [file.path]));
        return false;
      }
    }

    return true;
  },
 
  _updateUI: function manageMediaPrefsPane__updateUI() {
    // Get the controls so we can enable/disable them
    var managedFolder = document.getElementById("manage_media_library_file");
    var browseButton = document.getElementById("manage_media_library_browse");
    var dirFormatter = document.getElementById("manage_media_format_dir_formatter");
    var renameCheck = document.getElementById("manage_media_format_rename");
    var fileFormatter = document.getElementById("manage_media_format_file_formatter");
     
    var enableButton = document.getElementById("manage_media_global_cmd");
    var previewButton = document.getElementById("manage_media_global_preview");
    var prefElem = document.getElementById(enableButton.getAttribute("preference"));
    var enabled = prefElem.value;
    if (enabled) {
      // Enable all the controls
      enableButton.label = enableButton.getAttribute("label-disable");
      managedFolder.removeAttribute("disabled");
      browseButton.disabled = false;
      renameCheck.removeAttribute("disabled");
      dirFormatter.disableAll = false;
      fileFormatter.disableAll = false;
      previewButton.disabled = false;
    } else {
      // Disable the controls
      enableButton.label = enableButton.getAttribute("label-enable");
      managedFolder.setAttribute("disabled", true);
      browseButton.disabled = true;
      renameCheck.setAttribute("disabled", true);
      dirFormatter.disableAll = true;
      fileFormatter.disableAll = true;
      previewButton.disabled = true;
    }
  },

  /**
   * Saves the original values of the preferences
   */

  _saveCurrentPrefs: function manageMediaPrefsPane__saveCurrentPrefs() {
    // Go through all the preferences and save the current value
    var prefList = document.getElementById("manage_media_preferences");
    var childNode = prefList.firstChild;
    while (childNode) {
      // Only save preferences that matter or are not dealt with separately
      if (childNode.id != "manage_media_pref_library_enable") {
        this._prefs[childNode.id] = childNode.value;
      }
      childNode = childNode.nextSibling;
    }
  },

  /**
   * Checks to see if any of the preferences that we monitor have changed
   */

  _doUpdateCheck: function manageMediaPrefsPane__doUpdateCheck(isEnabled, willEnable) {
    if (!isEnabled || !willEnable) {
      // The Service will not be enabled anyways so just abort the check
      return false;
    }

    // Check all the items to see if any have changed, as soon as one has we return true
    for (var prefID in this._prefs) {
      var prefNode = document.getElementById(prefID);
      if (prefNode && prefNode.value != this._prefs[prefID]) {
        return true;
      }
    }

    // Nothing has changed so return false
    return false;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMEventListener])
};

