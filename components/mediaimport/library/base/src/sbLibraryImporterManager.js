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
Components.utils.import("resource://app/jsmodules/SBJobUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");
Components.utils.import("resource://app/jsmodules/WindowUtils.jsm");
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
  classDescription: "Songbird Library Importer Manager Service",
  className: "sbLibraryImporterManagerService",
  cid: Components.ID("{0ed2a7e0-78ac-4574-8554-b1e422b02642}"),
  contractID: "@songbirdnest.com/Songbird/LibraryImporterManager;1",
  ifList: [ Ci.nsIObserver,
            Ci.sbILibraryImporterManager,
            Ci.sbILibraryImporterListener ]
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
  //   _isInitialized           True if the library importer manager has been
  //                            initialized.
  //   _libraryImporterList     List of library importers.
  //   _prefsAvailable          True if preferences are available.
  //   _observerSvc             Observer services object.
  //

  classDescription: sbLibraryImporterManagerCfg.classDescription,
  className: sbLibraryImporterManagerCfg.className,
  classID: sbLibraryImporterManagerCfg.cid,
  contractID: sbLibraryImporterManagerCfg.contractID,
  _xpcom_categories: sbLibraryImporterManagerCfg.categoryList,

  _cfg: sbLibraryImporterManagerCfg,
  _isInitialized: false,
  _libraryImporterList: null,
  _prefsAvailable: false,
  _observerSvc: null,


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
  // Library importer manager sbILibraryImporterListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Handle library changed events.  These events occur when the
   * contents of the import library change.
   *
   * \param aLibFilePath File path to external library that changed.
   * \param aGUID GUID of Songbird library into which external library was
   * imported.
   */

  onLibraryChanged:
    function sbLibraryImporterManager_onLibraryChanged(aLibFilePath, aGUID) {

    // Get the FUEL application services.
    var Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);

    // Get library file path from which to import and previously imported
    // library file path.
    //XXXeps should use importer that called onLibraryChanged
    var importer = this.defaultLibraryImporter;
    var libraryFilePath = Application.prefs.getValue
                            ("songbird.library_importer.library_file_path",
                             "");
    var libraryPreviousImportPath = importer.libraryPreviousImportPath;

    // Initiate import.
    var job = importer.import(libraryFilePath, "songbird", false);

    // Get the library file from which to import.  Set to null for bad paths.
    var libraryFile = Cc["@mozilla.org/file/local;1"]
                        .createInstance(Ci.nsILocalFile);
    try { libraryFile.initWithPath(libraryFilePath); }
    catch (ex) { libaryFile = null; }

    // Get the previously imported library file.  Set to null for bad paths.
    var previousLibraryFile = Cc["@mozilla.org/file/local;1"]
                                .createInstance(Ci.nsILocalFile);
    try { previousLibraryFile.initWithPath(libraryPreviousImportPath); }
    catch (ex) { previousLibraryFile = null; }

    // Show progress dialog if importing from a new library file.
    if (!libraryFile ||
        !previousLibraryFile ||
        !libraryFile.equals(previousLibraryFile)) {
      SBJobUtils.showProgressDialog(job, null, 0, true);
    }
  },


  /**
   * \brief Handle library import error events.  These events occur whenever
   * an error is encountered while importing a library.
   */

  onImportError: function sbLibraryImporterManager_onImportError() {
    // Give user the option to try importing again.
    var doImport = {};
    WindowUtils.openModalDialog
                  (null,
                   "chrome://songbird/content/xul/importLibrary.xul",
                   "",
                   "chrome,centerscreen",
                   [ "error" ],
                   [ doImport ]);
    doImport = (doImport.value == "true");

    // Import the library as user directs.
    //XXXeps should use importer that called onImportError
    if (doImport) {
      var Application = Cc["@mozilla.org/fuel/application;1"]
                          .getService(Ci.fuelIApplication);
      var libraryFilePath = Application.prefs.getValue
                              ("songbird.library_importer.library_file_path",
                               "");
      this.defaultLibraryImporter.import(libraryFilePath, "songbird", false);
    }
  },


  /**
   * \brief Handle non-existent media events.  These events occur whenever the
   * media file for an imported track does not exist.
   *
   * \param aNonExistentMediaCount Count of the number of non-existent track
   * media files.
   * \param aTrackCount Count of the number of tracks.
   */

  onNonExistentMedia:
    function sbLibraryImporterManager_onNonExistentMedia(aNonExistentMediaCount,
                                                         aTrackCount) {
    // Present a non-existent media alert dialog.
    var prompter = Cc["@songbirdnest.com/Songbird/Prompter;1"]
                     .createInstance(Ci.sbIPrompter);
    var alertTitle = SBBrandedString
                       ("import_library.nonexistent_media_alert.title");
    var alertMsg =
          SBFormattedString("import_library.nonexistent_media_alert.msg",
                            [ aNonExistentMediaCount, aTrackCount ]);
    var songWin = Cc["@mozilla.org/appshell/window-mediator;1"]
                    .getService(Ci.nsIWindowMediator)
                    .getMostRecentWindow("Songbird:Main");

    prompter.alert(songWin, alertTitle, alertMsg);
  },


  /**
   * \brief Handle unsupported media events.  These events occur whenever an
   * attempt is made to import media that is not supported by Songbird.
   */

  onUnsupportedMedia: function sbLibraryImporterManager_onUnsupportedMedia() {
    // Get unsupported media alert enabled preference.
    var Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);
    var alertEnabledPref =
          "songbird.library_importer.unsupported_media_alert.enabled";
    var alertEnabled = Application.prefs.getValue(alertEnabledPref, false);

    // Do nothing if alert not enabled.
    if (!alertEnabled)
      return;

    // Present an unsupported media alert dialog.
    var prompter = Cc["@songbirdnest.com/Songbird/Prompter;1"]
                     .createInstance(Ci.sbIPrompter);
    var alertTitle = SBBrandedString
                       ("import_library.unsupported_media_alert.title");
    var alertMsg = SBString("import_library.unsupported_media_alert.msg");
    var alertCheckMsg =
          SBString("import_library.unsupported_media_alert.enable_label");
    var checkState = { value: alertEnabled };
    
    var songWin = Cc["@mozilla.org/appshell/window-mediator;1"]
                    .getService(Ci.nsIWindowMediator)
                    .getMostRecentWindow("Songbird:Main");
    
    prompter.alertCheck(songWin,
                        alertTitle,
                        alertMsg,
                        alertCheckMsg,
                        checkState);
    checkState = checkState.value;

    // Update the unsupported media alert enabled preference.
    Application.prefs.setValue(alertEnabledPref, checkState);
  },


  /**
   * \brief Handle dirty playlist events.  These events occur when an imported
   * library playlist has been modified in Songbird.  This method returns the
   * action to take, "keep" to keep the Songbird playlist unmodified, "merge"
   * to merge the import library playlist into the Songbird playlist, and
   * "replace" to replace the Songbird playlist.
   * This method also returns whether to apply the action to all dirty
   * playlists.
   *
   * \param aPlaylistName Name of dirty playlist.
   * \param aApplyAll Apply action to all playlists.
   * \return Import action.
   */

  onDirtyPlaylist:
    function sbLibraryImporterManager_onDirtyPlaylist(aPlaylistName,
                                                      aApplyAll) {
    // Present a dirty playlist dialog.
    var action = {};
    var applyAll = {};
    WindowUtils.openModalDialog
                  (null,
                   "chrome://songbird/content/xul/dirtyPlaylistDialog.xul",
                   "",
                   "chrome,centerscreen",
                   [ aPlaylistName ],
                   [ action, applyAll ]);
    action = action.value;
    applyAll = (applyAll.value == "true");

    // Return results.
    aApplyAll.value = applyAll;

    return action;
  },


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

      case "profile-after-change" :
        this._handleProfileAfterChange();
        break;

      case "songbird-library-manager-ready" :
        this._handleLibraryManagerReady();
        break;

      case "songbird-library-manager-before-shutdown" :
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
   * Handle profile after change events.
   */

  _handleProfileAfterChange:
    function sbLibraryImporterManager__handleProfileAfterChange() {
    // Preferences are now available.
    this._prefsAvailable = true;

    // Initialize the services.
    this._initialize();
  },


  /**
   * Handle library manager ready events.
   */

  _handleLibraryManagerReady:
    function sbLibraryImporterManager__handleLibraryManagerReady() {
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
    // Do nothing if already initialized.
    if (this._isInitialized)
      return;

    // Set up observers.
    if (!this._observerSvc) {
      this._observerSvc = Cc["@mozilla.org/observer-service;1"]
                            .getService(Ci.nsIObserverService);
      this._observerSvc.addObserver(this, "songbird-library-manager-before-shutdown", false);
      this._observerSvc.addObserver(this, "profile-after-change", false);
      this._observerSvc.addObserver(this,
                                    "songbird-library-manager-ready",
                                    false);
    }

    // Wait until preferences are available.
    if (!this._prefsAvailable)
      return;

    // Wait until the library manager services are available.
    try {
      var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                             .getService(Ci.sbILibraryManager);
      if (!libraryManager.mainLibrary)
        return;
    } catch (ex) {
      return;
    }

    // Initialize the list of library importers.
    this._libraryImporterList = [];

    // Add all library importers.
    this._addAllLibraryImporters();

    // If the library file path preference is not set, set a default one.
    var Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);
    var hasLibraryFilePath =
          Application.prefs.has("songbird.library_importer.library_file_path");
    if (!hasLibraryFilePath && this.defaultLibraryImporter) {
      var libraryDefaultFilePath =
            this.defaultLibraryImporter.libraryDefaultFilePath;
      if (libraryDefaultFilePath) {
        Application.prefs.setValue
                            ("songbird.library_importer.library_file_path",
                             libraryDefaultFilePath);
      }
    }

    // Initialization is now complete.
    this._isInitialized = true;

    // Initiate auto-import if configured to do so.
    this._autoImport();
  },


  /**
   * Finalize the services.
   */

  _finalize: function sbLibraryImporterManager__finalize() {
    // Remove observers.
    this._observerSvc.removeObserver(this, "songbird-library-manager-before-shutdown");
    this._observerSvc.removeObserver(this, "profile-after-change");
    this._observerSvc.removeObserver(this, "songbird-library-manager-ready");

    // Remove all library importers.
    this._removeAllLibraryImporters();

    // Clear object references.
    this._libraryImporterList = null;
    this.defaultLibraryImporter = null;
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

    // Initialize the library importer.
    libraryImporter.initialize();

    // Set the library importer listener.
    libraryImporter.setListener(this);

    // Add the library importer.
    this._libraryImporterList.push(libraryImporter);

    // Update the default library importer.
    this._updateDefaultLibraryImporter();
  },


  /**
   * Remove all library importers.
   */

  _removeAllLibraryImporters:
    function sbLibraryImporterManager__removeAllLibraryImporters() {
    for (var i = this._libraryImporterList.length - 1; i >= 0; i--) {
      // Remove the next library importer from the library importer list.
      var libraryImporter = this._libraryImporterList.pop();

      // Unset the library importer listener.
      libraryImporter.setListener(null);

      // Finalize the library importer.
      libraryImporter.finalize();
      libraryImporter = null;
    }
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
  },


  /**
   * Initiate auto-importing as configured.
   */

  _autoImport: function sbLibraryImporterManager_autoImport() {
    // Get the auto-import preferences.
    var Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);
    var firstRunDoImportLibrary =
          Application.prefs.getValue("songbird.firstrun.do_import_library",
                                     false);
    var importTracks = Application.prefs.getValue(
          "songbird.library_importer.import_tracks", false);
    var importPlaylists = Application.prefs.getValue(
          "songbird.library_importer.import_playlists", false);

    // Do nothing if not auto-importing, or if this is first run.
    if (!this.defaultLibraryImporter || firstRunDoImportLibrary ||
        (!importTracks && !importPlaylists))
      return;

    // Wait until the main Songbird window is ready before initiating
    // auto-import.  This ensures any import modal dialogs have a parent.
    var windowWatcher = Cc["@songbirdnest.com/Songbird/window-watcher;1"]
                          .getService(Ci.sbIWindowWatcher);
    var _this = this;
    var func = function(aWindow) { _this._autoImportWithWindow(aWindow); };
    windowWatcher.callWithWindow("Songbird:Main", func, false);
  },

  _autoImportWithWindow:
    function sbLibraryImporterManager_autoImportWithWindow(aWindow) {
    // Initiate auto-import.
    var Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);
    var libraryFilePath = Application.prefs.getValue
                            ("songbird.library_importer.library_file_path",
                             "");
    if (libraryFilePath)
      this.defaultLibraryImporter.import(libraryFilePath, "songbird", true);
  }
}


//------------------------------------------------------------------------------
//
// Library importer manager component services.
//
//------------------------------------------------------------------------------

var NSGetFactory = XPCOMUtils.generateNSGetFactory([sbLibraryImporterManager]);

