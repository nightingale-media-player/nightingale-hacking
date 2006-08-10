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

// Figure out what platform we're on.
var user_agent = navigator.userAgent;
var PLATFORM_WIN32 = user_agent.indexOf("Windows") != -1;
var PLATFORM_MACOSX = user_agent.indexOf("Mac OS X") != -1;
var PLATFORM_LINUX = user_agent.indexOf("Linux") != -1;

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

// The background image allows us to move the window around the screen
function onBkgDown( theEvent ) 
{
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

// Help
function onHelp()
{
  alert( "Aieeeeee, ayudame!" );
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
function onExit()
{
  try
  {
    onWindowSaveSizeAndPosition();
  }
  catch ( err )
  {
    // If you don't have data functions, just close.
  }
  document.defaultView.close();
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

// No forseen need to load _just_ size without position
function onWindowLoadSize()
{
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

var songbird_restartNow;

function restartApp()
{
  var as = Components.classes["@mozilla.org/toolkit/app-startup;1"].getService(Components.interfaces.nsIAppStartup);
  if (as)
  {
    // do NOT replace '_' with '.', or it will be handled as a metrics data: it would be posted to the metrics aggregator, then reset to 0 automatically
    SBDataSetBoolValue("metrics_ignorenextstartup", true);
    const V_RESTART = 16;
    const V_ATTEMPT = 2;
    as.quit(V_RESTART);
    as.quit(V_ATTEMPT);
  }
  onExit();
}

function quitApp()
{
  //thePlayerRepeater.doStop();
  onExit();

  var as = Components.classes["@mozilla.org/toolkit/app-startup;1"].getService(Components.interfaces.nsIAppStartup);
  if (as)
  {
    // do NOT replace '_' with '.', or it will be handled as a metrics data: it would be posted to the metrics aggregator, then reset to 0 automatically
    SBDataSetBoolValue("metrics_ignorenextstartup", false);
    const V_ATTEMPT = 2;
    as.quit(V_ATTEMPT);
  }
}

var songbird_playURL;
function SBUrlChanged( value )
{
  if (!coreInitialCloakDone) return;
  try
  {
    var windowCloak = Components.classes["@songbirdnest.com/Songbird/WindowCloak;1"];
    if (windowCloak) {
      var service = windowCloak.getService(Components.interfaces.sbIWindowCloak);
      if (service) {
        // value _should_ be set correctly now.
        if (value == null)
          value = SBDataGetStringValue("faceplate.play.url");
        if ( gPPS.isVideoURL( value ) ) {
          service.uncloak( document ); 
        }
        else {
          service.cloak( document ); 
        }
      }
    }
  }
  catch(e)
  {
    dump(e);
  }
}

function SBMetricsAppShutdown() 
{
  var startstamp = SBDataGetIntValue("startup_timestamp");
  var timenow = new Date();
  var diff = (timenow.getTime() - startstamp)/60000;
  metrics_add("player", "timerun", null, diff);
}

function SBInterfaceDeinitialize() 
{
  // Unbind restartapp remote
  songbird_restartNow.unbind();
}

function SBAppDeinitialize()
{
  // Make sure we stop before shutdown or our timer kills us.
  var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                      .getService(Components.interfaces.sbIPlaylistPlayback);
  try {
    gPPS.stop(); // else we crash?
  } catch (e) {}

  // Unattach the player repeater. (please deprecate me, soon!)
  //thePlayerRepeater.unbind();
  // Unbind the playback url viewer. (used by the code that uncloaks the video window)
  songbird_playURL.unbind();
  // Remember where the video window is.
  resetGlobalHotkeys();
  onWindowSaveSizeAndPosition();
  SBMetricsAppShutdown();
}

function SBMetricsAppStart() 
{
  // do NOT replace '_' with '.', or it will be handled as a metrics data: it would be posted to the metrics aggregator, then reset to 0 automatically
  if (!SBDataGetBoolValue("metrics_ignorenextstartup"))
  {
    metrics_inc("player", "appstart", null);
  }
  // do NOT replace '_' with '.', or it will be handled as a metrics data: it would be posted to the metrics aggregator, then reset to 0 automatically
  SBDataSetBoolValue("metrics_ignorenextstartup", false);
  var timestamp = new Date();
  SBDataSetIntValue("startup_timestamp", timestamp.getTime());
}

function SBInterfaceInitialize() 
{
  var sb_restart_app = {
    observe: function ( aSubject, aTopic, aData ) { restartApp(); }
  };
  songbird_restartNow = SB_NewDataRemote( "restart.restartnow", null );
  songbird_restartNow.bindObserver( sb_restart_app, true );
}

function SBAppInitialize()
{
  try
  {
    SBMetricsAppStart();
    setVideoMinMaxCallback();
    onWindowLoadSize();
    createLibraryRef();
    initGlobalHotkeys();

    // observer for DataRemote
    var sb_url_changed = {
      observe: function ( aSubject, aTopic, aData ) { SBUrlChanged(aData); }
    };

    // Create and bind DataRemote
    songbird_playURL = SB_NewDataRemote( "faceplate.play.url", null );
    songbird_playURL.bindObserver( sb_url_changed, true );

    /*
    */
    var theWMPInstance = document.getElementById( "core_wm" );
    var theWMPBox = document.getElementById( "box_wm" );

    /*
    */
    var theQTInstance = document.getElementById( "core_qt_document" );
    var theQTBox = document.getElementById( "box_qt" );

    /*
    */
    var theVLCInstance = document.getElementById( "core_vlc" );
    var theVLCBox = document.getElementById( "box_vlc" );

    /*
    */
    var theFLInstance = document.getElementById( "core_flash_frame" );
    var theFLBox = document.getElementById( "box_flash" );

    /*
    */
    var theTotemInstance = document.getElementById( "core_totem_frame" );
    var theTotemBox = document.getElementById( "box_totem" );

    //
    // Depending upon the platform, initialize one core
    // and hide all of the rest of them.
    //

    //Windows, prefer VLC.
    if ( PLATFORM_WIN32 ) {
      // Initialize with VLC
      CoreVLCDocumentInit( "core_vlc" );
      //InitPlaybackCoreFlash( "core_flash_frame" );
      // Hide Quicktime
      if (theQTBox) theQTBox.hidden = true;
      // Hide Flash
      if (theFLBox) theFLBox.hidden = true;
      // Hide Totem
      if (theTotemBox) theTotemBox.hidden = true;
    }

    //MacOSX, prefer QT.
    if( PLATFORM_MACOSX ) {
      // Initialize with Quicktime
      CoreQTDocumentInit( "core_qt_document" );
      // Hide VLC
      if (theVLCBox) theVLCBox.hidden = true;
      // Hide Flash
      if (theFLBox) theFLBox.hidden = true;
      // Hide Totem
      if (theTotemBox) theTotemBox.hidden = true;
    }
    
    //Linux, prefer totem-gstreamer
    if( PLATFORM_LINUX ) {
      //CoreVLCDocumentInit( "core_vlc_document" );
      //InitPlaybackCoreMPlayer( "core_mp_frame" );
      //InitPlaybackCoreFlash( "core_flash_frame" );
      CoreTotemDocumentInit( "core_totem_frame" );
      // Hide VLC
      if (theVLCBox) theVLCBox.hidden = true;
      // Hide Quicktime
      if (theQTBox) theQTBox.hidden = true;
      // Hide Flash
      if (theFLBox) theFLBox.hidden = true;
    }
    
    // Reset this on application startup. 
    SBDataSetIntValue("backscan.paused", 0);
    
    // Go make sure we really have a Songbird database
    SBInitializeNamedDatabase( "songbird" );
/*
*/    
    try
    {
      DPUpdaterInit(1);
    }
    catch(err)
    {
      alert("DPUpdaterInit(1) - " + err);
    }
    
    try
    {
      WFInit();
    }
    catch(err)
    {
      alert("WFInit() - " + err);
    }

    var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);

    try { prefs.getCharPref("extensions.getMoreExtensionsURL"); } 
    catch (e) { prefs.setCharPref("extensions.getMoreExtensionsURL", "http://songbirdnest.com/extensions.rdf"); }
    try { prefs.getCharPref("extensions.getMoreThemesURL"); } 
    catch (e) { prefs.setCharPref("extensions.getMoreThemesURL", "http://songbirdnest.com/themes.rdf"); }

    prefs.setCharPref("xpinstall.dialog.confirm", "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul");
    prefs.setCharPref("xpinstall.dialog.progress.chrome", "chrome://mozapps/content/extensions/extensions.xul?type=extensions");
    prefs.setCharPref("xpinstall.dialog.progress.skin", "chrome://mozapps/content/extensions/extensions.xul?type=themes");
    prefs.setCharPref("xpinstall.dialog.progress.type.chrome", "Extension:Manager-extensions");
    prefs.setCharPref("xpinstall.dialog.progress.type.skin", "Extension:Manager-themes");
    
    prefs.setBoolPref("browser.preferences.animateFadeIn", false);
    prefs.setBoolPref("browser.preferences.instantApply", true);
    
    // If we are a top level window, hide us.
    if ( window.parent == window )
    {
      var hide = true;
      if ( document.__dont_hide_rmpdemo_window )
      {
        hide = false;
      }
      if ( hide )
      {
        setTimeout(HideCoreWindow, 0);
      }
    }
  }
  catch( err )
  {
    alert( "SBAppInitialize\n" + err );
  }
}

function SBInitializeNamedDatabase( db_name )
{
  // This creates the a working songbird database.
  // If it already exists, it just errors silently and you don't care.
  try
  {
    // Make an async database query object for the main songbird database
    var aDBQuery = new sbIDatabaseQuery();
    aDBQuery.setAsyncQuery(true);
    aDBQuery.setDatabaseGUID(db_name);
    
    // Get the library interface, and create the library through the query
    var aMediaLibrary = new sbIMediaLibrary();    
    aMediaLibrary.setQueryObject(aDBQuery);
    aMediaLibrary.createDefaultLibrary();
    
    // Ger the playlist manager, and create the internal playlisting infrastructure.
    var aPlaylistManager = new sbIPlaylistManager();
    aPlaylistManager.createDefaultPlaylistManager(aDBQuery);
  }
  catch(err)
  {
    alert("SBInitializeNamedDatabase\n\n" + err);
  }
}

var coreInitialCloakDone = 0;
function HideCoreWindow() 
{
  try {
    var windowCloak = Components.classes["@songbirdnest.com/Songbird/WindowCloak;1"];
    if (windowCloak) {
      var service = windowCloak.createInstance(Components.interfaces.sbIWindowCloak);
      if (service)
        service.cloak( document ); 
    }
  }
  catch (err) {
    //No component
    dump("Error. Songbird.js:HideCoreWindow \n" + err + "\n");
  }
  coreInitialCloakDone = 1;
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

function SBOpenModalDialog( url, param1, param2, param3 )
{
  PushBackscanPause();
  param2 += ",resizable=no,titlebar=no"; // bonus stuff to shut the mac up.
  var retval = window.openDialog( url, param1, param2, param3 );
  PopBackscanPause();
  return retval;
}

var SBVideoMinMaxCB = 
{
  // Shrink until the box doesn't match the window, then stop.
  GetMinWidth: function()
  {
    // What we'd like it to be
    var retval = 720;
    // However, if in resizing the window size is different from the document's box object
    if (window.innerWidth != document.getElementById('window_parent').boxObject.width)
    { 
      // That means we found the document's min width.  Because you can't query it directly.
      retval = document.getElementById('window_parent').boxObject.width - 1;
    }
    return retval;
  },

  GetMinHeight: function()
  {
    // What we'd like it to be
    var retval = 450;
    // However, if in resizing the window size is different from the document's box object
    if (window.innerHeight != document.getElementById('window_parent').boxObject.height)
    { 
      // That means we found the document's min width.  Because you can't query it directly.
      retval = document.getElementById('window_parent').boxObject.height - 1;
    }
    return retval;
  },

  GetMaxWidth: function()
  {
    return -1;
  },

  GetMaxHeight: function()
  {
    return -1;
  },
  
  OnWindowClose: function()
  {
    setTimeout(quitApp, 0);
  },

  QueryInterface : function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbIWindowMinMaxCallback) &&
        !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    
    return this;
  }
}

function setVideoMinMaxCallback()
{
  try {
    var windowMinMax = Components.classes["@songbirdnest.com/Songbird/WindowMinMax;1"];
    if (windowMinMax) {
      var service = windowMinMax.getService(Components.interfaces.sbIWindowMinMax);
      if (service)
        service.setCallback(document, SBVideoMinMaxCB);
    }
  }
  catch (err) {
    // No component
    dump("Error. No WindowMinMax component available." + err + "\n");
  }
}

function createLibraryRef() {
 /* // this is so we can playRef the library even if it has never been shown
  var source = new sbIPlaylistsource();
  source.feedPlaylist( "NC:songbird_library", "songbird", "library");
  source.executeFeed( "NC:songbird_library" );
  // Synchronous call!  Woo hoo!
  while( source.isQueryExecuting( "NC:songbird_library" ) )
    ;
  // After the call is done, force GetTargets
  source.forceGetTargets( "NC:songbird_library" );
*/
}

