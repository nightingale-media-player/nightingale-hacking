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
Components.utils.import("resource://app/jsmodules/sbColumnSpecParser.jsm");

EXPORTED_SYMBOLS = ["LibraryUtils"];

const Cc = Components.classes;
const Ci = Components.interfaces;

/**
 * \class LibraryUtils
 * \brief Javascript wrappers for common library tasks
 */
var LibraryUtils = {

  get manager() {
    var manager = Cc["@songbirdnest.com/Songbird/library/Manager;1"].
                  getService(Ci.sbILibraryManager);
    if (manager) {
      // Succeeded in getting the library manager, don't do this again.
      this.__defineGetter__("manager", function() {
        return manager;
      });
    }
    return manager;
  },

  get mainLibrary() {
    return this.manager.mainLibrary;
  },

  get webLibrary() {
    var webLibraryGUID = Cc["@mozilla.org/preferences-service;1"].
                         getService(Ci.nsIPrefBranch).
                         getCharPref("songbird.library.web");
    var webLibrary = this.manager.getLibrary(webLibraryGUID);
    delete webLibraryGUID;

    if (webLibrary) {
      // Succeeded in getting the web library, don't ever do this again.
      this.__defineGetter__("webLibrary", function() {
        return webLibrary;
      });
    }

    return webLibrary;
  },

  _standardFilterConstraint: null,
  get standardFilterConstraint() {
    if (!this._standardFilterConstraint) {
      this._standardFilterConstraint = this.createConstraint([
        [
          [SBProperties.isList, ["0"]]
        ],
        [
          [SBProperties.hidden, ["0"]]
        ]
      ]);
    }
    return this._standardFilterConstraint;
  },

  createConstraint: function(aObject) {
    var builder = Cc["@songbirdnest.com/Songbird/Library/ConstraintBuilder;1"]
                    .createInstance(Ci.sbILibraryConstraintBuilder);
    aObject.forEach(function(aGroup, aIndex) {
      aGroup.forEach(function(aPair) {
        var property = aPair[0];
        var values = aPair[1];
        var enumerator = {
          a: values,
          i: 0,
          hasMore: function() {
            return this.i < this.a.length;
          },
          getNext: function() {
            return this.a[this.i++];
          }
        };
        builder.includeList(property, enumerator);
      });
      if (aIndex < aObject.length - 1) {
        builder.intersect();
      }
    });
    return builder.get();
  },

  createStandardSearchConstraint: function(aSearchString) {
    if (aSearchString == "" || !aSearchString) 
      return null;
    var builder = Cc["@songbirdnest.com/Songbird/Library/ConstraintBuilder;1"]
                    .createInstance(Ci.sbILibraryConstraintBuilder);
    var a = aSearchString.split(" ");
    var first = true;
    for (var i = 0; i < a.length; i++) {
      if (a[i] && a[i] != "") {
        if (!first) {
          builder.intersect();
        }
        builder.include(SBProperties.artistName, a[i]);
        builder.include(SBProperties.albumName, a[i]);
        builder.include(SBProperties.trackName, a[i]);
        first = false;
      }
    }
    return builder.get();
  },

  createStandardMediaListView: function(aMediaList, aSearchString) {
    var mediaListView = aMediaList.createView();
    // XXXXXX THIS IS WRONG? web library?!
    // Get the sort for this list by parsing the list's column spec.  Then hit
    // the property manager to see if there is a special sort profile for this
    // ID
    var parser = new ColumnSpecParser(LibraryUtils.webLibrary, null);
    if (parser.sortID) {
      var pm =
        Components.classes["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
                  .getService(Components.interfaces.sbIPropertyManager);
      var sort = pm.getPropertySort(parser.sortID, parser.sortIsAscending);
      mediaListView.setSort(sort);
    }
    
    // By default, we never want to show lists and hidden 
    // things in the playlist
    mediaListView.filterConstraint = LibraryUtils.standardFilterConstraint;
    
    // Set up a standard search filter.  
    // It can always be replaced later.
    var filter = mediaListView.cascadeFilterSet;
    filter.appendSearch([
      SBProperties.artistName,
      SBProperties.albumName,
      SBProperties.trackName
    ], 3);
   
    if (aSearchString) {
      // Set the search 
      var searchArray = aSearchString.split(" ");
      filter.set(0, searchArray, searchArray.length);
    } else { 
      // Or not.
      filter.set(0, [], 0);
    }

    return mediaListView;
  }
}



/**
 * \class LibraryUtils.BatchHelper
 * \brief Helper object for monitoring the state of 
 *        batch library operations.
 */
LibraryUtils.BatchHelper = function() {
  this._depth = 0;
}

LibraryUtils.BatchHelper.prototype.begin =
function BatchHelper_begin()
{
  this._depth++;
}

LibraryUtils.BatchHelper.prototype.end =
function BatchHelper_end()
{
  this._depth--;
  if (this._depth < 0) {
    throw new Error("Invalid batch depth!");
  }
}

LibraryUtils.BatchHelper.prototype.depth =
function BatchHelper_depth()
{
  return this._depth;
}

LibraryUtils.BatchHelper.prototype.isActive =
function BatchHelper_isActive()
{
  return this._depth > 0;
}



/**
 * \class LibraryUtils.MultiBatchHelper
 * \brief Helper object for monitoring the state of 
 *        batch operations in multiple libraries
 */
LibraryUtils.MultiBatchHelper = function() {
  this._libraries = {};
}

LibraryUtils.MultiBatchHelper.prototype.get =
function MultiBatchHelper_get(aLibrary)
{
  var batch = this._libraries[aLibrary.guid];
  if (!batch) {
    batch = new LibraryUtils.BatchHelper();
    this._libraries[aLibrary.guid] = batch;
  }
  return batch;
}

LibraryUtils.MultiBatchHelper.prototype.begin =
function MultiBatchHelper_begin(aLibrary)
{
  var batch = this.get(aLibrary);
  batch.begin();
}

LibraryUtils.MultiBatchHelper.prototype.end =
function MultiBatchHelper_end(aLibrary)
{
  var batch = this.get(aLibrary);
  batch.end();
}

LibraryUtils.MultiBatchHelper.prototype.depth =
function MultiBatchHelper_depth(aLibrary)
{
  var batch = this.get(aLibrary);
  return batch.depth();
}

LibraryUtils.MultiBatchHelper.prototype.isActive =
function MultiBatchHelper_isActive(aLibrary)
{
  var batch = this.get(aLibrary);
  return batch.isActive();
}




/**
 * \class LibraryUtils.RemovalMonitor
 * \brief Helps track removal/deletion of medialists.
 * \param aCallback An object with an onMediaListRemoved function.
 */
LibraryUtils.RemovalMonitor = function(aCallback) {
  //dump("RemovalMonitor: RemovalMonitor()\n");
    
  if (!aCallback || !aCallback.onMediaListRemoved) {
    throw new Error("RemovalMonitor() requires a callback object");
  }

  this._callback = aCallback;
}

LibraryUtils.RemovalMonitor.prototype = {

  // An object with an onMediaListRemoved function
  _callback: null,
 
  // MediaList GUID to monitor for removal
  _targetGUID: null,
  
  // Library that owns the target MediaList
  _library: null,
  
  _libraryManager: null,
  _batchHelper: null,
  
  // Flag to indicate that the target item
  // was deleted in a batch operation
  _removedInBatch: false,
 
 
  /**
   * Watch for removal of the given sbIMediaList.
   * Pass null to stop listening.
   */
  setMediaList:  function RemovalMonitor_setMediaList(aMediaList) {
    //dump("RemovalMonitor: RemovalMonitor.setMediaList()\n");
    this._removedInBatch = false;
  
    // If passed a medialist, hook up listeners
    if (aMediaList instanceof Ci.sbIMediaList) {
      
      // Listen to the library if we aren't doing so already
      if (aMediaList.library != this._library) {
        if (this._library && this._library.guid != this._targetGUID) {
          this._library.removeListener(this);
        }
        
        this._library = aMediaList.library
      
        // If this is a list within a library, then
        // we need to listen for clear/remove in the
        // library
        if (!(aMediaList instanceof Ci.sbILibrary)) {
          this._batchHelper = new LibraryUtils.BatchHelper();

          var flags = Ci.sbIMediaList.LISTENER_FLAGS_BATCHBEGIN |
                      Ci.sbIMediaList.LISTENER_FLAGS_BATCHEND |
                      Ci.sbIMediaList.LISTENER_FLAGS_AFTERITEMREMOVED |
                      Ci.sbIMediaList.LISTENER_FLAGS_LISTCLEARED;
                      
          this._library.addListener(this, false, flags, null);
        }
      }
      
      if (!this._libraryManager) {
        this._libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                                .getService(Ci.sbILibraryManager);
        this._libraryManager.addListener(this);            
      }
        
      // Remember which medialist we are supposed to watch
      this._targetGUID = aMediaList.guid;
    
    
    // If set to null, shut down any listeners
    } else {
      if (this._libraryManager) {
        this._libraryManager.removeListener(this);
        this._libraryManager = null;
      }
      
      if (this._library) {
        this._library.removeListener(this);
        this._library = null;
      }
      this._batchHelper = null;
      this._targetGUID = null;
    } 
  },


  /**
   * Notifies the listener that the list has been removed, 
   * and then stops monitoring
   */
  _onMediaListRemoved: function RemovalMonitor_onMediaListRemoved() {
    //dump("RemovalMonitor: RemovalMonitor.onMediaListRemoved()\n");
    
    // Our list has been removed. Stop tracking.
    this.setMediaList(null);
    
    // Notify
    this._callback.onMediaListRemoved();
  },


  /**
   * \sa sbIMediaListListener
   */
  onItemAdded: function(aMediaList, aMediaItem, aIndex) { return true; },
  onItemUpdated: function(aMediaList, aMediaItem, aProperties) { return true },
  onItemMoved: function(aMediaList, aFromIndex, aToIndex) { return true },
  onBeforeItemRemoved: function(aMediaList, aMediaItem, aIndex) { return true; },
  onAfterItemRemoved: function RemovalMonitor_onAfterItemRemoved(aMediaList,
                                                                 aMediaItem,
                                                                 aIndex)
  {
    //dump("RemovalMonitor: RemovalMonitor.onAfterItemRemoved()\n");
    
    // Do no more if in a batch
    if (this._batchHelper.isActive()) {
      if (aMediaItem.guid == this._targetGUID) {
        this._removedInBatch = true;
      }
      return true;
    }

    // If our list was removed, notify
    if (aMediaItem.guid == this._targetGUID) {
      this._onMediaListRemoved();
    }

    return false;
  },
  onListCleared: function RemovalMonitor_onListCleared(aMediaList)
  {
    //dump("RemovalMonitor: RemovalMonitor.onListCleared()\n");

    // Do no more if in a batch
    if (this._batchHelper.isActive()) {
      this._removedInBatch = true;
      return true;
    }

    // The current media list must have been removed, so notify
    this._onMediaListRemoved();

    return false;
  },
  
  onBatchBegin: function RemovalMonitor_onBatchBegin(aMediaList)
  {
    this._batchHelper.begin();
  },
  onBatchEnd: function RemovalMonitor_onBatchEnd(aMediaList)
  {
    //dump("RemovalMonitor: RemovalMonitor.onBatchEnd()\n");
    
    this._batchHelper.end();
    // If the batch is still in progress do nothing
    if (this._batchHelper.isActive()) {
      return;
    }

    var removed = false;
    
    // If we know our target was removed during the batch, notify
    if (this._removedInBatch) {
      removed = true;
      
    // If we don't know for sure, we need to check
    } else if (this._targetGUID != this._library.guid) {

      // Check if our media list was removed
      try {
        this._library.getMediaItem(this._targetGUID);
      } catch (e) {
        removed = true;
      }
    }

    this._removedInBatch = false;

    if (removed) { 
      this._onMediaListRemoved();    
    }
  },

  /**
   * \sa sbILibraryManagerListener
   */
  onLibraryRegistered: function(aLibrary) {},
  onLibraryUnregistered: function RemovalMonitor_onLibraryUnregistered(aLibrary)
  {
    //dump("RemovalMonitor: RemovalMonitor.onLibraryUnregistered()\n");
    // If the current library was unregistered, notify
    if (this._library && this._library.equals(aLibrary)) {
      this._onMediaListRemoved(); 
    }
  },


  /**
   * \sa nsISupports
   */
  QueryInterface: function RemovalMonitor_QueryInterface(aIID) {
    if (!aIID.equals(Components.interfaces.nsISupports) &&
        !aIID.equals(Components.interfaces.sbILibraryManagerListener) &&
        !aIID.equals(Components.interfaces.sbIMediaListListener)) {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
  }
}

