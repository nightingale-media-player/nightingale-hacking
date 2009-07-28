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
 * \file  watchFolderPrefs.js
 * \brief Javascript source for the watch folder preferences.
 */

//------------------------------------------------------------------------------
//
// Watch folder preferences imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/SBJobUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");

//------------------------------------------------------------------------------
//
// Watch folder preferences services defs.
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
//------------------------------------------------------------------------------
//
// Watch folder preference pane services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

var watchFolderPrefsPane = {

  // set to true when the folder path has changed
  // will also be set if watch folders was toggled disabled -> enabled
  folderPathChanged: false,

  //----------------------------------------------------------------------------
  //
  // Event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle a pane load event.
   */

  doPaneLoad: function watchFolderPrefsPane_doPaneLoad() {
    // Update the UI.
    this._checkForValidPref(true);
    var self = this;
    function forceCheck(event) {
      if (event.type == "dialogcancel" &&
          !Application.prefs.getValue("browser.preferences.instantApply", true))
      {
        return;
      }
      if (!self._checkForValidPref(false)) {
        event.preventDefault();
        event.stopPropagation();
        return false;
      }
      var watchFolderEnableCheckbox =
            document.getElementById("watch_folder_enable_checkbox");
      var watchFolderPathPrefElem =
            document.getElementById("watch_folder_path_pref");
      var shouldPrompt = self.folderPathChanged;

      // should never prompt for watch folder rescan if it's not enabled
      shouldPrompt &= watchFolderEnableCheckbox.checked;

      if (shouldPrompt) {
        // offer to rescan with the _new_ watch dir, taking into account that
        // instantApply may be off
        var watchFolderDir = Cc["@mozilla.org/file/local;1"]
                               .createInstance(Ci.nsILocalFile);
        var watchFolderPathTextboxElem =
          document.getElementById("watch_folder_path_textbox");
        var watchFolderPath =
          watchFolderPathPrefElem.getElementValue(watchFolderPathTextboxElem);
        watchFolderDir.initWithPath(watchFolderPath);
        self._rescan(watchFolderDir);
      }
    }
    window.addEventListener('dialogaccept', forceCheck, true);
    window.addEventListener('dialogcancel', forceCheck, true);
  },


  /**
   * Handle the import options change event specified aEvent.
   *
   * \param aEvent              Import options change event.
   */

  doImportOptionsChange:
    function watchFolderPrefsPane_doImportOptionsChange(aEvent) {
    // Update the UI.
    this._checkForValidPref(true);
  },


  /**
   * Handle the browse command event specified by aEvent.
   *
   * \param aEvent              Browse command event.
   */

  doBrowseCommand: function watchFolderPrefsPane_doBrowseCommand(aEvent) {
    // Get the currently selected watch folder directory.
    var watchFolderPathPrefElem =
          document.getElementById("watch_folder_path_pref");
    var watchFolderDir = Cc["@mozilla.org/file/local;1"]
                           .createInstance(Ci.nsILocalFile);
    try {
      watchFolderDir.initWithPath(watchFolderPathPrefElem.value);
      if (!watchFolderDir.exists() ||
          !watchFolderDir.isDirectory() ||
          watchFolderDir.isSymlink()) {
        watchFolderDir = null;
      }
    } catch (ex) {
      watchFolderDir = null;
    }

    // Set up a file picker for browsing.
    var filePicker = Cc["@mozilla.org/filepicker;1"]
                       .createInstance(Ci.nsIFilePicker);
    var filePickerMsg = SBString("watch_folder.file_picker_msg");
    filePicker.init(window, filePickerMsg, Ci.nsIFilePicker.modeGetFolder);

    // Set the file picker initial directory.
    if (watchFolderDir)
      filePicker.displayDirectory = watchFolderDir;

    // Show the file picker.
    var result = filePicker.show();

    // Update the watch folder path.
    if ((result == Ci.nsIFilePicker.returnOK) &&
        (filePicker.file.isDirectory())) {
      watchFolderPathPrefElem.value = filePicker.file.path;
      this.onPathChanged(aEvent);
    }
  },
  
  /**
   * Handle the command event specified by aEvent
   */
  onPathChanged: function watchFolderPrefsPane_onPathChanged(aEvent) {
    this.folderPathChanged = true;
    // everything's good, remember to apply the enabled pref if it was
    // originally checked but not saved
    var watchFolderEnableCheckboxElem =
          document.getElementById("watch_folder_enable_checkbox");
    if (aEvent.target != watchFolderEnableCheckboxElem) {
      watchFolderEnableCheckboxElem.doCommand();
    }
    return true;
  },


  //----------------------------------------------------------------------------
  //
  // Internal services.
  //
  //----------------------------------------------------------------------------
  
  /**
   * Check if the current value of the preference is valid; pops up a
   * notification if it is not.
   * @return true if valid, false if not.
   */
  _checkForValidPref: function watchFolderPrefsPane__checkForValidPref(aSilent) {
    var notifBox = document.getElementById("watch_folder_notification_box");
    function showErrorNotification(aMsg) {
      if (aSilent) {
        return;
      }
      
      // focus this pref pane and this tab
      var pane = document.getElementById("paneImportMedia");
      document.documentElement.showPane(pane);
      document.getElementById("import_media_tabs").selectedItem =
        document.getElementById("watch_folder_tab");
      
      // show the notification, hiding any other ones of this class
      var oldNotif;
      while ((oldNotif = notifBox.getNotificationWithValue("watch_folder_error"))) {
        notifBox.removeNotification(oldNotif);
      }
      notifBox.appendNotification(aMsg, "watch_folder_error", null,
                                  notifBox.PRIORITY_CRITICAL_LOW, []);
    }

    // Set the watch folder hidden if the watch folder services are not
    // available.
    var watchFolderSupported = ("@songbirdnest.com/watch-folder-service;1" in Cc);
    if (watchFolderSupported) {
      watchFolderSupported = Cc["@songbirdnest.com/watch-folder-service;1"]
                               .getService(Ci.sbIWatchFolderService)
                               .isSupported;
    }
    var watchFolderHiddenElem =
          document.getElementById("watch_folder_hidden_broadcaster");
    if (watchFolderSupported) {
        watchFolderHiddenElem.removeAttribute("hidden");
    } else {
        watchFolderHiddenElem.setAttribute("hidden", "true");
    }

    var watchFolderEnablePrefElem =
          document.getElementById("watch_folder_enable_pref");
    var watchFolderEnableCheckboxElem =
          document.getElementById("watch_folder_enable_checkbox");

    var watchFolderEnabled =
      watchFolderEnablePrefElem.getElementValue(watchFolderEnableCheckboxElem);

    // Disable the watch folder path preference if watch folder is disabled.
    var watchFolderDisabledElem =
          document.getElementById("watch_folder_disabled_broadcaster");
    if (watchFolderEnabled) {
      watchFolderDisabledElem.removeAttribute("disabled");
    } else {
      watchFolderDisabledElem.setAttribute("disabled", "true");
    }

    if (!watchFolderEnabled) {
      // if it's not enabled, we allow whatever.
      return true;
    }

    var watchFolderPathPrefElem =
          document.getElementById("watch_folder_path_pref");
    var watchFolderPathTextboxElem =
          document.getElementById("watch_folder_path_textbox");

    // only continue checking the folder path validity if the path changed
    if (!this.folderPathChanged)
      return true;

    var path = watchFolderPathPrefElem.getElementValue(watchFolderPathTextboxElem);
    if (!path) {
      // there is no pref set, but watch folder is enabled. Complain.
      showErrorNotification(SBString("prefs.watch_folder.error.no_path"));
      return false;
    }
    
    var file = null;
    try {
      file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      file.initWithPath(path);
      if (!file.exists()) {
        showErrorNotification(SBFormattedString("prefs.watch_folder.error.not_exist", [file.path]));
        return false;
      }
      if (!file.isDirectory()) {
        showErrorNotification(SBFormattedString("prefs.watch_folder.error.not_directory", [file.path]));
        return false;
      }
      if (!file.isReadable()) {
        showErrorNotification(SBFormattedString("prefs.watch_folder.error.not_readable", [file.path]));
        return false;
      }
    } catch (e) {
      // failed to create the file (e.g. invalid path?)
      Components.utils.reportError(e);
      showErrorNotification(SBFormattedString("prefs.watch_folder.error.generic", [path]));
      return false;
    }
    return true;
  },
  
  /**
   * Offer import of items in new watch folder
   */
  _rescan: function watchFolderPrefsPane__rescan(aFile) {
    var shouldImportTitle = 
      SBString("watch_folder.new_folder.scan_title");
    var shouldImportMsg = 
      SBFormattedString("watch_folder.new_folder.scan_text", [aFile.path]);
    
    var promptService = Cc['@mozilla.org/embedcomp/prompt-service;1']
                          .getService(Ci.nsIPromptService);
    var promptButtons = 
      promptService.STD_YES_NO_BUTTONS + promptService.BUTTON_POS_1_DEFAULT;
                            
    var checkState = {};       
    var result = promptService.confirmEx(window, 
                                         shouldImportTitle, 
                                         shouldImportMsg, 
                                         promptButtons, 
                                         null, 
                                         null, 
                                         null, 
                                         null, 
                                         checkState);
    if(result == 0) {
      var importer = Cc['@songbirdnest.com/Songbird/DirectoryImportService;1']
                       .getService(Ci.sbIDirectoryImportService);
      var directoryArray = ArrayConverter.nsIArray([aFile]);
      
      var job = importer.import(directoryArray);
      SBJobUtils.showProgressDialog(job, window, 0);
    }
  }
};

