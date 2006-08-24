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
// GNU General Public License Version 2 (the ?GPL?).
// 
// Software distributed under the License is distributed 
// on an ?AS IS? basis, WITHOUT WARRANTY OF ANY KIND, either 
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
 
 // Figure out what platform we're on.
var user_agent = navigator.userAgent;
var PLATFORM_WIN32 = user_agent.indexOf("Windows") != -1;
var PLATFORM_MACOSX = user_agent.indexOf("Mac OS X") != -1;
var PLATFORM_LINUX = user_agent.indexOf("Linux") != -1;
 

//
// XUL Event Methods
//

//Necessary when WindowDragger is not available on the current platform.
var trackerBkg = false;
var offsetScrX = 0;
var offsetScrY = 0;

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
      return;
  }
//  alert(theEvent.target.nodeName);
  try
  {
    var windowDragger = Components.classes["@songbirdnest.com/Songbird/WindowDragger;1"];
    if (windowDragger) {
      var service = windowDragger.getService(Components.interfaces.sbIWindowDragger);
      if (service)
        service.beginWindowDrag(0); // automatically ends
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
  try
  {
    var windowCloak = Components.classes["@songbirdnest.com/Songbird/WindowCloak;1"];
    if (windowCloak) {
      var service = windowCloak.getService(Components.interfaces.sbIWindowCloak);
      if (service) {
        service.cloak( document ); 
      }
    }
  }
  catch(e)
  {
    dump(e);
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
  dump("******** onWindowLoadSize: root:" + root +
                                  " root.w:" + SBDataGetIntValue(root+'.x') + 
                                  " root.h:" + SBDataGetIntValue(root+'.y') + 
                                  "\n");
*/
  SBDataSetIntValue( root + ".x", document.documentElement.boxObject.screenX );
  SBDataSetIntValue( root + ".y", document.documentElement.boxObject.screenY );
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
function onWindowLoadSize()
{
  delayedActivate();

  var root = "window." + document.documentElement.id;
/*
  dump("******** onWindowLoadSize: root:" + root +
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
    return;
  }

  // get the data once.
  var rootW = SBDataGetIntValue( root + ".w" );
  var rootH = SBDataGetIntValue( root + ".h" );

  if ( rootW && rootH )
  {
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
  onWindowLoadPosition();
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
    return;
  }

  // get the data once.
  var rootX = SBDataGetIntValue( root + ".x" );
  var rootY = SBDataGetIntValue( root + ".y" );

  window.moveTo( rootX, rootY );
  // do the (more or less) same adjustment for x,y as we did for w,h
  var diffX = rootX - document.documentElement.boxObject.screenX;
  var diffY = rootY - document.documentElement.boxObject.screenY;

  // This fix not needed for Linux - might need to add a MacOSX check.
  if (!PLATFORM_LINUX)
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


function quitApp()
{
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
  onExit();
}
