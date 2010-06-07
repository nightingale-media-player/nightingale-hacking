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
 * \file  firstRunEula.js
 * \brief Javascript source for the first-run wizard EULA widget services.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// First-run wizard EULA widget services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// First-run wizard EULA widget imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");


//------------------------------------------------------------------------------
//
// First-run wizard EULA widget services defs.
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

// DOM defs.
if (typeof(XUL_NS) == "undefined")
  var XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";


//------------------------------------------------------------------------------
//
// First-run wizard EULA widget services configuration.
//
//------------------------------------------------------------------------------

/*
 * localeBundleDataLoadTimeout  Timeout in milliseconds for loading the locale
 *                              bundle data.
 */

var firstRunEULASvcCfg = {
  languageNamesBundleURL: "chrome://global/locale/languageNames.properties",
  regionNamesBundleURL: "chrome://global/locale/regionNames.properties",
  localeBundleDataLoadTimeout: 15000
};



//------------------------------------------------------------------------------
//
// First-run wizard EULA widget services.
//
//------------------------------------------------------------------------------

/**
 * Construct a first-run wizard EULA widget services object for the widget
 * specified by aWidget.
 *
 * \param aWidget               First-run wizard EULA widget.
 */

function firstRunEULASvc(aWidget) {
  this._widget = aWidget;
}

// Define the object.
firstRunEULASvc.prototype = {
  // Set the constructor.
  constructor: firstRunEULASvc,

  //
  // Widget services fields.
  //
  //   _cfg                     Widget services configuration.
  //   _widget                  First-run wizard EULA widget.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _wizardElem              First-run wizard element.
  //   _wizardPageElem          First-run wizard EULA widget wizard page element.
  //   _perfTest                We're doing perf testing so don't stop for user input
  //
  //   _languageNamesBundle     Language names string bundle.
  //   _regionNamesBundle       Region names string bundle.
  //   _localeInfoTable         Table of locale information.
  //   _localeInfoList          Ordered list of locale information.
  //   _ignoreLocaleSelectEvents
  //                            If true, ignore locale selection events.
  //   _currentLocale           Current running application locale.
  //   _systemLocale            System locale.
  //   _userSelectedLocale      Locale selected by user.
  //   _appRestartRequired      True if locale selection requires an application
  //                            restart.
  //
  //   _localeBundleRunning     True if the locale bundle services are running.
  //   _localeBundle            Locale bundle object.
  //   _localeBundleDataLoadComplete
  //                            True if loading of locale bundle data is
  //                            complete.
  //   _localeBundleDataLoadSucceeded
  //                            True if loading of locale bundle data
  //                            succeeded.
  //

  _cfg: firstRunEULASvcCfg,
  _widget: null,
  _domEventListenerSet: null,
  _wizardElem: null,
  _wizardPageElem: null,
  _perfTest : false,

  _languageNamesBundle: null,
  _regionNamesBundle: null,
  _localeInfoTable: null,
  _localeInfoList: null,
  _localeIgnoreSelectEvents: false,
  _currentLocale: null,
  _systemLocale: null,
  _userSelectedLocale: null,
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

  initialize: function firstRunEULASvc_initialize() {
    var _this = this;
    var func;

    // Create a DOM event listener set.
    this._domEventListenerSet = new DOMEventListenerSet();

    // Get the first-run wizard and wizard page elements.
    this._wizardPageElem = this._widget.parentNode;
    this._wizardElem = this._wizardPageElem.parentNode;

    // Initialize the locale services.
    this._localeInitialize();

    // Listen for page show and advanced events.
    func = function() { _this._doPageShow(); };
    this._domEventListenerSet.add(this._wizardPageElem,
                                  "pageshow",
                                  func,
                                  false);
    func = function(aEvent) { _this._doPageAdvanced(aEvent); };
    this._domEventListenerSet.add(this._wizardPageElem,
                                  "pageadvanced",
                                  func,
                                  false);

    // Attach an event listener to the EULA browser.
    func = function(aEvent) { _this._onContentClick(aEvent); };
    this._domEventListenerSet.add(this._widget.eulaBrowser,
                                  "click",
                                  func,
                                  false);

    // Initialize the perf services.
    this._perfInitialize();
  },


  /**
   * Finalize the widget services.
   */

  finalize: function firstRunEULASvc_finalize() {
    // Finalize the locale services.
    this._localeFinalize();

    // Remove DOM event listeners.
    if (this._domEventListenerSet) {
      this._domEventListenerSet.removeAll();
    }
    this._domEventListenerSet = null;

    // Clear object fields.
    this._widget = null;
    this._wizardElem = null;
    this._wizardPageElem = null;
  },


  //----------------------------------------------------------------------------
  //
  // Widget event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle an accept EULA changed event.
   */

  doAcceptChanged: function firstRunEULASvc_doAcceptChanged() {
    // Update the UI.
    this._update();
  },


  /**
   * Handle a wizard page show event.
   */

  _doPageShow: function firstRunEULASvc__doPageShow() {
    // If the EULA has already been accepted or we're perf testing, skip the
    // first-run wizard EULA page.
    var eulaAccepted = Application.prefs.getValue("songbird.eulacheck", false);
    if (eulaAccepted || this._perfTest) {
      this._advance();
      return;
    }

    // Dispatch to the locale services.
    this._localeDoPageShow();

    // Update the UI.
    this._update();
  },


  /**
   * Handle the wizard page advanced event specified by aEvent.
   *
   * \param aEvent              Event to handle.
   */

  _doPageAdvanced: function firstRunEULASvc__doPageAdvanced(aEvent) {
    // Don't advance if EULA not accepted.
    var eulaAccepted = Application.prefs.getValue("songbird.eulacheck", false);
    var acceptCheckboxElem = this._getElement("accept_checkbox");
    if (!eulaAccepted && !acceptCheckboxElem.checked) {
      aEvent.preventDefault();
      return;
    }

    // Set the EULA accepted preference and flush to disk.
    Application.prefs.setValue("songbird.eulacheck", true);
    var prefService = Cc["@mozilla.org/preferences-service;1"]
                        .getService(Ci.nsIPrefService);
    prefService.savePrefFile(null);

    // Dispatch event to the locale services.
    this._localeDoPageAdvanced(aEvent);
  },


  /**
   * Handle a wizard connection reset event.
   */

  _doConnectionReset: function firstRunLocaleSvc__doConnectionReset() {
    // Re-initialize the locale bundle services.
    this._localeBundleFinalize();
    this._localeBundleInitialize();

    // Load the locale bundle.  The loading shouldn't start until after the
    // first time the first-run EULA page is shown.
    this._localeBundleLoad();
  },


  /**
   * Handle click events and forward URI loading to an external service.
   */
  _onContentClick: function(aEvent) {
    try {
      // If the event's target is a URL, pass that off to the external
      // protocol handler service since our web browser isn't available
      // during the eula stage$.
      var targetURI = Cc["@mozilla.org/network/io-service;1"]
                        .getService(Ci.nsIIOService)
                        .newURI(aEvent.target, null, null);

      var extLoader = Cc["@mozilla.org/uriloader/external-protocol-service;1"]
                        .getService(Ci.nsIExternalProtocolService);
      extLoader.loadURI(targetURI, null);
      aEvent.preventDefault();
    }
    catch (e) {
    }
  },


  //----------------------------------------------------------------------------
  //
  // Widget locale services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the widget locale services.
   */

  _localeInitialize: function firstRunEULASvc__localeInitialize() {
    // Get the locale names string bundles.
    var stringBundleService = Cc["@mozilla.org/intl/stringbundle;1"]
                                .getService(Ci.nsIStringBundleService);
    this._languageNamesBundle =
           stringBundleService.createBundle(this._cfg.languageNamesBundleURL);
    this._regionNamesBundle =
           stringBundleService.createBundle(this._cfg.regionNamesBundleURL);

    // Get the locale info.
    this._getLocaleInfo();

    // Get the current locale.
    var chromeRegistry = Cc["@mozilla.org/chrome/chrome-registry;1"]
                           .getService(Ci.nsIXULChromeRegistry);
    this._currentLocale = chromeRegistry.getSelectedLocale("songbird");

    // Get the system locale.
    var localeService = Cc["@mozilla.org/intl/nslocaleservice;1"]
                          .getService(Ci.nsILocaleService);
    this._systemLocale = localeService.getApplicationLocale()
                                      .getCategory("NSILOCALE_CTYPE");

    // Initialize the locale bundle services.
    this._localeBundleInitialize();

    // Listen for first-run wizard connection reset events.
    var _this = this;
    var func = function() { return _this._doConnectionReset(); };
    this._domEventListenerSet.add(firstRunWizard.wizardElem,
                                  "firstRunConnectionReset",
                                  func,
                                  false);
  },


  /**
   * Finalize the widget locale services.
   */

  _localeFinalize: function firstRunEULASvc__localeFinalize() {
    // Finalize the locale bundle services.
    this._localeBundleFinalize();

    // Clear object fields.
    this._languageNamesBundle = null;
    this._regionNamesBundle = null;
    this._localeInfoTable = null;
    this._localeInfoList = null;
    this._currentLocale = null;
    this._systemLocale = null;
    this._userSelectedLocale = null;
  },


  /**
   * Handle a wizard page show event.
   */

  _localeDoPageShow: function firstRunEULASvc__localeDoPageShow() {
    // Start loading the locale bundle.
    this._localeBundleStart();
  },


  /**
   * Handle the wizard page advanced event specified by aEvent.
   *
   * \param aEvent              Event to handle.
   */

  _localeDoPageAdvanced:
    function firstRunEULASvc__localeDoPageAdvanced(aEvent) {
    // Get the selected locale tag.
    var localeMenuListElem = this._getElement("locale_menulist");
    var selectedLocaleTag = localeMenuListElem.value;

    // Switch to a new locale if necessary.
    if (selectedLocaleTag != this._currentLocale) {
      // Install the new locale and switch.
      if (this._installLocale(selectedLocaleTag)) {
        // Switch to the new locale.
        switchLocale(selectedLocaleTag);

        // Set up to restart application or wizard.
        if (this._appRestartRequired)
          firstRunWizard.restartApp = true;
        else
          firstRunWizard.restartWizard = true;

        // Prevent wizard from advancing further.
        aEvent.preventDefault();

        // Close the wizard.
        document.defaultView.close();
      }
    }
  },


  /**
   * Handle a locale select event.
   */

  localeDoSelect: function firstRunEULASvc_localeDoSelect() {
    // Do nothing if ignoring selection events.
    if (this._ignoreLocaleSelectEvents)
      return;

    // Get the locale selected by the user.
    var localeMenuListElem = this._getElement("locale_menulist");
    this._userSelectedLocale = localeMenuListElem.value;
  },


  /**
   * Install the locale specified by aLocaleTag.  Return true if the locale is
   * successfully installed; return false otherwise.
   *
   * \param aLocaleTag          Tag of locale to install.
   *
   * \return                    True if locale successfully installed.
   */

  _installLocale: function firstRunEULASvc__installLocale(aLocaleTag) {
    // Check if locale is already installed.
    var chromeRegistry = Cc["@mozilla.org/chrome/chrome-registry;1"]
                           .getService(Ci.nsIToolkitChromeRegistry);
    var installedLocaleEnum = chromeRegistry.getLocalesForPackage("songbird");
    while (installedLocaleEnum.hasMore()) {
      // Get the next installed locale info.
      var installedLocaleTag = installedLocaleEnum.getNext();

      // Return if locale is installed.
      if (aLocaleTag == installedLocaleTag) {
        return true;
      }
    }

    // Can't install if no locale bundle exists.
    if (!this._localeBundle)
      return false;

    // Get the locale info.  Can't install if locale is not in the locale
    // bundle.
    if (!(aLocaleTag in this._localeInfoTable))
      return false;
    var localeInfo = this._localeInfoTable[aLocaleTag];
    if (!("bundleIndex" in localeInfo))
      return false;
    var bundleIndex = localeInfo.bundleIndex;

    // Set up to install the locale.
    var bundleExtensionCount = this._localeBundle.bundleExtensionCount;
    for (var i = 0; i < bundleExtensionCount; i++) {
      this._localeBundle.setExtensionInstallFlag(i, false);
    }
    this._localeBundle.setExtensionInstallFlag(bundleIndex, true);

    // Install the locale.
    var result = this._localeBundle.installFlaggedExtensions(window);
    if (result == Ci.sbIBundle.BUNDLE_INSTALL_ERROR)
      return false;

    // Check if an application restart is required.
    if (this._localeBundle.restartRequired)
      this._appRestartRequired = true;

    return true;
  },


  /**
   * Update the locale UI.
   */

  _localeUpdate: function firstRunEULASvc__localeUpdate() {
    // Handle any connection errors.
    //XXXeps ideally, we wouldn't handle non-connection errors as connection
    //XXXeps errors.
    if (this._localeBundleDataLoadComplete &&
        !this._localeBundleDataLoadSucceeded) {
      firstRunWizard.handleConnectionError();
    }

    // Clear the locale menu popup.
    var localeMenuPopupElem = this._getElement("locale_menupopup");
    while (localeMenuPopupElem.hasChildNodes()) {
      localeMenuPopupElem.removeChild(localeMenuPopupElem.firstChild);
    }

    // Fill the locale menu popup.
    for (var i = 0; i < this._localeInfoList.length; i++) {
      // Get the next locale info.
      var localeInfo = this._localeInfoList[i];

      // Create a locale menu item.
      var menuItemElem = document.createElementNS(XUL_NS, "menuitem");
      menuItemElem.setAttribute("label", localeInfo.displayName);
      menuItemElem.setAttribute("value", localeInfo.tag);

      // Add the locale menu item.
      localeMenuPopupElem.appendChild(menuItemElem);
    }

    // Determine the selected locale.
    var selectedLocale = this._findMatchingLocaleTag(this._userSelectedLocale);
    if (!selectedLocale)
      selectedLocale = this._findMatchingLocaleTag(this._systemLocale);
    if (!selectedLocale)
      selectedLocale = this._findMatchingLocaleTag(this._currentLocale);

    // Set the selected locale.
    var localeMenuListElem = this._getElement("locale_menulist");
    this._ignoreLocaleSelectEvents = true;
    if (selectedLocale)
      localeMenuListElem.value = selectedLocale;
    else
      localeMenuListElem.selectedIndex = 0;
    this._ignoreLocaleSelectEvents = false;
  },


  /**
   * Get info for all available locales.
   */

  _getLocaleInfo: function firstRunEULASvc__getLocaleInfo() {
    // Clear the locale info table.
    this._localeInfoTable = {};

    // Get info for all installed locales.
    var chromeRegistry = Cc["@mozilla.org/chrome/chrome-registry;1"]
                           .getService(Ci.nsIToolkitChromeRegistry);
    var localeEnum = chromeRegistry.getLocalesForPackage("songbird");
    while (localeEnum.hasMore())
    {
      // Get the locale info.
      var tag = localeEnum.getNext();
      var displayName = this._getLocaleDisplayName(tag);

      // Add the locale info.
      var localeInfo = { tag: tag, displayName: displayName };
      this._localeInfoTable[tag] = localeInfo;
    }

    // Get info for the locale bundle.
    if (this._localeBundle) {
      var bundleExtensionCount = this._localeBundle.bundleExtensionCount;
      for (var i = 0; i < bundleExtensionCount; i++) {
        // Get the next locale info in the bundle.
        var tag = this._localeBundle.getExtensionAttribute(i, "languageTag");
        var displayName = this._localeBundle.getExtensionAttribute(i, "name");

        // Add the locale info.
        var localeInfo = { tag: tag, displayName: displayName, bundleIndex: i };
        this._localeInfoTable[tag] = localeInfo;
      }
    }

    // Create an ordered locale info list.
    this._localeInfoList = [];
    for (var localeTag in this._localeInfoTable) {
      this._localeInfoList.push(this._localeInfoTable[localeTag]);
    }

    // Sort the ordered locale info list by display name.
    var sortFunc = function(a, b) {
      if (a.displayName > b.displayName)
        return 1;
      if (a.displayName < b.displayName)
        return -1;
      return 0;
    }
    this._localeInfoList.sort(sortFunc);
  },


  /**
   *   Return a locale tag from the locale info table that matches the locale
   * tag specified by aLocaleTag.  Return null, if no matching tag could be
   * found.  aLocaleTag may be null or empty.
   *   If the specified tag is not in the locale table, but its language tag is,
   * the language tag is returned (e.g., "en" will be returned for "en-US").
   *
   * \param aLocaleTag          Tag for which to find a matching tag.
   *
   * \return                    Matching tag.
   */

  _findMatchingLocaleTag:
    function firstRunEULASvc__findMatchingLocaleTag(aLocaleTag) {
    // Return null if no locale tag specified.
    if (!aLocaleTag)
      return null;

    // Return the locale tag if it's in the locale info table.
    if (aLocaleTag in this._localeInfoTable)
      return aLocaleTag;

    // Return the locale language tag if it's in the locale info table.
    var languageTag = aLocaleTag.split("-")[0];
    if (languageTag in this._localeInfoTable)
      return languageTag;

    return null;
  },


  /**
   * Return the display name for the locale tag specified by aLocaleTag.
   *
   * \param aLocaleTag          Locale tag for which to get display name.
   *
   * \return                    Locale display name.
   */

  _getLocaleDisplayName:
    function firstRunEULASvc__getLocaleDisplayName(aLocaleTag) {
    // Get the locale language and region tags.
    var localeTagPartList = aLocaleTag.split("-");
    var languageTag = localeTagPartList[0];
    var regionTag = "";
    if (localeTagPartList.length > 1)
      regionTag = localeTagPartList[1];

    // Get the language and region display names.
    var languageDisplayName = SBString(languageTag,
                                       "",
                                       this._languageNamesBundle);
    var regionDisplayName = "";
    if (regionTag)
      regionDisplayName = SBString(regionTag, "", this._regionNamesBundle);

    // Get the locale display name.
    var localeDisplayName = "";
    if (languageDisplayName && regionDisplayName) {
      localeDisplayName = SBFormattedString
                            ("locale.language_region.display_name",
                             [ languageDisplayName, regionDisplayName ],
                             "");
    } else if (languageDisplayName) {
      localeDisplayName = SBFormattedString
                            ("locale.language.display_name",
                             [ languageDisplayName ],
                             "");
    }
    if (!localeDisplayName)
      localeDisplayName = aLocaleTag;

    return localeDisplayName;
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

  onDownloadComplete: function firstRunEULASvc__onDownloadComplete(aBundle) {
    this._localeBundleDataLoadComplete = true;
    this._localeBundleDataLoadSucceeded = true;
    this._getLocaleInfo();
    this._update();
  },


  /**
   * \brief Bundle download error callback
   * This method is called upon error while downloading the bundle data
   * \param bundle An interface to the bundle manager that triggered the event
   * \sa onDownloadComplete, sbIBundle
   */

  onError: function firstRunEULASvc__onError(aBundle) {
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
    function firstRunEULASvc__localeBundleInitialize() {
    // Initialize the locale bundle fields.
    this._localeBundle = null;
    this._localeBundleDataLoadComplete = false;
    this._localeBundleDataLoadSucceeded = false;
  },


  /**
   * Finalize the locale bundle services.
   */

  _localeBundleFinalize:
    function firstRunEULASvc__localeBundleFinalize() {
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

  _localeBundleStart: function firstRunEULASvc__localeBundleStart() {
    // Mark the locale bundle services running and load the locale bundle.
    this._localeBundleRunning = true;
    this._localeBundleLoad();
  },


  /**
   * Load the locale bundle.
   */

  _localeBundleLoad: function firstRunEULASvc__localeBundleLoad() {
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
  },


  /**
   * Cancel loading of the locale bundle.
   */

  _localeBundleLoadCancel:
    function firstRunEULASvc__localeBundleLoadCancel() {
    // Finalize locale bundle.
    //XXXeps need way to cancel it.
    if (this._localeBundle)
      this._localeBundle.removeBundleDataListener(this);
  },


  //----------------------------------------------------------------------------
  //
  // Widget perf services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the perf services.
   */

  _perfInitialize: function firstRunEULASvc__perfInitialize() {
    // Get the perf test setting.
    this._perfTest = window.arguments[0].perfTest;

    try
    {
      /**
       *
       * Start and stop so we get a timestamp for when the EULA is displayed
       * We only care about the start time to calc from app start to EULA
       */
      var timingService = Cc["@songbirdnest.com/Songbird/TimingService;1"]
                          .getService(Ci.sbITimingService);
      timingService.startPerfTimer("CSPerfEULA");
      timingService.stopPerfTimer("CSPerfEULA");
    }
    catch (e)
    {
      // Ignore errors
    }
    if (this._perfTest) {
      var acceptCheckboxElem = this._getElement("accept_checkbox");
      acceptCheckboxElem.checked = true;
      this._update();
    }
  },


  //----------------------------------------------------------------------------
  //
  // Internal widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the UI.
   */

  _update: function firstRunEULASvc__update() {
    // Do nothing if wizard page is not being shown.
    if (this._wizardElem.currentPage != this._wizardPageElem)
      return;

    // If the EULA has already been accepted or we're perf testing, skip the
    // first-run wizard EULA page.
    var eulaAccepted = Application.prefs.getValue("songbird.eulacheck", false);
    if (eulaAccepted || this._perfTest) {
      this._advance();
      return;
    }

    // Update the locale UI.
    this._localeUpdate();

    // Only allow the first-run wizard to advance if the accept EULA checkbox is
    // checked.
    var acceptCheckboxElem = this._getElement("accept_checkbox");
    this._wizardElem.canAdvance = acceptCheckboxElem.checked;
  },


  /**
   * Advance to the next wizard page.  Only advance once if called multiple
   * times.
   */

  _advance: function firstRunEULASvc__advance() {
    // Only advance if this wizard page is the current wizard page.
    if (this._wizardElem.currentPage == this._wizardPageElem) {
      this._wizardElem.canAdvance = true;
      this._wizardElem.advance();
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

  _getElement: function firstRunEULASvc__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "anonid",
                                                   aElementID);
  }
};

