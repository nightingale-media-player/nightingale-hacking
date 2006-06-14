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
    var windowDragger = Components.classes["@songbird.org/Songbird/WindowDragger;1"].getService(Components.interfaces.sbIWindowDragger);
    windowDragger.BeginWindowDrag(0); // automatically ends
  }
  catch(e)
  {
    trackerBkg = true;
    offsetScrX = document.defaultView.screenX - theEvent.screenX;
    offsetScrY = document.defaultView.screenY - theEvent.screenY;
    document.addEventListener( "mousemove", onBkgMove, true );
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

// old version, just in case
/*var trackerBkg = false;
var offsetScrX = 0;
var offsetScrY = 0;
function onBkgDown( theEvent ) 
{
  trackerBkg = true;
  offsetScrX = document.defaultView.screenX - theEvent.screenX;
  offsetScrY = document.defaultView.screenY - theEvent.screenY;
  document.addEventListener( "mousemove", onBkgMove, true );
}
function onBkgMove( theEvent ) 
{
  if ( trackerBkg )
  {
//    window.moveTo( offsetScrX + theEvent.screenX, offsetScrY + theEvent.screenY );
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
}*/

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
    onWindowSaveSize();
  }
  catch ( err )
  {
    // If you don't have data functions, just close.
  }
  document.defaultView.close();
}

function onWindowSaveSize()
{
  var root = "window." + document.documentElement.id;
  SBDataSetValue( root + ".x", document.documentElement.boxObject.screenX );
  SBDataSetValue( root + ".y", document.documentElement.boxObject.screenY );
  SBDataSetValue( root + ".w", document.documentElement.boxObject.width );
  SBDataSetValue( root + ".h", document.documentElement.boxObject.height );
}

function onWindowSavePosition()
{
  var root = "window." + document.documentElement.id;
  SBDataSetValue( root + ".x", document.documentElement.boxObject.screenX );
  SBDataSetValue( root + ".y", document.documentElement.boxObject.screenY );
}

function onWindowLoadSize()
{
  var root = "window." + document.documentElement.id;
  if (SBDataGetValue( root + ".x" ) == "" && SBDataGetValue( root + ".w" ) == "") { return; }
  if ( SBDataGetIntValue( root + ".w" ) && SBDataGetIntValue( root + ".h" ) )
  {
    // https://bugzilla.mozilla.org/show_bug.cgi?id=322788
    // YAY YAY YAY the windowregion hack actualy fixes this :D
    window.resizeTo( SBDataGetIntValue( root + ".w" ), SBDataGetIntValue( root + ".h" ) );
    // for some reason, the resulting size isn't what we're asking (window currently has a border?) so determine what the difference is and add it to the resize
    var diffw = SBDataGetIntValue( root + ".w" ) - document.documentElement.boxObject.width;
    var diffh = SBDataGetIntValue( root + ".h" ) - document.documentElement.boxObject.height;
    window.resizeTo( SBDataGetIntValue( root + ".w" ) + diffw, SBDataGetIntValue( root + ".h" ) + diffh);
  }
  window.moveTo( SBDataGetIntValue( root + ".x" ), SBDataGetIntValue( root + ".y" ) );
  // do the (more or less) same adjustment for x,y as we did for w,h
  var diffx = SBDataGetIntValue( root + ".x" ) - document.documentElement.boxObject.screenX;
  var diffy = SBDataGetIntValue( root + ".y" ) - document.documentElement.boxObject.screenY;
  window.moveTo( SBDataGetIntValue( root + ".x" ) - diffx, SBDataGetIntValue( root + ".y" ) - diffy );
}

function onWindowLoadPosition()
{
  var root = "window." + document.documentElement.id;
  if (SBDataGetValue( root + ".x" ) == "" && SBDataGetValue( root + ".w" ) == "") { return; }
  window.moveTo( SBDataGetIntValue( root + ".x" ), SBDataGetIntValue( root + ".y" ) );
  // do the (more or less) same adjustment for x,y as we did for w,h
  var diffx = SBDataGetIntValue( root + ".x" ) - document.documentElement.boxObject.screenX;
  var diffy = SBDataGetIntValue( root + ".y" ) - document.documentElement.boxObject.screenY;
  window.moveTo( SBDataGetIntValue( root + ".x" ) - diffx, SBDataGetIntValue( root + ".y" ) - diffy );
}

function ConvertUrlToDisplayName( url )
{
  url = decodeURI( url );
  // Set the title display  
  var the_value = "";
  if ( url.lastIndexOf('/') != -1 )
  {
    the_value = url.substring( url.lastIndexOf('/') + 1, url.length );
  }
  else if ( url.lastIndexOf('\\') != -1 )
  {
    the_value = url.substring( url.lastIndexOf('\\') + 1, url.length );
  }
  else
  {
    the_value = url;
  }
  if ( ! the_value.length )
  {
    the_value = url;
  }
  return the_value;
}

document.restartOnPlaybackEnd = false;
function restartApp()
{
  //thePlayerRepeater.doStop();
  onExit();

  var as = Components.classes["@mozilla.org/toolkit/app-startup;1"].getService(Components.interfaces.nsIAppStartup);
  if (as)
  {
    // do NOT replace '_' with '.', or it will be handled as a metrics data: it would be posted to the metrics aggregator, then reset to 0 automatically
    SBDataSetValue("metrics_ignorenextstartup", 1);
    const V_RESTART = 16;
    const V_ATTEMPT = 2;
    as.quit(V_RESTART);
    as.quit(V_ATTEMPT);
  }
}

function quitApp()
{
  //thePlayerRepeater.doStop();
  onExit();

  var as = Components.classes["@mozilla.org/toolkit/app-startup;1"].getService(Components.interfaces.nsIAppStartup);
  if (as)
  {
    // do NOT replace '_' with '.', or it will be handled as a metrics data: it would be posted to the metrics aggregator, then reset to 0 automatically
    SBDataSetValue("metrics_ignorenextstartup", 0);
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
    var windowCloak = Components.classes["@songbird.org/Songbird/WindowCloak;1"].getService(Components.interfaces.sbIWindowCloak);
    if ( IsVideoUrl( value ) )
    {
      windowCloak.Uncloak( document ); 
    }
    else
    {
      windowCloak.Cloak( document ); 
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

function SBAppDeinitialize()
{
  // Make sure we stop before shutdown or our timer kills us.
  var gPPS = Components.classes["@songbird.org/Songbird/PlaylistPlayback;1"]
                      .getService(Components.interfaces.sbIPlaylistPlayback);
  try {
    gPPS.stop(); // else we crash?
  } catch (e) {}

  // Unattach the player repeater. (please deprecate me, soon!)
  //thePlayerRepeater.unbind();
  // Unbind the playback url viewer. (used by the code that uncloaks the video window)
  songbird_playURL.unbind();
  // Remember where the video window is.
  onWindowSaveSize();
  SBMetricsAppShutdown();
}

function SBMetricsAppStart() 
{
  // do NOT replace '_' with '.', or it will be handled as a metrics data: it would be posted to the metrics aggregator, then reset to 0 automatically
  if (SBDataGetIntValue("metrics_ignorenextstartup") == 0)
  {
    metrics_inc("player", "appstart", null);
  }
  // do NOT replace '_' with '.', or it will be handled as a metrics data: it would be posted to the metrics aggregator, then reset to 0 automatically
  SBDataSetValue("metrics_ignorenextstartup", 0);
  var timestamp = new Date();
  SBDataSetValue("startup_timestamp", timestamp.getTime());
}

function SBAppInitialize()
{
  try
  {
    SBMetricsAppStart();
    setVideoMinMaxCallback();
    onWindowLoadSize();
    createLibraryRef();
    songbird_playURL = new sbIDataRemote("faceplate.play.url");
    songbird_playURL.bindCallbackFunction( SBUrlChanged, true )

    /*
    */
    //var theWMPInstance = document.getElementById( "core_wm" );

    /*
    */
    //var theQTInstance = document.getElementById( "core_qt" );

    /*
    */
    var theVLCInstance = document.getElementById( "core_vlc" );

    // Bind the core instances to the core objects (I really should make these use createElementNS, if we can get it to work)      
    //theWMPCore.bindInstance( theWMPInstance );
    //theQTCore.bindInstance( theQTInstance );
    //theVLCCore.bindInstance( theVLCInstance );

    // Tell the repeater we will be playing only one specific type of media
    //thePlayerRepeater.setPlaybackCore( theWMPCore );
    //thePlayerRepeater.setPlaybackCore( theQTCore );
    //thePlayerRepeater.setPlaybackCore( theVLCCore );

    // Let the sbIPlaylistPlayback interface play in the game, too, maaaan.
    CoreVLCDocumentInit( "core_vlc" );
    //CoreWMPDocumentInit( "core_wm" );
    //CoreQTDocumentInit( "core_qt_document" );
    //setTimeout("CoreWMPDocumentInit( 'core_wm' );", 0);
    
    // Reset this on application startup. 
    SBDataSetValue("backscan.paused", false);
    
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
    aDBQuery.SetAsyncQuery(true);
    aDBQuery.SetDatabaseGUID(db_name);
    
    // Get the library interface, and create the library through the query
    var aMediaLibrary = new sbIMediaLibrary();    
    aMediaLibrary.SetQueryObject(aDBQuery);
    aMediaLibrary.CreateDefaultLibrary();
    
    // Ger the playlist manager, and create the internal playlisting infrastructure.
    var aPlaylistManager = new sbIPlaylistManager();
    aPlaylistManager.CreateDefaultPlaylistManager(aDBQuery);
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
    var windowCloak = Components.classes["@songbird.org/Songbird/WindowCloak;1"].getService(Components.interfaces.sbIWindowCloak);
    windowCloak.Cloak( document ); 
  }
  catch (e) {
    //No component
  }
  coreInitialCloakDone = 1;
}

function PushBackscanPause()
{
  try
  {
    SBDataSetValue('backscan.paused', SBDataGetIntValue('backscan.paused') + 1 );
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
    var pause = SBDataGetIntValue('backscan.paused') - 1;
    if ( pause < 0 ) pause = 0;
    SBDataSetValue('backscan.paused', pause );
  }
  catch ( err )
  {
    alert( "PushBackscanPause - " + err );
  }
}

function SBOpenModalDialog( url, param1, param2, param3 )
{
  PushBackscanPause();
  var retval = window.openDialog( url, param1, param2, param3 );
  PopBackscanPause();
  return retval;
}

function IsVideoUrl( the_url )
{

  if ( ( the_url.indexOf ) && 
        (
          ( the_url.indexOf( ".wmv" ) == ( the_url.length - 4 ) ) ||
          
          // A better solution is needed, as asx files are not always video..
          // The following hack brought to you by Nivi:
          ( the_url.indexOf( ".asx" ) == ( the_url.length - 4 ) && the_url.indexOf( "allmusic.com" ) == -1 ) ||
          
          ( the_url.indexOf( ".asf" ) == ( the_url.length - 4 ) ) ||
          ( the_url.indexOf( ".avi" ) == ( the_url.length - 4 ) ) ||
          ( the_url.indexOf( ".mov" ) == ( the_url.length - 4 ) ) ||
          ( the_url.indexOf( ".mpg" ) == ( the_url.length - 4 ) ) ||
          ( the_url.indexOf( ".mp4" ) == ( the_url.length - 4 ) )
        )
      )
  {
    return true;
  }
  return false;
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
    var windowMinMax = Components.classes["@songbird.org/Songbird/WindowMinMax;1"].getService(Components.interfaces.sbIWindowMinMax);
    windowMinMax.SetCallback(document, SBVideoMinMaxCB);
  }
  catch (e) {
    // No component
  }
}

function createLibraryRef() {
 /* // this is so we can playRef the library even if it has never been shown
  var source = new sbIPlaylistsource();
  source.FeedPlaylist( "NC:songbird_library", "songbird", "library");
  source.FeedFilters( "NC:songbird_library" );
  // Synchronous call!  Woo hoo!
  while( source.IsQueryExecuting( "NC:songbird_library" ) )
    ;
  // After the call is done, force GetTargets
  source.ForceGetTargets( "NC:songbird_library" );
*/
}

// Figure out what platform we're on.
var user_agent = navigator.userAgent;
var PLATFORM_WIN32 = user_agent.indexOf("Windows") != -1;
var PLATFORM_MACOSX = user_agent.indexOf("OSX") != -1;
var PLATFORM_LINUX = user_agent.indexOf("Linux") != -1;

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

