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
var PREF_BONES_SELECTED     = "general.bones.selectedMainWinURL";
var PREF_FEATHERS_SELECTED  = "general.skins.selectedSkin";
var BONES_DEFAULT_URL       = "chrome://rubberducky/content/xul/mainwin.xul";
var FEATHERS_DEFAULT_NAME   = "rubberducky";

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



function SBOpenModalDialog( url, param1, param2, param3 )
{
  PushBackscanPause();

  // bonus stuff to shut the mac up.
  var chromeFeatures = ",modal=yes,resizable=no";
  if (SBDataGetBoolValue("accessibility.enabled")) chromeFeatures += ",titlebar=yes";
  else chromeFeatures += ",titlebar=no";

  param2 += chromeFeatures;
  var retval = window.openDialog( url, param1, param2, param3 );
  PopBackscanPause();
  return retval;
}

function SBOpenWindow( url, param1, param2, param3 )
{
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

function PushBackscanPause()
{
  try
  {
    // increment the backscan pause count
    SBDataIncrementValue( "backscan.paused" );
  }
  catch ( err )
  {
    alert( "PushBackscanPause - " + err );
  }
}

function PopBackscanPause()
{
  try
  {
    // decrement the backscan pause count to a floor of 0
    SBDataDecrementValue( "backscan.paused", 0 );
  }
  catch ( err )
  {
    alert( "PushBackscanPause - " + err );
  }
}


function quitApp( skipSave )
{
  onExit(skipSave); // Don't always save the window position.
  // Why not stop playback, too?
  try {
    gPPS.stop();
  } catch (e) {}
  var nsIMetrics = new Components.Constructor("@songbirdnest.com/Songbird/Metrics;1", "sbIMetrics");
  var MetricsService = new nsIMetrics();
  MetricsService.setSessionFlag(false); // mark this session as clean, we did not crash
  var as = Components.classes["@mozilla.org/toolkit/app-startup;1"].getService(Components.interfaces.nsIAppStartup);
  if (as)
  {
    // do NOT replace '_' with '.', or it will be handled as a metrics data: it would be posted to the metrics aggregator, then reset to 0 automatically
    SBDataSetBoolValue("metrics_ignorenextstartup", false);
    const V_ATTEMPT = 2;
    as.quit(V_ATTEMPT);
  }
}

function SBMainWindowOpen()
{
  var location = "" + window.location; // Grrr.  Dumb objects.
  if ( location.indexOf("?video") == -1 )
  {
    // Get mainwin URL
    var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
    var mainwin = "chrome://rubberducky/content/xul/mainwin.xul";
    try {
      mainwin = prefs.getCharPref("general.bones.selectedMainWinURL", mainwin);  
    } catch (err) {}
   
    // Open the window

    var chromeFeatures = "chrome,modal=no,toolbar=no,popup=no";
    if (SBDataGetBoolValue("accessibility.enabled")) chromeFeatures += ",resizable=yes"; else chromeFeatures += ",titlebar=no";

    window.open( mainwin, "", chromeFeatures );
    onExit();
  }
}

/* Temporary hack to perform dynamic modifications depending on the platform,
 * ie, to get the system buttons in the right (er, left) place on a 
 * mac. To be replaced with some sort of XBL/preprocessor magic after 0.2.
 *
 * aBoxId  - The id of the <hbox> element used for the titlebar
 * aLabelId - The id of the <label> element used to display the title
 */
function fixWindow(aBoxId, aLabelId) 
{
  // Constants for titlebar manipulation
  const HBOX_CONTROLS_ID = "controlBox";
  const HBOX_TITLE_ID    = "titleBox";
  const HBOX_MINI_ID     = "miniBox";

  const BUTTON_CLOSE_ID  = "sysbtn_close";
  const BUTTON_MIN_ID    = "sysbtn_minimize";
  const BUTTON_MAX_ID    = "sysbtn_maximize";
  const BUTTON_MINI_ID   = "sysbtn_minimode";
  const BUTTON_HIDE_ID   = "sysbtn_hide";
  const BUTTON_APP_ID    = "app_icon";

  const CLASS_SYSBTNS    = "sb_faceplate";
  const CLASS_HBOX_OSX   = "mac";
  
  // Constants for resizer manipulation
  const HBOX_RESIZERS_TOP_ID    = "frame_resizers_top";
  const HBOX_RESIZERS_LEFT_ID   = "frame_resizers_left";
  const HBOX_RESIZERS_RIGHT_ID  = "frame_resizers_right";
  const HBOX_RESIZERS_BOTTOM_ID = "frame_resizers_bottom";
  
  const RESIZER_SPACE_SIZE = 9;

  const MENU_ID          = "songbird_menu";
  const LABEL_APPTITLE_ID = "mainwin_app_title";

  var platform;
  var accessible;
  
  var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
  
  var curfeathers = "rubberducky";
  try {
    curfeathers = prefs.getCharPref("general.skins.selectedSkin");  
  } catch (err) {}
  
  // Test if this is an accessible skin, and if so, make the necessary modification to the window so that it uses the OS frames and widgets

  accessible = (curfeathers.indexOf("/plucked") > 0);
  
  try {
    var sysInfo =
      Components.classes["@mozilla.org/system-info;1"]
                .getService(Components.interfaces.nsIPropertyBag2);
    platform = sysInfo.getProperty("name");                                          
  }
  catch (e) {
    var user_agent = navigator.userAgent;
    if (user_agent.indexOf("Mac OS X") != -1)
      platform = "Darwin";
    else
      platform = "";
  }

  // Fixes for OS X.

  if (!accessible && platform == "Darwin") {

    if (!(aBoxId && aLabelId))
      throw Components.results.NS_ERROR_INVALID_ARG;

    // Small helper to apply default attributes to new buttons
    function createNewSysButton(aTag, aId) {
      if (!(aTag && aId))
        throw Components.results.NS_ERROR_INVALID_ARG;

      var button = document.createElement(aTag);
      if (!button)
        throw Components.results.NS_ERROR_OUT_OF_MEMORY;

      button.id = aId;
      button.setAttribute("class", CLASS_SYSBTNS);
      button.setAttribute("disabled", "true");

      return button;
    }
    
    // Small helper to create a spacer with a default width of 5 (unless given)
    function createNewSpacer(aWidth) {
      var spacer = document.createElement("spacer");
      if (!spacer)
        throw Components.results.NS_ERROR_OUT_OF_MEMORY;

      spacer.width = aWidth ? aWidth : 6;
      return spacer;
    }
    
    // Small helper to remove all children of a certain tag
    function removeAllChildrenByTag(aParent, aTag) {
      if (!(aParent && aTag))
        throw Components.results.NS_ERROR_INVALID_ARG;
    
      var children = aParent.getElementsByTagName(aTag);
      var count = children.length;
      for (var index = 0; index < count; index++) {
        var child = children.item(0);
        if (child)
          aParent.removeChild(child);
      }
    }

    // Get the titlebar
    var topBar = document.getElementById(aBoxId);
    topBar.align = "center";
    
    // Get the title label
    var topLabel = document.getElementById(aLabelId);
    topBar.removeChild(topLabel);

    // Remove the app icon, if it exists
    var appIcon = document.getElementById(BUTTON_APP_ID);
    if (appIcon)
      topBar.removeChild(appIcon);

    // Get the close, minimize, and maximize buttons
    var closeButton = document.getElementById(BUTTON_CLOSE_ID);
    var minButton = document.getElementById(BUTTON_MIN_ID);
    var maxButton = document.getElementById(BUTTON_MAX_ID);  

    // Get the miniplayer button
    var miniButton = document.getElementById(BUTTON_MINI_ID);
    if (miniButton)
      topBar.removeChild(miniButton);

    // And then there's the 'hide' button on the video window
    var hideButton = document.getElementById(BUTTON_HIDE_ID);

    // Create a new hbox to hold the system buttons
    var controlBox = document.createElement("hbox");
    controlBox.id = HBOX_CONTROLS_ID;
    controlBox.align = "center";
    controlBox.setAttribute("class", CLASS_HBOX_OSX);

    // Add buttons and spacers to controlBox, removing them from the original
    // hbox. Also yank tooltips, as OS X assumes you know what the buttons do.

    // The hide button is to be used rather than the close button on the video
    // window.
    if (hideButton) {
      closeButton = hideButton;
      closeButton.removeAttribute("tooltiptext");
      closeButton.id = BUTTON_CLOSE_ID;
      topBar.removeChild(hideButton);
    }
    else if (closeButton) {
      topBar.removeChild(closeButton);
      closeButton.removeAttribute("tooltiptext");
    }
    else
      closeButton = createNewSysButton("button", BUTTON_CLOSE_ID);
    controlBox.appendChild(closeButton);

    controlBox.appendChild(createNewSpacer());

    if (minButton) {
      topBar.removeChild(minButton);
      minButton.removeAttribute("tooltiptext");
    }
    else
      minButton = createNewSysButton("button", BUTTON_MIN_ID);
    controlBox.appendChild(minButton);
    
    controlBox.appendChild(createNewSpacer());
    
    if (maxButton) {
      topBar.removeChild(maxButton);
      maxButton.removeAttribute("tooltiptext");
    }
    else
      maxButton = createNewSysButton("checkbox", BUTTON_MAX_ID);
    controlBox.appendChild(maxButton);
    
    // Make a stack so that the title stays centered regardless of the buttons
    var topStack = document.createElement("stack");
    topStack.flex = 1;
    
    // Move all the other stuff out of topBar and into the stack. This seems
    // silly, but we have to save the menu bar...
    var oldContentsBox = topBar.cloneNode(true);
    topStack.appendChild(oldContentsBox);
    
    // Now clear the contents of topBar
    var childList = topBar.childNodes;
    var childCount = childList.length;
    for (var index = 0; index < childCount; index++) {
      var child = childList.item(0);
      topBar.removeChild(child);
    }
    
    // And replace with our new stack
    topBar.appendChild(topStack);
    
    // Make the button hbox
    var buttonBox = document.createElement("hbox");
    buttonBox.flex = 1;
    
    // Make the text hbox
    var titleBox = document.createElement("hbox");
    titleBox.id = HBOX_TITLE_ID;
    titleBox.flex = 1;
    titleBox.align = "center";
    
    // Add the hboxes to the stack
    topStack.appendChild(titleBox);
    topStack.appendChild(buttonBox);

    // Insert the new control box
    buttonBox.appendChild(controlBox);
    
    // Create spacers to center the titlebar text
    var leftSpacer = document.createElement("spacer");
    var rightSpacer = document.createElement("spacer");
    leftSpacer.flex = rightSpacer.flex = 1;
    
    // Construct the new titlebar
    titleBox.appendChild(leftSpacer);
    titleBox.appendChild(topLabel);
    titleBox.appendChild(rightSpacer);

    // Add some space between the control box and the mini button
    var bigSpacer = document.createElement("spacer");
    bigSpacer.flex = 1;
    buttonBox.appendChild(bigSpacer);

    // Add a miniplayer button?
    if (miniButton) {
      miniButton.removeAttribute("tooltiptext");

      // Create a new hbox for the mini button
      var miniBox = document.createElement("hbox");
      miniBox.id = HBOX_MINI_ID;
      miniBox.align = "center";
      miniBox.setAttribute("class", CLASS_HBOX_OSX);

      // Add the miniButton
      miniBox.appendChild(miniButton);
    
      // Add to the topBar with a spacer
      buttonBox.appendChild(createNewSpacer());
      buttonBox.appendChild(miniBox);
    }
    
    // Now for the resizers...
    var resizersTopBox = document.getElementById(HBOX_RESIZERS_TOP_ID);
    var resizersLeftBox = document.getElementById(HBOX_RESIZERS_LEFT_ID);
    var resizersRightBox = document.getElementById(HBOX_RESIZERS_RIGHT_ID);
    var resizersBottomBox = document.getElementById(HBOX_RESIZERS_BOTTOM_ID);
    
    // Kill all the resizers in the top and left boxes and add back some space
    // so that the border shows up properly.
    if (resizersTopBox) {
      removeAllChildrenByTag(resizersTopBox, "resizer");
      var spacer = createNewSpacer();
      spacer.height = RESIZER_SPACE_SIZE;
      spacer.flex = 1;
      resizersTopBox.insertBefore(spacer, resizersTopBox.firstChild);
    }
    if (resizersLeftBox) {
      removeAllChildrenByTag(resizersLeftBox, "resizer");
      var spacer = createNewSpacer(RESIZER_SPACE_SIZE);
      spacer.flex = 1;
      resizersLeftBox.insertBefore(spacer, resizersLeftBox.firstChild);
    }
    
    // Now kill all other resizers except the two in the bottom right corner
    var boxList = [];
    if (resizersRightBox)
      boxList.push(resizersRightBox);
    if (resizersBottomBox)
      boxList.push(resizersBottomBox);
    var boxCount = boxList.length;
    
    // Loop over all boxes and all child resizer elements
    for (var boxIndex = 0; boxIndex < boxCount; boxIndex++) {
      var box = boxList[boxIndex];
      var resizersList = box.getElementsByTagName("resizer");
      var resizersCount = resizersList.length;
      for (var resizersIndex = 0;
          resizersIndex < resizersCount;
          resizersIndex++) {
        var resizer = resizersList.item(0);
        if (resizer.dir != "bottomright")
          box.removeChild(resizer);
      }
      // Add a spacer to make sure the resizer stays in the correct spot
      var newSpacer = createNewSpacer();
      newSpacer.flex = 1;
      box.insertBefore(newSpacer, box.firstChild);
    }
  }
  
  if (accessible) {

    if (!SBDataGetBoolValue("accessibility.enabled")) {
      // switching accessibility to on, some objects in the app will dynamically change their internal state to reflect this
      SBDataSetBoolValue("accessibility.enabled", true); 
      // change startup flags for video window
      try {
        prefs.setCharPref("toolkit.defaultChromeFeatures", "chrome,modal=no,toolbar=no,popup=no,titlebar=yes,dialog=no");  
      } catch (err) {}
    }

    // remove the hidechrome flag on the window
    document.documentElement.setAttribute("hidechrome", "false");
    
    // disable our custom resizers
    hideRealResizers();
    showFakeResizers();
    
    // make some modifications to the dom
    hideElement(aLabelId);
    hideElement(BUTTON_APP_ID);
    hideElement(BUTTON_MINI_ID);
    hideElement(BUTTON_MIN_ID);
    hideElement(BUTTON_MAX_ID);
    hideElement(BUTTON_CLOSE_ID);
    moveElement(MENU_ID, null);
    hideElement(LABEL_APPTITLE_ID);
  } else {
    if (SBDataGetBoolValue("accessibility.enabled")) {
      // switching accessibility to off
      SBDataSetBoolValue("accessibility.enabled", false); 
      // change startup flags for video window
      try {
        prefs.setCharPref("toolkit.defaultChromeFeatures", "chrome,modal=no,toolbar=no,popup=no,titlebar=no,dialog=no");
      } catch (err) {}
    }
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

function getCurrentPlaylist(disallowwebplaylist) {
  var pl = document.__CURRENTPLAYLIST__;
  if (!pl && !disallowwebplaylist) pl = document.__CURRENTWEBPLAYLIST__;
  if (!pl) return null;
  if ( pl.wrappedJSObject )
    pl = pl.wrappedJSObject;
  return pl;
}

