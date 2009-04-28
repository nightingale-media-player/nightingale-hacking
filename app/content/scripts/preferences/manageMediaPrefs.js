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
        this._updateUI();
        event.preventDefault();
        return false;
      }

      // need to manually set the dir pref if it's not user set, because it has
      // no default value (needs platform-specific path separator)
      var dirPrefElem = document.getElementById("manage_media_pref_library_format_dir");
      var dirFormatElem = document.getElementById("manage_media_format_dir_formatter");
      if (!dirPrefElem.hasUserValue) {
        dirPrefElem.valueFromPreferences = dirFormatElem.value;
      }

      // dialog is closing, apply any preferences that should never instant-apply
      var mediaMgmtSvc = Cc["@songbirdnest.com/Songbird/media-manager-service;1"]
                           .getService(Ci.sbIMediaManagementService);
      // if we want to toggle global enable, show a preview
      var enablePrefElem =
        document.getElementById("manage_media_pref_library_enable");
      if (!mediaMgmtSvc.isEnabled && enablePrefElem.value) {
        // show the preview
        var accepted =
          WindowUtils.openModalDialog(window,
                                      "chrome://songbird/content/xul/manageMediaPreview.xul",
                                      "manage_media_preview_dialog",
                                      "chrome,centerscreen",
                                      [LibraryUtils.mainLibrary],
                                      null);
        if (!accepted) {
          // cancelled the dialog; force write the pref
          enablePrefElem.valueFromPreferences = mediaMgmtSvc.isEnabled;
          enablePrefElem.value = mediaMgmtSvc.isEnabled;
          event.preventDefault();
          self._checkForValidPref(true);
          self._updateUI();
          return false;
        }
      }
      mediaMgmtSvc.isEnabled = enablePrefElem.value;

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
            // update the one for formatting as well
            let label = document.getElementById("manage_media_format_dir_label");
            let formatter = document.getElementById("manage_media_format_dir_formatter");
            label.value = SBFormattedString("prefs.media_management.file_label",
                                            [fileField.file.leafName,
                                             formatter.defaultSeparator]);
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

    var file = Cc["@songbirdnest.com/Songbird/DownloadDeviceHelper;1"]
                 .getService(Ci.sbIDownloadDeviceHelper)
                 .getDefaultMusicFolder();
    // XXX Mook: this needs more thought, since it could mean we clobber files!
    //document.getElementById("manage_media_pref_library_folder").value = file;
    return file;
  },
  
  set _libraryFolder(aValue) {
    if (!aValue || !(aValue instanceof Ci.nsIFile)) {
      aValue = null;
    }
    if (aValue) {
      document.getElementById("manage_media_pref_library_folder").value = aValue;
    } else {
      // set to default folder
      aValue = Cc["@mozilla.org/file/directory_service;1"]
                 .getService(Ci.nsIProperties)
                 .get("Music", Ci.nsIFile);
      // XXX Mook: this needs more thought, since it could mean we clobber files!
      //document.getElementById("manage_media_pref_library_folder").value = aValue;
    }
  },
  
  /**
   * Check if the current value of the preference is valid; pops up a
   * notification if it is not.
   * @return true if valid, false if not.
   */
  _checkForValidPref: function manageMediaPrefsPane__checkForValidPref(aSilent) {
    var notifBox = document.getElementById("media_manage_notification_box");
    function showErrorNotification(aMsg) {
      if (aSilent) {
        return;
      }
      
      // focus this pref pane and this tab
      var pane = document.getElementById("paneManageMedia");
      document.documentElement.showPane(pane);
      
      // show the notification, hiding any other ones of this class
      var oldNotif;
      while ((oldNotif = notifBox.getNotificationWithValue("media_manage_error"))) {
        notifBox.removeNotification(oldNotif);
      }
      notifBox.appendNotification(aMsg, "media_manage_error", null,
                                  notifBox.PRIORITY_CRITICAL_LOW, []);
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
      if (!file.exists()) {
        showErrorNotification(
            SBFormattedString("prefs.media_management.error.not_exist",
                              [file.path]));
        return false;
      }
      if (!file.isDirectory()) {
        showErrorNotification(
            SBFormattedString("prefs.media_management.error.not_directory",
                              [file.path]));
        return false;
      }
      if (!file.isReadable()) {
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
                              [testFile.path]));
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
    var fmtDirLabel = document.getElementById("manage_media_format_dir_label");
     
    var button = document.getElementById("manage_media_global_cmd");
    var prefElem = document.getElementById(button.getAttribute("preference"));
    var enabled = prefElem.value;
    if (enabled) {
      // Enable all the controls
      button.label = button.getAttribute("label-disable");
      managedFolder.removeAttribute("disabled");
      browseButton.disabled = false;
      renameCheck.removeAttribute("disabled");
      fmtDirLabel.removeAttribute("disabled");
      dirFormatter.disableAll = false;
      fileFormatter.disableAll = false;
    } else {
      // Disable the controls
      button.label = button.getAttribute("label-enable");
      managedFolder.setAttribute("disabled", true);
      browseButton.disabled = false;
      renameCheck.setAttribute("disabled", true);
      fmtDirLabel.setAttribute("disabled", true);
      dirFormatter.disableAll = true;
      fileFormatter.disableAll = true;
    }
    document.getElementById("manage_media_global_description").hidden = enabled;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMEventListener])
};

