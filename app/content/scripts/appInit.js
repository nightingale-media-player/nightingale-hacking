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

//
// Module specific auto-init/deinit support
//
var appInit = {};
appInit.init_once = 0;
appInit.deinit_once = 0;
appInit.onLoad = function()
{
  if (appInit.init_once++) { dump("WARNING: appInit double init!!\n"); return; }
  // Application, initialize thyself.
  SBAppInitialize();
  SBRestarterInitialize();
  try
  {
    //Whatever migration is required between version, this function takes care of it.
    SBMigrateDatabase();
  }
  catch(e) { }
}
appInit.onUnload = function()
{
  if (appInit.deinit_once++) { dump("WARNING: appInit double deinit!!\n"); return; }
  window.removeEventListener("load", appInit.onLoad, false);
  window.removeEventListener("unload", appInit.onUnload, false);
  SBAppDeinitialize();
  SBRestarterDeinitialize();
  // TEMP: shutdown the metadata job manager
  theMetadataJobManager.stop();
}
appInit.onScriptInit = function()
{
  // TEMP: startup the metadata job manager
  theMetadataJobManager = Components.classes["@songbirdnest.com/Songbird/MetadataJobManager;1"]
                                .getService(Components.interfaces.sbIMetadataJobManager);


  // !!!
  // !!! THIS FUNCTION GETS CALLED AT THE BOTTOM OF THIS MODULE FILE !!!
  // !!!
  
  // If we aren't the oldest video window around then self destruct.
  // This can happen when
  //  - The user tries to load songbird.xul in the main browser
  //  - The user launches a second xulrunner while songbird is running 
  //    but not showing a window of type Songbird:Main
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                      .getService(Components.interfaces.nsIWindowMediator);
  var videoWindowList = wm.getEnumerator("Songbird:Core"); 
  if (!videoWindowList.hasMoreElements() 
      || videoWindowList.getNext() != window) 
  {
    dump("\n\nSBAppStartup: Aborting since there is already a Songbird:Core window\n\n");
    
    // Can't close before onLoad has occured.. otherwise things go a bit crazy
    SBAppInitialize = function() {
      dump("\n\SBAppInitialize: shutting down.\n\n");
      document.defaultView.close();
      return false;
    }
    
    SBRestarterInitialize = null;
    SBAppDeinitialize = null;
    SBRestarterDeinitialize = null;
    
    
    // If there is main window up, try to focus on that.
    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                      .getService(Components.interfaces.nsIWindowMediator);
    var mainWin = wm.getMostRecentWindow("Songbird:Main");
    if (mainWin)
      mainWin.focus();
    
    return;
  }
  
  // Show EULA, then the first run (if accepted) or exit (if rejected)
  if ( doEULA( doFirstRun, doShutdown ) ) {
    doMainwinStart();
  }
}
window.addEventListener("load", appInit.onLoad, false);
window.addEventListener("unload", appInit.onUnload, false);


//
// Called on window-load of songbird.xul.
//
function SBAppInitialize()
{
  dump("SBAppInitialize\n");
  try
  {
    // Startup the Metrics
    SBMetricsAppStart();

    // Ensure the default library is created and available to gPPS
    SBCreateLibraryRef();
    
    // Startup the Hotkeys
    initGlobalHotkeys();

    // Reset this on application startup. 
    SBDataSetIntValue("backscan.paused", 0);
    
    // Go make sure we really have a Songbird database
    SBInitializeNamedDatabase( "songbird" );

    // Startup the Dynamic Playlist Updater
    try
    {
      DPUpdaterInit(1);
    }
    catch(err)
    {
      alert("DPUpdaterInit(1) - " + err);
    }

    // Startup the Watch Folders    
    try
    {
      WFInit();
    }
    catch(err)
    {
      alert("WFInit() - " + err);
    }
  }
  catch( err )
  {
    alert( "SBAppInitialize\n" + err );
  }
}

//
// Called on window-unload of songbird.xul.
//
function SBAppDeinitialize()
{
  // Get playlist playback and tell it to stop.
  try {
    var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                  .getService(Components.interfaces.sbIPlaylistPlayback);
    PPS.stop(); // else we crash?
  } catch (e) {}

  // Shutdown the Hotkeys
  resetGlobalHotkeys();
  
  // Shutdown the Metrics
  SBMetricsAppShutdown();
  
  // Shutdown the Watch Folders
  try {
    WFShutdown();
  } catch(err) {
    dump(err);
  }
}

//
// Shut down xulrunner
//
// - Passed as a param to doEULA
// - Called if user rejects EULA
function doShutdown() {
        
  var metrics = Components.classes["@songbirdnest.com/Songbird/Metrics;1"]
                    .createInstance(Components.interfaces.sbIMetrics);
  if (!metrics) 
    throw("doShutdown could not get the metrics component");
    
  // mark this session as clean, we did not crash
  metrics.setSessionFlag(false); 
  
  
  // get the startup service and tell it to shut us down
  var as = Components.classes["@mozilla.org/toolkit/app-startup;1"]
                     .getService(Components.interfaces.nsIAppStartup);
  if (!as) 
    throw("doShutdown could not get the app-startup component!");

  // do not count next startup in metrics, since we counted this one, 
  // but it was aborted.
  //
  // Note: do NOT replace '_' with '.', or it will be handled as metrics
  //    data: it would be posted to the metrics aggregator, then reset
  //    to 0 automatically
  SBDataSetBoolValue("metrics_ignorenextstartup", true);
  
  as.quit(Components.interfaces.nsIAppStartup.eAttemptQuit);
}

//
// Launch the main window.
//
// Note:  This used to be a closure in songbird.xul
//
function doMainwinStart() 
{
  dump("doMainwinStart\n");  

  try {
    var metrics =
      Components.classes["@songbirdnest.com/Songbird/Metrics;1"]
                .createInstance(Components.interfaces.sbIMetrics);

    if (metrics.getSessionFlag())
      metrics_inc("player", "crash");

    metrics.setSessionFlag(true);
    metrics.checkUploadMetrics();
  }
  catch (err) {
    SB_LOG("App Init - Metrics - ", "" + err);
  }

  // Make sure the web playlist is enabled.
  // This is to protect against cases where the app is shut down
  // while an extension has the web playlist disabled.
  SBDataSetBoolValue("webplaylist.enabled", true);

  var feathersManager = Components.classes['@songbirdnest.com/songbird/feathersmanager;1']
                                   .getService(Components.interfaces.sbIFeathersManager);
  var mainwinURL = feathersManager.currentLayoutURL;
  var showChrome = feathersManager.isChromeEnabled(mainwinURL, feathersManager.currentSkinName);

  // TEMP fix for the Mac to enable the titlebar on the main window.
  var sysInfo = Components.classes["@mozilla.org/system-info;1"]
                          .getService(Components.interfaces.nsIPropertyBag2);
  var platform = sysInfo.getProperty("name");
  
  if (platform == "Darwin") {
    showChrome = true;
  }
  
  var chromeFeatures = "chrome,modal=no,toolbar=no,popup=no";
  if (showChrome) {
    chromeFeatures += ",resizable=yes"; 
  } else  {
    chromeFeatures += ",titlebar=no";
  }
  
  var mainWin = window.open(mainwinURL, "", chromeFeatures);
  mainWin.focus();
}

// doEULA
//
// If it has not already been accepted, show the EULA and ask the user to
//   accept. If it is not accepted we call or eval the aCancelAction, if
//   it is accepted we call/eval aAcceptAction. The processing of the
//   actions happens in a different scope (eula.xul) so the best way
//   is to pass functions in that get called, instead of js.
// returns false if the main window should not be opened as we are showing
//   the EULA and awaitng acceptance by the user
// returns true if the EULA has already been accepted previously
function doEULA(aAcceptAction, aCancelAction)
{
  dump("doEULA\n");

  //SB_LOG("doEULA");
  // set to false just to be cautious
  var retval = false;
  try { 
    // setup the callbacks
    var eulaData = new Object();
    eulaData.acceptAction = aAcceptAction;
    eulaData.cancelAction = aCancelAction;

    var eulaCheck = false;
    try {
      eulaCheck = gPrefs.getBoolPref("songbird.eulacheck");
    } catch (err) { /* prefs throws an exepction if the pref is not there */ }

    if ( !eulaCheck ) {
      var eulaWindow =
        window.openDialog("chrome://songbird/content/xul/eula.xul",
                          "eula",
                          "chrome,centerscreen,modal=no,titlebar=yes",
                          eulaData );
      eulaWindow.focus();

      // We do not want to open the main window until we know EULA is accepted
      return false;
    }
    // Eula has been previously accepted, move along, move along.
    retval = true;  // if no accept action, just return true
    if (aAcceptAction) {
      if (typeof(aAcceptAction) == "function")
        retval = aAcceptAction();
      else
        retval = eval(aAcceptAction);
    }
  } catch (err) {
    SB_LOG("doEula", "" + err);
  }
  return retval; 
}

// doFirstRun
//
// Check the pref to see if this is the first run. If so, launch the firstrun
//   dialog and return. The handling in the firstrun dialog will cause the
//   main window to be launched.
// returns true to indicate the window should be launched
// returns false to indicate that the window should not be launched yet as the
//   firstrun dialog has been launched asynchronously and will launch the
//   main window on exit.
function doFirstRun()
{
  dump("doFirstRun\n");
    
  try {
    var haveRun = false;
    try {
      haveRun = gPrefs.getBoolPref("songbird.firstrun.check");
    } catch (err) { /* prefs throws an exepction if the pref is not there */ }

    if ( ! haveRun ) {
      var data = new Object();
      
      data.onComplete = firstRunComplete;
      data.document = document;

      // This cannot be modal it will block the download of extensions
      window.openDialog( "chrome://songbird/content/xul/firstrun.xul",
                         "firstrun", 
                         "chrome,centerscreen,titlebar=no,resizable=no,modal=no",
                         data );

      // Do not open main window until the non-modal first run dialog returns
      return false;
    }
  } catch (err) {
    SB_LOG("doFirstRun", "" + err);
  }

  // If we reach this point this is not the first run and the user has accepted
  //   the EULA so launch the main window.
  return true;
}

function firstRunComplete(restartfirstrun) {
  if (restartfirstrun) {
    doFirstRun();
  } else {
    doMainwinStart();
  }
}

function SBRestartApp()
{
  // mark this session as clean, we did not crash
  var metrics = Components.classes["@songbirdnest.com/Songbird/Metrics;1"]
                .createInstance(Components.interfaces.sbIMetrics);
  if (!metrics) 
    dump("SBRestartApp could not get the metrics component\n"); // Don't throw, we still want to restart.
  else
    metrics.setSessionFlag(false); 

  // do NOT replace '_' with '.', or it will be handled as a metrics data:
  // it would be posted to the metrics aggregator, then reset to 0 automatically
  SBDataSetBoolValue("metrics_ignorenextstartup", true);
  
  // attempt to restart
  var as = Components.classes["@mozilla.org/toolkit/app-startup;1"]
                     .getService(Components.interfaces.nsIAppStartup);
  if (!as) 
    dump("SBRestartApp could not get the nsIAppStartup component\n"); // Don't throw, we really do want to at least exit.
  else
  {
    as.quit(Components.interfaces.nsIAppStartup.eRestart);
    as.quit(Components.interfaces.nsIAppStartup.eAttemptQuit);
  }

  onExit();
}

function SBMetricsAppShutdown() 
{
  var startstamp = SBDataGetIntValue("startup_timestamp");
  var timenow = new Date();
  var diff = (timenow.getTime() - startstamp)/60000;
  metrics_add("player", "timerun", null, diff);
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

function SBCreateLibraryRef() {
  // this is so we can playRef the library even if it has never been shown

  // Get a query object to handle the database transactions.
  var aDBQuery = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                           .createInstance(Components.interfaces.sbIDatabaseQuery);
  aDBQuery.setAsyncQuery(false); // hard and slow.  should only have to happen once.
  aDBQuery.setDatabaseGUID("songbird");

  // Get the library interface, make sure there is a default library (fast if already exists)
  const MediaLibrary = new Components.Constructor("@songbirdnest.com/Songbird/MediaLibrary;1", "sbIMediaLibrary");
  var aMediaLibrary = (new MediaLibrary()).QueryInterface(Components.interfaces.sbIMediaLibrary);
  aMediaLibrary.setQueryObject(aDBQuery);
  aMediaLibrary.createDefaultLibrary();

  // Make it all go now, please.
  aDBQuery.execute();
  aDBQuery.resetQuery();

  // Now convince the playlistsource to load the library
  var source = new sbIPlaylistsource();
  source.feedPlaylist( "NC:songbird_library", "songbird", "library");
  source.executeFeed( "NC:songbird_library" );
  
  source.waitForQueryCompletion( "NC:songbird_library" );  
  
  // After the call is done, force GetTargets
  source.forceGetTargets( "NC:songbird_library", false );
}

//
// Observer for other code to request application restart.
//
var songbird_restartNow;
const sb_restart_app = {
    observe: function ( aSubject, aTopic, aData ) { setTimeout("SBRestartApp();", 0); }
}
function SBRestarterInitialize() 
{
  songbird_restartNow = SB_NewDataRemote( "restart.restartnow", null );
  songbird_restartNow.bindObserver( sb_restart_app, true );
}
function SBRestarterDeinitialize() 
{
  // Unbind restartapp remote
  songbird_restartNow.unbind();
  songbird_restartNow = null;
}

function SBMigrateDatabase()
{
  var queryObject = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"]
    .createInstance(Components.interfaces.sbIDatabaseQuery);
  queryObject.setAsyncQuery(false);
  
  queryObject.setDatabaseGUID("songbird");
  queryObject.addQuery("ALTER TABLE library ADD COLUMN origin_url TEXT DEFAULT ''");
  queryObject.addQuery("INSERT OR REPLACE INTO library_desc VALUES (\"origin_url\", \"&metadata.origin_url\", 1, 0, 1, 0, -1, 'text', 1)");
  queryObject.addQuery("CREATE UNIQUE INDEX IF NOT EXISTS library_index_origin_url ON library(origin_url)");
  queryObject.addQuery("UPDATE library SET origin_url = url WHERE origin_url = ''");
  
  queryObject.execute();
  
  queryObject.resetQuery();
  queryObject.setDatabaseGUID("webplaylist");
  queryObject.addQuery("ALTER TABLE library ADD COLUMN origin_url TEXT DEFAULT ''");
  queryObject.addQuery("INSERT OR REPLACE INTO library_desc VALUES (\"origin_url\", \"&metadata.origin_url\", 1, 0, 1, 0, -1, 'text', 1)");
  queryObject.addQuery("CREATE UNIQUE INDEX IF NOT EXISTS library_index_origin_url ON library(origin_url)");
  queryObject.addQuery("UPDATE library SET origin_url = url WHERE origin_url = ''");
  
  queryObject.execute();
}

var theMetadataJobManager = null;

// !!!
// !!! THIS FUNCTION MUST BE CALLED AT THE BOTTOM OF THIS MODULE OR ELSE BAD THINGS HAPPEN !!!
// !!!
appInit.onScriptInit();

