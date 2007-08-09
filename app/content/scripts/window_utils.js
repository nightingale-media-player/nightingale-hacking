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



/*
 * Contains functions common to all windows
 */

// Useful constants
var CORE_WINDOWTYPE         = "Songbird:Core";
var STATE_MAXIMIZED         = Components.interfaces.nsIDOMChromeWindow.STATE_MAXIMIZED;
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
 * Adapted from nsUpdateService.js.in. Need to replace with dataremotes.
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
function setPref(aFunc, aPreference, aValue) {
  var prefs = 
    Components.classes[PREFS_SERVICE_CONTRACTID].getService(nsIPrefBranch2);
  return prefs[aFunc](aPreference, aValue);
}

// Shorthand, hammerable 
function eatEvent(evt)
{
  evt.preventBubble();
}

// Minimize
function onMinimize()
{
  document.defaultView.minimize();
}

// Maximize
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
  syncMaxButton();
  syncResizers();
}

function isMaximized() {
  return (window.windowState == STATE_MAXIMIZED);
}

function isMinimized() {
  return (window.windowState == STATE_MINIMIZED);
}

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
  if (isMaximized()) disableResizers();
  else enableResizers();
}
	
function restoreWindow()
{
  if ( isMaximized() )
  {
    document.defaultView.restore();
  }
  syncMaxButton();
  syncResizers();
}

// Exit
function onExit( skipSave )
{
  try
  {
    if ( skipSave != true )
      onWindowSaveSizeAndPosition();
  }
  catch ( err )
  {
    // If you don't have data functions, just close.
  }
  document.defaultView.close();
}

// Hide
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

function onMinimumWindowSize()
{
/*
  //
  // SOMEDAY, we might be able to figure out how to properly constrain our
  // windows back to the proper min-width / min-height values for the document
  //


  // Yah, okay, this function only works if there's one top level object with an id of "window_parent" (sigh)
  var parent = document.getElementById('window_parent');
  if (!parent) parent = document.getElementById('frame_mini'); // grr, bad hardcoding!
  if ( parent ) {
    var w_width = window.innerWidth;
    var w_height = window.innerHeight;
    var p_width = parent.boxObject.width + 4; // borders?!  ugh.
    var p_height = parent.boxObject.height + 4;
    var size = false;
    // However, if in resizing the window size is different from the document's box object
    if (w_width != p_width) {
      // That means we found the document's min width.  Because you can't query it directly.
      w_width = p_width;
      size = true;
    }
    // However, if in resizing the window size is different from the document's box object
    if (w_height != p_height) {
      // That means we found the document's min height.  Because you can't query it directly.
      w_height = p_height;
      size = true;
    }
    if (size)
    {
      window.resizeTo(w_width, w_height);
    }
  }
*/  
}

function onWindowResizeComplete() {
  onMinimumWindowSize();
  onWindowSaveSizeAndPosition();
}

function onWindowDragComplete() {
  onWindowSavePosition();
}

// No forseen need to save _just_ size without position
function onWindowSaveSizeAndPosition()
{
  var root = "window." + document.documentElement.id;
  if (!isMaximized() && !isMinimized()) {
  /*
    dump("******** onWindowSaveSizeAndPosition: root:" + root +
                                    " root.w:" + SBDataGetIntValue(root+'.w') + 
                                    " root.h:" + SBDataGetIntValue(root+'.h') + 
                                    "\n");
  */
    SBDataSetIntValue( root + ".w", document.documentElement.boxObject.width );
    SBDataSetIntValue( root + ".h", document.documentElement.boxObject.height );

    onWindowSavePosition();
  }
  SBDataSetBoolValue( root + ".maximized", isMaximized() );
}

function onWindowSavePosition()
{
  var root = "window." + document.documentElement.id;
/*
  dump("******** onWindowSavePosition: root:" + root +
                                  " root.w:" + SBDataGetIntValue(root+'.x') + 
                                  " root.h:" + SBDataGetIntValue(root+'.y') + 
                                  "\n");
*/
  if (!(document.documentElement.boxObject.screenX == -32000 ||document.documentElement.boxObject.screenY == -32000)) { // never record the coordinates of a cloaked window !
    SBDataSetIntValue( root + ".x", document.documentElement.boxObject.screenX );
    SBDataSetIntValue( root + ".y", document.documentElement.boxObject.screenY );
  }
}

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

function delayedActivate()
{
  setTimeout( windowFocus, 50 );
}

// No forseen need to load _just_ size without position
function onWindowLoadSizeAndPosition()
{
  delayedActivate();

  var root = "window." + document.documentElement.id;
/*
  dump("******** onWindowLoadSizeAndPosition: root:" + root +
                                  " root.w:" + SBDataGetIntValue(root+'.w') + 
                                  " root.h:" + SBDataGetIntValue(root+'.h') + 
                                  " box.w:" + document.documentElement.boxObject.width +
                                  " box.h:" + document.documentElement.boxObject.height +
                                  "\n");
*/
  // If they have not been set they will be ""
  if ( SBDataGetStringValue( root + ".w" ) == "" ||
       SBDataGetStringValue( root + ".h" ) == "" )
  {
    // nothing to do to set the default width/height, we already get a default from xul and css definitions
    // but still try to load position !
    onWindowLoadPosition();
    return;
  }

  // get the data once.
  var rootW = SBDataGetIntValue( root + ".w" );
  var rootH = SBDataGetIntValue( root + ".h" );

  if ( rootW && rootH )
  {
    // test if the window is resizable. if it is, and rootW/rootH are below 
    // some limit, skip these steps so that we reload the default w/h from xul and css
    var resetsize = false;
    var resizers = document.getElementsByTagName("resizer");
    var resizable = resizers.length > 0;
    var minwidth = getStyle(document.documentElement, "min-width");
    var minheight = getStyle(document.documentElement, "min-height");
    if (minwidth) minwidth = parseInt(minwidth);
    if (minheight) minheight = parseInt(minheight);

/*
    dump("SCREEN LOAD: " + screen.width + " " + screen.height + "\n");
    dump("window = " + document.documentElement.getAttribute("id") + "\n");
    dump("minwidth = " + minwidth + "\n");
    dump("rootW = " + rootW + "\n");
    dump("minheight = " + minheight + "\n");
    dump("rootH = " + rootH + "\n");
    dump("resizable = " + resizable + "\n");
*/

    // also, if the desired w/h are larger than the screen, we should reset.

    if (resizable && 
        ( ((minwidth && rootW < minwidth) || (minheight && rootH < minheight))
        || (rootW > screen.width || rootH > screen.height) ))
      resetsize = true;
/*
    dump("resetsize = " + resetsize + "\n");
*/
    
    if (!resetsize) { 
      // https://bugzilla.mozilla.org/show_bug.cgi?id=322788
      // YAY YAY YAY the windowregion hack actualy fixes this :D
      window.resizeTo( rootW, rootH );

      // for some reason, the resulting size isn't what we're asking (window
      //   currently has a border?) so determine what the difference is and
      //   add it to the resize
      var diffW = rootW - document.documentElement.boxObject.width;
      var diffH = rootH - document.documentElement.boxObject.height;
      window.resizeTo( rootW + diffW, rootH + diffH);
    } 
  }
  onWindowLoadPosition();
  // and of course, like focusing, maximizing at this stage won't work :( so introduce a small delay
  setTimeout(delayedMaximize, 50);
}

function delayedMaximize() {
  var root = "window." + document.documentElement.id;
  if (SBDataGetBoolValue(root + ".maximized")) onMaximize();
}

function getStyle(el,styleProp)
{
  var v;
  if (el) {
    var s = document.defaultView.getComputedStyle(el,null);
    v = s.getPropertyValue(styleProp);
  }
  return v;
}

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

function onWindowLoadPosition()
{
  var root = "window." + document.documentElement.id;
/*
  dump("******** onWindowLoadPosition: root:" + root +
                                  " root.x:" + SBDataGetIntValue(root+'.x') +
                                  " root.y:" + SBDataGetIntValue(root+'.y') +
                                  " box.x:" + document.documentElement.boxObject.screenX +
                                  " box.y:" + document.documentElement.boxObject.screenY +
                                  "\n");
*/
  // If they have not been set they will be ""
  if ( SBDataGetStringValue( root + ".x" ) == "" ||
       SBDataGetStringValue( root + ".y" ) == "" )
  {
    var win = getXULWindowFromWindow(window);
    if (win) win.center(null, true, true);
    return;
  }

  // get the data once.
  var rootX = SBDataGetIntValue( root + ".x" );
  var rootY = SBDataGetIntValue( root + ".y" );

  window.moveTo( rootX, rootY );
  // do the (more or less) same adjustment for x,y as we did for w,h
  var diffX = rootX - document.documentElement.boxObject.screenX;
  var diffY = rootY - document.documentElement.boxObject.screenY;

  var platform;
  try {
    var sysInfo =
      Components.classes["@mozilla.org/system-info;1"]
                .getService(Components.interfaces.nsIPropertyBag2);
    platform = sysInfo.getProperty("name");                                          
  }
  catch (e) {
    dump("System-info not available, trying the user agent string.\n");
    var user_agent = navigator.userAgent;
    if (user_agent.indexOf("Linux") != -1)
      platform = "Linux";
  }

  // This fix not needed for Linux - might need to add a MacOSX check.
  if (platform != "Linux")
    window.moveTo( rootX - diffX, rootY - diffY );
}



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
    var newflags;
    for (var i in flags) {
      if (newflags != "") newflags += ",";
      if (flags[i] == "resizable=yes" ||
          flags[i] == "resizable") 
        continue;
    }
    param2 = newflags;
  }

  param2 += titlebar;
  var retval = window.openDialog( url, param1, param2, param3 );

  return retval;
}

function quitApp( skipSave )
{
  onExit(skipSave); // Don't always save the window position.
  // Why not stop playback, too?
  try {
    gPPS.stop();
  } catch (e) {}
  var as = Components.classes["@mozilla.org/toolkit/app-startup;1"].getService(Components.interfaces.nsIAppStartup);
  if (as)
  {
    const V_ATTEMPT = 2;
    as.quit(V_ATTEMPT);
  }
}

function hideRealResizers() {
  var resizers = document.getElementsByTagName("resizer");
  for (var i=0;i<resizers.length;i++) {
    resizers[i].setAttribute("hidden", "true");
  }
}

function showRealResizers() {
  var resizers = document.getElementsByTagName("resizer");
  for (var i=0;i<resizers.length;i++) {
    resizers[i].removeAttribute("hidden");
  }
}

function hideFakeResizers() {
  var xresizers = document.getElementsByTagName("x_resizer");
  for (var i=0;i<xresizers.length;i++) {
    xresizers[i].setAttribute("hidden", "true");
  }
}

function showFakeResizers() {
  var xresizers = document.getElementsByTagName("x_resizer");
  for (var i=0;i<xresizers.length;i++) {
    xresizers[i].removeAttribute("hidden");
  }
}

function disableResizers() {
  if (SBDataGetBoolValue("accessibility.enabled")) return;
  hideRealResizers();
  showFakeResizers();
}
	
function enableResizers() {
  if (SBDataGetBoolValue("accessibility.enabled")) return;
  hideFakeResizers();
  showRealResizers();
}

function hideElement(e) {
  var element = document.getElementById(e);
  if (element) element.setAttribute("hidden", "true");
}

function moveElement(e, before) {
  var element = document.getElementById(e);
  var beforeElement = document.getElementById(before);
  if (element && beforeElement) {
    element.parentNode.removeChild(element);
    beforeElement.parentNode.insertBefore(element, beforeElement);
  }
}

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
* Convert a string containing binary values to hex.
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
 * Makes a new URI from a url string
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

// Debugging Tool
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
