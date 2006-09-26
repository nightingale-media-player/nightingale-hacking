/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
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
var loaded_bundle = false;
var bundle = null;
var FirstRunBundleCB = 
{
  onLoad: function(bundle) { bundleDataReady(); },
  onError: function(bundle) { bundleDataReady(); },
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

function bundleDataReady() {
  var extlist = document.getElementById("songbird.extensionlist");
  // if the callback is called before the list has finished opening or closing, delay it a bit
  if (extlist && extlist.isAnimating()) {
    setTimeout("bundleDataReady();", 10);
    return;
  }
  if (bundle.getNumExtensions() > 0) {
    loaded_bundle = true;
    setTimeout( "openExtensionsList();", 250 );
  } else {
    hidePleaseWait();
    showErrorMessage();
  }
}

function initFirstRun() 
{
  try {
    fixOSXWindow("window_top", "app_title");
  }
  catch (e) { }
  
  try {
    var nsIBundle = new Components.Constructor("@songbirdnest.com/Songbird/Bundle;1", "sbIBundle");
    bundle = new nsIBundle();
    bundle.retrieveBundleFile();
  } catch ( err ) {
    SB_LOG("initFirstRun", "" + err );
  }

  // XXX Matt: Ah crap, this is totally going to leak
  // XXX lone: how so ?
  bundle.addBundleObserver(FirstRunBundleCB);
  var s = bundle.getStatus();
  if (s != 0) bundleDataReady(bundle);
  if (window.addEventListener) window.addEventListener("keydown", checkAltF4, true);
  fillLanguageBox();
}

function shutdownFirstRun()
{
  bundle.removeBundleObserver(FirstRunBundleCB);
  continueStartup();
}

function fillLanguageBox()
{
  var list = document.getElementById("language_list");
  var menu = list.childNodes[0];
  try {
    wanted_locale = gPrefs.getCharPref("general.useragent.locale");
  }
  catch (e) { }

  var cr = Components.classes["@mozilla.org/chrome/chrome-registry;1"].getService(Components.interfaces.nsIToolkitChromeRegistry);

  var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
  var langNames = sbs.createBundle("chrome://global/locale/languageNames.properties");
  var regNames  = sbs.createBundle("chrome://global/locale/regionNames.properties");

  var locales = cr.getLocalesForPackage("songbird");

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

    elements.push(item);
    i++;
  }
  elements.sort(sortLanguages);
  for (var i =0;i<elements.length;i++) menu.appendChild(elements[i]);
  list.selectedItem = select_item;
}

function sortLanguages(a, b) 
{
  var aname = a.getAttribute("label");
  var bname = b.getAttribute("label");
  if (aname == bname) return 0;
  if (aname < bname) return -1;
  return 1;
}

function setWantedLocale(locale)
{
  wanted_locale = locale;
}

function continueStartup() {
  //SB_LOG("continueStartup");
  window.arguments[0].onComplete();
}

function openExtensionsList()
{
  var extlist = document.getElementById("songbird.extensionlist");
  if (extlist)
  {
    extlist.bundleInterface = bundle;

    var top = document.getElementById("extensions.replacedcontent.top");
    var bottom = document.getElementById("extensions.replacedcontent.bottom");
    var replacedheight = bottom.boxObject.y - top.boxObject.y;

    hidePleaseWait();
    hideErrorMessage();

    extlist.openDrawer(false, replacedheight);
  }
}

function doOK() 
{
  //SB_LOG("doOK");
  handleOptOut(); // set the pref based upon the opt-out state.
  var noext = (bundle.getNumExtensions() == 0);
  if (!noext) {
    var count = 0;
    for ( var i = 0; i < bundle.getNumExtensions(); i++ ) {
      if ( bundle.getExtensionInstallState(i) )
        count++;
    }
    noext = (count == 0);
  }
  switchLocale(wanted_locale);
  if (noext) {
    gPrefs.setBoolPref("songbird.firstruncheck", true);  
    return true; 
  } else {
    var res = bundle.installSelectedExtensions(window);
    if (res == "failure") return false;
    gPrefs.setBoolPref("songbird.firstruncheck", true);  
    
    gPrefs.setCharPref("songbird.installedbundle", bundle.getBundleVersion());
    if (bundle.getNeedRestart()) {
      var nsIMetrics = new Components.Constructor("@songbirdnest.com/Songbird/Metrics;1", "sbIMetrics");
      var MetricsService = new nsIMetrics();
      MetricsService.setSessionFlag(false); // mark this session as clean, we did not crash
      var as = Components.classes["@mozilla.org/toolkit/app-startup;1"]
               .getService(Components.interfaces.nsIAppStartup);
      if (as)
      {
        //Both calls are needed as the restart path only sets an internal
        //   variable that gets cached and references during the second call.
        as.quit(Components.interfaces.nsIAppStartup.eRestart);
        as.quit(Components.interfaces.nsIAppStartup.eAttemptQuit);
      }
    }
    return true;
  }
  return false;
}

function handleKeyDown(event) 
{
  const VK_ENTER = 13;
  switch (event.keyCode)
  {
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
  window.openDialog( "chrome://browser/content/preferences/connection.xul", "Connections", "chrome,modal=yes,centerscreen", document );
  if (!bundle) {
    bundle = new nsIBundle();
    bundle.addBundleObserver(FirstRunBundleCB);
  }
  hideErrorMessage();
  showPleaseWait();
  loaded_bundle = false;
  bundle.retrieveBundleFile();
  var s = bundle.getStatus();
  if (s != 0) bundleDataReady(bundle);
}

function showErrorMessage() 
{
  document.getElementById("error_message").setAttribute("hidden", "false");
}

function hideErrorMessage()
{
  document.getElementById("error_message").setAttribute("hidden", "true");
}

function showPleaseWait()
{
  document.getElementById("please_wait").setAttribute("hidden", "false");
}

function hidePleaseWait()
{
  document.getElementById("please_wait").setAttribute("hidden", "true");
}

function onMetricsDrawer(evt) 
{
  if (evt) {
    evt.preventDefault();
    evt.stopPropagation();
  }
  var drawer = document.getElementById("metrics_drawer");
  if (drawer.state != "open") 
  {
    drawer.openDrawer();
    document.getElementById("metrics_href").setAttribute("style", "");
  }
  else
  {
    // if the drawer is already open, clicking on 'usage metrics' should toggle the metrics, just like the rest of the sentence
    toggleMetrics(evt);
  }
}

function toggleMetrics(evt) 
{
  var id = evt.target.getAttribute("id");
  if (id == "metrics_label" || id == "metrics_href") {
    var checkbox = document.getElementById("metrics_optout");
    checkbox.focus();
    var checked = checkbox.getAttribute("checked") == "true";
    checkbox.setAttribute("checked", checked ? "false" : "true");
  }
}
