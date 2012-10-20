/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

/**
* \file  firstRunWizard.js
* \brief Javascript source for the first-run wizard dialog.
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// First-run wizard dialog.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// First-run wizard dialog imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/SBUtils.jsm");

//------------------------------------------------------------------------------
//
// First-run wizard dialog defs.
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

var SBLanguage = {
    DEFAULT_LOCALE: "en-US",
    locale: "",
    firstRunWizard: null,
    getCountry: function() {
      /** ******************************************** */
      /* try to get the country from the preferences */
      /** ******************************************** */
       // Default country is United States
       var localeVal = SBLanguage.DEFAULT_LOCALE; // Default to english
       var regCountryVal = "1033";
       
       var langLocaleMap = {
           "1046": "pt-BR",
           "2052": "zh-CN",
           "1029":"cs",
           "1030":"da",
           "1035":"fi",
           "1036":"fr",
           "1031":"de",
           "1032":"el",
           "1038":"hu",
           "1040":"it",
           "1041":"ja",
           "1042":"ko",
           "1025":"ar",
           "1043":"nl",
           "1044":"nb-NO",
           "1045":"pl",
           "2070":"pt",
           "1049":"ru",
           "1051":"sk",
           "1034":"es-ES",
           "1053":"sv-SE" ,
           "1028":"zh-TW",
           "1054":"th",
           "1055":"tr",
           "1033":"en-US" 
       };
       try {
         var wrk = Cc["@mozilla.org/windows-registry-key;1"]
           .createInstance(Ci.nsIWindowsRegKey);
         if (wrk) {
           wrk.open(wrk.ROOT_KEY_LOCAL_MACHINE,
               "SOFTWARE\\Songbird",
               wrk.ACCESS_READ);
           if (wrk.hasValue("Installer Country")) {
             regCountryVal = wrk.readStringValue("Installer Country");
           }
           wrk.close();
         }
       }
       catch (e) {
         // Just continue if there is an error
       }
     
       // Convert the country code in country name
       if ( !isNaN(regCountryVal)) {
         localeVal = langLocaleMap["" + regCountryVal];
       }
       return localeVal;
    },
     
    exists: function (array, value) {
      for (i in array) {
        if (array[i] == value)
          return true;
      }
      return false;
    },
    menubar_locales_bundle: null,
    menubarLocalesBundleCB: {
      onDownloadComplete: function(bundle) {
        try {
          for (var i=0; i < bundle.bundleExtensionCount; i++) {
            var thisLocale = bundle.getExtensionAttribute(i, "languageTag");
            if (thisLocale == SBLanguage.locale) {
              for (var x = 0; x < bundle.bundleExtensionCount; x++) {
                bundle.setExtensionInstallFlag(x, x == i);
              }
              var res = bundle.installFlaggedExtensions(window);
              
              if (res == bundle.BUNDLE_INSTALL_SUCCESS) {
                SBLanguage.firstRunWizard._markFirstRunComplete = false;
                switchLocale(SBLanguage.locale, false);
                restartApp();
              } 
              else {
                gPrompt.alert( window, 
                              SBString( "locales.installfailed.title", "Language Download" ),
                              SBString( "locales.installfailed.msg", "Language installation failed, check your network connectivity!" ) );
              }
              break;
            }
          }
        }
        catch (e)
        {
          dump("Exception: " + e.toString() + "\n");
        }
      },
      onError: function(bundle) { dump("DWB: Error downloading bundles\n"); },
      QueryInterface : function(aIID) {
        if (!aIID.equals(Components.interfaces.sbIBundleDataListener) &&
            !aIID.equals(Components.interfaces.nsISupports)) 
        {
          throw Components.results.NS_ERROR_NO_INTERFACE;
        }
        return this;
      }
    },
    loadLocaleFromRegistry : function(firstRunWizard) {
      SBLanguage.firstRunWizard = firstRunWizard;
      try {
        SBLanguage.locale = SBLanguage.getCountry();
        var prefLocaleVal = Application.prefs.getValue("general.useragent.locale" , "");
        var prefLocaleFirstRun = Application.prefs.getValue("general.useragent.locale.firstrun", true);
        Application.prefs.setValue("general.useragent.locale.firstrun", false);
        // We only want to do this on first run, else if the users switches the
        // language we'll
        // end up switching it back here
        if (prefLocaleFirstRun && SBLanguage.locale != prefLocaleVal) {
          switchLocale(SBLanguage.locale);
          try {
            var sbIBundle = new Components.Constructor("@songbirdnest.com/Songbird/Bundle;1", "sbIBundle");
            menubar_locales_bundle = new sbIBundle();
            menubar_locales_bundle.bundleId = "locales";
            menubar_locales_bundle.bundleURL = Application.prefs.getValue("songbird.url.locales", "default");
            menubar_locales_bundle.addBundleDataListener(this.menubarLocalesBundleCB);
            menubar_locales_bundle.retrieveBundleData(MENUBAR_LOCALESBUNDLE_TIMEOUT);
          } catch ( err ) {
            SB_LOG("initLocalesBundle", "" + err );
          }
        }
      }
      catch (e)
      {
        dump("Exception:" + e.toString() + "\n");
      }
    }     
}

//------------------------------------------------------------------------------
//
// First-run wizard dialog services.
//
//------------------------------------------------------------------------------

var firstRunWizard = {
  //
  // Public first-run wizard fields.
  //
  //   wizardElem               Wizard element.
  //   restartApp               Restart application on wizard close.
  //   restartWizard            Restart wizard on wizard close.
  //

  restartApp: false,
  restartWizard: false,


  //
  // Internal first-run wizard fields.
  //
  //   _initialized             True if these services have been initialized.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _savedSettings           True if settings have been saved.
  //   _markFirstRunComplete    True if first-run should be marked as complete
  //                            when wizard exits.
  //   _connectionErrorHandled  True if a connection error has been handled.
  //

  _initialized: false,
  _domEventListenerSet: null,
  _savedSettings: false,
  _markFirstRunComplete: false,
  _connectionErrorHandled: false,


  //----------------------------------------------------------------------------
  //
  // Public first-run wizard field getters/setters.
  //
  //----------------------------------------------------------------------------

  //
  // wizardElem
  //

  _wizardElem: null,

  get wizardElem() {
    if (!this._wizardElem)
      this._wizardElem = document.getElementById("first_run_wizard");
    return this._wizardElem;
  },


  //----------------------------------------------------------------------------
  //
  // Event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle a load event.
   */

  doLoad: function firstRunWizard_doLoad() {
    // Initialize the services.
    this._initialize();
  },


  /**
   * Handle an unload event.
   */

  doUnload: function firstRunWizard_doUnload() {
    // Indicate that the first-run checks have been made.
    if (this._markFirstRunComplete) {
      // Set the first-run check preference and flush to disk.
      Application.prefs.setValue("songbird.firstrun.check.0.3", true);
      var prefService = Cc["@mozilla.org/preferences-service;1"]
                          .getService(Ci.nsIPrefService);
      prefService.savePrefFile(null);
    }

    // Indicate that the wizard is complete and whether it should be restarted.
    this._firstRunData.onComplete(this.restartWizard);

    // Finalize the services.
    this._finalize();

    // Restart application as specified. (AFTER we're done finalizing)
    if (this.restartApp)
      restartApp();
  },


  /**
   * Handle a wizard finish event.
   */

  doFinish: function firstRunWizard_doFinish() {
    // Record our time startup pref
    try {
        var timingService = Cc["@songbirdnest.com/Songbird/TimingService;1"]
                              .getService(Ci.sbITimingService);
        timingService.startPerfTimer("CSPerfEndEULA");
        timingService.stopPerfTimer("CSPerfEndEULA");
    } catch (e) {
        dump("Timing service exception: " + e);
        // Ignore errors
    }

    // Save wizard settings.
    this._saveSettings();
  },


  /**
   * Handle a wizard cancel event.
   */

  doCancel: function firstRunWizard_doCancel() {
  },


  /**
   * Handle a wizard quit event.
   */

  doQuit: function firstRunWizard_doQuit() {
    // Defer invocation of quitApp to outside this even handler so that this
    // window may be immediately closed by quitApp.  If this window doesn't
    // close, it will prevent the application from quitting.
    // See "http://bugzilla.songbirdnest.com/show_bug.cgi?id=21890".
    SBUtils.deferFunction(quitApp);
  },


  /**
   * Handle a wizard page show event.
   */

  doPageShow: function firstRunWizard_doPageShow() {
    // Initialize the services.  Page show can occur before load.
    this._initialize();

    // Update the UI.
    this.update();
  },
  
  /**
   * Return the next proxy source that we can try to import from.
   * \return                    The id of the next proxy import source or null.
   */
  
  _getNextProxyImport: function 
    firstRunWizard_getNextProxyImport(aProxyImport, aLastImportId) {
    var importSources = aProxyImport.importSources.enumerate();
    while (importSources.hasMoreElements()) {
      var id = importSources.getNext()
                            .QueryInterface(Ci.nsISupportsString) + '';
      if (!aLastImportId)
        return id;
      if (id == aLastImportId) {
        if (importSources.hasMoreElements())
          return importSources.getNext()
                 .QueryInterface(Ci.nsISupportsString) + '';
        return null;
      }
    }
    return null;
  },
  
  /**
   * Try to import the proxy settings from the next available source
   * \return                    True if a new set of proxy settings was loaded.
   */
  
  _tryNextProxyImport: function firstRunWizard_tryNextProxyImport() {
    var proxyImport = Cc["@songbirdnest.com/Songbird/NetworkProxyImport;1"]
                        .getService(Ci.sbINetworkProxyImport);
    var lastImportId = this._lastProxyImport;
    var importId;
    var success = false;
    while (!success) {
      importId = this._getNextProxyImport(proxyImport, lastImportId);
      if (!importId)
        return false;
      success = proxyImport.importProxySettings(importId);
    }
    this._lastProxyImport = importId;
    return true;
  },
  
  /**
   * Reset the proxy settings
   */
  
  _resetProxySettings: function firstRunWizard_resetProxySettings() {
    var proxyData = [
      ["network.proxy.autoconfig_url", ""],
      ["network.proxy.type", 0],
      ["network.proxy.ftp", ""],
      ["network.proxy.ftp_port", 0],
      ["network.proxy.gopher", ""],
      ["network.proxy.gopher_port", 0],
      ["network.proxy.http", ""],
      ["network.proxy.http_port", 0],
      ["network.proxy.ssl", ""],
      ["network.proxy.ssl_port", 0],
      ["network.proxy.socks", ""],
      ["network.proxy.socks_port", 0],
      ["network.proxy.share_proxy_settings", false],
      ["network.proxy.no_proxies_on", "127.0.0.1;localhost"]
    ];
    for (var i in proxyData) {
      Application.prefs.setValue(proxyData[i][0], proxyData[i][1]);
    }
  },

  /**
   * Handle internet connection errors.
   *
   * \return                    True if error was handled.
   */

  handleConnectionError: function firstRunWizard_handleConnectionError() {
    // Only handle connection errors once.
    if (this._connectionErrorHandled)
      return false;

    // Defer handling of the connection error until after the current event
    // completes.  This allows the calling wizard page to complete any updates
    // before the first-run wizard connection page is presented.
    var _this = this;
    var func = function() { _this._handleConnectionError(); };
    SBUtils.deferFunction(func);

    return true;
  },

  _handleConnectionError: function firstRunWizard__handleConnectionError() {
    // Only handle connection errors once.
    if (this._connectionErrorHandled)
      return;

    if (this._tryNextProxyImport()) {
      // retry whatever caused the connection failure
      var event = document.createEvent("Events");
      event.initEvent("firstRunConnectionReset", true, true);
      this.wizardElem.dispatchEvent(event); 
      // reload the same page
      this.wizardElem.goTo(this.wizardElem.currentPage.pageid);
      return;
    } else {
      this._resetProxySettings();
    }
    
    this._connectionErrorHandled = true;

    // Suppress the wizard page advanced event sent when advancing to the
    // first-run wizard connection page.  The wizard is not really advancing.
    var func = function(aEvent) { aEvent.stopPropagation(); };
    this._domEventListenerSet.add(this.wizardElem,
                                  "pageadvanced",
                                  func,
                                  true,
                                  true);

    // Go to the first-run wizard connection page.
    this.wizardElem.canAdvance = true;
    this.wizardElem.advance("first_run_connection_page");

    // A connection error has been handled.
    this._connectionErrorHandled = true;
  },


  //----------------------------------------------------------------------------
  //
  // UI services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the UI.
   */

  update: function firstRunWizard_update() {
    // Get the current wizard page.
    var currentPage = this.wizardElem.currentPage;

    // Set to mark first-run complete if showing welcome page.
    if (currentPage.id == "first_run_welcome_page")
      this._markFirstRunComplete = true;

    // If we're perf testing then we want to just after the EULA is done
    if (window.arguments[0].perfTest && this._markFirstRunComplete) {
        finishButton.click();
    }
  },

  /**
   * Open a URL in an external window
   * @param aURL: the URL to open
   */
  
  openURL: function firstRunWizard_openURL(aURL) {
    var uri = null;
    if (aURL instanceof Ci.nsIURI) {
      uri = aURL;
    }
    else {
      uri = Cc["@mozilla.org/network/io-service;1"]
              .getService(Ci.nsIIOService)
              .newURI(aURL, null, null);
    }
    Cc["@mozilla.org/uriloader/external-protocol-service;1"]
      .getService(Ci.nsIExternalProtocolService)
      .loadURI(uri);
  },

  //----------------------------------------------------------------------------
  //
  // Internal services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the first-run wizard dialog services.
   */

  _initialize: function firstRunWizard__initialize() {
    // Do nothing if already initialized.
    if (this._initialized)
      return;

    SBLanguage.loadLocaleFromRegistry(this);
    
    // Get the wizard element.
    this.wizardElem = document.getElementById("first_run_wizard");

    // Create a DOM event listener set.
    this._domEventListenerSet = new DOMEventListenerSet();

    // Grap the param block
    var dialogPB = 
      window.arguments[0].QueryInterface(Ci.nsIDialogParamBlock);

    // Unwrap the object coming from the application startup service
    this._firstRunData = 
      dialogPB.objects
              .queryElementAt(0, Ci.nsISupportsInterfacePointer)
              .data
              .wrappedJSObject;

    // Services are now initialized.
    this._initialized = true;
    var skipToPage = Application.prefs.getValue("songbird.firstrun.goto", null);
    if (skipToPage) {
      Application.prefs.get("songbird.firstrun.goto").reset();
      this.wizardElem.advance(skipToPage);
      return;
    }
    // we want to skip the EULA if we're on Windows, as the Windows' installer 
    // has already presented it
    if (getPlatformString() == "Windows_NT") {
      Application.prefs.setValue("songbird.eulacheck", true);
    }
  },


  /**
   * Finalize the first-run wizard dialog services.
   */

  _finalize: function firstRunWizard__finalize() {
    // Remove DOM event listeners.
    if (this._domEventListenerSet)
      this._domEventListenerSet.removeAll();
    this._domEventListenerSet = null;
  },


  /**
   * Save settings from all wizard pages.
   */

  _saveSettings: function firstRunWizard__saveSettings() {
    // Do nothing if settings already saved.
    if (this._savedSettings)
      return;

    // Get all first-run wizard page elements.
    var firstRunWizardPageElemList =
          DOMUtils.getElementsByAttribute(this.wizardElem,
                                          "firstrunwizardpage",
                                          "true");

    // Save settings for each first-run wizard page.
    for (var i = 0; i < firstRunWizardPageElemList.length; i++) {
      // Invoke the page saveSettings method.
      var firstRunWizardPageElem = firstRunWizardPageElemList[i];
      if (typeof(firstRunWizardPageElem.saveSettings) == "function")
        firstRunWizardPageElem.saveSettings();
    }
    // Settings have now been saved.
    this._savedSettings = true;
  }
};

