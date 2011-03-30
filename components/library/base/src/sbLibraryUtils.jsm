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
const Cr = Components.results;

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

  getMediaListByGUID: function(aLibraryGUID, aMediaListGUID) {
    var mediaList = (aLibraryGUID instanceof Ci.sbILibrary) ?
                    aLibraryGUID :
                    this.manager.getLibrary( aLibraryGUID );

    // Are we loading the root library or a media list within it?
    if (aMediaListGUID && aLibraryGUID != aMediaListGUID) {
      mediaList = mediaList.getMediaItem( aMediaListGUID );
    }
    return mediaList;
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

  /**
   * \brief Create a new view for the param medialist.  This is most often the
   *        function you want for making views.  mediaList.createView() can be
   *        used directly, but then the mediaListView.filterConstraint should be
   *        carefully considered and likely set manually.
   *
   * \param aMediaList - The medialist for which the view should be created
   * \param aSearchString - OPTIONAL - If present this string contains the
   *                        values used to constrain the filter. aSearchString
   *                        will be split, space-delimited, into an array
   *                        of values passed to the new view's filter.
   *                        A text search filter will only be constrained by
   *                        the first item in this array, however.
   *
   * \return - A medialist view for aMediaList with any special sorting the
   *           list may have and the standard filter constraints, which can be
   *           further refined by aSearchString.
   */
  createStandardMediaListView: function(aMediaList, aSearchString) {
    var mediaList = aMediaList.QueryInterface(Ci.sbIMediaList);
    var mediaListView = mediaList.createView();

    // Get the sort for this list by parsing the list's column spec.  Then hit
    // the property manager to see if there is a special sort profile for this
    // ID
    var parser = new ColumnSpecParser(aMediaList, null);
    if (parser.sortID) {
      var propertyArray =
        Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"].
        createInstance(Ci.sbIMutablePropertyArray);
      propertyArray.strict = false;
      propertyArray.appendProperty(parser.sortID,
          (parser.sortIsAscending ? "a" : "d"));
      mediaListView.setSort(propertyArray);
    }

    // By default, we never want to show lists and hidden
    // things in the playlist
    mediaListView.filterConstraint = LibraryUtils.standardFilterConstraint;

    // Set up a standard search filter.
    // It can always be replaced later.
    var filter = mediaListView.cascadeFilterSet;
    filter.appendSearch(["*"], 1);

    if (aSearchString) {
      // Set the search
      var searchArray = aSearchString.split(" ");
      filter.set(0, searchArray, searchArray.length);
    } else {
      // Or not.
      filter.set(0, [], 0);
    }

    return mediaListView;
  },

  createConstrainedMediaListView: function(aMediaList, aConstraint,
                                           aSearchString) {
    var mediaListView = this.createStandardMediaListView(aMediaList,
                                                         aSearchString);

    // Take the existing filter constraint and intersect it with the
    // additional constraint we're about to apply
    var constraintBuilder =
          Cc["@songbirdnest.com/Songbird/Library/ConstraintBuilder;1"]
            .createInstance(Ci.sbILibraryConstraintBuilder);
    constraintBuilder.includeConstraint(mediaListView.filterConstraint);
    constraintBuilder.intersect();
    constraintBuilder.include(aConstraint[0], aConstraint[1]);
    mediaListView.filterConstraint = constraintBuilder.get();

    return mediaListView;
  },

  /**
   * \brief Determine if a url is a media tab url.
   * \param aURL url to check.
   * \param aBrowser an sbtabbrowser object (optional, for mediaTab test).
   * \return true if aURL is a media tab url, false if not.
   */
  isMediaTabURL: function(aURL, aBrowser) {
    if (!aURL) {
      return Components.results.NS_ERROR_INVALID_ARG;
    }
    if (aBrowser && !aBrowser.mediaTab) {
      // no media tab, can't be a media tab url
      return false;
    }
    var url = aURL;
    if (aURL instanceof Components.interfaces.nsIURI) {
      url = aURL.spec;
    }
    if (!(/chrome:\/\//.test(url))) {
      // not chrome
      return false;
    }

    const PREF_FIRSTRUN_URL = "songbird.url.firstrunpage";
    var firstRunURL = Cc["@mozilla.org/preferences-service;1"].
                        getService(Ci.nsIPrefBranch).
                        getCharPref(PREF_FIRSTRUN_URL);
    if (url == firstRunURL) {
      // first run url, sure this can be a media tab
      return true;
    }
    var service =
      Components.classes['@songbirdnest.com/servicepane/service;1']
      .getService(Components.interfaces.sbIServicePaneService);
    var node = service.getNodeForURL(url,
                                     Ci.sbIServicePaneService.URL_MATCH_PREFIX);
    if (!node) return false;
    return true;
  },

  /**
   * \brief Find an item in the main library whose guid matches the
   *        originItemGuid of the param aMediaItem and return it, or null if
   *        not found.
   * \param aMediaItem Retrieve an item from the mainLibrary whose guid
   *                   matches the originItemGuid of this mediaitem
   * \return The mediaitem from the mainLibrary whose guid matches the
   *         originItemGuid of aMediaItem.  null if none was found.
   */
  getMainLibraryOriginItem: function(aMediaItem) {
    var originGUID = aMediaItem.getProperty(SBProperties.originItemGuid);
    if (!originGUID) {
      return null;
    }
    try {
      var foundItem = this.mainLibrary.getMediaItem(originGUID);
      return foundItem;
    }
    catch (e) {
      // item couldn't be found, return null
      return null;
    }
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
                      Ci.sbIMediaList.LISTENER_FLAGS_BEFOREITEMREMOVED |
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
   * Notifies the listener that the list is about to be removed
   */
  _onBeforeMediaListRemoved: function RemovalMonitor_onBeforeMediaListRemoved()
  {
    // Notify
    this._callback.onBeforeMediaListRemoved();
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
  onBeforeItemRemoved: function (aMediaList, aMediaItem, aIndex) {
    // Do no more if in a batch
    if (this._batchHelper.isActive()) {
      if (aMediaItem.guid == this._targetGUID) {
        this._removedInBatch = true;
      }
      return true;
    }

    // If our list is about to be removed, notify
    if (aMediaItem.guid == this._targetGUID) {
      this._onBeforeMediaListRemoved();
    }

    return false;
  },
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
  onBeforeListCleared: function RemovalMonitor_onBeforeListCleared
                                  (aMediaList,
                                   aExcludeLists)
  {
    //dump("RemovalMonitor: RemovalMonitor.onBeforeListCleared()\n");

    return true;
  },
  onListCleared: function RemovalMonitor_onListCleared(aMediaList,
                                                       aExcludeLists)
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

/**
 * \class LibraryUtils.MediaListEnumeratorToArray
 * \brief Enumerates items in a media list and stores them in an array.
 */
LibraryUtils.MediaListEnumeratorToArray = function() {
}

LibraryUtils.MediaListEnumeratorToArray.prototype = {
  //
  // Media list enumeration array listener fields.
  //
  //   array                    Enumeration array.
  //

  array: null,


  /**
   * \brief Called when enumeration is about to begin.
   *
   * \param aMediaList - The media list that is being enumerated.
   *
   * \return true to begin enumeration, false to cancel.
   */

  onEnumerationBegin: function DSW_MLEAL_onEnumerationBegin(aMediaList) {
    // Initialize the enumeration array.
    this.array = [];
    return Ci.sbIMediaListEnumerationListener.CONTINUE;
  },


  /**
   * \brief Called once for each item in the enumeration.
   *
   * \param aMediaList - The media list that is being enumerated.
   * \param aMediaItem - The media item.
   *
   * \return true to continue enumeration, false to cancel.
   */

  onEnumeratedItem: function DSW_MLEAL_onEnumeratedItem(aMediaList,
                                                        aMediaItem) {
    // Add the item to the enumeration array.
    this.array.push(aMediaItem);
    return Ci.sbIMediaListEnumerationListener.CONTINUE;
  },


  /**
   * \brief Called when enumeration has completed.
   *
   * \param aMediaList - The media list that is being enumerated.
   * \param aStatusCode - A code to determine if the enumeration was successful.
   */

  onEnumerationEnd: function DSW_MLEAL_onEnumerationEnd(aMediaList,
                                                        aStatusCode) {
  }
}

/**
 * \class LibraryUtils.GlobalMediaListListener
 * \brief Attaches a listener to all currently existing libraries
 *        and lists in the system, and monitors the new playlists
 *        and libraries in order to automatically attach the
 *        listener whenever they are created.
 *        You may also specify a library to restrict the listeners
 *        to that library and its playlists' events instead of
 *        listening for all events in all libraries and lists.
 */
LibraryUtils.GlobalMediaListListener = function(aListener,
                                                aOwnsWeak,
                                                aFlags,
                                                aPropFilter,
                                                aOnlyThisLibrary) {
  if (aFlags === undefined) {
    aFlags = Components.interfaces.sbIMediaList.LISTENER_FLAGS_ALL;
  }

  this.listener = aListener;
  this.ownsWeak = aOwnsWeak;
  this.propFilter = aPropFilter;
  this.listenerFlags = aFlags;

  this.libraryManager =
    Cc["@songbirdnest.com/Songbird/library/Manager;1"]
      .getService(Ci.sbILibraryManager);

  this.listListener = {
    cb: this,
    onitemadded_skipbatch : false,
    onitemadded_stack     : [],
    onafteritemremoved_skipbatch : false,
    onafteritemremoved_stack     : [],
    batchcount : 0,
    onItemAdded: function(aMediaList, aMediaItem, aIndex) {
      if (aMediaItem instanceof Ci.sbIMediaList)
        this.cb.addMediaListListener(aMediaItem);
      if (this.batchcount > 0 && this.onitemadded_skipbatch)
        return Ci.sbIMediaListEnumerationListener.CONTINUE;
      if (this.cb.listener.onItemAdded(aMediaList, aMediaItem, aIndex) !=
          Ci.sbIMediaListEnumerationListener.CONTINUE) {
        this.onitemadded_skipbatch = true;
      }
      return Ci.sbIMediaListEnumerationListener.CONTINUE;
    },
    onBeforeItemRemoved: function(aMediaList, aMediaItem, aIndex) {
      return this.cb.listener.onBeforeItemRemoved(aMediaList, aMediaItem, aIndex);
    },
    onAfterItemRemoved: function(aMediaList, aMediaItem, aIndex) {
      if (aMediaItem instanceof Ci.sbIMediaList)
        this.cb.removeMediaListListener(aMediaItem);
      if (this.onafteritemremoved_skipbatch)
        return Ci.sbIMediaListEnumerationListener.CONTINUE;
      if (this.cb.listener.onAfterItemRemoved(aMediaList, aMediaItem, aIndex) !=
        Ci.sbIMediaListEnumerationListener.CONTINUE) {
        this.onafteritemremoved_skipbatch = true;
      }
      return Ci.sbIMediaListEnumerationListener.CONTINUE;
    },
    onItemUpdated: function(aMediaList, aMediaItem, aProperties) {
      return this.cb.listener.onItemUpdated(aMediaList, aMediaItem, aProperties);
    },
    onItemMoved: function(aMediaList, aFromIndex, aToIndex) {
      return this.cb.listener.onItemMoved(aMediaList, aFromIndex, aToIndex);
    },
    onBeforeListCleared: function(aMediaList, aExcludeLists) {
      return this.cb.listener.onBeforeListCleared(aMediaList, aExcludeLists);
    },
    onListCleared: function(aMediaList, aExcludeLists) {
      return this.cb.listener.onListCleared(aMediaList, aExcludeLists);
    },
    onBatchBegin: function(aMediaList) {
      ++this.batchcount;
      this.onitemadded_stack.push(this.onitemadded_skipbatch);
      this.onitemadded_skipbatch = false;
      this.onafteritemremoved_stack.push(this.onafteritemremoved_skipbatch);
      this.onafteritemremoved_skipbatch = false;
      return this.cb.listener.onBatchBegin(aMediaList);
    },
    onBatchEnd: function(aMediaList) {
      --this.batchcount;
      this.onitemadded_skipbatch = this.onitemadded_stack.pop();
      this.onafteritemremoved_skipbatch = this.onafteritemremoved_stack.pop();
      return this.cb.listener.onBatchEnd(aMediaList);
    },
    QueryInterface: function(iid) {
      if (iid.equals(Components.interfaces.sbIMediaListListener) ||
          iid.equals(Components.interfaces.nsISupports))
        return this;
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
  };


  var enumListener = {
    cb: this,
    onEnumerationBegin: function(aMediaList) { },
    onEnumerationEnd: function(aMediaList) { },
    onEnumeratedItem: function(aMediaList, aMediaItem) {
      this.cb.addMediaListListener(aMediaItem);
      return Components.interfaces.sbIMediaListEnumerationListener.CONTINUE;
    },
    QueryInterface: function(iid) {
      if (iid.equals(Components.interfaces.sbIMediaListEnumerationListener) ||
          iid.equals(Components.interfaces.nsISupports))
        return this;
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
  };

  var libs = this.libraryManager.getLibraries();
  while (libs.hasMoreElements()) {
    var library = libs.getNext();
    if (aOnlyThisLibrary &&
        library != aOnlyThisLibrary)
      continue;
    enumListener.onEnumeratedItem(library, library);
    library.
      enumerateItemsByProperty(
        SBProperties.isList,
        "1",
        enumListener,
        Components.interfaces.sbIMediaList.ENUMERATIONTYPE_LOCKING);
  }

  if (!aOnlyThisLibrary) {
    this.managerListener = {
      cb: this,
      onLibraryRegistered: function(aLibrary) {
        this.cb.addMediaListListener(aLibrary);
        this.cb.listener.onItemAdded(null, aLibrary, 0);
      },
      onLibraryUnregistered: function(aLibrary) {
        this.cb.listener.onBeforeItemRemoved(null, aLibrary, 0);
        this.cb.removeMediaListListener(aLibrary);
        this.cb.listener.onAfterItemRemoved(null, aLibrary, 0);
      },
      QueryInterface: function(iid) {
        if (iid.equals(Components.interfaces.sbILibraryManagerListener) ||
            iid.equals(Components.interfaces.nsISupports))
          return this;
        throw Components.results.NS_ERROR_NO_INTERFACE;
      }
    };

    this.libraryManager.addListener(this.managerListener);
  }
}

LibraryUtils.GlobalMediaListListener.prototype = {
  listener        : null,
  listento        : [],
  managerListener : null,
  listListener    : null,
  ownsWeak        : null,
  propFilter      : null,
  listenerFlags   : 0,

  shutdown: function() {
    for (var i=0;i<this.listento.length;i++)
      this.listento[i].removeListener(this.listListener);

    this.listento = [];
    this.listListener = null;

    this.listener = null;
    this.propFilter = null;

    if (this.managerListener)
      this.libraryManager.removeListener(this.managerListener);
    this.managerListener = null;

    this.libraryManager = null;
  },

  addMediaListListener: function(aList) {
    if (this.listento.indexOf(aList) >= 0)
      return;
    aList.addListener(this.listListener,
                      this.ownsWeak,
                      this.listenerFlags,
                      this.propFilter);
    this.listento.push(aList);
  },

  removeMediaListListener: function(aList) {
    aList.removeListener(this.listListener);
    var p = this.listento.indexOf(aList);
    if (p >= 0) this.listento.splice(p, 1);
  }
}

/**
 * This function is a big ugly hack until we get x-mtp channel working
 * We're punting for now and making mtp item editable, but they aren't
 * as far as the track editor is concerned.
 * presence of the device ID property means we're dealing with an MTP
 * device
 * TODO: XXX Remove this function when x-mtp channel lands and replace
 * it's calls with item.userEditable
 */
LibraryUtils.canEditMetadata = function (aItem) {
  var editable = aItem.userEditable;
  if (editable) {
    try {
      var contentSrc = aItem.contentSrc;
      if (contentSrc.scheme == "x-mtp")
        editable = false;
    } catch (e) {
      // Errors means it's not x-mtp and edtible
    }
  }
  return editable;
}
