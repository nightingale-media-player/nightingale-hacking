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
 * \file  firstRuniTunes.js
 * \brief Javascript source for the first-run wizard iTunes import/export
 *        widget services.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// First-run wizard iTunes import/export widget services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// First-run wizard iTunes import/export widget imported services.
//
//------------------------------------------------------------------------------

// Nightingale imports.
Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");


//------------------------------------------------------------------------------
//
// First-run wizard iTunes import/export widget services defs.
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
// First-run wizard iTunes import/export widget services.
//
//------------------------------------------------------------------------------

/**
 * Construct a first-run wizard iTunes import/export widget services object
 * for the widget specified by aWidget.
 *
 * \param aWidget               First-run wizard iTunes import/export widget.
 */

function firstRuniTunesSvc(aWidget) {
  this._widget = aWidget;
}

// Define the object.
firstRuniTunesSvc.prototype = {
  // Set the constructor.
  constructor: firstRuniTunesSvc,

  //
  // Widget services fields.
  //
  //   _widget                  First-run wizard iTunes import/export widget.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _libraryImporter         Library importer object.
  //

  _widget: null,
  _domEventListenerSet: null,
  _libraryImporter: null,


  //----------------------------------------------------------------------------
  //
  // Widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the widget services.
   */

  initialize: function firstRuniTunesSvc_initialize() {
    // Get the library importer.
    this._libraryImporter = Cc["@getnightingale.com/Nightingale/ITunesImporter;1"]
                              .getService(Ci.sbILibraryImporter);
  },


  /**
   * Finalize the widget services.
   */

  finalize: function firstRuniTunesSvc_finalize() {
    // Remove DOM event listeners.
    if (this._domEventListenerSet)
      this._domEventListenerSet.removeAll();
    this._domEventListenerSet = null;

    // Clear object fields.
    this._widget = null;
  },


  /**
   * Save the user settings in the first run wizard page.
   */

  saveSettings: function firstRuniTunesSvc_saveSettings() {
    // Dispatch processing of the import/export checkboxes
    this._saveITunesSettings();
  },



  //----------------------------------------------------------------------------
  //
  // Internal widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Save the iTunes import/export user settings.
   */

  _saveITunesSettings:
    function firstRuniTunesSvc__saveITunesImportSettings() {
    // Get the iTunes import settings.
    var importCheckbox = this._getElement("itunes_import_checkbox");
    var importEnabled = importCheckbox.checked;
    var importLibraryFilePath = this._libraryImporter.libraryDefaultFilePath;

    // If we don't have a valid iTunes Library path then iTunes isn't installed
    // and this wizard was in fact skipped, so no need to save any preferences
    if (!importLibraryFilePath)
	  return;

    // Save the iTunes import settings.
    Application.prefs.setValue
                        ("nightingale.library_importer.library_file_path",
                         importLibraryFilePath);
    Application.prefs.setValue("nightingale.firstrun.do_import_library", true);
    Application.prefs.setValue("nightingale.library_importer.import_tracks",
                        importEnabled);
    Application.prefs.setValue("nightingale.library_importer.import_playlists",
                        importEnabled);

    // Get the iTunes export settings.
    var exportCheckbox = this._getElement("itunes_export_checkbox");
    var exportEnabled = exportCheckbox.checked;

    // Save the iTunes export settings.
    Application.prefs.setValue("nightingale.library_exporter.export_tracks",
                        exportEnabled);
    Application.prefs.setValue("nightingale.library_exporter.export_playlists",
                        exportEnabled);
    Application.prefs.setValue
                        ("nightingale.library_exporter.export_smartplaylists",
                         exportEnabled);

    // Set the selected media import type for metrics collection.
    var metrics = Cc["@getnightingale.com/Nightingale/Metrics;1"]
                    .createInstance(Ci.sbIMetrics);
    if (importEnabled) {
      // Update metrics here.  If the user opts not to import the iTunes
      // library, the next firstrun wizard will take care of the case where
      // the firstrun.mediaimport metric should be 'filescan' or 'none'
      metrics.metricsInc("firstrun", "mediaimport", "itunes");
    }
    
    if (exportEnabled) {
      metrics.metricsInc("firstrun", "mediaexport", "itunes");
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

  _getElement: function firstRuniTunesSvc__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "anonid",
                                                   aElementID);
  }
};

