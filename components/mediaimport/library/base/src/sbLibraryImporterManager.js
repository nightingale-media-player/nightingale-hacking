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
 * \file  sbLibraryImporterManager.js
 * \brief Javascript source for the library importer manager services.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Library imoprter manager services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Library importer manager imported services.
//
//------------------------------------------------------------------------------

// Songbird services.
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


//------------------------------------------------------------------------------
//
// Library importer manager services defs.
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
// Library importer manager services configuration.
//
//------------------------------------------------------------------------------

//
// className                    Name of component class.
// cid                          Component CID.
// contractID                   Component contract ID.
// ifList                       List of external component interfaces.
// categoryList                 List of component categories.
//

var sbLibraryImporterManagerCfg = {
  className: "Songbird Library Importer Manager Service",
  cid: Components.ID("{0ed2a7e0-78ac-4574-8554-b1e422b02642}"),
  contractID: "@songbirdnest.com/Songbird/LibraryImporterManager;1",
  ifList: [ Ci.nsIObserver, Ci.sbILibraryImporterManager ]
};

sbLibraryImporterManagerCfg.categoryList = [
  {
    category: "app-startup",
    entry:    sbLibraryImporterManagerCfg.className,
    value:    "service," + sbLibraryImporterManagerCfg.contractID
  }
];


//------------------------------------------------------------------------------
//
// Library importer manager services.
//
//------------------------------------------------------------------------------

/**
 * Construct a library importer manager services object.
 */

function sbLibraryImporterManager() {
}

// Define the object.
sbLibraryImporterManager.prototype = {
  // Set the constructor.
  constructor: sbLibraryImporterManager,

  //
  // Library importer manager services fields.
  //
  //   classDescription         Description of component class.
  //   classID                  Component class ID.
  //   contractID               Component contract ID.
  //   _xpcom_categories        List of component categories.
  //
  //   _cfg                     Configuration settings.
  //   _libraryImporterList     List of library importers.
  //

  classDescription: sbLibraryImporterManagerCfg.className,
  classID: sbLibraryImporterManagerCfg.cid,
  contractID: sbLibraryImporterManagerCfg.contractID,
  _xpcom_categories: sbLibraryImporterManagerCfg.categoryList,

  _cfg: sbLibraryImporterManagerCfg,
  _libraryImporterList: null,


  //----------------------------------------------------------------------------
  //
  // Library importer manager sbILibraryImporterManager services.
  //
  //----------------------------------------------------------------------------

  /**
   * Default library importer.
   */

  defaultLibraryImporter: null,


  //----------------------------------------------------------------------------
  //
  // Library importer manager nsIObserver services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the observed event specified by aSubject, aTopic, and aData.
   *
   * \param aSubject            Event subject.
   * \param aTopic              Event topic.
   * \param aData               Event data.
   */

  observe: function sbLibraryImporterManager_observe(aSubject, aTopic, aData) {
    // Dispatch processing of the event.
    switch (aTopic) {
      case "app-startup" :
        this._handleAppStartup();
        break;

      case "quit-application" :
        this._handleAppQuit();
        break;

      default :
        break;
    }
  },


  //----------------------------------------------------------------------------
  //
  // Library importer manager nsISupports services.
  //
  //----------------------------------------------------------------------------

  QueryInterface: XPCOMUtils.generateQI(sbLibraryImporterManagerCfg.ifList),


  //----------------------------------------------------------------------------
  //
  // Library importer manager event handler services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle application startup events.
   */

  _handleAppStartup: function sbLibraryImporterManager__handleAppStartup() {
    // Initialize the services.
    this._initialize();
  },


  /**
   * Handle application quit events.
   */

  _handleAppQuit: function sbLibraryImporterManager__handleAppQuit() {
    // Finalize the services.
    this._finalize();
  },


  //----------------------------------------------------------------------------
  //
  // Internal library importer manager services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the services.
   */

  _initialize: function sbLibraryImporterManager__initialize() {
    // Set up observers.
    var observerSvc = Cc["@mozilla.org/observer-service;1"]
                        .getService(Ci.nsIObserverService);
    observerSvc.addObserver(this, "quit-application", false);

    // Initialize the list of library importers.
    this._libraryImporterList = [];

    // Add all library importers.
    this._addAllLibraryImporters();
  },


  /**
   * Finalize the services.
   */

  _finalize: function sbLibraryImporterManager__finalize() {
    // Remove observers.
    var observerSvc = Cc["@mozilla.org/observer-service;1"]
                        .getService(Ci.nsIObserverService);
    observerSvc.removeObserver(this, "quit-application");

    // Clear object references.
    this._libraryImporterList = null;
  },


  /**
   * Add all library importers.
   */

  _addAllLibraryImporters:
    function sbLibraryImporterManager__addAllLibraryImporters() {
    // Add all of the library importers.
    var categoryManager = Cc["@mozilla.org/categorymanager;1"]
                            .getService(Ci.nsICategoryManager);
    var libraryImporterEnum = categoryManager.enumerateCategory
                                                ("library-importer");
    while (libraryImporterEnum.hasMoreElements()) {
      // Get the next library importer category entry.
      var entry = libraryImporterEnum.getNext()
                    .QueryInterface(Ci.nsISupportsCString);

      // Get the library importer contract ID from the category entry value.
      var contractID = categoryManager.getCategoryEntry("library-importer",
                                                        entry);

      // Add the library importer.
      this._addLibraryImporter(contractID);
    }
  },


  /**
   * Add the library importer with the contract ID specified by aContractID.
   *
   * \param aContractID         Contract ID of library importer to add.
   */

  _addLibraryImporter:
    function sbLibraryImporterManager__addLibraryImporter(aContractID) {
    // Get the library importer.
    var libraryImporter = Cc[aContractID].getService(Ci.sbILibraryImporter);

    // Add the library importer.
    this._libraryImporterList.push(libraryImporter);

    // Update the default library importer.
    this._updateDefaultLibraryImporter();
  },


  /**
   * Update the default library importer.
   */

  _updateDefaultLibraryImporter:
    function sbLibraryImporterManager__updateDefaultLibraryImporter() {
    // Choose the first library importer as the default.
    if (!this.defaultLibraryImporter) {
      if (this._libraryImporterList.length > 0)
        this.defaultLibraryImporter = this._libraryImporterList[0];
    }
  }
}


//------------------------------------------------------------------------------
//
// Library importer manager component services.
//
//------------------------------------------------------------------------------

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbLibraryImporterManager]);
}

