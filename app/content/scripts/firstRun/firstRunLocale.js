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
* \file  firstRunLocale.js
* \brief Javascript source for the first-run wizard locale widget services.
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// First-run wizard locale widget services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// First-run wizard locale widget services defs.
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
// First-run wizard locale widget services configuration.
//
//------------------------------------------------------------------------------

/*
 * localeBundleDataLoadTimeout  Timeout in milliseconds for loading the locales
 *                              bundle data.
 */

var firstRunLocaleSvcCfg = {
  localeBundleDataLoadTimeout: 15000
};


//------------------------------------------------------------------------------
//
// First-run wizard locale widget services.
//
//------------------------------------------------------------------------------

/**
 * Construct a first-run wizard locale widget services object for the widget
 * specified by aWidget.
 *
 * \param aWidget               First-run wizard locale widget.
 */

function firstRunLocaleSvc(aWidget) {
  this._widget = aWidget;
}

// Define the object.
firstRunLocaleSvc.prototype = {
  // Set the constructor.
  constructor: firstRunLocaleSvc,

  //
  // Widget services fields.
  //
  //   _cfg                     Widget services configuration.
  //   _widget                  First-run wizard locale widget.
  //   _targetLocale            Target first-run locale.
  //   _targetLocaleInstalled   True if target first-run locale is installed.
  //   _localesBundleDataLoadComplete
  //                            True if loading of locales bundle data is
  //                            complete.
  //   _localesBundleDataLoadSucceeded
  //                            True if loading of locales bundle data
  //                            succeeded.
  //

  _cfg: firstRunLocaleSvcCfg,
  _widget: null,
  _targetLocale: null,
  _targetLocaleInstalled: false,
  _localesBundleDataLoadComplete: false,
  _localesBundleDataLoadSucceeded: false,


  //----------------------------------------------------------------------------
  //
  // Widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the widget services.
   */

  initialize: function firstRunLocaleSvc_initialize() {
    // Check the locale.
    this._checkLocale();
  },


  /**
   * Finalize the widget services.
   */

  finalize: function firstRunLocaleSvc_finalize() {
    // Cancel locale switch.
    this.cancelSwitchLocale();

    // Clear object fields.
    this._widget = null;
    this._localesBundle = null;
  },


  /**
   * Switch to the target locale.
   */

  switchLocale: function firstRunLocaleSvc_switchLocale() {
    // Switch to the target locale if it's installed.  Otherwise, install the
    // target locale first.
    if (this._targetLocaleInstalled) {
      // Switch locale.
      switchLocale(this._targetLocale);

      // Send a locale switch complete event.
      this._widget._localeSwitchSucceeded = true;
      this._dispatchEvent(this._widget, "localeswitchcomplete");
    } else {
      this._installTargetLocale();
    }
  },


  /**
   * Cancel locale switch.
   */

  cancelSwitchLocale: function firstRunLocaleSvc_cancelSwitchLocale() {
    // Remove the locales bundle data listener.
    if (this._localesBundle)
      this._localesBundle.removeBundleDataListener(this);

    // Clear locales bundle.
    //XXXeps no way to cancel loading or installation.
    this._localesBundle = null;
  },


  //----------------------------------------------------------------------------
  //
  // sbIBundleDataListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Bundle download completion callback
   * This method is called upon completion of the bundle data download
   * \param bundle An interface to the bundle manager that triggered the event
   * \sa onError, sbIBundle
   */

  onDownloadComplete: function firstRunLocaleSvc__onDownloadComplete(aBundle) {
    this._localesBundleDataLoadComplete = true;
    this._localesBundleDataLoadSucceeded = true;
    this._installTargetLocale();
  },


  /**
   * \brief Bundle download error callback
   * This method is called upon error while downloading the bundle data
   * \param bundle An interface to the bundle manager that triggered the event
   * \sa onDownloadComplete, sbIBundle
   */

  onError: function firstRunLocaleSvc__onError(aBundle) {
    this._localesBundleDataLoadComplete = true;
    this._localesBundleDataLoadSucceeded = false;
    this._installTargetLocale();
  },


  //----------------------------------------------------------------------------
  //
  // Internal widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Check if the locale needs to be switched and, if so, if the target locale
   * is installed.
   */

  _checkLocale: function firstRunLocaleSvc__checkLocale() {
    // Get the target locale that applications should use.
    var localeService = Cc["@mozilla.org/intl/nslocaleservice;1"]
                          .getService(Ci.nsILocaleService);
    this._targetLocale = localeService.getApplicationLocale()
                                      .getCategory("NSILOCALE_CTYPE");

    // Get the locale selected for Songbird.
    var chromeRegistry = Cc["@mozilla.org/chrome/chrome-registry;1"]
                           .getService(Ci.nsIXULChromeRegistry);
    var selectedLocale = chromeRegistry.getSelectedLocale("songbird");

    // If Songbird is already selected to use the locale that applications
    // should use, then no locale switch is required.  Otherwise, a locale
    // switch is required.
    if (selectedLocale == this._targetLocale) {
      this._widget._localeSwitchRequired = false;
      return;
    }
    this._widget._localeSwitchRequired = true;

    // Get the list of installed locales.
    var chromeRegistry = Cc["@mozilla.org/chrome/chrome-registry;1"]
                           .getService(Ci.nsIToolkitChromeRegistry);
    var localeEnum = chromeRegistry.getLocalesForPackage("songbird");

    // Check if the target locale is installed.
    while (localeEnum.hasMore()) {
      var locale = localeEnum.getNext();
      if (locale == this._targetLocale) {
        this._targetLocaleInstalled = true;
        break;
      }
    }
  },


  /**
   * Install the target locale and switch to it.
   */

  _installTargetLocale: function firstRunLocaleService__installTargetLocale() {
    // Load the locales bundle data if it's not loaded.
    if (!this._localesBundle) {
      // Set up the locales bundle for loading.
      this._localesBundle = Cc["@songbirdnest.com/Songbird/Bundle;1"]
                              .createInstance(Ci.sbIBundle);
      this._localesBundle.bundleId = "locales";
      this._localesBundle.bundleURL =
        Application.prefs.getValue("songbird.url.locales", "default");
      this._localesBundle.addBundleDataListener(this);

      // Load the locales bundle data.
      try {
        this._localesBundle.retrieveBundleData
                              (this._cfg.localeBundleDataLoadTimeout);
      } catch (ex) {
        this._localesBundleDataLoadComplete = true;
        this._localesBundleDataLoadSucceeded = false;
      }
    }

    // Wait until the locale bundle data loading has completed.
    if (!this._localesBundleDataLoadComplete)
      return;

    // If the locales bundle data load failed, send an event and do nothing
    // more.
    if (!this._localesBundleDataLoadSucceeded) {
      this._widget._localeSwitchSucceeded = false;
      this._dispatchEvent(this._widget, "localeswitchcomplete");
      return;
    }

    // Set up to install the target locale.
    var bundleExtensionCount = this._localesBundle.bundleExtensionCount;
    var isTargetLocaleAvailable = false;
    for (var i = 0; i < bundleExtensionCount; i++) {
      // Get the next locale in the bundle.
      var localeName = this._localesBundle.getExtensionAttribute(i, "name");
      localeName = localeName.split(" ")[0];

      // If the locale is the target locale, mark it to be installed.
      // Otherwise, mark it to not be installed.
      if (localeName == this._targetLocale) {
        this._localesBundle.setExtensionInstallFlag(i, true);
        isTargetLocaleAvailable = true;
      } else {
        this._localesBundle.setExtensionInstallFlag(i, false);
      }
    }

    // If the target locale is not available, send an event and do nothing more.
    if (!isTargetLocaleAvailable) {
      this._widget._localeSwitchSucceeded = false;
      this._dispatchEvent(this._widget, "localeswitchcomplete");
      return;
    }

    // Install the target locale.
    var result = this._localesBundle.installFlaggedExtensions(window);
    if (result == this._localesBundle.BUNDLE_INSTALL_ERROR) {
      this._widget._localeSwitchSucceeded = false;
      this._dispatchEvent(this._widget, "localeswitchcomplete");
      return;
    }
    this._targetLocaleInstalled = true;

    // Check if an application restart is required.
    if (this._localesBundle.restartRequired)
      this._widget._appRestartRequired = true;

    // Switch to the target locale.
    this.switchLocale();
  },


  /**
   * Dispatch the event of type specified by aType with the target specified by
   * aTarget.
   *
   * \param aTarget             Target of the event.
   * \param aType               Type of event.
   */

  _dispatchEvent: function firstRunLocaleSvc__dispatchEvent(aTarget, aType) {
    // Create the event.
    var event = document.createEvent("Events");
    event.initEvent(aType, true, true);

    // Dispatch to DOM event handlers.
    var noCancel = aTarget.dispatchEvent(event);

    // Dispatch to XML attribute event handlers.
    var handler = aTarget.getAttribute("on" + aType);
    if (handler != "") {
      var func = new Function("event", handler);
      var returned = func.apply(aTarget, [event]);
      if (returned == false)
        noCancel = false;
    }

    return noCancel;
  }
};

