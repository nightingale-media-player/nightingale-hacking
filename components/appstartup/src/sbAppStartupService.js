/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const SB_APPSTARTUPSERVICE_CLASSNAME  = "sbAppStartupService";
const SB_APPSTARTUPSERVICE_DESC       = "Songbird Application Startup Service";
const SB_APPSTARTUPSERVICE_CONTRACTID = "@songbirdnest.com/app-startup-service;1";
const SB_APPSTARTUPSERVICE_CID        = "{bf8300d3-95a7-4f52-b8e1-7012d8c639b8}";

const COMMAND_LINE_TOPIC  = "sb-command-line-startup";
const SHUTDOWN_TOPIC      = "quit-application";

const FUEL_CONTRACTID = "@mozilla.org/fuel/application;1";
const DATAREMOTE_TESTMODE = "__testmode__";

const TEST_FLAG = "test";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://app/jsmodules/WindowUtils.jsm");

/**
 * Global getter for FUEL
 */
__defineGetter__("Application", function() {
  delete Application;
  Application = Cc["@mozilla.org/fuel/application;1"]
                  .getService(Ci.fuelIApplication);
  return Application;
});

/**
 * Global getter for Observer Service
 */
__defineGetter__("ObserverService", function() {
  delete ObserverService;
  ObserverService = Cc["@mozilla.org/observer-service;1"]
                      .getService(Ci.nsIObserverService);
  return ObserverService;
});

function sbAppStartupService() {
  // We need to look at the command line asap so we can determine
  // if we are in test mode or not.
  ObserverService.addObserver(this, COMMAND_LINE_TOPIC, false);
}

sbAppStartupService.prototype = 
{
  // Application is initialized
  _initialized: false,
  // Application is in test mode
  _testMode: false,
  // Application is shutdown
  _shutdownComplete: false,
  // Application restart is required
  _restartRequired: false,
  // DataRemote Command Line Handler 
  _dataRemoteCmdLineHandler: null,
  // DataRemote For App Restart
  _dataRemoteAppRestart: null,
  
  // nsIObserver
  observe: function(aSubject, aTopic, aData) {
    switch(aTopic) {
      case COMMAND_LINE_TOPIC: {
        // Done with this notification
        ObserverService.removeObserver(this, COMMAND_LINE_TOPIC);

        // We need to catch quit-application to clean up
        ObserverService.addObserver(this, SHUTDOWN_TOPIC, false);

        // Check to see if we are running in test mode.
        this._checkForTestCommandLine(aSubject);

        // Bootstrap Application        
        this._bootstrap();
      }
      break;
      
      case SHUTDOWN_TOPIC: {
        ObserverService.removeObserver(this, SHUTDOWN_TOPIC);
        
        // Shut it all down!
        this._shutdown();
      }
      break;
    }
  },
  
  ///////////////////////////////////////////////////////////////////
  // Internal Methods
  ///////////////////////////////////////////////////////////////////
  
  _bootstrap: function() {
    if (this._initialized) {
      Cu.reportError("bootstrap is getting called multiple times");
      return;
    }
    
    // Show the first-run wizard
    if (this._firstRun()) {
      this._mainWindowStart();
    }
    
    this._initApplication();
    this._initRestarter();
    
    this._initialized = true;
    
    // Restart app if required.  Do this after initializing so that 
    // shutting down does not cause errors.
    if (this._restartRequired) {
      WindowUtils.restartApp();
      return;
    }
    
    // Check to see if any version update migrations need to be performed
    this._migrator.doMigrations();
  },
  
  _shutdown: function() {
    if(this._shutdownComplete) {
      Cu.reportError("shutdown is getting called multiple times");
      return;
    }
    
    this._shutdownApplication();
    this._cleanupRestarter();
    
    if(this._testMode) {
      SBDataSetBoolValue(DATAREMOTE_TESTMODE, false);
    }
  },
  
  _checkForTestCommandLine: function(aCommandLine) {
    try {
      var cmdLine = aCommandLine.QueryInterface(Ci.nsICommandLine);
      var flagPos = cmdLine.findFlag(TEST_FLAG, false);

      if(flagPos >= 0) {
        this._testMode = true;
        SBDataSetBoolValue(DATAREMOTE_TESTMODE, true);
      }
      else {
        // Explicitily reset testmode if we don't see the test flag.
        // We do this in case a previous test run didn't shut down
        // cleanly so that a developer isn't confused about the main
        // window not coming up.
        SBDataSetBoolValue(DATAREMOTE_TESTMODE, false);
      }
    } 
    catch(e) {
      Cu.reportError(e);
    }
  },
  
  /**
   * \brief Main Application Startup
   */
  _initApplication: function () {
    try {
      // XXXAus: We should not have to call this when mozbug 461399 is fixed.
      // Initialize the extension manager permissions.
      this._initExtensionManagerPermissions();
    }
    catch (e) {
      Cu.reportError(e);
    }
    
    try {
      // Startup the Metrics
      this._metricsAppStart();
    }
    catch (e) {
      Cu.reportError(e);
    }
    
    try {    
      // Handle dataremote commandline parameters
      this._initDataRemoteCmdLine();
    }
    catch (e) {
      Cu.reportError(e);
    }
    
    try {
      // Handle app startup command line parameters
      this._initStartupCmdLine();
    }
    catch( e ) {
      Cu.reportError(e);
    }

    try {
      // On Windows and Linux, register the songbird:// protocol.
      // (Mac is in the info.plist)
      var platform = Cc["@mozilla.org/system-info;1"]
                       .getService(Ci.nsIPropertyBag2) 
                       .getProperty("name");
      if (platform == "Windows_NT") {
        this._registerProtocolHandlerWindows();
      }
      else {
        this._registerProtocolHandlerGConf();
      }
    }
    catch (e) {
      Cu.reportError(e);
    }
  },
  
  /**
   * \brief Main Application Shutdown
   */
  _shutdownApplication: function () {
    // Shutdown app startup commandline handler
    this._cleanupStartupCmdLine();

    // Shutdown dataremote commandline handler
    this._cleanupDataRemoteCmdLine();
    
    // Shutdown the Metrics
    this._metricsAppShutdown();
  },

  /**
   * \brief Main Application Window Startup
   */
  _mainWindowStart: function () {
    try {
      var metrics = Cc["@songbirdnest.com/Songbird/Metrics;1"]
                      .createInstance(Ci.sbIMetrics);
      metrics.checkUploadMetrics();
    }
    catch (e) {
      Cu.reportError(e);
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
    } catch (e) {
      Cu.reportError(e);
    }

    // Make sure the web playlist is enabled.
    // This is to protect against cases where the app is shut down
    // while an extension has the web playlist disabled.
    SBDataSetBoolValue("webplaylist.enabled", true);

    var feathersManager = Cc['@songbirdnest.com/songbird/feathersmanager;1']
                            .getService(Ci.sbIFeathersManager);
    feathersManager.openPlayerWindow();
  },

  /**
   * \brief Application First Run 
   */
  _firstRun: function () {
    var perfTest = false;

    // Skip first-run if we are in test mode (ie. started with -test)
    if (this._testMode) {
      return true;
    }

    // Skip first-run if allowed (debug mode) and set to skip
    if (Application.prefs.getValue("songbird.first_run.allow_skip", false)) {
      var environment = Cc["@mozilla.org/process/environment;1"]
                          .getService(Ci.nsIEnvironment);
                          
      // If we're skipping the first run
      if (environment.exists("SONGBIRD_SKIP_FIRST_RUN")) {
        // Check to see if we're skipping all or just part
        var skipFirstRun = environment.get("SONGBIRD_SKIP_FIRST_RUN");
        perfTest = (skipFirstRun == "CSPerf");
        
        // Skip the first run wizard unless we're in a perf run.
        if (skipFirstRun && !perfTest)
          return true;
      }
    }

    try {
      var haveRun = Application.prefs.getValue("songbird.firstrun.check.0.3", 
                                               false);
      if (!haveRun) {
        
        var self = this;
        var data = {
          perfTest: null,
          onComplete: function(aRedo) {
            self._firstRunComplete(aRedo); 
          },
          QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports])
        };
        
        data.perfTest = perfTest;
        
        // Bah, we need to wrap the object so it crosses 
        // xpconnect boundaries properly.
        var sip = Cc["@mozilla.org/supports-interface-pointer;1"]
                    .createInstance(Ci.nsISupportsInterfacePointer);
        sip.data = data;
        
        // Wrap it.
        data.wrappedJSObject = data;

        // This cannot be modal as it will block the download of extensions.
        WindowUtils.openDialog(null,
                               "chrome://songbird/content/xul/firstRunWizard.xul",
                               "firstrun",
                               "chrome,titlebar,centerscreen,resizable=no",
                               false,
                               [sip]);
        // Avoid leaks
        sip.data = null;
                               
        // Do not open main window until the non-modal first run dialog returns
        return false;
      }
    } catch (e) {
      Cu.reportError(e);
    }

    // If we reach this point this is not the first run and the user has accepted
    //   the EULA so launch the main window.
    return true;
  },
  
  _firstRunComplete: function(aRedo) {
    if (aRedo) {
      this._firstRun();
    } 
    else {
      // If EULA has not been accepted, quit application.
      var eulaAccepted = 
        Application.prefs.getValue("songbird.eulacheck", false);
      
      if (!eulaAccepted) {
        // XXXAus: QUIT APP!!!
        //this.quitApp();
      }

      this._mainWindowStart();
    }
  },
  
  /**
   * \brief Initialize the Application Restarter. The restarter is used
   *        during the first run process to restart the application
   *        after first-run add-ons have been installed.
   */
  _initRestarter: function () {
    this._dataRemoteAppRestart = SBNewDataRemote( "restart.restartnow", null );
    this._dataRemoteAppRestart.boolValue = false;
    
    this._dataRemoteAppRestartHandler = {
      observe: function ( aSubject, aTopic, aData ) { 
        var appStartup = Components.classes["@mozilla.org/toolkit/app-startup;1"]
                                   .getService(Components.interfaces.nsIAppStartup);
        appStartup.quit(appStartup.eAttemptQuit | appStartup.eRestart);
      }
    };
    
    this._dataRemoteAppRestart.bindObserver( this._dataRemoteAppRestartHandler, 
                                             true );
  },
  
  /**
   * \brief Cleanup the Application Restarter.
   */
  _cleanupRestarter: function () {
    this._dataRemoteAppRestart.unbind();
    this._dataRemoteAppRestart = null;
  }, 

  /**
   * \brief Initialize the DataRemote used to propagate the command line 
   *        parameters.
   */
  _initDataRemoteCmdLine: function () {
    var cmdLine = Cc["@songbirdnest.com/commandlinehandler/general-startup;1?type=songbird"];
    if (!cmdLine)
      return;

    var cmdLineService = cmdLine.getService(Ci.nsICommandLineHandler);
    if (!cmdLineService)
      return;
    
    var cmdLineManager = 
      cmdLineService.QueryInterface(Ci.sbICommandLineManager);
    if (!cmdLineManager)
      return;

    this._dataRemoteCmdLineHandler = {
      handleFlag: function(aFlag, aParam)
      {
        var v = aParam.split("=");
        if (v[0] && v[1]) {
          SBDataSetStringValue(v[0], v[1]);
          return true;
        }
        
        return false;
      },

      QueryInterface: XPCOMUtils.generateQI([Ci.sbICommandLineFlagHandler])
    };
    
    cmdLineManager.addFlagHandler(this._dataRemoteCmdLineHandler, "data");
  },

  /**
   * \brief Cleanup the DataRemote used to propagate the command line
   *        parameters.
   */
  _cleanupDataRemoteCmdLine: function () {
    var cmdLine = Cc["@songbirdnest.com/commandlinehandler/general-startup;1?type=songbird"];
    if (!cmdLine)
      return;
      
    var cmdLineService = cmdLine.getService(Ci.nsICommandLineHandler);
    if (!cmdLineService)
      return;
      
    var cmdLineManager = 
      cmdLineService.QueryInterface(Ci.sbICommandLineManager);
    if (!cmdLineManager) 
      return;
      
    cmdLineManager.removeFlagHandler(this._dataRemoteCmdLineHandler, "data");
  }, 

  /**
   * \brief Initialize the DataRemote used to propagate the command line
   *        parameters.
   */
  _initStartupCmdLine: function () {
    var cmdLine = Cc
      ["@songbirdnest.com/commandlinehandler/general-startup;1?type=songbird"];
    if (!cmdLine)
      return;

    var cmdLineService = cmdLine.getService(Ci.nsICommandLineHandler);
    if (!cmdLineService)
      return;

    var cmdLineManager =
      cmdLineService.QueryInterface(Ci.sbICommandLineManager);
    if (!cmdLineManager)
      return;

    // Add the start in application directory command line flag handler.
    cmdLineManager.addFlagHandler(startupCmdLineHandler,
                                  "start-in-app-directory");
  },

  /**
   * \brief Cleanup the DataRemote used to propagate the command line
   *        parameters.
   */
  _resetStartupCmdLine: function () {
    var cmdLine = Cc
      ["@songbirdnest.com/commandlinehandler/general-startup;1?type=songbird"];
    if (!cmdLine)
      return;

    var cmdLineService = cmdLine.getService(Ci.nsICommandLineHandler);
    if (!cmdLineService)
      return;

    var cmdLineManager =
      cmdLineService.QueryInterface(Ci.sbICommandLineManager);
    if (!cmdLineManager)
      return;

    // Remove the start in application directory command line flag handler.
    commandLineManager.removeFlagHandler(startupCmdLineHandler,
                                         "start-in-app-directory");
  },

  /**
   * \brief Register the songbird:// protocol handler on Windows
   */
  _registerProtocolHandlerWindows: function () {
    if (!("@mozilla.org/windows-registry-key;1" in Components.classes)) {
      // no windows registry key, probably not Windows
      return;
    }

    var path = Cc["@mozilla.org/file/directory_service;1"]
                 .getService(Ci.nsIProperties)
                 .get("CurProcD", Ci.nsIFile);
    var file = path.clone();
    // It'd be nice if there were a way to look this up. 
    // (mook suggests) ::GetModuleFileNameW(). 
    // http://mxr.mozilla.org/seamonkey/source/browser/components/shell/src/nsWindowsShellService.cpp#264
    file.append("songbird.exe");

    var wrk = Cc["@mozilla.org/windows-registry-key;1"]
                .createInstance(Ci.nsIWindowsRegKey);

    wrk.create(wrk.ROOT_KEY_CURRENT_USER,
               "Software\\Classes\\songbird",
               wrk.ACCESS_WRITE);
    wrk.writeStringValue("", "URL:Songbird protocol");
    wrk.writeStringValue("URL Protocol", "");

    // The EditFlags key will make the protocol show up in the File Types
    // dialog in Windows XP.
    wrk.writeBinaryValue("EditFlags", '\002\000\000\000');

    wrk.create(wrk.ROOT_KEY_CURRENT_USER,
               "Software\\Classes\\songbird\\DefaultIcon",
               wrk.ACCESS_WRITE);
    wrk.writeStringValue("", '"' + file.path + '"');

    wrk.create(wrk.ROOT_KEY_CURRENT_USER,
               "Software\\Classes\\songbird\\shell\\open\\command",
               wrk.ACCESS_WRITE);
    wrk.writeStringValue("", '"' + file.path + '" "%1"');
    wrk.close();
  },

  /**
   * \brief Register the songbird:// protocol handler on Gnome/Linux
   */
  _registerSongbirdProtocolGConf: function () {
    if (!("@mozilla.org/gnome-gconf-service;1" in Components.classes)) {
      // gnome-gconf-service doesn't exist
      return;
    }
    var path = Cc["@mozilla.org/file/directory_service;1"]
                 .getService(Ci.nsIProperties)
                 .get("CurProcD", Ci.nsIFile);
    var file = path.clone();
    file.append("songbird");

    var gconf = Cc["@mozilla.org/gnome-gconf-service;1"]
                  .getService(Ci.nsIGConfService);
    gconf.setString("/desktop/gnome/url-handlers/songbird/command", '"' + file.path + '" "%s"');
    gconf.setBool("/desktop/gnome/url-handlers/songbird/enabled", true);
    gconf.setBool("/desktop/gnome/url-handlers/songbird/needs_terminal", false);
  },
  
  /**
   * \brief Metrics Startup Cleanup
   */
  _metricsAppStart: function () {
    var metrics = Cc["@songbirdnest.com/Songbird/Metrics;1"]
                    .createInstance(Ci.sbIMetrics);
    metrics.metricsInc("app", "appstart", null);

    // saw some strange issues with using the shortcut get/set versions of our
    //   dataremote global methods so actually creating the dataremotes.
    var reportedCrash = SBNewDataRemote( "crashlog.reported", null );
    var dirtyExit = SBNewDataRemote( "crashlog.dirtyExit", null );
    if ( dirtyExit.boolValue ) {
      // we are reporting the crash, save that knowledge
      reportedCrash.boolValue = true;

      var crashNum = SBNewDataRemote( "crashlog.num", null );
      var uptime = SBNewDataRemote( "crashlog.uptime", null );

      // send the uptime for this past crash
      // metrics key = crashlog.app.# = minute.
      // The thinking here is that we may want to report on different kinds of
      // crashes in the future. Therefore we will keep the top level bucket as
      // crashlog, with a subdivision for the app.
      metrics.metricsAdd("crashlog", 
                         "app" , 
                         crashNum.intValue, 
                         uptime.intValue);

      // reset the uptime, inc the crashNum
      uptime.intValue = 0;
      crashNum.intValue = crashNum.intValue+1
    } else {
      // keep track of if we are reporting the crash, may be useful later
      reportedCrash.boolValue = false;
    }

    // mark this true so if we crash we know it. It gets reset by AppShutdown
    dirtyExit.boolValue = true;
    
    var startstamp = (new Date()).getTime();
    SBDataSetStringValue("startup_timestamp", startstamp); // 64bit, use StringValue

    // force prefs to write out
    //  - too bad Application.prefs doesn't expose this method.
    var prefService =
      Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefService);
    prefService.savePrefFile(null);
  },

  /**
   * \brief Metrics Shutdown Cleanup
   */
  _metricsAppShutdown: function () {
    var startstamp = parseInt( SBDataGetStringValue("startup_timestamp") ); // 64bit, use StringValue
    var timenow = (new Date()).getTime();
    var ticsPerMinute = 1000 * 60;
    var minutes = ( (timenow - startstamp) / ticsPerMinute ) + 1; // Add one for fractionals
    
    var metrics = Cc["@songbirdnest.com/Songbird/Metrics;1"]
                    .createInstance(Ci.sbIMetrics);
    metrics.metricsAdd("app", "timerun", null, minutes);

    // we shut down cleanly, let AppStart know that -- for some reason
    // SBDataSetBoolValue didn't work here, create the dataremote entirely.
    var dirtyExit = SBNewDataRemote( "crashlog.dirtyExit", null );
    dirtyExit.boolValue = false;

    // increment uptime since last crash
    var uptime = SBNewDataRemote( "crashlog.uptime", null );
    uptime.intValue = uptime.intValue + minutes;
  },

  /**
   * \brief Initialize the Extension Manager Permissions so it includes
   *        the Songbird Add-Ons site as a permitted install location.
   */ 
  _initExtensionManagerPermissions: function () {
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
          if (value.length > 0) {
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
        }
        
        prefBranch.setCharPref(prefName, "");
      }
    }
  },

  /////////////////////////////////////////////////////////////////////
  // Migrator
  /////////////////////////////////////////////////////////////////////
  /**
   * Responsible for performing version update migrations
   */
  _migrator: {
    /**
     * Handle any necessary migration tasks. Called at application initialization.
     */
    doMigrations: function doMigrations() {
      try {
        this._updateUI();
      } catch (e) {
        Cu.reportError(e);
      }
    },
    
    /**
     * Perform UI migration tasks
     */
    _updateUI: function _updateUI() {
      var prefBranch = Cc["@mozilla.org/preferences-service;1"]
                         .getService(Ci.nsIPrefBranch);
                         
      var migration = 
        Application.prefs.getValue("songbird.migration.ui.version", 0);
      
      // In 0.5 we added the "media pages" switcher button.
      // Make sure it appears in the nav-bar currentset.
      switch (migration) {
        case 0:
        {
          // Get a wrapper for localstore.rdf
          var localStoreHelper = this._getLocalStoreHelper();

          // Make sure the media page switching is in the web toolbar 
          // currentset for each known layout
          var feathersManager = Cc['@songbirdnest.com/songbird/feathersmanager;1']
                                  .getService(Ci.sbIFeathersManager);
          var layouts = feathersManager.getLayoutDescriptions();
          while (layouts.hasMoreElements()) {
            var layoutURL = layouts.getNext()
                                   .QueryInterface(Ci.sbILayoutDescription).url;
            var currentSet = 
              localStoreHelper.getPersistedAttribute(layoutURL, 
                                                     "nav-bar", 
                                                     "currentset");
            if (currentSet && currentSet.indexOf("mediapages-container") == -1) {
              currentSet += ",mediapages-container";
              localStoreHelper.setPersistedAttribute(layoutURL, 
                                                     "nav-bar", 
                                                     "currentset", 
                                                     currentSet);
            }
          }
          localStoreHelper.flush();

          // migrate preferences
          const PREF_OLD_DOWNLOAD_MUSIC_FOLDER = 
            "songbird.download.folder";
          const PREF_OLD_DOWNLOAD_MUSIC_ALWAYSPROMPT = 
            "songbird.download.always";
          const PREF_DOWNLOAD_MUSIC_FOLDER  = 
            "songbird.download.music.folder";
          const PREF_DOWNLOAD_MUSIC_ALWAYSPROMPT = 
            "songbird.download.music.alwaysPrompt";

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

          // Fall through to process the next migration. 
        }
        case 1:
        {
          // Migrate iTunes importer pref changes introduced in Songbird 1.2
          const PREF_OLD_AUTO_ITUNES_IMPORT = 
            "songbird.library_importer.auto_import";
          const PREF_IMPORT_ITUNES =
            "songbird.library_importer.import_tracks";

          const PREF_OLD_ITUNES_DONT_IMPORT_PLAYLISTS = 
            "songbird.library_importer.dont_import_playlists";
          const PREF_ITUNES_IMPORT_PLAYLISTS =
            "songbird.library_importer.import_playlists";

          // Migrating these prefs isn't as easy as |_migratePref|, since the 
          // "auto_import" pref will determine the value for "dont import 
          // playlists" pref.
          if (Application.prefs.getValue(PREF_OLD_AUTO_ITUNES_IMPORT, 
                                              false)) {
            // Migrate the "auto_import" pref.
            Application.prefs.setValue(PREF_IMPORT_ITUNES, true);

            // Now migrate the import playlists pref.
            Application.prefs.setValue(
              PREF_ITUNES_IMPORT_PLAYLISTS,
              !Application.prefs.getValue(
                  PREF_OLD_ITUNES_DONT_IMPORT_PLAYLISTS, false)
            );
          }

          // update the migration version
          prefBranch.setIntPref("songbird.migration.ui.version", ++migration);
        }
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
  },

  // XPCOM Goo
  className: SB_APPSTARTUPSERVICE_CLASSNAME,
  classDescription: SB_APPSTARTUPSERVICE_DESC,
  classID: Components.ID(SB_APPSTARTUPSERVICE_CID),
  contractID: SB_APPSTARTUPSERVICE_CONTRACTID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]) 
};

//------------------------------------------------------------------------------
// XPCOM Registration

function NSGetModule(compMgr, fileSpec)
{
  return XPCOMUtils.generateModule([sbAppStartupService],
    function(aCompMgr, aFileSpec, aLocation) {
      XPCOMUtils.categoryManager.addCategoryEntry("app-startup",
                                                  SB_APPSTARTUPSERVICE_DESC,
                                                  "service," +
                                                  SB_APPSTARTUPSERVICE_CONTRACTID,
                                                  true,
                                                  true);
    }
  );
}

//------------------------------------------------------------------------------
// Startup command line handler

var startupCmdLineHandler = {
  handleFlag: function startupCmdLineHandler_handleFlag(aFlag, aParam) {
    switch (aFlag)
    {
      case "start-in-app-directory" :
        this._handleStartInAppDirectory();
        break;

      default :
        return false;
    }

    return true;
  },

  _handleStartInAppDirectory:
    function startupCmdLineHandler__handleStartInAppDirectory() {
    // Get the application directory.
    var directoryService = Cc["@mozilla.org/file/directory_service;1"]
                             .getService(Ci.nsIProperties);
    var appDirectory = directoryService.get("resource:app", Ci.nsIFile);

    // Set the current working directory to the application directory.
    var fileUtils = Cc["@songbirdnest.com/Songbird/FileUtils;1"]
                      .getService(Ci.sbIFileUtils);
    fileUtils.currentDir = appDirectory;
  },

  QueryInterface: XPCOMUtils.generateQI([ Ci.sbICommandLineFlagHandler ])
};

