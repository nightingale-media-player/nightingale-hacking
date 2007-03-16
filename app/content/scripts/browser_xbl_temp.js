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


var theCanGoBackData = SB_NewDataRemote( "browser.cangoback", null );
var theCanGoFwdData = SB_NewDataRemote( "browser.cangofwd", null );
var theCanAddToPlaylistData = SB_NewDataRemote( "browser.canplaylist", null );
var theBrowserUrlData = SB_NewDataRemote( "browser.url.text", null );
var theBrowserImageData = SB_NewDataRemote( "browser.url.image", null );
var theBrowserUriData = SB_NewDataRemote( "browser.uri", null );
var theShowWebPlaylistData = SB_NewDataRemote( "browser.playlist.show", null );



// onBrowserBack
function onBrowserBack()
{
  // Disable the "add to playlist" button until we see that there is anything to add.
  theCanAddToPlaylistData.boolValue = false;
  gBrowser.hidePlaylist();
  var theMainPane = document.getElementById( "frame_main_pane" );
  mainpane_listener_set = false;
  theMainPane.goBack();
}

// onBrowserFwd
function onBrowserFwd()
{
  // Disable the "add to playlist" button until we see that there is anything to add.
  theCanAddToPlaylistData.boolValue = false;
  gBrowser.hidePlaylist();
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
    theShowWebPlaylistData.boolValue = true;
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
    theShowWebPlaylistData.boolValue = true;
  }
  else
  {
    gServicePane.loadURL( "chrome://songbird/content/xul/playlist_test.xul?" + table + "," + guid);
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
    theBrowserUrlData.stringValue = gServicePane.getURLName( value );
    var image = gServicePane.getURLImage( value );
//    if ( image.length )
    {
      theBrowserImageData.stringValue = image;
    }
    // And then go to the url.  Easy, no?
    window.gServicePane.loadURL(value);
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
    
/*  // Allow the user to try to add any link as a playlist.

    // Disable "Add as Playlist" if the url isn't a playlist (NOTE: any HTML url will go as playlist)
    disabled = "true";
    if ( gPPS.isPlaylistURL( theHTMLContextURL ) )
    {
      disabled = "false"
    }
    document.getElementById( "html.context.playlist" ).setAttribute( "disabled", disabled );
*/    
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
    if (document.__SEARCHWIDGET__) document.__SEARCHWIDGET__.loadSearchStringForCurrentUrl();
  }
}

function handleMediaURL( aURL, aShouldBeginPlayback, forcePlaylist )
{
  var retval = false;
  try
  {
    // Stick playlists in the service pane (for now).
    if ( forcePlaylist || gPPS.isPlaylistURL( aURL ) )
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
  handleMediaURL( GetHREFFromEvent(evt), true, false );
  evt.stopPropagation();
  evt.preventDefault();
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
        if ( !handleMediaURL(theHTMLContextURL, true, false) )
        {
          gServicePane.loadURL( theHTMLContextURL );
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
        handleMediaURL(theHTMLContextURL, true, false);
      break;
      case "html.context.add":
        gPPS.importURL(theHTMLContextURL);
      break;
      case "html.context.playlist":
        // Add playlists to the service pane (force it as a playlist)
        handleMediaURL(theHTMLContextURL, false, true);
      break;
      case "html.context.copytoclipboard":
        var clipboard = Components.classes["@mozilla.org/widget/clipboardhelper;1"].getService(Components.interfaces.nsIClipboardHelper);
        clipboard.copyString(theHTMLContextURL);
      break;
    }
    theHTMLContextURL = null; // clear it because now we're done.
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
