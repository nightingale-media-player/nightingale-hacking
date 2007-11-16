/** vim: ts=2 sw=2 expandtab
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

/**
 * \file sbLibraryServicePane.js
 */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://app/components/sbProperties.jsm");
Components.utils.import("resource://app/components/sbLibraryUtils.jsm");

const CONTRACTID = "@songbirdnest.com/servicepane/library;1";
const ROOTNODE = "SB:Bookmarks";

const PROP_ISLIST = "http://songbirdnest.com/data/1.0#isList";
const PROP_ISHIDDEN = "http://songbirdnest.com/data/1.0#hidden";

const URN_PREFIX_ITEM = 'urn:item:';
const URN_PREFIX_LIBRARY = 'urn:library:';

// TODO: Remove this
const URL_PLAYLIST_DISPLAY = "chrome://songbird/content/xul/sbLibraryPage.xul?"

const LSP = 'http://songbirdnest.com/rdf/library-servicepane#';
const SP='http://songbirdnest.com/rdf/servicepane#';


const TYPE_X_SB_TRANSFER_MEDIA_ITEM = "application/x-sb-transfer-media-item";
const TYPE_X_SB_TRANSFER_MEDIA_LIST = "application/x-sb-transfer-media-list";
const TYPE_X_SB_TRANSFER_MEDIA_ITEMS = "application/x-sb-transfer-media-items";

/** 
 * Given the arguments var of a function, dump the
 * name of the function and the parameters provided
 */
function logcall(parentArgs) {
  dump("\n");
  dump(parentArgs.callee.name + "(");
  for (var i = 0; i < parentArgs.length; i++) {
    dump(parentArgs[i]);
    if (i < parentArgs.length - 1) {
      dump(', ');
    }
  }
  dump(")\n");
}



/**
 * /class sbLibraryServicePane
 * /brief Provides library related nodes for the service pane
 * \sa sbIServicePaneService sbILibraryServicePaneService
 */
function sbLibraryServicePane() {
  this._servicePane = null;
  this._libraryManager = null;
  this._lastShortcuts = null;
  this._lastMenuitems = null;
  
  // use the default stringbundle to translate tree nodes
  this.stringbundle = null;

  this._batch = new BatchHelper();
  this._refreshPending = false;
}
sbLibraryServicePane.prototype.QueryInterface = 
function sbLibraryServicePane_QueryInterface(iid) {
  if (!iid.equals(Ci.nsISupports) &&
    !iid.equals(Ci.nsIObserver) &&
    !iid.equals(Ci.sbIServicePaneModule) &&
    !iid.equals(Ci.sbILibraryServicePaneService) &&    
    !iid.equals(Ci.sbILibraryManagerListener) &&
    !iid.equals(Ci.sbIMediaListListener)) {
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
  return this;
}


//////////////////////////
// sbIServicePaneModule //
//////////////////////////

sbLibraryServicePane.prototype.servicePaneInit = 
function sbLibraryServicePane_servicePaneInit(sps) {
  //logcall(arguments);

  // keep track of the service pane service
  this._servicePane = sps;

  // Before initializing, hide all existing library nodes.
  // This way we only show things when we are notified
  // that they actually exist.
  this._hideLibraryNodes(this._servicePane.root);

  // register for notification that the library manager is initialized
  var obs = Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService);
  obs.addObserver(this, "songbird-library-manager-ready", false);
  obs.addObserver(this, "songbird-library-manager-before-shutdown", false);

  // get the library manager
  this._initLibraryManager();
}

sbLibraryServicePane.prototype.shutdown = 
function sbLibraryServicePane_shutdown() {
  this._removeAllLibraries();
}

sbLibraryServicePane.prototype.fillContextMenu =
function sbLibraryServicePane_fillContextMenu(aNode, aContextMenu, aParentWindow) {

  // the playlists folder and the local library node get the "New Foo..." items
  if (aNode.id == 'SB:Playlists' || 
      aNode.getAttributeNS(LSP, 'ListCustomType') == 'local') {
    this.fillNewItemMenu(aNode, aContextMenu, aParentWindow);
  }

  var list = this.getLibraryResourceForNode(aNode);
  if (list) {
    this._appendCommands(aContextMenu, list, aParentWindow);

    // Add menu items for a smart media list
    if (list instanceof Ci.sbILocalDatabaseSmartMediaList) {
      this._appendMenuItem(aContextMenu, "Properties", function(event) { //XXX todo: localize
        var watcher = Cc["@mozilla.org/embedcomp/window-watcher;1"]
                        .getService(Ci.nsIWindowWatcher);
        watcher.openWindow(null,
                          "chrome://songbird/content/xul/smartPlaylist.xul",
                          "_blank",
                          "chrome,dialog=yes",
                          list);
      });
    }

    // Add menu items for a dynamic media list
    if (list.getProperty("http://songbirdnest.com/data/1.0#isSubscription") == "1") {
      this._appendMenuItem(aContextMenu, "Properties", function(event) { //XXX todo: localize

        var params = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
        params.appendElement(list, false);

        var watcher = Cc["@mozilla.org/embedcomp/window-watcher;1"]
                        .getService(Ci.nsIWindowWatcher);
        watcher.openWindow(null,
                           "chrome://songbird/content/xul/subscribe.xul",
                           "_blank",
                           "chrome,dialog=yes",
                           params);
      });
      this._appendMenuItem(aContextMenu, "Update", function(event) { //XXX todo: localize
        var dps = Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/DynamicPlaylistService;1"]
                    .getService(Ci.sbILocalDatabaseDynamicPlaylistService);
        dps.updateNow(list);
      });
    }
  }
}

sbLibraryServicePane.prototype.fillNewItemMenu =
function sbLibraryServicePane_fillNewItemMenu(aNode, aContextMenu, aParentWindow) {
  var sbSvc = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService);
  var stringBundle = sbSvc.createBundle("chrome://songbird/locale/songbird.properties");
  
  function add(id, label, accesskey, oncommand) {
    var menuitem = aContextMenu.ownerDocument.createElement('menuitem');
    menuitem.setAttribute('id', id);
    menuitem.setAttribute('class', 'menuitem-iconic');
    menuitem.setAttribute('label', stringBundle.GetStringFromName(label));
    menuitem.setAttribute('accesskey', stringBundle.GetStringFromName(accesskey));
    menuitem.setAttribute('oncommand', oncommand);
    aContextMenu.appendChild(menuitem);
  }

  add('menuitem_file_new', 'menu.file.new', 'menu.file.new.accesskey', 'doMenu("menuitem_file_new")');
  //XXX Mook: disabled; see bug 4001
  //add('file.smart', 'menu.file.smart', 'menu.file.smart.accesskey', 'doMenu("file.smart")');
  add('menuitem_file_remote', 'menu.file.remote', 'menu.file.remote.accesskey', 'doMenu("menuitem_file_remote")');
}

sbLibraryServicePane.prototype.onSelectionChanged =
function sbLibraryServicePane_onSelectionChanged(aNode, aContainer, aParentWindow) {
  this._destroyShortcuts(aContainer, aParentWindow);
  var list;
  if (aNode) list = this.getLibraryResourceForNode(aNode);
  if (list) this._createShortcuts(list, aContainer, aParentWindow);
}

sbLibraryServicePane.prototype._createShortcuts =
function sbLibraryServicePane__createShortcuts(aList, aContainer, aWindow) {
  var shortcuts = aWindow.document.createElement("sb-commands-shortcuts");
  shortcuts.setAttribute("id", "playlist-commands-shortcuts");
  shortcuts.setAttribute("commandtype", "medialist");
  shortcuts.setAttribute("bind", aList.library.guid + ';' + aList.guid);
  aContainer.appendChild(shortcuts);
  this._lastShortcuts = shortcuts;
}

sbLibraryServicePane.prototype._destroyShortcuts =
function sbLibraryServicePane__destroyShortcuts(aContainer, aWindow) {
  if (this._lastShortcuts) {
    this._lastShortcuts.destroy();
    aContainer.removeChild(this._lastShortcuts);
    this._lastShortcuts = null;
  }
}

sbLibraryServicePane.prototype._getMediaListForDrop =
function sbLibraryServicePane__getMediaListForDrop(aNode, aDragSession, aOrientation) {
  dump('_getMediaListForDrop('+aNode+', '+aDragSession+', '+aOrientation+')\n');
  // work out what the drop would target and return an sbIMediaList to
  // represent that target, or null if the drop is not allowed
  
  // work out what's being dropped
  var dropList = aDragSession.isDataFlavorSupported(TYPE_X_SB_TRANSFER_MEDIA_LIST);
  
  // if it's not a list and not items, then we don't know or care what happens
  // next
  if (!dropList &&
      !aDragSession.isDataFlavorSupported(TYPE_X_SB_TRANSFER_MEDIA_ITEM) &&
      !aDragSession.isDataFlavorSupported(TYPE_X_SB_TRANSFER_MEDIA_ITEMS)) {
    return null;
  }
  
  dump('dropList='+dropList+'\n');
  
  // work out where its going
  var container = aNode;
  var dropOnto = true;
  if (aOrientation != 0) {
    // if we're dropping before/after we're dropping into the parent
    container = aNode.parentNode;
    dropOnto = false;
  }
  
  dump('container='+container+' ('+container.id+')\n');
  dump('dropOnto='+dropOnto+'\n');
  
  // work out what library resource is associated with the container node
  var containerResource = this.getLibraryResourceForNode(container);
  
  dump('containerResource='+containerResource+'\n');
  
  // if there's no associated library then the target is assumed to be the
  // default library
  if (containerResource == null) {
    containerResource = this._libraryManager.mainLibrary;
  }
  
  // is the target a library
  var isLibrary = (containerResource instanceof Ci.sbILibrary);
  
  dump('isLibrary='+isLibrary+'\n');
  
  // The Rules:
  //  * (non-playlist) items can be dropped on top of playlists or libraries
  //  * playlists can be dropped next to other playlists in the same container

  if (!dropList && dropOnto) {
    return containerResource;
  }
  if (dropList && isLibrary) {
    var draggedItem = this._getDndData(aDragSession,
                                       TYPE_X_SB_TRANSFER_MEDIA_LIST,
                                       Ci.sbISingleListTransferContext);
    if (!draggedItem) {
      return null;
    }
    var draggedContainer = this.getNodeForLibraryResource(draggedItem.list);
    if (draggedContainer == container) {
      return containerResource;
    }
  }
  
  return null;
}

sbLibraryServicePane.prototype.canDrop =
function sbLibraryServicePane_canDrop(aNode, aDragSession, aOrientation) {
  dump('\n\n\ncanDrop:\n');
  
  var list = this._getMediaListForDrop(aNode, aDragSession, aOrientation);
  
  if (list) {
    dump('I CAN HAS DORP\n');
    // XXX Mook: hack for bug 4760 to do special handling for the download
    // playlist.  This will need to be expanded later to use IDLs on the
    // list so that things like extensions can do this too.
    var customType = list.getProperty(SBProperties.customType);
    if (customType == "download") {
      var IOS = Cc["@mozilla.org/network/io-service;1"]
                  .getService(Ci.nsIIOService);

      function canDownload(aMediaItem) {
        var contentSpec = aMediaItem.getProperty(SBProperties.contentURL);
        var contentURL = IOS.newURI(contentSpec, null, null);
        switch(contentURL.scheme) {
          case "http":
          case "https":
          case "ftp":
            // these are safe to download
            dump(<>[{contentURL.spec} accept]&#10;</>);
            return true;
        }
        dump(<>[{contentURL.spec} DENY]&#10;</>);
        return false;
      }

      if (aDragSession.isDataFlavorSupported(TYPE_X_SB_TRANSFER_MEDIA_ITEMS)) {
        var context = this._getDndData(aDragSession,
                                       TYPE_X_SB_TRANSFER_MEDIA_ITEMS,
                                       Ci.sbIMultipleItemTransferContext);
        var items = context.items;
        // we must remember to reset the context before we exit, so that when we
        // actually need the items in onDrop we can get them again!
        var count = 0;
        while (items.hasMoreElements()) {
          var item = items.getNext();
          item.QueryInterface(Ci.sbIIndexedMediaItem);
          if (!canDownload(item.mediaItem)) {
            context.reset();
            return false;
          }
          ++count;
        }
        if (count < 1) {
          // umm, where's that list of items?
          context.reset();
          return false;
        }
        // all items in the list are downloadable
        context.reset();
        return true;
      } else if (aDragSession.isDataFlavorSupported(TYPE_X_SB_TRANSFER_MEDIA_ITEM)) {
        var context = this._getDndData(aDragSession,
                                       TYPE_X_SB_TRANSFER_MEDIA_ITEM,
                                       Ci.sbISingleItemTransferContext);
        return canDownload(context.item);
      } else {
        // huh? _getMediaListForDrop should have said no
        return false;
      }
    }
    return true;
  } else {
    dump('NO DROP FOR YOU!\n');
    return false;
  }
}
sbLibraryServicePane.prototype._getDndData =
function sbLibraryServicePane__getDndData(aDragSession, aDataType, aInterface) {
  // create an nsITransferable
  var transferable = Components.classes["@mozilla.org/widget/transferable;1"].
      createInstance(Components.interfaces.nsITransferable);
  // specify what kind of data we want it to contain
  transferable.addDataFlavor(aDataType);
  // ask the drag session to fill the transferable with that data
  aDragSession.getData(transferable, 0);
  // get the data from the transferable
  var data = {};
  var dataLength = {};
  transferable.getTransferData(aDataType, data, dataLength);
  // it's always a string. always.
  data = data.value.QueryInterface(Components.interfaces.nsISupportsString);
  data = data.toString();
  
  // get the object from the dnd source tracker
  var dnd = Components.classes["@songbirdnest.com/Songbird/DndSourceTracker;1"]
      .getService(Components.interfaces.sbIDndSourceTracker);
  return dnd.getSource(data).QueryInterface(aInterface);
}

sbLibraryServicePane.prototype.onDrop =
function sbLibraryServicePane_onDrop(aNode, aDragSession, aOrientation) {
  dump('\n\n\nonDrop:\n');
  
  // where are we dropping?
  var targetList = this._getMediaListForDrop(aNode, aDragSession, aOrientation);
  if (!targetList) {
    // don't know how to drop here
    return;
  }
  
  var targetListIsLibrary = (targetList instanceof Ci.sbILibrary);
  
  var metrics = Components.classes["@songbirdnest.com/Songbird/Metrics;1"]
                    .createInstance(Components.interfaces.sbIMetrics);
  
  var targettype = targetList.getProperty(SBProperties.customType);
  var totype = targetList.library.getProperty(SBProperties.customType);

  if (aDragSession.isDataFlavorSupported(TYPE_X_SB_TRANSFER_MEDIA_LIST)) {
    dump('media list dropped\n');
    
    var context = this._getDndData(aDragSession,
        TYPE_X_SB_TRANSFER_MEDIA_LIST, Ci.sbISingleListTransferContext);
    var list = context.list;
    
    // Metrics!
    var fromtype = list.library.getProperty(SBProperties.customType);
    metrics.metricsAdd("app.servicepane.copy", fromtype, totype, list.length);
    
    if (targetListIsLibrary) {
      // want to copy the list and the contents
      if (targetList == list.library) {
        dump('this list is already in this library\n');
        return;
      }
      
      // create a copy of the list
      var newlist = targetList.copyMediaList('simple', list);
      
      // find the node that was created
      var newnode = this._servicePane.getNode(this._itemURN(newlist));
      if (aOrientation < 0) {
        aNode.parentNode.insertBefore(newnode, aNode);
      } else if (aNode.nextSibling) {
        aNode.parentNode.insertBefore(newnode, aNode.nextSibling);
      } else {
        aNode.parentNode.appendChild(newnode);
      }
      
      // FIXME: handle the creation of the node in the pane correctly
    } else {
      if (context.list == targetList) {
        // uh oh - you can't drop a list onto itself
        dump("you can't drop a list onto itself\n");
        return;
      }
      // just add the contents
      targetList.addAll(list);
    }
    dump('added\n');
  } else if (aDragSession.isDataFlavorSupported(TYPE_X_SB_TRANSFER_MEDIA_ITEMS)) {
    dump('media items dropped\n');
    
    var context = this._getDndData(aDragSession,
        TYPE_X_SB_TRANSFER_MEDIA_ITEMS, Ci.sbIMultipleItemTransferContext);
    
    var items = context.items;

    // Metrics!
    var fromtype = context.source.library.getProperty("http://songbirdnest.com/data/1.0#customType");
    //Components.utils.reportError(fromtype + " - " + totype );
    metrics.metricsAdd("app.servicepane.copy", fromtype, totype, context.count);
    
    var itemEnumerator = {
      hasMoreElements: function() { return items.hasMoreElements(); },
      getNext: function() {
        var item = items.getNext().QueryInterface(Ci.sbIIndexedMediaItem);
        return item.mediaItem;
      },
      QueryInterface: function(iid) {
        if (iid.equals(Ci.nsISupports) ||
            iid.equals(Ci.nsISimpleEnumerator)) {
          return this;
        }
        throw Cr.NS_ERROR_NO_INTERFACE;
      }
    };

    if (targettype == "download") {
      // XXX Mook: Special handling for dragging to download list, bug 4760
      Cc["@songbirdnest.com/Songbird/DownloadDeviceHelper;1"]
        .getService(Ci.sbIDownloadDeviceHelper)
        .downloadSome(itemEnumerator);
    } else {
      // add all the items to the target list
      targetList.beginUpdateBatch();
      targetList.addSome(itemEnumerator);
      targetList.endUpdateBatch();
    }
    
    dump('added\n');

  } else if (aDragSession.isDataFlavorSupported(TYPE_X_SB_TRANSFER_MEDIA_ITEM)) {
    dump('media item dropped\n');

    var context = this._getDndData(aDragSession,
        TYPE_X_SB_TRANSFER_MEDIA_ITEM, Ci.sbISingleItemTransferContext);
    
    // add the item
    if (targettype == "download") {
      // XXX Mook: Special handling for dragging to download list, bug 4760
      Cc["@songbirdnest.com/Songbird/DownloadDeviceHelper;1"]
        .getService(Ci.sbIDownloadDeviceHelper)
        .downloadItem(context.item);
    } else {
      targetList.add(context.item);
    }

    // Metrics!
    var fromtype = context.source.library.getProperty(SBProperties.customType);
    metrics.metricsAdd("app.servicepane.copy", fromtype, totype, 1);
    
    dump('added\n');
  }
}
sbLibraryServicePane.prototype._nodeIsLibrary =
function sbLibraryServicePane__nodeIsLibrary(aNode) {
  return aNode.getAttributeNS(LSP, "LibraryGUID") ==
      aNode.getAttributeNS(LSP, "ListGUID");
}
sbLibraryServicePane.prototype.onDragGesture =
function sbLibraryServicePane_onDragGesture(aNode, aTransferable) {
  if (this._nodeIsLibrary(aNode)) {
    // a library isn't dragable
    return false;
  }
  
  // get the list and create the source context
  var list = this._getItemForURN(aNode.id);
  var context = {
    source: list.library,
    count: 1,
    list: list,
    QueryInterface: function(iid) {
      if (iid.equals(Components.interfaces.sbISingleListTransferContext) ||
          iid.equals(Components.interfaces.nsISupports)) {
        return this;
      }
      throw Components.results.NS_NOINTERFACE;
    }
  }

  // register the source context
  var dnd = Components.classes['@songbirdnest.com/Songbird/DndSourceTracker;1']
      .getService(Components.interfaces.sbIDndSourceTracker);
  dnd.reset();  
  var handle = dnd.registerSource(context);
  
  // attach the source context to the transferable
  aTransferable.addDataFlavor(TYPE_X_SB_TRANSFER_MEDIA_LIST);
  var text = Components.classes["@mozilla.org/supports-string;1"].
     createInstance(Components.interfaces.nsISupportsString);
  text.data = handle;
  aTransferable.setTransferData(TYPE_X_SB_TRANSFER_MEDIA_LIST, text,
                                text.data.length*2);
  
  return true;
}


/**
 * Called when the user has attempted to rename a library/medialist node
 */
sbLibraryServicePane.prototype.onRename =
function sbLibraryServicePane_onRename(aNode, aNewName) {
  //logcall(arguments);
  if (aNode && aNewName) {
    var libraryResource = this.getLibraryResourceForNode(aNode);
    libraryResource.name = aNewName;
  }
}


//////////////////////////////////
// sbILibraryServicePaneService //
//////////////////////////////////


/* \brief Suggest a library for creating a new media list
 *
 * \param aMediaListType string identifying a media list type, eg "simple"
 * \param aNode A service pane node to provide context for new list creation
 * \return a library, or null if this service can't suggest anything based on 
 *         the given context and type.
 */
sbLibraryServicePane.prototype.suggestLibraryForNewList =
function sbLibraryServicePane_suggestLibraryForNewList(aMediaListType, aNode) {
  //logcall(arguments);
  
  // Must provide a media list type
  if (!aMediaListType) {
    throw Components.results.NS_ERROR_INVALID_ARG;
  }
  
  // Make sure we are fully initialized
  if (!this._libraryManager || !this._servicePane) {
    throw Components.results.NS_ERROR_NOT_INITIALIZED;
  }
  
  // Move up the tree looking for libraries that support the
  // given media list type.
  while (aNode && aNode != this._servicePane.root) {
  
    // If this node is visible and belongs to the library 
    // service pane service...
    if (aNode.contractid == CONTRACTID && !aNode.hidden) {
      // If this is a playlist and the playlist belongs
      // to a library that supports the given type, 
      // then suggest that library
      var mediaItem = this._getItemForURN(aNode.id);
      if (mediaItem && mediaItem instanceof Ci.sbIMediaList &&
          this._doesLibrarySupportListType(mediaItem.library, aMediaListType))
      {
        return mediaItem.library;
      }
      
      // If this is a library that supports the given type, 
      // then suggest the library
      var library = this._getLibraryForURN(aNode.id);
      if (library && library instanceof Ci.sbILibrary &&
          this._doesLibrarySupportListType(library, aMediaListType)) 
      {
        return library;
      }
    }

    // Move up the tree
    aNode = aNode.parentNode;
  } // end of while

  // If the main library supports the given type, then return that
  if (this._doesLibrarySupportListType(this._libraryManager.mainLibrary, 
                                       aMediaListType)) 
  {
    return this._libraryManager.mainLibrary;
  }
  
  // Oh well, out of luck
  return null;
}


/* \brief Attempt to get a service pane node for the given library resource
 *         
 * \param aResource an sbIMediaItem, sbIMediaItem, or sbILibrary
 * \return a service pane node that represents the given resource, if one exists
 */
sbLibraryServicePane.prototype.getNodeForLibraryResource =
function sbLibraryServicePane_getNodeForLibraryResource(aResource) {
  //logcall(arguments);
  
  // Must be initialized
  if (!this._libraryManager || !this._servicePane) {
    throw Components.results.NS_ERROR_NOT_INITIALIZED;
  }  
  
  var node;
  
  // If this is a library, make a library node
  if (aResource instanceof Ci.sbILibrary) {
    node = this._servicePane.getNode(this._libraryURN(aResource));
  
  // If this is a mediaitem, make a mediaitem node
  } else if (aResource instanceof Ci.sbIMediaItem) {
    node = this._servicePane.getNode(this._itemURN(aResource));
  
  // Else we don't know what to do, so 
  // the arg must be invalid
  } else {
    throw Components.results.NS_ERROR_INVALID_ARG;
  }
  
  return node;  
}


/* \brief Attempt to get a library resource for the given service pane node.
 *
 * Note that there is no guarantee that hidden service pane nodes
 * will have corresponding library resources
 *
 * \param aNode 
 * \return a sbIMediaItem, sbIMediaItem, sbILibrary, or null
 */
sbLibraryServicePane.prototype.getLibraryResourceForNode =
function sbLibraryServicePane_getLibraryResourceForNode(aNode) {
  //logcall(arguments);
  
  // Must provide a node
  if (!(aNode instanceof Ci.sbIServicePaneNode)) {
    throw Components.results.NS_ERROR_INVALID_ARG;
  }  
  // Must be initialized
  if (!this._libraryManager || !this._servicePane) {
    throw Components.results.NS_ERROR_NOT_INITIALIZED;
  }
  
  // If the node does not belong to us, then we aren't
  // going to find a resource
  if (aNode.contractid != CONTRACTID) {
    return null;
  }
  
  // Attempt to get a resource from the id of the given node
  var resource = this._getItemForURN(aNode.id);
  if (!resource) {
    resource = this._getLibraryForURN(aNode.id);
  }
  
  return resource;
}


/////////////////////
// Private Methods //
/////////////////////


/**
 * Return true if the given library supports the given list type
 */
sbLibraryServicePane.prototype._doesLibrarySupportListType =
function sbLibraryServicePane__doesLibrarySupportListType(aLibrary, aListType) {
  //logcall(arguments);
  var types = aLibrary.mediaListTypes;
  while (types.hasMore()) {
    if(aListType == types.getNext())  {
      return true;
    }
  }
  return false;
}


/**
 * Hide this node and any nodes below it that belong to
 * the library service pane service
 */
sbLibraryServicePane.prototype._hideLibraryNodes =
function sbLibraryServicePane__hideLibraryNodes(aNode) {
  //logcall(arguments);
  
  // If this is one of our nodes, hide it
  if (aNode.contractid == CONTRACTID) {
    aNode.hidden = true;
  }

  // If this is a container, descend into all children  
  if (aNode.isContainer) {
    var children = aNode.childNodes;
    while (children.hasMoreElements()) {
      var child = children.getNext().QueryInterface(Ci.sbIServicePaneNode);
      this._hideLibraryNodes(child);
    }
  }
}


/**
 * Add all registered libraries to the service pane
 */
sbLibraryServicePane.prototype._addAllLibraries =
function sbLibraryServicePane__addAllLibraries() {
  //logcall(arguments);
  var libraries = this._libraryManager.getLibraries();
  while (libraries.hasMoreElements()) {
    var library = libraries.getNext();
    this._libraryAdded(library);
  }
}

/**
 * Remove all registered libraries from the service pane
 */
sbLibraryServicePane.prototype._removeAllLibraries =
function sbLibraryServicePane__removeAllLibraries() {
  //logcall(arguments);
  var libraries = this._libraryManager.getLibraries();
  while (libraries.hasMoreElements()) {
    var library = libraries.getNext();
    this._libraryRemoved(library);
  }
}

/**
* Add all media lists found in the given library
 */
sbLibraryServicePane.prototype._processListsInLibrary =
function sbLibraryServicePane__processListsInLibrary(aLibrary) {
  //logcall(arguments);

  // Listener to receive enumerated items and store then in an array
  var listener = {
    items: [],
    onEnumerationBegin: function() { return true; },
    onEnumerationEnd: function() {return true; },
    onEnumeratedItem: function(list, item) {
      this.items.push(item);
      return true;
    }
  };

  // Enumerate all lists in this library
  aLibrary.enumerateItemsByProperty(PROP_ISLIST, "1",
                                   listener,
                                   Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

  // Make sure we have a node for each list
  for (var i = 0; i < listener.items.length; i++) {
    this._ensureMediaListNodeExists(listener.items[i]);  
  }
}


/**
 * The given library has been added.  Show it in the service pane.
 */
sbLibraryServicePane.prototype._libraryAdded =
function sbLibraryServicePane__libraryAdded(aLibrary) {
  //logcall(arguments);  
  this._ensureLibraryNodeExists(aLibrary);

  // Listen to changes in the library so that we can display new playlists
  var filter = SBProperties.createArray([[SBProperties.mediaListName, null]]);
  aLibrary.addListener(this,
                       false,
                       Ci.sbIMediaList.LISTENER_FLAGS_ALL &
                         ~Ci.sbIMediaList.LISTENER_FLAGS_AFTERITEMREMOVED,
                       filter);

  this._processListsInLibrary(aLibrary);
}


/**
 * The given library has been removed.  Just hide the contents
 * rather than deleting so that if it is ever reattached
 * we will remember any ordering (drag-drop) information
 */
sbLibraryServicePane.prototype._libraryRemoved =
function sbLibraryServicePane__libraryRemoved(aLibrary) {
  //logcall(arguments);
  
  // Find the node for this library
  var id = this._libraryURN(aLibrary);
  var node = this._servicePane.getNode(id);
  
  // Hide this node and everything below it
  if (node) {
    this._hideLibraryNodes(node);
  }

  aLibrary.removeListener(this);
}

sbLibraryServicePane.prototype._refreshLibraryNodes =
function sbLibraryServicePane__refreshLibraryNodes(aLibrary) {
  var id = this._libraryURN(aLibrary);
  var node = this._servicePane.getNode(id);
  this._hideLibraryNodes(node);
  this._ensureLibraryNodeExists(aLibrary);
  this._processListsInLibrary(aLibrary);
}

/** 
 * The given media list has been added. Show it in the service pane.
 */
sbLibraryServicePane.prototype._playlistAdded =
function sbLibraryServicePane__playlistAdded(aMediaList) {
  //logcall(arguments);
  this._ensureMediaListNodeExists(aMediaList);
}


/** 
 * The given media list has been removed. Delete the node, as it 
 * is unlikely that the same playlist will come back again.
 */
sbLibraryServicePane.prototype._playlistRemoved =
function sbLibraryServicePane__playlistRemoved(aMediaList) {
  //logcall(arguments);
  
  var id = this._itemURN(aMediaList);
  var node = this._servicePane.getNode(id);
  if (node) {
    this._servicePane.removeNode(node);
  }
}


/** 
 * The given media list has been updated. 
 * The name and other properties may have changed. 
 */
sbLibraryServicePane.prototype._mediaListUpdated =
function sbLibraryServicePane__mediaListUpdated(aMediaList) {
  //logcall(arguments);
  if (aMediaList instanceof Ci.sbILibrary) {
    this._ensureLibraryNodeExists(aMediaList);
  } else if (aMediaList instanceof Ci.sbIMediaList) {
    this._ensureMediaListNodeExists(aMediaList);
  }
}


/**
 * Get a service pane identifier for the given media item
 */
sbLibraryServicePane.prototype._itemURN =
function sbLibraryServicePane__itemURN(aMediaItem) {
  return URN_PREFIX_ITEM + aMediaItem.guid;
}


/**
 * Get a service pane identifier for the given library
 */
sbLibraryServicePane.prototype._libraryURN =
function sbLibraryServicePane__libraryURN(aLibrary) {
  return URN_PREFIX_LIBRARY + aLibrary.guid;
}


/**
 * Given a resource id, attempt to extract the GUID of a media item.
 */
sbLibraryServicePane.prototype._getItemGUIDForURN =
function sbLibraryServicePane__getItemGUIDForURN(aID) {
  //logcall(arguments);
  var index = aID.indexOf(URN_PREFIX_ITEM);
  if (index >= 0) {
    return aID.slice(URN_PREFIX_ITEM.length);
  }
  return null;
}


/**
 * Given a resource id, attempt to extract the GUID of a library.
 */
sbLibraryServicePane.prototype._getLibraryGUIDForURN =
function sbLibraryServicePane__getLibraryGUIDForURN(aID) {
  //logcall(arguments);
  var index = aID.indexOf(URN_PREFIX_LIBRARY);
  if (index >= 0) {
    return aID.slice(URN_PREFIX_LIBRARY.length);
  }
  return null;
}


/**
 * Given a resource id, attempt to get an sbIMediaItem.
 * This is the inverse of _itemURN
 */
sbLibraryServicePane.prototype._getItemForURN =
function sbLibraryServicePane__getItemForURN(aID) {
  //logcall(arguments);
  var guid = this._getItemGUIDForURN(aID);
  if (guid) {
    var node = this._servicePane.getNode(aID);
    var libraryGUID = node.getAttributeNS(LSP, "LibraryGUID");      
    if (libraryGUID) {
      try {
        var library = this._libraryManager.getLibrary(libraryGUID);
        return library.getMediaItem(guid);
      } catch (e) {
        dump("sbLibraryServicePane__getItemForURN: error trying to get medialist " +
             guid + " from library " + libraryGUID + "\n");
      }
    }
    
    // URNs of visible nodes in the servicetree should always refer
    // to an existing media item...
    dump("sbLibraryServicePane__getItemForURN: could not find a mediaItem " +
         "for URN " + aID + ". The service pane must be out of sync with " + 
         "the libraries!\n");
  }
  return null;
}


/**
 * Given a resource id, attempt to get an sbILibrary.
 * This is the inverse of _libraryURN
 */
sbLibraryServicePane.prototype._getLibraryForURN =
function sbLibraryServicePane__getLibraryForURN(aID) {
  //logcall(arguments);
  var guid = this._getLibraryGUIDForURN(aID);
  if (guid) {
    return this._libraryManager.getLibrary(guid);
  }
  return null;
}


/**
 * Make a URL for the given library resource.  
 * Loading this URL should display the resource in a playlist.
 */
sbLibraryServicePane.prototype._getDisplayURL =
function sbLibraryServicePane__getDisplayURL(aResource) {
  //logcall(arguments);
  
  // Should really ask someone else... but for now just hardcode
  var url = URL_PLAYLIST_DISPLAY;
  if (aResource instanceof Ci.sbILibrary) {
    url += "library,"
  }
  url += aResource.guid;
  if (aResource instanceof Ci.sbIMediaList && 
      !(aResource instanceof Ci.sbILibrary))
  {
    url += "," + aResource.library.guid;
  }
  return url;
}



/**
 * Get the service pane node for the given library,
 * creating one if none exists.
 */
sbLibraryServicePane.prototype._ensureLibraryNodeExists =
function sbLibraryServicePane__ensureLibraryNodeExists(aLibrary) {
  //logcall(arguments);

  // Get the Node.
  var id = this._libraryURN(aLibrary);
  var node = this._servicePane.getNode(id);
  var newnode = false;
  if (!node) {
    // Create the node
    node = this._servicePane.addNode(id, this._servicePane.root, true);
    newnode = true;
  }
  
  var customType = aLibrary.getProperty(SBProperties.customType);
  
  // Refresh the information just in case it is supposed to change
  node.name = aLibrary.name;  
  node.url = this._getDisplayURL(aLibrary);
  node.contractid = CONTRACTID;
  node.editable = false;
  node.hidden = aLibrary.getProperty(SBProperties.hidden) == "1";

  if (aLibrary == this._libraryManager.mainLibrary) {
    // the main library uses a separate Playlists folder
    this._ensurePlaylistFolderExists();
    // Set the weight of the main library
    node.setAttributeNS(SP, 'Weight', -4);
  } if (customType == 'web') {
    // Set the weight of the web library
    node.setAttributeNS(SP, 'Weight', 5);
  } else {
    // other libraries store the playlists under them
    node.dndAcceptIn = 'text/x-sb-playlist-'+aLibrary.guid;
  }
  // Set properties for styling purposes
  this._mergeProperties(node,
                        ["library",
                         "libraryguid-" + aLibrary.guid,
                         aLibrary.type,
                         customType]);
  // Save the type of media list so that we can group by type
  node.setAttributeNS(LSP, "ListType", aLibrary.type)
  // Save the guid of the library
  node.setAttributeNS(LSP, "LibraryGUID", aLibrary.guid);
  // and save it as the list guid
  node.setAttributeNS(LSP, "ListGUID", aLibrary.guid);
  // Save the customType for use by metrics.
  node.setAttributeNS(LSP, "ListCustomType", customType);
  // Save the customType for use by metrics.
  node.setAttributeNS(LSP, "LibraryCustomType", customType);

  if (newnode) {
    // Position the node in the tree
    this._insertLibraryNode(node, aLibrary);
  }
    
  return node;
}

 
/**
 * Get the service pane node for the given media list,
 * creating one if none exists.
 */ 
sbLibraryServicePane.prototype._ensureMediaListNodeExists =
function sbLibraryServicePane__ensureMediaListNodeExists(aMediaList) {
  //logcall(arguments);

  var id = this._itemURN(aMediaList);
  var node = this._servicePane.getNode(id);
  var newnode = false;
  if (!node) {
    // Create the node
    // NOTE: it's a container for drag and drop purposes only.
    node = this._servicePane.addNode(id, this._servicePane.root, true);
    newnode = true;
  }
  
  var customType = aMediaList.getProperty(SBProperties.customType);
  var libCustomType = aMediaList.library.getProperty(SBProperties.customType);
  
  // Refresh the information just in case it is supposed to change
  node.name = aMediaList.name;  
  node.url = this._getDisplayURL(aMediaList);
  node.contractid = CONTRACTID;
  if (customType == 'download') {
    // the download media list isn't editable
    node.editable = false;
    // set the weight of the downloads list
    node.setAttributeNS(SP, 'Weight', -3);
  } else {
    // the rest are
    node.editable = true;
  }
  // Set properties for styling purposes
  if (aMediaList.getProperty("http://songbirdnest.com/data/1.0#isSubscription") == "1") {
    this._mergeProperties(node, ["medialist", "medialisttype-dynamic"]);
  } else {
    this._mergeProperties(node, ["medialist medialisttype-" + aMediaList.type]);
  }
  // Add the customType to the properties to encourage people to set it for CSS
  this._mergeProperties(node, [customType]);
  // Save the type of media list so that we can group by type
  node.setAttributeNS(LSP, "ListType", aMediaList.type);
  // Save the guid of the library that owns this media list
  node.setAttributeNS(LSP, "LibraryGUID", aMediaList.library.guid);
  // and the guid of this list
  node.setAttributeNS(LSP, "ListGUID", aMediaList.guid);
  // Save the parent library custom type for this list.
  node.setAttributeNS(LSP, "LibraryCustomType", libCustomType);
  // Save the list customType for use by metrics.
  node.setAttributeNS(LSP, "ListCustomType", customType);

  if (aMediaList.library == this._libraryManager.mainLibrary) {
    // a playlist in the main library is considered a toplevel node
    if (customType == 'download') {
      // unless its the download playlist
      node.dndDragTypes = '';
      node.dndAcceptNear = '';
    } else {
      node.dndDragTypes = 'text/x-sb-playlist';
      node.dndAcceptNear = 'text/x-sb-playlist';
    }
  } else {
    // playlists in other libraries can only go into their libraries' nodes
    node.dndDragTypes = 'text/x-sb-playlist-'+aMediaList.library.guid;
    node.dndAcceptNear = 'text/x-sb-playlist-'+aMediaList.library.guid;
  }


  if (newnode) {
    // Place the node in the tree
    this._insertMediaListNode(node, aMediaList);
  }
  
  
  // Get hidden state from list, since we hide all list nodes on startup
  node.hidden = aMediaList.getProperty(PROP_ISHIDDEN) == "1";
      
  return node;
}

/**
 * Get the service pane node for the Playlists folder (which contains all
 * the playlists in the main library).
 */
sbLibraryServicePane.prototype._ensurePlaylistFolderExists =
function sbLibraryServicePane__ensurePlaylistFolderExists() {
  var fnode = this._servicePane.getNode('SB:Playlists');
  if (!fnode) {
    // make sure it exists
    var fnode = this._servicePane.addNode('SB:Playlists', 
        this._servicePane.root, true);
  }
  fnode.name = '&servicesource.playlists';
  this._mergeProperties(fnode, ["folder", "Playlists"]);
  fnode.hidden = false;
  fnode.contractid = CONTRACTID;
  fnode.dndAcceptIn = 'text/x-sb-playlist';
  fnode.editable = false;
  fnode.setAttributeNS(SP, 'Weight', 3);
  return fnode;
}


/**
 * Logic to determine where a library node should appear
 * in the service pane tree
 */ 
sbLibraryServicePane.prototype._insertLibraryNode =
function sbLibraryServicePane__insertLibraryNode(aNode, aLibrary) {
  //logcall(arguments);
  
  // If this is the main library it should go at
  // the top of the list
  if (aLibrary == this._libraryManager.mainLibrary) {
    if (this._servicePane.root.firstChild && 
        this._servicePane.root.firstChild.id != aNode.id) 
    {
      this._servicePane.root.insertBefore(aNode, 
              this._servicePane.root.firstChild);    
    }
  // Otherwise, add above the first non-library item?
  } else {
    this._insertAfterLastOfSameType(aNode, this._servicePane.root);
  }
}


/**
 * Logic to determine where a media list node should appear
 * in the service pane tree
 */ 
sbLibraryServicePane.prototype._insertMediaListNode =
function sbLibraryServicePane__insertMediaListNode(aNode, aMediaList) {
  //logcall(arguments);

  // If it is a main library media list, it belongs in the "Playlists" folder
  if (aMediaList.library == this._libraryManager.mainLibrary) 
  {
    // unless it's the download playlist
    if (aNode.getAttributeNS(LSP, 'ListCustomType') == 'download') {
      // FIXME: put it right after the library
      this._servicePane.root.appendChild(aNode);
    } else {
      var folder = this._ensurePlaylistFolderExists();
      folder.appendChild(aNode);
    }
  } 
  // If it is a secondary library playlist, it should be
  // added as a child of that library
  else 
  {
    // Find the parent libary in the tree
    var parentLibraryGUID = aMediaList.library.guid;
    var parentLibraryNode;
    var children = this._servicePane.root.childNodes;
    while (children.hasMoreElements()) {
      var child = children.getNext().QueryInterface(Ci.sbIServicePaneNode);
      if (child.contractid == CONTRACTID && 
          this._getLibraryGUIDForURN(child.id) == parentLibraryGUID)
      {
        parentLibraryNode = child;
        break;
      }
    }
    
    // Insert after last of same type as child of
    // this library
    if (parentLibraryNode && parentLibraryNode.isContainer) {
      this._insertAfterLastOfSameType(aNode, parentLibraryNode);
    } else {
      dump("sbLibraryServicePane__insertMediaListNode: ");
      dump("could not add media list to parent library ");
      if (parentLibraryNode)
          dump(parentLibraryNode.name + "\n");
      else
          dump("\n");
      this._servicePane.root.appendChild(aNode);
    }
  }
}


 
/**
 * Inserts the given node under the given parent and attempts to keep
 * children grouped by type.  The node is inserted at the end of the list
 * or next to the last child of the same type as the given node.
 */ 
sbLibraryServicePane.prototype._insertAfterLastOfSameType =
function sbLibraryServicePane__insertAfterLastOfSameType(aNode, aParent) {
  //logcall(arguments);
    
  if (!aParent.isContainer) {
    dump("sbLibraryServicePane__insertAfterLastOfSameType: ");
    dump("cannot insert under non-container node.\n");
    return;
  }      
  aParent.isOpen = true;
  
  // Insert after last of same type before hitting 
  // non-library related items
  var children = aParent.childNodes;
  var nodeType = aNode.getAttributeNS(LSP, "ListType");
  var lastOfSameType;
  var inserted = false;
  while (children.hasMoreElements()) {
    var child = children.getNext().QueryInterface(Ci.sbIServicePaneNode);
    
    // If this node belongs to the library service pane, and 
    // is not the node we are trying to insert, then check
    // to see if this is an insertion point candidate
    if (child.contractid == CONTRACTID && child.id != aNode.id) {
    
      // Keep track of last node similar to this one, preferring
      // nodes that are exactly the same type as this one
      if (nodeType == child.getAttributeNS(LSP, "ListType") ||
          lastOfSameType == null ||
          (lastOfSameType != null && 
             lastOfSameType.getAttributeNS(LSP, "ListType") != nodeType))
      {
        lastOfSameType = child;
      } 
    }     
  } // end of while
  
  // Insert the new list after the last of the same type
  // or at the end of the list
  if (lastOfSameType && lastOfSameType.nextSibling) {
    if (lastOfSameType.nextSibling.id != aNode.id) {
      aParent.insertBefore(aNode, lastOfSameType.nextSibling);
    }
  } else {
    aParent.appendChild(aNode);
  }
}

sbLibraryServicePane.prototype._appendMenuItem =
function sbLibraryServicePane__appendMenuItem(aContextMenu, aLabel, aCallback) {
  var item = aContextMenu.ownerDocument.createElement("menuitem");
  item.setAttribute("label", aLabel);
  item.addEventListener("command", aCallback, false);
  aContextMenu.appendChild(item);
}

sbLibraryServicePane.prototype._appendCommands =
function sbLibraryServicePane__appendCommands(aContextMenu, aList, aParentWindow) {
  if (this._lastMenuitems && this._lastMenuitems.destroy) {
    var pnode = this._lastMenuitems.parentNode;
    this._lastMenuitems.destroy();
    this._lastMenuitems = null;
  }
  var itemBuilder = aContextMenu.ownerDocument.createElement("sb-commands-menuitems");
  itemBuilder.setAttribute("id", "playlist-commands");
  itemBuilder.setAttribute("commandtype", "medialist");
  itemBuilder.setAttribute("bind", aList.library.guid + ';' + aList.guid);
  aContextMenu.appendChild(itemBuilder);
  this._lastMenuitems = itemBuilder;
}

/**
 * This function is a recursive helper for onListCleared (below) that will
 * remove all the playlist nodes for a given library.
 */
sbLibraryServicePane.prototype._removeListNodesForLibrary =
function sbLibraryServicePane__removeListNodesForLibrary(aStartNode, aLibraryGUID) {

  var node = aStartNode.firstChild;
  
  while (node) {
  
    if (node.isContainer) {
      this._removeListNodesForLibrary(node, aLibraryGUID);
    }
    
    var nextSibling = node.nextSibling;
    
    if (this._getItemGUIDForURN(node.id)) {
      var nodeLibraryGUID = node.getAttributeNS(LSP, "LibraryGUID");
      if (nodeLibraryGUID == aLibraryGUID) {
        this._servicePane.removeNode(node);
      }
    }
    
    node = nextSibling;
  }
}

sbLibraryServicePane.prototype._mergeProperties =
function sbLibraryServicePane__mergeProperties(aNode, aList) {

  var o = {};
  var a = aNode.properties?aNode.properties.split(" "):[];
  a.forEach(function(prop) {
    o[prop] = 1;
  });

  aList.forEach(function(prop) {
    o[prop] = 1;
  });

  var retval = [];
  for (var prop in o) {
    retval.push(prop);
  }
  aNode.properties = retval.join(" ");
}

///////////////////////////////
// sbILibraryManagerListener //
///////////////////////////////

sbLibraryServicePane.prototype.onLibraryRegistered =
function sbLibraryServicePane_onLibraryRegistered(aLibrary) {
  //logcall(arguments);
  this._libraryAdded(aLibrary);
}
sbLibraryServicePane.prototype.onLibraryUnregistered =
function sbLibraryServicePane_onLibraryUnregistered(aLibrary) {
  //logcall(arguments);
  this._libraryRemoved(aLibrary);
}

//////////////////////////
// sbIMediaListListener //
//////////////////////////

sbLibraryServicePane.prototype.onItemAdded =
function sbLibraryServicePane_onItemAdded(aMediaList, aMediaItem) {
  //logcall(arguments);
  if (this._batch.isActive()) {
    // We are going to refresh all the nodes once we exit the batch so
    // we don't need any more of these notifications
    this._refreshPending = true;
    return true;
  }
  else {
    var isList = aMediaItem instanceof Ci.sbIMediaList;
    if (isList) {
      this._playlistAdded(aMediaItem);
    }
    return false;
  }
}
sbLibraryServicePane.prototype.onBeforeItemRemoved =
function sbLibraryServicePane_onBeforeItemRemoved(aMediaList, aMediaItem) {
  //logcall(arguments);
  if (this._batch.isActive()) {
    // We are going to refresh all the nodes once we exit the batch so
    // we don't need any more of these notifications
    this._refreshPending = true;
    return true;
  }
  else {
    var isList = aMediaItem instanceof Ci.sbIMediaList;
    if (isList) {
      this._playlistRemoved(aMediaItem);
    }
    return false;
  }
}
sbLibraryServicePane.prototype.onAfterItemRemoved =
function sbLibraryServicePane_onAfterItemRemoved(aMediaList, aMediaItem) {
}
sbLibraryServicePane.prototype.onItemUpdated =
function sbLibraryServicePane_onItemUpdated(aMediaList,
                                            aMediaItem,
                                            aProperties) {
  if (this._batch.isActive()) {
    // We are going to refresh all the nodes once we exit the batch so
    // we don't need any more of these notifications
    this._refreshPending = true;
    return true;
  }
  else {
    var isList = aMediaItem instanceof Ci.sbIMediaList;
    if (isList) {
      this._mediaListUpdated(aMediaItem);
    }
    return false;
  }
}
sbLibraryServicePane.prototype.onListCleared =
function sbLibraryServicePane_onListCleared(aMediaList) {
  if (this._batch.isActive()) {
    // We are going to refresh all the nodes once we exit the batch so
    // we don't need any more of these notifications
    this._refreshPending = true;
    return true;
  }
  else {
    if (aMediaList instanceof Ci.sbILibrary) {
      var libraryGUID = aMediaList.guid;
      
      var node = this._servicePane.root;
      this._removeListNodesForLibrary(node, libraryGUID);
    }
    return false;
  }
}
sbLibraryServicePane.prototype.onBatchBegin =
function sbLibraryServicePane_onBatchBegin(aMediaList) {
  //logcall(arguments);
  this._batch.begin();
}
sbLibraryServicePane.prototype.onBatchEnd =
function sbLibraryServicePane_onBatchEnd(aMediaList) {
  //logcall(arguments);
  this._batch.end();
  if (!this._batch.isActive() && this._refreshPending) {
    this._refreshLibraryNodes(aMediaList);
  }

}


sbLibraryServicePane.prototype._initLibraryManager =
function sbLibraryServicePane__initLibraryManager() {
  // get the library manager
  this._libraryManager = Cc['@songbirdnest.com/Songbird/library/Manager;1']
                           .getService(Ci.sbILibraryManager);

  // register for notifications so that we can keep the service pane
  // in sync with the the libraries
  this._libraryManager.addListener(this);

  this._addAllLibraries();
}
/////////////////
// nsIObserver //
/////////////////

sbLibraryServicePane.prototype.observe = 
function sbLibraryServicePane_observe(subject, topic, data) {

  var obs = Cc["@mozilla.org/observer-service;1"]
              .getService(Ci.nsIObserverService);

  if (topic == "songbird-library-manager-ready") {
    obs.removeObserver(this, "songbird-library-manager-ready");

    if (!this._libraryManager) {
      this._initLibraryManager();
    }
  }
  else if (topic == "songbird-library-manager-before-shutdown") {
    obs.removeObserver(this, "songbird-library-manager-before-shutdown");

    var libraryManager = Cc['@songbirdnest.com/Songbird/library/Manager;1']
                           .getService(Ci.sbILibraryManager);
    libraryManager.removeListener(this);
  }
}

///////////
// XPCOM //
///////////

/**
 * /brief XPCOM initialization code
 */
function makeGetModule(CONSTRUCTOR, CID, CLASSNAME, CONTRACTID, CATEGORIES) {
  return function (comMgr, fileSpec) {
    return {
      registerSelf : function (compMgr, fileSpec, location, type) {
        compMgr.QueryInterface(Ci.nsIComponentRegistrar);
        compMgr.registerFactoryLocation(CID,
                        CLASSNAME,
                        CONTRACTID,
                        fileSpec,
                        location,
                        type);
        if (CATEGORIES && CATEGORIES.length) {
          var catman =  Cc["@mozilla.org/categorymanager;1"]
              .getService(Ci.nsICategoryManager);
          for (var i=0; i<CATEGORIES.length; i++) {
            var e = CATEGORIES[i];
            catman.addCategoryEntry(e.category, e.entry, e.value, 
              true, true);
          }
        }
      },

      getClassObject : function (compMgr, cid, iid) {
        if (!cid.equals(CID)) {
          throw Cr.NS_ERROR_NO_INTERFACE;
        }

        if (!iid.equals(Ci.nsIFactory)) {
          throw Cr.NS_ERROR_NOT_IMPLEMENTED;
        }

        return this._factory;
      },

      _factory : {
        createInstance : function (outer, iid) {
          if (outer != null) {
            throw Cr.NS_ERROR_NO_AGGREGATION;
          }
          return (new CONSTRUCTOR()).QueryInterface(iid);
        }
      },

      unregisterSelf : function (compMgr, location, type) {
        compMgr.QueryInterface(Ci.nsIComponentRegistrar);
        compMgr.unregisterFactoryLocation(CID, location);
        if (CATEGORIES && CATEGORIES.length) {
          var catman =  Cc["@mozilla.org/categorymanager;1"]
              .getService(Ci.nsICategoryManager);
          for (var i=0; i<CATEGORIES.length; i++) {
            var e = CATEGORIES[i];
            catman.deleteCategoryEntry(e.category, e.entry, true);
          }
        }
      },

      canUnload : function (compMgr) {
        return true;
      },

      QueryInterface : function (iid) {
        if ( !iid.equals(Ci.nsIModule) ||
             !iid.equals(Ci.nsISupports) )
          throw Cr.NS_ERROR_NO_INTERFACE;
        return this;
      }

    };
  }
}

var NSGetModule = makeGetModule (
  sbLibraryServicePane,
  Components.ID("{64ec2154-3733-4862-af3f-9f2335b14821}"),
  "Songbird Library Service Pane Service",
  CONTRACTID,
  [{
    category: 'service-pane',
    entry: '0library', // we want this to load first
    value: CONTRACTID
  }]);


