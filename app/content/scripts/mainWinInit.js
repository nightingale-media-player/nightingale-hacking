// JScript source code
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
// Mainwin Initialization
//

/**
 * \file mainWinInit.js
 * \brief Main window initialization functions and objects.
 * \internal
 */

var thePollPlaylistService = null;

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

  try
  {
    //Whatever migration is required between version, this function takes care of it.
    SBMigrateDatabase();
  }
  catch(e) { }

  dump("SBInitialize *** \n");

  window.focus();

  try
  {
    windowPlacementSanityChecks();

    // Set attributes on the Window element so we can use them in CSS.
    var platform = getPlatformString();
    var windowElement = document.getElementsByTagName("window")[0];
    windowElement.setAttribute("platform", platform);

    // Delay setting the min max callback to enable the reflow of the mainwin to
    // happen. The reflow is caused by restoring the window size and position (or
    // using the default size from xul/css).
    // See bug #5185.
    setTimeout("setMinMaxCallback()", 25);

    initJumpToFileHotkey();

    if (window.addEventListener)
      window.addEventListener("keydown", checkAltF4, true);

    window.gServicePane.onPlaylistDefaultCommand = onServiceTreeCommand;
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

  thePollPlaylistService = null;
}

var SBWindowMinMaxCB =
{
  // Shrink until the box doesn't match the window, then stop.
  GetMinWidth: function()
  {
    // What we'd like it to be
    var retval = 750;
    var outerframe = window.gOuterFrame;
    // However, if in resizing the window size is different from the document's box object
    if (window.innerWidth != outerframe.boxObject.width)
    {
      // That means we found the document's min width.  Because you can't query it directly.
      retval = outerframe.boxObject.width - 1;
    }
    return retval;
  },

  GetMinHeight: function()
  {
    // What we'd like it to be
    var outerframe = window.gOuterFrame;
    var retval = 400;
    // However, if in resizing the window size is different from the document's box object
    if (window.innerHeight != outerframe.boxObject.height)
    {
      // That means we found the document's min width.  Because you can't query it directly.
      outerframe = parent.boxObject.height - 1;
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

  window.addEventListener("resize", onMinimumWindowSize, false);

  // Read minimum width and height from CSS.
  var minWidth = getStyle(document.documentElement, "min-width");
  var minHeight = getStyle(document.documentElement, "min-height");

  if (minWidth) {
    minWidth = parseInt(minWidth);
  }

  if (!minWidth) {
    minWidth = 750;
  }

  if (minHeight) {
    minHeight = parseInt(minHeight);
  }

  if (!minHeight) {
    minHeight = 400;
  }

  gMinWidth = minWidth;
  gMinHeight = minHeight;

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

  window.removeEventListener("resize", onMinimumWindowSize, false);

  return;
}

function SBPostOverlayLoad()
{
  // After the overlays load, launch the scan for media loop if no scans have
  // previously occurred.  Check again if scan has occurred after a delay to
  // allow other scanners to run first (e.g., iTunes importer).
  var dataScan = SBDataGetBoolValue("firstrun.scancomplete");
  if (dataScan != true)
    setTimeout( SBPostOverlayLoad1, 1000 );
}

function SBPostOverlayLoad1()
{
  // Launch the scan for media loop if no scans have previously occurred.
  var dataScan = SBDataGetBoolValue("firstrun.scancomplete");
  if (dataScan != true)
  {
    SBDataSetBoolValue("firstrun.scancomplete", true);
    SBScanMedia(null);
  }
}
