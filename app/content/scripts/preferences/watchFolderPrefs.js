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
    this._update();
  },


  /**
   * Handle the import options change event specified aEvent.
   *
   * \param aEvent              Import options change event.
   */

  doImportOptionsChange:
    function watchFolderPrefsPane_doImportOptionsChange(aEvent) {
    // Update the UI.
    this._update();
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
    }
  },


  //----------------------------------------------------------------------------
  //
  // Internal services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the UI.
   */

  _update: function watchFolderPrefsPane__update() {
    // Get the watch folder enable preference value.
    var watchFolderEnablePrefElem =
          document.getElementById("watch_folder_enable_pref");
    var watchFolderEnabled = watchFolderEnablePrefElem.value;

    // Disable the watch folder path preference if watch folder is disabled.
    var watchFolderDisabledElem =
          document.getElementById("watch_folder_disabled_broadcaster");
    if (watchFolderEnabled)
      watchFolderDisabledElem.removeAttribute("disabled");
    else
      watchFolderDisabledElem.setAttribute("disabled", "true");
  },
}

