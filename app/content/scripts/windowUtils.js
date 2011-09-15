/*
// BEGIN SONGBIRD GPL
//
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
 * \file windowUtils.js
 * \brief Window Utility constants, functions and objects that are common
 * to all windows.
 *
 * Note: This file is dependent on chrome://global/content/globalOverlay.js
 */
 
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");

if (typeof(Cc) == "undefined")
  window.Cc = Components.classes;
if (typeof(Ci) == "undefined")
  window.Ci = Components.interfaces;
if (typeof(Cu) == "undefined")
  window.Cu = Components.utils;
if (typeof(Cr) == "undefined")
  window.Cr = Components.results;
 

/**
 * \brief The Songbird Core Window Type.
 */
var CORE_WINDOWTYPE         = "Songbird:Core";

/**
 * \brief Maximized State value.
 */
var STATE_MAXIMIZED         = Ci.nsIDOMChromeWindow.STATE_MAXIMIZED;

/**
 * \brief Minimized State value.
 */
var STATE_MINIMIZED         = Ci.nsIDOMChromeWindow.STATE_MINIMIZED;

var gMM      = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                 .getService(Ci.sbIMediacoreManager);
var gPrompt  = Cc["@mozilla.org/embedcomp/prompt-service;1"]
                 .getService(Ci.nsIPromptService);
var gPrefs   = Cc["@mozilla.org/preferences-service;1"]
                 .getService(Ci.nsIPrefBranch);
var gConsole = Cc["@mozilla.org/consoleservice;1"]
                 .getService(Ci.nsIConsoleService);

var gTypeSniffer = Cc["@songbirdnest.com/Songbird/Mediacore/TypeSniffer;1"]
                     .createInstance(Ci.sbIMediacoreTypeSniffer);
                                    

/**
 * Simple JS container that holds rectangle information
 * for (x, y) and (width, height).
 */
function sbScreenRect(inWidth, inHeight, inX, inY) {
  this.width = inWidth;
  this.height = inHeight;
  this.x = inX;
  this.y = inY;
}

/**
 * Get the current maximum available screen rect based on which screen
 * the window is currently on.
 * @return A |sbScreenRect| object containing the max available 
 *         coordinates of the current screen.
 */
function getCurMaxScreenRect() {
  var screenManager = Cc["@mozilla.org/gfx/screenmanager;1"]
                        .getService(Ci.nsIScreenManager);
                        
  var curX = parseInt(document.documentElement.boxObject.screenX);
  var curY = parseInt(document.documentElement.boxObject.screenY);
  var curWidth = parseInt(document.documentElement.boxObject.width);
  var curHeight = parseInt(document.documentElement.boxObject.height);
  
  var curScreen = screenManager.screenForRect(curX, curY, curWidth, curHeight);
  var x = {}, y = {}, width = {}, height = {};
  curScreen.GetAvailRect(x, y, width, height);
  
  return new sbScreenRect(width.value, height.value, x.value, y.value);
}


/**
 * Since the Mac doesn't technically "maximize" - it "zooms", this controller
 * recreates that functionality for mac windows.
 *
 * KREEGER: This functionality is also affected by mozbug 407405 and will make
 *          zoom to the secondary monitor work properly once that bug is fixed.
 */
function sbMacWindowZoomController() {
  this._init();
}

sbMacWindowZoomController.prototype = {
  _mIsZoomed:              false,
  _mIsResizeEventFromZoom: false,
  _mSavedXPos:   0,
  _mSavedYPos:   0,
  _mSavedWidth:  0,
  _mSavedHeight: 0,
 
  _init: function() {
    // Listen to document dragging events, we need a closure when a message
    // is dispatched directly through the document object.
    var self = this;
    this._windowdragexit = function(evt) {
      self._onWindowDragged();
    };
    this._windowresized = function(evt) {
      self._onWindowResized();
    };
    this._documentunload = function(evt) {
      self._onUnload();
    };
    document.addEventListener("ondragexit", this._windowdragexit, false);
    document.addEventListener("resize", this._windowresized, false);
    document.addEventListener("unload", this._documentunload, false);
  },
  
  _onWindowResized: function() {
    if (this._mIsZoomed) {
      if (this._mIsResizeEventFromZoom) {
        this._mIsResizeEventFromZoom = false;
      }
      // Only save window coordinates if the window is out of the 'zoomed' area..
      else if (!this._isConsideredZoomed()) {
        this._mIsZoomed = false;
        this._saveWindowCoords();
      }
    }
  },
  
  _onWindowDragged: function() {
    var maxScreenRect = getCurMaxScreenRect();
    var curX = parseInt(document.documentElement.boxObject.screenX);
    var curY = parseInt(document.documentElement.boxObject.screenY);

    if (this._mIsZoomed) {
      // If the window was moved more than 10 pixels either way - this
      // window is no longer considered "zoomed".
      if (curX > (maxScreenRect.x + 10) || curY > (maxScreenRect.y + 10)) {
        this._mIsZoomed = false;
        this._saveWindowCoords();
      }
    }
    else {
      this._mIsZoomed = this._isConsideredZoomed();
    }
  },
  
  onZoom: function() {    
    if (this._mIsZoomed) {
      window.resizeTo(this._mSavedWidth, this._mSavedHeight);
      window.moveTo(this._mSavedXPos, this._mSavedYPos);
      this._mIsZoomed = false;
    }
    else {
      this._saveWindowCoords();
      
      var maxScreenRect = getCurMaxScreenRect();
      window.moveTo(maxScreenRect.x, maxScreenRect.y);
      window.resizeTo(maxScreenRect.width, maxScreenRect.height);
      
      this._mIsZoomed = true;
      this._mIsResizeEventFromZoom = true;
    }
  },
  
  _onUnload: function() {
    document.removeEventListener("ondragexit", this._windowdragexit, false);
    document.removeEventListener("resize", this._windowresized, false);
    document.removeEventListener("unload", this._documentunload, false);
    this._windowdragexit = null;
    this._windowresized = null;
    this._documentunload = null;
  },
  
  _saveWindowCoords: function() {
    this._mSavedYPos = parseInt(document.documentElement.boxObject.screenY);
    this._mSavedXPos = parseInt(document.documentElement.boxObject.screenX);
    this._mSavedWidth = parseInt(document.documentElement.boxObject.width);
    this._mSavedHeight = parseInt(document.documentElement.boxObject.height);
  },

  _isConsideredZoomed: function() {
    // If the window was moved into the "zoom" zone 
    // (10 pixel square in top-left corner) then it should be considered 
    // "zoomed" and be set as so.
    var isZoomed = false;
    var maxScreenRect = getCurMaxScreenRect();
    var curX = parseInt(document.documentElement.boxObject.screenX);
    var curY = parseInt(document.documentElement.boxObject.screenY);
    var curWidth = parseInt(document.documentElement.boxObject.width);
    var curHeight = parseInt(document.documentElement.boxObject.height);

    if (curX < (maxScreenRect.x + 10) && 
        curY < (maxScreenRect.y + 10) &&
        curWidth > (maxScreenRect.width - 10) && 
        curHeight > (maxScreenRect.height - 10))
    {
      isZoomed = true;
    }

    return isZoomed;
  }
};

// Only use our mac zoom controller on the mac:
var macZoomWindowController = null;
if (getPlatformString() == "Darwin")
  macZoomWindowController = new sbMacWindowZoomController();


// Strings are cool.
var theSongbirdStrings = document.getElementById( "songbird_strings" );

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
function onMaximize(aMaximize)
{
  if ( macZoomWindowController != null )
  {
    macZoomWindowController.onZoom();
  }
  else
  {
    if ( aMaximize )
    {
      document.defaultView.maximize();
      // Force sizemode attribute, due to mozbug 409095.
      document.documentElement.setAttribute("sizemode", "maximized");
    }
    else
    {
      document.defaultView.restore();
      // Force sizemode attribute, due to mozbug 409095.
      document.documentElement.setAttribute("sizemode", "normal");
    }
  }
  // TODO
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
function syncResizers()
{
  // TODO?
  if (isMaximized()) disableResizers();
  else enableResizers();
}
*/

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
  // Fire custom DOM event so that potential listeners interested in
  // the fact that the window is about to hide can do something about it.
  e = document.createEvent("UIEvents");
  e.initUIEvent("hide", true, true, window, 1);
  document.dispatchEvent(e);

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
  /**
   * \brief Get a style property from an element in the window in the current context.
   * \param el The element.
   * \param styleProp The style property.
   * \param defaultValue [optional] the default value to return; 0 if not supplied
   * \return The computed style value.
   */
  function getStyle(el, styleProp, defaultValue)
  {
    if (!defaultValue)
      defaultValue = 0;
    var v = defaultValue;
    if (el) {
      var s = document.defaultView.getComputedStyle(el,null);
      v = s.getPropertyValue(styleProp);
    }
    return parseInt(v, 10) || defaultValue;
  }

  delayedActivate();
  
  // Grab all the values we'll need.
  var x, oldx = x = parseInt(document.documentElement.boxObject.screenX, 10);
  var y, oldy = y = parseInt(document.documentElement.boxObject.screenY, 10);
  
  /*
   * xul:    the property as set on XUL, or via persist=
   * min:    the property minimum as computed by CSS.  Has a fallback minimum.
   * max:    the property maximum as computed by CSS.
   */
  var width = {
    xul: parseInt(document.documentElement.getAttribute("width"), 10),
    min: Math.max(getStyle(document.documentElement, "min-width"), 16),
    max: getStyle(document.documentElement, "max-width", Number.POSITIVE_INFINITY)
  };
  var height = {
    xul: parseInt(document.documentElement.getAttribute("height"), 10),
    min: Math.max(getStyle(document.documentElement, "min-height"), 16),
    max: getStyle(document.documentElement, "max-height", Number.POSITIVE_INFINITY)
  };
  
  // correct width
  var newWidth = width.xul || 0;

  // correct for maximum and minimum sizes (including not larger than the screen)
  newWidth = Math.min(newWidth, width.max);
  newWidth = Math.min(newWidth, screen.availWidth);
  newWidth = Math.max(newWidth, width.min);

  // correct height
  var newHeight = height.xul || 0;

  // correct for maximum and minimum sizes (including not larger than the screen)
  newHeight = Math.min(newHeight, height.max);
  newHeight = Math.min(newHeight, screen.availHeight);
  newHeight = Math.max(newHeight, height.min);

  // resize the window if necessary
  if (newHeight != height.xul || newWidth != width.xul) {
    window.resizeTo(newWidth, newHeight);
  }

  // check if we need to move the window, and
  // move fully offscreen windows back onto the center of the screen
  var screenRect = getCurMaxScreenRect();
  if ((x - screenRect.x > screenRect.width)   ||  // offscreen right
      (x - screenRect.x + newWidth  < 0)      ||  // offscreen left
      (y - screenRect.y > screenRect.height)  ||  // offscreen bottom
      (y - screenRect.y + newHeight  < 0))        // offscreen top
  {
    x = (screenRect.width / 2) - (window.outerWidth / 2);
    x = Math.max(x, 0); // don't move window left of zero.
    y = (screenRect.height / 2) - (window.outerHeight / 2);
    y = Math.max(y, 0); // don't move window above zero.
  }

  // Make sure our (x, y) coordinate is at least on screen.
  if (x < screenRect.x) {
    x = screenRect.x;
  }
  if (y < screenRect.y) {
    y = screenRect.y;
  }

  if (!document.documentElement.hasAttribute("screenX")) {
    // no persisted x, move the window back into the screen
    // (we allow the user to persist having the window be partially off screen)
    x = Math.max(x, 0);
    x = Math.min(x, screen.availWidth - newWidth);
  }
  if (!document.documentElement.hasAttribute("screenY")) {
    // no persisted y, move the window back into the screen
    // (we allow the user to persist having the window be partially off screen)
    y = Math.max(y, 0);
    y = Math.min(y, screen.availHeight - newHeight);
  }

  // Move the window if necessary
  if (x != oldx || y != oldy) {
    // Moving a maximized window will break the maximized state, so save
    // the current state so that the window can be re-maximized.
    var isCurMaximized = 
      document.documentElement.getAttribute("sizemode") == "maximized";
    
    // Move
    window.moveTo(x, y);

    // Re-maximize
    if (isCurMaximized) {
      onMaximize(true);
    }
  }

/*
  // debugging dumps
  Components.utils.reportError(<>
    {arguments.callee.name}:
    location: {location.href}
    persisted position: ({document.documentElement.getAttribute("screenX")}, {document.documentElement.getAttribute("screenY")})
    previous position: ({oldx}, {oldy})
    computed position: ({x}, {y})
    
    new dimensions: {newWidth} x {newHeight}
    current dimensions: {width.actual} x {height.actual}
    min dimensions: {width.min} x {height.min}
    max dimensions: {width.max} x {height.max}
</>);
/* */

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
  if (!parentWindow) { 
    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                       .getService(Components.interfaces.nsIWindowMediator);
    parentWindow = wm.getMostRecentWindow("Songbird:Main");
  }
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
  if (!parentWindow) { 
    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                       .getService(Components.interfaces.nsIWindowMediator);
    parentWindow = wm.getMostRecentWindow("Songbird:Main");
  }
  
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
    if (gMM.playbackControl)
      gMM.playbackControl.stop();
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
 * \retval SunOS Running under Solaris.
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
    else if (user_agent.indexOf("SunOS") != -1)
      return "SunOS";
    return "";
  }
}

/**
 * \brief Verify if a key event is an ALT-F4 key event.
 * \param evt Key event.
 */
function checkQuitKey(evt)
{
  // handle alt-F4 on all platforms
  if (evt.keyCode == 0x73 && evt.altKey)
  {
    evt.preventDefault();
    quitApp();
  }

  // handle ctrl-Q on UNIX
  let platform = getPlatformString();
  if (platform == 'Linux' || platform == 'SunOS') {
    let keyCode = String.fromCharCode(evt.which).toUpperCase();
    if ( keyCode == 'Q' && evt.ctrlKey) {
      evt.preventDefault();
      quitApp();
    }
  }
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
        try {
          result += objName + "." + i + " = " + obj[i] + "\t\t\t";
        } catch (e) {
          result += objName + "." + i + " = [exception thrown]\t\t\t";
        }
        count = ++count % columns;
        if ( count == columns - 1 )
        {
          result += "\n";
        }
    }
    alert(result);
}


/**
 * \brief Handle initialization of layout windows
 */
function onLayoutLoad(event) {
  // don't leak, plz
  window.removeEventListener('load', onLayoutLoad, false);

  var feathersMgr = 
    Components.classes['@songbirdnest.com/songbird/feathersmanager;1']
              .getService(Components.interfaces.sbIFeathersManager);
  
  if (feathersMgr.currentLayoutURL == window.location.href) {
     // this is the primary window for the current layout
    var nativeWinMgr = 
      Components.classes["@songbirdnest.com/integration/native-window-manager;1"]
      .getService(Components.interfaces.sbINativeWindowManager);
    
    // Check to see if "ontop" is supported with the current window manager
    // and the current layout:
    if (nativeWinMgr && nativeWinMgr.supportsOnTop &&
        feathersMgr.canOnTop(feathersMgr.currentLayoutURL, 
                             feathersMgr.currentSkinName)) 
    {
      var isOnTop = feathersMgr.isOnTop(feathersMgr.currentLayoutURL,
                                        feathersMgr.currentSkinName);
      
      nativeWinMgr.setOnTop(window, isOnTop);
    }
    
    // Check to see if shadowing is supported and enable it.
    if (nativeWinMgr && nativeWinMgr.supportsShadowing) 
    {
      nativeWinMgr.setShadowing(window, true);
    }
    
    // Set the min-window size if the window supports it
    if (nativeWinMgr.supportsMinimumWindowSize) {
      var cstyle = window.getComputedStyle(document.documentElement, '');
      if (cstyle) {
        var minWidth = parseInt(cstyle.minWidth);
        var minHeight = parseInt(cstyle.minHeight);
        if (minWidth > 0 && minHeight > 0) {
          nativeWinMgr.setMinimumWindowSize(window, minWidth, minHeight);
        }
        
        var maxWidth = parseInt(cstyle.maxWidth);
        var maxHeight = parseInt(cstyle.maxHeight);
        if (maxWidth > 0 && maxHeight > 0) {
          nativeWinMgr.setMaximumWindowSize(window, maxWidth, maxHeight);
        }
      }
    }
  }
}
window.addEventListener('load', onLayoutLoad, false);


/**
 * \brief Add a "platform" attribute to the current document's root element.
 * This attribute is used by CSS developers to be able to target a particular
 * platform.
 * eg: window[platform=Darwin] {  someOSXspecificstuff }
 */ 
function initializeDocumentPlatformAttribute() {
    // Perform platform specific customization
    var platform = getPlatformString();

    // Set attributes on the document element so we can use them in CSS.
    document.documentElement.setAttribute("platform",platform);
}

/**
 * \brief Get the main window browser.
 *
 * \return Main window browser.
 */

function SBGetBrowser() 
{
  // Return global browser if defined.
  if (typeof gBrowser != 'undefined') {
    return gBrowser;
  }

  // Get the main window.
  var mainWindow = window
                    .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                    .getInterface(Components.interfaces.nsIWebNavigation)
                    .QueryInterface(Components.interfaces.nsIDocShellTreeItem)
                    .rootTreeItem
                    .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                    .getInterface(Components.interfaces.nsIDOMWindow);

  // Return the main window browser.
  if (typeof mainWindow.gBrowser != 'undefined') {
    return mainWindow.gBrowser;
  }

  return null;
}

/**
 * \brief Get the application notification box.
 *
 * \return Application notification box.
 */

function SBGetApplicationNotificationBox() {
  return document.getElementById("application-notificationbox");
}

/**
 * Based on original code from Mozilla's browser/base/content/browser.js
 *
 * Update the global flag that tracks whether or not any edit UI (the Edit menu,
 * edit-related items in the context menu) is visible, then update the edit
 * commands' enabled state accordingly.  We use this flag to skip updating the
 * edit commands on focus or selection changes when no UI is visible to
 * improve performance (including pageload performance, since focus changes
 * when you load a new page).
 *
 * If UI is visible, we use goUpdateGlobalEditMenuItems to set the commands'
 * enabled state so the UI will reflect it appropriately.
 * 
 * If the UI isn't visible, we enable all edit commands so keyboard shortcuts
 * still work and just lazily disable them as needed when the user presses a
 * shortcut.
 *
 * This doesn't work on Mac, since Mac menus flash when users press their
 * keyboard shortcuts, so edit UI is essentially always visible on the Mac,
 * and we need to always update the edit commands.  Thus on Mac this function
 * is a no op.
 */
var gEditUIVisible = true;
function updateEditUIVisibility()
{
  if ( getPlatformString() != "Darwin" ) {
    let editMenuPopupState = document.getElementById("menu_EditPopup").state;
    let contextMenuPopupState = document.getElementById("contentAreaContextMenu").state;

    // The UI is visible if the Edit menu is opening or open, if the context menu
    // is open, or if the toolbar has been customized to include the Cut, Copy,
    // or Paste toolbar buttons.
    gEditUIVisible = editMenuPopupState == "showing" ||
                     editMenuPopupState == "open" ||
                     contextMenuPopupState == "showing" ||
                     contextMenuPopupState == "open" ? true : false;

    // If UI is visible, update the edit commands' enabled state to reflect
    // whether or not they are actually enabled for the current focus/selection.
    if (gEditUIVisible) {
      goUpdateGlobalEditMenuItems();
    } else {
      // Otherwise, enable all commands, so that keyboard shortcuts still work,
      // then lazily determine their actual enabled state when the user presses
      // a keyboard shortcut.
      goSetCommandEnabled("cmd_undo", true);
      goSetCommandEnabled("cmd_redo", true);
      goSetCommandEnabled("cmd_cut", true);
      goSetCommandEnabled("cmd_copy", true);
      goSetCommandEnabled("cmd_paste", true);
      goSetCommandEnabled("cmd_selectAll", true);
      goSetCommandEnabled("cmd_delete", true);
      goSetCommandEnabled("cmd_switchTextDirection", true);
    }
  }
}


/**
 * This method updates all commands that deal with the type of content that is
 * currently in the selected browser tab such as find/find again.
 */
function goUpdateGlobalContentMenuItems()
{
  goUpdateCommand("cmd_find");
  goUpdateCommand("cmd_findAgain");
  goUpdateCommand("cmd_print");
  goUpdateCommand("cmd_getartwork");
  goUpdateCommand("cmd_control_previous");
  goUpdateCommand("cmd_control_next");
  goUpdateCommand("cmd_exportmedia");
}

/**
 * This method assists with updating the menu items.
 * See the <commandset id="toolsMenuCommandSetMetadata" tag in
 * layoutBaseOverlay.xul
 */
function goUpdateGlobalMetadataMenuItems()
{
  // This will call gSongbirdWindowController.isCommandEnabled in
  // mainPlayerWindow.js with the command id as the parameter
  goUpdateCommand("cmd_metadata");
  goUpdateCommand("cmd_editmetadata");
  goUpdateCommand("cmd_viewmetadata");
  goUpdateCommand("cmd_reveal");
}

/**
 * Global function to cycle the current feather to the next layout.
 */
function toggleNextFeatherLayout()
{
  Components.classes['@songbirdnest.com/songbird/feathersmanager;1']
            .getService(Components.interfaces.sbIFeathersManager)
            .switchToNextLayout();
}

