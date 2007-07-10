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
Components.utils.import("resource://app/components/sbProperties.jsm");

var PROFILE_TIME = false;
var theSongbirdLibrary = null;

var theTotalItems = 0;

var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);

var theTitle = document.getElementById( "songbird_media_scan_title" );
theTitle.value = "";

var theLabel = document.getElementById( "songbird_media_scan_label" );
theLabel.value = "";

var theProgress = document.getElementById( "songbird_media_scan" );
theProgress.setAttribute( "mode", "undetermined" );
theProgress.value = 0;

var theProgressValue = 0; // to 100;
var theProgressText = "";

var aFileScan = null;
var aFileScanQuery = null;

var theTargetDatabase = null;
var theTargetPlaylist = null;

// Init the text box to the last url played (shrug).
var polling_interval;

var timethen;
var timenow;
var start;

function onLoad()
{
  if ( ( typeof( window.arguments[0] ) != 'undefined' ) && ( typeof( window.arguments[0].URL ) != 'undefined' ) )
  {
    try
    {
      aFileScan = new sbIFileScan();
      aFileScanQuery = new sbIFileScanQuery();

      // Use the requested library, or use the main library as default      
      theTargetDatabase = window.arguments[0].target_db;
      if (!theTargetDatabase)
      {
        var libraryManager =
          Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                    .getService(Components.interfaces.sbILibraryManager);
        theTargetDatabase = libraryManager.mainLibrary.guid;
      }
      theTargetPlaylist = window.arguments[0].target_pl;

      if (aFileScan && aFileScanQuery)
      {
        aFileScanQuery.setDirectory(window.arguments[0].URL);
        aFileScanQuery.setRecurse(true);

        // Filter file extensions as part of the scan.        
        var eExtensions = gPPS.getSupportedFileExtensions();
        while (eExtensions.hasMore())
          aFileScanQuery.addFileExtension(eExtensions.getNext());

        aFileScan.submitQuery(aFileScanQuery);
        
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
    if ( !aFileScanQuery.isScanning() )
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
      var len = aFileScanQuery.getFileCount();
      if ( len )
      {
        var value = aFileScanQuery.getLastFileFound();
        theLabel.value = gPPS.convertURLToFolder(value);
      }
    }
  }
  catch ( err )  
  {
    alert( "onPollScan\n\n"+err );
  }
}

function sbBatchCreateListener(aArray) {
  this._array = aArray;
  this._length = aArray.length;
}

sbBatchCreateListener.prototype.onProgress = 
function sbBatchCreateListener_onProgress(aIndex)
{
  var fraction = aIndex / this._length;
  theTitle.value = SBString("media_scan.adding", "Adding") +
                            " (" + aIndex + "/" + this._length + ")";
  var uri = this._array.queryElementAt(aIndex, Components.interfaces.nsIURI);
  theLabel.value = gPPS.convertURLToDisplayName( uri.spec );
  theProgress.value = fraction * 100.0;
}

sbBatchCreateListener.prototype.onComplete = 
function sbBatchCreateListener_onComplete(aItemArray)
{
  if ( PROFILE_TIME )
  {
    timenow = new Date();
    start = timenow.getTime();
  }

  // The batch load is complete.
  theTitle.value = this._length + " " + SBString("media_scan.added", "Added");
  theLabel.value = SBString("media_scan.complete", "Complete");
  theProgress.value = 100.0;

  // TODO: Add items to theTargetPlaylist if it was requested.

  // Create and submit a metadata job for the
  // new items
  var metadataJobManager =
    Components.classes["@songbirdnest.com/Songbird/MetadataJobManager;1"]
              .getService(Components.interfaces.sbIMetadataJobManager);
  var metadataJob = metadataJobManager.newJob(aItemArray, 5);

  if ( PROFILE_TIME )
  {
    timethen = new Date();
    alert( "newJob - " + ( timethen.getTime() - start ) / 1000 + "s" );
    
    timenow = new Date();
    start = timenow.getTime();
  }

  // Enable the OK button
  document.getElementById("button_ok").removeAttribute( "disabled" );
}


function onScanComplete( )
{
  clearInterval( polling_interval );
  polling_interval = null;

  theProgress.removeAttribute( "mode" );

  if ( aFileScanQuery.getFileCount() )
  {
    try
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

      // Take the file array and turn it into an array of nsIURIs.
      var i = 0, total = 0;
      total = aFileScanQuery.getFileCount();
      var array =
        Components.classes["@mozilla.org/array;1"]
                  .createInstance(Components.interfaces.nsIMutableArray);
      var propsArray =
        Components.classes["@mozilla.org/array;1"]
                  .createInstance(Components.interfaces.nsIMutableArray);
      var ios = Components.classes["@mozilla.org/network/io-service;1"]
                          .getService(Components.interfaces.nsIIOService);

      for ( i = 0; i < total; i++ )
      {
        try {
          var uri = ios.newURI(aFileScanQuery.getFilePath( i ) , null, null);
          var props =
            Components.classes["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
                      .createInstance(Components.interfaces.sbIMutablePropertyArray);
          props.appendProperty(SBProperties.trackName,
                               createDefaultTitle(uri));
          array.appendElement(uri, false);
          propsArray.appendElement(props, false);
        }
        catch(e) {
          Components.utils.reportError("Error pasing file scan url " + e);
        }
      }
      theTotalItems = total;

      if ( PROFILE_TIME )
      {
        var timethen = new Date();
        alert( "Generate Array Loop - " + ( timethen.getTime() - start ) / 1000 + "s" );
        
        timenow = new Date();
        start = timenow.getTime();
      }

      // Start the async batch create
      var listener = new sbBatchCreateListener(array);
      theSongbirdLibrary.batchCreateMediaItemsAsync(listener, array, propsArray);

      if ( PROFILE_TIME )
      {
        timethen = new Date();
        alert( "Generate Query Loop - " + ( timethen.getTime() - start ) / 1000 + "s" );
        
        timenow = new Date();
        start = timenow.getTime();
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

function doOK()
{
  if ( polling_interval != null ) return false;

  if (document.getElementById("watch_check").checked) {
    // XXX need to port folder watcher
    // var wfManager = new CWatchFolderManager();
    // XXXredfive - componentize WatchFolderManager
    // wfManager.CreateWatchFolderManager();
    // wfManager.AddWatchFolder(aFileScanQuery.getDirectory()); 
  }
  document.defaultView.close();
  return true;
}
function doCancel()
{
  // Run away!!
  if (aFileScanQuery)
    aFileScanQuery.cancel();
  document.defaultView.close();
  return true;
}

function createDefaultTitle(aURI) {

  var netutil =
    Components.classes["@mozilla.org/network/util;1"]
              .getService(Components.interfaces.nsINetUtil);

  var s = netutil.unescapeString(aURI.spec,
                                 Components.interfaces.nsINetUtil.ESCAPE_ALL);
  var lastSlash = s.lastIndexOf("/");
  if (lastSlash >= 0) {
    s = s.substr(lastSlash + 1);
  }
  return s;
}

