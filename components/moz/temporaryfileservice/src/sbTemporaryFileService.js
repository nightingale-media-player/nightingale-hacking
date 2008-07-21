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
 * \file  sbTemporaryFileService.js
 * \brief Javascript source for the temporary file service component.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Temporary file service component.
//
//   This component provides support for creating temporary files and
// directories.  It deletes temporary files and directories when the application
// quits.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Temporary file service imported services.
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

// Songbird imports.
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


//------------------------------------------------------------------------------
//
// Temporary file service configuration.
//
//------------------------------------------------------------------------------

//
// classDescription             Description of component class.
// classID                      Component class ID.
// contractID                   Component contract ID.
// ifList                       List of external component interfaces.
// _xpcom_categories            List of component categories.
//

var sbTemporaryFileServiceCfg = {
  classDescription: "Temporary File Service",
  classID: Components.ID("{04ad95de-d3fe-4159-a97a-d39878978acd}"),
  contractID: "@songbirdnest.com/Songbird/TemporaryFileService;1",
  ifList: [ Ci.sbITemporaryFileService, Ci.nsIObserver ],

  temporaryRootDirectoryName: "TemporaryFileService",
  temporaryFileBaseName: "tmp"
};

sbTemporaryFileServiceCfg._xpcom_categories = [
  {
    category: "app-startup",
    entry:    sbTemporaryFileServiceCfg.classDescription,
    value:    "service," + sbTemporaryFileServiceCfg.contractID
  }
];


//------------------------------------------------------------------------------
//
// Temporary file service object.
//
//------------------------------------------------------------------------------

/**
 * Construct a temporary file service object.
 */

function sbTemporaryFileService() {
}

// Define the object.
sbTemporaryFileService.prototype = {
  // Set the constructor.
  constructor: sbTemporaryFileService,

  //
  // Temporary file service component configuration fields.
  //
  //   classDescription         Description of component class.
  //   classID                  Component class ID.
  //   contractID               Component contract ID.
  //   _xpcom_categories        List of component categories.
  //

  classDescription: sbTemporaryFileServiceCfg.classDescription,
  classID: sbTemporaryFileServiceCfg.classID,
  contractID: sbTemporaryFileServiceCfg.contractID,
  _xpcom_categories: sbTemporaryFileServiceCfg._xpcom_categories,

  //
  // Temporary file service fields.
  //
  //   _cfg                     Configuration settings.
  //   _initStage               Current initialization stage.
  //   _profileAvailable        True if user profile is available.
  //

  _cfg: sbTemporaryFileServiceCfg,
  _initStage: 1,
  _profileAvailable: false,


  //----------------------------------------------------------------------------
  //
  // Temporary file service sbITemporaryFileService services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Root directory of temporary files and directories.
   */

  rootTemporaryDirectory: null,


  /**
   * \brief Create and return a new and unique temporary file of the type
   *        specified by aType.  If aBaseName is specified, use aBaseName as the
   *        base name of the file.  If aExtension is specified, use aExtension
   *        as the file extension.
   *
   * \param aType               Type of file to create.  One of nsIFile file
   *                            types.
   * \param aBaseName           Optional file base name.
   * \param aExtension          Optional file extension.
   */

  createFile: function sbTemporaryFileService_createFile(aType,
                                                         aBaseName,
                                                         aExtension) {
    // Construct a temporary file object and get its permissions.
    var file = this.rootTemporaryDirectory.clone();
    var permissions = this.rootTemporaryDirectory.permissions;

    // If a base name is specified, create a unique directory for the file.
    if (aBaseName) {
      file.append(this._cfg.temporaryFileBaseName);
      file.createUnique(Ci.nsIFile.DIRECTORY_TYPE, permissions);
    }

    // Append base file name with extension.
    var baseName = aBaseName ? aBaseName : this._cfg.temporaryFileBaseName;
    if (aExtension)
      baseName += "." + aExtension;
    file.append(baseName);

    // Create a unique temporary file.
    file.createUnique(aType, permissions);

    return file;
  },


  //----------------------------------------------------------------------------
  //
  // Temporary file service nsIObserver services.
  //
  //----------------------------------------------------------------------------

 /**
  * Observe will be called when there is a notification for the
  * topic |aTopic|.  This assumes that the object implementing
  * this interface has been registered with an observer service
  * such as the nsIObserverService.
  *
  * If you expect multiple topics/subjects, the impl is
  * responsible for filtering.
  *
  * You should not modify, add, remove, or enumerate
  * notifications in the implemention of observe.
  *
  * @param aSubject : Notification specific interface pointer.
  * @param aTopic   : The notification topic or subject.
  * @param aData    : Notification specific wide string.
  *                    subject event.
  */

  observe: function sbTemporaryFileService_observe(aSubject, aTopic, aData) {
    // Dispatch processing of the event.
    switch (aTopic) {
      case "app-startup" :
        // Initialize the services.
        this._initialize();
        break;

      case "profile-after-change" :
        // User profile is now available.
        this._profileAvailable = true;

        // Initialize the services.
        this._initialize();

        break;

      case "quit-application" :
        // Finalize the services.
        this._finalize();
        break;
    }
  },


  //----------------------------------------------------------------------------
  //
  // Temporary file service nsISupports services.
  //
  //----------------------------------------------------------------------------

  QueryInterface: XPCOMUtils.generateQI(sbTemporaryFileServiceCfg.ifList),


  //----------------------------------------------------------------------------
  //
  // Internal temporary file service services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the services.
   */

  _initialize: function sbTemporaryFileService__initialize() {
    // Initialize.
    if (this._initStage == 1)
      this._initialize1();
    if (this._initStage == 2)
      this._initialize2();
  },

  _initialize1: function sbTemporaryFileService__initialize1() {
    // Set up observers.
    var observerSvc = Cc["@mozilla.org/observer-service;1"]
                        .getService(Ci.nsIObserverService);
    observerSvc.addObserver(this, "profile-after-change", false);
    observerSvc.addObserver(this, "quit-application", false);

    // Advance to next initialization stage.
    this._initStage++;
  },

  _initialize2: function sbTemporaryFileService__initialize2() {
    // Wait for the user profile to be available.
    if (!this._profileAvailable)
      return;

    // Set up temporary directory.
    var dirSvc = Cc["@mozilla.org/file/directory_service;1"]
                   .getService(Ci.nsIProperties);
    var profileDir = dirSvc.get("ProfD", Ci.nsIFile);
    var tmpDir = profileDir.clone();
    tmpDir.append(this._cfg.temporaryRootDirectoryName);
    if (!tmpDir.exists()) {
      var permissions = profileDir.permissions;
      tmpDir.create(Ci.nsIFile.DIRECTORY_TYPE, permissions);
    }
    this.rootTemporaryDirectory = tmpDir;

    // Advance to next initialization stage.
    this._initStage++;
  },


  /**
   * Finalize the services.
   */

  _finalize: function sbTemporaryFileService__finalize() {
    // Remove observers.
    var observerSvc = Cc["@mozilla.org/observer-service;1"]
                        .getService(Ci.nsIObserverService);
    observerSvc.removeObserver(this, "profile-after-change");
    observerSvc.removeObserver(this, "quit-application");

    // Remove root temporary directory.
    if (this.rootTemporaryDirectory)
      this.rootTemporaryDirectory.remove(true);
  }
};


//------------------------------------------------------------------------------
//
// Temporary file service component services.
//
//------------------------------------------------------------------------------

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbTemporaryFileService]);
}

