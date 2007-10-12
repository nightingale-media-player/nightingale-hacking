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

    // Startup the Hotkeys
    initGlobalHotkeys();
    
    // Handle dataremote commandline parameters
    initDataRemoteCmdLine();

/*
    // XXX Migrate this crap

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
*/    
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
  
  // Shutdown dataremote commandline handler
  resetDataRemoteCmdLine();
  
  // Shutdown the Metrics
  SBMetricsAppShutdown();
  
/*  
  // Shutdown the Watch Folders
  try {
    WFShutdown();
  } catch(err) {
    dump(err);
  }
*/  
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
    
  // get the startup service and tell it to shut us down
  var as = Components.classes["@mozilla.org/toolkit/app-startup;1"]
                     .getService(Components.interfaces.nsIAppStartup);
  if (!as) 
    throw("doShutdown could not get the app-startup component!");

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
  feathersManager.openPlayerWindow();
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
      haveRun = gPrefs.getBoolPref("songbird.firstrun.check.0.3");
    } catch (err) { /* prefs throws an exepction if the pref is not there */ }

    if ( ! haveRun ) {
      var data = new Object();
      
      data.onComplete = firstRunComplete;
      data.document = document;

      // This cannot be modal it will block the download of extensions
      window.openDialog( "chrome://songbird/content/xul/firstRun.xul",
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
  // attempt to restart
  var as = Components.classes["@mozilla.org/toolkit/app-startup;1"]
                     .getService(Components.interfaces.nsIAppStartup);
  if (!as) 
    dump("SBRestartApp could not get the nsIAppStartup component\n"); // Don't throw, we really do want to at least exit.
  else
  {
    as.quit(Components.interfaces.nsIAppStartup.eRestart |
            Components.interfaces.nsIAppStartup.eAttemptQuit);
  }

  onExit();
}

function SBMetricsAppShutdown() 
{
  var startstamp = parseInt( SBDataGetStringValue("startup_timestamp") ); // 64bit, use StringValue
  var timenow = (new Date()).getTime();
  var ticsPerMinute = 1000 * 60;
  var minutes = ( (timenow - startstamp) / ticsPerMinute ) + 1; // Add one for fractionals
  metrics_add("app", "timerun", null, minutes);
}

function SBMetricsAppStart()
{
  metrics_inc("app", "appstart", null);
  var startstamp = (new Date()).getTime();
  SBDataSetStringValue("startup_timestamp", startstamp); // 64bit, use StringValue
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

  var directory = Components.classes["@mozilla.org/file/directory_service;1"].
                  getService(Components.interfaces.nsIProperties).
                  get("ProfD", Components.interfaces.nsIFile);
  directory.append("db");
  if (directory.exists()) {
    var old_database_file = directory.clone();
    old_database_file.append("songbird.db");
    if (!old_database_file.exists()) {
      return;
    }
  }
    

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

function initDataRemoteCmdLine()
{
  var cmdline = Components.classes["@songbirdnest.com/commandlinehandler/general-startup;1?type=songbird"];
  if (cmdline) {
    var cmdline_service = cmdline.getService(Components.interfaces.nsICommandLineHandler);
    if (cmdline_service) {
      var cmdline_mgr = cmdline_service.QueryInterface(Components.interfaces.sbICommandLineManager);
      if (cmdline_mgr) {
        cmdline_mgr.addFlagHandler(dataRemoteCmdlineHandler, "data");
      }
    }
  }
}

function resetDataRemoteCmdLine()
{
  var cmdline = Components.classes["@songbirdnest.com/commandlinehandler/general-startup;1?type=songbird"];
  if (cmdline) {
    var cmdline_service = cmdline.getService(Components.interfaces.nsICommandLineHandler);
    if (cmdline_service) {
      var cmdline_mgr = cmdline_service.QueryInterface(Components.interfaces.sbICommandLineManager);
      if (cmdline_mgr) {
        cmdline_mgr.removeFlagHandler(dataRemoteCmdlineHandler, "data");
      }
    }
  }
}

var dataRemoteCmdlineHandler = 
{
  handleFlag: function(aFlag, aParam)
  {
    var v = aParam.split("=");
    if (v[0] && v[1]) {
      SBDataSetStringValue(v[0], v[1]);
      return true;
    } 
    return false;
  },

  QueryInterface : function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbICommandLineFlagHandler) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    
    return this;
  }
};

var registeredCores = [];
function UnregiserCoreWrappers() {

  // We must remove the core wrappers before this window closes since the core
  // wrapper JS objects live in the scope of this window.  If they outlive
  // the scope of this window we get "XPConnect is being called on a scope
  // without a 'Components' property!" assertions.
  var pps =
    Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
              .getService(Components.interfaces.sbIPlaylistPlayback);

  registeredCores.forEach(function(e) {
    pps.removeCore(e);
  });

}

var theMetadataJobManager = null;

// !!!
// !!! THIS FUNCTION MUST BE CALLED AT THE BOTTOM OF THIS MODULE OR ELSE BAD THINGS HAPPEN !!!
// !!!
appInit.onScriptInit();

