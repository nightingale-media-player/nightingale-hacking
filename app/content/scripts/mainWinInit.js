// JScript source code
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
  dump("SBInitialize *** \n");
  
  try
  {
    windowPlacementSanityChecks();
    initializeDocumentPlatformAttribute();

    // Delay setting the min max callback to enable the reflow of the mainwin to
    // happen. The reflow is caused by restoring the window size and position (or
    // using the default size from xul/css).
    // See bug #5185.
    setTimeout("setMinMaxCallback()", 25);

    initJumpToFileHotkey();

    if (window.addEventListener)
      window.addEventListener("keydown", checkAltF4, true);
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
  window.removeEventListener("keydown", checkAltF4, true);

  window.gServicePane = null;
  window.gBrowser = null;

  resetJumpToFileHotkey();
  closeJumpTo();

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
  // Run first-run default smart playlist creation
  SBFirstRunSmartPlaylists();

  // Run first-run directory scan.
  // delay this until later, to give the session restore code time to load the
  // placeholder page if we are not importing anything
  setTimeout(SBFirstRunScanDirectories, 0);

  // Run first-run library import.
  SBFirstRunImportLibrary();
}

function SBFirstRunScanDirectories()
{
  var libMgr = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                 .getService(Ci.sbILibraryManager);

  // Do nothing if not set to scan directories.
  var firstRunDoScanDirectory =
        Application.prefs.getValue("songbird.firstrun.do_scan_directory",
                                   false);
  var isFirstRun =
        Application.prefs.getValue("songbird.firstrun.is_session", false);
  if (!firstRunDoScanDirectory) {
    if (isFirstRun) {
      const placeholderURL = "chrome://songbird/content/mediapages/firstrun.xul";
      var currentURI = gBrowser.selectedBrowser.currentURI.spec;
      if (currentURI == placeholderURL || currentURI == "about:blank") {
        gBrowser.loadMediaList(libMgr.mainLibrary, null, gBrowser.selectedTab);
      }
    }
    return;
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
  

  // Start scanning.  Report error if scan directory does not exist.
  if (firstRunScanDirectory && firstRunScanDirectory.exists()) {
    var scanJob = SBScanMedia(null, firstRunScanDirectory);
    if (scanJob) {
      scanJob.addJobProgressListener(function firstRunLibraryHider(){
        // only do anything on complete
        if (scanJob.status == Ci.sbIJobProgress.STATUS_RUNNING) {
          return;
        }
        
        // unhook the listener
        scanJob.removeJobProgressListener(arguments.callee);
        // load the main library in the media tab / first tab
        gBrowser.loadMediaList(libMgr.mainLibrary, null, gBrowser.selectedTab);
      });
    }
  } else {
    Cu.reportError("Scan directory does not exist: \"" +
                   firstRunScanDirectoryPath + "\"\n");
    // load the main library in the media tab / first tab
    gBrowser.loadMediaList(libMgr.mainLibrary, null, gBrowser.selectedTab);
  }
}

function SBFirstRunImportLibrary()
{
  // Do nothing if not set to import library.
  var firstRunDoImportLibrary =
        Application.prefs.getValue("songbird.firstrun.do_import_library",
                                   false);
  if (!firstRunDoImportLibrary)
    return;

  // Don't do a first-run library import again.  Flush to disk to be sure.
  Application.prefs.setValue("songbird.firstrun.do_import_library", false);
  var prefService = Cc["@mozilla.org/preferences-service;1"]
                      .getService(Ci.nsIPrefService);
  prefService.savePrefFile(null);

  // Import library.
  SBLibraryOpen(null, true);
}

function SBFirstRunSmartPlaylists() {
  var defaultSmartPlaylists = SBDataGetIntValue("firstrun.smartplaylist");
  if (defaultSmartPlaylists != 1) {
    SBDataSetIntValue("firstrun.smartplaylist", 1)
    var prefService = Cc["@mozilla.org/preferences-service;1"]
                        .getService(Ci.nsIPrefService);
    prefService.savePrefFile(null);
    createDefaultSmartPlaylists();
  }
}

function createDefaultSmartPlaylists() {
  var defaultSmartPlaylists = [];

  var propertyManager = 
    Components.classes["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
              .getService(Components.interfaces.sbIPropertyManager);
  var numberPI = 
    propertyManager.getPropertyInfo(SBProperties.playCount);
  var ratingPI = 
    propertyManager.getPropertyInfo(SBProperties.rating);
  var datePI = 
    propertyManager.getPropertyInfo(SBProperties.created);
    
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
      name: "&smart.defaultlist.mostplayed",
      conditions: [
        { 
          property     : SBProperties.playCount,
          operator     : numberPI.getOperator(numberPI.OPERATOR_GREATER),
          leftValue    : 0,
          rightValue   : null,
          displayUnit  : null
        }
      ],
      matchType        : sbILDSML.MATCH_TYPE_ALL,
      limitType        : sbILDSML.LIMIT_TYPE_ITEMS,
      limit            : 25,
      selectPropertyID : SBProperties.playCount,
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
}
