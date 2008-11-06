/*
//
// BEGIN SONGBIRD GPL
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

//
// XXX: This functionality was copied from the original 0.1 
// cheezyVideoWindow.xul and desperately needs to be rewritten.
//



//
// Module specific auto-init/deinit support
//
var appInit = {};
appInit.init_once = 0;
appInit.deinit_once = 0;
appInit.restartRequired = false;
appInit.onLoad = function()
{
  if (appInit.init_once++) { Components.utils.reportError("WARNING: appInit double init!!\n"); return; }
  // Application, initialize thyself.
  SBAppInitialize();
  SBRestarterInitialize();

  // Restart app if required.  Do this after initializing so that shutting down
  // does not cause errors.
  if (this.restartRequired) {
    restartApp();
    return;
  }

  // Check to see if any version update migrations need to be performed
  appInit.migrator.doMigrations();
}
appInit.onUnload = function()
{
  if (appInit.deinit_once++) { Components.utils.reportError("WARNING: appInit double deinit!!\n"); return; }
  window.removeEventListener("load", appInit.onLoad, false);
  window.removeEventListener("unload", appInit.onUnload, false);
  SBAppDeinitialize();
  SBRestarterDeinitialize();
}
appInit.onScriptInit = function()
{

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
  
  // Show the first-run wizard
  if ( doFirstRun() ) {
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
  try {
    // XXXAus: We should not have to call this when mozbug 461399 is fixed.
    // Initialize the extension manager permissions.
    initExtensionManagerPermissions();
    
    // Startup the Metrics
    SBMetricsAppStart();

    // Startup the Hotkeys
    initGlobalHotkeys();
    
    // Handle dataremote commandline parameters
    initDataRemoteCmdLine();
  }
  catch( err ) {
    alert( "SBAppInitialize\n" + err );
  }
}

//
// Called on window-unload of songbird.xul.
//
function SBAppDeinitialize()
{
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

  // Check for recommended add-on bundle updates.  This may wait for recommended
  // add-on bundle download or the recommended add-on wizard.
  try {
    // Check for updates.
    var addOnBundleUpdateService =
          Cc["@songbirdnest.com/AddOnBundleUpdateService;1"]
            .getService(Ci.sbIAddOnBundleUpdateService);
    addOnBundleUpdateService.checkForUpdates();

    // If restart is required, mark application for restart and return.  Don't
    // restart here, because doing so will lead to errors in shutting down.
    if (addOnBundleUpdateService.restartRequired) {
      this.restartRequired = true;
      return;
    }
  } catch (err) {
    SB_LOG("App Init - Add-On Bundle Update - ", "" + err);
  }

  // Make sure the web playlist is enabled.
  // This is to protect against cases where the app is shut down
  // while an extension has the web playlist disabled.
  SBDataSetBoolValue("webplaylist.enabled", true);

  var feathersManager = Components.classes['@songbirdnest.com/songbird/feathersmanager;1']
                                   .getService(Components.interfaces.sbIFeathersManager);
  feathersManager.openPlayerWindow();
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
  var perfTest = false;
  // Skip first-run if allowed (debug mode) and set to skip
  if (Application.prefs.getValue("songbird.first_run.allow_skip", false)) {
    var environment = Cc["@mozilla.org/process/environment;1"]
                        .getService(Ci.nsIEnvironment);
                        
    // If we're skipping the first run
    if (environment.exists("SONGBIRD_SKIP_FIRST_RUN")) {
      // Check to see if we're skipping all or just part
      var skipFirstRun = environment.get("SONGBIRD_SKIP_FIRST_RUN");
      perfTest = (skipFirstRun == "CSPerf");
      
      // Skip the first run wizard unless we're in a perf run
      if (skipFirstRun && !perfTest)
        return true;
    }
  }

  try {
    var haveRun = false;
    try {
      haveRun = gPrefs.getBoolPref("songbird.firstrun.check.0.3");
    } catch (err) { /* prefs throws an exepction if the pref is not there */ }

    if ( ! haveRun ) {
      var data = new Object();
      
      data.onComplete = firstRunComplete;
      data.document = document;
      data.perfTest = perfTest;
      
      // This cannot be modal as it will block the download of extensions
      SBOpenWindow("chrome://songbird/content/xul/firstRunWizard.xul",
                   "firstrun",
                   "chrome,centerscreen,resizable=no",
                   data,
                   window);

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
    // If EULA has not been accepted, quit application.
    var eulaAccepted = Application.prefs.getValue("songbird.eulacheck", false);
    if (!eulaAccepted)
      quitApp();

    doMainwinStart();
  }
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
    observe: function ( aSubject, aTopic, aData ) { setTimeout("restartApp();", 0); }
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





/**
 * Responsible for performing version update migrations
 */
appInit.migrator = {
  
  /**
   * Handle any necessary migration tasks. Called at application initialization.
   */
  doMigrations: function doMigrations() {
    try {
      this._updateUI();
    } catch (e) {
      Components.utils.reportError(e);
    }
  },
  
  /**
   * Perform UI migration tasks
   */
  _updateUI: function _updateUI() {
    var prefBranch = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
    
    var migration = 0;
    try {
      migration = prefBranch.getIntPref("songbird.migration.ui.version");
    } catch(ex) {}
    
    // In 0.5 we added the "media pages" switcher button.
    // Make sure it appears in the nav-bar currentset.
    if (migration == 0) {
      // Get a wrapper for localstore.rdf
      var localStoreHelper = this._getLocalStoreHelper();
      
      // Make sure the media page switching is in the web toolbar 
      // currentset for each known layout
      var feathersManager = Components.classes['@songbirdnest.com/songbird/feathersmanager;1']
                                      .getService(Components.interfaces.sbIFeathersManager);
      var layouts = feathersManager.getLayoutDescriptions();
      while (layouts.hasMoreElements()) {
        var layoutURL = layouts.getNext().QueryInterface(
                          Components.interfaces.sbILayoutDescription).url;      
        var currentSet = localStoreHelper.getPersistedAttribute(layoutURL, "nav-bar", "currentset");
        if (currentSet && currentSet.indexOf("mediapages-container") == -1) {
          currentSet += ",mediapages-container";
          localStoreHelper.setPersistedAttribute(layoutURL, "nav-bar", "currentset", currentSet);
        }
      }
      localStoreHelper.flush();
      
      // migrate preferences
      const PREF_OLD_DOWNLOAD_MUSIC_FOLDER        = "songbird.download.folder";
      const PREF_OLD_DOWNLOAD_MUSIC_ALWAYSPROMPT  = "songbird.download.always";
      const PREF_DOWNLOAD_MUSIC_FOLDER            = "songbird.download.music.folder";
      const PREF_DOWNLOAD_MUSIC_ALWAYSPROMPT      = "songbird.download.music.alwaysPrompt";

      this._migratePref(prefBranch, 
                        "setCharPref", 
                        "getCharPref", 
                        function(p) { return p; }, 
                        PREF_OLD_DOWNLOAD_MUSIC_FOLDER, 
                        PREF_DOWNLOAD_MUSIC_FOLDER);
      
      this._migratePref(prefBranch, 
                        "setBoolPref", 
                        "getCharPref", 
                        function(p) { return p=="1"; }, 
                        PREF_OLD_DOWNLOAD_MUSIC_ALWAYSPROMPT, 
                        PREF_DOWNLOAD_MUSIC_ALWAYSPROMPT);
      
      // update the migration version
      prefBranch.setIntPref("songbird.migration.ui.version", ++migration);
    }
  },
  
  _migratePref: function(prefBranch, setMethod, getMethod, cvtFunction, oldPrefKey, newPrefKey) {
    // if the old pref exists, do the migration
    if (this._hasPref(prefBranch, getMethod, oldPrefKey)) {
      // but only if the new pref does not exists, otherwise, keep the new pref's value
      if (!this._hasPref(prefBranch, getMethod, newPrefKey)) {
        prefBranch[setMethod](newPrefKey, cvtFunction(prefBranch[getMethod](oldPrefKey)));
      }
      // in every case, get rid of the old pref
      prefBranch.clearUserPref(oldPrefKey); 
    }
  },

  _hasPref: function(prefBranch, getMethod, prefKey) {
    try {
      var str = prefBranch[getMethod](prefKey);
      return (str && str != "");
    } catch (e) {
      return false;
    }
  },
  
  /**
   * Gets a wrapper for localstore.rdf
   */
  _getLocalStoreHelper: function _getLocalStoreHelper() {
    var LocalStoreHelper = function() {
      this._rdf = Cc["@mozilla.org/rdf/rdf-service;1"].getService(Ci.nsIRDFService);
      this._localStore = this._rdf.GetDataSource("rdf:local-store");
      this.dirty = false;
    }
    LocalStoreHelper.prototype = {
      
      // Get an attribute value for an element id in a given file
      getPersistedAttribute: function(file, id, attribute) {
        var source = this._rdf.GetResource(file + "#" + id);
        var property = this._rdf.GetResource(attribute);
        var target = this._localStore.GetTarget(source, property, true);
        if (target instanceof Ci.nsIRDFLiteral)
          return target.Value;
        return null;
      },
      
      // Set an attribute on an element id in a given file
      setPersistedAttribute: function(file, id, attribute, value) {
        var source = this._rdf.GetResource(file + "#" +  id);    
        var property = this._rdf.GetResource(attribute);
        try {
          var oldTarget = this._localStore.GetTarget(source, property, true);
          if (oldTarget) {
            if (value)
              this._localStore.Change(source, property, oldTarget, this._rdf.GetLiteral(value));
            else
              this._localStore.Unassert(source, property, oldTarget);
          }
          else {
            this._localStore.Assert(source, property, this._rdf.GetLiteral(value), true);
          }
          this.dirty = true;
        }
        catch(ex) {
          Components.utils.reportError(ex);
        }
      },
        
      // Save changes if needed
      flush: function flush() {
        if (this.dirty) {
          this._localStore.QueryInterface(Ci.nsIRDFRemoteDataSource).Flush();
        }
      }
    }
    return new LocalStoreHelper();
  }
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

// XXXAus: This function should be removed when mozbug 461399 is fixed.
function initExtensionManagerPermissions() {
  const httpPrefix = "http://";
  const prefRoot = "xpinstall.whitelist.add";
  const permissionType = "install";
  
  var prefBranch = 
    Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
    
  var ioService = 
    Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    
  var permissionManager = 
    Cc["@mozilla.org/permissionmanager;1"].getService(Ci.nsIPermissionManager);
  
  var childBranches = prefBranch.getChildList(prefRoot, {});
  
  for each (let prefName in childBranches) {
    if(prefBranch.getPrefType(prefName) == Ci.nsIPrefBranch.PREF_STRING) {
      let prefValue = prefBranch.getCharPref(prefName);
      let values = prefValue.split(",");
      for each (let value in values) {
        value = value.replace(" ", "", "g");
        value = httpPrefix + value;
        let uri = null;
        try {
          uri = ioService.newURI(value, null, null);
          permissionManager.add(uri, 
                                permissionType, 
                                Ci.nsIPermissionManager.ALLOW_ACTION);
        }
        catch(e) {
          Cu.reportError(e);
        }
      }
      
      prefBranch.setCharPref(prefName, "");
    }
  }
}

// !!!
// !!! THIS FUNCTION MUST BE CALLED AT THE BOTTOM OF THIS MODULE OR ELSE BAD THINGS HAPPEN !!!
// !!!
appInit.onScriptInit();
