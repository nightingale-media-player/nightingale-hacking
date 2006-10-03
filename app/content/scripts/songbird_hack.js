// JScript source code
/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
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

// Useful constants
const CORE_WINDOWTYPE = "Songbird:Core";

const PREF_BONES_SELECTED = "general.bones.selectedMainWinURL";
const PREF_FEATHERS_SELECTED = "general.skins.selectedSkin";

const BONES_DEFAULT_URL = "chrome://rubberducky/content/xul/mainwin.xul";
const FEATHERS_DEFAULT_NAME = "rubberducky";

const PREFS_SERVICE_CONTRACTID = "@mozilla.org/preferences-service;1";
const nsIPrefBranch2 = Components.interfaces.nsIPrefBranch2;

// The global progress filter, set in SBInitialize and released in SBUnitialize
var gProgressFilter = null;

try
{
const LOAD_FLAGS_BYPASS_HISTORY = 64;

// okay
var thePlaylistReader = null;

// Hooray for event handlers!
function myPlaybackEvent( key, value )
{
}

// NOW we have the playlist playback service!
var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                      .getService(Components.interfaces.sbIPlaylistPlayback);

function getPlatformString()
{
  try {
    var sysInfo =
      Components.classes["@mozilla.org/system-info;1"]
                .getService(Components.interfaces.nsIPropertyBag2);
    return sysInfo.getProperty("name");                                          
  }
  catch (e) {
    dump("System-info not available, trying the user agent string.\n");
    var user_agent = navigator.userAgent;
    if (user_agent.indexOf("Windows") != -1)
      return "Windows_NT";
    else if (user_agent.indexOf("Mac OS X") != -1)
      return "Darwin";
    else if (user_agent.indexOf("Linux") != -1)
      return "Linux";
    return "";
  }
}

/**
 * Adapted from nsUpdateService.js.in. Need to replace with dataremotes.
 */
function getPref(aFunc, aPreference, aDefaultValue) {
  var prefs = 
    Components.classes[PREFS_SERVICE_CONTRACTID].getService(nsIPrefBranch2);
  try {
    return prefs[aFunc](aPreference);
  }
  catch (e) { }
  return aDefaultValue;
}
function setPref(aFunc, aPreference, aValue) {
  var prefs = 
    Components.classes[PREFS_SERVICE_CONTRACTID].getService(nsIPrefBranch2);
  return prefs[aFunc](aPreference, aValue);
}

var theSongbirdStrings = document.getElementById( "songbird_strings" );

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

function checkAltF4(evt)
{
  if (evt.keyCode == 0x73 && evt.altKey) 
  {
    evt.preventDefault();
    quitApp();
  }
}

/**
* Convert a string containing binary values to hex.
*/
function binaryToHex(input)
{
  var result = "";
  
  for (var i = 0; i < input.length; ++i) 
  {
    var hex = input.charCodeAt(i).toString(16);
  
    if (hex.length == 1)
      hex = "0" + hex;
  
    result += hex;
  }
  
  return result;
}

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

function SBInitialize()
{
  const BROWSER_FILTER_CONTRACTID =
    "@mozilla.org/appshell/component/browser-status-filter;1";
  const nsIWebProgress = Components.interfaces.nsIWebProgress;

  dump("SBInitialize *** \n");
  
  try {
    fixOSXWindow("songbird_top", "mainwin_app_title");
  }
  catch(e) { }

  window.focus();

  const MediaLibrary = new Components.Constructor("@songbirdnest.com/Songbird/MediaLibrary;1", "sbIMediaLibrary");
  const PlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");
  const PlaylistReaderManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistReaderManager;1", "sbIPlaylistReaderManager");
  thePlaylistReader = (new PlaylistReaderManager()).QueryInterface(Components.interfaces.sbIPlaylistReaderManager);

  try
  {
    onWindowLoadSizeAndPosition();
    setMinMaxCallback();
    SBInitMouseWheel();
    initJumpToFileHotkey();

    if (window.addEventListener)
      window.addEventListener("keydown", checkAltF4, true);

    // Make sure we have a fake database in which to play
    var aDBQuery = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"];
    if (aDBQuery)
    {
      aDBQuery = aDBQuery.createInstance();
      aDBQuery = aDBQuery.QueryInterface(Components.interfaces.sbIDatabaseQuery);
      aDBQuery.setAsyncQuery(false);
      aDBQuery.setDatabaseGUID("testdb-0000");
      aDBQuery.addQuery("create table test (idx integer primary key autoincrement, url text, name text, tim text, artist text, album text, genre text)");
      aDBQuery.addQuery("create index testindex on test(idx, url, name, tim, artist, album, genre)");
      
      var ret = aDBQuery.execute();
      
      // If it actually worked, that means we created the database
      // ask the user if they would like to fill their empty bucket.
      if ( ret == 0 )
      {
        theMediaScanIsOpen.boolValue = true;
        setTimeout( SBScanMedia, 1000 );
      }
    }
    
    // Install listeners on the main pane. - This is the browser xul element from mainwin.xul
    var theMainPane = document.getElementById("frame_main_pane");
    if (!mainpane_listener_set)
    {
      mainpane_listener_set = false;
      if (theMainPane.addEventListener) {
        theMainPane.addEventListener("DOMContentLoaded", onMainPaneLoad, false);
        theMainPane.addEventListener("unload", onMainPaneUnload, true);
        
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
    
    
    var theServiceTree = document.getElementById( 'frame_servicetree' );
    theServiceTree.init(theMainPane);
    theServiceTree.setPlaylistPopup(document.getElementById( "service_popup_playlist" ));
    theServiceTree.setSmartPlaylistPopup(document.getElementById( "service_popup_smart" ));
    //theServiceTree.setDefaultPopup(document.getElementById( "?" ));
    theServiceTree.setNotAnItemPopup(document.getElementById( "service_popup_none" ));
    theServiceTree.onPlaylistHide = onBrowserPlaylistHide;
    document.__THESERVICETREE__ = theServiceTree;

    document.__SEARCHWIDGET__ = document.getElementById( "search_widget" );

    theWebPlaylist = document.getElementById( "playlist_web" );
    // hack, to let play buttons find the visible playlist if needed
    document.__CURRENTWEBPLAYLIST__ = theWebPlaylist;
    theWebPlaylist.addEventListener( "playlist-play", onPlaylistPlay, true );
//    theWebPlaylist.addEventListener( "playlist-noplaylist", onPlaylistNoPlaylist, true );
    
// no!    theWebPlaylist.addEventListener( "playlist-edit", onPlaylistEdit, true );
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
    
//    
// Let's test loading all sorts of random urls as if they were playlists!    
//    var the_url = "ftp://ftp.openbsd.org/pub/OpenBSD/songs";
//    var the_url = "http://www.blogotheque.net/mp3/";
//    var the_url = "file:///c:\\vice.html";
//    var the_url = "http://odeo.com/channel/38104/rss";
//    var the_url = "http://takeyourmedicinemp3.blogspot.com/atom.xml";
//    var success = thePlaylistReader.autoLoad(the_url, "songbird", gPPS.convertURLToDisplayName( the_url ), "http", the_url, "", null);

/*
    try
    {
      const MetadataManager = new Components.Constructor("@songbirdnest.com/Songbird/MetadataManager;1", "sbIMetadataManager");
      var aMetadataManager = new MetadataManager();
      var aMetadataHandler = aMetadataManager.getHandlerForMediaURL("http://www.morphius.com/label/mp3/Labtekwon_RealEmcee.mp3");
      //aMetadataHandler = aMetadataManager.getHandlerForMediaURL("file://c:/junq/01_The_Gimp_Sometimes.mp3");
      var retval = aMetadataHandler.Read();
      
      function PollMetadata( )
      {
        if ( aMetadataHandler.isExecuting() )
        {
          setTimeout( PollMetadata, 500 );
        }
        else
        {
          var text = "";
          const keys = new Array("title", "length", "album", "artist", "genre", "year", "composer");
          for ( var i in keys )
          {
            text += keys[ i ] + ": " + aMetadataHandler.values.getValue( keys[ i ] ) + "\n";
          }
          alert( text );
        }
      }
      setTimeout( PollMetadata, 500 );
    }
    catch(err)
    {
      alert("songbird_hack.js - SBInitialize - " +  err);
    }
*/

  }
  catch(err)
  {
    alert("songbird_hack.js - SBInitialize - " +  err);
  }
}

function SBUninitialize()
{
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
}

//
// XUL Event Methods
//

function switchFeathers(aFeathersName)
{
  // Figure out if we're being asked to switch to what we already are
  var currentFeathers = getPref("getCharPref", PREF_FEATHERS_SELECTED,
                                FEATHERS_DEFAULT_NAME);
  if (currentFeathers == aFeathersName)
    return;

  // Change the feathers (XUL skin) -- this only changes colors on the
  // currently loaded windows, not images. Hence the magic below.
  setPref("setCharPref", PREF_FEATHERS_SELECTED, aFeathersName);  

  // Get mainwin URL
  var mainWinURL = getPref("getCharPref", PREF_BONES_SELECTED,
                           BONES_DEFAULT_URL);
  
  // Save our current values before flipping out.
  onWindowSaveSizeAndPosition();
  
  // Open the new window
  var chromeFeatures =
    "chrome,modal=no,toolbar=no,popup=no,titlebar=no,resizable=no";
  var newMainWin = window.open(mainWinURL, "", chromeFeatures);
  newMainWin.focus();

  // Kill this window
  onExit(true);
}

var URL = SB_NewDataRemote( "faceplate.play.url", null );
var thePlaylistIndex = SB_NewDataRemote( "playlist.index", null );
var seen_playing = SB_NewDataRemote( "faceplate.seenplaying", null );
var theTitleText = SB_NewDataRemote( "metadata.title", null );
var theArtistText = SB_NewDataRemote( "metadata.artist", null );
var theAlbumText = SB_NewDataRemote( "metadata.album", null );
var theStatusText = SB_NewDataRemote( "faceplate.status.text", null );
var theStatusStyle = SB_NewDataRemote( "faceplate.status.style", null );

// Help
function onHelp()
{
  var helpitem = document.getElementById("help.topics");
  onMenu(helpitem);
}

var theCurrentTrackInterval = null;
function onCurrentTrack()
{
  if ( theCurrentTrackInterval )
  {
    clearInterval( theCurrentTrackInterval );
  }

  var guid;
  var table;
  var ref = SBDataGetStringValue("playing.ref");
  if (ref != "") {
    source_ref = ref;
    var source = new sbIPlaylistsource();
    guid = source.getRefGUID( ref );
    table = source.getRefTable( ref );
  } else {
    source_ref = "NC:songbird_library";
    guid = "songbird";
    table = "library";
  }
  
  if ( thePlaylistTree ) {
    var curplaylist = document.__CURRENTPLAYLIST__;
    if (curplaylist && curplaylist.ref == source_ref) {
      curplaylist.syncPlaylistIndex(true);
      return;
    }
  }
  
  var theServiceTree = document.getElementById( 'frame_servicetree' );
  if (guid == "songbird" && table == "library") { 
    theServiceTree.launchServiceURL( "chrome://songbird/content/xul/playlist_test.xul?library" );
  }
  else 
  {
    theServiceTree.launchServiceURL( "chrome://songbird/content/xul/playlist_test.xul?" + table+ "," + guid);
  } 
  theCurrentTrackInterval = setInterval( onCurrentTrack, 500 );
}

function onNextService()
{
  // this could, down the road become integer based for >2 services
  SBDataToggleBoolValue("faceplate.state");
}

function onServiceTreeCommand( theEvent )
{
  if ( theEvent && theEvent.target )
  {
    // These attribs get set when the menu is popped up on a playlist.
    var label = theEvent.target.parentNode.getAttribute( "label" );
    var guid = theEvent.target.parentNode.getAttribute( "sb_guid" );
    var table = theEvent.target.parentNode.getAttribute( "sb_table" );
    var type = theEvent.target.parentNode.getAttribute( "sb_type" );
    var base_type = theEvent.target.parentNode.getAttribute( "sb_base_type" );
    switch ( theEvent.target.id )
    {
      case "service_popup_new":
        SBNewPlaylist();
      break;
      case "service_popup_new_smart":
        SBNewSmartPlaylist();
      break;
      case "service_popup_new_remote":
        SBSubscribe( "", "", "", "" );
      break;
      
      case "playlist_context_smartedit":
        if ( base_type == "smart" )
        {
          if ( guid && guid.length > 0 && table && table.length > 0 )
          {
            SBNewSmartPlaylist( guid, table );
          }
        }
        if ( base_type == "dynamic" )
        {
          if ( guid && guid.length > 0 && table && table.length > 0 )
          {
            SBSubscribe( type, guid, table, label );
          }
        }
      break;
      
      case "playlist_context_rename":
        if ( guid && guid.length > 0 && table && table.length > 0 )
        {
          onServiceEdit();
        }
      break;
      
      case "playlist_context_remove":
        if ( guid && guid.length > 0 && table && table.length > 0 )
        {
          // Assume we'll need this...
          const PlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");
          var aPlaylistManager = new PlaylistManager();
          aPlaylistManager = aPlaylistManager.QueryInterface(Components.interfaces.sbIPlaylistManager);
          var aDBQuery = new sbIDatabaseQuery();
          
          aDBQuery.setAsyncQuery(false);
          aDBQuery.setDatabaseGUID(guid);

          switch ( base_type )
          {
            case "simple":
              aPlaylistManager.deleteSimplePlaylist(table, aDBQuery);
              break;
            case "dynamic":
              aPlaylistManager.deleteDynamicPlaylist(table, aDBQuery);
              break;
            case "smart":
              aPlaylistManager.deleteSmartPlaylist(table, aDBQuery);
              break;
            default:
              aPlaylistManager.deletePlaylist(table, aDBQuery);
              break;
          }
        }
      break;
    }
  }
}

var theServiceTreeScanItems = new Array();
var theServiceTreeScanCount = 0;
function SBScanServiceTreeNewEntryEditable()
{
  var theServiceTree = document.getElementById( "frame_servicetree" );
  theServiceTreeScanItems.length = 0;
  theServiceTreeScanCount = 0;
  
  // Go get all the current service tree urls.
  var theServiceTree_tree = theServiceTree.tree;
  for ( var i = 0; i < theServiceTree_tree.view.rowCount; i++ )
  {
    theServiceTreeScanItems.push( theServiceTree_tree.contentView.getItemAtIndex( i ).getAttribute( "url" ) );
  }
}

function SBScanServiceTreeNewEntryStart()
{
  setTimeout( SBScanServiceTreeNewEntryCallback, 500 );
}

function SBScanServiceTreeNewEntryCallback()
{
  var theServiceTree = document.getElementById( "frame_servicetree" );
  
  if ( ++theServiceTreeScanCount > 10 )
  {
    return; // don't loop more than 1 second.
  }
  
  // Go through all the current service tree items.
  var done = false;
  var theServiceTree_tree = theServiceTree.tree;
  for ( var i = 0; i < theServiceTree_tree.view.rowCount; i++ )
  {
    var found = false;
    var url = theServiceTree_tree.contentView.getItemAtIndex( i ).getAttribute( "url" );
    // Match them against the scan items
    for ( var j = 0; j < theServiceTreeScanItems.length; j++ )
    {
      if ( url == theServiceTreeScanItems[ j ] )
      {
        found = true;
        break;
      }
    }
    // Right now, only songbird playlists are editable.
    if ( ( ! found ) && ( url.indexOf( ",songbird" ) != -1 ) )
    {
/*    
      // This must be the new one?
      theServiceTree_tree.view.selection.currentIndex = i;
      
      // HACK: flag to prevent the empty playlist from launching on select
      theServiceTree_tree.newPlaylistCreated = true;
      theServiceTree_tree.view.selection.select( i );
      theServiceTree_tree.newPlaylistCreated = false;
*/      
      onServiceEdit(i);
      done = true;
      break;
    }
  }
  if ( ! done )
  {
    setTimeout( SBScanServiceTreeNewEntryCallback, 100 );
  }
}

function onServiceEdit( index )
{
  try
  {
    // in case onpopuphiding was not called (ie, ctrl-n + ctrl-n)
    if (isServiceEditShowing) {
      // validate changes in the previous popup, if any
      onServiceEditChange();
      // reset, this is important
      isServiceEditShowing = false; 
    }
    var theServiceTree = document.getElementById( "frame_servicetree" );
    if ( theServiceTree)
    {
      var theServiceTree_tree = theServiceTree.tree;
      if ( index == null ) // Optionally specify which index to edit.
        index = theServiceTree_tree.currentIndex;
      if ( theServiceTree_tree && ( index > -1 ) )
      {
        var column = theServiceTree_tree.columns["frame_service_tree_label"];
        var cell_text = theServiceTree_tree.view.getCellText( index, column );
        var cell_url =  theServiceTree_tree.view.getCellText( index, theServiceTree_tree.columns["url"] );

        // This is nuts!
        var text_x = {}, text_y = {}, text_w = {}, text_h = {}; 
        theServiceTree_tree.treeBoxObject.
          getCoordsForCellItem( index, column, "text", text_x, text_y , text_w , text_h );
        var cell_x = {}, cell_y = {}, cell_w = {}, cell_h = {}; 
        theServiceTree_tree.treeBoxObject.
          getCoordsForCellItem( index, column, "cell", cell_x, cell_y , cell_w , cell_h );
        var image_x = {}, image_y = {}, image_w = {}, image_h = {}; 
        theServiceTree_tree.treeBoxObject.
          getCoordsForCellItem( index, column, "image", image_x, image_y , image_w , image_h );
        var twisty_x = {}, twisty_y = {}, twisty_w = {}, twisty_h = {}; 
        theServiceTree_tree.treeBoxObject.
          getCoordsForCellItem( index, column, "twisty", twisty_x, twisty_y , twisty_w , twisty_h );
          
        var out_x = {}, out_y = {}, out_w = {}, out_h = {};
        out_x = text_x;
        out_y = cell_y;
        out_w.value = cell_w.value - twisty_w.value - image_w.value;
        out_h = cell_h;
        
        // Then pop the edit box to the bounds of the cell.
        var theEditPopup = document.getElementById( "service_edit_popup" );
        var theEditBox = document.getElementById( "service_edit" );
        var extra_x = 3; // Why do I have to give it extra?  What am I calculating wrong?
        var extra_y = 7; // Why do I have to give it extra?  What am I calculating wrong?
        if (getPlatformString() == "Darwin")
          extra_y -= 1;  // And an extra pixel on the mac.  Great.
        var less_w  = 5;
        var less_h  = -2;
        var pos_x = extra_x + theServiceTree_tree.boxObject.screenX + out_x.value;
        var pos_y = extra_y + theServiceTree_tree.boxObject.screenY + out_y.value;
        theEditBox.setAttribute( "hidden", "false" );
        theEditPopup.showPopup( theServiceTree_tree, pos_x, pos_y, "popup" );
        theEditPopup.sizeTo( out_w.value - less_w, out_h.value - less_h ); // increase the width to the size of the cell.
        theEditBox.value = cell_text;
        theEditBox.index = index;
        theEditBox.url = cell_url;
        theEditBox.focus();
        theEditBox.select();
        isServiceEditShowing = true;
      }
    }
  }
  catch ( err )
  {
    alert( "onServiceEdit - " + err );
  }
}

function getItemByUrl( tree, url )
{
  var retval = null;

  for ( var i = 0; i < tree.view.rowCount; i++ )
  {
    var cell_url =  tree.view.getCellText( i, tree.columns["url"] );
    
    if ( cell_url == url )
    {
      retval = tree.contentView.getItemAtIndex( i );
      break;
    }
  }
  
  return retval;
}

function onServiceEditChange( )
{
  try
  {
    var theServiceTree = document.getElementById( "frame_servicetree" );
    if ( theServiceTree)
    {
      var theServiceTree_tree = theServiceTree.tree;
      if ( theServiceTree_tree )
      {
        var theEditBox = document.getElementById( "service_edit" );

        var element = getItemByUrl( theServiceTree_tree, theEditBox.url );
        
        if ( element == null ) 
          return; // ??
        
        var properties = element.getAttribute( "properties" ).split(" ");
        if ( properties.length >= 5 && properties[0] == "playlist" )
        {
          var table = properties[ 1 ];
          var guid = properties[ 2 ];
          var type = properties[ 3 ];
          var base_type = properties[ 4 ];
          
          const PlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");
          var aPlaylistManager = (new PlaylistManager()).QueryInterface(Components.interfaces.sbIPlaylistManager);
          var aDBQuery = new sbIDatabaseQuery();
          
          aDBQuery.setAsyncQuery(false);
          aDBQuery.setDatabaseGUID(guid);
          
          var playlist = null;
          
          // How do I edit a table's readable name?  I have to know what type it is?  Ugh.
          switch ( base_type )
          {
            case "simple":  playlist = aPlaylistManager.getSimplePlaylist(table, aDBQuery);  break;
            case "dynamic": playlist = aPlaylistManager.getDynamicPlaylist(table, aDBQuery); break;
            case "smart":   playlist = aPlaylistManager.getSmartPlaylist(table, aDBQuery);   break;
            default:        playlist = aPlaylistManager.getPlaylist(table, aDBQuery);        break;
          }
          
          if(playlist)
          {
            var theEditBox = document.getElementById( "service_edit" );
            var strReadableName = theEditBox.value;
            playlist.setReadableName(strReadableName);
          }
        }      
        HideServiceEdit( null );
      }
    }
  }
  catch ( err )
  {
    alert( "onServiceEditChange\n\n" + err );
  }
}

var hasServiceEditBeenModified = false;
function onServiceEditKeypress( evt )
{
  switch ( evt.keyCode )
  {
    case 27: // Esc
      HideServiceEdit( null );
      break;
    case 13: // Return
      onServiceEditChange();
      break;
    default:
      hasServiceEditBeenModified = true;
  }
}

function onServiceEditHide( evt ) {
  isServiceEditShowing = false;
}

var isServiceEditShowing = false;
function HideServiceEdit( evt )
{
  try
  {
    if (hasServiceEditBeenModified && evt)
    {
      hasServiceEditBeenModified = false;
      onServiceEditChange();
    }

    if ( isServiceEditShowing )
    {
      var theEditBox = document.getElementById( "service_edit" );
      theEditBox.setAttribute( "hidden", "true" );
      var theEditPopup = document.getElementById( "service_edit_popup" );
      theEditPopup.hidePopup();
      isServiceEditShowing = false;
      
    }
  }
  catch ( err )
  {
    alert( "HideServiceEdit\n\n" + err );
  }
}

var theCanGoBackData = SB_NewDataRemote( "browser.cangoback", null );
var theCanGoFwdData = SB_NewDataRemote( "browser.cangofwd", null );
var theCanAddToPlaylistData = SB_NewDataRemote( "browser.canplaylist", null );
var theBrowserUrlData = SB_NewDataRemote( "browser.url.text", null );
var theBrowserImageData = SB_NewDataRemote( "browser.url.image", null );
var theBrowserUriData = SB_NewDataRemote( "browser.uri", null );
var theShowWebPlaylistData = SB_NewDataRemote( "browser.playlist.show", null );

var sbWebProgressListener = {

  QueryInterface : function(aIID) {
    if (!aIID.equals(Components.interfaces.nsIWebProgressListener) &&
        !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
  },

  onStateChange : function( aWebProgress, aRequest, aState, aStatus ) 
  {
    const nsIWebProgressListener =
      Components.interfaces.nsIWebProgressListener;

    const NS_ERROR_UNKNOWN_HOST = 0x804B001E;

    const URI_PAGE_CANNOT_LOAD =
      "chrome://songbird/content/html/cannot_load.html";

    if (aState & nsIWebProgressListener.STATE_START) {
      // Start the spinner if necessary
      thePaneLoadingData.boolValue = true;
    }
    else if (aState & nsIWebProgressListener.STATE_STOP) {
      // If we have an error then show the "Page Not Found" page
      if (aStatus == NS_ERROR_UNKNOWN_HOST) {
        var serviceTree = document.getElementById("frame_servicetree");
        serviceTree.launchURL(URI_PAGE_CANNOT_LOAD);
      }
      // Stop the spinner if necessary
      thePaneLoadingData.boolValue = false;
    }
  },
  
  onStatusChange : function( aWebProgress, aRequest, aLocation ) 
  {
  },
  
  onProgressChange : function( aWebProgress, aRequest, aLocation ) 
  {
  },
  
  onSecurityChange : function( aWebProgress, aRequest, aLocation ) 
  {
  },
  
  onLocationChange : function( aWebProgress, aRequest, aLocation ) 
  {
    try
    {
      // Set the value in the text box (shown or not)
      var theServiceTree = document.getElementById( "frame_servicetree" );
      var theMainPane = document.getElementById( "frame_main_pane" );
      var cur_uri = aLocation.asciiSpec;
      if ( aRequest && aRequest.name )    
      {
        // Set the box
        theBrowserUriData.stringValue = cur_uri;
        theBrowserUrlData.stringValue = SBGetServiceFromUrl( cur_uri ) ;
        var image = SBGetServiceImageFromUrl( cur_uri );
        if ( image.length )
        {
          theBrowserImageData.stringValue = image;
        }
        
        // Set the buttons based on the session history.
        if ( theMainPane.webNavigation.sessionHistory )
        {
          // Check the buttons
          theCanGoBackData.boolValue = theMainPane.webNavigation.canGoBack;
          theCanGoFwdData.boolValue = theMainPane.webNavigation.canGoForward;
        }
        else
        {
          // Error?
        }

/*        
        // Hide or show the HTML box based upon whether or not the loaded page is .xul (lame heuristic)
        var theHTMLBox = document.getElementById( "frame_main_pane_html" );
        if ( theHTMLBox )
        {
          if ( cur_uri.indexOf(".xul") != -1 )
          {
            if ( SBDataGetIntValue( "option.htmlbar" ) == 0 )
            {
              theHTMLBox.setAttribute( "hidden", "true" );
            }
          }
          else
          {
            theHTMLBox.setAttribute( "hidden", "false" );
          }
        }
*/        
      }
      if ( ! theServiceTree.urlFromServicePane )
      {
        // Clear the service tree selection (asynchronously?  is this from out of thread?)
        setTimeout( "document.getElementById( 'frame_servicetree' ).tree.view.selection.currentIndex = -1;" +
                    "document.getElementById( 'frame_servicetree' ).tree.view.selection.clearSelection();",
                    50 );
      }
      theServiceTree.urlFromServicePane = false;
      
      // Clear the playlist tree variable so we are not confused.
      thePlaylistTree = null;
      theLibraryPlaylist = null;
      // hack, to let play buttons find the visible playlist if needed
      document.__CURRENTPLAYLIST__ = null;
      
      // Clear the tracking variable
      mainpane_listener_set = false;
      
      // Disable the "add to playlist" button until we see that there is anything to add.
      theCanAddToPlaylistData.boolValue = false;
      onBrowserPlaylistHide();

      // Clear the playlist tree variable so we are not confused.
      thePlaylistTree = null;
      theLibraryPlaylist = null;
      // hack, to let play buttons find the visible playlist if needed
      document.__CURRENTPLAYLIST__ = null;
      
      // Nothing in the status text
      theStatusText.stringValue = "";
      
      theServiceTree.current_url = cur_uri;
    }
    catch ( err )
    {
      alert( "onLocationChange\n\n" + err );
    }
  }
}; // sbWebProgressListener definition

// onBrowserBack
function onBrowserBack()
{
  // Disable the "add to playlist" button until we see that there is anything to add.
  theCanAddToPlaylistData.boolValue = false;
  onBrowserPlaylistHide();
  var theMainPane = document.getElementById( "frame_main_pane" );
  mainpane_listener_set = false;
  theMainPane.goBack();
}

// onBrowserFwd
function onBrowserFwd()
{
  // Disable the "add to playlist" button until we see that there is anything to add.
  theCanAddToPlaylistData.boolValue = false;
  onBrowserPlaylistHide();
  var theMainPane = document.getElementById( "frame_main_pane" );
  mainpane_listener_set = false;
  theMainPane.goForward();
}

// onBrowserRefresh
function onBrowserRefresh()
{
  try
  {
    var theMainPane = document.getElementById( "frame_main_pane" );
    mainpane_listener_set = false;
    theMainPane.reload();
  }
  catch( err )
  {
    alert( err );
  }
}

// onBrowserStop
function onBrowserStop()
{
  try
  {
    //SB_LOG("songbird_hack.js", "onBrowserStop");
    var theMainPane = document.getElementById( "frame_main_pane" );
    theMainPane.stop();
    mainpane_listener_set = false;
    onMainPaneLoad();
  }
  catch( err )
  {
    alert( err );
  }
}

// onBrowserHome
function onBrowserHome()
{
  var theServiceTree = document.getElementById( 'frame_servicetree' );
  var defaultHomepage = "http://www.songbirdnest.com/birdhouse/";
  
  try {
    var homepage;
    var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                getService(Components.interfaces.nsIPrefBranch);
    
    if (prefs.getPrefType("browser.startup.homepage") == prefs.PREF_STRING){
      homepage = prefs.getCharPref("browser.startup.homepage");
    }
    
    if(homepage && homepage != "")
      defaultHomepage = homepage;
      
    theServiceTree.launchServiceURL( defaultHomepage );
  } catch(e) {}
}

// onBrowserBookmark
function onBrowserBookmark()
{
  try
  {
    alert( "Uh... there's no bookmarks service in XULRunner.  We'll implement this soon." );
/*  
    var url = SBDataGetStringValue( "browser.uri" );
    alert(url + "\n" + Components.interfaces.nsIBookmarksService);
    var bmarks = Components.classes["@mozilla.org/browser/bookmarks-service;1"].getService();
    bmarks.QueryInterface(Components.interfaces.nsIBookmarksService);
    bmarks.addBookmarkImmediately(url,url,0,null);
    alert(url);
*/    
  }  
  catch ( err )
  {
    alert( "onBrowserBookmark\n" + err );
  }
}



// onBrowserClick
// Catches links with target=_blank and opens them with the default browser.
// Added as a click listener to theMainPane.
function onBrowserClick(evt)
{
  if (evt.target && evt.button == 0 && evt.target.tagName == "A") {

    //dump("\n\n\nClick in main pane: "
    //    + "tag " + evt.target.tagName + "\n"
    //    + "href "  + evt.target.href + "\n" 
    //    + "target " + evt.target.target
    //    + "\n\n\n");  
        
    var is_media = gPPS.isMediaURL( evt.target.href );
    var is_playlist = gPPS.isPlaylistURL( evt.target.href );
    if (!is_media && !is_playlist && (evt.target.target == "_blank" || evt.target.target == "_new")) {
      var externalLoader = (Components
            .classes["@mozilla.org/uriloader/external-protocol-service;1"]
            .getService(Components.interfaces.nsIExternalProtocolService));
      var nsURI = (Components
            .classes["@mozilla.org/network/io-service;1"]
            .getService(Components.interfaces.nsIIOService)
            .newURI(evt.target.href, null, null));
      externalLoader.loadURI(nsURI, null);
    }
  }
}


var SBWebPlaylistCommands = 
{
  m_Playlist: null,
  
  m_Query: null,

  m_Ids: new Array
  (
    "library_cmd_play",
//    "library_cmd_edit",
    "library_cmd_download",
    "library_cmd_subscribe",
    "library_cmd_addtoplaylist",
    "library_cmd_addtolibrary",
    "*separator*",
    "library_cmd_showdlplaylist"
//    "library_cmd_burntocd"
//    "library_cmd_device"
  ),
  
  m_Names: new Array
  (
    "&command.play",
//    "&command.edit",
    "&command.download",
    "&command.subscribe",
    "&command.addtoplaylist",
    "&command.addtolibrary",
    "*separator*",
    "&command.showdlplaylist"
//    "&command.burntocd"
//    "&command.device"
  ),
  
  m_Tooltips: new Array
  (
    "&command.tooltip.play",
//    "&command.tooltip.edit",
    "&command.tooltip.download",
    "&command.tooltip.subscribe",
    "&command.tooltip.addtoplaylist",
    "&command.tooltip.addtolibrary",
    "*separator*",
    "&command.tooltip.showdlplaylist"
//    "&command.tooltip.burntocd"
//    "&command.tooltip.device"
  ),

  getNumCommands: function()
  {
    if ( 
        ( this.m_Tooltips.length != this.m_Ids.length ) ||
        ( this.m_Names.length != this.m_Ids.length ) ||
        ( this.m_Tooltips.length != this.m_Names.length )
        )
    {
      alert( "PlaylistCommands - Array lengths do not match!" );
      return 0;
    }
    return this.m_Ids.length;
  },

  getCommandId: function( index )
  {
    if ( index >= this.m_Ids.length )
    {
      return "";
    }
    return this.m_Ids[ index ];
  },

  getCommandText: function( index )
  {
    if ( index >= this.m_Names.length )
    {
      return "";
    }
    return this.m_Names[ index ];
  },

  getCommandFlex: function( index )
  {
    if ( this.m_Ids[ index ] == "*separator*" ) return 1;
    return 0;
  },

  getCommandToolTipText: function( index )
  {
    if ( index >= this.m_Tooltips.length )
    {
      return "";
    }
    return this.m_Tooltips[ index ];
  },

  getCommandEnabled: function( index )
  {
    // First time, make a query to be able to check for the existence of the 
    // download playlist
    if ( this.m_Query == null )
    {
      // Find the guid and table for the download playlist.
      var guid = "downloadDB";
      var table = "download";
      var deviceManager = Components.classes["@songbirdnest.com/Songbird/DeviceManager;1"].
                                      getService(Components.interfaces.sbIDeviceManager);
      if (deviceManager)
      {
        var downloadCategory = 'Songbird Download Device';
        if (deviceManager.hasDeviceForCategory(downloadCategory))
        {
          var downloadDevice =
            deviceManager.getDeviceByCategory(downloadCategory);
          SBDownloadCommands.m_Device = downloadDevice;
          guid = downloadDevice.getContext('');
          table = downloadDevice.getDownloadTable('');
        }
      }
      
      // Setup a query to execute to test the existence of the table
      this.m_Query = new sbIDatabaseQuery();
      this.m_Query.setDatabaseGUID(guid);
      this.m_Query.addQuery("select * from " + table + " limit 1");
    }
  
    var retval = false;
    switch ( this.m_Ids[index] )
    {
      case "library_cmd_device":
        retval = false; // not yet implemented
      break;
      case "library_cmd_showdlplaylist":
        retval = this.m_Query.execute() == 0;
      break;
      default:
        retval = true;
      break;
    }
    return retval;
  },

  onCommand: function( event )
  {
    if ( event.target && event.target.id )
    {
      // Was it from the toolbarbutton?
      var tbb = ( event.target.tagName == "button" || event.target.tagName == "xul:button" );
      switch( event.target.id )
      {
        case "library_cmd_play":
          // If the user hasn't selected anything, select the first thing for him.
          if ( this.m_Playlist.tree.currentIndex == -1 )
          {
            this.m_Playlist.tree.view.selection.currentIndex = 0;
            this.m_Playlist.tree.view.selection.select( 0 );
          }
          // Repurpose the command to act as if a doubleclick
          this.m_Playlist.sendPlayEvent();
        break;
        case "library_cmd_download":
        {
          try
          {        
            var filterCol = "uuid";
            var filterVals = new Array();
            
            var columnObj = this.m_Playlist.tree.columns.getNamedColumn(filterCol);
            var rangeCount = this.m_Playlist.tree.view.selection.getRangeCount();
            for (var i=0; i < rangeCount; i++) 
            {
              var start = {};
              var end = {};
              this.m_Playlist.tree.view.selection.getRangeAt( i, start, end );
              for( var c = start.value; c <= end.value; c++ )
              {
                if (c >= this.m_Playlist.tree.view.rowCount) 
                {
//                  alert( c + ">=" + this.m_Playlist.tree.view.rowCount );
                  continue; 
                }
                
                var value = this.m_Playlist.tree.view.getCellText(c, columnObj);
                
                filterVals.push(value);
              }
            }

  /*
            for(var i in filterVals)
            {
              alert(filterVals[i]);
            }
  */
            
            onBrowserTransfer( this.m_Playlist.guid, this.m_Playlist.table, filterCol, filterVals.length, filterVals );
            // And show the download table in the chrome playlist.
            onBrowserDownload();
          }
          catch( err )          
          {
            alert( "SBWebPlaylistCommands Error:" + err );
          }
        }
        break;
        case "library_cmd_subscribe":
        {
          // Bring up the subscribe dialog with the web playlist url
          var url = this.m_Playlist.type;
          var readable_name = null;
          var guid = null;
          var table = null;
          if ( url == "http" )
          {
            url = this.m_Playlist.description;
            readable_name = this.m_Playlist.readable_name;
            guid = this.m_Playlist.guid;
            table = this.m_Playlist.table;
          }
          SBSubscribe( url, guid, table, readable_name );
        }
        break;
        case "library_cmd_addtoplaylist":
        {
          this.m_Playlist.addToPlaylist();
        }
        break;
        case "library_cmd_addtolibrary":
        {
          this.m_Playlist.addToLibrary();
        }
        break;
        case "library_cmd_showdlplaylist":
        {
          onBrowserDownload();
        }
        break;
      }
    }
  },
  
  // The object registered with the sbIPlaylistSource interface acts 
  // as a template for instances bound to specific playlist elements
  duplicate: function()
  {
    var obj = {};
    for ( var i in this )
    {
      obj[ i ] = this[ i ];
    }
    return obj;
  },
  
  setPlaylist: function( playlist )
  {
    // Ah.  Sometimes, things are being secure.
    if ( playlist.wrappedJSObject )
      playlist = playlist.wrappedJSObject;
    this.m_Playlist = playlist;
  },
  
  QueryInterface : function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbIPlaylistCommands) &&
        !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    
    return this;
  }
}; // SBWebPlaylistCommands declaration

// Register the web playlist commands at startup
if ( ( WEB_PLAYLIST_CONTEXT != "" ) && ( WEB_PLAYLIST_TABLE != "" ) )
{
  var source = new sbIPlaylistsource();
  source.registerPlaylistCommands( WEB_PLAYLIST_CONTEXT, WEB_PLAYLIST_TABLE, "http", SBWebPlaylistCommands );
}

function onBrowserPlaylist()
{
  //SB_LOG("songbird_hack.js", "onBrowserPlaylist");
  metrics_inc("player", "urlslurp", null);
  if ( ! thePlaylistTree )
  {
    if (!theWebPlaylist.source ||
        theWebPlaylist.source.getRefGUID(theWebPlaylist.ref) != WEB_PLAYLIST_CONTEXT ||
        theWebPlaylist.source.getRefTable(theWebPlaylist.ref) != WEB_PLAYLIST_TABLE)
    {
      SBWebPlaylistCommands.m_Playlist = theWebPlaylist;
      theWebPlaylist.bind( WEB_PLAYLIST_CONTEXT, WEB_PLAYLIST_TABLE, null, SBWebPlaylistCommands, SBDataGetIntValue( "browser.playlist.height" ), SBDataGetBoolValue( "browser.playlist.collapsed" ) );
    }
    
    // Show/hide them
    theShowWebPlaylistData.boolValue = true;
  }
  else
  {
    var theServiceTree = document.getElementById( 'frame_servicetree' );
    theServiceTree.launchServiceURL( "chrome://songbird/content/xul/playlist_test.xul?" +
                                     WEB_PLAYLIST_TABLE + "," +
                                     WEB_PLAYLIST_CONTEXT );
  }
}

function onBrowserDownload()
{
  metrics_inc("player", "downloads", null);

  // Work to figure out guid and table
  var guid = theDownloadContext.stringValue;
  var table = theDownloadTable.stringValue;
  var deviceManager = Components.classes["@songbirdnest.com/Songbird/DeviceManager;1"].
                                  getService(Components.interfaces.sbIDeviceManager);
  if (deviceManager)
  {
    var downloadCategory = 'Songbird Download Device';
    if (deviceManager.hasDeviceForCategory(downloadCategory))
    {
      var downloadDevice =
        deviceManager.getDeviceByCategory(downloadCategory);
      SBDownloadCommands.m_Device = downloadDevice;
      guid = downloadDevice.getContext('');
      table = "download";
    }
  }
  
  // Actual functionality
  if ( ! thePlaylistTree )
  {
    // Errrr, nope?
    if ( ( guid == "" ) || ( table == "" ) )
    {
      return;
    }

    if (!theWebPlaylist.source ||
        theWebPlaylist.source.getRefGUID(theWebPlaylist.ref) != guid ||
        theWebPlaylist.source.getRefTable(theWebPlaylist.ref) != table)
    {
      theWebPlaylist.bind( guid, table, null, SBDownloadCommands, SBDataGetIntValue( "browser.playlist.height" ), SBDataGetBoolValue( "browser.playlist.collapsed" ) );
    }
    
    // Show/hide them
    theShowWebPlaylistData.boolValue = true;
  }
  else
  {
    var theServiceTree = document.getElementById( 'frame_servicetree' );
    theServiceTree.launchServiceURL( "chrome://songbird/content/xul/playlist_test.xul?" + table + "," + guid );
  }
}

function onBrowserPlaylistHide()
{
  //SB_LOG("songbird_hack.js", "onBrowserPlaylistHide");
  // Hide the web table if it exists
  theShowWebPlaylistData.boolValue = false;
 
  // And unhook the playlist from the database
  var theTree = document.getElementById( "playlist_web" );
  if ( theTree )
  {
    var source = new sbIPlaylistsource();
    theTree.datasources = "";
    theTree.ref = "";
  }
}

// onHTMLUrlChange
function onHTMLUrlChange( evt )
{
  var value = evt.target.value;
  if ( value && value.length )
  {
    // Make sure the value is an url
    value = SBGetUrlFromService( value );
    // And then put it back in the box as a service
    theBrowserUriData.stringValue = value;
    theBrowserUrlData.stringValue = SBGetServiceFromUrl( value );
    var image = SBGetServiceImageFromUrl( value );
//    if ( image.length )
    {
      theBrowserImageData.stringValue = image;
    }
    // And then go to the url.  Easy, no?
    var theServiceTree = document.getElementById( 'frame_servicetree' );
    theServiceTree.launchURL( value );
  }
}

function onHTMLURLFocus (evt) 
{
  var url = document.getElementById("browser_url");
  var text = url.value;
  url.selectionStart = 0;
  url.selectionEnd = text.length;
  url.setAttribute("savefocus", text);
}

function onHTMLURLRestore( evt ) 
{
  var url = document.getElementById("browser_url");
  if ( url.getAttribute("savefocus") != "" )
  {
    url.value = url.getAttribute("savefocus");
    url.setAttribute("savefocus", "");
  }
}

function onHTMLUrlKeypress( evt )
{
  switch ( evt.keyCode )
  {
    case 27: // Esc
      var oldval = evt.target.getAttribute("savefocus");
      onHTMLURLRestore(evt);
      evt.target.setAttribute("savefocus", oldval);
      break;
      
    case 13: // Enter
      evt.target.setAttribute("savefocus", "");
      evt.target.value = SBTabcompleteService( evt.target.value );
      onHTMLUrlChange( evt );
/*      
      evt.target.selectionStart = 0;
      evt.target.selectionEnd = evt.target.value.length;
*/      
      break;
          
/*      
    case 9:  // Tab
      var value = SBTabcompleteService( evt.target.value );
      if ( value != evt.target.value )
      {
        alert ( value + " != " + evt.target.value )
        evt.target.value = value;
        onHTMLUrlChange( evt );
      }
      break;
*/      
  }
}

var mainpane_listener_set = false;
var thePlaylistRef = SB_NewDataRemote( "playlist.ref", null );
var thePaneLoadingData = SB_NewDataRemote( "faceplate.loading", null );
thePaneLoadingData.boolValue = false;
var thePlaylistTree;

var theCurrentMainPaneDocument = null;
function onMainPaneLoad()
{
  //SB_LOG("songbird_hack.js", "onMainPaneLoad");
  try
  {
    if ( ! mainpane_listener_set )
    {
      var theMainPane = document.getElementById( "frame_main_pane" );
      if ( typeof( theMainPane ) == 'undefined' )
      {
        //SB_LOG("songbird_hack.js", "onMainPaneLoad - returning early, no browser object yet");
        return;
      }
      
      //
      //
      // HORRIBLE SECURITY HACK TO GET THE PLAYLIST TREE AND INJECT FUN EVENT HANDLERS 
      //
      //
      var installed_listener = false;
      var playlist = theMainPane.contentDocument.getElementById( "playlist_test" );
      if (playlist) {
        // Crack the security if necessary
        if ( playlist.wrappedJSObject )
          playlist = playlist.wrappedJSObject;

        // Wait until after the bind call?
        if ( playlist.ref == "" )
        {
          //SB_LOG("songbird_hack.js", "onMainPaneLoad - setting timeout(2), haven't loaded yet");
          // Try again in 250 ms?
          setTimeout( onMainPaneLoad, 250 );
          return;
        }

        // hack, to let play buttons find the visible playlist if needed
        document.__CURRENTPLAYLIST__ = playlist;

        // Drag and Drop tracker object
        playlist.setDnDSourceTracker(sbDnDSourceTracker);

        // Events on the playlist object itself.            
        playlist.addEventListener( "playlist-edit", onPlaylistEdit, true );
        playlist.addEventListener( "playlist-editor", onPlaylistEditor, true );
        playlist.addEventListener( "playlist-play", onPlaylistPlay, true );
        playlist.addEventListener( "playlist-burntocd", onPlaylistBurnToCD, true );
        playlist.addEventListener( "playlist-noplaylist", onPlaylistNoPlaylist, true );
        playlist.addEventListener( "playlist-filterchange", onPlaylistFilterChange, true );
        playlist.addEventListener( "command", onPlaylistContextMenu, false );  // don't force it!

        // Remember some values
        theLibraryPlaylist = playlist;
        thePlaylistTree = playlist = playlist.tree;
        thePlaylistRef.stringValue = playlist.getAttribute( "ref" ); // is this set yet?

        // Set the current selection
        theLibraryPlaylist.syncPlaylistIndex(false);

        // And remember that we did this
        installed_listener = true;

        mainpane_listener_set = true;
        
        if (document.__JUMPTO__) document.__JUMPTO__.syncJumpTo();
      }
      else {
        //SB_LOG("songbird_hack.js", "onMainPaneLoad - no playlist_test setting playlists to null");
        thePlaylistTree = null;
        theLibraryPlaylist = null;
        // hack, to let play buttons find the visible playlist if needed
        document.__CURRENTPLAYLIST__ = null;
      }

      // If we have not installed a playlist listener, install an url listener.
      if ( ! installed_listener )
      {
        // wait until the document exists to see if there are any A tags
        if ( ! theMainPane.contentDocument )
        {
          //SB_LOG("songbird_hack.js", "onMainPaneLoad - setting timeout(3), no document");
          setTimeout( onMainPaneLoad, 2500 );
          return;
        }
        else if ( theMainPane.contentDocument.getElementsByTagName('A').length != 0 )
        {
          AsyncWebDocument( theMainPane.contentDocument );
        } 


        // XXXlone -- we need this to happen so that the playlist gets initialized if it wasn't ready yet but is coming up soon !
        // unfortunately, if the page is not going to be a playlist, but is a document with no A element, we'll keep hitting this code
        // and rerun this function every 2.5s, forever. We need to detect that a page is a not a playlist in the process of being loaded 
        // so that we can avoid issuing a setTimeout(onMainPaneLoad, 2500) in this case, but ONLY in this case.
        else 
        {
          setTimeout( onMainPaneLoad, 2500 );
          return;
        }
        // XXXlone -- end case where page is going to be a playlist but contentDocument does not yet contains a playlist

        mainpane_listener_set = true;
      }
    }
  }
  catch( err )
  {
    alert( err );
  }
  //SB_LOG("songbird_hack.js", "onMainPaneLoad - leaving");
}

function onMainPaneUnload()
{
  //SB_LOG("songbird_hack.js", "onMainPaneUnload");
  try
  {
  }
  catch( err )
  {
    alert( err );
  }
}

function GetHREFFromEvent( evt )
{
  var the_href = "";
  try
  {
    var node = evt.target;
    while ( node ) // Walk up from the event target to find the A? 
    {
      if ( node.href )
      {
        the_href = node.href;
        break;
      }
      node = node.parentNode;
    }
  }
  catch ( err )
  {
    alert( err );
  }
  return the_href;
}

// Catch a contextual on a media url and attempt to play it
function onLinkOver( evt )
{
  var the_url = GetHREFFromEvent( evt )
  theStatusText.stringValue = the_url;
  if ( gPPS.isMediaURL( the_url ) || gPPS.isPlaylistURL( the_url ) )
  {
    theStatusStyle.stringValue = "font-weight: bold;";
  }
  else
  {
    theStatusStyle.stringValue = "font-weight: normal;";
  }
}

// Catch a contextual on a media url and attempt to play it
function onLinkOut( evt )
{
  theStatusText.stringValue = "";
}

// Catch a contextual on a media url and attempt to play it
var theHTMLContextURL = null;
function onLinkContext( evt )
{
  try
  {
    var theMainPane = document.getElementById( "frame_main_pane" );
    var theHTMLPopup = document.getElementById( "html_context_menu" );
    theHTMLContextURL = GetHREFFromEvent( evt );
    
    // Disable "Add" if the url isn't media or is already there.
    var disabled = "true";
    if ( gPPS.isMediaURL( theHTMLContextURL ) && ! SBUrlExistsInDatabase( theHTMLContextURL ) )
    {
      disabled = "false"
    }
    document.getElementById( "html.context.add" ).setAttribute( "disabled", disabled );
    
    // Disable "Add as Playlist" if the url isn't a playlist (NOTE: any HTML url will go as playlist)
    disabled = "true";
    if ( gPPS.isPlaylistURL( theHTMLContextURL ) )
    {
      disabled = "false"
    }
    document.getElementById( "html.context.playlist" ).setAttribute( "disabled", disabled );
    
    theHTMLPopup.showPopup( theMainPane, theMainPane.boxObject.screenX + evt.clientX + 5, theMainPane.boxObject.screenY + evt.clientY, "context", null, null );
  }
  catch ( err )
  {
    alert( err );
  }
}

function playExternalUrl(the_url, tryweb) 
{
  //SB_LOG("songbird_hack.js", "playExternalUrl: " + the_url);
  var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
  // figure out if the url is in the webplaylist
  if (tryweb && theWebPlaylist && !theWebPlaylist.hidden) 
  {
    var row = theWebPlaylist.findRowIdByUrl(the_url);
    if (row != -1) 
    {
      // if so, play the ref, from that entry's index
      PPS.playRef("NC:webplaylist_webplaylist", row);
    }
  } else {
    // otherwise, play the url as external (added to the db, plays the library from that point on)
    PPS.playAndImportURL(the_url); // if the url is already in the lib, it is not added twice
  }
}

function handleMediaURL( aURL, aShouldBeginPlayback )
{
  var retval = false;
  try
  {
    // Stick playlists in the service pane (for now).
    if ( gPPS.isPlaylistURL( aURL ) )
    {
      var playlistReader = Components.classes["@songbirdnest.com/Songbird/PlaylistReaderManager;1"]
                           .createInstance(Components.interfaces.sbIPlaylistReaderManager);
      var playlistReaderListener = Components.classes["@songbirdnest.com/Songbird/PlaylistReaderListener;1"]
                           .createInstance(Components.interfaces.sbIPlaylistReaderListener);

      // if we can find it in the service pane already then we shouldn't add it again.
      var queryObj = new sbIDatabaseQuery();
      queryObj.setDatabaseGUID("songbird");
      var playlistManager = new sbIPlaylistManager();
      playlistManager.getAllPlaylistList( queryObj );
      var resultset = queryObj.getResultObject();
      for ( var index = 0; index < resultset.getRowCount(); index++ )
      {
        // if we match don't add it, just play it.
        if ( aURL == resultset.getRowCellByColumn( index, "description" ) )
        {
          gPPS.playTable(resultset.getRowCellByColumn(index, "service_uuid"),
                         resultset.getRowCellByColumn(index, "name"),
                         0);
          return true;
        }
      }

      // Create this closure here to prevent this object from getting garbage
      // collected too soon.  The playlist reader uses the nsIWebBrowserPersist
      // component that does _not_ addref this listener :(
      var playlist_observer = {
        observe: function ( aSubject, aTopic, aData ) {
          if (aTopic.indexOf("error") != -1) {
            var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                                          .getService(Components.interfaces.nsIPromptService);
            promptService.alert(null,
                                "Playlist Error",
                                "Unable to read playlist file -- please try again later.");
          }
          else {
            SBScanServiceTreeNewEntryStart();
          }
        }
      };

      SBScanServiceTreeNewEntryEditable();

      playlistReaderListener.playWhenLoaded = aShouldBeginPlayback;
      playlistReaderListener.observer = playlist_observer;
      playlistReaderListener.mediaMimetypesOnly = true;
      playlistReader.autoLoad( aURL,
                               "songbird", 
                               gPPS.convertURLToDisplayName( aURL ),
                               "http",
                               aURL,
                               "", 
                               playlistReaderListener );
      retval = true;
    }
    // Everything else gets played directly.
    else if ( gPPS.isMediaURL( aURL ) )
    {
      playExternalUrl(aURL, true);
      retval = true;
    }
  }
  catch ( err )
  {
    alert("songbird_hack.js: handleMediaURL(" + aURL + "); " + err );
  }
  return retval;
}

// Catch a click on a media url and attempt to play it
function onMediaClick( evt )
{
  handleMediaURL( GetHREFFromEvent(evt), true );
  evt.stopPropagation();
  evt.preventDefault();
}

function onPlaylistKeypress( evt )
{
  switch ( evt.keyCode )
  {
    case 13: // Return
      SBPlayPlaylistIndex( thePlaylistTree.currentIndex );
      break;
  }
}

// Yo, play something, bitch.
function onPlaylistPlay( evt )
{
  var target = evt.target;
  if ( target.wrappedJSObject )
  {
    target = target.wrappedJSObject;
  }
  SBPlayPlaylistIndex( target.tree.currentIndex, target );
}

function onPlaylistBurnToCD( evt )
{
  var target = evt.target;
  if ( target.wrappedJSObject )
  {
    target = target.wrappedJSObject;
  }
  
  playlist = target;
  playlistTree = target.tree;
  
	var filterCol = "uuid";
	var filterVals = new Array();

	var columnObj = playlistTree.columns.getNamedColumn(filterCol);
	var rangeCount = playlistTree.view.selection.getRangeCount();
	for (var i=0; i < rangeCount; i++) 
	{
		var start = {};
		var end = {};
		playlistTree.view.selection.getRangeAt( i, start, end );
		for( var c = start.value; c <= end.value; c++ )
		{
			if (c >= playlistTree.view.rowCount) 
			{
			continue; 
			}
	        
			var value = playlistTree.view.getCellText(c, columnObj);
			filterVals.push(value);
		}
	}
	
    onAddToCDBurn( playlist.guid, playlist.table, filterCol, filterVals.length, filterVals );
}

function onPlaylistNoPlaylist() 
{ 
  document.getElementById( 'frame_servicetree' ).launchServiceURL( 'chrome://songbird/content/xul/playlist_test.xul?library' ); 
}

function onPlaylistFilterChange() {
  if (document.__JUMPTO__) document.__JUMPTO__.syncJumpTo();
}

function onPlaylistDblClick( evt )
{
  if ( typeof( thePlaylistTree ) == 'undefined' )
  {
    alert( "DOM?" );
    return;
  }
  var obj = {}, row = {}, col = {}; 
  thePlaylistTree.treeBoxObject.getCellAt( evt.clientX, evt.clientY, row, col, obj );
  // If the "obj" has a value, it is a cell?
  if ( obj.value )
  {
    if ( thePlaylistTree.currentIndex != -1 )
    {
      SBPlayPlaylistIndex( thePlaylistTree.currentIndex );
    }
  }
}

function SBDownloadDeviceTest()
{
try
{
  var downloadDevice = Components.classes["@songbirdnest.com/Songbird/DownloadDevice;1"];
  if (downloadDevice)
  {
    downloadDevice = downloadDevice.createInstance();
    downloadDevice = downloadDevice.QueryInterface(Components.interfaces.sbIDeviceBase);
    
    if (downloadDevice)
    {
      listProperties( downloadDevice, "downloadDevice" );
      alert( Components.interfaces.sbIDownloadDevice );
          downloadDevice.name;
          downloadDevice.isDownloadSupported();
          t = downloadDevice.DownloadTrackTable('testdb-0000','download');
    }
  }
}
catch ( err )  
{
  alert( err );
}
}

// Sigh.  The track editor assumes this document object will be populated with
// special info.  So we have to pop the track editor from this DOM, not XBL.
function onPlaylistEditor( evt )
{
  var playlist = evt.target;
  if ( playlist.wrappedJSObject )
    playlist = playlist.wrappedJSObject;
  
  if (playlist && playlist.guid == "songbird") 
    SBTrackEditorOpen();
}

var theCurrentlyEditingPlaylist = null;
function onPlaylistEdit( evt )
{
  var playlist = evt.target;
  if ( playlist.wrappedJSObject )
    playlist = playlist.wrappedJSObject;
  theCurrentlyEditingPlaylist = playlist;
  setTimeout(doPlaylistEdit, 0);
}

function doPlaylistEdit()
{
  try
  {
    // Make sure it's something with a uuid column.
    var filter = "uuid";
    var filter_column = theCurrentlyEditingPlaylist.tree.columns ? theCurrentlyEditingPlaylist.tree.columns[filter] : filter;
    var filter_value = theCurrentlyEditingPlaylist.tree.view.getCellText( theCurrentlyEditingPlaylist.tree.currentIndex, filter_column );
    if ( !filter_value )
    {
      return;
    }
    
    // We want to resize the edit box to the size of the cell.
    var out_x = {}, out_y = {}, out_w = {}, out_h = {}; 
    theCurrentlyEditingPlaylist.tree.treeBoxObject.getCoordsForCellItem( theCurrentlyEditingPlaylist.edit_row, theCurrentlyEditingPlaylist.edit_col, "cell",
                                                        out_x , out_y , out_w , out_h );
                           
    var cell_text = theCurrentlyEditingPlaylist.tree.view.getCellText( theCurrentlyEditingPlaylist.edit_row, theCurrentlyEditingPlaylist.edit_col );
    
    // Then pop the edit box to the bounds of the cell.
    var theMainPane = document.getElementById( "frame_main_pane" );
    var theEditPopup = document.getElementById( "playlist_edit_popup" );
    var theEditBox = document.getElementById( "playlist_edit" );
    var extra_x = 5; // Why do I have to give it extra?  What am I calculating wrong?
    var extra_y = 21; // Why do I have to give it extra?  What am I calculating wrong?
    var less_w  = 6;
    var less_h  = 0;
    var pos_x = extra_x + theCurrentlyEditingPlaylist.tree.boxObject.screenX + out_x.value;
    var pos_y = extra_y + theCurrentlyEditingPlaylist.tree.boxObject.screenY + out_y.value;
    theEditBox.setAttribute( "hidden", "false" );
    theEditPopup.showPopup( theMainPane, pos_x, pos_y, "context" );
    theEditPopup.sizeTo( out_w.value - less_w, out_h.value - less_h ); // increase the width to the size of the cell.
    theEditBox.value = cell_text;
    theEditBox.focus();
    theEditBox.select();
    isPlaylistEditShowing = true;
    theCurrentlyEditingPlaylist.theCurrentlyEditedUUID = filter_value;
    theCurrentlyEditingPlaylist.theCurrentlyEditedOldValue = cell_text;
  }
  catch ( err )
  {
    alert( err );
  }
}

function onPlaylistEditChange( evt )
{
  try
  {
    if (isPlaylistEditShowing) {
      var theEditBox = document.getElementById( "playlist_edit" );
      
      var filter_value = theCurrentlyEditingPlaylist.theCurrentlyEditedUUID;
      
      var the_table_column = theCurrentlyEditingPlaylist.edit_col.id;
      var the_new_value = theEditBox.value
      if (theCurrentlyEditingPlaylist.theCurrentlyEditedOldValue != the_new_value) {
      
        var aDBQuery = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].createInstance(Components.interfaces.sbIDatabaseQuery);
        var aMediaLibrary = Components.classes["@songbirdnest.com/Songbird/MediaLibrary;1"].createInstance(Components.interfaces.sbIMediaLibrary);
        
        if ( ! aDBQuery || ! aMediaLibrary)
          return;
        
        aDBQuery.setAsyncQuery(true);
        aDBQuery.setDatabaseGUID(theCurrentlyEditingPlaylist.guid);
        aMediaLibrary.setQueryObject(aDBQuery);
        
        aMediaLibrary.setValueByGUID(filter_value, the_table_column, the_new_value, false);
        
        //var table = "library" // hmm... // theCurrentlyEditingPlaylist.table;
        //var q = 'update ' + table + ' set ' + the_table_column + '="' + the_new_value + '" where ' + filter + '="' + filter_value + '"';
        //aDBQuery.addQuery( q );
        
        //var ret = aDBQuery.execute();
      }    
      HidePlaylistEdit();
    }
  }
  catch ( err )
  {
    alert( err );
  }
}

function onPlaylistEditKeypress( evt )
{
  switch ( evt.keyCode )
  {
    case 27: // Esc
      HidePlaylistEdit();
      break;
    case 13: // Return
      onPlaylistEditChange( evt );
      break;
  }
}

function focusSearch() 
{
  var search_widget = document.getElementById( "search_widget" );
  search_widget.onFirstMousedown(); // Sets focus.  Clears "search" text.
}

function focusLocationBar()
{
  var location_bar = document.getElementById( "browser_url" );
  location_bar.focus();
}

var isPlaylistEditShowing = false;
function HidePlaylistEdit()
{
  try
  {
    if ( isPlaylistEditShowing )
    {
      var theEditBox = document.getElementById( "playlist_edit" );
      theEditBox.setAttribute( "hidden", "true" );
      var theEditPopup = document.getElementById( "playlist_edit_popup" );
      theEditPopup.hidePopup();
      isPlaylistEditShowing = false;        
      theCurrentlyEditingPlaylist = null;
    }
  }
  catch ( err )
  {
    alert( err );
  }
}

// Menubar handling
function onPlaylistContextMenu( evt )
{
  try
  {
    // hacky for now
    var playlist = theLibraryPlaylist;
    if ( !playlist )
    {
      playlist = theWebPlaylist;
    }
    
    // All we do up here, now, is dispatch the search items
    onSearchTerm( playlist.context_item, playlist.context_term );
  }
  catch ( err )
  {
    alert( err );
  }
}

function onSearchTerm( target, in_term )
{
  var search_url = "";
  if ( target && in_term && in_term.length )
  {
//    var term = '"' + in_term + '"';
    var term = in_term;
    var v = target.getAttribute( "id" );
    switch ( v )
    {
      case "search.popup.songbird":
        onSearchEditIdle();
      break;
      case "search.popup.google":
        search_url = "http://www.google.com/musicsearch?q=" + term + "&sa=Search";
      break;
      case "search.popup.wiki":
        search_url = "http://en.wikipedia.org/wiki/Special:Search?search=" + term;
      break;
      case "search.popup.yahoo":
        search_url = "http://audio.search.yahoo.com/search/audio?ei=UTF-8&fr=sfp&p=" + term;
      break;
      case "search.popup.emusic":
        search_url = "http://www.emusic.com/search.html?mode=x&QT=" + term + "&x=0&y=0";
      break;
      case "search.popup.insound":
        search_url = "http://search.insound.com/search/searchmain.jsp?searchby=meta&query=" + term + "&fromindex=1&submit.x=0&submit.y=0";
      break;
      case "search.popup.odeo":
        search_url = "http://odeo.com/search/query/?q=" + term + "&Search.x=0&Search.y=0";
      break;
      case "search.popup.shoutcast":
        search_url = "http://www.shoutcast.com/directory/?s=" + term;
      break;
      case "search.popup.radiotime":
        search_url = "http://radiotime.com/Search.aspx?query=" + term;
      break;        
      case "search.popup.elbows":
        search_url = "http://elbo.ws/mp3s/" + term;
      break;
      case "search.popup.singingfish":
        search_url = "http://search.singingfish.com/sfw/search?a_submit=1&aw=1&sfor=a&dur=1&fmp3=1&cmus=1&cmov=1&crad=1&rpp=20&persist=1&exp=0&x=0&y=0&query=" + term;
      break;
      case "lyrics.popup.google":
        search_url = "http://www.google.com/search?q=lyrics " + term + "&sa=Search&client=pub-4053348708517670&forid=1&ie=ISO-8859-1&oe=ISO-8859-1&hl=en&GALT:#333333;GL:1;DIV:#37352E;VLC:000000;AH:center;BGC:C6B396;LBGC:8E866F;ALC:000000;LC:000000;T:44423A;GFNT:663333;GIMP:663333;FORID:1;";
      break;
    }
  }
  if ( search_url.length )
  {
    var theServiceTree = document.getElementById( 'frame_servicetree' );
    theServiceTree.launchURL( search_url );
  }
}

function doMenu( command ) {
  switch ( command )
  {
    case "file.new":
      SBNewPlaylist();
    break;
    case "file.smart":
      SBNewSmartPlaylist();
    break;
    case "file.remote":
      SBSubscribe( "", "", "", "" );
    break;
    case "file.file":
      SBFileOpen();
    break;
    case "file.url":
      SBUrlOpen();
    break;
    case "file.mab":
      SBMabOpen();
    break;
    case "file.playlist":
      SBPlaylistOpen();
    break;
    case "file.scan":
      SBScanMedia();
    break;
    case "file.dlfolder":
      SBSetDownloadFolder();
    break;
    case "file.watch":
      SBWatchFolders();
    break;
/*    
    case "file.htmlbar":
      if ( SBDataGetIntValue( "option.htmlbar" ) == 0 )
      {
        SBDataSetIntValue( "option.htmlbar", 1 );
      }
      else
      {
        SBDataSetIntValue( "option.htmlbar", 0 );
      }
    break;
*/    

    case "file.window":
      SBMiniplayerOpen();
    break;
    case "file.koshi":
      SBKoshiOpen();
    break;
    case "aboutName": // This has to be hardcoded this way for the stinky mac.
      About(); 
    break;
    case "menu_FileQuitItem": // Stinky mac again.  See nsMenuBarX::AquifyMenuBar
      quitApp();
    break;
    case "control.play":
      gPPS.playing ? ( gPPS.paused ? gPPS.play() : gPPS.pause() ) : gPPS.play();
    break;
    case "control.next":
      gPPS.next();
    break;
    case "control.prev":
      gPPS.previous();
    break;
    case "control.shuf":
      SBDataSetIntValue( "playlist.shuffle", (SBDataGetIntValue( "playlist.shuffle" ) + 1) % 2 );
    break;
    case "control.repa":
      SBDataSetIntValue( "playlist.repeat", 2 );
    break;
    case "control.rep1":
      SBDataSetIntValue( "playlist.repeat", 1 );
    break;
    case "control.repx":
      SBDataSetIntValue( "playlist.repeat", 0 );
    break;
    case "control.jumpto":
      toggleJumpTo();
    break;
    case "menu.extensions":
      SBExtensionsManagerOpen();
    break;
    case "menu_preferences":
      SBOpenPreferences();
    break;
/*    case "menu.downloadmgr":
      SBOpenDownloadManager();
    break;*/
    case "menu.dominspector":
      SBDOMInspectorOpen();
    break;
    case "checkForUpdates": {
      var um =
          Components.classes["@mozilla.org/updates/update-manager;1"].
          getService(Components.interfaces.nsIUpdateManager);
      var prompter =
          Components.classes["@mozilla.org/updates/update-prompt;1"].
          createInstance(Components.interfaces.nsIUpdatePrompt);
     
      // If there's an update ready to be applied, show the "Update Downloaded"
      // UI instead and let the user know they have to restart the browser for
      // the changes to be applied. 
      if (um.activeUpdate && um.activeUpdate.state == "pending")
        prompter.showUpdateDownloaded(um.activeUpdate);
      else
        prompter.checkForUpdates();
    }
    break;
    case "services.browse":
      var theServiceTree = document.getElementById( 'frame_servicetree' );
      theServiceTree.launchURL( "http://extensions.songbirdnest.com/" );
    break;
    
    default:
      return false;
  }
  return true;
}

// Menubar handling
function onMenu( target )
{
  var v = target.getAttribute( "id" );
  if (!doMenu(v)) {
    // ==== Default is to launch the value property if one exists, or do nothing.      
    if ( target.value )
    {
      var theServiceTree = document.getElementById( 'frame_servicetree' );
      theServiceTree.launchURL( target.value );
    }
  }
}

function SBOpenPreferences(paneID)
{
  // On all systems except Windows pref changes should be instant.
  //
  // In mozilla this is the browser.prefereces.instantApply pref,
  // and is set at compile time.
  var instantApply = navigator.userAgent.indexOf("Windows") == -1;
	
  var features = "chrome,titlebar,toolbar,centerscreen" + (instantApply ? ",dialog=no" : ",modal");

  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].getService(Components.interfaces.nsIWindowMediator);
  var win = wm.getMostRecentWindow("Browser:Preferences");
  if (win) {
    win.focus();
    if (paneID) {
      var pane = win.document.getElementById(paneID);
      win.document.documentElement.showPane(pane);
    }
  }
  else
    openDialog("chrome://browser/content/preferences/preferences.xul", "Preferences", features, paneID);
    
  // to open connection settings only:
  // SBOpenModalDialog("chrome://browser/content/preferences/connection.xul", "chrome,modal=yes,centerscreen", null);
}

function SBSetDownloadFolder()
{
  // Just open the window, we don't care what the user does in it.
  SBOpenModalDialog( "chrome://songbird/content/xul/download.xul", "", "chrome,modal=yes,centerscreen", null );
}

/*function SBOpenDownloadManager()
{
  var dlmgr = Components.classes['@mozilla.org/download-manager;1'].getService();
  dlmgr = dlmgr.QueryInterface(Components.interfaces.nsIDownloadManager);

  var windowMediator = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
  windowMediator = windowMediator.QueryInterface(Components.interfaces.nsIWindowMediator);

  var dlmgrWindow = windowMediator.getMostRecentWindow("Download:Manager");
  if (dlmgrWindow) {
    dlmgrWindow.focus();
  }
  else {
    openDialog("chrome://mozapps/content/downloads/downloads.xul", "Download:Manager", "chrome,centerscreen", null);
   }
}*/

function SBWatchFolders()
{
  SBOpenModalDialog( "chrome://songbird/content/xul/watch_folders.xul", "", "chrome,modal=yes,centerscreen", null );
}

// Menubar handling
function onHTMLContextMenu( target )
{
  if ( theHTMLContextURL )
  {
    var v = target.getAttribute( "id" );
    switch ( v )
    {
      case "html.context.open":
        // can be track or playlist
        // try dealing with media, might just be web content.
        if ( !handleMediaURL(theHTMLContextURL, true) )
        {
          var theServiceTree = document.getElementById( 'frame_servicetree' );
          theServiceTree.launchURL( theHTMLContextURL );
        }
      break;
      case "html.context.openexternal":
          var externalLoader = (Components
                   .classes["@mozilla.org/uriloader/external-protocol-service;1"]
                  .getService(Components.interfaces.nsIExternalProtocolService));
          var nsURI = (Components
                  .classes["@mozilla.org/network/io-service;1"]
                  .getService(Components.interfaces.nsIIOService)
                  .newURI(theHTMLContextURL, null, null));
          externalLoader.loadURI(nsURI, null);
      break;
      case "html.context.play":
        // can be track or playlist
        handleMediaURL(theHTMLContextURL, true);
      break;
      case "html.context.add":
        gPPS.importURL(theHTMLContextURL);
      break;
      case "html.context.playlist":
        // Add playlists to the service pane
        handleMediaURL(theHTMLContextURL, false);
      break;
    }
    theHTMLContextURL = null; // clear it because now we're done.
  }
}


var theMediaScanIsOpen = SB_NewDataRemote( "media_scan.open", null );
function SBScanMedia( )
{
  theMediaScanIsOpen.boolValue = true;
  const nsIFilePicker = Components.interfaces.nsIFilePicker;
  const CONTRACTID_FILE_PICKER = "@mozilla.org/filepicker;1";
  var fp = Components.classes[CONTRACTID_FILE_PICKER].createInstance(nsIFilePicker);
  var welcome = "Welcome";
  var scan = "Scan";
  try
  {
    welcome = theSongbirdStrings.getString("faceplate.welcome");
    scan = theSongbirdStrings.getString("faceplate.scan");
  } catch(e) {}
  if (getPlatformString() == "Darwin") {
    fp.init( window, scan, nsIFilePicker.modeGetFolder );
    var defaultDirectory =
    Components.classes["@mozilla.org/file/directory_service;1"]
              .getService(Components.interfaces.nsIProperties)
              .get("Home", Components.interfaces.nsIFile);
    defaultDirectory.append("Music");
    fp.displayDirectory = defaultDirectory;
  } else {
    fp.init( window, welcome + "\n\n" + scan, nsIFilePicker.modeGetFolder );
  }
  var res = fp.show();
  if ( res == nsIFilePicker.returnOK )
  {
    var media_scan_data = new Object();
    media_scan_data.URL = fp.file.path;
    media_scan_data.retval = "";
    // Open the non-modal dialog
    SBOpenModalDialog( "chrome://songbird/content/xul/media_scan.xul", "media_scan", "chrome,modal=yes,centerscreen", media_scan_data );
  }
  theMediaScanIsOpen.boolValue = false;
}

function SBMabOpen()
{
  var mab_data = new Object();
  mab_data.retval = "";
  
  // Open the modal dialog
  SBOpenModalDialog( "chrome://songbird/content/xul/mab.xul", "Mozilla Amazon Browser", "chrome,modal=no", mab_data );
}

function SBNewPlaylist()
{
  try
  {
    SBScanServiceTreeNewEntryEditable();
    var query = new sbIDatabaseQuery();
    query.setDatabaseGUID( "songbird" );
    var playlistmanager = new sbIPlaylistManager();
    var aUUIDGenerator = Components.classes["@mozilla.org/uuid-generator;1"].createInstance(Components.interfaces.nsIUUIDGenerator);
    var playlistguid = aUUIDGenerator.generateUUID();
    var name = "Playlist";
    try
    {
      name = theSongbirdStrings.getString("playlist");
    } catch(e) {}
    var playlist = playlistmanager.createPlaylist( playlistguid, name, "Playlist", "user", query );
    SBScanServiceTreeNewEntryStart();
  }
  catch ( err )
  {
    alert( "SBNewPlaylist - " + err );
  }
}

function SBMiniplayerOpen()
{
  // Get miniplayer URL
  var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
  var miniwin = "chrome://rubberducky/content/xul/miniplayer.xul";
  try {
    miniwin = prefs.getCharPref("general.bones.selectedMiniPlayerURL", miniwin);  
  } catch (err) {}

  // Open the window
  window.open( miniwin, "", "chrome,titlebar=no,resizable=no" );
  onExit();
}

function SBNewSmartPlaylist( guid, table )
{
  SBScanServiceTreeNewEntryEditable(); // Do this right before you add to the servicelist?
  
  // Make a magic data object to get passed to the dialog
  var smart_playlist = new Object();
  smart_playlist.retval = "";
  smart_playlist.guid = guid;
  smart_playlist.table = table
  // Open the window
  SBOpenModalDialog( "chrome://songbird/content/xul/smart_playlist.xul", "", "chrome,modal=yes,centerscreen", smart_playlist );
  
  if ( smart_playlist.retval == "ok" )
  {
    SBScanServiceTreeNewEntryStart(); // And this once you know you really did?
  }
}

function SBKoshiOpen()
{
  // Make a magic data object to get passed to the dialog
  var koshi_data = new Object();
  koshi_data.retval = "";
  // Open the window
  SBOpenModalDialog( "chrome://songbird/content/xul/koshi_test.xul", "", "chrome,modal=yes,centerscreen", koshi_data );
}

function SBExtensionsManagerOpen()
{
  const EM_TYPE = "Extension:Manager";
  
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  var theEMWindow = wm.getMostRecentWindow(EM_TYPE);
  if (theEMWindow) {
    theEMWindow.focus();
    return;
  }
  
  const EM_URL = "chrome://mozapps/content/extensions/extensions.xul?type=extensions";
  const EM_FEATURES = "chrome,menubar,extra-chrome,toolbar,dialog=no,resizable";
  window.openDialog(EM_URL, "", EM_FEATURES);
}

function SBTrackEditorOpen()
{
  // for now only allow editing the main db
  if (!theLibraryPlaylist || theLibraryPlaylist.guid != "songbird") return;
  
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  var theTE = wm.getMostRecentWindow("track_editor");
  if (theTE) {
    theTE.focus();
  } else {
    const TEURL = "chrome://songbird/content/xul/trackeditor.xul";
    const TEFEATURES = "chrome,dialog=no,resizable=no,titlebar=no";
    window.openDialog(TEURL, "track_editor", TEFEATURES, document);
  }
}

function SBDOMInspectorOpen()
{
  window.open("chrome://inspector/content/", "", "chrome,dialog=no,resizable");
}

function SBSubscribe( url, guid, table, readable_name )
{
  // Make a magic data object to get passed to the dialog
  var subscribe_data = new Object();
  subscribe_data.retval = "";
  subscribe_data.url = url;
  subscribe_data.readable_name = readable_name;
  // Open the window
  SBScanServiceTreeNewEntryEditable();
  SBOpenModalDialog( "chrome://songbird/content/xul/subscribe.xul", "", "chrome,modal=yes,centerscreen", subscribe_data );
  if ( subscribe_data.retval == "ok" )
  {
    if ( guid && table )
    {
      const PlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");
      var aPlaylistManager = new PlaylistManager();
      aPlaylistManager = aPlaylistManager.QueryInterface(Components.interfaces.sbIPlaylistManager);
      var aDBQuery = new sbIDatabaseQuery();
      aDBQuery.setAsyncQuery(false);
      aDBQuery.setDatabaseGUID(guid);
      aPlaylistManager.deletePlaylist( table, aDBQuery );
    }

    SBScanServiceTreeNewEntryStart();
  }
}

function About( )
{
  // Make a magic data object to get passed to the dialog
  var about_data = new Object();
  about_data.retval = "";
  // Open the modal dialog
  SBOpenModalDialog( "chrome://songbird/content/xul/about.xul", "about", "chrome,modal=yes,centerscreen", about_data );
  if ( about_data.retval == "ok" )
  {
  }  
}

// Debugging Tool
function listProperties(obj, objName) 
{
    var columns = 3;
    var count = 0;
    var result = "";
    for (var i in obj) 
    {
        result += objName + "." + i + " = " + obj[i] + "\t\t\t";
        count = ++count % columns;
        if ( count == columns - 1 )
        {
          result += "\n";
        }
    }
    alert(result);
}

var SBDropObserver = 
{
  getSupportedFlavours : function () 
  {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage("get flavours");
    var flavours = new FlavourSet();
    flavours.appendFlavour("application/x-moz-file","nsIFile");
//    flavours.appendFlavour("application/x-moz-url");
    return flavours;
  },
  onDragOver: function ( evt, flavour, session )
  {
  },
  onDrop: function ( evt, dropdata, session )
  {
    if ( dropdata.data != "" )
    {
      // if it has a path property
      if ( dropdata.data.path )
      {
        theDropPath = dropdata.data.path;
        theDropIsDir = dropdata.data.isDirectory();
        setTimeout( SBDropped, 10 ); // Next frame
        
      }
    }
  }
};

var theDropPath = "";
var theDropIsDir = false;
function SBDropped()
{
  if ( theDropIsDir )
  {
    theMediaScanIsOpen.boolValue = true;
    // otherwise, fire off the media scan page.
    var media_scan_data = new Object();
    media_scan_data.URL = theDropPath;
    media_scan_data.retval = "";
    // Open the non-modal dialog
    SBOpenModalDialog( "chrome://songbird/content/xul/media_scan.xul", "media_scan", "chrome,modal=yes,centerscreen", media_scan_data );
    theMediaScanIsOpen.boolValue = false;
  }
  else if ( gPPS.isMediaURL( theDropPath ) )
  {
    // add it to the db and play it.
    playExternalUrl(theDropPath, false);
  }
}

function SBGetServiceFromUrl( url, nodefault )
{
  var retval;
  if (!nodefault) retval = url;
  var text = "";
  try
  {
    var theServiceTree = document.getElementById( "frame_servicetree" );
    if ( theServiceTree && (url.indexOf("chrome://") == 0) ) // Only if it is a chrome url.
    {
      text += "tree\n";
      var theServiceTree_tree = theServiceTree.tree;
      if (theServiceTree_tree)
      {
        text += "tree_tree\n";
        // Find the columns. 
        var urlcolumn = theServiceTree_tree.columns ? theServiceTree_tree.columns["url"] : "url";
        var labelcolumn = theServiceTree_tree.columns ? theServiceTree_tree.columns["frame_service_tree_label"] : "frame_service_tree_label";
        
        for ( var i = 0; i < theServiceTree_tree.view.rowCount; i++ )
        {
          // Get the text of the hidden tree cell, this contains the url.
          var tree_url = theServiceTree_tree.view.getCellText( i, urlcolumn );
          var tree_label = theServiceTree_tree.view.getCellText( i, labelcolumn );
          
          if ( tree_url == url )
          {
            retval = tree_label;
            break;
          }
          text += "row " + tree_url + "\n";
        }
        text += "end\n";
      }
    }
  }
  catch ( err )
  {
    alert( "SBGetServiceFromUrl - " + err )
  }
  return retval;
}

function SBGetServiceImageFromUrl( url )
{
  retval = "";
  try
  {
    var theServiceTree = document.getElementById( "frame_servicetree" );
    if ( theServiceTree )
    {
      var theServiceTree_tree = theServiceTree.tree;
      if (theServiceTree_tree)
      {
        // Find the columns. 
        var urlcolumn = theServiceTree_tree.columns ? theServiceTree_tree.columns["url"] : "url";
        var labelcolumn = theServiceTree_tree.columns ? theServiceTree_tree.columns["frame_service_tree_label"] : "frame_service_tree_label";
        
        for ( var i = 0; i < theServiceTree_tree.view.rowCount; i++ )
        {
          // Get the text of the hidden tree cell, this contains the url.
          var tree_url = theServiceTree_tree.view.getCellText( i, urlcolumn );
          var tree_image = theServiceTree_tree.view.getImageSrc( i, labelcolumn );
          
          if ( tree_url == url )
          {
            //
            // Uhhhh, what do we do for images that come from the skin?!
            if ( tree_image == "" )
            {
              // For now, use a default.  Assume it's playlisty.
              tree_image = "chrome://songbird/skin/default/icon_lib_16x16.png";
            }
          
            retval = tree_image;
            break;
          }
        }
      }
    }
  }
  catch ( err )
  {
    alert( "SBGetServiceImageFromUrl - " + err )
  }
  return retval;
}

function SBGetUrlFromService( service )
{
  retval = service;
  try
  {
    var theServiceTree = document.getElementById( "frame_servicetree" );
    if ( theServiceTree )
    {
      var theServiceTree_tree = theServiceTree.tree;
      if (theServiceTree_tree)
      {
        // Find the columns. 
        var urlcolumn = theServiceTree_tree.columns ? theServiceTree_tree.columns["url"] : "url";
        var labelcolumn = theServiceTree_tree.columns ? theServiceTree_tree.columns["frame_service_tree_label"] : "frame_service_tree_label";
        
        for ( var i = 0; i < theServiceTree_tree.view.rowCount; i++ )
        {
          // Get the text of the hidden tree cell, this contains the url.
          var tree_url = theServiceTree_tree.view.getCellText( i, urlcolumn );
          var tree_label = theServiceTree_tree.view.getCellText( i, labelcolumn );
          
          if ( tree_label == service )
          {
            retval = tree_url;
            break;
          }
        }
      }
    }
  }
  catch ( err )
  {
    alert( "SBGetUrlFromService - " + err )
  }
  return retval;
}

function SBTabcompleteService( service )
{
  retval = service;
  var service_lc = service.toLowerCase();
  try
  {
    var theServiceTree = document.getElementById( "frame_servicetree" );
    if ( theServiceTree )
    {
      var theServiceTree_tree = theServiceTree.tree;
      if (theServiceTree_tree)
      {
        // Find the columns. 
        var urlcolumn = theServiceTree_tree.columns ? theServiceTree_tree.columns["url"] : "url";
        var labelcolumn = theServiceTree_tree.columns ? theServiceTree_tree.columns["frame_service_tree_label"] : "frame_service_tree_label";

        var found_one = false;      
        for ( var i = 0; i < theServiceTree_tree.view.rowCount; i++ )
        {
          // Get the text of the hidden tree cell, this contains the url.
          var tree_label = theServiceTree_tree.view.getCellText( i, labelcolumn );
          
          var label_lc = tree_label.toLowerCase();
          
          // If we are the beginning of the label string
          if ( label_lc.indexOf( service_lc ) == 0 )
          {
            if ( found_one )
            {
              retval = service; // only find ONE!
              break;
            }
            else
            {
              found_one = true; // only find ONE!
              retval = tree_label;
            }
          }
        }
      }
    }
  }
  catch ( err )
  {
    alert( "SBTabcompleteService - " + err )
  }
  return retval;
}


// Assume there's just one?
var theDownloadContext = SB_NewDataRemote( "download.context", null );
var theDownloadTable = SB_NewDataRemote( "download.table", null );
var theDownloadExists = SB_NewDataRemote( "browser.hasdownload", null );

/*
var theDownloadListener = 
{
  m_queryObj: null,
  m_libraryObj: null,
  
  CreateQueryObj: function()
  {
    this.m_queryObj = new sbIDatabaseQuery();
    this.m_queryObj.setAsyncQuery(true);
    this.m_queryObj.setDatabaseGUID("songbird");
  },

  CreateLibraryObj: function()
  {
    if(this.m_libraryObj == null)
    {
      const MediaLibrary = new Components.Constructor("@songbirdnest.com/Songbird/MediaLibrary;1", "sbIMediaLibrary");
      this.m_libraryObj = (new MediaLibrary()).QueryInterface(Components.interfaces.sbIMediaLibrary);
    
      if(this.m_queryObj == null)
          this.CreateQueryObj();
        
      this.m_libraryObj.setQueryObject(this.m_queryObj);
    }
  },
  
  QueryInterface : function(aIID) 
  {
    if (!aIID.equals(Components.interfaces.sbIDeviceBaseCallback) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
  },
  
  onTransferStart: function(sourceURL, destinationURL)
  {
  },
  
  onTransferComplete: function(sourceURL, destinationURL, transferStatus)
  {
    if(transferStatus == 1)
    {
      this.CreateLibraryObj(); 
      
      var aKeys = ["title"];
      var aValues = [];
      
      var aLocalFile = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
      aLocalFile.initWithPath(destinationURL);
    
      aValues.push(aLocalFile.leafName);
      this.m_libraryObj.addMedia(destinationURL, aKeys.length, aKeys, aValues.length, aValues, false, false);
    }
  }
};
*/

function onBrowserTransfer(guid, table, strFilterColumn, nFilterValueCount, aFilterValues)
{
    try
    {
        theWebPlaylistQuery = null; 
          
        deviceManager = Components.classes["@songbirdnest.com/Songbird/DeviceManager;1"].
                                    getService(Components.interfaces.sbIDeviceManager);
        if (deviceManager)
        {
            var downloadCategory = 'Songbird Download Device';
            if (deviceManager.hasDeviceForCategory(downloadCategory))
            {
                var downloadDevice =
                  deviceManager.getDeviceByCategory(downloadCategory);
                
                // Make a magic data object to get passed to the dialog
                var download_data = new Object();
                download_data.retval = "";
                download_data.value = SBDataGetStringValue( "download.folder" );
                
                if ( ( SBDataGetIntValue( "download.always" ) == 1 ) && ( download_data.value.length > 0 ) )
                {
                  download_data.retval = "ok";
                }
                else
                {
                  // Open the window
                  SBOpenModalDialog( "chrome://songbird/content/xul/download.xul", "", "chrome,modal=yes,centerscreen", download_data );
                }

                // Pick download destination
                if ( ( download_data.retval == "ok" ) && ( download_data.value.length > 0 ) )
                {
                  var downloadTable = {};
                  // Passing empty string for device name as download device has just one device
                  // Prepare table for download & get the name for newly prepared download table
                  //downloadDevice.addCallback(theDownloadListener);
                  
                  downloadDevice.autoDownloadTable('', guid, table, strFilterColumn, nFilterValueCount, aFilterValues, '', download_data.value, downloadTable);
                  
                  // Record the current download table
                  theDownloadContext.stringValue = downloadDevice.getContext('');
                  theDownloadTable.stringValue = downloadTable.value;
                  theDownloadExists.boolValue = true;
                  
                  // Register the guid and table with the playlist source to always show special download commands.
                  SBDownloadCommands.m_Device = downloadDevice;
                  var source = new sbIPlaylistsource();
                  source.registerPlaylistCommands( downloadDevice.getContext(''), downloadTable.value, "download", SBDownloadCommands );
                }
            }
        }
    }
    
    catch ( err )
    {
        alert( err );
    }
}

var SBDownloadCommands = 
{
  DEVICE_IDLE :               0,
  DEVICE_BUSY :               1,
  DEVICE_DOWNLOADING :        2,
  DEVICE_UPLOADING :          3,
  DEVICE_DOWNLOAD_PAUSED :    4,
  DEVICE_UPLOAD_PAUSED :      5,
  DEVICE_DELETING :           6,
  
  m_Playlist: null,
  m_Device: null,

  m_Ids: new Array
  (
    "library_cmd_play",
    "library_cmd_remove",
    "library_cmd_pause",
    "*separator*",
    "library_cmd_showwebplaylist"
  ),
  
  m_Names: new Array
  (
    "&command.play",
    "&command.remove",
    "&command.pausedl",
    "*separator*",
    "&command.showwebplaylist"
  ),
  
  m_Tooltips: new Array
  (
    "&command.tooltip.play",
    "&command.tooltip.remove",
    "&command.tooltip.pause",
    "*separator*",
    "&command.tooltip.showwebplaylist"
  ),

  getNumCommands: function()
  {
    if ( 
        ( this.m_Tooltips.length != this.m_Ids.length ) ||
        ( this.m_Names.length != this.m_Ids.length ) ||
        ( this.m_Tooltips.length != this.m_Names.length )
       )
    {
      alert( "PlaylistCommands - Array lengths do not match!" );
      return 0;
    }
    return this.m_Ids.length;
  },

  getCommandId: function( index )
  {
    // Ah! magic number - what does it mean???
    if ( index == 2 ) 
    {
      if ( this.m_Device )
      {
        if ( this.m_Device.getDeviceState('') == this.DEVICE_DOWNLOAD_PAUSED )
        {
          this.m_Ids[ index ] = "library_cmd_resume";
        }
        else
        {
          this.m_Ids[ index ] = "library_cmd_pause";
        }
      }
    }
    if ( index >= this.m_Ids.length )
    {
      return "";
    }
    return this.m_Ids[ index ];
  },

  getCommandText: function( index )
  {
    if ( index == 2 )
    {
      if ( this.m_Device )
      {
        if ( this.m_Device.getDeviceState('') == this.DEVICE_DOWNLOAD_PAUSED )
        {
          this.m_Names[ index ] = "&command.resumedl";
        }
        else
        {
          this.m_Names[ index ] = "&command.pausedl";
        }
      }
    }
    if ( index >= this.m_Names.length )
    {
      return "";
    }
    return this.m_Names[ index ];
  },

  getCommandFlex: function( index )
  {
    if ( this.m_Ids[ index ] == "*separator*" ) return 1;
    return 0;
  },

  getCommandToolTipText: function( index )
  {
    if ( index == 2 )
    {
      if ( this.m_Device )
      {
        if ( this.m_Device.getDeviceState('') == this.DEVICE_DOWNLOAD_PAUSED )
        {
          this.m_Tooltips[ index ] = "&command.tooltip.resume";
        }
        else
        {
          this.m_Tooltips[ index ] = "&command.tooltip.pause";
        }
      }
    }
    if ( index >= this.m_Tooltips.length )
    {
      return "";
    }
    return this.m_Tooltips[ index ];
  },

  getCommandEnabled: function( index )
  {
    var retval = false;
    if ( this.m_Device )
    {
      switch( index )
      {
        case 2:
          retval = ( this.m_Device.getDeviceState('') == this.DEVICE_DOWNLOADING ) || ( this.m_Device.getDeviceState('') == this.DEVICE_DOWNLOAD_PAUSED )
        break;
        default:
          retval = true;
        break;
      }
    }
    return retval;
  },

  onCommand: function( event )
  {
    if ( this.m_Device && event.target && event.target.id )
    {
      // Was it from the toolbarbutton?
      var tbb = ( event.target.tagName == "button" || event.target.tagName == "xul:button" );
      switch( event.target.id )
      {
        case "library_cmd_play":
          if ( this.m_Playlist.tree.currentIndex != -1 )
          {
            // Repurpose the command to act as if a doubleclick
            this.m_Playlist.sendPlayEvent();
          }
        break;
        case "library_cmd_remove":
          if ( this.m_Playlist.tree.currentIndex != -1 )
          {
            // remove the currently select tracks
            this.m_Playlist.removeTracks();
          }
        break;
        case "library_cmd_pause":
        case "library_cmd_resume":
          if ( this.m_Device.getDeviceState('') == this.DEVICE_DOWNLOADING )
          {
            this.m_Device.suspendTransfer('');
          }
          else if ( this.m_Device.getDeviceState('') == this.DEVICE_DOWNLOAD_PAUSED )
          {
            this.m_Device.resumeTransfer('');
          }
          // Since we changed state, update the command buttons.
          this.m_Playlist.refreshCommands();
        break;
        case "library_cmd_showwebplaylist":
        {
          onBrowserPlaylist();
        }
        break;
      }
      event.stopPropagation();
    }
  },
  
  // The object registered with the sbIPlaylistSource interface acts 
  // as a template for instances bound to specific playlist elements
  duplicate: function()
  {
    var obj = {};
    for ( var i in this )
    {
      obj[ i ] = this[ i ];
    }
    return obj;
  },
  
  setPlaylist: function( playlist )
  {
    // Ah.  Sometimes, things are being secure.
    if ( playlist.wrappedJSObject )
      playlist = playlist.wrappedJSObject;
    this.m_Playlist = playlist;
  },
  
  QueryInterface : function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbIPlaylistCommands) &&
        !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    
    return this;
  }

}; // SBDownloadCommands definition

try
{
  // Register the download commands at startup if we know what the download table is.
  var deviceManager = Components.classes["@songbirdnest.com/Songbird/DeviceManager;1"].
                                  getService(Components.interfaces.sbIDeviceManager);

  if (deviceManager)
  {
    var downloadCategory = 'Songbird Download Device';
    if (deviceManager.hasDeviceForCategory(downloadCategory))
    {
      var downloadDevice =
        deviceManager.getDeviceByCategory(downloadCategory);
      SBDownloadCommands.m_Device = downloadDevice;
      var guid = downloadDevice.getContext('');
      var table = "download"; // downloadDevice.GetTransferTableName('');
      var source = new sbIPlaylistsource();
      try
      {
        source.registerPlaylistCommands( guid, table, "download", SBDownloadCommands );
      }
      catch ( err )
      {
        alert( "source.registerPlaylistCommands( " + guid+ ", " + table+ " );\r\n" + err )
      }
    }
  }
} catch(e) {}


var SBCDCommands = 
{
  DEVICE_IDLE :               0,
  DEVICE_BUSY :               1,
  DEVICE_DOWNLOADING :        2,
  DEVICE_UPLOADING :          3,
  DEVICE_DOWNLOAD_PAUSED :    4,
  DEVICE_UPLOAD_PAUSED :      5,
  DEVICE_DELETING :           6,
  
  m_Playlist: null,
  m_Device: null,
  m_Context: null, 
  m_Table: null,
  m_DeviceName: null,

  m_Ids: new Array
  (
    "library_cmd_play",
    "library_cmd_rip",
    "library_cmd_edit"
  ),
  
  m_Names: new Array
  (
    "&command.play",
    "&command.rip",
    "&command.edit"
  ),
  
  m_Tooltips: new Array
  (
    "&command.tooltip.play",
    "&command.tooltip.rip",
    "&command.tooltip.edit"
  ),

  getNumCommands: function()
  {
    if ( 
        ( this.m_Tooltips.length != this.m_Ids.length ) ||
        ( this.m_Names.length != this.m_Ids.length ) ||
        ( this.m_Tooltips.length != this.m_Names.length )
       )
    {
      alert( "PlaylistCommands - Array lengths do not match!" );
      return 0;
    }
    return this.m_Ids.length;
  },

  getCommandId: function( index )
  {
    if ( index >= this.m_Ids.length )
    {
      return "";
    }
    return this.m_Ids[ index ];
  },

  getCommandText: function( index )
  {
    if ( index >= this.m_Names.length )
    {
      return "";
    }
    return this.m_Names[ index ];
  },

  getCommandFlex: function( index )
  {
    if ( this.m_Ids[ index ] == "*separator*" ) return 1;
    return 0;
  },


  getCommandToolTipText: function( index )
  {
    if ( index >= this.m_Tooltips.length )
    {
      return "";
    }
    return this.m_Tooltips[ index ];
  },

  getCommandEnabled: function( index )
  {
    var retval = false;
    if ( this.m_Device )
    {
      switch( index )
      {
        case 0:
        case 1:
        case 2:
          retval = true;
        break;
      }
    }
    return retval;
  },

  onCommand: function( event )
  {
    if ( this.m_Device && event.target && event.target.id )
    {
      // Was it from the toolbarbutton?
      var tbb = ( event.target.tagName == "button" || event.target.tagName == "xul:button" );
      switch( event.target.id )
      {
        case "library_cmd_play":
          if ( this.m_Playlist.tree.currentIndex != -1 )
          {
            // Repurpose the command to act as if a doubleclick
            this.m_Playlist.sendPlayEvent();
          }
        break;
        case "library_cmd_rip":
          if ( this.m_Playlist.tree.currentIndex != -1 )
          {
            try
            {        
                var filterCol = "uuid";
                var filterVals = new Array();
                
                var columnObj = this.m_Playlist.tree.columns.getNamedColumn(filterCol);
                var rangeCount = this.m_Playlist.tree.view.selection.getRangeCount();
                for (var i=0; i < rangeCount; i++) 
                {
                    var start = {};
                    var end = {};
                    this.m_Playlist.tree.view.selection.getRangeAt( i, start, end );
                    for( var c = start.value; c <= end.value; c++ )
                    {
                        if (c >= this.m_Playlist.tree.view.rowCount) 
                        {
                        continue; 
                        }
                        
                        var value = this.m_Playlist.tree.view.getCellText(c, columnObj);
                        
                        filterVals.push(value);
                    }
                }
                onCDRip( this.m_DeviceName, this.m_Context, this.m_Table, filterCol, filterVals.length, filterVals, this.m_Device);
                // And show the download table in the chrome playlist.
                //onBrowserDownload();
            }
            catch( err )          
            {
                alert( err );
            }
          }
        break;
        case "library_cmd_edit":
        break;
      }
      event.stopPropagation();
    }
  },
  
  // The object registered with the sbIPlaylistSource interface acts 
  // as a template for instances bound to specific playlist elements
  duplicate: function()
  {
    var obj = {};
    for ( var i in this )
    {
      obj[ i ] = this[ i ];
    }
    return obj;
  },
  
  setPlaylist: function( playlist )
  {
    // Ah.  Sometimes, things are being secure.
    if ( playlist.wrappedJSObject )
      playlist = playlist.wrappedJSObject;
    this.m_Playlist = playlist;
  },
  
  QueryInterface : function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbIPlaylistCommands) &&
        !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    
    return this;
  }

}; // SBCDCommands definition

var SBRippingCommands = 
{
  DEVICE_IDLE :               0,
  DEVICE_BUSY :               1,
  DEVICE_DOWNLOADING :        2,
  DEVICE_UPLOADING :          3,
  DEVICE_DOWNLOAD_PAUSED :    4,
  DEVICE_UPLOAD_PAUSED :      5,
  DEVICE_DELETING :           6,
  
  m_Playlist: null,
  m_Device: null,
  m_DeviceName: null,

  m_Ids: new Array
  (
    "library_cmd_remove"
  ),
  
  m_Names: new Array
  (
    "&command.remove"
  ),
  
  m_Tooltips: new Array
  (
    "&command.tooltip.remove"
  ),

  getNumCommands: function()
  {
    if ( 
        ( this.m_Tooltips.length != this.m_Ids.length ) ||
        ( this.m_Names.length != this.m_Ids.length ) ||
        ( this.m_Tooltips.length != this.m_Names.length )
       )
    {
      alert( "PlaylistCommands - Array lengths do not match!" );
      return 0;
    }
    return this.m_Ids.length;
  },

  getCommandId: function( index )
  {
    if ( index >= this.m_Ids.length )
    {
      return "";
    }
    return this.m_Ids[ index ];
  },

  getCommandText: function( index )
  {
    if ( index >= this.m_Names.length )
    {
      return "";
    }
    return this.m_Names[ index ];
  },

  getCommandFlex: function( index )
  {
    if ( this.m_Ids[ index ] == "*separator*" ) return 1;
    return 0;
  },

  getCommandToolTipText: function( index )
  {
    if ( index >= this.m_Tooltips.length )
    {
      return "";
    }
    return this.m_Tooltips[ index ];
  },

  getCommandEnabled: function( index )
  {
    var retval = false;
    if ( this.m_Device )
    {
      switch( index )
      {
        default:
          retval = true;
        break;
      }
    }
    return retval;
  },

  onCommand: function( event )
  {
    if ( this.m_Device && event.target && event.target.id )
    {
      // Was it from the toolbarbutton?
      var tbb = ( event.target.tagName == "button" || event.target.tagName == "xul:button" );
      switch( event.target.id )
      {
        case "library_cmd_remove":
          if ( this.m_Playlist.tree.currentIndex != -1 )
          {
            // remove the currently select tracks
            this.m_Playlist.removeTracks();
          }
        break;
      }
      event.stopPropagation();
    }
  },
  
  // The object registered with the sbIPlaylistSource interface acts 
  // as a template for instances bound to specific playlist elements
  duplicate: function()
  {
    var obj = {};
    for ( var i in this )
    {
      obj[ i ] = this[ i ];
    }
    return obj;
  },
  
  setPlaylist: function( playlist )
  {
    // Ah.  Sometimes, things are being secure.
    if ( playlist.wrappedJSObject )
      playlist = playlist.wrappedJSObject;
    this.m_Playlist = playlist;
  },
  
  QueryInterface : function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbIPlaylistCommands) &&
        !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    
    return this;
  }

}; // SBRippingCommands definition


function onCDRip(deviceName, guid, table, strFilterColumn, nFilterValueCount, aFilterValues, aCDDevice)
{
    try
    {
        theWebPlaylistQuery = null; 
          
        if (aCDDevice)
        {
            // Make a magic data object to get passed to the dialog
            var ripping_data = new Object();
            ripping_data.retval = "";
            ripping_data.value = SBDataGetStringValue( "ripping.folder" );
            
            if ( ( SBDataGetIntValue( "ripping.always" ) == 1 ) && ( ripping_data.value.length > 0 ) )
            {
                ripping_data.retval = "ok";
            }
            else
            {
                // Open the window
                SBOpenModalDialog( "chrome://songbird/content/xul/download.xul", "", "chrome,modal=yes,centerscreen", ripping_data );
            }

            // Pick download destination
            if ( ( ripping_data.retval == "ok" ) && ( ripping_data.value.length > 0 ) )
            {
                var rippingTable = {};
                // Passing empty string for device name as download device has just one device
                // Prepare table for download & get the name for newly prepared download table
                //downloadDevice.addCallback(theDownloadListener);
                
                aCDDevice.autoDownloadTable(deviceName, guid, table, strFilterColumn, nFilterValueCount, aFilterValues, '', ripping_data.value, rippingTable);
                
                // Register the guid and table with the playlist source to always show special download commands.
                SBRippingCommands.m_Device = aCDDevice;
                SBRippingCommands.m_DeviceName = deviceName;
                var source = new sbIPlaylistsource();
                source.registerPlaylistCommands( guid, rippingTable.value, "download", SBRippingCommands );
            }
        }
    }
    
    catch ( err )
    {
        alert( err );
    }
}

var theCDListener = 
{
  m_queryObj: null,
  m_libraryObj: null,
  
  CreateQueryObj: function()
  {
    this.m_queryObj = new sbIDatabaseQuery();
    this.m_queryObj.setAsyncQuery(true);
    this.m_queryObj.setDatabaseGUID("songbird");
  },

  CreateLibraryObj: function()
  {
    if(this.m_libraryObj == null)
    {
      const MediaLibrary = new Components.Constructor("@songbirdnest.com/Songbird/MediaLibrary;1", "sbIMediaLibrary");
      this.m_libraryObj = (new MediaLibrary()).QueryInterface(Components.interfaces.sbIMediaLibrary);
    
      if(this.m_queryObj == null)
          this.CreateQueryObj();
        
      this.m_libraryObj.setQueryObject(this.m_queryObj);
    }
  },
  
  QueryInterface : function(aIID) 
  {
    if (!aIID.equals(Components.interfaces.sbIDeviceBaseCallback) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
  },
  
  onTransferStart: function(sourceURL, destinationURL)
  {
  },
  
  onTransferComplete: function(sourceURL, destinationURL, transferStatus)
  {
  },
  
  onDeviceConnect: function(deviceName)
  {
    OnCDInsert(deviceName);
  },
  
  onDeviceDisconnect: function(deviceName)
  {
  }
};

function OnCDInsert(deviceName)
{
  if (deviceManager)
  {
    var CDCategory = 'Songbird CD Device';
    if (deviceManager.hasDeviceForCategory(CDCategory))
    {
      var aCDDevice =
        deviceManager.getDeviceByCategory(CDCategory);
      SBCDCommands.m_DeviceName = deviceName;
      SBCDCommands.m_Device = aCDDevice;
      var cdTable = {};
      var cdContext = {};
      aCDDevice.getTrackTable(SBCDCommands.m_DeviceName, cdContext, cdTable);
      cdContext = cdContext.value;
      cdTable = cdTable.value;
      SBCDCommands.m_Context = cdContext;
      SBCDCommands.m_Table = cdTable;
      var source = new sbIPlaylistsource();
      if ( cdContext && cdTable )
      {
        try
        {
          source.registerPlaylistCommands( cdContext, cdTable, cdTable, SBCDCommands );
        }
        catch ( err )
        {
          alert( "source.registerPlaylistCommands( " + SBCDCommands.m_Context + ", " + SBCDCommands.m_Table+ " );\r\n" + err )
        }
      }
    }
  }
}


// Register for CD notifications
if (deviceManager)
{
    var CDCategory = 'Songbird CD Device';
    if (deviceManager.hasDeviceForCategory(CDCategory))
    {
        var aCDDevice = deviceManager.getDeviceByCategory(CDCategory);
        try
        {
            aCDDevice.addCallback(theCDListener);
            if (aCDDevice.deviceCount > 0)
            {
			  // Currently handling just the first device
			  OnCDInsert(aCDDevice.getDeviceStringByIndex(0));
            }
        }
        catch ( err )
        {
            alert( "aCDDevice.addCallback(theCDListener);\r\n" + err );
        }
    }
}


var SBCDBurningCommands = 
{
  DEVICE_IDLE :               0,
  DEVICE_BUSY :               1,
  DEVICE_DOWNLOADING :        2,
  DEVICE_UPLOADING :          3,
  DEVICE_DOWNLOAD_PAUSED :    4,
  DEVICE_UPLOAD_PAUSED :      5,
  DEVICE_DELETING :           6,
  
  m_Playlist: null,
  m_Device: null,
  m_DeviceName: null,
  m_TableName: null,

  m_Ids: new Array
  (
    "library_cmd_start",
    "library_cmd_stop"
  ),
  
  m_Names: new Array
  (
    "&command.start",
    "&command.stop"
  ),
  
  m_Tooltips: new Array
  (
    "&command.tooltip.start",
    "&command.tooltip.stop"
  ),

  getNumCommands: function()
  {
    if ( 
        ( this.m_Tooltips.length != this.m_Ids.length ) ||
        ( this.m_Names.length != this.m_Ids.length ) ||
        ( this.m_Tooltips.length != this.m_Names.length )
       )
    {
      alert( "PlaylistCommands - Array lengths do not match!" );
      return 0;
    }
    return this.m_Ids.length;
  },

  getCommandId: function( index )
  {
    if ( index >= this.m_Ids.length )
    {
      return "";
    }
    return this.m_Ids[ index ];
  },

  getCommandText: function( index )
  {
    if ( index >= this.m_Names.length )
    {
      return "";
    }
    return this.m_Names[ index ];
  },

  getCommandFlex: function( index )
  {
    if ( this.m_Ids[ index ] == "*separator*" ) return 1;
    return 0;
  },

  getCommandToolTipText: function( index )
  {
    if ( index >= this.m_Tooltips.length )
    {
      return "";
    }
    return this.m_Tooltips[ index ];
  },

  getCommandEnabled: function( index )
  {
    var retval = false;
    if ( this.m_Device )
    {
      switch( index )
      {
        default:
          retval = true;
        break;
      }
    }
    return retval;
  },

  onCommand: function( event )
  {
    if ( this.m_Device && event.target && event.target.id )
    {
      // Was it from the toolbarbutton?
      var tbb = ( event.target.tagName == "toolbarbutton" || event.target.tagName == "xul:toolbarbutton" );
      switch( event.target.id )
      {
        case "library_cmd_start":
            // start CD rip
            //onBrowserCDTransfer(this.m_Device, this.m_DeviceName, 0 );
            onStartCDBurn(this.m_Device, this.m_DeviceName, this.m_TableName);
        break;
        case "library_cmd_stop":
            // stop CD rip
            onStopCDBurn(this.m_DeviceName, this.m_Device);
        break;
      }
      event.stopPropagation();
    }
  },
  
  // The object registered with the sbIPlaylistSource interface acts 
  // as a template for instances bound to specific playlist elements
  duplicate: function()
  {
    var obj = {};
    for ( var i in this )
    {
      obj[ i ] = this[ i ];
    }
    return obj;
  },
  
  setPlaylist: function( playlist )
  {
    // Ah.  Sometimes, things are being secure.
    if ( playlist.wrappedJSObject )
      playlist = playlist.wrappedJSObject;
    this.m_Playlist = playlist;
  },
  
  QueryInterface : function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbIPlaylistCommands) &&
        !aIID.equals(Components.interfaces.nsISupportsWeakReference) &&
        !aIID.equals(Components.interfaces.nsISupports)) 
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    
    return this;
  }

};  // SBCDBurningCommands class definition

function onStartCDBurn(cdDevice, deviceName, table)
{
  try
  {
    if (cdDevice != null)
    {
      CheckCDAvailableForBurn();
      if (cdAvailableForWrite)
      {
        cdDevice.uploadTable(deviceName, table);
      }
    }
  }
  
  catch (err)
  {
    alert(err);
  }
}

var cdAvailableForWrite = 0;
var writableCDDeviceString = '';

function onAddToCDBurn(guid, table, strFilterColumn, nFilterValueCount, aFilterValues)
{
    try
    {
        theWebPlaylistQuery = null; 
        // deviceName is ignored for now and we ask cd device for the writable cd drive
        CheckCDAvailableForBurn();
        if (cdAvailableForWrite == 0)
           return; 
                
        var burnTable = {};
        
        // CD burning will be a two step process, user has to click the 'burn' button to initiate a CD burn
        aCDDevice.makeTransferTable(writableCDDeviceString, guid, table, strFilterColumn, nFilterValueCount, aFilterValues, '', '', false, burnTable);
        
        // Register the guid and table with the playlist source to always show special burn commands.
        SBCDBurningCommands.m_Device = aCDDevice;
        SBCDBurningCommands.m_DeviceName = writableCDDeviceString;
        SBCDBurningCommands.m_TableName = burnTable.value;
        var source = new sbIPlaylistsource();
        source.registerPlaylistCommands( aCDDevice.getContext(writableCDDeviceString), burnTable.value, burnTable.value, SBCDBurningCommands );

        // And show the download table in the chrome playlist.
        //onBrowserCDTransfer(aCDDevice, writableCDDeviceString, 0 /*Burning*/);
    }
    
    catch ( err )
    {
        alert( err );
    }
}

function CheckCDAvailableForBurn()
{
    cdAvailableForWrite = 0;
    var aCDDevice = null;
    deviceManager = Components.classes["@songbirdnest.com/Songbird/DeviceManager;1"].getService(Components.interfaces.sbIDeviceManager);
    if (deviceManager)
    {
        var CDCategory = 'Songbird CD Device';
        if (deviceManager.hasDeviceForCategory(CDCategory))
          aCDDevice = deviceManager.getDeviceByCategory(CDCategory);
    }

    if (!aCDDevice)
    {
        return;
    }
    aCDDevice = aCDDevice.QueryInterface(Components.interfaces.sbICDDevice);
    if (!aCDDevice)
    {
        return;
    }

    var temp = {};
    if (!aCDDevice.getWritableCDDrive(temp))
    {
        return;
    }
    writableCDDeviceString = temp.value;
    
    if (!aCDDevice.isUploadSupported(writableCDDeviceString))
    {
        return;
    }

    if (aCDDevice.getAvailableSpace(writableCDDeviceString) == 0)
    {
        return;
    }
    
	alert("Writable CD Found at " + writableCDDeviceString);
    cdAvailableForWrite = 1;
}

function onStopCDBurn(deviceName, aCDDevice)
{
  if (aCDDevice)
  {
    aCDDevice.abortTransfer(deviceName);
  }
}

function onBrowserCDTransfer(cdDevice, deviceString, ripping)
{  
  if  (ripping == 1)
	metrics_inc("player", "cd ripping", null);
  else
	metrics_inc("player", "cd burning", null);
    
  // Work to figure out guid and table
  var guid = cdDevice.getContext(deviceString);
  var table;
  if (ripping)
    table = cdDevice.getDownloadTable(deviceString);
  else
    table = cdDevice.getUploadTable(deviceString);

  // Actual functionality
  if ( ! thePlaylistTree )
  {
    // Errrr, nope?
    if ( ( guid == "" ) || ( table == "" ) )
    {
      return;
    }

    if ( theWebPlaylist.ref != ( "NC:" + guid + "_" + table ) )
    {
      if (ripping)
        theWebPlaylist.bind( guid, table, null, SBCDRippingCommands, SBDataGetIntValue( "browser.playlist.height" ), SBDataGetBoolValue( "browser.playlist.collapsed" ) );
      else
        theWebPlaylist.bind( guid, table, null, SBCDBurningCommands, SBDataGetIntValue( "browser.playlist.height" ), SBDataGetBoolValue( "browser.playlist.collapsed" ) );
    }
    
    // Show/hide them
    theShowWebPlaylistData.SetValue( true );
  }
  else
  {
    var theServiceTree = document.getElementById( 'frame_servicetree' );
    theServiceTree.launchURL( "chrome://songbird/content/xul/playlist_test.xul?" + table + "," + guid );
  }

}// END

}
catch ( err )
{
  alert( err );
}

// alert( "success!" );
// END

/**
 * Opens the update manager and checks for updates to the application.
 */
function checkForUpdates()
{
  var um = 
      Components.classes["@mozilla.org/updates/update-manager;1"].
      getService(Components.interfaces.nsIUpdateManager);
  var prompter = 
      Components.classes["@mozilla.org/updates/update-prompt;1"].
      createInstance(Components.interfaces.nsIUpdatePrompt);

  // If there's an update ready to be applied, show the "Update Downloaded"
  // UI instead and let the user know they have to restart the browser for
  // the changes to be applied. 
  if (um.activeUpdate && um.activeUpdate.state == "pending")
    prompter.showUpdateDownloaded(um.activeUpdate);
  else
    prompter.checkForUpdates();
}

function buildHelpMenu()
{
  var updates = 
      Components.classes["@mozilla.org/updates/update-service;1"].
      getService(Components.interfaces.nsIApplicationUpdateService);
  var um = 
      Components.classes["@mozilla.org/updates/update-manager;1"].
      getService(Components.interfaces.nsIUpdateManager);

  // Disable the UI if the update enabled pref has been locked by the 
  // administrator or if we cannot update for some other reason
  var checkForUpdates = document.getElementById("updateCmd");
  var canUpdate = updates.canUpdate;
  checkForUpdates.setAttribute("disabled", !canUpdate);
  if (!canUpdate)
    return; 

  var strings = document.getElementById("songbird_strings");
  var activeUpdate = um.activeUpdate;
  
  // If there's an active update, substitute its name into the label
  // we show for this item, otherwise display a generic label.
  function getStringWithUpdateName(key) {
    if (activeUpdate && activeUpdate.name)
      return strings.getFormattedString(key, [activeUpdate.name]);
    return strings.getString(key + "Fallback");
  }
  
  // By default, show "Check for Updates..."
  var key = "default";
  if (activeUpdate) {
    switch (activeUpdate.state) {
    case "downloading":
      // If we're downloading an update at present, show the text:
      // "Downloading Firefox x.x..." otherwise we're paused, and show
      // "Resume Downloading Firefox x.x..."
      key = updates.isDownloading ? "downloading" : "resume";
      break;
    case "pending":
      // If we're waiting for the user to restart, show: "Apply Downloaded
      // Updates Now..."
      key = "pending";
      break;
    }
  }
  checkForUpdates.label = getStringWithUpdateName("updateCmd_" + key);
}

