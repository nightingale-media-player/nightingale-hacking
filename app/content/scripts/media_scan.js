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

var PROFILE_TIME = false;
var USE_NEW_API = true;
var theGUIDSArray = {};
var theSongbirdLibrary = null;

var theTotalItems = 0;

const MediaLibrary = new Components.Constructor("@songbirdnest.com/Songbird/MediaLibrary;1", "sbIMediaLibrary");
var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);

var aMediaLibrary = null;
var msDBQuery = new sbIDatabaseQuery();
var polling_interval = null;

var aQueryFileArray = new Array();

var theTitle = document.getElementById( "songbird_media_scan_title" );
theTitle.value = "";

var theLabel = document.getElementById( "songbird_media_scan_label" );
theLabel.value = "";

var theProgress = document.getElementById( "songbird_media_scan" );
theProgress.setAttribute( "mode", "undetermined" );
theProgress.value = 0;

var theProgressValue = 0; // to 100;
var theProgressText = "";

var aMediaScan = null;
var aMediaScanQuery = null;

var theTargetDatabase = null;
var theTargetPlaylist = null;
var theAddedGuids = null;

// Init the text box to the last url played (shrug).
var polling_interval;


function onLoad()
{
  if ( ( typeof( window.arguments[0] ) != 'undefined' ) && ( typeof( window.arguments[0].URL ) != 'undefined' ) )
  {
    try
    {
      aMediaScan = new sbIMediaScan();
      aMediaScanQuery = new sbIMediaScanQuery();
      
      theTargetDatabase = window.arguments[0].target_db;
      if (!theTargetDatabase || theTargetDatabase == "") theTargetDatabase = (USE_NEW_API) ? "main@library.songbirdnest.com" : "songbird";
      theTargetPlaylist = window.arguments[0].target_pl;
      theAddedGuids = Array();

      if (aMediaScan && aMediaScanQuery)
      {
        aMediaScanQuery.setDirectory(window.arguments[0].URL);
        aMediaScanQuery.setRecurse(true);

        // Filter file extensions as part of the scan.        
        var eExtensions = gPPS.getSupportedFileExtensions();
        while (eExtensions.hasMore())
          aMediaScanQuery.addFileExtension(eExtensions.getNext());

        aMediaScan.submitQuery(aMediaScanQuery);
        
        polling_interval = setInterval( onPollScan, 333 );
        
        theTitle.value = SBString("media_scan.scanning", "Scanning") + " -- " + window.arguments[0].URL;
      }
    }
    catch(err)
    {
      alert("onLoad\n\n"+err);
    }
  }
  return true;
}

function onPollScan()
{
  try
  {
    if ( !aMediaScanQuery.isScanning() )
    {
      clearInterval( polling_interval );

      theLabel.value = SBString("media_scan.composing", "Composing Query...");
      onScanComplete();
      document.getElementById("button_ok").removeAttribute( "hidden" );
      document.getElementById("button_ok").setAttribute( "disabled", "true" );
      document.getElementById("button_ok").focus();
      document.getElementById("button_cancel").setAttribute( "hidden", "true" );
    }   
    else
    {
      var len = aMediaScanQuery.getFileCount();
      if ( len )
      {
        var value = aMediaScanQuery.getLastFileFound();
        theLabel.value = gPPS.convertURLToFolder(value);
      }
    }
  }
  catch ( err )  
  {
    alert( "onPollScan\n\n"+err );
  }
}

function onScanComplete( )
{
  clearInterval( polling_interval );
  polling_interval = null;

  theProgress.removeAttribute( "mode" );

  if ( aMediaScanQuery.getFileCount() )
  {
    try
    {
      if ( USE_NEW_API )
      {
        if ( PROFILE_TIME )
        {
          timenow = new Date();
          start = timenow.getTime();
        }
              
        // Go get the library we're supposed to be scanning into.
        var libraryManager = Components.classes["@songbirdnest.com/Songbird/library/Manager;1"].
                              getService(Components.interfaces.sbILibraryManager);
        theSongbirdLibrary = libraryManager.getLibrary( theTargetDatabase );
        
        // Take the file array and turn it into a string array.
        var i = 0, count = 0, total = 0;
        total = aMediaScanQuery.getFileCount();
        var array = new Array();
        for ( i = 0, count = 0; i < total; i++ )
        {
          array.push( aMediaScanQuery.getFilePath( i ) );
        }
        theTotalItems = total;

        if ( PROFILE_TIME )
        {
          var timethen = new Date();
          alert( "Generate Array Loop - " + ( timethen.getTime() - start ) / 1000 + "s" );
          
          timenow = new Date();
          start = timenow.getTime();
        }
        
        // Ask the library to generate all the query strings to add the items to the library.
        msDBQuery = theSongbirdLibrary.QueryInterface(Components.interfaces.sbILocalDatabaseLibrary).batchCreateMediaItemsQuery( array.length, array, theGUIDSArray );

        if ( PROFILE_TIME )
        {
          timethen = new Date();
          alert( "Generate Query Loop - " + ( timethen.getTime() - start ) / 1000 + "s" );
          
          timenow = new Date();
          start = timenow.getTime();
        }

        // Run it asynchronously because it can take forever
        msDBQuery.setAsyncQuery(true);
        msDBQuery.execute();

        // And start up another polling loop to monitor the query.
        polling_interval = setInterval( onPollQuery, 333 );
      }
      else
      {
        aMediaLibrary = new MediaLibrary();
        aMediaLibrary = aMediaLibrary.QueryInterface( Components.interfaces.sbIMediaLibrary );

        if ( ! msDBQuery || ! aMediaLibrary )
        {
          return;
        }
        
        msDBQuery = new sbIDatabaseQuery();
        
        msDBQuery.resetQuery();
        msDBQuery.setAsyncQuery( true );
        msDBQuery.setDatabaseGUID( theTargetDatabase );

        aMediaLibrary.setQueryObject( msDBQuery );

        // Take the file array and for everything that seems to be a media url, add it to the database.
        var i = 0, count = 0, total = 0;
        total = aMediaScanQuery.getFileCount();

        msDBQuery.addQuery( "start" );

        for ( i = 0, count = 0; i < total; i++ )
        {
          var the_url = null;
          var is_url = null;
          the_url = aMediaScanQuery.getFilePath( i );

          var keys = new Array( "title" );
          var values = new Array();
          values.push( gPPS.convertURLToDisplayName( the_url ) );
          var guid = aMediaLibrary.addMedia( the_url, keys.length, keys, values.length, values, false, true );
          aQueryFileArray.push( values[0] );
          theAddedGuids.push( guid );
          
          if ( ( ( i + 1 ) % 500 ) == 0 )
          {
            msDBQuery.addQuery( "commit" );
            msDBQuery.addQuery( "start" );
          }
        }
        theTotalItems = total;;

        msDBQuery.addQuery( "commit" );
        
        count = msDBQuery.getQueryCount();
        if ( count )
        {
          theTitle.value = SBString("media_scan.adding", "Adding") + " (" + count + ")";
          theLabel.value = "";
          msDBQuery.execute();
          polling_interval = setInterval( onPollQuery, 333 );
        }
        else
        {
          theTitle.value = SBString("media_scan.none", "Nothing");
          onPollComplete();
        }
      }
    }
    catch(err)
    {
      alert("onScanComplete\n\n"+err);
    }
  }
  else
  {
    theTitle.value = SBString("media_scan.none", "Nothing");
  }
}

function onPollComplete() {
  clearInterval( polling_interval );
  polling_interval = null;
  
  document.getElementById("button_ok").removeAttribute( "disabled" );
  
  if ( USE_NEW_API ) {

    if ( PROFILE_TIME )
    {
      timethen = new Date();
      alert( "Execute Query Loop - " + ( timethen.getTime() - start ) / 1000 + "s" );
      
      timenow = new Date();
      start = timenow.getTime();
    }

    // Get the items from the guids
    var mediaItems = theSongbirdLibrary.batchGetMediaItems( theGUIDSArray.value );
    
    if ( PROFILE_TIME )
    {
      timethen = new Date();
      alert( "Media Items Loop - " + ( timethen.getTime() - start ) / 1000 + "s" );
      
      timenow = new Date();
      start = timenow.getTime();
    }
    
    // Tell the world they were added
    theSongbirdLibrary.batchNotifyAdded( mediaItems );

    if ( PROFILE_TIME )
    {
      timethen = new Date();
      alert( "Notify Loop - " + ( timethen.getTime() - start ) / 1000 + "s" );
      
      timenow = new Date();
      start = timenow.getTime();
    }

    // Create a metadata task    
    var metadataJobManager = Components.classes["@songbirdnest.com/Songbird/MetadataJobManager;1"]
                                 .getService(Components.interfaces.sbIMetadataJobManager);
    var metadataJob = metadataJobManager.newJob( mediaItems, 5 );
    
    if ( PROFILE_TIME )
    {
      timethen = new Date();
      alert( "Launch Metadata - " + ( timethen.getTime() - start ) / 1000 + "s" );
      
      timenow = new Date();
      start = timenow.getTime();
    }

    // TODO:
    //  - Add items to theTargetPlaylist if it was requested.
    
  } else {
    if (theTargetPlaylist && theAddedGuids && theAddedGuids.length > 0) {
      var PlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");
      var playlistManager = new PlaylistManager();
      playlistManager = playlistManager.QueryInterface(Components.interfaces.sbIPlaylistManager);
      msDBQuery.resetQuery();
      var thePlaylist;
      if (theTargetPlaylist != null) thePlaylist = playlistManager.getPlaylist(theTargetPlaylist, msDBQuery);
      if (thePlaylist) {
        for (var i=0;i<theAddedGuids.length;i++) {
          var guid = theAddedGuids[i];
          thePlaylist.addByGUID(guid, theTargetDatabase, -1, false, true);
        }
      }
      msDBQuery.execute();
      theAddedGuids = null;
    }
  }
}

var lastpos = 0;
function onPollQuery()
{
  try
  {
    if ( msDBQuery )
    {
      var len = msDBQuery.getQueryCount();
      var pos = msDBQuery.currentQuery() + 1;

      lastpos = pos;
      
      if ( ! msDBQuery.isExecuting() )
      {
        theTitle.value = theTotalItems + " " + SBString("media_scan.added", "Added");
        theLabel.value = SBString("media_scan.complete", "Complete");
        theProgress.value = 100.0;
        clearInterval( polling_interval );
        onPollComplete();
      }   
      else
      {
        var fraction = pos / len;
        var index = parseInt( theTotalItems * fraction );
        theTitle.value = SBString("media_scan.adding", "Adding") + " (" + index + "/" + theTotalItems + ")";
        theLabel.value = gPPS.convertURLToDisplayName( aMediaScanQuery.getFilePath( index ) );
        theProgress.value = fraction * 100.0;
      }
    }
  }
  catch ( err )
  {
    alert( "onPollQuery\n\n"+err );
  }
}

function doOK()
{
  if ( polling_interval != null ) return;

  if (document.getElementById("watch_check").checked) {
    var wfManager = new CWatchFolderManager();
    // XXXredfive - componentize WatchFolderManager
    wfManager.CreateWatchFolderManager();
    wfManager.AddWatchFolder(aMediaScanQuery.getDirectory()); 
  }
  document.defaultView.close();
  return true;
}
function doCancel()
{
  // Run away!!
  if (aMediaScanQuery)
    aMediaScanQuery.cancel();
  document.defaultView.close();
  return true;
}


