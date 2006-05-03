//@line 36 "/cygdrive/c/builds/tinderbox/Fx-Mozilla1.8/WINNT_5.2_Depend/mozilla/browser/components/preferences/privacy.js"

var gPrivacyPane = {
  _inited: false,
  tabSelectionChanged: function (event)
  {
    if (event.target.localName != "tabpanels" || !this._inited)
      return;
    var privacyPrefs = document.getElementById("privacyPrefs");
    var preference = document.getElementById("browser.preferences.privacy.selectedTabIndex");
    preference.valueFromPreferences = privacyPrefs.selectedIndex;
  },
  
  _sanitizer: null,
  init: function ()
  {
    this._inited = true;
    var privacyPrefs = document.getElementById("privacyPrefs");
    var preference = document.getElementById("browser.preferences.privacy.selectedTabIndex");
    if (preference.value !== null)
      privacyPrefs.selectedIndex = preference.value;
    
    // Update the clear buttons
    if (!this._sanitizer)
      this._sanitizer = new Sanitizer();
    this._updateClearButtons();
    
    window.addEventListener("unload", this.uninit, false);
    
    // Update the MP buttons
    this.updateMasterPasswordButton();
  },
  
  uninit: function ()
  {
    if (this._updateInterval != -1)
      clearInterval(this._updateInterval);
  },
  
  _updateInterval: -1,
  _updateClearButtons: function ()
  {
    var buttons = document.getElementsByAttribute("item", "*");
    var buttonCount = buttons.length;
    for (var i = 0; i < buttonCount; ++i)
      buttons[i].disabled = !this._sanitizer.canClearItem(buttons[i].getAttribute("item"));
  },

//@line 93 "/cygdrive/c/builds/tinderbox/Fx-Mozilla1.8/WINNT_5.2_Depend/mozilla/browser/components/preferences/privacy.js"
  showSanitizeSettings: function ()
  {
    document.documentElement.openSubDialog("chrome://browser/content/preferences/sanitize.xul",
                                           "", null);
  },

  _enableRestrictedWasChecked: false,
  writeEnableCookiesPref: function ()
  { 
    var enableCookies = document.getElementById("enableCookies");
    if (enableCookies.checked) {
      var enableRestricted = document.getElementById("enableCookiesForOriginating");
      this._enableRestrictedWasChecked = enableRestricted.checked;
      return this._enableRestrictedWasChecked ? 1 : 0;
    }
    return 2;
  },

  readEnableCookiesPref: function ()
  {
    var pref = document.getElementById("network.cookie.cookieBehavior");
    var enableRestricted = document.getElementById("enableCookiesForOriginating");    
    enableRestricted.disabled = pref.value == 2;
    this.enableCookiesChanged();
    return (pref.value == 0 || pref.value == 1);
  },
  
  readEnableRestrictedPref: function ()
  {
    var pref = document.getElementById("network.cookie.cookieBehavior");
    return pref.value != 2 ? pref.value : this._enableRestrictedWasChecked;
  },

  enableCookiesChanged: function ()
  {
    var preference = document.getElementById("network.cookie.cookieBehavior");
    var blockFutureCookies = document.getElementById("network.cookie.blockFutureCookies");
    blockFutureCookies.disabled = preference.value == 2;
    var lifetimePref = document.getElementById("network.cookie.lifetimePolicy");
    lifetimePref.disabled = preference.value == 2;
  },
  
  readCacheSizePref: function ()
  {
    var preference = document.getElementById("browser.cache.disk.capacity");
    return preference.value / 1000;
  },
  
  writeCacheSizePref: function ()
  {
    var cacheSize = document.getElementById("cacheSize");
    var intValue = parseInt(cacheSize.value, 10) + '';
    return intValue != "NaN" ? intValue * 1000 : 0;
  },
  
  _sanitizer: null,
  clear: function (aButton) 
  {
    var category = aButton.getAttribute("item");
    this._sanitizer.clearItem(category);
    if (category == "cache")
      aButton.disabled = true;
    else
      aButton.disabled = !this._sanitizer.canClearItem(category);
    if (this._updateInterval == -1)
      this._updateInterval = setInterval("gPrivacyPane._updateClearButtons()", 10000);
  },
  
  viewCookies: function (aCategory) 
  {
    document.documentElement.openWindow("Browser:Cookies",
                                        "chrome://browser/content/preferences/cookies.xul",
                                        "", null);
  },
  viewDownloads: function (aCategory) 
  {
    document.documentElement.openWindow("Download:Manager", 
                                        "chrome://mozapps/content/downloads/downloads.xul",
                                        "", null);
  },
  viewPasswords: function (aCategory) 
  {
    document.documentElement.openWindow("Toolkit:PasswordManager",
                                        "chrome://passwordmgr/content/passwordManager.xul",
                                        "", null);
  },
  
  viewCookieExceptions: function ()
  {
    var bundlePreferences = document.getElementById("bundlePreferences");
    var params = { blockVisible   : true, 
                   sessionVisible : true, 
                   allowVisible   : true, 
                   prefilledHost  : "", 
                   permissionType : "cookie",
                   windowTitle    : bundlePreferences.getString("cookiepermissionstitle"),
                   introText      : bundlePreferences.getString("cookiepermissionstext") };
    document.documentElement.openWindow("Browser:Permissions",
                                        "chrome://browser/content/preferences/permissions.xul",
                                        "", params);
  },
  
  changeMasterPassword: function ()
  {
    document.documentElement.openSubDialog("chrome://mozapps/content/preferences/changemp.xul",
                                           "", null);
    this.updateMasterPasswordButton();
  },
  
  updateMasterPasswordButton: function ()
  {
    // See if there's a master password and set the button label accordingly
    var secmodDB = Components.classes["@mozilla.org/security/pkcs11moduledb;1"]
                             .getService(Components.interfaces.nsIPKCS11ModuleDB);
    var slot = secmodDB.findSlotByName("");
    if (slot) {
      const nsIPKCS11Slot = Components.interfaces.nsIPKCS11Slot;
      var status = slot.status;
      var noMP = status == nsIPKCS11Slot.SLOT_UNINITIALIZED || 
                 status == nsIPKCS11Slot.SLOT_READY;

      var bundle = document.getElementById("bundlePreferences");
      var button = document.getElementById("setMasterPassword");
      button.label = bundle.getString(noMP ? "setMasterPassword" : "changeMasterPassword");
      
      var removeButton = document.getElementById("removeMasterPassword");
      removeButton.disabled = noMP;
    }
  },
  
  removeMasterPassword: function ()
  {
    var secmodDB = Components.classes["@mozilla.org/security/pkcs11moduledb;1"]
                              .getService(Components.interfaces.nsIPKCS11ModuleDB); 
    if (secmodDB.isFIPSEnabled) {
      var bundle = document.getElementById("bundlePreferences");
      promptService.alert(window,
                          bundle.getString("pw_change_failed_title"),
                          bundle.getString("pw_change2empty_in_fips_mode"));
    }
    else {
      document.documentElement.openSubDialog("chrome://mozapps/content/preferences/removemp.xul",
                                             "", null);
      this.updateMasterPasswordButton();
      document.getElementById("setMasterPassword").focus();
    }
  }
};
