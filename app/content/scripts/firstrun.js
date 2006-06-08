/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed 
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either 
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

var FirstRunBundleCB = 
{
  onLoad: function(bundle) { bundleDataReady(bundle); },
  onError: function(bundle) { bundleDataReady(bundle); },
  QueryInterface : function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbIBundleObserver) &&
        !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
  }
}

function bundleDataReady(bundle) {
  if (bundle.getNumExtensions() > 0) {
    enableCustomInstall(); 
  } else {
    var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
    var songbirdStrings = sbs.createBundle("chrome://songbird/locale/songbird.properties");
    var errortitle = "Network Error"; 
    var errormsg = "Songbird could not retrieve the list of extensions to install from the internet. Please visit http://songbirdnest.com to extend your media player today!";
    try  { // GetStringFromName likes to throw.  Mozilla is _nasty_ about localization.
      errortitle = songbirdStrings.GetStringFromName("setup.networkerrortitle");
      errormsg = songbirdStrings.GetStringFromName("setup.networkerrormsg");
    }catch(e){}
    sbMessageBox(errortitle, errormsg, false);
  }
  enableGoAhead();
  hidePleaseWait();
}

var wanted_locale = "en-US";
 
function initFirstRun() 
{
  var bundle = window.arguments[0].bundle;
  bundle.addBundleObserver(FirstRunBundleCB);
  var s = bundle.getStatus();
  if (s != 0) bundleDataReady(bundle);
  if (window.addEventListener) window.addEventListener("keydown", checkAltF4, true);
  fillLanguageBox();
}

function shutdownFirstRun()
{
  var bundle = window.arguments[0].bundle;
  bundle.removeBundleObserver(FirstRunBundleCB);
  continueStartup();
}

function fillLanguageBox()
{
  var list = document.getElementById("language_list");
  var menu = list.childNodes[0];
  var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
  try {
    wanted_locale = prefs.getCharPref("general.useragent.locale");
  }
  catch (e) { }

  var cr = Components.classes["@mozilla.org/chrome/chrome-registry;1"].getService(Components.interfaces.nsIToolkitChromeRegistry);

  var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
  var langNames = sbs.createBundle("chrome://global/locale/languageNames.properties");
  var regNames  = sbs.createBundle("chrome://global/locale/regionNames.properties");

  var locales = cr.getLocalesForPackage("songbird");

  var i = 0;
  var select_item = null;
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

    var item = document.createElement("menuitem");
    var className = menu.parentNode.getAttribute("class");

    item.setAttribute("label", displayName);
    item.setAttribute("name", "locale.switch");
    item.setAttribute("type", "radio");
    item.setAttribute("class", className);
    if (wanted_locale == locale) {
      select_item = item;
    }
    item.setAttribute("oncommand", "setWantedLocale(\"" + locale + "\")");

    menu.appendChild(item);
    i++;
  }
  list.selectedItem = select_item;
}

function setWantedLocale(locale)
{
  wanted_locale = locale;
}

function enableCustomInstall() 
{
  document.getElementById("custom_button").setAttribute("disabled", "false");
}

function enableGoAhead() 
{
  document.getElementById("ok_button").setAttribute("disabled", "false");
}

function hidePleaseWait()
{
  var pleasewait = document.getElementById("pleasewait");
  pleasewait.setAttribute("hidden", "true");
}

function doFirstRunTest(doc, bundle)
{
  // Data remotes not available for this function
  var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
  var firstruncheck = "";
  try { firstruncheck = prefs.getCharPref("firstruncheck"); } catch (e) { }
  if (firstruncheck != "1")
  {
    var data = new Object();
    data.bundle = bundle;
    data.document = document;
    window.openDialog( "chrome://songbird/content/xul/firstrun.xul", "firstrun", "chrome,centerscreen,modal=no", data);
    return 1;
  }
  return 0;
}

function continueStartup() {
  window.arguments[0].document.__STARTMAINWIN();
}

function customInstall()
{
  var bundle = window.arguments[0].bundle;
  var extlist = document.getElementById("songbird.extensionlist");
  if (extlist)
  {
    extlist.bundleInterface = bundle;
    extlist.toggleList();
  }
}

function doOK() 
{
  var bundle = window.arguments[0].bundle;
  var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
  var noext = (bundle.getNumExtensions() == 0);
  if (!noext) {
    var count=0;
    for (var i=0;i<bundle.getNumExtensions();i++) if (bundle.getExtensionInstallState(i)) count++;
    noext = (count == 0);
  }
  if (noext) {
    var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
    var songbirdStrings = sbs.createBundle("chrome://songbird/locale/songbird.properties");
    var noxpititle = "No extension"; 
    var noxpimsg = "Press Ok to keep a minimal installation, or Cancel to go back.";
    try { // GetStringFromName likes to throw.  Mozilla is _nasty_ about localization.
      noxpititle = songbirdStrings.GetStringFromName("setup.noxpititle");
      noxpimsg = songbirdStrings.GetStringFromName("setup.noxpimsg");
    }catch(e){}
    var r = sbMessageBox(noxpititle, noxpimsg, true);
    if (r == "accept") { 
      prefs.setCharPref("firstruncheck", "1");  
      return true; 
    } else {
      return false;
    }
  } else {
    bundle.installSelectedExtensions(window);
    switchLocale(wanted_locale);
    prefs.setCharPref("firstruncheck", "1");  
    prefs.setCharPref("installedbundle", bundle.getBundleVersion());
    if (bundle.getNeedRestart()) {
      var as = Components.classes["@mozilla.org/toolkit/app-startup;1"].getService(Components.interfaces.nsIAppStartup);
      if (as)
      {
        const V_RESTART = 16;
        const V_ATTEMPT = 2;
        as.quit(V_RESTART);
        as.quit(V_ATTEMPT);
      }
    }
    return true;
  }
  return false;
}

function doCancel()
{
  var bundle = window.arguments[0].bundle;
  var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
  var songbirdStrings = sbs.createBundle("chrome://songbird/locale/songbird.properties");
  var bypasstitle = "Proceed ?"; 
  var bypassmsg = "Are you sure you want to bypass the final setup? (you will be offered another opportunity to revisit this screen the next time you run Songbird).";
  try  { // GetStringFromName likes to throw.  Mozilla is _nasty_ about localization.
    bypasstitle = songbirdStrings.GetStringFromName("setup.bypasstitle");
    bypassmsg = songbirdStrings.GetStringFromName("setup.bypassmsg");
  }catch(e){}
  var r = sbMessageBox(bypasstitle, bypassmsg, true); 
  if (r == "accept") { 
    window.arguments[0].cancelled = true; 
    return true; 
  }
  return false;
}

function handleKeyDown(event) 
{
  const VK_ESCAPE = 27;
  const VK_ENTER = 13;
  switch (event.keyCode)
  {
    case VK_ESCAPE:
      document.getElementById("cancel_button").doCommand();
      break;
    case VK_ENTER:
      document.getElementById("ok_button").doCommand();
      break;
  }
}

function checkAltF4(evt)
{
  const VK_F4 = 0x73;
  if (evt.keyCode == VK_F4 && evt.altKey) 
  {
    evt.preventDefault();
    if (doCancel()) onExit();
  }
}
