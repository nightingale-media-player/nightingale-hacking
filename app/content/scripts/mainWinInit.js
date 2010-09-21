/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

//
// Mainwin Initialization
//
// XXX: This functionality was copied from the original 0.1 
// songbird_hack.js and desperately needs to be rewritten.
//


/**
 * \file mainWinInit.js
 * \brief Main window initialization functions and objects.
 * \internal
 */

var gServicePane = null;

Components.utils.import("resource://app/jsmodules/sbSmartMediaListColumnSpecUpdater.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");

//
// Module specific auto-init/deinit support
//
var mainWinInit = {};
mainWinInit.init_once = 0;
mainWinInit.deinit_once = 0;
mainWinInit.onLoad = function()
{
  if (mainWinInit.init_once++) { dump("WARNING: mainWinInit double init!!\n"); return; }
  SBInitialize();
}
mainWinInit.onUnload = function()
{
  if (mainWinInit.deinit_once++) { dump("WARNING: mainWinInit double deinit!!\n"); return; }
  window.removeEventListener("load", mainWinInit.onLoad, false);
  window.removeEventListener("unload", mainWinInit.onUnload, false);
  document.removeEventListener("sb-overlay-load", SBPostOverlayLoad, false);
  SBUninitialize();
}
window.addEventListener("load", mainWinInit.onLoad, false);
window.addEventListener("unload", mainWinInit.onUnload, false);
document.addEventListener("sb-overlay-load", SBPostOverlayLoad, false);



/**
 * \brief Initialize the main window.
 * \note Do not call more than once.
 * \internal
 */
function SBInitialize()
{
  try
  {
    windowPlacementSanityChecks();
    initializeDocumentPlatformAttribute();

    // Delay setting the min max callback to enable the reflow of the mainwin to
    // happen. The reflow is caused by restoring the window size and position (or
    // using the default size from xul/css).
    // See bug #5185.
    setTimeout("setMinMaxCallback()", 25);

    if (window.addEventListener)
      window.addEventListener("keydown", checkQuitKey, true);
  }
  catch(err)
  {
    alert("mainWinInit.js - SBInitialize - " +  err);
  }
}

/**
 * \brief Uninitialize the main window.
 * \note Do not call more than once.
 * \internal
 */
function SBUninitialize()
{
  window.removeEventListener("keydown", checkQuitKey, true);

  window.gServicePane = null;
  window.gBrowser = null;

  resetMinMaxCallback();
}

var SBWindowMinMaxCB =
{
  // Shrink until the box doesn't match the window, then stop.
  GetMinWidth: function()
  {
    return 700;
  },

  GetMinHeight: function()
  {
    return 400;
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
}; // SBWindowMinMax callback class definition

function setMinMaxCallback(evt)
{
  var platfrom = getPlatformString();

  try {

    if (platfrom == "Windows_NT") {
      var windowMinMax = Components.classes["@songbirdnest.com/Songbird/WindowMinMax;1"];
      var service = windowMinMax.getService(Components.interfaces.sbIWindowMinMax);

      service.setCallback(window, SBWindowMinMaxCB);
      return;
    }

  }
  catch (err) {
    // No component
    dump("Error. songbird_hack.js:setMinMaxCallback() \n " + err + "\n");
  }

  return;
}

function resetMinMaxCallback()
{
  var platform = getPlatformString();

  try
  {
    if (platform == "Windows_NT") {
      var windowMinMax = Components.classes["@songbirdnest.com/Songbird/WindowMinMax;1"];
      var service = windowMinMax.getService(Components.interfaces.sbIWindowMinMax);
      service.resetCallback(window);

      return;
    }

  }
  catch(err) {
    dump("Error. songbird_hack.js: SBUnitialize() \n" + err + "\n");
  }

  return;
}

function SBPostOverlayLoad()
{
  // Handle first run media import.
  // Delay this until later, to give the session restore code time to load the
  // placeholder page if we are not importing anything
  setTimeout(SBDoFirstRun, 500);
  
}

// Find media on first run
function SBDoFirstRun() {

  // Run first-run library import.
  var job = SBFirstRunImportLibrary();

  // Run first-run directory scan.
  job = SBFirstRunScanDirectories() || job;

  // determine whether to load the media library in the background by preference
  // so it can be overriden by partners in the distribution.ini
  var loadMLInBackground =
          Application.prefs.getValue("songbird.firstrun.load_ml_in_background", false);

  // If we are scanning directories or importing a library, 
  // track the progress and show the library on completion.
  // This is done to simplify the display, avoid some bugs,
  // and improve performance.
  if (job) {
    function onJobComplete() {
      // load the main library in the media tab / first tab
      const nsIWebNavigation = Components.interfaces.nsIWebNavigation;
      var mediaListView =
          LibraryUtils.createConstrainedMediaListView(
              LibraryUtils.mainLibrary, [SBProperties.contentType, "audio"]);
      gBrowser.loadMediaListViewWithFlags(mediaListView,
                                          gBrowser.mediaTab,
                                          null,
                                          nsIWebNavigation.LOAD_FLAGS_REPLACE_HISTORY,
                                          loadMLInBackground);

      // Set up the smart playlists after import is complete
      // (improves performance slightly)
      SBFirstRunSmartPlaylists();
    }
    if (job.status != Ci.sbIJobProgress.STATUS_RUNNING)
      setTimeout(onJobComplete, 100);
    else {
      job.addJobProgressListener(function firstRunLibraryHider(){
        // only do anything on complete
        if (job.status == Ci.sbIJobProgress.STATUS_RUNNING) {
          return;
        }
        // unhook the listener
        job.removeJobProgressListener(arguments.callee);
        // We have to do this from the "main thread" via the timer
        setTimeout(onJobComplete, 100);
      });
    }
  } else {
    // Make sure we have the default smart playlists
    SBFirstRunSmartPlaylists();
    
    var isFirstRun =
          Application.prefs.getValue("songbird.firstrun.is_session", false);
    var mediaListView =
        LibraryUtils.createConstrainedMediaListView(
            LibraryUtils.mainLibrary, [SBProperties.contentType, "audio"]);
    if (isFirstRun) {    
      const placeholderURL = "chrome://songbird/content/mediapages/firstrun.xul";
      var currentURI = gBrowser.mediaTab.linkedBrowser.currentURI.spec;
      if (currentURI == placeholderURL || currentURI == "about:blank") {
        const nsIWebNavigation = Components.interfaces.nsIWebNavigation;
        gBrowser.loadMediaListViewWithFlags(mediaListView,
                                            gBrowser.mediaTab,
                                            null,
                                            nsIWebNavigation.LOAD_FLAGS_REPLACE_HISTORY,
                                            loadMLInBackground);
      }
    }
  }
}

function SBFirstRunScanDirectories()
{
  // Do nothing if not set to scan directories.
  var firstRunDoScanDirectory =
        Application.prefs.getValue("songbird.firstrun.do_scan_directory",
                                   false);
  if (!firstRunDoScanDirectory) {
    return null;
  }

  // Don't do a first-run directory scan again.  Flush to disk to be sure.
  Application.prefs.setValue("songbird.firstrun.do_scan_directory", false);
  var prefService = Cc["@mozilla.org/preferences-service;1"]
                      .getService(Ci.nsIPrefService);
  prefService.savePrefFile(null);

  // Get the first-run scan directory.
  var firstRunScanDirectoryPath =
        Application.prefs.getValue("songbird.firstrun.scan_directory_path", "");
  var firstRunScanDirectory = Cc["@mozilla.org/file/local;1"]
                                .createInstance(Ci.nsILocalFile);
  try {
    firstRunScanDirectory.initWithPath(firstRunScanDirectoryPath);
  } catch (ex) {
    firstRunScanDirectory = null;
  }
  
  var job = null;

  // Start scanning.  Report error if scan directory does not exist.
  if (firstRunScanDirectory && firstRunScanDirectory.exists()) {
    job = SBScanMedia(null, firstRunScanDirectory);
  } else {
    Cu.reportError("Scan directory does not exist: \"" +
                   firstRunScanDirectoryPath + "\"\n");
  }
  return job;
}

function SBFirstRunImportLibrary()
{
  // Do nothing if not set to import library.
  var firstRunDoImportLibrary =
        Application.prefs.getValue("songbird.firstrun.do_import_library",
                                   false);
  if (!firstRunDoImportLibrary)
    return null;

  // Don't do a first-run library import again.  Flush to disk to be sure.
  Application.prefs.setValue("songbird.firstrun.do_import_library", false);
  var prefService = Cc["@mozilla.org/preferences-service;1"]
                      .getService(Ci.nsIPrefService);
  prefService.savePrefFile(null);

  // Import library.
  return SBLibraryOpen(null, true);
}

function SBFirstRunSmartPlaylists() {
  // Do nothing if first-run smart playlists were already created in the main
  // library.
  var libraryManager =
    Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
              .getService(Components.interfaces.sbILibraryManager);
  var library = libraryManager.mainLibrary;
  if (library.getProperty(SBProperties.createdFirstRunSmartPlaylists) == "1")
    return;

  // For backward compatibility, check preferences to see if the first-run smart
  // playlists were already created.
  var defaultSmartPlaylists = SBDataGetIntValue("firstrun.smartplaylist");
  if (defaultSmartPlaylists) {
    library.setProperty(SBProperties.createdFirstRunSmartPlaylists, "1");
    return;
  }

  library.setProperty(SBProperties.createdFirstRunSmartPlaylists, "1");
  createDefaultSmartPlaylists();
}

function createDefaultSmartPlaylists() {
  var defaultSmartPlaylists = [];

  var propertyManager =
    Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
      .getService(Ci.sbIPropertyManager);
  var numberPI =
    propertyManager.getPropertyInfo(SBProperties.playCount);
  var ratingPI =
    propertyManager.getPropertyInfo(SBProperties.rating);
  var datePI =
    propertyManager.getPropertyInfo(SBProperties.created);
  var typePI =
    propertyManager.getPropertyInfo(SBProperties.contentType);
 
  const sbILDSML = Components.interfaces.sbILocalDatabaseSmartMediaList;
 
  datePI.QueryInterface(Components.interfaces.sbIDatetimePropertyInfo);
 
  // XXXlone> waiting for a patch to land before autoUpdateMode fields
  // can be enabled

  defaultSmartPlaylists = [
    {
      name: "&smart.defaultlist.highestrated",
      conditions: [
        {
          property     : SBProperties.rating,
          operator     : ratingPI.getOperator(ratingPI.OPERATOR_GREATER),
          leftValue    : 3,
          rightValue   : null,
          displayUnit  : null
        },
        {
          property     : SBProperties.contentType,
          operator     : typePI.getOperator(typePI.OPERATOR_NOTEQUALS),
          leftValue    : "video",
          rightValue   : null,
          displayUnit  : null
        }
      ],
      matchType        : sbILDSML.MATCH_TYPE_ALL,
      limitType        : sbILDSML.LIMIT_TYPE_NONE,
      limit            : 0,
      selectPropertyID : SBProperties.rating,
      selectDirection  : false,
      randomSelection  : false,
      autoUpdate       : true
    },
    {
      name: "&smart.defaultlist.recentlyadded",
      conditions: [
        {
          property     : SBProperties.created,
          operator     : datePI.getOperator(datePI.OPERATOR_INTHELAST),
          leftValue    : 1000*60*60*24*30, // 30 days
          rightValue   : null,
          displayUnit  : "m"
        },
        {
          property     : SBProperties.contentType,
          operator     : typePI.getOperator(typePI.OPERATOR_NOTEQUALS),
          leftValue    : "video",
          rightValue   : null,
          displayUnit  : null
        }
      ],
      matchType        : sbILDSML.MATCH_TYPE_ALL,
      limitType        : sbILDSML.LIMIT_TYPE_NONE,
      limit            : 0,
      selectPropertyID : SBProperties.created,
      selectDirection  : false,
      randomSelection  : false,
      autoUpdate       : true
    },
    {
      name: "&smart.defaultlist.recentlyplayed",
      conditions: [
        {
          property     : SBProperties.lastPlayTime,
          operator     : datePI.getOperator(datePI.OPERATOR_INTHELAST),
          leftValue    : 1000*60*60*24*7, // 7 days
          rightValue   : null,
          displayUnit  : "w"
        },
        {
          property     : SBProperties.contentType,
          operator     : typePI.getOperator(typePI.OPERATOR_NOTEQUALS),
          leftValue    : "video",
          rightValue   : null,
          displayUnit  : null
        }
      ],
      matchType        : sbILDSML.MATCH_TYPE_ALL,
      limitType        : sbILDSML.LIMIT_TYPE_NONE,
      limit            : 0,
      selectPropertyID : SBProperties.lastPlayTime,
      selectDirection  : false,
      randomSelection  : false,
      autoUpdate       : true
    }
  ];
  
  for each (var item in defaultSmartPlaylists) {
    addSmartPlaylist(item);
  }
}

function addSmartPlaylist(aItem) {
  var libraryManager = 
    Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
              .getService(Components.interfaces.sbILibraryManager);
  
  library = libraryManager.mainLibrary;
  var mediaList = library.createMediaList("smart");
  for (var prop in aItem) {
    if (prop == "conditions") {
      for each (var condition in aItem.conditions) {
        mediaList.appendCondition(condition.property,
                                  condition.operator,
                                  condition.leftValue,
                                  condition.rightValue,
                                  condition.displayUnit);
      }
    } else {
      mediaList[prop] = aItem[prop];
    }
  }
  SmartMediaListColumnSpecUpdater.update(mediaList);
  mediaList.rebuild();
}
