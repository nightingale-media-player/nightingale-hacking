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

    if (aState & nsIWebProgressListener.STATE_START) {
      // Start the spinner if necessary
      thePaneLoadingData.boolValue = true;
    }
    else if (aState & nsIWebProgressListener.STATE_STOP) {
      // Stop the spinner if necessary
      thePaneLoadingData.boolValue = false;

      // Notfiy that the page is done loading
      onMainPaneLoad();
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
      var cur_uri = aLocation.asciiSpec;
      // save url for reload on next servicetree init
      SBDataSetStringValue( "servicetree.selected_url", cur_uri );  
      // Set the value in the text box (shown or not)
      var servicepane = document.getElementById('servicepane');
      var theMainPane = document.getElementById( "frame_main_pane" );
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
      /*
      if ( ! theServiceTree.urlFromServicePane )
      {
        // Clear the service tree selection (asynchronously?  is this from out of thread?)
        setTimeout( "document.getElementById( 'frame_servicetree' ).tree.view.selection.currentIndex = -1;" +
                    "document.getElementById( 'frame_servicetree' ).tree.view.selection.clearSelection();",
                    50 );
      }
      theServiceTree.urlFromServicePane = false;
      */
      
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
      
      //theServiceTree.current_url = cur_uri;

      document.__SEARCHWIDGET__.loadSearchStringForCurrentUrl();
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

      // Kill the event since we've handled it
      evt.preventDefault();
    }
  }
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

      // Should the webplaylist scan and listener binding be performed?
      var enableWebPlaylist = SBDataGetBoolValue("webplaylist.enabled");
      
      // If we have not installed a playlist listener, and the web playlist
      // is enabled, install the link url listener.
      if ( !installed_listener && enableWebPlaylist)
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
          var playlist_guid = WEB_PLAYLIST_CONTEXT;
          var playlist_table = WEB_PLAYLIST_TABLE;
          // BIG HACK to show the subscribed playlist
          var cur_url = SBDataGetStringValue( "browser.uri" );
          var aPlaylistManager = new sbIPlaylistManager();
          var queryObj = new sbIDatabaseQuery();
          queryObj.setAsyncQuery(false);
          queryObj.setDatabaseGUID("songbird");      
          aPlaylistManager.getDynamicPlaylistList(queryObj);
          var resObj = queryObj.getResultObject();
          var i, rows = resObj.getRowCount();
          for ( i = 0; i < rows; i++ )
          {
            if ( cur_url == resObj.getRowCellByColumn( i, "description" ) )
            {
              var table = resObj.getRowCellByColumn( i, "name" );
              // Bind the Web Playlist UI element to the subscribed playlist instead of scraping.
              theWebPlaylist.bind( "songbird", table, null, SBDefaultCommands, SBDataGetIntValue( "browser.playlist.height" ), SBDataGetBoolValue( "browser.playlist.collapsed" ) );
              // Show/hide them
              SBDataSetBoolValue( "browser.playlist.show", true );
              playlist_guid = "songbird";
              playlist_table = table;
            }
          }
          // scrape the document.
          AsyncWebDocument( theMainPane.contentDocument, playlist_guid, playlist_table );
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
