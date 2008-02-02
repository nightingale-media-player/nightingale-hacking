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
 
/*


  ExternalDropHandler JSM Module



This helper is used to let you handle external file drops and automatically
handle scanning directories as needed, injecting items at a specific spot in
a media list, schedule a metadata scanner job for the newly imported items,
and so on.

There are two ways of triggering a drop handling, the question of which one you
should be using depends on how it is you would like to handle the drop:

To handle a drop in a generic manner, and have all dropped items automatically
directed to the default library, all you need to do is add the following code in
your onDrop/ondragdrop handler:

  ExternalDropHandler.drop(window, dragSession, dropHandlerListener);

The last parameter is optional, it allows you to receive notifications. Here is
a minimal implementation:

  var dropHandlerListener = {
    // called when the drop handling has completed
    onDropComplete: function(aTargetList,
                             aImportedInLibrary,
                             aDuplicates,
                             aInsertedInMediaList,
                             aOtherDropsHandled) { 
      // returning true causes the standard drop report to be printed
      // on the status bar, it is equivalent to calling standardReport
      // using the parameters received on this callback
      return true; 
    },
    // called when the first item is handled (eg, to implement playback)
    onFirstMediaItem: function(aTargetList, aFirstMediaItem) { }
  };

To handle a drop with a specific mediaList target and drop insertion point, use
the following code:

  ExternalDropHandler.dropOnList(window, 
                                 dragSession, 
                                 targetMediaList, 
                                 targetMediaListPosition,
                                 dropHandlerListener);

In order to target the drop at the end of the targeted mediaList, you
should give a value of -1 for targetMediaListPosition.

The other public methods in this helper can be used to simplify the rest of your
drag and drop handler as well. For instance, an nsDragAndDrop observer's 
getSupportedFlavours() method may be implemented simply as:

    var flavours = new FlavourSet();
    ExternalDropHandler.addFlavours(flavours);
    return flavours;

Also, getTransferData and getTransferDataForFlavour may be used to inspect the
content of the dragSession before deciding what to do with it.

Finally, the standardReport and reportAddedTracks methods are used to send a 
temporary message on the status bar, to report the result of a drag and drop 
session. standardReport will format the text using the specific rules for what
to show and in which circumstances, and reportAddedTracks will report exactly
what you tell it to.

Important note:
---------------

The window being passed as a parameter to both the drop and dropOnList methods
must implement the following two functions :

SBOpenModalDialog(aChromeUrl, aTargetId, aWindowFeatures, aWindowArguments);
installXPI(aXpiUrl);

These two methods are respectively implemented in windowUtils.js and 
playerOpen.js, importing these scripts in your window ensures that the
requirements are met.

*/

EXPORTED_SYMBOLS = [ "ExternalDropHandler" ];

var ExternalDropHandler = {

  _Ci: Components.interfaces,
  _Cc: Components.classes,

  supportedFlavours: [ "application/x-moz-file",
                       "text/x-moz-url",
                       "text/unicode",
                       "text/plain" ],
  
  // returns true if the drag session contains supported external flavors
  isSupported: function(aDragSession) {
    for (var i in this.supportedFlavours) {
      if (aDragSession.isDataFlavorSupported(this.supportedFlavours[i])) {
        return true;
      }
    }
    return false;
  },
  
  // performs a default drop of the drag session. media items go to the
  // main library. 
  drop: function(aWindow, aDragSession, aListener) {
    var mainLibrary = this._Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                          .getService(this._Ci.sbILibraryManager)
                          .mainLibrary;
    this.dropOnList(aWindow, aDragSession, mainLibrary, -1, aListener);
  },

  // perform a drop onto a medialist. media items are inserted at the specified
  // position in the list if that list is orderable. otherwise, or if the
  // position is invalid, the items are added to the target list.
  dropOnList: function(aWindow, 
                       aDragSession, 
                       aTargetList, 
                       aDropPosition, 
                       aListener) {
    if (!aTargetList) {
      throw new Error("No target medialist specified for dropOnList");
    }
    this._dropFiles(aWindow,
                    aDragSession, 
                    aTargetList, 
                    aDropPosition, 
                    aListener);
  },

  // call this to automatically add the supported external flavors 
  // to a flavourSet object
  addFlavours: function(aFlavourSet) {
    for (var i in this.supportedFlavours) {
      aFlavourSet.appendFlavour(this.supportedFlavours[i]);
    }
  },

  // fills an array with the data for all items of a given flavor
  getTransferDataForFlavour: function(aFlavour, aArray) {
    var dragService = this._Cc["@mozilla.org/widget/dragservice;1"]
                          .getService(this._Ci.nsIDragService);
    var session = dragService.getCurrentSession();

    var nitems = session.numDropItems;
    var r = null;

    if (session.isDataFlavorSupported(aFlavour)) {

      for (var i=0;i<nitems;i++) {
        var transfer = this._Cc["@mozilla.org/widget/transferable;1"]
                           .createInstance(this._Ci.nsITransferable);

        transfer.addDataFlavor(aFlavour);
        session.getData(transfer, i);
        var data = {};
        var length = {};
        transfer.getTransferData(aFlavour, data, length);
        if (!r) r = data.value;
        if (aArray) aArray.push([data.value, length.value, aFlavour]);
      }
    }

    return r;
  },

  // returns an array with the data for any supported external flavor
  getTransferData: function() {
    var data = [];
    for (var i in this.supportedFlavours) {
      if (this.getTransferDataForFlavour(this.supportedFlavours[i], data)) 
        break;
    }
    return data;
  },

  _str_tracksaddedto        : null,
  _str_trackaddedto         : null,
  _str_notracksaddedto      : null,
  _str_trackalreadypresent  : null,
  _str_tracksalreadypresent : null,

  // temporarilly writes "X tracks added to <name>, Y tracks already present" 
  // in the status bar. if 0 is specified for aDups, the second part of the
  // message is skipped.
  reportAddedTracks: function(aAdded, aDups, aDestName) {
    if (!this._str_tracksaddedto) {
      this._gotStrings = true;
      this._str_tracksaddedto =
        this._getString("library.tracksaddedto", "tracks added to");
      this._str_trackaddedto =
        this._getString("library.trackaddedto", "track added to");
      this._str_notracksaddedto =
        this._getString("library.notracksaddedto", "No tracks added to");
      this._str_trackalreadypresent =
        this._getString("library.trackalreadypresent", "track already present");
      this._str_tracksalreadypresent =
        this._getString("library.tracksalreadypresent","tracks already present");
      this._stringbundle = null;
    }
    var msg = aAdded + " ";

    // start message with 'xxx added to'... except for 0 ('no tracks added')
    switch (aAdded) {
      case 0:  msg = this._str_notracksaddedto;  break;
      case 1:  msg += this._str_trackaddedto;    break;
      default: msg += this._str_tracksaddedto;   break;
    }

    msg += " " + aDestName;

    // append the message about items that were already present (if any)
    if (aDups > 0) {
      msg += " (" + aDups + " ";
      if (aDups == 1) {
        msg += this._str_trackalreadypresent;
      }
      else {
        msg += this._str_tracksalreadypresent;
      }
      msg += ")";
    }

    var SB_NewDataRemote = 
      Components.Constructor("@songbirdnest.com/Songbird/DataRemote;1",
                             "sbIDataRemote",
                             "init");
    var statusOverrideText = 
      SB_NewDataRemote( "faceplate.status.override.text", null );
    var statusOverrideStyle = 
      SB_NewDataRemote( "faceplate.status.override.style", null );

    statusOverrideText.stringValue = "";
    statusOverrideText.stringValue = msg;
    statusOverrideStyle.stringValue = "font-weight: bold;";
  },
  
  // reports stats on the statusbar using standard rules for what to show and
  // in which circumstances
  standardReport: function(aTargetList,
                           aImportedInLibrary,
                           aDuplicates,
                           aInsertedInMediaList,
                           aOtherDropsHandled) {
    // do not report anything if all we did was drop an XPI
    if ((aImportedInLibrary == 0) &&
        (aInsertedInMediaList == 0) &&
        (aDuplicates == 0) &&
        (aOtherDropsHandled != 0)) 
      return;
    
    // report different things depending on whether we dropped
    // on a library, or just a list
    if (aTargetList != aTargetList.library) {
      ExternalDropHandler.reportAddedTracks(aInsertedInMediaList, 
                                            0, 
                                            aTargetList.name);
    } else {
      ExternalDropHandler.reportAddedTracks(aImportedInLibrary, 
                                            aDuplicates, 
                                            aTargetList.name);
    }
  },
  
  // --------------------------------------------------------------------------
  // methods below this point are pretend-private
  // --------------------------------------------------------------------------

  _importInProgress    : false, // are we currently importing a drop ?
  _fileList            : [],    // list of files URLs to import
  _directoryList       : [],    // queue of directories to scan
  _targetList          : null,  // target mediaList for the drop
  _targetPosition      : -1,    // position in the mediaList we should drop at
  _firstMediaItem      : null,  // first mediaItem that was handled in this drop
  _scanList            : null,  // list of newly created medaItems for metadata scan
  _window              : null,  // window that received this drop
  _listener            : null,  // listener object, for notifications
  _totalImported       : 0,     // number of items imported in the library
  _totalInserted       : 0,     // number of items inserted in the medialist
  _totalDups           : 0,     // number of items we already had in the library
  _otherDrops          : 0,     // number of other drops handled (eg, XPI, JAR)

  // initiate the handling of all dropped files: this handling is sliced up 
  // into a number of 'frames', each frame importing one item, or queuing up 
  // one directory for later import. at the end of each frame, the 
  // _nextImportDropFrame method is called to schedule the next frame using a 
  // short timer, so as to give the UI time to catch up, and we keep doing that 
  // until everything in the file queue has been processed. when that's done, 
  // we then look for queued directory scans, which we give to the mediaScan 
  // modal dialog. after the directories have been processed, we notify the 
  // listener that processing has ended.
  _dropFiles: function(window, session, targetlist, position, listener) {
  
    // check that we are indeed processing an external drop
    if (!this.isSupported(session)) {
      return;
    }

    // the array to record items to feed to the metadata scanner
    if (!this._scanList) {
      this._scanList = this._Cc["@mozilla.org/array;1"]
                          .createInstance(this._Ci.nsIMutableArray);
    }

    // if we are on win32, we will need to make local filenames lowercase
    var lcase = (this._getPlatformString() == "Windows_NT");
    
    // get drop data in any of the supported formats
    var dropdata = this.getTransferData();
    
    // remember the target list and position for this drop
    this._targetList = targetlist;
    this._targetPosition = position;
    
    // reset first media item, so we know to record it again
    this._firstMediaItem = null;
    
    // remember window for openModalDialog
    this._window = window;
    
    // remember listener
    this._listener = listener;
    
    // reset stats
    this._totalImported = 0;
    this._totalInserted = 0;
    this._totalDups = 0;
    
    // process all entries in the drop
    for (var dropentry in dropdata) {
      var dropitem = dropdata[dropentry];
    
      var item = dropitem[0];
      var flavour = dropitem[2];
      var islocal = true;
      var rawData;
      
      var prettyName;

      if (flavour == "application/x-moz-file") {
        var ioService = this._Cc["@mozilla.org/network/io-service;1"]
                            .getService(this._Ci.nsIIOService);
        var fileHandler = ioService.getProtocolHandler("file")
                          .QueryInterface(this._Ci.nsIFileProtocolHandler);
        rawData = fileHandler.getURLSpecFromFile(item);
      } else {
        if (item instanceof this._Ci.nsISupportsString) {
          rawData = item.data;
        } else {
          rawData = ""+item;
        }
        if (rawData.toLowerCase().indexOf("http://") < 0) {
          // not a url, ignore
          continue;
        } else {
          // remember that this is not a local file
          islocal = false;
        }
      }
      
      // rawData contains a file or http URL to the dropped media.

      // check if there is a pretty name we can grab
      var separator = rawData.indexOf("\n");
      if (separator != -1) {
        prettyName = rawData.substr(separator+1);
        rawData = rawData.substr(0,separator);
      }
      
      // make filename lowercase if necessary (win32)
      if (lcase && islocal) {
        rawData = rawData.toLowerCase();
      }

      // record this file for later processing
      this._fileList.push(rawData);
    }

    // if we are already importing, our entries will be processed at the end
    // of the current batch, otherwise, we need to start a new one
    if (!this._importInProgress) {
      
      // remember that we are currently processing the drop.
      // if more files are dropped while we're importing, they will be added
      // to the end of the batch import
      this._importInProgress = true;
      
      // begin processing array of dropped items
      this._nextImportDropFrame();
    }

  },
  
  // give a little bit of time for the main thread to react to UI events, and 
  // then execute the next frame of the drop import session.
  _nextImportDropFrame: function() {
    this._timer = this._Cc["@mozilla.org/timer;1"]
                      .createInstance(this._Ci.nsITimer);
    this._timer.initWithCallback(this, 10, this._Ci.nsITimer.TYPE_ONE_SHOT);    
  },

  // one shot timer notification method
  notify: function(timer) {
    this._timer = null;
    this._importDropFrame();
  },
  
  // executes one frame of the drop import session
  _importDropFrame: function() {
    try {
      // any more files to process ?
      if (this._fileList.length > 0) {
        
        // get the first of the remaining files
        var url;
        url = this._fileList[0];
        
        // remove it from the array of files to process
        this._fileList.splice(0, 1);
        
        // safety check
        if (url != "") {

          // make a URI object for this file
          var ios = this._Cc["@mozilla.org/network/io-service;1"]
                        .getService(this._Ci.nsIIOService);
          var uri = ios.newURI(url, null, null);
          
          // make a file URL object and check if the object is a directory.
          // if it is a directory, then we record it in a separate array,
          // which we will process at the end of the batch.
          var fileUrl;
          try {
            fileUrl = uri.QueryInterface(this._Ci.nsIFileURL);
          } catch (e) { 
            fileUrl = null; 
          }
          if (fileUrl) {
            // is this is a directory?
            if (fileUrl.file.isDirectory()) {
              // yes, record it and delay processing
              this._directoryList.push(fileUrl.file.path);
              // continue with the current batch
              this._nextImportDropFrame();
              return;
            }
          }

          // process this item
          this._importDropFile(uri);
        }

        // continue with the current batch
        this._nextImportDropFrame();

      } else {
        
        // there are no more items in the drop array, start the metadata scanner
        // if we created any mediaitem
        if (this._scanList && 
            this._scanList.length > 0) {
          var metadataJobMgr = 
            this._Cc["@songbirdnest.com/Songbird/MetadataJobManager;1"]
                .getService(this._Ci.sbIMetadataJobManager);
          metadataJobMgr.newJob(this._scanList, 5);
          this._scanList = null;
        }
        
        // see if there are any directories to be scanned now.
        if (this._directoryList.length > 0) {

          // yes, there are, import them.
          this._importDropDirectories();
          
          // continue with the current batch, in case more stuff
          // has been dropped. this shouldnt be possible because the
          // mediascan dialog is modal, but it is better to check than
          // to lose drops
          this._nextImportDropFrame();
        } 
        
        // all done.
        this._dropComplete();
      }
    } catch (e) {

      // oops, something wrong happened, we do not want to abort the whole
      // import batch tho, so just print a debug message and try the next
      // frame
      Components.utils.reportError(e);
      this._nextImportDropFrame();

    }
  },
  
  // import the given file URI into the target library and optionally inserts in
  // into the target playlist at the desired spot. if the item already exists,
  // it is only inserted to the playlist
  _importDropFile: function(aURI) {
    try {    
      // is this a media url ?
      var gPPS = this._Cc["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                     .getService(this._Ci.sbIPlaylistPlayback);
      if (gPPS.isMediaURL( aURI.spec )) {
        
        // check whether the item already exists in the library 
        // for the target list
        var item = this._getFirstItemByProperty(this._targetList.library, 
                        "http://songbirdnest.com/data/1.0#contentURL", 
                        aURI.spec);

        // if the item didnt exist before, create it now
        if (!item) {
          item = this._targetList.library.createMediaItem(aURI);
          if (item) {
            this._scanList.appendElement(item, false);
            this._totalImported++;
          }
        } else {
          this._totalDups++;
        }

        // if the item is valid, and we are inserting in a medialist, insert it 
        // to the requested position
        if (item && 
            (this._targetList != this._targetList.library)) {
          if ((this._targetList instanceof this._Ci.sbIOrderableMediaList) &&
              (this._targetPosition >= 0) && 
              (this._targetPosition < this._targetList.length)) {
            this._targetList.insertBefore(this._targetPosition, item);
            this._targetPosition++;
          } else {
            this._targetList.add(item);
          }
          
          this._totalInserted++;
        }
        // report the first item that was dropped
        if (!this._firstMediaItem) {
          this._firstMediaItem = item;
          if (this._listener) {
            this._listener.onFirstMediaItem(this._targetList, item);
          }
        }
      } else if ( /\.(xpi|jar)$/i.test(aURI.spec) ) {
        this._otherDrops++;
        // this._window.installXPI(aURI.spec) will not work but this will:
        this._window.setTimeout(this._window.installXPI, 10, aURI.spec);
        // wicked! :D
      }
    } catch (e) {
      Components.utils.reportError(e);
    }
  },
  
  // search for an item inside a list based on a property value. this is used
  // by the drop handler to determine if a drop item is already in the target 
  // library, by looking for its contentURL.
  _getFirstItemByProperty: function(aMediaList, aProperty, aValue) {
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

    aMediaList.enumerateItemsByProperty(aProperty,
                                        aValue,
                                        listener);
    return listener.item;
  },

  // trigger the import of all directory entries that have been defered until
  // this point. this launches the media scanner dialog box.
  _importDropDirectories: function() {

    var SB_NewDataRemote = 
      Components.Constructor("@songbirdnest.com/Songbird/DataRemote;1",
                             "sbIDataRemote",
                             "init");
    var fileScanIsOpen = SB_NewDataRemote( "media_scan.open", null );

    fileScanIsOpen.boolValue = true;
    
    // fire off the media scan page
    var media_scan_data = new Object();

    // set up media scan target
    media_scan_data.target_pl = this._targetList;
    media_scan_data.target_pl_row = this._targetPosition;
    
    // attach the list of directories to scan
    media_scan_data.URL = this._directoryList;
    
    // receive notification of the first handled item
    var cbObj = {
      dropHandler: this,
      onFirstItem: function(aItem) {
        if (!this.dropHandler._firstMediaItem) {
          this.dropHandler._firstMediaItem = aItem;
          if (this.dropHandler._listener) {
            this.dropHandler._listener.
              onFirstMediaItem(this.dropHandler._targetList, aItem);
          }
        }
      }
    };
    media_scan_data.onFirstItem = cbObj;
    
    // reset list of directories
    this._directoryList = [];
    
    // if we are dropping onto a playlist, and not just a library, then we
    // want the import box to autoClose once it's done, because it is very
    // misleading to see "No items added" just because the items were already in
    // the library, although items have still been inserted in the target playlist
    if (this._targetList != this._targetList.library)
      media_scan_data.autoClose = true;

    // Open the modal dialog
    this._window.
      SBOpenModalDialog( 
        "chrome://songbird/content/xul/mediaScan.xul", 
        "media_scan", 
        "chrome,centerscreen", 
        media_scan_data, 
        this._window); 
    
    this._totalImported += media_scan_data.totalImportedToLibrary;
    this._totalInserted += media_scan_data.totalAddedToMediaList;
    this._totalDups += media_scan_data.totalDups;
                              
    fileScanIsOpen.boolValue = false;
  },
  
  // called when the whole drop handling operation has completed, used
  // to notify the original caller and free up any resources we can
  _dropComplete: function() {
    // notify the listener that we're done
    if (this._listener) {
      if (this._listener.onDropComplete(this._targetList,
                                        this._totalImported,
                                        this._totalDups, 
                                        this._totalInserted,
                                        this._otherDrops)) {
        this.standardReport(this._targetList,
                            this._totalImported,
                            this._totalDups, 
                            this._totalInserted,
                            this._otherDrops);
      }
    }
    
    // and reset references we do not need anymore, coz leaks suck
    this._importInProgress = false;
    this._targetList = null;
    this._scanList = null;
    this._window = null;
    this._listener = null;
  },
  
  // returns the platform string
  _getPlatformString: function() {
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
  },

  _stringbundle : null,
  
  // get a string from the default songbird bundle, with fallback
  _getString: function(name, defaultValue) {
    if (!this._stringbundle) {
      var src = "chrome://songbird/locale/songbird.properties";
      var stringBundleService = this._Cc["@mozilla.org/intl/stringbundle;1"]
                                    .getService(this._Ci.nsIStringBundleService);
      this._stringbundle = stringBundleService.createBundle(src);
    }

    try {
      return this._stringbundle.GetStringFromName(name);
    }
    catch(e) {
      return defaultValue;
    }
  }
  
}


