/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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

var FirstRunUpdateCB = 
{
  onLoad: function(upd) { updateDataReady(upd); },
  onError: function(upd) { updateDataReady(upd); },
  QueryInterface : function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbIUpdateObserver) &&
        !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
  }
}

function updateDataReady(upd) {
  if (upd.getNumExtensions() > 0) {
    enableCustomInstall(); 
  } else {
    var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
    var songbirdStrings = sbs.createBundle("chrome://songbird/locale/songbird.properties");
    var errortitle = songbirdStrings.GetStringFromName("setup.networkerrortitle");
    var errormsg = songbirdStrings.GetStringFromName("setup.networkerrormsg");
    sbMessageBox(errortitle, errormsg, false);
  }
  enableGoAhead();
  hidePleaseWait();
}

var wanted_locale = "en-US";
 
function initFirstRun() 
{
  var upd = window.arguments[0].update;
  upd.addUpdateObserver(FirstRunUpdateCB);
  var s = upd.getStatus();
  if (s != 0) updateDataReady(upd);
  if (window.addEventListener) window.addEventListener("keydown", checkAltF4, true);
  fillLanguageBox();
}

function shutdownFirstRun()
{
  var upd = window.arguments[0].update;
  upd.removeUpdateObserver(FirstRunUpdateCB);
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
    className = menu.parentNode.getAttribute("class");

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
  pleasewait = document.getElementById("pleasewait");
  pleasewait.setAttribute("hidden", "true");
}

function doFirstRunTest(doc, upd)
{
  // Data remotes not available for this function
  var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
  var firstruncheck = "";
  try { firstruncheck = prefs.getCharPref("firstruncheck"); } catch (e) { }
  if (firstruncheck != "1")
  {
    var data = new Object();
    data.update = upd;
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
  var upd = window.arguments[0].update;
  var extlist = document.getElementById("songbird.extensionlist");
  if (extlist)
  {
    extlist.updateInterface = upd;
    extlist.toggleList();
  }
}

function doOK() 
{
  var upd = window.arguments[0].update;
  var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
  var noext = (upd.getNumExtensions() == 0);
  if (!noext) {
    var count=0;
    for (var i=0;i<upd.getNumExtensions();i++) if (upd.getExtensionInstallState(i)) count++;
    noext = (count == 0);
  }
  if (noext) {
    var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
    var songbirdStrings = sbs.createBundle("chrome://songbird/locale/songbird.properties");
    var noxpititle = songbirdStrings.GetStringFromName("setup.noxpititle");
    var noxpimsg = songbirdStrings.GetStringFromName("setup.noxpimsg");
    var r = sbMessageBox(noxpititle, noxpimsg, true);
    if (r == "accept") { 
      prefs.setCharPref("firstruncheck", "1");  
      return true; 
    } else {
      return false;
    }
  } else {
    upd.installSelectedExtensions(window);
    switchLocale(wanted_locale);
    prefs.setCharPref("firstruncheck", "1");  
    prefs.setCharPref("installedbundle", upd.getBundleVersion());
    if (upd.getNeedRestart()) {
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
  var upd = window.arguments[0].update;
  var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
  var songbirdStrings = sbs.createBundle("chrome://songbird/locale/songbird.properties");
  var bypasstitle = songbirdStrings.GetStringFromName("setup.bypasstitle");
  var bypassmsg = songbirdStrings.GetStringFromName("setup.bypassmsg");
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
