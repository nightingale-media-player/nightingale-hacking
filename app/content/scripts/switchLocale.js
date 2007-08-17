/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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
 * \file switchLocale.js
 * \brief Locales switching function implementations.
 * \note Based on Firefox Locale Switcher, by Benjamin Smedberg (http://benjamin.smedbergs.us/blog/2005-11-29/locale-switcher-15/)
 * \internal
 */

const MENUBAR_LOCALESBUNDLE_TIMEOUT = 15000;

try {
  // Module specific global for auto-init/deinit support
  var menubar_locales_bundle_module = {};
  var menubar_locales_bundle = null;

  menubar_locales_bundle_module.init_once = 0;
  menubar_locales_bundle_module.deinit_once = 0;
  menubar_locales_bundle_module.onLoad = function()
  {
    if (menubar_locales_bundle_module.init_once++) { dump("WARNING: menubar_locales_bundle double init!!\n"); return; }
    initLocalesBundle();
  }
  menubar_locales_bundle_module.onUnload = function()
  {
    if (menubar_locales_bundle_module.deinit_once++) { dump("WARNING: menubar_locales_bundle double deinit!!\n"); return; }
    window.removeEventListener("load", menubar_locales_bundle_module.onLoad, false);
    window.removeEventListener("unload", menubar_locales_bundle_module.onUnload, false);
    resetLocalesBundle();
  }

  // Auto-init/deinit registration
  window.addEventListener("load", menubar_locales_bundle_module.onLoad, false);
  window.addEventListener("unload", menubar_locales_bundle_module.onUnload, false);

 
  var menubar_locales_installed = null;

  /**
   * \brief Initialize the Locales Bundle.
   * \internal
   */
  function initLocalesBundle() {
    try {
      var sbIBundle = new Components.Constructor("@songbirdnest.com/Songbird/Bundle;1", "sbIBundle");
      menubar_locales_bundle = new sbIBundle();
      menubar_locales_bundle.bundleId = "locales";
      menubar_locales_bundle.addBundleDataListener(menubarLocalesBundleCB);
      menubar_locales_bundle.retrieveBundleData(MENUBAR_LOCALESBUNDLE_TIMEOUT);
    } catch ( err ) {
      SB_LOG("initLocalesBundle", "" + err );
    }
  }

  /**
   * \brief Reset the Locales Bundle to an uninitialized state.
   * \internal
   */
  function resetLocalesBundle() {
    if (menubar_locales_bundle) menubar_locales_bundle.removeBundleDataListener(menubarLocalesBundleCB);
    menubar_locales_bundle = null;
  }

  /**
   * \brief Locales Bundle listener.
   * Handles download completion and errors.
   * \internal
   */
  var menubarLocalesBundleCB = 
  {
    onDownloadComplete: function(bundle) { menubarLocalesBundleDataReady(); },
    onError: function(bundle) { menubarLocalesBundleDataReady(); },
    QueryInterface : function(aIID)
    {
      if (!aIID.equals(Components.interfaces.sbIBundleDataListener) &&
          !aIID.equals(Components.interfaces.nsISupports)) 
      {
        throw Components.results.NS_ERROR_NO_INTERFACE;
      }
      return this;
    }
  }

  /**
   * \brief Listener for Locales bundle data availability.
   * \internal
   */
  function menubarLocalesBundleDataReady() {
    if (menubar_locales_bundle.bundleExtensionCount > 0) {
      if (_lastmenu) fillLocaleList(_lastmenu);
    }
  }

  /**
   * \brief Build the Locales menu.
   * \internal
   */
  function menubarLoadBundledLocales(menu) {
    if (menubar_locales_bundle && menubar_locales_bundle.bundleExtensionCount > 0) {
      var className = menu.parentNode.getAttribute("class");

      var separator = null;

      var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
      var langNames = sbs.createBundle("chrome://global/locale/languageNames.properties");
      var regNames  = sbs.createBundle("chrome://global/locale/regionNames.properties");

      var elements = Array();
      
      for (var i=0; i < menubar_locales_bundle.bundleExtensionCount; i++) {
        menubar_locales_bundle.setExtensionInstallFlag(i, false);

        var locale = menubar_locales_bundle.getExtensionAttribute(i, "name");
        locale = locale.split(" ")[0]; // FIX ! name should not have a trailing " (Country)"
        
        if (menubarIsLanguageInstalled(locale)) continue;

        var parts = locale.split(/-/);

        var displayName;
        try {
          displayName = langNames.GetStringFromName(parts[0]);
          if (parts.length > 1) {
            try {
              displayName += " (" + regNames.GetStringFromName(parts[1].toLowerCase()) + ")";
            }
            catch (e) {
              displayName += " (" + parts[1] + ")";
            }
          }
        }
        catch (e) {
          displayName = menubar_locales_bundle.getExtensionAttribute(i, "desc");
        }

        if (!separator) {
          separator = document.createElement("menuseparator");
          separator.setAttribute("class", className);
          menu.appendChild(separator);
        }

        var item = document.createElement("menuitem");
        item.setAttribute("label", displayName);
        item.setAttribute("name", "locale.switch");
        item.setAttribute("type", "radio");
        item.setAttribute("class", className); 
        item.setAttribute("oncommand", "installLocaleFromBundle(\"" + locale + "\", " + i + ")");

        elements.push(item);
      }
      elements.sort(sortLanguages);
      for (var i =0;i<elements.length;i++) menu.appendChild(elements[i]);
    }
  }


  /**
   * \brief Switch to a different Locale.
   * \param locale The local to switch to (ie. "en-US").
   * \param wantmessagebox Flag enabling warning messaging about locale switch.
   * \internal
   */
  function switchLocale(locale, wantmessagebox) {
    try 
    {
      var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
      var curLocale = "en-US";
      try {
        curLocale = prefs.getCharPref("general.useragent.locale");
      }
      catch (e) { }

      if (locale != curLocale) {
        prefs.setCharPref("general.useragent.locale", locale);
        if (wantmessagebox) {
          sbRestartBox_strings("message.localization", "message.needrestart", "Localization", "This setting will take effect after you restart Songbird");
        }
      }
    }
    catch ( err )
    {
      SB_LOG( "switchLocale - " + err);
    }
  }

  var _lastmenu = null;

  /**
   * \brief Fill the Locales menu with all available Locales.
   * \internal
   */
  function fillLocaleList(menu) {
    try 
    {
      menubar_locales_installed = Array();
      _lastmenu = menu;
      var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
      var curLocale = "en-US";
      try {
        curLocale = prefs.getCharPref("general.useragent.locale");
      }
      catch (e) { }

      var cr = Components.classes["@mozilla.org/chrome/chrome-registry;1"].getService(Components.interfaces.nsIToolkitChromeRegistry);

      var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
      var langNames = sbs.createBundle("chrome://global/locale/languageNames.properties");
      var regNames  = sbs.createBundle("chrome://global/locale/regionNames.properties");

      /* clear the existing children */
      var children = menu.childNodes;
      for (var i = children.length; i > 0; --i) {
        menu.removeChild(children[i - 1]);
      }

      var locales = cr.getLocalesForPackage("songbird");
      var elements = Array();
      
      while (locales.hasMore()) {
        var locale = locales.getNext();

        var parts = locale.split(/-/);

        var displayName;
        try {
          displayName = langNames.GetStringFromName(parts[0]);
          if (parts.length > 1) {
            try {
              displayName += " (" + regNames.GetStringFromName(parts[1].toLowerCase()) + ")";
            }
            catch (e) {
              displayName += " (" + parts[1] + ")";
            }
          }
        }
        catch (e) {
          displayName = locale;
        }

        menubar_locales_installed.push(locale);

        var item = document.createElement("menuitem");
        var className = menu.parentNode.getAttribute("class");

        item.setAttribute("label", displayName);
        item.setAttribute("name", "locale.switch");
        item.setAttribute("type", "radio");

        var itemclass = className;
        var flag = getFlagFromBundle(locale);
        if (flag && flag != "") {
          itemclass = itemclass + " menuitem-iconic";
          item.setAttribute("image", flag);
        }
        item.setAttribute("class", itemclass);
 
        if (curLocale == locale) {
          item.setAttribute("checked", "true");
        }
        item.setAttribute("oncommand", "switchLocale(\"" + locale + "\", true)");
        
        elements.push(item);
      }
      
      elements.sort(sortLanguages);
      for (var i =0;i<elements.length;i++) menu.appendChild(elements[i]);
      if (menubar_locales_bundle && menubar_locales_bundle.bundleDataStatus == Components.interfaces.sbIBundle.BUNDLE_DATA_STATUS_SUCCESS) menubarLoadBundledLocales(menu);
    }
    catch ( err )
    {
      SB_LOG( "fillLocaleList - " + err);
    }
  }
  
  /**
   * \brief Get the flag icon URL for a Locales from the Locales bundle.
   * \param locale The locale.
   * \return The flag icon URL or null on error.
   * \internal
   */
  function getFlagFromBundle(locale) {
    for (var i=0; i < menubar_locales_bundle.bundleExtensionCount; i++) {
      var name = menubar_locales_bundle.getExtensionAttribute(i, "name");
      name = name.split(" ")[0]; // FIX ! name should not have a trailing " (Country)"
      if (name == locale) return menubar_locales_bundle.getExtensionAttribute(i, "flag");
    }
    return null;
  } 

  /**
   * \brief Determine if a Locale is marked installed in the Locales menu.
   * \internal
   */
  function menubarIsLanguageInstalled(locale) {
    for (var i=0;i<menubar_locales_installed.length;i++) {
      if (menubar_locales_installed[i] == locale) return true;
    }
    return false;
  }

  /**
   * \brief Sort Locale language names.
   * \internal
   */
  function sortLanguages(a, b) 
  {
    var aname = a.getAttribute("label");
    var bname = b.getAttribute("label");
    if (aname == bname) return 0;
    if (aname < bname) return -1;
    return 1;
  }
  
  /**
   * \brief Install a Locale from the Locales Bundle.
   * \param locale The locale to install (ie. "en-US").
   * \param bundleindex The index of the locale within the bundle. Index starts at 0.
   * \internal
   */
  function installLocaleFromBundle(locale, bundleindex) {
    if (gPrompt.confirm( window, 
                        SBString( "locales.installconfirm.title", "Language Download" ),
                        SBString( "locales.installconfirm.msg", "This language is not installed, would you like to download and install it?" ) ) ) {
      if (bundleindex != -1 && menubar_locales_bundle && bundleindex < menubar_locales_bundle.bundleExtensionCount) {
        for (var i=0;i<menubar_locales_bundle.bundleExtensionCount;i++) {
          menubar_locales_bundle.setExtensionInstallFlag(i, i == bundleindex);
        }
        var res = menubar_locales_bundle.installFlaggedExtensions(window);
        
        if (res == menubar_locales_bundle.BUNDLE_INSTALL_SUCCESS) {
          switchLocale(locale, true);
        } else {
          gPrompt.alert( window, 
                        SBString( "locales.installfailed.title", "Language Download" ),
                        SBString( "locales.installfailed.msg", "Language installation failed, check your network connectivity!" ) );
        }
      }
    }
  }

}
catch (err)
{
  dump("switch_locales.js - " + err);
}
