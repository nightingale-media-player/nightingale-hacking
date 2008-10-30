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
// First-run wizard locale widget imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");


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
 * localeBundleDataLoadTimeout  Timeout in milliseconds for loading the locale
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
  //   _wizardPageElem          First-run wizard locale widget wizard page
  //                            element.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _targetLocale            Target first-run locale.
  //   _targetLocaleInstalled   True if target first-run locale is installed.
  //   _targetLocaleInstallFailed
  //                            True if target locale installation failed.
  //   _localeSwitchRequired    True if a locale switch is required.
  //   _localeSwitchSucceeded   True if a locale switch completed successfully.
  //   _appRestartRequired      True if an application restart is required for
  //                            a locale switch.
  //   _localeBundleRunning     True if the locale bundle services are running.
  //   _localeBundle            Locale bundle object.
  //   _localeBundleDataLoadComplete
  //                            True if loading of locale bundle data is
  //                            complete.
  //   _localeBundleDataLoadSucceeded
  //                            True if loading of locale bundle data
  //                            succeeded.
  //

  _cfg: firstRunLocaleSvcCfg,
  _widget: null,
  _wizardPageElem: null,
  _domEventListenerSet: null,
  _targetLocale: null,
  _targetLocaleInstalled: false,
  _targetLocaleInstallFailed: false,
  _localeSwitchRequired: false,
  _localeSwitchSucceeded: false,
  _appRestartRequired: false,
  _localeBundleRunning: false,
  _localeBundle: null,
  _localeBundleDataLoadComplete: false,
  _localeBundleDataLoadSucceeded: false,


  //----------------------------------------------------------------------------
  //
  // Widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the widget services.
   */

  initialize: function firstRunLocaleSvc_initialize() {
    var _this = this;
    var func;

    // Get the first-run wizard page element.
    this._wizardPageElem = this._widget.parentNode;

    // Create a DOM event listener set.
    this._domEventListenerSet = new DOMEventListenerSet();

    // Initialize the locale bundle services.
    this._localeBundleInitialize();

    // Listen for page show and hide events.
    var func = function() { return _this._doPageShow(); };
    this._domEventListenerSet.add(this._wizardPageElem,
                                  "pageshow",
                                  func,
                                  false);
    var func = function() { return _this._doPageHide(); };
    this._domEventListenerSet.add(this._wizardPageElem,
                                  "pagehide",
                                  func,
                                  false);

    // Listen for first-run wizard connection reset events.
    func = function() { return _this._doConnectionReset(); };
    this._domEventListenerSet.add(firstRunWizard.wizardElem,
                                  "firstRunConnectionReset",
                                  func,
                                  false);

    // Check the locale.
    this._checkLocale();
  },


  /**
   * Finalize the widget services.
   */

  finalize: function firstRunLocaleSvc_finalize() {
    // Remove DOM event listeners.
    if (this._domEventListenerSet) {
      this._domEventListenerSet.removeAll();
    }
    this._domEventListenerSet = null;

    // Finalize the locale bundle services.
    this._localeBundleFinalize();

    // Clear object fields.
    this._widget = null;
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
    this._localeBundleDataLoadComplete = true;
    this._localeBundleDataLoadSucceeded = true;
    this._update();
  },


  /**
   * \brief Bundle download error callback
   * This method is called upon error while downloading the bundle data
   * \param bundle An interface to the bundle manager that triggered the event
   * \sa onDownloadComplete, sbIBundle
   */

  onError: function firstRunLocaleSvc__onError(aBundle) {
    this._localeBundleDataLoadComplete = true;
    this._localeBundleDataLoadSucceeded = false;
    this._update();
  },


  //----------------------------------------------------------------------------
  //
  // Widget locale bundle services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the locale bundle services.
   */

  _localeBundleInitialize:
    function firstRunLocaleSvc__localeBundleInitialize() {
    // Initialize the locale bundle fields.
    this._localeBundle = null;
    this._localeBundleDataLoadComplete = false;
    this._localeBundleDataLoadSucceeded = false;
  },


  /**
   * Finalize the locale bundle services.
   */

  _localeBundleFinalize:
    function firstRunLocaleSvc__localeBundleFinalize() {
    // Finalize locale bundle.
    //XXXeps need way to cancel it.
    if (this._localeBundle)
      this._localeBundle.removeBundleDataListener(this);

    // Reset the locale bundle fields.
    this._localeBundleDataLoadComplete = false;
    this._localeBundleDataLoadSucceeded = false;

    // Clear locale bundle object fields.
    this._localeBundle = null;
  },


  /**
   * Start running the locale bundle services.
   */

  _localeBundleStart: function firstRunLocaleSvc__localeBundleStart() {
    // Mark the locale bundle services running and load the locale bundle.
    this._localeBundleRunning = true;
    this._localeBundleLoad();
  },


  /**
   * Load the locale bundle.
   */

  _localeBundleLoad: function firstRunLocaleSvc__localeBundleLoad() {
    // Do nothing if not running.
    if (!this._localeBundleRunning)
      return;

    // Start loading the locale bundle data.
    if (!this._localeBundle) {
      // Set up the locale bundle for loading.
      this._localeBundle = Cc["@songbirdnest.com/Songbird/Bundle;1"]
                             .createInstance(Ci.sbIBundle);
      this._localeBundle.bundleId = "locales";
      this._localeBundle.bundleURL =
        Application.prefs.getValue("songbird.url.locales", "default");
      this._localeBundle.addBundleDataListener(this);

      // Start loading the locale bundle data.
      try {
        this._localeBundle.retrieveBundleData
                             (this._cfg.localeBundleDataLoadTimeout);
      } catch (ex) {
        // Report the exception as an error.
        Components.utils.reportError(ex);

        // Indicate that the locale bundle loading failed.
        this._localeBundleDataLoadComplete = true;
        this._localeBundleDataLoadSucceeded = false;
      }
    }

    // Update the UI.
    this._update();
  },


  /**
   * Cancel loading of the locale bundle.
   */

  _localeBundleLoadCancel:
    function firstRunLocaleSvc__localeBundleLoadCancel() {
    // Finalize locale bundle.
    //XXXeps need way to cancel it.
    if (this._localeBundle)
      this._localeBundle.removeBundleDataListener(this);
  },


  //----------------------------------------------------------------------------
  //
  // Widget event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle a wizard page show event.
   */

  _doPageShow: function firstRunLocaleSvc__doPageShow() {
    // Start the locale bundle services.
    this._localeBundleStart();

    // Update the UI.
    this._update();
  },


  /**
   * Handle a wizard page hide event.
   */

  _doPageHide: function firstRunLocaleSvc__doPageHide() {
    // Cancel locale bundle load.
    this._localeBundleLoadCancel();

    return true;
  },


  /**
   * Handle a wizard connection reset event.
   */

  _doConnectionReset: function firstRunLocaleSvc__doConnectionReset() {
    // Re-initialize the locale bundle services.
    this._localeBundleFinalize();
    this._localeBundleInitialize();

    // Load the locale bundle.  The loading shouldn't start until after the
    // first time the first-run locale page is shown.
    this._localeBundleLoad();
  },


  //----------------------------------------------------------------------------
  //
  // Internal widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the UI.
   */

  _update: function firstRunLocaleSvc__update() {
    // If first-run locale wizard page is not showing, do nothing more.
    if (firstRunWizard.wizardElem.currentPage != this._wizardPageElem)
      return;

    // If no locale switch is required, advance to the next page and do nothing
    // more.
    if (!this._localeSwitchRequired) {
      firstRunWizard.wizardElem.canAdvance = true;
      firstRunWizard.wizardElem.advance();
      return;
    }

    // Handle any connection errors and do nothing more.
    //XXXeps ideally, we wouldn't handle non-connection errors as connection
    //XXXeps errors.
    if (this._localeBundleDataLoadComplete &&
        !this._localeBundleDataLoadSucceeded) {
      // Handle the connection error.  If it's not handled, advance to the next
      // page.
      if (!firstRunWizard.handleConnectionError()) {
        firstRunWizard.wizardElem.canAdvance = true;
        firstRunWizard.wizardElem.advance();
      }
      return;
    }

    // Install the target locale when the locale bundle has been loaded.
    if (!this._targetLocaleInstalled && this._localeBundleDataLoadComplete)
      this._installTargetLocale();

    // If the locale installation failed, advance to the next page and do
    // nothing more.
    if (this._targetLocaleInstallFailed) {
      firstRunWizard.wizardElem.canAdvance = true;
      firstRunWizard.wizardElem.advance();
      return;
    }

    // If the target locale is installed, switch to it and do nothing more.
    if (this._targetLocaleInstalled) {
      this._switchLocale();
      return;
    }

    // Prevent page advancement if loading locale bundle.
    if (this._localeBundle && !this._localeBundleDataLoadComplete)
      firstRunWizard.wizardElem.canAdvance = false;
  },


  /**
   * Switch to the target locale.
   */

  _switchLocale: function firstRunLocaleSvc__switchLocale() {
    // Switch locale.
    switchLocale(this._targetLocale);

    // Determine whether to restart app or just the wizard.
    if (this._appRestartRequired)
      firstRunWizard.restartApp = true;
    else
      firstRunWizard.restartWizard = true;

    // Close the wizard.
    document.defaultView.close();
  },


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
      this._localeSwitchRequired = false;
      return;
    }
    this._localeSwitchRequired = true;

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
    // Set up to install the target locale.
    var bundleExtensionCount = this._localeBundle.bundleExtensionCount;
    var isTargetLocaleAvailable = false;
    for (var i = 0; i < bundleExtensionCount; i++) {
      // Get the next locale in the bundle.
      var localeName = this._localeBundle.getExtensionAttribute(i,
                                                                "languageTag");

      // If the locale is the target locale, mark it to be installed.
      // Otherwise, mark it to not be installed.
      if (localeName == this._targetLocale) {
        this._localeBundle.setExtensionInstallFlag(i, true);
        isTargetLocaleAvailable = true;
      } else {
        this._localeBundle.setExtensionInstallFlag(i, false);
      }
    }

    // If the target locale is not available, mark failure and do nothing more.
    if (!isTargetLocaleAvailable) {
      this._targetLocaleInstallFailed = true;
      return;
    }

    // Install the target locale.
    var result = this._localeBundle.installFlaggedExtensions(window);
    if (result == this._localeBundle.BUNDLE_INSTALL_ERROR) {
      this._targetLocaleInstallFailed = true;
      return;
    }
    this._targetLocaleInstalled = true;

    // Check if an application restart is required.
    if (this._localeBundle.restartRequired)
      this._appRestartRequired = true;
  }
};

