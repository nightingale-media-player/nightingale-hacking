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
Components.utils.import("resource://app/components/sbProperties.jsm");

var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                             .getService(Components.interfaces.sbIPlaylistPlayback);

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

var theTargetLibrary = null;
var theTargetPlaylist = null;
var theTargetInsertIndex = -1;

// Init the text box to the last url played (shrug).
var polling_interval;

var CHUNK_SIZE = 200;
var nextStartIndex = 0;
var batchLoadsPending = 0;
var closePending = false;
var scanIsDone = false;
var totalAdded = 0;

function onLoad()
{
  // This window does not get focused properly on Linux, so force focus here.
  // It seems to have to do with the window getting opened immediately after
  // the file picker.
  window.focus();

  if ( ( typeof( window.arguments[0] ) != 'undefined' ) && ( typeof( window.arguments[0].URL ) != 'undefined' ) )
  {
    try
    {
      aFileScan = new sbIFileScan();
      aFileScanQuery = new sbIFileScanQuery();

      var libraryManager =
        Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                  .getService(Components.interfaces.sbILibraryManager);

      // Use the requested library, or use the main library as default      
      var theTargetDatabase = window.arguments[0].target_db;
      if (theTargetDatabase)
      {
        theTargetLibrary = libraryManager.getLibrary( theTargetDatabase );
      }
      else
      {
        theTargetLibrary = libraryManager.mainLibrary;
      }

      theTargetPlaylist = window.arguments[0].target_pl;
      theTargetInsertIndex = window.arguments[0].target_pl_row;

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
  // Check to see if the scan is complete or if it has progressed past our
  // chunk size
  var count = aFileScanQuery.getFileCount();
  scanIsDone = !aFileScanQuery.isScanning();
  if (count > 0 && (count - nextStartIndex > CHUNK_SIZE || scanIsDone)) {

    // Grab the next chunk of scanned tracks from the query
    var endIndex = scanIsDone ? count - 1 : nextStartIndex + CHUNK_SIZE - 1;
    if (endIndex >= count) {
      endIndex = count - 1;
    }

    var array = aFileScanQuery.getResultRangeAsURIs(nextStartIndex, endIndex);
    nextStartIndex = endIndex + 1;

    // Create a temporary track nane for all of the found tracks
    var propsArray =
      Components.classes["@mozilla.org/array;1"]
                .createInstance(Components.interfaces.nsIMutableArray);
    for (let i = 0; i < array.length; i++) {
      let uri = array.queryElementAt(i, Components.interfaces.nsIURI);
      let props =
        Components.classes["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
                  .createInstance(Components.interfaces.sbIMutablePropertyArray);
      props.appendProperty(SBProperties.trackName,
                           createDefaultTitle(uri));
      propsArray.appendElement(props, false);
    }

    // Asynchronously load the scanned chunk into the library
    var listener = new sbBatchCreateListener(array);
    batchLoadsPending++;
    theTargetLibrary.batchCreateMediaItemsAsync(listener, array, propsArray);
  }

  if (scanIsDone) {
    clearInterval(polling_interval);
    
    // We didn't find any items. Let's indicate this to the user.
    if(count < 1) {
      theTitle.value = SBString("media_scan.none", "Nothing");
      theLabel.value = SBString("media_scan.complete", "Complete");
      
      theProgress.removeAttribute( "mode" );
      document.documentElement.buttons = "accept";
    }
  }
  else {
    var value = aFileScanQuery.getLastFileFound();
    theLabel.value = gPPS.convertURLToFolder(value);
  }
}

var gMediaScanMetadataJob = null;
function appendToMetadataQueue( aItemArray ) {
  // If we need to, make a new job.
  if ( gMediaScanMetadataJob == null || gMediaScanMetadataJob.complete ) {
    // Create and submit a metadata job for the new items
    var metadataJobManager =
      Components.classes["@songbirdnest.com/Songbird/MetadataJobManager;1"]
                .getService(Components.interfaces.sbIMetadataJobManager);
    gMediaScanMetadataJob = metadataJobManager.newJob(aItemArray, 5);
  } else {
    // Otherwise, just append.
    gMediaScanMetadataJob.append(aItemArray);
  }
}

function sbBatchCreateListener(aArray) {
  this._array = aArray;
  this._length = aArray.length;
}

sbBatchCreateListener.prototype.onProgress = 
function sbBatchCreateListener_onProgress(aIndex)
{
}

sbBatchCreateListener.prototype.onComplete = 
function sbBatchCreateListener_onComplete(aItemArray)
{
  batchLoadsPending--;
  totalAdded += aItemArray.length;

  // New items to be sent for metadata scanning.
  if (aItemArray.length > 0) {
    appendToMetadataQueue( aItemArray );
  }

  if (batchLoadsPending == 0 && scanIsDone) {
    if (totalAdded > 0) {
      theTitle.value = totalAdded + " " + SBString("media_scan.added", "Added");
    }
    else {
      theTitle.value = SBString("media_scan.none", "Nothing");
    }
    theLabel.value = SBString("media_scan.complete", "Complete");
    theProgress.removeAttribute( "mode" );
    document.documentElement.buttons = "accept";
  }

  if (closePending && batchLoadsPending == 0) {
    onExit();
  }

}

function doOK()
{
  if (document.getElementById("watch_check").checked) {
    // XXX need to port folder watcher
    // var wfManager = new CWatchFolderManager();
    // XXXredfive - componentize WatchFolderManager
    // wfManager.CreateWatchFolderManager();
    // wfManager.AddWatchFolder(aFileScanQuery.getDirectory()); 
  }

  // We can't just close the window if we have pending batch loads since we
  // have a listener registered with the library.  Closing the window while
  // it has this reference causes that "Components not defined" error.
  if (batchLoadsPending == 0) {
    return true;
  }
  else {
    closePending = true;
    return false;
  }

}
function doCancel()
{
  // Run away!!
  if (aFileScanQuery)
    aFileScanQuery.cancel();

  clearInterval(polling_interval);

  // See comment in doOk()
  if (batchLoadsPending == 0) {
    return true;
  }
  else {
    closePending = true;
    return false;
  }
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

