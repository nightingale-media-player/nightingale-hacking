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
  // Run first-run directory scan.
  SBFirstRunScanDirectories();

  // Run first-run library import.
  SBFirstRunImportLibrary();
}

function SBFirstRunScanDirectories()
{
  // Do nothing if not set to scan directories.
  var firstRunDoScanDirectory =
        Application.prefs.getValue("songbird.firstrun.do_scan_directory",
                                   false);
  if (!firstRunDoScanDirectory)
    return;

  // Don't do a first-run directory scan again.
  Application.prefs.setValue("songbird.firstrun.do_scan_directory", false);

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
    SBScanMedia(null, firstRunScanDirectory);
  } else {
    Cu.reportError("Scan directory does not exist: \"" +
                   firstRunScanDirectoryPath + "\"\n");
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

  // Don't do a first-run library import again.
  Application.prefs.setValue("songbird.firstrun.do_import_library", false);

  // Import library.
  SBLibraryOpen(null, true);
}

