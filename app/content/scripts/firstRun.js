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

// Globals
var wanted_locale = "en-US";
var wanted_locale_bundleindex = -1;
var firstrun_bundle = null;
var locales_bundle = null;
var installed_locales = null;
var restartfirstrun = false;
var margin = 0;

const FIRSTRUN_BUNDLE_TIMEOUT = 15000;
const RESTART_ON_LOCALE_SELECTION = true;

const VK_ENTER = 13;
const VK_F4 = 0x73;

var firstRunBundleCB = 
{
  onDownloadComplete: function(bundle) { firstrunBundleDataReady(); },
  onError: function(bundle) { firstrunBundleDataReady(); },
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

var localesBundleCB = 
{
  onDownloadComplete: function(bundle) { localesBundleDataReady(); },
  onError: function(bundle) { localesBundleDataReady(); },
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

function firstrunBundleDataReady() {
  var extlist = document.getElementById("songbird.extensionlist");
  // if the callback is called before the list has finished opening or closing, delay it a bit
  if (extlist && extlist.isAnimating()) {
    setTimeout("firstrunBundleDataReady();", 10);
    return;
  }
  if (firstrun_bundle.bundleDataStatus !=
      Components.interfaces.sbIBundle.BUNDLE_DATA_STATUS_SUCCESS) {
    hidePleaseWait();
    showErrorMessage();
  } else if (firstrun_bundle.bundleExtensionCount < 1) {
    // there are no extensions
    hidePleaseWait();
    hideExtensionList();
  } else {
    setTimeout( "openExtensionsList();", 250 );
  }
}

function localesBundleDataReady() {
  if (locales_bundle.bundleExtensionCount > 0) {
    loadBundledLocales();
  }
}

function initFirstRun() {
  showMetricsInfo();
  // Determine if we should show the error box for people who have previously run Songbird
  try {
    var oldLibrary = false;
    var oldError = false;

    var haveRun = false;
    try {
      oldError = gPrefs.getBoolPref("songbird.firstrun.libraryerror.check.0.3");
    } catch (err) { /* prefs throws an exepction if the pref is not there */ }
    
    if (!oldError) {
      // Try to find the old 0.2 database file.    
      var directory = Components.classes["@mozilla.org/file/directory_service;1"].
                      getService(Components.interfaces.nsIProperties).
                      get("ProfD", Components.interfaces.nsIFile);
      directory.append("db");
      if (directory.exists()) {
        var old_database_file = directory.clone();
        old_database_file.append("songbird.db");
        if (old_database_file.exists()) {
          oldLibrary = true;
        }
      }
    }
    
    // If there's an old library file and we haven't shown the error, yet, show it.
    if ( oldLibrary) {
      showUpdateMessage();    
    }
  } catch ( err ) {
    SB_LOG("initFirstRun - library error box", "" + err );
  }


  try {
    var sbIBundle = new Components.Constructor("@songbirdnest.com/Songbird/Bundle;1", "sbIBundle");
    firstrun_bundle = new sbIBundle();
    firstrun_bundle.bundleId = "firstrun";
    // XXX Matt: Ah crap, this is totally going to leak
    // XXX lone: how so ?
    firstrun_bundle.addBundleDataListener(firstRunBundleCB);
    firstrun_bundle.retrieveBundleData(FIRSTRUN_BUNDLE_TIMEOUT);
  } catch ( err ) {
    SB_LOG("initFirstRun1", "" + err );
  }

  try {
    var sbIBundle = new Components.Constructor("@songbirdnest.com/Songbird/Bundle;1", "sbIBundle");
    locales_bundle = new sbIBundle();
    locales_bundle.bundleId = "locales";
    locales_bundle.addBundleDataListener(localesBundleCB);
    locales_bundle.retrieveBundleData(FIRSTRUN_BUNDLE_TIMEOUT);
  } catch ( err ) {
    SB_LOG("initFirstRun2", "" + err );
  }

  if (window.addEventListener) window.addEventListener("keydown", checkAltF4, true);
  if (window.addEventListener) window.addEventListener("command", checkCustomClose, true);
  fillLanguageBox();
}

function shutdownFirstRun()
{
  if (window.addEventListener) window.removeEventListener("keydown", checkAltF4, true);
  if (window.addEventListener) window.removeEventListener("command", checkCustomClose, true);
  firstrun_bundle.removeBundleDataListener(firstRunBundleCB);
  locales_bundle.removeBundleDataListener(localesBundleCB);
  continueStartup();
}

function fillLanguageBox()
{
  installed_locales = Array();
  var list = document.getElementById("language_list");
  var menu = list.childNodes[0];
  try {
    wanted_locale = gPrefs.getCharPref("general.useragent.locale");
  }
  catch (e) { 
    wanted_locale = "en-US";
  }
  wanted_locale_bundleindex = -1;
  if (!menu) {
    menu = document.createElement("menupopup");
    list.appendChild(menu);
  }

  var cr = Components.classes["@mozilla.org/chrome/chrome-registry;1"].getService(Components.interfaces.nsIToolkitChromeRegistry);

  var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
  var langNames = sbs.createBundle("chrome://global/locale/languageNames.properties");
  var regNames  = sbs.createBundle("chrome://global/locale/regionNames.properties");

  var locales = cr.getLocalesForPackage("songbird");
  var en_us_item = null;

  var i = 0;
  var select_item = null;
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
    
    installed_locales.push(locale);

    var item = document.createElement("menuitem");
    var className = menu.parentNode.getAttribute("class");

    item.setAttribute("label", displayName);
    item.setAttribute("name", "locale.switch");
    item.setAttribute("type", "radio");
    item.setAttribute("class", className);
    if (wanted_locale == locale) {
      select_item = item;
    }
    if (locale == "en-US") 
      en_us_item = item;
    item.setAttribute("oncommand", "setWantedLocale(\"" + locale + "\", -1)");

    elements.push(item);
    i++;
  }
  elements.sort(_sortLanguages);
  for (var i =0;i<elements.length;i++) menu.appendChild(elements[i]);
  if (select_item) {
    list.selectedItem = select_item;
  } else {
    wanted_locale = "en-US";
    wanted_locale_bundleindex = -1;
    list.selectedItem = en_us_item;
  }
}

function _sortLanguages(a, b) 
{
  var aname = a.getAttribute("label");
  var bname = b.getAttribute("label");
  if (aname == bname) return 0;
  if (aname < bname) return -1;
  return 1;
}

function setWantedLocale(locale, bundleindex)
{
  wanted_locale = locale;
  wanted_locale_bundleindex = bundleindex;
  if (RESTART_ON_LOCALE_SELECTION) {
    if (handleLocaleSelection()) {
      if (locales_bundle.restartRequired) {
        restartApp();
      } else {
        restartfirstrun = true;
        document.defaultView.close();
      }
    } else {
      reinitLanguageBox();
      loadBundledLocales();
    }
  }
}

function continueStartup() {
  //SB_LOG("continueStartup");
  window.arguments[0].onComplete(restartfirstrun);
}

function openExtensionsList()
{
  var extlist = document.getElementById("songbird.extensionlist");
  if (extlist)
  {
    extlist.bundleInterface = firstrun_bundle;

    var top = document.getElementById("extensions.replacedcontent.top");
    var bottom = document.getElementById("extensions.replacedcontent.bottom");
    var replacedheight = bottom.boxObject.y - top.boxObject.y;

    hidePleaseWait();
    hideErrorMessage();

    // Make the drawer open immediately instead of animating.
    // This is so that the drawer doesn't interfere with 
    // showing/hiding other elements in the window.
    extlist.openDrawer(true, replacedheight);
  }
}

function doOK() 
{
  //SB_LOG("doOK");
  handleOptOut(); // set the pref based upon the opt-out state.
  
  // do we need to install extensions ?
  var noext = (firstrun_bundle.bundleExtensionCount== 0);
  if (!noext) {
    var count = 0;
    for ( var i = 0; i < firstrun_bundle.bundleExtensionCount; i++ ) {
      if ( firstrun_bundle.getExtensionInstallFlag(i) )
        count++;
    }
    noext = (count == 0);
  }
  
  if (!handleLocaleSelection()) {
    reinitLanguageBox();
    loadBundledLocales();
    return false;
  }
  
  if (!noext) {
    var res = firstrun_bundle.installFlaggedExtensions(window);
    if (res == firstrun_bundle.BUNDLE_INSTALL_ERROR) 
      return false;
    gPrefs.setCharPref("songbird.installedbundle", firstrun_bundle.bundleDataVersion);
  }
  
  gPrefs.setBoolPref("songbird.firstrun.check.0.3", true);  
  gPrefs.setBoolPref("songbird.firstrun.libraryerror.check.0.3", true);
  
  
  if ((firstrun_bundle && firstrun_bundle.restartRequired) || 
      (locales_bundle && locales_bundle.restartRequired)) {
    restartApp();
  }

  return true;
}


function handleLocaleSelection() {
  // do we need to install a language pack ?
  if (wanted_locale_bundleindex != -1) {
    if (locales_bundle) {
      for (var i=0;i<locales_bundle.bundleExtensionCount;i++) {
        locales_bundle.setExtensionInstallFlag(i, i == wanted_locale_bundleindex);
      }
      var res = locales_bundle.installFlaggedExtensions(window);
      if (res == locales_bundle.BUNDLE_INSTALL_ERROR) 
        return false;
    }
  }
  switchLocale(wanted_locale);
  return true;
}

function handleKeyDown(event) 
{
  switch (event.keyCode)
  {
    case VK_ENTER:
      document.getElementById("ok_button").doCommand();
      break;
  }
}

function checkAltF4(evt)
{
  if (evt.keyCode == VK_F4 && evt.altKey) 
  {
    evt.preventDefault();
  }
}

function checkCustomClose(evt)
{
  if (evt.originalTarget.getAttribute("sbid") == "close") {
    quitApp();
  }
}

function handleOptOut()
{
  try {
    var metrics_enabled = document.getElementById("metrics_optout").checked ? 1 : 0;
    gPrefs.setCharPref("app.metrics.enabled", metrics_enabled);
  } catch (e) {}; // Stuff likes to throw.
};

function openConnectionSettings(evt)
{
  if (evt) {
    evt.preventDefault();
    evt.stopPropagation();
  }
  
  var psvc = Components.classes["@mozilla.org/preferences-service;1"]
                        .getService(Components.interfaces.nsIPrefBranch);
  // save value for instantApply
  var oldInstantApply = psvc.getBoolPref("browser.preferences.instantApply");
  // set it to true no matter what, it doesnt actually make it apply 
  // the settings instantly unless you're on a mac, but it makes clicking 
  // 'ok' apply the changes on all platforms (because the code for the 
  // prefwindow has no provision for child prefwindows running standalone
  // when they are not instantApply).
  psvc.setBoolPref("browser.preferences.instantApply", true);
  // open the connection settings
  window.openDialog( "chrome://browser/content/preferences/connection.xul", "Connections", "chrome,modal=yes,centerscreen", null);
  // restore old value
  psvc.setBoolPref("browser.preferences.instantApply", oldInstantApply);
  
  var psvc = Components.classes["@mozilla.org/preferences-service;1"]
                        .getService(Components.interfaces.nsIPrefService);
  psvc.savePrefFile(null);
  if (!firstrun_bundle) {
    var sbIBundle = new Components.Constructor("@songbirdnest.com/Songbird/Bundle;1", "sbIBundle");
    firstrun_bundle = new sbIBundle();
    firstrun_bundle.bundleId = "firstrun";
    firstrun_bundle.addBundleDataListener(firstRunBundleCB);
  }
  if (!locales_bundle) {
    var sbIBundle = new Components.Constructor("@songbirdnest.com/Songbird/Bundle;1", "sbIBundle");
    locales_bundle = new sbIBundle();
    locales_bundle.bundleId = "locales";
    locales_bundle.addBundleDataListener(localesBundleCB);
  }
  reinitLanguageBox();
  hideErrorMessage();
  showPleaseWait();
  firstrun_bundle.retrieveBundleData(FIRSTRUN_BUNDLE_TIMEOUT);
  locales_bundle.retrieveBundleData(FIRSTRUN_BUNDLE_TIMEOUT);
}

function reinitLanguageBox() {
  var list = document.getElementById("language_list");
  list.removeAllItems();
  fillLanguageBox();
}

function loadBundledLocales() {
  if (locales_bundle && locales_bundle.bundleExtensionCount > 0) {
    var list = document.getElementById("language_list");
    var menu = list.childNodes[0];
    var className = menu.parentNode.getAttribute("class");
    var separator = null;

    var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
    var langNames = sbs.createBundle("chrome://global/locale/languageNames.properties");
    var regNames  = sbs.createBundle("chrome://global/locale/regionNames.properties");

    var elements = Array();
    
    for (var i=0;i<locales_bundle.bundleExtensionCount;i++) {
      locales_bundle.setExtensionInstallFlag(i, false);

      var locale = locales_bundle.getExtensionAttribute(i, "name");
      locale = locale.split(" ")[0]; // FIX ! name should not have a trailing " (Country)"
      
      if (isLanguageInstalled(locale)) continue;

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
        displayName = locales_bundle.getExtensionAttribute(i, "desc");
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
      var itemclass = className;
      var flag = menubar_locales_bundle.getExtensionAttribute(i, "flag");
      if (flag && flag != "") {
        itemclass = itemclass + " menuitem-iconic";
        item.setAttribute("image", flag);
      }
      item.setAttribute("class", itemclass); 
      item.setAttribute("oncommand", "setWantedLocale(\"" + locale + "\", " + i + ")");

      elements.push(item);
    }
    elements.sort(sortLanguages);
    for (var i =0;i<elements.length;i++) menu.appendChild(elements[i]);
  }
}

function isLanguageInstalled(locale) {
  for (var i=0;i<installed_locales.length;i++) {
    if (installed_locales[i] == locale) return true;
  }
  return false;
}

function showErrorMessage() 
{
  document.getElementById("error_message").removeAttribute("hidden");
  window.sizeToContent();
}

function hideErrorMessage()
{
  document.getElementById("error_message").setAttribute("hidden", "true");
  window.sizeToContent();
}

function showPleaseWait()
{
  document.getElementById("please_wait").removeAttribute("hidden");
  window.sizeToContent();
}

function hidePleaseWait()
{
  document.getElementById("please_wait").setAttribute("hidden", "true");
  window.sizeToContent();
}

function showUpdateMessage() {
  document.getElementById("error_noupdate_vbox").removeAttribute("hidden");
  window.sizeToContent();
}

function hideExtensionList() {
  document.getElementById("extensions_vbox").setAttribute("hidden", "true");
  window.sizeToContent();
}

function showMetricsInfo() {
  document.getElementById("metrics_info").removeAttribute("hidden");
  window.sizeToContent();
}

