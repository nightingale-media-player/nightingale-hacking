/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
 * \file  importLibrary.js
 * \brief Javascript source for the import library dialog.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Import library dialog.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Import library dialog imported services.
//
//------------------------------------------------------------------------------

// Nightingale imports.
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");


//------------------------------------------------------------------------------
//
// Import library dialog defs.
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
// Import library dialog services.
//
//------------------------------------------------------------------------------

var importLibrary = {
  //
  // Import library dialog services fields.
  //
  //   _dialogPB                Dialog box parameter block.
  //   _importType              Type of import to perform.
  //     "first_run"            Import run for the first time.
  //     "manual"               Import manually initiated by user.
  //     "manual_first_import"  First import manual initiated by user.
  //     "changed"              Import intitiated due to change in library.
  //     "error"                Import re-run after error.
  //

  _dialogPB: null,
  _importType: null,


  //----------------------------------------------------------------------------
  //
  // Event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle a load event.
   */

  doLoad: function importLibrary_doLoad() {
    // Get the dialog box parameters.
    this._dialogPB = window.arguments[0].QueryInterface(Ci.nsIDialogParamBlock);
    this._importType = this._dialogPB.GetString(0);

    // Default to not doing import.
    this._dialogPB.SetString(0, "false");

    // Get the library importer.
    var libraryImporterManager =
          Cc["@getnightingale.com/Nightingale/LibraryImporterManager;1"]
            .getService(Ci.sbILibraryImporterManager);
    var libraryImporter = libraryImporterManager.defaultLibraryImporter;
    if (!libraryImporter) {
      onExit();
      return;
    }

    // Check if this is the first manual import.
    if ((this._importType == "manual") &&
        !libraryImporter.libraryPreviouslyImported) {
      this._importType = "manual_first_import";
    }

    // Set the user query text.
    //XXXeps don't hardcode iTunes
    //XXXeps set query based on dialog params
    var queryDescElem = document.getElementById("import_library_query");
    var userQuery = SBBrandedFormattedString
                      ("import_library.dialog_query." + this._importType,
                       [ "iTunes" ]);
    var userQueryNode = document.createTextNode(userQuery);
    queryDescElem.appendChild(userQueryNode);

    // Initialize the import library preferences UI.
    importLibraryPrefsUI.initialize();

    // Read the import library preferences.
    importLibraryPrefsUI.readPrefs();
  },


  /**
   * Handle an accept event.
   */

  doAccept: function importLibrary_doAccept() {
    // Write the import library preferences.
    importLibraryPrefsUI.writePrefs();

    // Indicate that importing should be done.
    this._dialogPB.SetString(0, "true");

    return true;
  }
}

