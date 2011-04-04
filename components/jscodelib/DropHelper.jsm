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

    - ExternalDropHandler

      Used to handle external drops (ie, standard file drag and drop from the
      operating system).
      
    - InternalDropHandler

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

const SB_MEDIALISTDUPLICATEFILTER_CONTRACTID =
  "@songbirdnest.com/Songbird/Library/medialistduplicatefilter;1";
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
      var transfer = Cc["@mozilla.org/widget/transferable;1"]
                       .createInstance(Ci.nsITransferable);
      transfer.addDataFlavor(aFlavour);

      for (var i=0;i<nitems;i++) {
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
    // I know this is confusing but because numDropItems is a Number it causes
    // the Array constructor to pre-allocate N slots which should speed up 
    // pushing elements into it.
    var data = new Array(aSession.numDropItems);
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
  reportAddedTracks: function(aAdded, aDups, aUnsupported, aDestName, aIsDevice) {
    var msg = "";
    
    var single = SBString("library.singletrack");
    var plural = SBString("library.pluraltracks");
  
    if (aIsDevice) {
      if (aDups) {
        msg = SBFormattedString("device.tracksadded.with.dups", 
          [aDestName]);
      }
      else {
        msg = SBFormattedString("device.tracksadded", 
          [aDestName]);
      }
    }  
    else if (aDups && aUnsupported) {
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
                           aOtherDropsHandled,
                           aDevice) {
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
                                 aTargetList.name,
                                 aDevice);
    } else {
      DNDUtils.reportAddedTracks(aImportedInLibrary, 
                                 aDuplicates, 
                                 aUnsupported, 
                                 aTargetList.name,
                                 aDevice);
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
      var enumerator = Cc["@songbirdnest.com/Songbird/Library/EnumeratorWrapper;1"]
                         .createInstance(Ci.sbIMediaListEnumeratorWrapper);
      enumerator.initialize(this._mediaListView
                                .selection
                                .selectedIndexedMediaItems);
      this.items = enumerator;
      
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
   * Return true if the given device supports playlist
   */
  _doesDeviceSupportPlaylist: function(aDevice) {
    // Check the device capabilities to see if it supports playlists.
    // Device implementations may respond to CONTENT_PLAYLIST for either
    // FUNCTION_DEVICE or FUNCTION_AUDIO_PLAYBACK.
    var capabilities = aDevice.capabilities;
    var sbIDC = Ci.sbIDeviceCapabilities;
    try {
      if (capabilities.supportsContent(sbIDC.FUNCTION_DEVICE,
                                       sbIDC.CONTENT_PLAYLIST) ||
          capabilities.supportsContent(sbIDC.FUNCTION_AUDIO_PLAYBACK,
                                       sbIDC.CONTENT_PLAYLIST)) {
        return true;
      }
    } catch (e) {}

    // couldn't find PLAYLIST support in either the DEVICE
    // or AUDIO_PLAYBACK category
    return false;
  },

  _getTransferForDeviceChanges: function(aDevice, aSourceItems, aSourceList,
                                         aDestLibrary) {
    var differ = Cc["@songbirdnest.com/Songbird/Device/DeviceLibrarySyncDiff;1"]
                   .createInstance(Ci.sbIDeviceLibrarySyncDiff);

    var sourceLibrary;
    if (aSourceItems)
      sourceLibrary = aSourceItems.queryElementAt(0, Ci.sbIMediaItem).library;
    else
      sourceLibrary = aSourceList.library;

    var changeset = {};
    var destItems = {};
    differ.generateDropLists(sourceLibrary,
                             aDestLibrary,
                             aSourceList,
                             aSourceItems,
                             destItems,
                             changeset);

    return {changeset: changeset.value, items: destItems.value};
  },

  _notifyListeners: function(aListener, aTargetList, aNewList, aSourceItems,
                             aSourceList, aIsDevice) {
    var sourceLength = 0;

    if (aSourceList)
      sourceLength = aSourceList.length;
    else if (aSourceItems)
      sourceLength = aSourceItems.length;

    // Let our listeners know.
    if (aListener) {
      if (aNewList)
        aListener.onCopyMediaList(aTargetList, aNewList);

      if (sourceLength) {
        if (aSourceList)
          aListener.onFirstMediaItem(aSourceList.getItemByIndex(0));
        else if (aSourceItems)
          aListener.onFirstMediaItem(
                  aSourceItems.queryElementAt(0, Ci.sbIMediaItem));
      }
    }

    // These values are bogus, at least if the target is a library (and hence
    // already-existing items won't be copied). Better than nothing?
    this._dropComplete(aListener,
                       aTargetList,
                       sourceLength,
                       0,
                       0,
                       sourceLength,
                       0,
                       aIsDevice);
  },

  // aItem.library doesn't return the device library. Find the matching one
  // from the device.
  _getDeviceLibraryForItem: function(aDevice, aItem) {
    var lib = aItem.library;

    var libs = aDevice.content.libraries;
    for (var i = 0; i < libs.length; i++) {
      var deviceLib = libs.queryElementAt(i, Ci.sbIDeviceLibrary);
      if (lib.equals(deviceLib))
        return deviceLib;
    }

    return null;
  },

  // Transfer a dropped list where the source is a device, and the destination
  // is not.
  _dropListFromDevice: function(aDevice, aSourceList, aTargetList,
                                aDropPosition, aListener)
  {
    // Find out what changes need making.
    var changes = this._getTransferForDeviceChanges(aDevice,
            null, aSourceList, aTargetList.library);
    var changeset = changes.changeset;

    var targetLibrary = aTargetList.library;

    aDevice.importFromDevice(targetLibrary, changeset);

    // Get the list created, if any.
    var newlist = null;
    var changes = changeset.changes;
    for (var i = 0; i < changes.length; i++) {
      change = changes.queryElementAt(i, Ci.sbILibraryChange);
      if (change.itemIsList) {
        newlist = change.destinationItem;
        break;
      }
    }

    this._notifyListeners(aListener, aTargetList, newlist,
                          null, aSourceList, true);
  },

  _dropItemsFromDevice: function(aDevice, aSourceItems, aTargetList,
                                 aDropPosition, aListener)
  {
    // Find out what changes need making.
    var changes = this._getTransferForDeviceChanges(aDevice,
            aSourceItems, null, aTargetList.library);
    var changeset = changes.changeset;

    var targetLibrary = aTargetList.library;

    aDevice.importFromDevice(targetLibrary, changeset);

    if (aTargetList.library != aTargetList) {
      // This was a drop on an existing playlist. Now the items need
      // adding to the playlist - changes.items is an nsIArray containing these
      if (aTargetList instanceof Ci.sbIOrderableMediaList &&
          aDropPosition != -1) {
        aTargetList.insertSomeBefore(aDropPosition,
                                     ArrayConverter.enumerator(changes.items));
      }
      else {
        aTargetList.addSome(ArrayConverter.enumerator(changes.items));
      }
    }

    this._notifyListeners(aListener, aTargetList, null,
                          aSourceItems, null, true);
  },


  // Transfer a dropped list to a device.
  //
  // aTargetList may be a device library or a device playlist.
  _dropListOnDevice: function(aDevice, aSourceList, aTargetList,
                              aDropPosition, aListener) {
    // Find out what changes need making.
    var changes = this._getTransferForDeviceChanges(aDevice,
            null, aSourceList, aTargetList.library);
    var changeset = changes.changeset;

    var deviceLibrary = this._getDeviceLibraryForItem(aDevice, aTargetList);

    // Apply the changes to get the actual media items onto the device, even
    // if the device doesn't support playlists. This will add the items to the
    // device library and schedule all the necessary file copies/etc. This
    // also creates the device-side list if appropriate.
    aDevice.exportToDevice(deviceLibrary, changeset);

    // Get the list created, if any.
    var newlist = null;
    var changes = changeset.changes;
    for (var i = 0; i < changes.length; i++) {
      change = changes.queryElementAt(i, Ci.sbILibraryChange);
      if (change.itemIsList) {
        newlist = change.destinationItem;
        break;
      }
    }

    this._notifyListeners(aListener, aTargetList, newlist,
                          null, aSourceList, true);
  },

  // Transfer dropped items to a device.
  //
  // aTargetList may be a device library or a device playlist.
  _dropItemsOnDevice: function(aDevice, aSourceItems, aTargetList,
                               aDropPosition, aListener) {
    // Find out what changes need making.
    var changes = this._getTransferForDeviceChanges(aDevice,
            aSourceItems, null, aTargetList.library);
    var changeset = changes.changeset;

    var deviceLibrary = this._getDeviceLibraryForItem(aDevice, aTargetList);

    // Apply the changes to get the media items onto the device
    // if the device doesn't support playlists. This will add the items to the
    // device library and schedule all the necessary file copies/etc.
    aDevice.exportToDevice(deviceLibrary, changeset);

    if (aTargetList.library != aTargetList) {
      // This was a drop on an existing device playlist. Now the items need
      // adding to the playlist - changes.items is an nsIArray containing these
      if (aTargetList instanceof Ci.sbIOrderableMediaList &&
          aDropPosition != -1) {
        aTargetList.insertSomeBefore(aDropPosition,
                                     ArrayConverter.enumerator(changes.items));
      }
      else {
        aTargetList.addSome(ArrayConverter.enumerator(changes.items));
      }
    }

    this._notifyListeners(aListener, aTargetList, null,
                          aSourceItems, null, true);
  },

  // Transfer dropped items, where neither source or destination is on a device
  _dropItemsSimple: function(aItems, aTargetList, aDropPosition, aListener) {
    if (aTargetList instanceof Ci.sbIOrderableMediaList &&
        aDropPosition != -1) {
      aTargetList.insertSomeBefore(aDropPosition,
                                   ArrayConverter.enumerator(aItems));
    }
    else {
      aTargetList.addSome(ArrayConverter.enumerator(aItems));
    }

    this._notifyListeners(aListener, aTargetList, null, aItems, null, false);
  },

  // Drop from a list to another list, where neither list is on a device.
  _dropListSimple: function(aSourceList, aTargetList,
                            aDropPosition, aListener) {
    var newlist = null;

    var targetIsLibrary = (aTargetList instanceof Ci.sbILibrary);
    if (targetIsLibrary) {
      newlist = aTargetList.copyMediaList('simple', aSourceList, false);
    }
    else {
      if (aTargetList instanceof Ci.sbIOrderableMediaList &&
          aDropPosition != -1)
      {
        aTargetList.insertAllBefore(aDropPosition, aSourceList);
      }
      else {
        aTargetList.addAll(aSourceList);
      }
    }

    this._notifyListeners(aListener, aTargetList, newlist, null, aSourceList,
                          false);
  },

  /**
   * Performs the actual drop of a media list into another media list
   * This is the media list variant of _dropItems(); please see that method for
   * documentation.
   */
  _dropItemsList: function(aDragSession, aTargetList,
                           aDropPosition, aListener) {
    var context = DNDUtils.
      getInternalTransferDataForFlavour(aDragSession,
                                        TYPE_X_SB_TRANSFER_MEDIA_LIST, 
                                        Ci.sbIMediaListTransferContext);
    var sourceList = context.list;
    if (sourceList == aTargetList) {
      // uh oh - you can't drop a list onto itself
      this._dropComplete(aListener, aTargetList, 0, context.count, 0, 0, 0,
                         false);
      return;
    }

    // Check if our destination is on a device; some behaviour differs if it is.
    var deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                          .getService(Ci.sbIDeviceManager2);
    var destDevice = deviceManager.getDeviceForItem(aTargetList);
    var sourceDevice = deviceManager.getDeviceForItem(sourceList);

    if (destDevice) {
      // We use heavily customised behaviour if the target is a device.
      if (aTargetList.library == aTargetList) {
        // Drop onto a library
        this._dropListOnDevice(destDevice, sourceList, aTargetList,
                               aDropPosition, aListener);
      }
      else {
        // Drop onto a playlist in a library. This should actually be
        // treated as a drop of the ITEMS in the source list.
        items = this._itemsFromList(sourceList);
        this._dropItemsOnDevice(destDevice, items, aTargetList,
                                aDropPosition, aListener);
      }
    }
    else if (sourceDevice) {
      // We use special D&D behaviour for dragging a list _from_ a device too.
      if (aTargetList.library == aTargetList) {
        // Drop onto a library
        this._dropListFromDevice(sourceDevice, sourceList, aTargetList,
                                 aDropPosition, aListener);
      }
      else {
        // Drop onto a playlist in a library; treat as a drop of the items.
        items = this._itemsFromList(sourceList);
        this._dropItemsFromDevice(destDevice, items, aTargetList,
                                  aDropPosition, aListener);
      }
    }
    else {
      // No devices are involved, we just need the simple behaviour.
      this._dropListSimple(sourceList, aTargetList,
                           aDropPosition, aListener);
    }
  },

  // Get an nsIArray of sbIMediaItems from a media list
  _itemsFromList: function(list)
  {
    var items = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
    var listener = {
      onEnumerationBegin : function(aMediaList) {
        return Ci.sbIMediaListEnumerationListener.CONTINUE;
      },
      onEnumeratedItem : function(aMediaList, aMediaItem) {
        items.appendElement(aMediaItem, false);
        return Ci.sbIMediaListEnumerationListener.CONTINUE;
      },
      onEnumerationEnd : function(aMediaList, aStatusCode) {
      }
    };
    list.enumerateAllItems(listener, Ci.sbIMediaList.ENUMERATIONTYPE_SNAPSHOT);

    return items;
  },

  // Get an nsIArray of sbIMediaItems from an nsISimpleEnumerator of same.
  _itemsFromEnumerator: function(itemEnum) {
    var items = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);

    while (itemEnum.hasMoreElements())
      items.appendElement(itemEnum.getNext(), false);

    return items;
  },

  /**
   * Performs the actual drop of media items into a media list
   * This is the media items variant of _dropItems(); please see that method for
   * documentation.
   */
  _dropItemsItems: function(aDragSession, aTargetList,
                            aDropPosition, aListener) {
    var context = DNDUtils.
      getInternalTransferDataForFlavour(aDragSession,
                                        TYPE_X_SB_TRANSFER_MEDIA_ITEMS, 
                                        Ci.sbIMediaItemsTransferContext);

    var itemEnumerator = context.items;

    var items = this._itemsFromEnumerator(itemEnumerator);
    
    // are we dropping on a library? Assume all source items are from the same
    // library.
    if (aTargetList instanceof Ci.sbILibrary) {
      if (items.length > 0 &&
              items.queryElementAt(0, Ci.sbIMediaItem).library == aTargetList)
      {
        // can't add items to a library to which they already belong
        this._dropComplete(aListener, aTargetList, 0, context.count, 0, 0, 0,
                           false);
        return;
      }
    }

    var deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                          .getService(Ci.sbIDeviceManager2);
    var destDevice = deviceManager.getDeviceForItem(aTargetList);
    var sourceDevice = null;
    if (items.length > 0)
      sourceDevice = deviceManager.getDeviceForItem(
              items.queryElementAt(0, Ci.sbIMediaItem));

    if (destDevice) {
      // We use heavily customised behaviour if the target is a device.
      this._dropItemsOnDevice(destDevice, items, aTargetList,
                              aDropPosition, aListener);
    }
    else if (sourceDevice) {
      this._dropItemsFromDevice(sourceDevice, items, aTargetList,
                                aDropPosition, aListener);
    }
    else {
      // If the source is a device, or no devices are involved, we just need
      // the simple behaviour.
      this._dropItemsSimple(items, aTargetList,
                            aDropPosition, aListener);
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
                          otherDrops,
                          isDevice) {
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
                                otherDrops,
                                isDevice);
      }
    } else {
      DNDUtils.standardReport(targetList,
                              totalImported,
                              totalDups, 
                              totalUnsupported,
                              totalInserted,
                              otherDrops,
                              isDevice);
    }
  },
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




