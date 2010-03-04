/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

/*

  DropHelper Module

    This module contains three different helpers:

    - ExternalDropHander

      Used to handle external drops (ie, standard file drag and drop from the
      operating system).
      
    - InternalDropHander

      Used to handle internal drops (ie, mediaitems, medialists)
      
    - DNDUtils

      Contains methods that are used by both of the helpers, and that are useful for
      drag and drop operations in general.
  
*/

EXPORTED_SYMBOLS = [ "DNDUtils",
                     "ExternalDropHandler",
                     "InternalDropHandler" ];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ce = Components.Exception;
const Cu = Components.utils;

const URI_GENERIC_ICON_XPINSTALL = 
  "chrome://songbird/skin/base-elements/icon-generic-addon.png";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/SBJobUtils.jsm");
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://app/jsmodules/SBUtils.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");


/*

  DNDUtils
  
  This helper contains a number of methods that may be used when implementing
  a drag and drop handler.
  
*/


var DNDUtils = {

  // returns true if the drag session contains supported flavors
  isSupported: function(aDragSession, aFlavourArray) {
    for (var i=0;i<aFlavourArray.length;i++) {
      if (aDragSession.isDataFlavorSupported(aFlavourArray[i])) {
        return true;
      }
    }
    return false;
  },
  
  // adds the flavors in the array to the given flavourset
  addFlavours: function(aFlavourSet, aFlavourArray) {
    for (var i=0; i<aFlavourArray.length; i++) {
      aFlavourSet.appendFlavour(aFlavourArray[i]);
    }
  },

  // fills an array with the data for all items of a given flavor
  getTransferDataForFlavour: function(aFlavour, aSession, aArray) {
    if (!aSession) {
      var dragService = Cc["@mozilla.org/widget/dragservice;1"]
                            .getService(Ci.nsIDragService);
      aSession = dragService.getCurrentSession();
    }

    var nitems = aSession.numDropItems;
    var r = null;

    if (aSession.isDataFlavorSupported(aFlavour)) {

      for (var i=0;i<nitems;i++) {
        var transfer = Cc["@mozilla.org/widget/transferable;1"]
                           .createInstance(Ci.nsITransferable);

        transfer.addDataFlavor(aFlavour);
        aSession.getData(transfer, i);
        var data = {};
        var length = {};
        transfer.getTransferData(aFlavour, data, length);
        if (!r) r = data.value;
        if (aArray) aArray.push([data.value, length.value, aFlavour]);
      }
    }

    return r;
  },
  
  // similar to getTransferDataForFlavour but adds extraction of the internal
  // drag data from the dndSourceTracker, and queries an interface from the
  // result
  getInternalTransferDataForFlavour: function(aSession, aFlavour, aInterface) {
    var data = this.getTransferDataForFlavour(aFlavour, aSession);
    if (data) 
      return this.getInternalTransferData(data, aInterface);
    return null;
  },
  
  // similar to getTransferData but adds extraction of the internal drag data
  // from the dndSourceTracker, and queries an interface from the result
  getInternalTransferData: function(aData, aInterface) {
    // get the object from the dnd source tracker
    var dnd = Components.classes["@songbirdnest.com/Songbird/DndSourceTracker;1"]
        .getService(Ci.sbIDndSourceTracker);
    var source = dnd.getSourceSupports(aData);
    // and request the specified interface
    return source.QueryInterface(aInterface);
  },

  // returns an array with the data for any flavour listed in the given array
  getTransferData: function(aSession, aFlavourArray) {
    var data = [];
    for (var i=0;i<aFlavourArray.length;i++) {
      if (this.getTransferDataForFlavour(aFlavourArray[i], aSession, data)) 
        break;
    }
    return data;
  },
  
  // reports a custom temporary message to the status bar
  customReport: function(aMessage) {
    var SB_NewDataRemote = 
      Components.Constructor("@songbirdnest.com/Songbird/DataRemote;1",
                             "sbIDataRemote",
                             "init");
    var statusOverrideText = 
      SB_NewDataRemote( "faceplate.status.override.text", null );
    var statusOverrideType = 
      SB_NewDataRemote( "faceplate.status.override.type", null );

    statusOverrideText.stringValue = "";
    statusOverrideText.stringValue = aMessage;
    statusOverrideType.stringValue = "report";
  },

  // temporarily writes "X tracks added to <name>, Y tracks already present" 
  // in the status bar. if 0 is specified for aDups, the second part of the
  // message is skipped.
  reportAddedTracks: function(aAdded, aDups, aUnsupported, aDestName) {
    var msg = "";
    
    var single = SBString("library.singletrack");
    var plural = SBString("library.pluraltracks");
    
    if (aDups && aUnsupported) {
      msg = SBFormattedString("library.tracksadded.with.dups.and.unsupported",
        [aAdded, aDestName, aDups, aUnsupported]);
    }
    else if (aDups) {
      msg = SBFormattedString("library.tracksadded.with.dups",
        [aAdded, aDestName, aDups]);
    }
    else if (aUnsupported) {
      msg = SBFormattedString("library.tracksadded.with.unsupported",
        [aAdded, aDestName, aUnsupported]);
    }
    else {
      msg = SBFormattedString("library.tracksadded",
        [aAdded, aDestName]);
    }

    this.customReport(msg);
  },
  
  // reports stats on the statusbar using standard rules for what to show and
  // in which circumstances
  standardReport: function(aTargetList,
                           aImportedInLibrary,
                           aDuplicates,
                           aUnsupported,
                           aInsertedInMediaList,
                           aOtherDropsHandled) {
    // do not report anything if all we did was drop an XPI
    if ((aImportedInLibrary == 0) &&
        (aInsertedInMediaList == 0) &&
        (aDuplicates == 0) &&
        (aUnsupported == 0) && 
        (aOtherDropsHandled != 0)) 
      return;
    
    // report different things depending on whether we dropped
    // on a library, or just a list
    if (aTargetList != aTargetList.library) {
      DNDUtils.reportAddedTracks(aInsertedInMediaList, 
                                 0, 
                                 aUnsupported,
                                 aTargetList.name);
    } else {
      DNDUtils.reportAddedTracks(aImportedInLibrary, 
                                 aDuplicates, 
                                 aUnsupported, 
                                 aTargetList.name);
    }
  }
}

/* MediaListViewSelectionTransferContext
 *
 * Create a drag and drop context containing the selected items in the view
 * which is passed in.
 * Implements: sbIMediaItemsTransferContext
 */
DNDUtils.MediaListViewSelectionTransferContext = function (mediaListView) {
    this.items          = null; // filled in during reset()
    this.indexedItems   = null;
    this.source         = mediaListView.mediaList;
    this.count          = mediaListView.selection.count;
    this._mediaListView = mediaListView;
    this.reset();
};
DNDUtils.MediaListViewSelectionTransferContext.prototype = {
    reset: function() {
      // Create an enumerator that unwraps the sbIIndexedMediaItem enumerator
      // which selection provides.
      var enumerator = this._mediaListView.selection.selectedIndexedMediaItems;
      this.items = {
        hasMoreElements : function() {
          return enumerator.hasMoreElements();
        },
        getNext : function() {
          var item = enumerator.getNext().mediaItem
                       .QueryInterface(Components.interfaces.sbIMediaItem);
          
          item.setProperty(SBProperties.downloadStatusTarget,
                           item.library.guid + "," + item.guid);
          return item;
        },
        QueryInterface : function(iid) {
          if (iid.equals(Components.interfaces.nsISimpleEnumerator) ||
              iid.equals(Components.interfaces.nsISupports))
            return this;
          throw Components.results.NS_NOINTERFACE;
        }
      };

      // and here's the wrapped form for those cases where you want it
      this.indexedItems = this._mediaListView.selection.selectedIndexedMediaItems;
    },
    QueryInterface : function(iid) {
      if (iid.equals(Components.interfaces.sbIMediaItemsTransferContext) ||
          iid.equals(Components.interfaces.nsISupports))
        return this;
      throw Components.results.NS_NOINTERFACE;
    }
  };
  
/* EntireMediaListViewTransferContext
 *
 * Create a drag and drop context containing all the items in the view
 * which is passed in.
 * Implements: sbIMediaItemsTransferContext
 */
DNDUtils.EntireMediaListViewTransferContext = function(view) {
    this.items          = null;
    this.indexedItems   = null;
    this.source         = view.mediaList;
    this.count          = view.length;
    this._mediaListView = view;
    this.reset();
  }
DNDUtils.EntireMediaListViewTransferContext.prototype = {
  reset: function() {
    // Create an ugly pseudoenumerator
    var that = this;
    this.items = {
      i: 0,
      hasMoreElements : function() {
        return this.i < that._mediaListView.length;
      },
      getNext : function() {
        var item = that._mediaListView.getItemByIndex(this.i++);
        item.setProperty(SBProperties.downloadStatusTarget,
                         item.library.guid + "," + item.guid);
        return item;
      },
      QueryInterface : function(iid) {
        if (iid.equals(Components.interfaces.nsISimpleEnumerator) ||
            iid.equals(Components.interfaces.nsISupports))
          return this;
        throw Components.results.NS_NOINTERFACE;
      }
    };
  },
  QueryInterface : function(iid) {
    if (iid.equals(Components.interfaces.sbIMediaItemsTransferContext) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_NOINTERFACE;
  }
};

/* MediaListTransferContext
 *
 * A transfer context suitable for moving a single media list around the system.
 * As of this writing, the only place to create a drag/drop session of a single
 * media list is the service pane, though it is also possible that extension
 * developers will create them in the playlist widget.
 */
DNDUtils.MediaListTransferContext = function (item, mediaList) {
    this.item   = item;
    this.list   = item;
    this.source = mediaList;
    this.count  = 1;
  }
DNDUtils.MediaListTransferContext.prototype = {
    QueryInterface : function(iid) {
      if (iid.equals(Components.interfaces.sbIMediaListTransferContext) ||
          iid.equals(Components.interfaces.nsISupports))
        return this;
      throw Components.results.NS_NOINTERFACE;
    }
  }

/*

  InternalDropHandler
  
  
This helper is used to let you handle internal drops (mediaitems and medialists)
and inject the items into a medialist, potentially at a specific position.

There are two ways of triggering a drop handling, the question of which one you
should be using depends on how it is you would like to handle the drop:

To handle a drop in a generic manner, and have all dropped items automatically
directed to the default library, all you need to do is add the following code in
your onDrop/ondragdrop handler:

  InternalDropHandler.drop(window, dragSession, dropHandlerListener);

The last parameter is optional, it allows you to receive notifications. Here is
a minimal implementation:

  var dropHandlerListener = {
    // called when the drop handling has completed
    onDropComplete: function(aTargetList,
                             aImportedInLibrary,
                             aDuplicates,
                             aUnsupported,
                             aInsertedInMediaList,
                             aOtherDropsHandled) { 
      // returning true causes the standard drop report to be printed
      // on the status bar, it is equivalent to calling standardReport
      // using the parameters received on this callback
      return true; 
    },
    // called when the first item is handled (eg, to implement playback)
    onFirstMediaItem: function(aTargetList, aFirstMediaItem) { }
    // called when a medialist has been copied from a different source library
    onCopyMediaList: function(aSourceList, aNewList) { }
  };

To handle a drop with a specific mediaList target and drop insertion point, use
the following code:

  InternalDropHandler.dropOnList(window, 
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
    InternalDropHandler.addFlavours(flavours);
    return flavours;

Also, getTransferData, getInternalTransferDataForFlavour, and 
getTransferDataForFlavour may be used to inspect the content of the dragSession 
before deciding what to do with it.

*/

const TYPE_X_SB_TRANSFER_MEDIA_ITEM = "application/x-sb-transfer-media-item";
const TYPE_X_SB_TRANSFER_MEDIA_LIST = "application/x-sb-transfer-media-list";
const TYPE_X_SB_TRANSFER_MEDIA_ITEMS = "application/x-sb-transfer-media-items";

var InternalDropHandler = {

  supportedFlavours: [ TYPE_X_SB_TRANSFER_MEDIA_ITEM,
                       TYPE_X_SB_TRANSFER_MEDIA_LIST,
                       TYPE_X_SB_TRANSFER_MEDIA_ITEMS ],
  
  // returns true if the drag session contains supported internal flavors
  isSupported: function(aDragSession) {
    return DNDUtils.isSupported(aDragSession, this.supportedFlavours);
  },
  
  // performs a default drop of the drag session. media items go to the
  // main library.
  drop: function(aWindow, aDragSession, aListener) {
    var mainLibrary = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                          .getService(Ci.sbILibraryManager)
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
    this._dropItems(aDragSession, 
                    aTargetList, 
                    aDropPosition, 
                    aListener);
  },

  // call this to automatically add the supported internal flavors 
  // to a flavourSet object
  addFlavours: function(aFlavourSet) {
    DNDUtils.addFlavours(aFlavourSet, this.supportedFlavours);
  },

  // returns an array with the data for any supported internal flavor
  getTransferData: function(aSession) {
    return DNDUtils.getTransferData(aSession, this.supportedFlavours);
  },
  
  // simply forward the call. here in this object for completeness
  // see DNDUtils.getTransferDataForFlavour for more info
  getTransferDataForFlavour: function(aFlavour, aSession, aArray) {
    return DNDUtils.getTransferDataForFlavour(aFlavour, aSession, aArray);
  },

  // --------------------------------------------------------------------------
  // methods below this point are pretend-private
  // --------------------------------------------------------------------------
  
  /**
   * Performs the actual drop of items into a media list
   * \param aDragSession The nsIDragSession associated with the drop
   * \param aTargetList The list to drop the items into
   * \param aDropPosition The position to drop the items into in the list; if
   *                      the list is unordered or if the position is invalid,
   *                      the items will be inserted at the end of the list
   *                      instead.
   * \param aListener The listener for drop progress feedback; see the
   *                  documentation for InternalDropHandler for details.
   */
  _dropItems: function(aDragSession, aTargetList, aDropPosition, aListener) {
    // are we dropping a media list ?
    if (aDragSession.isDataFlavorSupported(TYPE_X_SB_TRANSFER_MEDIA_LIST)) {
      this._dropItemsList(aDragSession, aTargetList, aDropPosition, aListener);
    } else if (aDragSession.isDataFlavorSupported(TYPE_X_SB_TRANSFER_MEDIA_ITEMS)) {
      this._dropItemsItems(aDragSession, aTargetList, aDropPosition, aListener);
    }
  },
  
  /**
   * Performs the actual drop of a media list into another media list
   * This is the media list variant of _dropItems(); please see that method for
   * documentation.
   */
  _dropItemsList: function(aDragSession, aTargetList, aDropPosition, aListener) {
    var context = DNDUtils.
      getInternalTransferDataForFlavour(aDragSession,
                                        TYPE_X_SB_TRANSFER_MEDIA_LIST, 
                                        Ci.sbIMediaListTransferContext);
    var list = context.list;

    // record metrics
    var metrics_totype = aTargetList.library.getProperty(SBProperties.customType);
    var metrics_fromtype = list.library.getProperty(SBProperties.customType);
    this._metrics("app.servicepane.copy", 
                  metrics_fromtype, 
                  metrics_totype, 
                  list.length);

    // are we dropping on a library ?
    var targetListIsLibrary = (aTargetList instanceof Ci.sbILibrary);
    var rejectedItems = 0; // for devices we may have to reject some items

    if (targetListIsLibrary) {
      // want to copy the list and the contents
      if (aTargetList == list.library) {
        return;
      }

      // create a copy of the list
      var newlist = aTargetList.copyMediaList('simple', list);
      
      if (aListener) {
        aListener.onCopyMediaList(aTargetList, newlist);
        if (newlist.length > 0)
          aListener.onFirstMediaItem(newlist.getItemByIndex(0));
      }

      // lone> some of these values may not be accurate, this assumes that all 
      // tracks have been copied, which is true if both the source and target 
      // are playlists, but could be false if the target is a library. better 
      // than nothing anyway.
      this._dropComplete(aListener,
                         aTargetList,
                         list.length,
                         0,
                         rejectedItems,
                         list.length,
                         0);

    } else {
      if (context.list == aTargetList) {
        // uh oh - you can't drop a list onto itself
        return;
      }
      
      // is this a device media list we're going to?
      var deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                            .getService(Ci.sbIDeviceManager2);
      var selectedDevice = deviceManager.getDeviceForItem(aTargetList);
      
      // make an enumerator with all the items from the source playlist        
      var allItems = {
        items: [],
        itemsPending: 0,
        enumerationEnded: false,
        onEnumerationBegin: function(aMediaList) {
          this.items = [];
        },
        onEnumeratedItem: function(aMediaList, aMediaItem) {
          if (selectedDevice) {
            this.itemsPending++;
            selectedDevice.supportsMediaItem(aMediaItem, this);
          }
          else {
            this.items.push(aMediaItem);
          }
        },
        onEnumerationEnd: function(aMediaList, aResultCode) {
          this.enumerationEnded = true;
          this._checkEnumerationComplete();
        },
        onSupportsMediaItem: function(aMediaItem, aIsSupported) {
          if (!aIsSupported) {
            rejectedItems++;
          }
          this.itemsPending--;
          this._checkEnumerationComplete();
        },
        _checkEnumerationComplete : function() {
          if (this.enumerationEnded && (this.itemsPending < 1))
            onEnumerateComplete();
        },
      };

      list.enumerateAllItems(allItems);

      function onEnumerateComplete() {
        // add the contents
        if (aDropPosition != -1 &&
            aTargetList instanceof Ci.sbIOrderableMediaList)
        {
          aTargetList.insertSomeBefore(aDropPosition,
                                       ArrayConverter.enumerator(allItems.items));
        } else {
          aTargetList.addSome(ArrayConverter.enumerator(allItems.items));
        }
        if (aListener && list.length > 0) {
          aListener.onFirstMediaItem(list.getItemByIndex(0));
        }
  
        // lone> some of these values may not be accurate, this assumes that all 
        // tracks have been copied, which is true if both the source and target 
        // are playlists, but could be false if the target is a library. better 
        // than nothing anyway.
        InternalDropHandler._dropComplete(aListener,
                                          aTargetList,
                                          list.length,
                                          0,
                                          rejectedItems,
                                          list.length,
                                          0);
      }
    }
  },

  /**
   * Performs the actual drop of media items into a media list
   * This is the media items variant of _dropItems(); please see that method for
   * documentation.
   */
  _dropItemsItems: function(aDragSession, aTargetList, aDropPosition, aListener) {
    var context = DNDUtils.
      getInternalTransferDataForFlavour(aDragSession,
                                        TYPE_X_SB_TRANSFER_MEDIA_ITEMS, 
                                        Ci.sbIMediaItemsTransferContext);

    var items = context.items;
    
    // are we dropping on a library ?
    if (aTargetList instanceof Ci.sbILibrary) {
      if (items.hasMoreElements() && 
          items.getNext().library == aTargetList) {
        // can't add items to a library to which they already belong
        this._dropComplete(aListener, aTargetList, 0, context.count, 0, 0, 0);
        return;
      }
      // we just ate an item; reset the enumerator
      context.reset();
      items = context.items;
    }

    // record metrics
    var metrics_totype = aTargetList.library.getProperty(SBProperties.customType);
    var metrics_fromtype = context.source.library.getProperty(SBProperties.customType);
    this._metrics("app.servicepane.copy", 
                  metrics_fromtype, 
                  metrics_totype, 
                  context.count);

    // Create a media item duplicate enumerator filter to count the number of
    // duplicate items and to remove them from the enumerator if the target is
    // a library.
    var dupFilter = new LibraryUtils.EnumeratorDuplicateFilter
                                       (aTargetList.library);
    if (aTargetList instanceof Ci.sbILibrary) {
      dupFilter.removeDuplicates = true;
    }

    // If the library is a device library, get the device object.
    var libraryDevice = null;
    try {
      var deviceLibrary =
            aTargetList.library.QueryInterface(Ci.sbIDeviceLibrary);
      libraryDevice = deviceLibrary.device;
    } catch (ex) {
      libraryDevice = null;
    }

    // Create a filtered item enumerator that filters duplicates
    var func = function(aElement) {
      return dupFilter.filter(aElement);
    };
    var filteredItems = new SBFilteredEnumerator(items, func);
    var totalUnsupported = 0;

    // Create an enumerator that wraps the enumerator we were handed.
    // We use this to set downloadStatusTarget and to notify the onFirstMediaItem
    // listener, as well as counting the number of unsupported items
    var unwrapper = {
      enumerator: filteredItems,
      first: true,
      itemsPending: 0,
      _hasMoreElements: true,

      hasMoreElements : function() {
        this._hasMoreElements = this.enumerator.hasMoreElements();
        this._checkEnumerationComplete();
        return this._hasMoreElements;
      },
      getNext : function() {
        var item = this.enumerator.getNext();

        if (this.first) {
          this.first = false;
          if (aListener)
            aListener.onFirstMediaItem(item);
        }

        item.setProperty(SBProperties.downloadStatusTarget,
                         item.library.guid + "," + item.guid);

        if (libraryDevice) {
          this.itemsPending++;
          libraryDevice.supportsMediaItem(item, this);
        }

        return item;
      },
      onSupportsMediaItem : function(aMediaItem, aIsSupported) {
        if (!aIsSupported) {
          totalUnsupported++;
        }
        this.itemsPending--;
        this._checkEnumerationComplete();
      },
      _checkEnumerationComplete : function() {
        if (!this._hasMoreElements && (this.itemsPending < 1))
          onEnumerateComplete();
      },
      QueryInterface : XPCOMUtils.generateQI([Ci.nsISimpleEnumerator,
                                              Ci.sbIDeviceSupportsItemCallback])
    }

    if (aDropPosition != -1 && aTargetList instanceof Ci.sbIOrderableMediaList) {
      aTargetList.insertSomeBefore(unwrapper, aDropPosition);
    } else {
      aTargetList.addSome(unwrapper);
    }

    function onEnumerateComplete() {
      var totalImported = dupFilter.mediaItemCount - dupFilter.duplicateCount;
      var totalDups = dupFilter.duplicateCount;
      var totalInserted = dupFilter.mediaItemCount;
      if (dupFilter.removeDuplicates)
        totalInserted = totalImported;
      totalImported -= totalUnsupported;
      totalInserted -= totalUnsupported;
  
      InternalDropHandler._dropComplete(aListener,
                                        aTargetList,
                                        totalImported,
                                        totalDups,
                                        totalUnsupported,
                                        totalInserted,
                                        0);
    }
  },

  // called when the whole drop handling operation has completed, used
  // to notify the original caller and free up any resources we can
  _dropComplete: function(listener,
                          targetList, 
                          totalImported, 
                          totalDups, 
                          totalUnsupported,
                          totalInserted,
                          otherDrops) {
    // notify the listener that we're done
    if (listener) {
      if (listener.onDropComplete(targetList,
                                  totalImported,
                                  totalDups,
                                  totalUnsupported, 
                                  totalInserted,
                                  otherDrops)) {
        DNDUtils.standardReport(targetList,
                                totalImported,
                                totalDups,
                                totalUnsupported, 
                                totalInserted,
                                otherDrops);
      }
    } else {
      DNDUtils.standardReport(targetList,
                              totalImported,
                              totalDups, 
                              totalUnsupported,
                              totalInserted,
                              otherDrops);
    }
  },
  
  _metrics: function(aCategory, aUniqueID, aExtraString, aIntValue) {
    var metrics = Components.classes["@songbirdnest.com/Songbird/Metrics;1"]
                      .createInstance(Ci.sbIMetrics);
    metrics.metricsAdd(aCategory, aUniqueID, aExtraString, aIntValue);
  }
 
}

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

Two similar methods (dropUrls and dropUrlsOnList) exist that let you simulate a 
drop by giving a list of URLs, and triggering the same handling as the one that 
would happen had these URLs been part of a dragsession drop.

The other public methods in this helper can be used to simplify the rest of your
drag and drop handler as well. For instance, an nsDragAndDrop observer's 
getSupportedFlavours() method may be implemented simply as:

    var flavours = new FlavourSet();
    ExternalDropHandler.addFlavours(flavours);
    return flavours;

Also, getTransferData and DNDUtils.getTransferDataForFlavour may be used to 
inspect the content of the dragSession before deciding what to do with it.

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

var ExternalDropHandler = {

  supportedFlavours: [ "application/x-moz-file",
                       "text/x-moz-url",
                       "text/unicode"],
  
  // returns true if the drag session contains supported external flavors
  isSupported: function(aDragSession) {
    return DNDUtils.isSupported(aDragSession, this.supportedFlavours);
  },
  
  // performs a default drop of the drag session. media items go to the
  // main library. 
  drop: function(aWindow, aDragSession, aListener) {
    var mainLibrary = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                          .getService(Ci.sbILibraryManager)
                          .mainLibrary;
    this.dropOnList(aWindow, aDragSession, mainLibrary, -1, aListener);
  },

  // performs a default drop of a list of urls. media items go to the
  // main library. 
  dropUrls: function(aWindow, aUrlArray, aListener) {
    var mainLibrary = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                          .getService(Ci.sbILibraryManager)
                          .mainLibrary;
    this.dropUrlsOnList(aWindow, aUrlArray, mainLibrary, -1, aListener);
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
                    null,
                    aTargetList, 
                    aDropPosition, 
                    aListener);
  },

  // perform a drop of a list of urls onto a medialist. media items are inserted at 
  // the specified position in the list if that list is orderable. otherwise, or if the
  // position is invalid, the items are added to the target list.
  dropUrlsOnList: function(aWindow, 
                           aUrlArray, 
                           aTargetList, 
                           aDropPosition, 
                           aListener) {
    if (!aTargetList) {
      throw new Error("No target medialist specified for dropOnList");
    }
    this._dropFiles(aWindow,
                    null, 
                    aUrlArray,
                    aTargetList, 
                    aDropPosition, 
                    aListener);
  },

  // call this to automatically add the supported external flavors 
  // to a flavourSet object
  addFlavours: function(aFlavourSet) {
    DNDUtils.addFlavours(aFlavourSet, this.supportedFlavours);
  },

  // returns an array with the data for any supported external flavor
  getTransferData: function(aSession) {
    return DNDUtils.getTransferData(aSession, this.supportedFlavours);
  },

  // simply forward the call. here in this object for completeness
  // see DNDUtils.getTransferDataForFlavour for more info
  getTransferDataForFlavour: function(aFlavour, aSession, aArray) {
    return DNDUtils.getTransferDataForFlavour(aFlavour, aSession, aArray);
  },

  // --------------------------------------------------------------------------
  // methods below this point are pretend-private
  // --------------------------------------------------------------------------
  
  _listener            : null,  // listener object, for notifications

  // initiate the handling of all dropped files: this handling is sliced up 
  // into a number of 'frames', each frame importing one item, or queuing up 
  // one directory for later import. at the end of each frame, the 
  // _nextImportDropFrame method is called to schedule the next frame using a 
  // short timer, so as to give the UI time to catch up, and we keep doing that 
  // until everything in the file queue has been processed. when that's done, 
  // we then look for queued directory scans, which we give to the directory 
  // import service. after the directories have been processed, we notify the 
  // listener that processing has ended. Note that the function can take either
  // a drag session of an array of URLs. If both are provided, only the session
  // will be handled (ie, the method is not meant to be called with both a session
  // and a urlarray).
  _dropFiles: function(window, session, urlarray, targetlist, position, listener) {
  
    // check that we are indeed processing an external drop
    if (session && !this.isSupported(session)) {
      return;
    }

    // if we are on win32, we will need to make local filenames lowercase
    var lcase = (this._getPlatformString() == "Windows_NT");
    
    // get drop data in any of the supported formats
    var dropdata = session ? this.getTransferData(session) : urlarray;
    
    // reset first media item, so we know to record it again
    this._firstMediaItem = null;
    
    // remember listener
    this._listener = listener;

    // Install all the dropped XPI files at the same time
    var xpiArray = {};
    var xpiCount = 0;

    var uriList = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                      .createInstance(Ci.nsIMutableArray);

    var ioService = Cc["@mozilla.org/network/io-service;1"]
                        .getService(Ci.nsIIOService);
    
    // process all entries in the drop
    for (var dropentry in dropdata) {
      var dropitem = dropdata[dropentry];
    
      var item, flavour;
      if (session) {
        item = dropitem[0];
        flavour = dropitem[2];
      } else {
        item = dropitem;
        flavour = "text/x-moz-url";
      }
      var islocal = true;
      var rawData;
      
      var prettyName;

      if (flavour == "application/x-moz-file") {
        var ioService = Cc["@mozilla.org/network/io-service;1"]
                            .getService(Ci.nsIIOService);
        var fileHandler = ioService.getProtocolHandler("file")
                          .QueryInterface(Ci.nsIFileProtocolHandler);
        rawData = fileHandler.getURLSpecFromFile(item);

        // Check to see that this is a xpi/jar - if so handle that event
        if ( /\.(xpi|jar)$/i.test(rawData) && (item instanceof Ci.nsIFile) ) {
          xpiArray[item.leafName] = {
            URL: rawData,
            IconURL: URI_GENERIC_ICON_XPINSTALL,
            toString: function() { return this.URL; }
          };
          ++xpiCount;
        }
      } else {
        if (item instanceof Ci.nsISupportsString) {
          rawData = item.toString();
        } else {
          rawData = ""+item;
        }
        if (rawData.toLowerCase().indexOf("http://") >= 0) {
          // remember that this is not a local file
          islocal = false;
        } else if (rawData.toLowerCase().indexOf("file://") >= 0) {
          islocal = true;
        } else {
          // not a url, ignore
          continue;
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
      uriList.appendElement(ioService.newURI(rawData, null, null), false); 
    }

    // Timeout the XPI install
    if (xpiCount > 0) {
      window.setTimeout(window.installXPIArray, 10, xpiArray);
    }

    var uriImportService = Cc["@songbirdnest.com/uri-import-service;1"]
                               .getService(Ci.sbIURIImportService);
    uriImportService.importURIArray(uriList,
                                    window,
                                    targetlist,
                                    position,
                                    this);
  },

  // sbIURIImportListener
  onImportComplete: function(aTargetMediaList,
                             aTotalImportCount,
                             aTotalDupeCount,
                             aTotalUnsupported,
                             aTotalInserted,
                             aOtherDrops)
  {
    if (this._listener) {
      if (this._listener.onDropComplete(aTargetMediaList,
                                        aTotalImportCount,
                                        aTotalDupeCount, 
                                        aTotalInserted,
                                        aTotalUnsupported,
                                        aOtherDrops)) {
        DNDUtils.standardReport(aTargetMediaList,
                                aTotalImportCount,
                                aTotalDupeCount, 
                                aTotalUnsupported, 
                                aTotalInserted,
                                aOtherDrops);
      }
    } 
    else {
      DNDUtils.standardReport(aTargetMediaList,
                              aTotalImportCount,
                              aTotalInserted, // usually dupes
                              aTotalUnsupported, 
                              aTotalInserted,
                              aOtherDrops);
    }
  },
  
  
  onFirstMediaItem: function(aTargetMediaList, aTargetMediaItem)
  {
    if (this._listener) {
      this._listener.onFirstMediaItem(aTargetMediaList, aTargetMediaItem);
    }
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
      var user_agent = navigator.userAgent;
      if (user_agent.indexOf("Windows") != -1)
        return "Windows_NT";
      else if (user_agent.indexOf("Mac OS X") != -1)
        return "Darwin";
      else if (user_agent.indexOf("Linux") != -1)
        return "Linux";
      else if (user_agent.indexOf("SunOS") != -1)
        return "SunOS";
      return "";
    }
  }
 
}




