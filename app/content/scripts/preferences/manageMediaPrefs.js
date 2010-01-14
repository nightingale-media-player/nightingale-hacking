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

  // holds a copy of the library folder that was in use when the user
  // opened the dialog, so that we can tell when the user has chosen a
  // new folder
  _origLibraryFolder: null,

  /**
   * Handles the preference pane load event.
   */

  doPaneLoad: function manageMediaPrefsPane_doPaneLoad() {
    var self = this;
    this._origLibraryFolder = this._libraryFolder.clone();

    function forceCheck(event) {
      const instantApply =
        Application.prefs.getValue("browser.preferences.instantApply", true);
      if (event.type == "dialogcancel" && !instantApply) {
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
      var folderPrefElem =
        document.getElementById("manage_media_pref_library_folder");

      // explicitly saving these prefs because they're not inside a prefpane
      enablePrefElem.valueFromPreferences = enablePrefElem.value;
      folderPrefElem.valueFromPreferences = folderPrefElem.value;

      var startMgtProcess = (enablePrefElem.value && !mediaMgmtSvc.isEnabled) ||
                            self._doUpdateCheck(enablePrefElem.value);
      mediaMgmtSvc.isEnabled = enablePrefElem.value;

      if (startMgtProcess) {
        // Disable then enable the mediaMgmtSvc to force a scan
        mediaMgmtSvc.isEnabled = false;
        mediaMgmtSvc.isEnabled = true;
      }

      return true;
    }

    var button = document.getElementById("mac_apply_button")
    if (/^Mac/.test(navigator.platform)) {
      button.addEventListener("command", forceCheck, false);
    } else {
      button.hidden = true;
      window.addEventListener("dialogaccept", forceCheck, false);
      window.addEventListener("dialogcancel", forceCheck, false);
    }
    
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

    var manageMode = 0;
    if (Application.prefs.getValue("songbird.media_management.library.copy", true)) {
      manageMode |= Ci.sbIMediaFileManager.MANAGE_COPY;
    }
    if (Application.prefs.getValue("songbird.media_management.library.move", true)) {
      manageMode |= Ci.sbIMediaFileManager.MANAGE_MOVE;
    }
    if (document.getElementById("manage_media_format_rename").checked) {
      manageMode |= Ci.sbIMediaFileManager.MANAGE_RENAME;
    }

    // show the preview
    WindowUtils.openModalDialog(window,
                                "chrome://songbird/content/xul/manageMediaPreview.xul",
                                "manage_media_preview_dialog",
                                "chrome,centerscreen",
                                [LibraryUtils.mainLibrary,
                                 this._libraryFolder,
                                 fileFormat,
                                 dirFormat,
                                 String(manageMode)],
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

    // remove the notifications, because the user is trying to fix the
    // condition that caused the error (if any), and will be confused
    // if the error message doesn't go away as a result
    this._removeErrorNotifications();

    if (folderPicker.show() == Ci.nsIFilePicker.returnOK) {
      this._libraryFolder = folderPicker.file;
    }

    this._checkForValidPref(false);
    this._updateUI();
  },
  
  /**
   * Handle the toggle management mode on/off command
   */
  doEnableCommand: function manageMediaPrefsPane_doEnableCommand(aEvent) {
    var prefElem = document.getElementById(aEvent.target.getAttribute("preference"));
    if (prefElem) {
      prefElem.value = !prefElem.value;
    }

    this._removeErrorNotifications();
    this._checkForValidPref(false);
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

  _removeErrorNotifications: function()
  {
    var oldNotif, notifBox = document.getElementById("media_manage_notification_box");
    while ((oldNotif = notifBox.getNotificationWithValue("media_manage_error"))) {
      notifBox.removeNotification(oldNotif);
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
    this._removeErrorNotifications();
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
    function showErrorNotification(aMsg, aLevel) {
      if (aSilent) {
        return;
      }
      self._showErrorNotification(aMsg, aLevel); 
    }

    // need to manually set the dir pref if it's not user set, because it has
    // no default value (needs platform-specific path separator)
    var dirPrefElem = document.getElementById("manage_media_pref_library_format_dir");
    var dirFormatElem = document.getElementById("manage_media_format_dir_formatter");
    // dirFormatElem.value will be empty if the dialog hasn't been correctly
    // initialized yet; in that case, don't force set the preference
    if (!dirPrefElem.hasUserValue && (dirFormatElem.value != "")) {
      dirPrefElem.valueFromPreferences = dirFormatElem.value;
    }

    // Need to check if the user chose a usable folder for the managed folder.
    var managedFolder = document.getElementById("manage_media_library_file");
    if (!managedFolder || !managedFolder.file) {
      showErrorNotification(SBString("prefs.media_management.error.no_path2"));
      return false;
    }

    var button = document.getElementById("manage_media_global_cmd");
    var prefElem = document.getElementById(button.getAttribute("preference"));
    var enabled = prefElem.value;
    var dir = managedFolder.file;

    if (!enabled) {
      if (!dir.exists()) {
        // The user didn't select a valid managed folder, reset this pref
        // back to the default library folder value value.
        var folderPrefElem =
          document.getElementById("manage_media_pref_library_folder");
        Application.prefs.setValue(folderPrefElem.getAttribute("name"),
                                   self._defaultLibraryFolder.path);
      }
    }
    else {
      var missingDefault = false;
      if (!dir.exists()) {
        var parentDir = dir.parent;
        if (!parentDir.exists()) {
          showErrorNotification(
              SBFormattedString("prefs.media_management.error.not_exist",
                                [dir.path]));
          return false;
        }

        try {
          var permissions = parentDir.permissions;
          dir.create(Ci.nsIFile.DIRECTORY_TYPE, permissions);
          missingDefault = true;
        } catch (e) {
          Cu.reportError("Unable to create folder: " + dir.path +
                         ", defaulting to " + parentDir.path +
                         " - Error: " + e);
          showErrorNotification(
              SBFormattedString("prefs.media_management.error.not_exist",
                                [dir.path]));
          return false;
        }
      }
      if (!missingDefault && !dir.isDirectory()) {
        showErrorNotification(
            SBFormattedString("prefs.media_management.error.not_directory",
                              [dir.path]));
        return false;
      }
      if (!missingDefault && !dir.isReadable()) {
        showErrorNotification(
            SBFormattedString("prefs.media_management.error.not_readable",
                              [dir.path]));
        return false;
      }

      // Try to create a test file so we can be sure it can be written to
      // (unless it's the default, in which case creating the directory is bad)
      if (!(missingDefault && aSilent)) {
        try {
          var testFile = dir.clone();
          testFile.append(MMPREF_WRITE_TEST_FILE);
          if (!testFile.exists()) {
            testFile.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, PERMS_FILE);
            testFile.remove(false);
          }
        } catch (ex) {
          showErrorNotification(
              SBFormattedString("prefs.media_management.error.not_writable",
                                [dir.path]));
          return false;
        }
      }

      if (document.getElementById("watch_folder_enable_pref").value)
      {
        var watched = Cc["@mozilla.org/file/local;1"]
                        .createInstance(Ci.nsILocalFile);
        watched.initWithPath(document.getElementById("watch_folder_path_pref").value);

        if (dir.equals(watched) || dir.contains(watched, true) || watched.contains(dir, true))
        {
          showErrorNotification(SBString("prefs.media_management.error.contains_watched"));
          return false;
        }
      }

      // show a warning if the folder is not empty
      // TODO: perhaps check for files that could actually be imported?
      var complete = document.getElementById("management-complete").value;
      var hasMore = dir.directoryEntries.hasMoreElements();
      var thesame = dir.equals(this._origLibraryFolder);

      if ((!complete || (complete && !thesame)) && hasMore)
      {
        showErrorNotification(SBString("prefs.media_management.warning.not_empty"), "PRIORITY_WARNING_HIGH");
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
     
    var previewButton = document.getElementById("manage_media_global_preview");
    var prefElem = document.getElementById("manage_media_pref_library_enable");
    if (prefElem.value) {
      // Enable all the controls
      managedFolder.removeAttribute("disabled");
      browseButton.disabled = false;
      renameCheck.removeAttribute("disabled");
      dirFormatter.disableAll = false;
      fileFormatter.disableAll = false;
      previewButton.disabled = false;
    } else {
      // Disable the controls
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
    var commonPrefs = document.getElementById("common_prefs");

    var self = this;
    function remember(node)
    {
      if (node.id != "manage_media_pref_library_enable" &&
          !/^watch/.test(node.id))
        self._prefs[node.id] = node.value;
    }

    Array.forEach(prefList.childNodes, remember);
    Array.forEach(commonPrefs.childNodes, remember);
  },

  /**
   * Checks to see if any of the preferences that we monitor have changed
   */

  _doUpdateCheck: function manageMediaPrefsPane__doUpdateCheck(enable) {
    if (!enable) {
      // The Service will not be enabled anyways so just abort the check
      return false;
    }

    // Check all the items to see if any have changed, as soon as one has we return true
    for (var prefID in this._prefs) {
      var prefNode = document.getElementById(prefID);

      if (prefNode) {
        var value = prefNode.value;
        var stored = this._prefs[prefID];

        if (value instanceof Ci.nsIFile && value.equals(stored))
          return true;
        if (value != stored)
          return true;
      }
    }

    // Nothing has changed so return false
    return false;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMEventListener])
};
