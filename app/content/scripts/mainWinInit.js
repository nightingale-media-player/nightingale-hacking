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

// The global progress filter, set in SBInitialize and released in SBUnitialize
var gProgressFilter = null;

// This functionality should be encapsulated into its own xbl.
function NumItemsPoll()
{
  try
  {
    // Display the number of items in the currently viewed playlist.
    var tree_ref = "";
    var display_string = "";
    // thePlaylistTree is non-null when a playlist is showing.
    if ( thePlaylistTree )
    {
      tree_ref = thePlaylistTree.getAttribute( "ref" );
    }
    else if ( theWebPlaylistQuery )
    {
      // If there is a web playlist query, then we can pop the webplaylist.
      var mediafound = "Media Found"; 
      try {
        mediafound = theSongbirdStrings.getString("faceplate.mediafound");
      } catch(e) { /* ignore error, we have a default string*/ }
      var pct = parseInt( SBDataGetIntValue( "webplaylist.current" ) * 100 / SBDataGetIntValue( "webplaylist.total" ) );
      if ( pct < 100 )
      {
        display_string = mediafound + " " + pct + "%";
      }
      else
      {
        tree_ref = theWebPlaylist.ref;
      }
    }
    
    if ( tree_ref.length )
    {
      var rows = thePollPlaylistService.getRefRowCount( tree_ref );
      if ( rows > 0 )
      {
        var items = "items";
        try {
          items = theSongbirdStrings.getString("faceplate.items");
        } catch(e) { /* ignore error, we have a default string*/ }
        display_string = rows + " " + items;
      }
    }
    
    theNumPlaylistItemsRemote.stringValue = display_string;
  }
  catch ( err )
  {
    alert( "PFU - " + err );
  }
}    

//
// Mainwin Initialization
//
var thePollPlaylistService = null;
var theWebPlaylist = null;
var theWebPlaylistQuery = null;
var theNumPlaylistItemsRemote = SB_NewDataRemote( "playlist.numitems", null );
var theDefaultUrlOverride = null;

function SBInitialize()
{

  try
  {
    //Whatever migration is required between version, this function takes care of it.
    SBMigrateDatabase();
  }
  catch(e) { }

  const BROWSER_FILTER_CONTRACTID =
    "@mozilla.org/appshell/component/browser-status-filter;1";
  const nsIWebProgress = Components.interfaces.nsIWebProgress;

  dump("SBInitialize *** \n");
  
  try {
    fixWindow("songbird_top", "mainwin_app_title"); 
  }
  catch(e) { }

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
    initJumpToFileHotkey();
    initFaceplateButton();

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

    // Install listeners on the main pane. - This is the browser xul element from mainwin.xul
    var theMainPane = document.getElementById("frame_main_pane");
    if (!mainpane_listener_set)
    {
      mainpane_listener_set = false;
      if (theMainPane.addEventListener) {
        // Initialize the global progress filter
        if (!gProgressFilter) {
          gProgressFilter = Components.classes[BROWSER_FILTER_CONTRACTID]
                                      .createInstance(nsIWebProgress);
        }
        // And add our own filter to it
        gProgressFilter.addProgressListener(sbWebProgressListener,
                                            nsIWebProgress.NOTIFY_ALL);
        theMainPane.addProgressListener(gProgressFilter);

        // Trap clicks on links with target=_blank or _new
        // and launch them in the default browser.
        // Note: _new isn't valid.. but seems to be used frequently
        // so we might as well support it.
        theMainPane.addEventListener("click", onBrowserClick, true);

      }
    }
    
    // Look at all these ugly hacks that need to go away.  (sigh)
    var theServiceTree = document.getElementById( 'frame_servicetree' );
    theServiceTree.init(theMainPane, theDefaultUrlOverride);
    theServiceTree.onPlaylistHide = onBrowserPlaylistHide;
    theServiceTree.onPlaylistDefaultCommand = onServiceTreeCommand;
    
    document.__THESERVICETREE__ = theServiceTree;

    document.__SEARCHWIDGET__ = document.getElementById( "search_widget" );

    theWebPlaylist = document.getElementById( "playlist_web" );
    // hack, to let play buttons find the visible playlist if needed
    document.__CURRENTWEBPLAYLIST__ = theWebPlaylist;
    theWebPlaylist.addEventListener( "playlist-play", onPlaylistPlay, true );
    theWebPlaylist.addEventListener( "command", onPlaylistContextMenu, false );  // don't force it!
    theWebPlaylist.setDnDSourceTracker(sbDnDSourceTracker);
    
    try {
      var metadataBackscanner = Components.classes["@songbirdnest.com/Songbird/MetadataBackscanner;1"]
        .getService(Components.interfaces.sbIMetadataBackscanner);
      metadataBackscanner.start();
      
    } catch(err) { dump(err); }
    
    // Poll the playlist source every 500ms to drive the display update (STOOOOPID! This should be encapsulated)
    thePollPlaylistService = new sbIPlaylistsource();
    theNumPlaylistItemsRemote.stringValue = "";
    setInterval( NumItemsPoll, 500 );
    
  }
  catch(err)
  {
    alert("songbird_hack.js - SBInitialize - " +  err);
  }
}

function SBUninitialize()
{
  shutdownFaceplateButton();
  
  window.removeEventListener("keydown", checkAltF4, true);
  
  var mainPane = document.getElementById("frame_main_pane");
  mainPane.removeProgressListener(gProgressFilter);

  var webPlaylist = document.getElementById("playlist_web");
  webPlaylist.removeEventListener("playlist-play", onPlaylistPlay, true);
  webPlaylist.removeEventListener("command", onPlaylistContextMenu, false);

  document.__THESERVICETREE__ = null;
  document.__SEARCHWIDGET__ = null;
  document.__CURRENTWEBPLAYLIST__ = null;

  // Make sure to destroy the global progress filter
  if (gProgressFilter)
    gProgressFilter = null;

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
  theWebPlaylist = null;
  theWebPlaylistQuery = null;
  theNumPlaylistItemsRemote = null;
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
