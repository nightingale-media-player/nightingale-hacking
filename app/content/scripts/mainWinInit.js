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
//  Yes, I know this file is a mess.
//
//  Yes, I know we have to clean it up.
//
//  Yes, this will happen soon.
//
//  I promise.  Or something.
//
//                  - mig
//

//
// Mainwin Initialization
//
var thePollPlaylistService = null;

var gServicePane = null;
var gBrowser = null;
function getBrowser() { return gBrowser; }

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
    onWindowLoadSizeAndPosition();

    // because the main window can change its minmax size from session to session (ie, long items in the service tree), 
    // we need to determine whether the loaded size is within the current minmax. If not, tweak the size by the difference
    /*
    var w = document.documentElement.boxObject.width;
    var h = document.documentElement.boxObject.height;
    var diffw = document.getElementById('window_parent').boxObject.width - window.innerWidth;
    var diffh = document.getElementById('window_parent').boxObject.height - window.innerHeight;
    // todo: see if that detects the situation
    dump("diffw = " + diffw + "\n");
    dump("diffh = " + diffh + "\n");
    // todo: resize the window accordingly (same method as window_utils.js: 448 to 455)
    */
    
    setMinMaxCallback();
    
    // This is the sb-tabbrowser xul element from mainwin.xul
    gBrowser = document.getElementById("frame_main_pane");
    // looks like we need to attach this to the window...
    window.gBrowser = gBrowser;
            
    initJumpToFileHotkey();

    if (window.addEventListener)
      window.addEventListener("keydown", checkAltF4, true);
      
    // On firstrun, popup the scan for media dialog.
    // So, now you can't force the dialog by deleting all your databasey files.  Oh well.
    var dataScan = SBDataGetBoolValue("firstrun.scancomplete");
    if (dataScan != true)
    {
      theMediaScanIsOpen.boolValue = true;  // We don't use this anymore, I should pull it.
                                            // But it's everywhere, so after 0.2 release.
      setTimeout( SBScanMedia, 1000 );
      SBDataSetBoolValue("firstrun.scancomplete", true);
    }
       
    // Look at all these ugly hacks that need to go away.  (sigh)
    gServicePane = document.getElementById('servicepane');
    
    gServicePane.onPlaylistDefaultCommand = onServiceTreeCommand;
    
  }
  catch(err)
  {
    alert("mainWinInit.js - SBInitialize - " +  err);
  }
}

function SBUninitialize()
{
  window.removeEventListener("keydown", checkAltF4, true);
  
  var mainPane = document.getElementById("frame_main_pane");

  gServicePane = null;
  gBrowser = null;

  resetJumpToFileHotkey();
  closeJumpTo();
  try {
    var windowMinMax = Components.classes["@songbirdnest.com/Songbird/WindowMinMax;1"];
    if (windowMinMax) {
      var service = windowMinMax.getService(Components.interfaces.sbIWindowMinMax);
      if (service)
        service.resetCallback(document);
    }
  }
  catch(err) {
    dump("Error. songbird_hack.js: SBUnitialize() \n" + err + "\n");
  }

  thePollPlaylistService = null;
}

var SBWindowMinMaxCB = 
{
  // Shrink until the box doesn't match the window, then stop.
  GetMinWidth: function()
  {
    // What we'd like it to be
    var retval = 750;
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
    var retval = 400;
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
}; // SBWindowMinMax callback class definition

function setMinMaxCallback()
{
  try {
    var windowMinMax = Components.classes["@songbirdnest.com/Songbird/WindowMinMax;1"];
    if (windowMinMax) {
      var service = windowMinMax.getService(Components.interfaces.sbIWindowMinMax);
      if (service)
        service.setCallback(document, SBWindowMinMaxCB);
    }
  }
  catch (err) {
    // No component
    dump("Error. songbird_hack.js:setMinMaxCallback() \n " + err + "\n");
  }
}
