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
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");

var gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                             .getService(Components.interfaces.sbIPlaylistPlayback);

var theTitle = document.getElementById( "songbird_media_scan_title" );
theTitle.value = "";

var theLabel = document.getElementById( "songbird_media_scan_label" );
theLabel.value = "";

var theProgress = document.getElementById( "songbird_media_scan" );
theProgress.setAttribute( "mode", "undetermined" );
theProgress.value = 0;

var gFileScan = null;
var gDirectoriesToScan = [];
var gOldJobs = [];
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
var autoClose = false;
var scanIsDone = false;
var totalAdded = 0;
var totalInserted = 0;
var totalDups = 0;
var scannedChunks = [];
var firstItem = null;
var onFirstItem = null;

function onLoad()
{
  // This window does not get focused properly on Linux, so force focus here.
  // It seems to have to do with the window getting opened immediately after
  // the file picker.
  window.focus();
  
  // disable the media scan menu item, since we're already in it (although that
  // does not disable the item in the parent window's menu)
  document.getElementById("menuitem_file_scan").setAttribute("disabled", "true");

  if ( ( typeof( window.arguments[0] ) != 'undefined' ) && ( typeof( window.arguments[0].URL ) != 'undefined' ) )
  {
    try
    {
      gFileScan =
        Components.classes["@songbirdnest.com/Songbird/FileScan;1"]
                  .createInstance(Components.interfaces.sbIFileScan);

      var libraryManager =
        Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                  .getService(Components.interfaces.sbILibraryManager);

      theTargetPlaylist = window.arguments[0].target_pl;
      if (theTargetPlaylist) {
        theTargetLibrary = theTargetPlaylist.library; 
      } else {
        theTargetLibrary = libraryManager.mainLibrary;
      }
      theTargetInsertIndex = window.arguments[0].target_pl_row;
      gDirectoriesToScan = window.arguments[0].URL;
      if (typeof window.arguments[0].autoClose != "undefined")
        autoClose = window.arguments[0].autoClose;
      onFirstItem = window.arguments[0].onFirstItem;
      if (!isinstance(gDirectoriesToScan, Array)) {
        gDirectoriesToScan = [gDirectoriesToScan];
      }
      scanNextDirectory();

    }
    catch(err)
    {
      alert("onLoad\n\n"+err);
    }
  }
  return true;
}

function isinstance(inst, base)
{
    /* Returns |true| if |inst| was constructed by |base|. Not 100% accurate,
     * but better than instanceof when the two sides are from different windows
     * copied from jsirc source (js/lib/utils.js)
     */
    return (inst && base &&
            ((inst instanceof base) ||
             (inst.constructor && (inst.constructor.name == base.name))));
}

function scanNextDirectory()
{
  try {
    if ( typeof(aFileScanQuery) != "undefined" && aFileScanQuery && aFileScanQuery.isScanning() ) {
/*
      alert("CRAP!!  MULTIPLE FILE SCANS AT ONCE!");
*/  
      return;
    }
  
    aFileScanQuery = 
      Components.classes["@songbirdnest.com/Songbird/FileScanQuery;1"]
                .createInstance(Components.interfaces.sbIFileScanQuery);
    if (gFileScan && aFileScanQuery)
    {
      nextStartIndex = 0;
      scanIsDone = false;
      // do not use pop(), that would scan the directories in reverse order,
      // we want to maintain the order in which they were selected in the
      // d&d source or the file picker
      var url = gDirectoriesToScan.shift();
      aFileScanQuery.setDirectory(url);
      aFileScanQuery.setRecurse(true);

      // Filter file extensions as part of the scan.        
      var eExtensions = gPPS.getSupportedFileExtensions();
      while (eExtensions.hasMore())
        aFileScanQuery.addFileExtension(eExtensions.getNext());

      gFileScan.submitQuery(aFileScanQuery);
      
      polling_interval = setInterval( onPollScan, 133 );
      
      theTitle.value = SBString("media_scan.scanning", "Scanning") + " -- " + url;
    } else {
      // ??? Awww, crap.
      throw "Something's wrong with the filescanner, dude";
    }
  }
  catch (err)
  {
    alert("scanNextDirectory\n\n"+err);
  }
}

var accumulatorArray = Components.classes["@mozilla.org/array;1"]
  .createInstance(Components.interfaces.nsIMutableArray);
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
    if (!scanIsDone)
      nextStartIndex = endIndex + 1;

    // Append the new array to the last array
    for (let i = 0; i < array.length; i++) {
      accumulatorArray.appendElement( 
        array.queryElementAt(i, Components.interfaces.nsIURI), false );
    }

    // We don't have a full chunk, and there's still more directories to go...
    if (accumulatorArray.length < CHUNK_SIZE && scanIsDone && gDirectoriesToScan.length > 0) {
      // Ooops, NOP.
    } else {
      // Create a temporary track nane for all of the found tracks
      var propsArray =
        Components.classes["@mozilla.org/array;1"]
                  .createInstance(Components.interfaces.nsIMutableArray);
      for (let i = 0; i < accumulatorArray.length; i++) {
        let uri = accumulatorArray.queryElementAt(i, Components.interfaces.nsIURI);
        let props =
          Components.classes["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
                    .createInstance(Components.interfaces.sbIMutablePropertyArray);
        props.appendProperty(SBProperties.trackName,
                            createDefaultTitle(uri));
        propsArray.appendElement(props, false);
      }

      // push all items on a fifo, so that each time the batch listener completes,
      // we can grab the full list of items in the chunk again (not just those
      // that were created), in order to be able to insert them in the target
      // playlist regardless of whether they already existed or not
      scannedChunks.push(accumulatorArray);

      // Asynchronously load the scanned chunk into the library
      var listener = new sbBatchCreateListener(accumulatorArray);
      batchLoadsPending++;
      theTargetLibrary.batchCreateMediaItemsAsync(listener, accumulatorArray, propsArray);
      
      // Make a new accumulator.
      accumulatorArray = Components.classes["@mozilla.org/array;1"]
        .createInstance(Components.interfaces.nsIMutableArray);    
    }
  }

  if (scanIsDone) {
    clearInterval(polling_interval);

    // if we have more things to look at, do that
    if (gDirectoriesToScan.length > 0) {
      scanNextDirectory();
      return;
    }
    
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
  try {
    // xxxlone> reusing the previous job fails ! no idea why :( to try it out, 
    // re-enable the test, then drop a few directories onto a empty library, and
    // observe how the first job get processed, but the following ones, those
    // added via append(), fail.
    // see bug 7264
    
    // If we need to, make a new job.
    var createNewJob = false;
    var freakout = false;
    if ( typeof gMediaScanMetadataJob == 'unknown' || gMediaScanMetadataJob == null || gMediaScanMetadataJob.complete ) {
      createNewJob = true;
    } else {
      try {
        // Otherwise, try to append.
        gMediaScanMetadataJob.append(aItemArray);
      } catch(e) {
        // Sometimes, there might be a race condition, yea?
        Components.utils.reportError(e);
        freakout = true;
        createNewJob = true;
      }
    }
    
    // Create and submit a metadata job for the new items
    if ( createNewJob ) {
      gOldJobs.push( gMediaScanMetadataJob ); // Just in case?  Keep a reference on it?  This seems odd.
      var metadataJobManager =
        Components.classes["@songbirdnest.com/Songbird/MetadataJobManager;1"]
                  .getService(Components.interfaces.sbIMetadataJobManager);
      gMediaScanMetadataJob = metadataJobManager.newJob(aItemArray, 5);
/*      
      if (freakout)
        alert("FREAKOUT!");
*/  
    }
  } catch (e) {
    Components.utils.reportError(e);
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
  
  // get the original list of URIs corresponding to this batch chunk
  var chunk = scannedChunks[0];
  scannedChunks.splice(0, 1);
  
  // calculate number of items that were dropped because they already existed 
  // in the library
  totalDups += chunk.length - aItemArray.length;
  
  // find the first item via the original list, we want to notify with the first
  // item, not necessarilly the first one that didnt exist in the target library
  if (!firstItem && 
      onFirstItem &&
      chunk.length > 0) {
    
    var itemURI = chunk.enumerate().getNext();
    itemURI = itemURI.QueryInterface(Components.interfaces.nsIURI);
    if (itemURI) {
      var listener = {
        item: null,
        onEnumerationBegin: function() {
        },
        onEnumeratedItem: function(list, item) {
          this.item = item;
          return Components.interfaces.sbIMediaListEnumerationListener.CANCEL;
        },
        onEnumerationEnd: function() {
        }
      };

      theTargetLibrary.enumerateItemsByProperty(SBProperties.contentURL,
                                                itemURI.spec,
                                                listener);
      callFirstItemCallback(listener.item)
    }
  }


  // if we are inserting into a list, there is more to do than just importing 
  // the tracks into its library, we also need to insert all the items (even
  // the ones that previously existed) into the list at the requested position
  if (theTargetPlaylist && 
      (theTargetPlaylist != theTargetLibrary)) {
    try {
      // make an array of contentURL properties, so we can search 
      // for all tracks in one go
      var propertyArray = SBProperties.createArray();
      var enumerator = chunk.enumerate();
      while (enumerator.hasMoreElements()) {
        var itemURI = enumerator.getNext();
        itemURI = itemURI.QueryInterface(Components.interfaces.nsIURI);
        propertyArray.appendProperty(SBProperties.contentURL, itemURI.spec);
      } 

      // search all the items for this chunk, and build a list of mediaItems
      var mediaItems = [];
      var listener = {
        onEnumerationBegin: function() {
        },
        onEnumeratedItem: function(list, item) {
          mediaItems.push(item);
        },
        onEnumerationEnd: function() {
        },
        QueryInterface : function(iid) {
          if (iid.equals(Components.interfaces.sbIMediaListEnumerationListener) ||
              iid.equals(Components.interfaces.nsISupports))
            return this;
          throw Components.results.NS_NOINTERFACE;
        }
      };

      theTargetLibrary.
        enumerateItemsByProperties(propertyArray,
                                  listener);

      if (mediaItems.length > 0) {
        // if we need to insert, then do so
        if ((theTargetPlaylist instanceof Components.interfaces.sbIOrderableMediaList) && 
            (theTargetInsertIndex >= 0) && 
            (theTargetInsertIndex < theTargetPlaylist.length)) {
          theTargetPlaylist.insertSomeBefore(theTargetInsertIndex, ArrayConverter.enumerator(mediaItems));
          theTargetInsertIndex += mediaItems.length;
          totalInserted += mediaItems.length;
        } else {
          // otherwise, just add
          theTargetPlaylist.addSome(ArrayConverter.enumerator(mediaItems));
          totalInserted += mediaItems.length;
        }
      }
    } catch (e) {
      Components.utils.reportError(e);
    }
  }
  
  // New items to be sent for metadata scanning.
  if (aItemArray.length > 0) {
    appendToMetadataQueue( aItemArray );
  }
  

  if (batchLoadsPending == 0 && scanIsDone) {

    if (gDirectoriesToScan.length > 0) {
      scanNextDirectory();
      return;
    }

    if (totalAdded > 0) {
      theTitle.value = totalAdded + " " + SBString("media_scan.added", "Added");
    }
    else {
      theTitle.value = SBString("media_scan.none", "Nothing");
    }
    theLabel.value = SBString("media_scan.complete", "Complete");
    theProgress.removeAttribute( "mode" );
    document.documentElement.buttons = "accept";
    
    closePending = autoClose;
  }

  if (closePending && batchLoadsPending == 0) {
    beforeClose();
    onExit();
  }
}

function callFirstItemCallback(aItem) {
  firstItem = aItem;
  var cb = onFirstItem;
  if (cb) {
    if (typeof cb == "function") {
      cb(firstItem);
    } else if ((typeof cb == "object") && 
            cb.onFirstItem &&
            (typeof cb.onFirstItem == "function")) {
      cb.onFirstItem(firstItem);
    } else if (isinstance(cb, String) || typeof(cb) == "string") {
      var fn = new Function("item", cb);
      fn(firstItem); 
    }
  }
}

function beforeClose() {
  window.arguments[0].totalImportedToLibrary = totalAdded;
  window.arguments[0].totalDups = totalDups;
  if (theTargetPlaylist && 
      (theTargetPlaylist != theTargetLibrary))
    window.arguments[0].totalAddedToMediaList = totalInserted;
  else
    window.arguments[0].totalAddedToMediaList = totalAdded;
  onFirstItem = null;
  theTargetLibrary = null;
  theTargetPlaylist = null;
  aFileScanQuery = null;
  gFileScan = null;
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
    beforeClose();
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
    beforeClose();
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



