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
 * \file windowUtils.js
 * \brief Window Utility constants, functions and objects that are common
 * to all windows.
 *
 * Note: This file is dependent on chrome://global/content/globalOverlay.js
 */

/**
 * \brief The Songbird Core Window Type.
 */
var CORE_WINDOWTYPE         = "Songbird:Core";

/**
 * \brief Maximized State value.
 */
var STATE_MAXIMIZED         = Components.interfaces.nsIDOMChromeWindow.STATE_MAXIMIZED;

/**
 * \brief Minimized State value.
 */
var STATE_MINIMIZED         = Components.interfaces.nsIDOMChromeWindow.STATE_MINIMIZED;

// Lots of things assume the playlist playback service is a global
var gPPS    = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                      .getService(Components.interfaces.sbIPlaylistPlayback);
// Overused services get put in headers?
var gPrompt = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                      .getService(Components.interfaces.nsIPromptService);

// Strings are cool.
var theSongbirdStrings = document.getElementById( "songbird_strings" );

// XXXredfive - this goes in the sbWindowUtils.js file when I get around to making it.
var gPrefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
var gConsole = Components.classes["@mozilla.org/consoleservice;1"]
               .getService(Components.interfaces.nsIConsoleService);

// log to JS console AND to the command line (in case of crashes)
function SB_LOG (scopeStr, msg) {
  msg = msg ? msg : "";
  //This works, but adds everything as an Error
  //Components.utils.reportError( scopeStr + " : " + msg);
  gConsole.logStringMessage( scopeStr + " : " + msg );
  dump( scopeStr + " : " + msg + "\n");
}

var PREFS_SERVICE_CONTRACTID = "@mozilla.org/preferences-service;1";
var nsIPrefBranch2 = Components.interfaces.nsIPrefBranch2;
/**
 * \brief Get a preference.
 * Adapted from nsUpdateService.js.in. Need to replace with dataremotes.
 * \param aFunc Function to used to retrieve the pref.
 * \param aPreference The name of the pref to retrieve.
 * \param aDefaultValue The default value to return if it is impossible to read the pref or it does not exist.
 * \internal
 */
function getPref(aFunc, aPreference, aDefaultValue) {
  var prefs =
    Components.classes[PREFS_SERVICE_CONTRACTID].getService(nsIPrefBranch2);
  try {
    return prefs[aFunc](aPreference);
  }
  catch (e) { }
  return aDefaultValue;
}

/**
 * \brief Set a preference.
 * \param aFunc Function to used to set the pref.
 * \param aPreference The name of the pref to set.
 * \param aValue The value of the pref.
 * \return The return value of the function used to set the pref.
 * \internal
 */
function setPref(aFunc, aPreference, aValue) {
  var prefs =
    Components.classes[PREFS_SERVICE_CONTRACTID].getService(nsIPrefBranch2);
  return prefs[aFunc](aPreference, aValue);
}

/**
 * \brief onMinimize handler, minimizes the window in the current context.
 * \internal
 */
function onMinimize()
{
  document.defaultView.minimize();
}

/**
 * \brief onMaximize handler, maximizes the window in the current context.
 * \internal
 */
function onMaximize()
{
  if ( isMaximized() )
  {
    document.defaultView.restore();
  }
  else
  {
    document.defaultView.maximize();
  }
  // TODO
  //syncMaxButton();
  //syncResizers();
}

/**
 * \brief Is the window in the current context maximized?
 * \return true or false.
 * \internal
 */
function isMaximized() {
  return (window.windowState == STATE_MAXIMIZED);
}

/**
 * \brief Is the window in the current context minimized?
 * \return true or false.
 * \internal
 */
function isMinimized() {
  return (window.windowState == STATE_MINIMIZED);
}

/* TODO: These broke circa 0.2.  The logic needs to be moved into sys-outer-frame
function syncMaxButton()
{
  var maxButton = document.getElementById("sysbtn_maximize");
  if (maxButton)
  {
    if (isMaximized()) maxButton.setAttribute("checked", "true");
    else maxButton.removeAttribute("checked");
  }
}
function syncResizers()
{
  // TODO?
  if (isMaximized()) disableResizers();
  else enableResizers();
}
*/

/**
 * \brief Restore the window in the current context to unmaximized state.
 * \internal
 */
function restoreWindow()
{
  if ( isMaximized() )
  {
    document.defaultView.restore();
  }
  
  // TODO
  //syncMaxButton();
  //syncResizers();
}

/**
 * \brief onExit handler, saves window size and position before closing the window.
 * \internal
 */
function onExit( skipSave )
{
  document.defaultView.close();
}

/**
 * \brief onHide handler, handles hiding the window in the current context.
 * \internal
 */
function onHide()
{
  var windowCloak =
    Components.classes["@songbirdnest.com/Songbird/WindowCloak;1"]
              .getService(Components.interfaces.sbIWindowCloak);
  windowCloak.cloak(window);

  // And try to focus another of our windows. We'll try to focus in this order:
  var windowList = ["Songbird:Main",
                    "Songbird:TrackEditor",
                    "Songbird:Firstrun",
                    "Songbird:EULA"];
  var windowCount = windowList.length;

  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);

  for (var index = 0; index < windowCount; index++) {
    var windowType = windowList[index];
    var lastWindow = wm.getMostRecentWindow(windowType);
    if (lastWindow && (lastWindow != window)) {
      try {
        lastWindow.focus();
      } catch (e) {}
      break;
    }
  }

}


/**
 * \brief Handles completion of resizing of the window in the current context.
 */
function onWindowResizeComplete() {
  var d = window.document;
  var dE = window.document.documentElement;
  if (dE.getAttribute("persist").match("width"))  { d.persist(dE.id, "width");  }
  if (dE.getAttribute("persist").match("height")) { d.persist(dE.id, "height"); }
}

/**
 * \brief Handles completion of dragging of the window in the current context.
 */
function onWindowDragComplete() {
  var d = window.document;
  var dE = window.document.documentElement;
  if (dE.getAttribute("persist").match("screenX")) { d.persist(dE.id, "screenX");  }
  if (dE.getAttribute("persist").match("screenY")) { d.persist(dE.id, "screenY"); }
}

/**
 * \brief Focus the window in the current context.
 */
function windowFocus()
{
  // Try to activate the window if it isn't cloaked.
  var windowCloak =
    Components.classes["@songbirdnest.com/Songbird/WindowCloak;1"]
              .getService(Components.interfaces.sbIWindowCloak);
  if (windowCloak.isCloaked(window))
    return;

  try {
    window.focus();
  } catch(e) {
  }
}

/**
 * \brief Delayed focus of the window in the current context.
 */
function delayedActivate()
{
  setTimeout( windowFocus, 50 );
}

/**
 * \brief See if a window needs to be somehow "fixed" after it is opened.
 *
 * In addition to loading the size and position (if stored), we also perform some sanity checks. 
 * The window must be:
 * - sized appropriately if we have never seen it before
 * - not too small for its min-size styles or some default minimum.
 * - actually on screen somewhere.
 * 
 */
function windowPlacementSanityChecks()
{
  delayedActivate();
  
  dump("New window sanity checks. \n");
  var x = document.documentElement.getAttribute("screenX");
  var y = document.documentElement.getAttribute("screenY");
  var width  = document.documentElement.getAttribute("width");
  var height = document.documentElement.getAttribute("height");
  dump("Before: x/y:\t" + x + "\t" + y + "\tw/h: " + width + "\t" + height + "\n"); 

  // Set a sane starting width/height for all resolutions on new profiles.
  // lifted nearly verbatim from seamonkey/source/browser/base/content/browser.js
  if (!document.documentElement.hasAttribute("width")) {
    var defaultWidth = 994, defaultHeight;
    if (screen.availHeight <= 600 || screen.availWidth <= 600) {
      document.documentElement.setAttribute("sizemode", "maximized");
      defaultWidth = 590;
      defaultHeight = 450;
    }
    else {
      defaultWidth = 988;
      defaultHeight = 720;
    }
      
    document.documentElement.setAttribute("width", defaultWidth);
    document.documentElement.setAttribute("height", defaultHeight);
  }

  // is the window too small? (the 16 is an arbitrary "too small" size to prevent ungrabbably small windows)
  var minWidth  = getStyle(document.documentElement, "min-width");
  var minHeight = getStyle(document.documentElement, "min-height");
  if (minWidth)  { minWidth  = parseInt(minWidth);  } else { minWidth  = 16; }
  if (minHeight) { minHeight = parseInt(minHeight); } else { minHeight = 16; }

  var width  = document.documentElement.getAttribute("width");
  var height = document.documentElement.getAttribute("height");
  
  if (width  < minWidth)  { document.documentElement.setAttribute("width",  minWidth);  }
  if (height < minHeight) { document.documentElement.setAttribute("height", minHeight); }


  setTimeout(windowRescueOffscreen, 0);
}

/// bring back a window which has lost its way due to something like a disconnected monitor
// TODO: is there some way to detect a screen disconnect event and run this code?
function windowRescueOffscreen() {
  dump("Screen: left/top:\t" + screen.left + "\t" + screen.top + 
       "\tavail w/h\t" + screen.availWidth + "\t" + screen.availHeight + "\n");
  var width  = document.documentElement.getAttribute("width");
  var height = document.documentElement.getAttribute("height");
  var x = document.documentElement.getAttribute("screenX");
  var y = document.documentElement.getAttribute("screenY");
  if (x - screen.left > screen.availWidth)  { document.documentElement.setAttribute("screenX", 0); }
  if (y - screen.top  > screen.availHeight) { document.documentElement.setAttribute("screenY", 0); }
  if (x - screen.left + width  < 0)         { document.documentElement.setAttribute("screenX", 0); }
  if (y - screen.top  + height < 0)         { document.documentElement.setAttribute("screenY", 0); }
  
  
  var x = document.documentElement.getAttribute("screenX");
  var y = document.documentElement.getAttribute("screenY");
  var width  = document.documentElement.getAttribute("width");
  var height = document.documentElement.getAttribute("height");
  dump("After: x/y:\t" + x + "\t" + y + "\tw/h: " + width + "\t" + height + "\n"); 

}

/**
 * \brief Get a style property from an element in the window in the current context.
 * \param el The element.
 * \param styleProp The style property.
 * \return The computed style value.
 */
function getStyle(el,styleProp)
{
  var v;
  if (el) {
    var s = document.defaultView.getComputedStyle(el,null);
    v = s.getPropertyValue(styleProp);
  }
  return v;
}

/**
 * \brief Get a XULWindow (nsIXULWindow) from a regular window.
 * \param win The window for which you want the nsIXULWindow
 * \return The XULWindow for the window or null.
 */
function getXULWindowFromWindow(win) // taken from venkman source
{
    var rv;
    try
    {
        var requestor = win.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
        var nav = requestor.getInterface(Components.interfaces.nsIWebNavigation);
        var dsti = nav.QueryInterface(Components.interfaces.nsIDocShellTreeItem);
        var owner = dsti.treeOwner;
        requestor = owner.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
        rv = requestor.getInterface(Components.interfaces.nsIXULWindow);
    }
    catch (ex)
    {
        rv = null;
    }
    return rv;
}

/**
 * \brief Open a modal dialog.
 * Opens a modal dialog and ensures proper accessibility for this modal dialog.
 */
function SBOpenModalDialog( url, param1, param2, param3, parentWindow )
{
  if (!parentWindow) parentWindow = window;
  // bonus stuff to shut the mac up.
  var chromeFeatures = ",modal=yes,resizable=no";
  if (SBDataGetBoolValue("accessibility.enabled")) chromeFeatures += ",titlebar=yes";
  else chromeFeatures += ",titlebar=no";

  param2 += chromeFeatures;
  var retval = parentWindow.openDialog( url, param1, param2, param3 );
  return retval;
}

/**
 * \brief Open a window.
 * Opens a window and ensures proper accessibility.
 */
function SBOpenWindow( url, param1, param2, param3, parentWindow )
{
  if (!parentWindow) parentWindow = window;
  var titlebar = ",modal=no";
  if (SBDataGetBoolValue("accessibility.enabled")) {
    titlebar += ",titlebar=yes";
    // if in accessible mode, resizable flag determines whether or not the window is resizable
  } else {
    titlebar += ",titlebar=no";
    // if not in accessible mode, resizable flag does not determine if the window is resizable
    // or not, that's determined by the presence or absence of resizers in the xul.
    // on the other hand, if resizable=yes is present in the flags, that create a border
    // around the window in OSX, so remove it
    var flags = param2.split(",");
    for (var i = flags.length - 1 ; i >= 0; --i) {
      if (flags[i] == "resizable=yes" ||
          flags[i] == "resizable")
        flags.splice(i, 1);
    }
    param2 = flags.join(",");
  }

  param2 += titlebar;
  var retval = window.openDialog( url, param1, param2, param3 );

  return retval;
}

/**
 * \brief Quit the application.
 */
function quitApp( )
{
  // Why not stop playback, too?
  try {
    gPPS.stop();
  } catch (e) {
    dump("windowUtils.js:quitApp() Error: could not stop playback.\n");
  }

  // Defer to toolkit/globalOverlay.js
  return goQuitApplication();
}


/**
 * \brief Attempt to restart the application.
 */
function restartApp( )
{

  // Notify all windows that an application quit has been requested.
  var os = Components.classes["@mozilla.org/observer-service;1"]
                     .getService(Components.interfaces.nsIObserverService);
  var cancelQuit = Components.classes["@mozilla.org/supports-PRBool;1"]
                             .createInstance(Components.interfaces.nsISupportsPRBool);
  os.notifyObservers(cancelQuit, "quit-application-requested", "restart");

  // Something aborted the quit process.
  if (cancelQuit.data)
    return;

  // attempt to restart
  var as = Components.classes["@mozilla.org/toolkit/app-startup;1"]
                     .getService(Components.interfaces.nsIAppStartup);
  as.quit(Components.interfaces.nsIAppStartup.eRestart |
          Components.interfaces.nsIAppStartup.eAttemptQuit);

  onExit( );
}


/**
 * \brief Hide an element in the window in the current context.
 * \param e The element to hide.
 */
function hideElement(e) {
  var element = document.getElementById(e);
  if (element) element.setAttribute("hidden", "true");
}

/**
 * \brief Move an element before another element in the window in the current context.
 * \param e The element to hide.
 */
function moveElement(e, before) {
  var element = document.getElementById(e);
  var beforeElement = document.getElementById(before);
  if (element && beforeElement) {
    element.parentNode.removeChild(element);
    beforeElement.parentNode.insertBefore(element, beforeElement);
  }
}

/**
 * \brief Get the name of the platform we are running on.
 * \return The name of the OS platform. (ie. Windows).
 * \retval Windows_NT Running under Windows.
 * \retval Darwin Running under Darwin/OS X.
 * \retval Linux Running under Linux.
 */
function getPlatformString()
{
  try {
    var sysInfo =
      Components.classes["@mozilla.org/system-info;1"]
                .getService(Components.interfaces.nsIPropertyBag2);
    return sysInfo.getProperty("name");
  }
  catch (e) {
    dump("System-info not available, trying the user agent string.\n");
    var user_agent = navigator.userAgent;
    if (user_agent.indexOf("Windows") != -1)
      return "Windows_NT";
    else if (user_agent.indexOf("Mac OS X") != -1)
      return "Darwin";
    else if (user_agent.indexOf("Linux") != -1)
      return "Linux";
    return "";
  }
}

/**
 * \brief Verify if a key event is an ALT-F4 key event.
 * \param evt Key event.
 */
function checkAltF4(evt)
{
  if (evt.keyCode == 0x73 && evt.altKey)
  {
    evt.preventDefault();
    quitApp();
  }
}

var SBStringBundleBundle = Components.classes["@mozilla.org/intl/stringbundle;1"]
                                     .getService(Components.interfaces.nsIStringBundleService)
                                     .createBundle("chrome://songbird/locale/songbird.properties");

/**
 * \brief Lookup a string in the songbird.properties locale file.
 * \param key The key for the string.
 * \param dflt The
 */
function SBString( key, dflt )
{
  // If there is no default, the key is the default.
  var retval = (dflt) ? dflt : key;
  try {
    retval = SBStringBundleBundle.GetStringFromName(key);
  } catch (e) {}
  return retval;
}

/**
 * \brief Convert a string containing binary values to hex.
 * \param input Input string to convert to hex.
 * \return The hex encoded string.
 */
function binaryToHex(input)
{
  var result = "";

  for (var i = 0; i < input.length; ++i)
  {
    var hex = input.charCodeAt(i).toString(16);

    if (hex.length == 1)
      hex = "0" + hex;

    result += hex;
  }

  return result;
}

/**
 * \brief Makes a new URI from a url string
 * \param aURLString String URL.
 * \return nsIURI object.
 */
function newURI(aURLString)
{
  var ioService =
    Components.classes["@mozilla.org/network/io-service;1"]
              .getService(Components.interfaces.nsIIOService);

  try {
    return ioService.newURI(aURLString, null, null);
  }
  catch (e) { }

  return null;
}

/**
 * \brief List all properties on an object and display them in a message box.
 * \param obj The object.
 * \param objName The readable name of the object, helps format the output.
 */
function listProperties(obj, objName)
{
    var columns = 3;
    var count = 0;
    var result = "";
    for (var i in obj)
    {
        result += objName + "." + i + " = " + obj[i] + "\t\t\t";
        count = ++count % columns;
        if ( count == columns - 1 )
        {
          result += "\n";
        }
    }
    alert(result);
}

