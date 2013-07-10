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

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/SBJobUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");

//------------------------------------------------------------------------------

//
// @brief A XPCOM service to help with importing URI's into a specified media
//        list. For more information, see sbIURIImportService.idl.
//
function sbURIImportService()
{
}

sbURIImportService.prototype = 
{
  _importInProgress: false,  // are we currently importing a drop ?
  _uriArray:        null,    // array of URI's to import
  _directoryList:    [],     // queue of nsIFile directories to scan
  _targetList:       null,   // target mediaList for the drop
  _targetPosition:   -1,     // position in the mediaList we should drop at
  _firstMediaItem:   null,   // first mediaItem that was handled in this drop
  _scanList:         null,   // list of newly created medaItems for metadata scan
  _window:           null,   // window that received this drop
  _listener:         null,   // listener object, for notifications
  _totalImported:    0,      // number of items imported in the library
  _totalInserted:    0,      // number of items inserted in the medialist
  _totalDups:        0,      // number of items we already had in the library
  _otherDrops:       0,      // number of other drops handled (eg, XPI, JAR)
  _mainLibrary:      null,   // The main library


  // sbIURIImportService
  importURIArray: function(aMutableURIArray, 
                           aDOMWindow, 
                           aTargetMediaList, 
                           aTargetPosition,
                           aImportListener) 
  {
    this._window = aDOMWindow;
    this._targetList = aTargetMediaList;
    this._targetPosition = aTargetPosition;
    this._listener = aImportListener;

    // the array to record items to feed to the metadata scanner
    if (!this._scanList) {
      this._scanList = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                         .createInstance(Ci.nsIMutableArray);
    }

    // Reset stats and various job variables
    this._firstMediaItem = null;
    this._totalImported = 0;
    this._totalInserted = 0;
    this._totalDups = 0;
    this._mainLibrary = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                          .getService(Ci.sbILibraryManager)
                          .mainLibrary;

    this._uriArray = aMutableURIArray;

    // If we are already importing, our entries will be processed at the end
    // of the current batch. Otherwise, we need to start a new one.
    if (!this._importInProgress) {
      // Remember that we are currently processing a batch - if more files are
      // dropped while we're importing, they will be added to the end of the
      // batch import.
      this._importInProgress = true;

      // Begin processing the URI array.
      // (The first item will be handled immediately, and the subsequent ones
      // will be processed each time a new "frame" occurs, on a timer).
      this._importDropFrame();
    }
  },

  // give a little bit of time for the main thread to react to UI events, and 
  // then execute the next frame of the drop import session.
  _nextImportDropFrame: function() 
  {
    this._timer = Cc["@mozilla.org/timer;1"]
                    .createInstance(Ci.nsITimer);
    this._timer.initWithCallback(this, 10, Ci.nsITimer.TYPE_ONE_SHOT);    
  },

  // nsITimer
  notify: function(timer) 
  {
    this._timer = null;
    this._importDropFrame();
  },

  // Executes one frame of the drop import session
  _importDropFrame: function() 
  {
    try {
      // any more files to process ?
      if (this._uriArray.length > 0) {
        
        // get the first of the remaining files
        var uri = this._uriArray.queryElementAt(0, Ci.nsIURI);
        
        // remove it from the array of files to process
        this._uriArray.removeElementAt(0);

        if (uri) {
          // make a file URL object and check if the object is a directory.
          // if it is a directory, then we record it in a separate array,
          // which we will process at the end of the batch.
          var fileUrl;
          try {
            fileUrl = uri.QueryInterface(Ci.nsIFileURL);
          } 
          catch (e) { 
            fileUrl = null; 
          }
          if (fileUrl) {
            // is this is a directory?
            if (fileUrl.file.isDirectory()) {
              // yes, record it and delay processing
              this._directoryList.push(fileUrl.file);
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
      } 
      else {
        // there are no more items in the drop array, start the metadata scanner
        // if we created any mediaitem
        if (this._scanList && this._scanList.length > 0) {
          var metadataService = 
            Cc["@songbirdnest.com/Songbird/FileMetadataService;1"]
              .getService(Ci.sbIFileMetadataService);

          metadataService.read(this._scanList);
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
        else {
          // all done.
          this._dropComplete();
        }
      }
    } 
    catch (e) {
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
      let typeSniffer = 
        Cc["@songbirdnest.com/Songbird/Mediacore/TypeSniffer;1"]
          .createInstance(Ci.sbIMediacoreTypeSniffer);
      let Application = Cc["@mozilla.org/fuel/application;1"]
                          .getService(Ci.fuelIApplication);

      let isValidMediaURL = typeSniffer.isValidMediaURL(aURI);
      if (!Application.prefs.getValue("songbird.mediascan.enableVideoImporting", true)
          && typeSniffer.isValidVideoURL(aURI)) {
        isValidMediaURL = false;
      }

      if (isValidMediaURL) {
        // check whether the item already exists in the library 
        // for the target list
        let item = 
          this._getFirstItemByProperty(this._targetList.library,
                                       SBProperties.contentURL,
                                       aURI.spec);
        // If we didn't find the content URL try the originURL
        if (!item) {
          item = 
            this._getFirstItemByProperty(this._targetList.library,
                                         SBProperties.originURL,
                                         aURI.spec);
        }
        let mainLibraryItem = false;
        // If we're not dropping on the main library look to see if the main
        // library knows about this item. If it does, then use that item to add
        if (!this._targetList.library.equals(this._mainLibrary)) {
          if (!item) {
            item = this._getFirstItemByProperty(this._mainLibrary,
                                                SBProperties.contentURL,
                                                aURI.spec);
          }
          // If we didn't find the content URL try the originURL
          if (!item) {
            item = 
              this._getFirstItemByProperty(this._mainLibrary,
                                           SBProperties.originURL,
                                           aURI.spec);
          }
          mainLibraryItem = item != null;
        }
        // if the item didnt exist before, create it now
        let itemAdded = false;
        if (mainLibraryItem || !item) {
          let holder = {};
          if (!mainLibraryItem) {
              
              itemAdded = 
                this._targetList.library.createMediaItemIfNotExist(aURI, 
                                                                   null, 
                                                                   holder);
              item = holder.value;
          }
          else {
            
            // If we add it from the main library, we need to get the item
            // that might have been created and use that
            let newItem = this._targetList.library.addItem(item);
            itemAdded = newItem && !newItem.equals(item);
            if (newItem) {
              item = newItem;
            }
          }
          if (itemAdded) {
            this._scanList.appendElement(item, false);
            this._totalImported++;
          }
        } 

        // The item was not added because it was a duplicate.
        if (!itemAdded) {
          this._totalDups++;
        }

        // if the item is valid, and we are inserting in a medialist, insert it 
        // to the requested position
        if (item) {
          if ((this._targetList instanceof Ci.sbIOrderableMediaList) &&
              (this._targetPosition >= 0) && 
              (this._targetPosition < this._targetList.length)) 
          {
            this._targetList.insertBefore(this._targetPosition, item);
            this._targetPosition++;
          } 
          else {
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
      } 
    } 
    catch (e) {
      Components.utils.reportError(e);
    }
  },

  // search for an item inside a list based on a property value. this is used
  // by the drop handler to determine if a drop item is already in the target 
  // library, by looking for its contentURL.
  _getFirstItemByProperty: function(aMediaList, aProperty, aValue) 
  {
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
  _importDropDirectories: function() 
  {
    var importService = 
      Cc['@songbirdnest.com/Songbird/DirectoryImportService;1']
        .getService(Ci.sbIDirectoryImportService);
    
    var job = importService.import(ArrayConverter.nsIArray(this._directoryList),
                                   this._targetList, this._targetPosition);
    
    // Reset list of directories
    this._directoryList = [];
                                   
    SBJobUtils.showProgressDialog(job, this._window, /** No delay **/ 0);
    
    // If this job provided the first item, we may want to play it
    if (!this._firstMediaItem) {
      var allItems = job.enumerateAllItems();
      if (allItems.hasMoreElements()) {
        this._firstMediaItem = 
          allItems.getNext().QueryInterface(Ci.sbIMediaItem);
        
        if (this._listener) {
          this._listener.
            onFirstMediaItem(this._targetList, this._firstMediaItem);
        }
      }
    }

    this._totalImported += job.totalAddedToLibrary;
    this._totalInserted += job.totalAddedToMediaList;
    this._totalDups += job.totalDuplicates;
  },
  
  // called when the whole drop handling operation has completed, used
  // to notify the original caller and free up any resources we can
  _dropComplete: function() 
  {
    if (this._listener) {
      this._listener.onImportComplete(this._targetList,
                                      this._totalImported,
                                      this._totalDups,
                                      this._otherDrops,
                                      this._totalInserted,
                                      this._otherDrops);
    }

    // and reset references we do not need anymore, coz leaks suck
    this._importInProgress = false;
    this._targetList = null;
    this._scanList = null;
    this._window = null;
    this._listener = null;
    this._mainLibrary = null;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.sbIURIImportService])
};

//------------------------------------------------------------------------------
// XPCOM Registration

sbURIImportService.prototype.classDescription = 
  "Songbird URI Import Service";
sbURIImportService.prototype.classID =
  Components.ID("{CC435D34-F76A-458F-B786-FAF897CA69BD}");
sbURIImportService.prototype.contractID =
  "@songbirdnest.com/uri-import-service;1";

var NSGetFactory = XPCOMUtils.generateNSGetFactory([sbURIImportService]);

