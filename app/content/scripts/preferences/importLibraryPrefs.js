/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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
 * \file  importLibraryPrefs.js
 * \brief Javascript source for the import library preferences UI.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Import library preferences UI services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Import library preferences UI imported services.
//
//------------------------------------------------------------------------------

// Nightingale imports.
Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBJobUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");


//------------------------------------------------------------------------------
//
// Import library preferences UI services defs.
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
// Import library preferences UI services.
//
//   These services provide support for implementing library importer preference
// UI inside and outside of a preference window.
//   Elements such as textboxes and checkboxes can be bound to preferences
// without using real preference elements.  In these cases, the preference name
// and type are specified with the "prefname" and "preftype" attributes.
//   All preference or preference bound elements should specify the appropriate
// preference ID from the following list using the "prefid" attribute:
//
//   library_file_path_pref     Library import file path preference.
//   auto_import_pref           Startup auto-import preference.
//   dont_import_playlists_pref Don't import playlists preference.
//   unsupported_media_alert_pref
//                              Alert user on unsupported media preference.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

var importLibraryPrefsUI = {
  //
  // Import library preferences UI services fields.
  //
  //   _userReadableLibraryType Type of library in user readable format.
  //   _defaultLibraryFileName  Default file name of library.
  //   _libraryFileExtFilter    Filter of library file extensions.
  //   _prefBranch              Root preference branch object.
  //

  _userReadableLibraryType: null,
  _defaultLibraryFileName: null,
  _libraryFileExtFilter: null,
  _prefBranch: null,


  //----------------------------------------------------------------------------
  //
  // Public services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the import library preferences UI services.
   */

  initialize: function importLibraryPrefsUI_initialize() {
    // Get the library importer and its attributes.
    //XXXeps don't hard code.
    this._userReadableLibraryType = "iTunes";
    this._defaultLibraryFileName = "iTunes Music Library.xml";
    this._libraryFileExtFilter = "*.xml";

    // Get the preferences branch.
    this._prefBranch = Cc["@mozilla.org/preferences-service;1"]
                         .getService(Ci.nsIPrefBranch);

    // Update the UI.
    this._update();
  },


  /**
   * Read preference values into all elements bound to a preference.
   */

  readPrefs: function importLibraryPrefsUI_readPrefs() {
    // Read the preferences into all preference bound elements.
    var prefElemList = DOMUtils.getElementsByAttribute(document, "prefname");
    for (var i = 0; i < prefElemList.length; i++) {
      // Get the preference bound element.
      var prefElem = prefElemList[i];

      // Don't read into real preference elements.
      if (prefElem.localName == "preference")
        continue;

      // Read the preference value and set the preference bound element value.
      var prefValue = this._readPrefValue(prefElem);
      this._setPrefElemValue(prefElem, prefValue);
    }

    // Update the UI.
    this._update();
  },


  /**
   * Write preference values from all elements bound to a preference.
   */

  writePrefs: function importLibraryPrefsUI_writePrefs() {
    // Write the preferences from all preference bound elements.
    var prefElemList = DOMUtils.getElementsByAttribute(document, "prefname");
    for (var i = 0; i < prefElemList.length; i++) {
      // Get the preference bound element.
      var prefElem = prefElemList[i];

      // Don't write from real preference elements.
      if (prefElem.localName == "preference")
        continue;

      // Write the preference value from the preference bound element.
      var prefValue = this._getPrefElemValue(prefElem);
      this._writePrefValue(prefElem, prefValue);
    }
  },


  //----------------------------------------------------------------------------
  //
  // Event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the browse command event specified by aEvent.
   *
   * \param aEvent              Browse command event.
   */

  doBrowseCommand: function importLibraryPrefsUI_doBrowseCommand(aEvent) {
    // Get the currently selected import library file and its directory.
    var importLibraryPathPrefElem = this._getPrefElem("library_file_path_pref");
    var importLibraryFile = Cc["@mozilla.org/file/local;1"]
                              .createInstance(Ci.nsILocalFile);
    var importLibraryDir = null;
    try {
      importLibraryFile.initWithPath(importLibraryPathPrefElem.value);
      importLibraryDir = importLibraryFile.parent;
    } catch (ex) {
      importLibraryFile = null;
      importLibraryDir = null;
    }

    // Set up a file picker for browsing.
    var filePicker = Cc["@mozilla.org/filepicker;1"]
                       .createInstance(Ci.nsIFilePicker);
    var filePickerMsg = SBFormattedString("import_library.file_picker_msg",
                                          [ this._userReadableLibraryType ]);
    filePicker.init(window, filePickerMsg, Ci.nsIFilePicker.modeOpen);

    // Set the file picker default file name.
    filePicker.defaultString = this._defaultLibraryFileName;

    // Set the file picker initial directory.
    if (importLibraryDir && importLibraryDir.exists())
      filePicker.displayDirectory = importLibraryDir;

    // Set the list of file picker file extension filters.
    var filePickerFilterMsg =
          SBFormattedString("import_library.file_picker_filter_msg",
                            [ this._userReadableLibraryType ]);
    filePicker.appendFilter (filePickerFilterMsg, this._libraryFileExtFilter);

    // Show the file picker.
    var result = filePicker.show();

    // Update the scan directory path.
    if (result == Ci.nsIFilePicker.returnOK)
      importLibraryPathPrefElem.value = filePicker.file.path;
  },

  /**
   * Handle the import command event specified by aEvent.
   * Manually import a library into Nightingale.
   *
   * \param aEvent              Import Library command event.
   */
  doImportCommand: function importLibraryPrefsUI_doIMportCommand(aEvent) {
    document.getElementById("import_playlists_itunes_pref")
            .valueFromPreferences =
      document.getElementById("import_playlists_itunes_checkbox").checked;

    // Get the default library importer.  Do nothing if none available.
    var libraryImporterManager =
          Cc["@getnightingale.com/Nightingale/LibraryImporterManager;1"]
            .getService(Ci.sbILibraryImporterManager);
    var libraryImporter = libraryImporterManager.defaultLibraryImporter;

    // Import the library as user directs.
    var libraryFilePath = this._getPrefElem("library_file_path_pref").value;

    var job = libraryImporter.import(libraryFilePath, "nightingale", false);

    // Pass a timeout of 0 so the dialog opens immediately, and don't make
    // it modal so it won't block the dirty playlist dialog
    SBJobUtils.showProgressDialog(job, window, 0, true);
  },

  /**
   * Handle the import options change event specified aEvent.
   *
   * \param aEvent              Import options change event.
   */

  doOptionsChange: function libraryPrefsUI_doOptionsChange(aEvent, aElement) {
    // Turn off all child prefs that are parent to the root impor/export
    // checkboxes.
    var prefid = aElement.getAttribute("id");
    var prefValue = aElement.value;
    if (!prefValue && prefid == "import_tracks_itunes_pref") {
      document.getElementById("import_playlists_itunes_pref").value = false;
    }
    else if (!prefValue && prefid == "export_tracks_itunes_pref") {
      document.getElementById("export_playlists_itunes_pref").value = false;
      document.getElementById("export_smartplaylists_itunes_pref").value = false;
    }
    
    // Update the UI.
    this._update();
  },

  //----------------------------------------------------------------------------
  //
  // Internal services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the UI.
   */

  _update: function importLibrary__update() {
    // Disable the library import command when there is no file selected.
    var importLibraryButton = document.getElementById
                                         ("import_library_button");
    var libraryPath = this._getPrefElem("library_file_path_pref").value;
    var choseLibrary = (libraryPath != "");
    importLibraryButton.setAttribute("disabled", choseLibrary ? "false" : "true");

    // Update the broadcasters
    this._updateCheckboxBroadcaster("import_tracks_itunes");
    this._updateCheckboxBroadcaster("export_tracks_itunes");
  },


  /**
   * Update a broadcaster based on the corresponding checkbox 'checked' value.
   *
   * \param aIdBaseString The base string of the checkbox and broadcaster ID.
   *        i.e. "myoption_for_something"
   *        Where "myoption_for_something_checkbox" and 
   *        "myoption_for_something_broadcaster" are element IDs.
   */
  
  _updateCheckboxBroadcaster: function(aIdBaseString) {
    var broadcasterElem = 
      document.getElementById(aIdBaseString + "_broadcaster");
    var checkboxElem =
      document.getElementById(aIdBaseString + "_checkbox");

    // Update the broadcaster based on the checkbox value
    if (checkboxElem.checked) {
      broadcasterElem.removeAttribute("disabled");
    }
    else {
      broadcasterElem.setAttribute("disabled", "true");
    }
  },


  /**
   * Return the preference element with the preference ID specified by aPrefID.
   *
   * \param aPrefID             ID of preference element to get.
   *
   * \return                    Preference element.
   */

  _getPrefElem: function importLibrary__getPrefElem(aPrefID) {
    // Get the requested element.
    var prefElemList = DOMUtils.getElementsByAttribute(document,
                                                       "prefid",
                                                       aPrefID);
    if (!prefElemList || (prefElemList.length == 0))
      return null;

    return prefElemList[0];
  },


  /**
   * Return the preference value of the element specified by aElement.
   *
   * \param aElement            Element for which to get preference value.
   *
   * \return                    Preference value of element.
   */

  _getPrefElemValue: function importLibrary__getPrefElemValue(aElement) {
    // Return the checked value for checkbox preferences.
    if (aElement.localName == "checkbox")
      return aElement.checked;

    return aElement.value;
  },


  /**
   * Set the preference value of the element specified by aElement to the value
   * specified by aValue.
   *
   * \param aElement            Element for which to set preference value.
   * \param aValue              Preference value.
   */

  _setPrefElemValue: function importLibrary__setPrefElemValue(aElement,
                                                              aValue) {
    // Set the checked value for checkbox preferences.
    if (aElement.localName == "checkbox") {
      aElement.checked = aValue;
      return;
    }

    aElement.value = aValue;
  },


  /**
   * Read and return the preference value from the element specified by
   * aElement in the same manner as the "preference" xul element.
   *
   * \param aElement            Element for which to read preference value.
   *
   * \return                    Preference value.
   */

  _readPrefValue: function importLibraryPrefsUI__readPrefValue(aElement) {
    // Get the preference name and type.
    var prefName = aElement.getAttribute("prefname");
    var prefType = aElement.getAttribute("preftype");

    // Read the preference value.
    var prefValue = null;
    try {
      switch (prefType) {
        case "int" :
          prefValue = this._prefBranch.getIntPref(prefName);
          break;

        case "bool" :
          prefValue = this._prefBranch.getBoolPref(prefName);
          if (aElement.getAttribute("prefinverted") == "true")
            prefValue = !prefValue;
          break;

        case "string" :
        case "unichar" :
          prefValue = this._prefBranch.getComplexValue(prefName,
                                                       Ci.nsISupportsString);
          break;

        default :
          break;
      }
    } catch (ex) {}

    return prefValue;
  },


  /**
   * Write the preference value specified by aValue to the preference bound to
   * the element specified by aElement in the same manner as the "preference"
   * xul element.
   *
   * \param aElement            Element for which to write preference value.
   * \param aValue              Preference value.
   */

  _writePrefValue: function importLibraryPrefsUI__writePrefValue(aElement,
                                                                 aValue) {
    // Get the preference name and type.
    var prefName = aElement.getAttribute("prefname");
    var prefType = aElement.getAttribute("preftype");

    // Write the preference value.
    switch(prefType) {
      case "int" :
        this._prefBranch.setIntPref(prefName, aValue);
        break;

      case "bool" :
        if (aElement.getAttribute("prefinverted") == "true")
          this._prefBranch.setBoolPref(prefName, !aValue);
        else
          this._prefBranch.setBoolPref(prefName, aValue);
        break;

      case "string" :
      case "unichar" :
        var value = Cc["@mozilla.org/supports-string;1"]
                      .createInstance(Ci.nsISupportsString);
        value.data = aValue;
        this._prefBranch.setComplexValue(prefName, Ci.nsISupportsString, value);
        break;

      default :
        break;
    }
  }
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Import library preference pane services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

var importLibraryPrefsPane = {
  //----------------------------------------------------------------------------
  //
  // Event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle a pane load event.
   */

  doPaneLoad: function importLibraryPrefs_doPaneLoad() {
    // Initialize the import library preferences UI.
    importLibraryPrefsUI.initialize();
  }
}

