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
 * \file  AddOnUtils.jsm
 * \brief Javascript source for the add-on utilities.
 */

//------------------------------------------------------------------------------
//
// Add-on utilities configuration.
//
//------------------------------------------------------------------------------

EXPORTED_SYMBOLS = [ "AddOnBundleLoader" ];

//
// addOnBundleURLPref           Preference with the add-on bundle URL.
// addOnBundleDataLoadTimeout   Timeout in milliseconds for loading the add-on
//                              bundle data.
// addOnBundleCacheFileName     Add-on bundle cache file name.
//

var AddOnUtilsCfg = {
  addOnBundleURLPref: "nightingale.url.firstrun",
  addOnBundleBlacklistPref: "nightingale.recommended_addons.update.blacklist",
  addOnBundleDataLoadTimeout: 15000,
  addOnBundleCacheFileName: "recommendedAddOnBundle.xml"
};


//------------------------------------------------------------------------------
//
// Add-on utilities imported services.
//
//------------------------------------------------------------------------------

Components.utils.import("resource://app/jsmodules/RDFHelper.jsm");


//------------------------------------------------------------------------------
//
// Add-on utilities defs.
//
//------------------------------------------------------------------------------

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const DEFAULT_IO_FLAGS = -1;
const DEFAULT_PERMISSIONS = -1;


//------------------------------------------------------------------------------
//
// Add-on bundle loader services.
//
//   These services may be used to load add-on bundles.
//
//------------------------------------------------------------------------------

/**
 * Construct an add-on bundle loader object.
 */

function AddOnBundleLoader()
{
}


/**
 * Add all currently installed add-ons to the recommended add-on black list.
 */

AddOnBundleLoader.addInstalledAddOnsToBlacklist =
  function AddOnBundleLoader_addInstalledAddOnsToBlacklist() {
  // Get the list of installed add-ons.
  var installedAddOnList = RDFHelper.help("rdf:addon-metadata",
                                          "urn:nightingale:addon:root",
                                          RDFHelper.DEFAULT_RDF_NAMESPACES);

  // Add each installed add-on to the blacklist.
  for (var i = 0; i < installedAddOnList.length; i++) {
    this.addAddOnToBlacklist(installedAddOnList[i].id);
  }
}


/**
 * Add the add-on specified by aAddOnID to the recommended add-on black list.
 *
 * \param aAddOnID            ID of add-on to add to black list.
 */

AddOnBundleLoader.addAddOnToBlacklist =
  function AddOnBundleLoader_addAddOnToBlacklist(aAddOnID) {
  // Do nothing if no ID.
  if (!aAddOnID)
    return;

  // Get the add-on blacklist.
  var Application = Cc["@mozilla.org/fuel/application;1"]
                      .getService(Ci.fuelIApplication);
  var blacklist =
        Application.prefs.getValue(AddOnUtilsCfg.addOnBundleBlacklistPref, "");
  if (blacklist.length > 0)
    blacklist = blacklist.split(",");
  else
    blacklist = [];

  // Do nothing if add-on is already in the blacklist.
  for (var i = 0; i < blacklist.length; i++) {
    if (blacklist[i] == aAddOnID)
      return;
  }

  // Add the add-on to the blacklist.
  blacklist.push(aAddOnID);
  Application.prefs.setValue(AddOnUtilsCfg.addOnBundleBlacklistPref,
                             blacklist.join(","));
}


// Define the class.
AddOnBundleLoader.prototype = {
  // Set the constructor.
  constructor: AddOnBundleLoader,


  //
  // Public object fields.
  //
  //   filterInstalledAddOns    Filter out from the add-on bundle the set of
  //                            already installed add-ons.
  //   filterBlacklistedAddOns  Filter out from the add-on bundle the set of
  //                            blacklisted add-ons.
  //   readFromCache            If true, read add-on bundle from cache.
  //   addOnBundle              Loaded add-on bundle.
  //   complete                 True if add-on bundle loading is complete.
  //   result                   Result of add-on bundle loading.  A value of
  //                            Cr.NS_OK indicates a successful load.
  //

  filterInstalledAddOns: false,
  filterBlacklistedAddOns: false,
  readFromCache: false,
  addOnBundle: null,
  complete: false,
  result: Cr.NS_OK,


  //
  // Internal object fields.
  //
  //   _cfg                     Configuration settings.
  //   _completionCallback      Completion callback function.
  //   _cancelled               True if add-on loading was cancelled.
  //

  _cfg: AddOnUtilsCfg,
  _completionCallback: null,
  _cancelled: false,


  //----------------------------------------------------------------------------
  //
  // Public services.
  //
  //----------------------------------------------------------------------------

  /**
   * Start loading the add-on bundle.  Call the completion callback function
   * specified by aCompletionCallback upon completion of the add-on bundle
   * loading.
   *
   * \param aCompletionCallback Completion callback function.
   */

  start: function AddOnBundleLoader_start(aCompletionCallback) {
    this._completionCallback = aCompletionCallback;
    this._loadAddOnBundle();
  },


  /**
   * Cancel loading of the add-on bundle.
   */

  cancel: function AddOnBundleLoader_cancel() {
    //XXXeps a way should be provided to cancel the bundle loading.
    // Mark add-on bundle loading as cancelled.
    this._cancelled = true;

    // Clear object references.
    this.addOnBundle = null;
    this._completionCallback = null;
  },


  //----------------------------------------------------------------------------
  //
  // Add-on bundle update service sbIBundleDataListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Bundle download completion callback
   * This method is called upon completion of the bundle data download
   * \param bundle An interface to the bundle manager that triggered the event
   * \sa onError, sbIBundle
   */

  onDownloadComplete: function AddOnBundleLoader_onDownloadComplete(aBundle) {
    this.complete = true;
    this._loadAddOnBundle();
  },


  /**
   * \brief Bundle download error callback
   * This method is called upon error while downloading the bundle data
   * \param bundle An interface to the bundle manager that triggered the event
   * \sa onDownloadComplete, sbIBundle
   */

  onError: function AddOnBundleLoader_onDownloadComplete_onError(aBundle) {
    this.result = Cr.NS_ERROR_FAILURE;
    this.complete = true;
    this._loadAddOnBundle();
  },


  //----------------------------------------------------------------------------
  //
  // Internal services.
  //
  //----------------------------------------------------------------------------

  /**
   * Load the add-on bundle.
   */

  _loadAddOnBundle: function AddOnBundleLoader__loadAddOnBundle() {
    // Do nothing if add-on bundle loading has been cancelled.
    if (this._cancelled)
      return;

    // Start loading the add-on bundle data.
    if (!this.addOnBundle) {
      // Set up the add-on bundle for loading.
      this.addOnBundle = Cc["@getnightingale.com/Nightingale/Bundle;1"]
                           .createInstance(Ci.sbIBundle);
      this.addOnBundle.bundleId = "firstrun";

      // Start loading the add-on bundle data.
      if (this.readFromCache)
        this._readAddOnBundleFromCache();
      else
        this._readAddOnBundleFromServer();
    }

    // Do nothing more until bundle loading is complete.
    if (!this.complete)
      return;

    // Post-process the add-on bundle.
    if (Components.isSuccessCode(this.result)) {
      // Write the add-on bundle to the add-on bundle cache file.
      if (!this.readFromCache)
        this._writeAddOnBundleToCache();

      // If filtering out blacklisted add-ons, remove them.
      if (this.filterBlacklistedAddOns)
        this._removeBlacklistedAddOns();

      // If filtering out installed add-ons, remove them.
      if (this.filterInstalledAddOns)
        this._removeInstalledAddOns();
    }

    // Call the completion callback function.
    if (this._completionCallback)
      this._completionCallback(this);
  },


  /**
   * Read the add-on bundle from the add-on bundle server.
   */

  _readAddOnBundleFromServer:
    function AddOnBundleLoader__readAddOnBundleFromServer() {
    // Set the add-on bundle URL.
    var Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);
    this.addOnBundle.bundleURL = Application.prefs.getValue
                                   (this._cfg.addOnBundleURLPref,
                                    "default");

    // Start loading the add-on bundle data and listen for add-on bundle events.
    try {
      this.addOnBundle.addBundleDataListener(this);
      this.addOnBundle.retrieveBundleData
                         (this._cfg.addOnBundleDataLoadTimeout);
    } catch (ex) {
      // Report the exception as an error.
      Cu.reportError(ex);

      // Indicate that the add-on bundle loading failed.
      this.result = Cr.NS_ERROR_FAILURE;
      this.complete = true;
    }
  },


  /**
   * Read the add-on bundle from the add-on bundle cache file.
   */

  _readAddOnBundleFromCache:
    function AddOnBundleLoader__readAddOnBundleFromCache() {
    // Get the recommended add-on bundle cache file.
    var recommendedAddOnBundleFile = this._getRecommendedAddOnCacheFile();

    // Check if cache file exists.
    if (!recommendedAddOnBundleFile.exists()) {
      this.result = Cr.NS_ERROR_FAILURE;
      this.complete = true;
    }

    // Set the add-on bundle URL.
    var ioService = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);
    var fileURI = ioService.newFileURI(recommendedAddOnBundleFile);
    this.addOnBundle.bundleURL = fileURI.spec;

    // Load the add-on bundle data.
    try {
      this.addOnBundle.retrieveLocalBundleData();
      this.complete = true;
    } catch (ex) {
      // Report the exception as an error.
      Cu.reportError(ex);

      // Indicate that the add-on bundle loading failed.
      this.result = Cr.NS_ERROR_FAILURE;
      this.complete = true;
    }
  },


  /**
   * Write the add-on bundle to the add-on bundle cache file.
   */

  _writeAddOnBundleToCache:
    function AddOnBundleLoader__writeAddOnBundleToCache() {
    // Get the recommended add-on bundle cache file.
    var recommendedAddOnBundleFile = this._getRecommendedAddOnCacheFile();

    // Open an output stream to the file.
    var outputStream = Cc["@mozilla.org/network/file-output-stream;1"]
                         .createInstance(Ci.nsIFileOutputStream);
    outputStream.init(recommendedAddOnBundleFile,
                      DEFAULT_IO_FLAGS,
                      DEFAULT_PERMISSIONS,
                      0);

    // Write the add-on bundle data to the file.
    try {
      outputStream.write(this.addOnBundle.bundleDataText,
                         this.addOnBundle.bundleDataText.length);
    } catch (ex) {
      Cu.reportError(ex);
    } finally {
      outputStream.close();
    }
  },


  /**
   * Return the recommended add-on bundle cache file.
   *
   * \return                    Recommended add-on bundle cache file.
   */

  _getRecommendedAddOnCacheFile:
    function AddOnBundleLoader__getRecommendedAddOnCacheFile() {
    // Get the recommended add-on bundle cache file.
    var recommendedAddOnBundleFile =
          Cc["@mozilla.org/file/directory_service;1"]
            .getService(Ci.nsIProperties)
            .get("ProfD", Ci.nsIFile);
    recommendedAddOnBundleFile.append(this._cfg.addOnBundleCacheFileName);

    return recommendedAddOnBundleFile;
  },


  /**
   * Remove add-ons from the loaded bundle that are already installed.
   */

  _removeInstalledAddOns: function AddOnBundleLoader__removeInstalledAddOns() {
    // Get the list of installed add-ons.
    var installedAddOnList = RDFHelper.help("rdf:addon-metadata",
                                            "urn:nightingale:addon:root",
                                            RDFHelper.DEFAULT_RDF_NAMESPACES);

    // Create a table of installed add-ons, indexed by add-on ID.
    var installedAddOnTable = {};
    for (var i = 0; i < installedAddOnList.length; i++) {
      var installedAddOn = installedAddOnList[i];
      installedAddOnTable[installedAddOn.id] = installedAddOn;
    }

    // Remove bundle add-ons that are already installed.
    var extensionCount = this.addOnBundle.bundleExtensionCount;
    for (var i = extensionCount - 1; i >= 0; i--) {
      // Get the bundle add-on ID.
      var addOnID = this.addOnBundle.getExtensionAttribute(i, "id");

      // Remove bundle add-on if it's already installed.
      if (installedAddOnTable[addOnID])
        this.addOnBundle.removeExtension(i);
    }
  },


  /**
   * Remove add-ons from the loaded bundle that are blacklisted.
   */

  _removeBlacklistedAddOns:
    function AddOnBundleLoader__removeBlacklistedAddOns() {
    // Get the add-on blacklist.
    var Application = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication);
    var blacklist =
          Application.prefs.getValue(this._cfg.addOnBundleBlacklistPref, "");
    if (blacklist.length > 0)
      blacklist = blacklist.split(",");
    else
      blacklist = [];

    // Create a table of blacklisted add-ons, indexed by add-on ID.
    var blacklistedAddOnTable = {};
    for (var i = 0; i < blacklist.length; i++) {
      var blacklistedAddOnID = blacklist[i];
      blacklistedAddOnTable[blacklistedAddOnID] = true;
    }

    // Remove bundle add-ons that are blacklisted.
    var extensionCount = this.addOnBundle.bundleExtensionCount;
    for (var i = extensionCount - 1; i >= 0; i--) {
      // Get the bundle add-on ID.
      var addOnID = this.addOnBundle.getExtensionAttribute(i, "id");

      // Remove bundle add-on if it's blacklisted.
      if (blacklistedAddOnTable[addOnID])
        this.addOnBundle.removeExtension(i);
    }
  }
}

