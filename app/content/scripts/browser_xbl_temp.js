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

// This is a temporary file to house methods that need to roll into
// our new Tabbed Browser XBL object that we'll be building for 0.3

var WEB_PLAYLIST_CONTEXT      = "webplaylist";
var WEB_PLAYLIST_TABLE        = "webplaylist";
var WEB_PLAYLIST_TABLE_NAME   = "&device.webplaylist";
var WEB_PLAYLIST_LIBRARY_NAME = "&device.weblibrary";




// onBrowserBack
function onBrowserBack()
{
  // Disable the "add to playlist" button until we see that there is anything to add.
  SBDataSetBoolValue("browser.canplaylist", false);
  gBrowser.showWebPlaylist = false;
  gBrowser.mainpane_listener_set = false;
  gBrowser.goBack();
}

// onBrowserFwd
function onBrowserFwd()
{
  // Disable the "add to playlist" button until we see that there is anything to add.
  SBDataSetBoolValue("browser.canplaylist", false);
  gBrowser.showWebPlaylist = false;
  gBrowser.mainpane_listener_set = false;
  gBrowser.goForward();
}

// onBrowserRefresh
function onBrowserRefresh()
{
  try
  {
    gBrowser.mainpane_listener_set = false;
    gBrowser.reload();
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
    gBrowser.mainpane_listener_set = false;
  }
  catch( err )
  {
    alert( err );
  }
}

// onBrowserHome
function onBrowserHome()
{
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
      
    gServicePane.loadURL(defaultHomepage, true);
  } catch(e) {}
}

// onBrowserBookmark
function onBrowserBookmark()
{
  bmManager.addBookmark()
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
    gBrowser.showWebPlaylist = true;
  }
  else
  {
    gServicePane.loadURL( "chrome://songbird/content/xul/playlist_test.xul?" +
                          WEB_PLAYLIST_TABLE + "," + WEB_PLAYLIST_CONTEXT);
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
    gBrowser.showWebPlaylist = true;
  }
  else
  {
    gServicePane.loadURL( "chrome://songbird/content/xul/playlist_test.xul?" + table + "," + guid);
  }
}




var thePlaylistTree;











function focusSearch() 
{
  var search_widget = document.getElementById( "search_widget" );
  search_widget.onFirstMousedown(); // Sets focus.  Clears "search" text.
}

