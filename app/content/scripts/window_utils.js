/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright? 2006 POTI, Inc.
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
 
 
 
/*
// If we are running under windows, there's a bug with background-color: transparent;
if (PLATFORM_WIN32)
{
  // During script initialization, set the background color to black.
  // Otherwise, all iframes are blank.  Dumb bug.
  var win = document.getElementById("video_window");
  if (win)
    win.setAttribute("style","background-color: #000 !important;");
  // At least this fixes it.
}
*/
 
//
// XUL Event Methods
//

//Necessary when WindowDragger is not available on the current platform.
var trackerBkg = false;
var offsetScrX = 0;
var offsetScrY = 0;

var windowDragCallback = {
  onWindowDragComplete: function () {
    onWindowDragComplete();
  },

  QueryInterface : function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbIWindowDraggerCallback) &&
        !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    
    return this;
  }
  
};

// The background image allows us to move the window around the screen
function onBkgDown( theEvent, popup ) 
{
  // Don't allow dragging on nodes that want their own click handling.
  // This is kinda dumb.  :(
  switch (theEvent.target.nodeName.toLowerCase())
  {
    // Songbird Custom Elements
    case "player_seekbar":
    case "player_volume":
    case "player_repeat":
    case "player_shuffle":
    case "player_playpause":
    case "player_back":
    case "player_forward":
    case "player_mute":
    case "player_numplaylistitems":
    case "player_scaning":
    case "dbedit_textbox":
    case "dbedit_menulist":
    case "exttrackeditor":
    case "servicetree":
    case "playlist":
    case "search":
    case "smartsplitter":
    case "sbextensions":
    case "smart_conditions":
    case "watch_folders":
    case "clickholdbutton":
    // XUL Elements
    case "splitter":
    case "grippy":
    case "button":
    case "toolbarbutton":
    case "scrollbar":
    case "slider":
    case "thumb":
    case "checkbox":
    case "resizer":
    case "textbox":
    case "tree":
    case "listbox":
    case "listitem":
    case "menu":
    case "menulist":
    case "menuitem":
    case "menupopup":
    case "menuseparator":
    // Heehee
    case "parsererror":
    // HTML Elements
    case "img":
    case "input":
    case "select":
    case "option":
    case "object":
    case "embed":
    case "body":
    case "html":
    case "div":
    case "a":
    case "ul":
    case "ol":
    case "dl":
    case "dt":
    case "dd":
    case "li":
    case "#text":
    case "textarea":
      return;
  }
//  alert(theEvent.target.nodeName);
  try
  {
    var windowDragger = Components.classes["@songbirdnest.com/Songbird/WindowDragger;1"];
    if (windowDragger) {
      var service = windowDragger.getService(Components.interfaces.sbIWindowDragger);
      if (service) {
        var dockDistance = (window.dockDistance ? window.dockDistance : 0);
        service.beginWindowDrag(dockDistance, windowDragCallback); // automatically ends
      }
    }
    else {
      trackerBkg = true;
      offsetScrX = document.documentElement.boxObject.screenX - theEvent.screenX;
      offsetScrY = document.documentElement.boxObject.screenY - theEvent.screenY;
      // ScreenY is reported incorrectly on osx for non-popup windows without title bars.
      if ( ( popup != true ) && (navigator.userAgent.indexOf("Mac OS X") != -1) ) {
        // TODO: This will be incorrect in the jumptofile dialog, as it is loaded as a popup.
        // How do I know from this scope whether the window is loaded as popup?
        offsetScrY -= 22; 
      }
      document.addEventListener( "mousemove", onBkgMove, true );
    }
  }
  catch(err) {
    dump("Error. Songbird.js::onBkDown() \n" + err + "\n");
  }
}

function onBkgMove( theEvent ) 
{
  if ( trackerBkg )
  {
    document.defaultView.moveTo( offsetScrX + theEvent.screenX, offsetScrY + theEvent.screenY );
  }
}

function onBkgUp( ) 
{
  if ( trackerBkg )
  {
    trackerBkg = false;
    onWindowDragComplete();
    document.removeEventListener( "mousemove", onBkgMove, true );
  }
}

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
var maximized = false;
function onMaximize()
{
  if ( maximized )
  {
    document.defaultView.restore();
    maximized = false;
  }
  else
  {
    document.defaultView.maximize();
    maximized = true;
  }
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
  // Try to activate the window.
  try {
    window.focus();
  } catch (e) {}
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
    // test if the window is resizable. if it is, and rootW/rootH are below some limit, skip these steps so that we reload the default w/h from xul and css
    var resetsize = false;
    var resizers = document.getElementsByTagName("resizer");
    var resizable = resizers.length > 0;
    var minwidth = getStyle(document.documentElement, "min-width");
    var minheight = getStyle(document.documentElement, "min-height");
    if (minwidth) minwidth = parseInt(minwidth);
    if (minheight) minheight = parseInt(minheight);
/*
    dump("window = " + document.documentElement.getAttribute("id") + "\n");
    dump("minwidth = " + minwidth + "\n");
    dump("rootW = " + rootW + "\n");
    dump("minheight = " + minheight + "\n");
    dump("rootH = " + rootH + "\n");
    dump("resizable = " + resizable + "\n");
*/
    if (resizable &&
        ((minwidth && rootW < minwidth) || 
         (minheight && rootH < minheight))) resetsize = true;
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
  // Activate whatever window is attempting to reload its info.
  delayedActivate();
  
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
  param2 += ",resizable=no,titlebar=no"; // bonus stuff to shut the mac up.
  var retval = window.openDialog( url, param1, param2, param3 );
  PopBackscanPause();
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
    window.open( mainwin, "", "chrome,modal=no,titlebar=no,resizable=no,toolbar=no,popup=no,titlebar=no" );
    onExit();
  }
}

/* Temporary hack to get the system buttons in the right (er, left) place on a
 * mac. To be replaced with some sort of XBL/preprocessor magic after 0.2.
 *
 * aBoxId  - The id of the <hbox> element used for the titlebar
 * aLabelId - The id of the <label> element used to display the title
 */
function fixOSXWindow(aBoxId, aLabelId)
{
  var platform;
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

  // Currently we only care about fixing OS X.
  if (platform != "Darwin")
    return;

  if (!(aBoxId && aLabelId))
    throw Components.results.NS_ERROR_INVALID_ARG;

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
