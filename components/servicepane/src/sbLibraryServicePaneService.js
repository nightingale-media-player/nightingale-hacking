/** vim: ts=2 sw=2 expandtab
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

/**
 * \file sbLibraryServicePane.js
 */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/DropHelper.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");
Components.utils.import("resource://app/jsmodules/WindowUtils.jsm");

const CONTRACTID = "@songbirdnest.com/servicepane/library;1";
const ROOTNODE = "SB:Bookmarks";

const URN_PREFIX_ITEM = 'urn:item:';
const URN_PREFIX_LIBRARY = 'urn:library:';

const LSP = 'http://songbirdnest.com/rdf/library-servicepane#';
const SP='http://songbirdnest.com/rdf/servicepane#';


const TYPE_X_SB_TRANSFER_MEDIA_ITEM = "application/x-sb-transfer-media-item";
const TYPE_X_SB_TRANSFER_MEDIA_LIST = "application/x-sb-transfer-media-list";
const TYPE_X_SB_TRANSFER_MEDIA_ITEMS = "application/x-sb-transfer-media-items";
const TYPE_X_SB_TRANSFER_DISABLE_DOWNLOAD = "application/x-sb-transfer-disable-download";

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

const DEBUG_MODE = false;

function LOG(s) {
  if(DEBUG_MODE) {
    dump(s + "\n");
  }
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

  this._batch = {}; // we'll keep the batch counters in here
  this._refreshPending = false;
}

//////////////////////////
// nsISupports / XPCOM  //
//////////////////////////

sbLibraryServicePane.prototype.QueryInterface =
  XPCOMUtils.generateQI([Ci.nsIObserver,
                         Ci.sbIServicePaneModule,
                         Ci.sbILibraryServicePaneService,
                         Ci.sbILibraryManagerListener,
                         Ci.sbIMediaListListener]);

sbLibraryServicePane.prototype.classID =
  Components.ID("{64ec2154-3733-4862-af3f-9f2335b14821}");
sbLibraryServicePane.prototype.classDescription =
  "Songbird Library Service Pane Service";
sbLibraryServicePane.prototype.contractID =
  CONTRACTID;
sbLibraryServicePane.prototype._xpcom_categories = [{
  category: 'service-pane',
  entry: '0library', // we want this to load first
  value: CONTRACTID
}];
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
      if (list.userEditable) {
        this._appendMenuItem(aContextMenu, SBString("command.smartpl.properties"), function(event) {
          var watcher = Cc["@mozilla.org/embedcomp/window-watcher;1"]
                          .getService(Ci.nsIWindowWatcher);
          watcher.openWindow(aParentWindow,
                            "chrome://songbird/content/xul/smartPlaylist.xul",
                            "_blank",
                            "chrome,dialog=yes,centerscreen,modal,titlebar=no",
                            list);
        });
      }
    }
    // Append the media export context menuitem only if the user has exporting turned on.
    var appPrefs = Cc["@mozilla.org/fuel/application;1"]
                     .getService(Ci.fuelIApplication).prefs;
    if (appPrefs.getValue("songbird.library_exporter.export_tracks", false)) {
      // Add media export hook if this is the main library
      var libraryMgr = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                         .getService(Ci.sbILibraryManager);
      if (list == libraryMgr.mainLibrary) {
        var exportService = Cc["@songbirdnest.com/media-export-service;1"]
                              .getService(Ci.sbIMediaExportService);

        var item = aContextMenu.ownerDocument.createElement("menuitem");
        item.setAttribute("label", SBString("command.libraryexport"));
        item.addEventListener(
          "command",
          function(event) {
            var exportService = Cc["@songbirdnest.com/media-export-service;1"]
                                  .getService(Ci.sbIMediaExportService);
            exportService.exportSongbirdData(); 
          },
          false);

        // Disable the item if it doesn't have pending changes.
        if (!exportService.hasPendingChanges) {
          item.setAttribute("disabled", "true");
        }

        aContextMenu.appendChild(item);
      }
    }

    // Add menu items for a dynamic media list
    if (list.getProperty("http://songbirdnest.com/data/1.0#isSubscription") == "1") {
      this._appendMenuItem(aContextMenu, SBString("command.subscription.properties"), function(event) {
        var params = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"].createInstance(Ci.nsIMutableArray);
        params.appendElement(list, false);

        WindowUtils.openModalDialog(null,
                                    "chrome://songbird/content/xul/subscribe.xul",
                                    "",
                                    "chrome,modal=yes,centerscreen",
                                    params,
                                    null);
      });
      this._appendMenuItem(aContextMenu, "Update", function(event) { //XXX todo: localize
        var dps = Cc["@songbirdnest.com/Songbird/Library/DynamicPlaylistService;1"]
                    .getService(Ci.sbIDynamicPlaylistService);
        dps.updateNow(list);
      });
    }
  }
}

sbLibraryServicePane.prototype.fillNewItemMenu =
function sbLibraryServicePane_fillNewItemMenu(aNode, aContextMenu, aParentWindow) {
  var sbSvc = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService);
  var stringBundle = sbSvc.createBundle("chrome://songbird/locale/songbird.properties");

  function add(id, label, accesskey, oncommand, modifiers) {
    var menuitem = aContextMenu.ownerDocument.createElement('menuitem');
    menuitem.setAttribute('id', id);
    menuitem.setAttribute('class', 'menuitem-iconic');
    menuitem.setAttribute('label', stringBundle.GetStringFromName(label));
    menuitem.setAttribute('accesskey', stringBundle.GetStringFromName(accesskey));
    menuitem.setAttribute('oncommand', oncommand);
    if (typeof(modifiers) != "undefined") {
      menuitem.setAttribute('modifiers', modifiers);
    }
    aContextMenu.appendChild(menuitem);
  }

  add('menuitem_file_new',
      'menu.servicepane.file.new',
      'menu.servicepane.file.new.accesskey',
      'doMenu("menuitem_file_new", event)');
  add('file.smart',
      'menu.servicepane.file.smart',
      'menu.servicepane.file.smart.accesskey',
      'doMenu("menuitem_file_smart")',
      "alt");
  add('menuitem_file_remote',
      'menu.servicepane.file.remote',
      'menu.servicepane.file.remote.accesskey',
      'doMenu("menuitem_file_remote")');
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

sbLibraryServicePane.prototype._canDownloadDrop =
function sbLibraryServicePane__canDownloadDrop(aDragSession) {
  // bail out if the drag session does not contain internal items.
  // We may eventually add a handler to add an external media URL drop on 
  // the download playlist.
  if (!InternalDropHandler.isSupported(aDragSession)) 
    return false;
   
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
        return true;
    }
    return false;
  }

  if (aDragSession.isDataFlavorSupported(TYPE_X_SB_TRANSFER_MEDIA_ITEMS)) {
    var context = DNDUtils.
      getInternalTransferDataForFlavour(aDragSession,
                                        TYPE_X_SB_TRANSFER_MEDIA_ITEMS,
                                        Ci.sbIMediaItemsTransferContext);
    var items = context.items;
    // we must remember to reset the context before we exit, so that when we
    // actually need the items in onDrop we can get them again!
    var count = 0;    
    var downloadable = true;
    while (items.hasMoreElements() && downloadable) {
      downloadable = canDownload(items.getNext());
      ++count;
    }
    
    // we can't download nothing.
    if (count == 0) { downloadable = false; }
    
    // rewind the items list.
    context.reset();
    
    return downloadable;
  } else {
    Components.utils.reportError("_getMediaListForDrop should have returned null");
    return false;
  }
}

sbLibraryServicePane.prototype._getMediaListForDrop =
function sbLibraryServicePane__getMediaListForDrop(aNode, aDragSession, aOrientation) {
  // work out what the drop would target and return an sbIMediaList to
  // represent that target, or null if the drop is not allowed

  // check if we support this drop at all
  if (!InternalDropHandler.isSupported(aDragSession)&& 
      !ExternalDropHandler.isSupported(aDragSession)) {
    return null;
  }

  // are we dropping a list ?
  var dropList = aDragSession.isDataFlavorSupported(TYPE_X_SB_TRANSFER_MEDIA_LIST);

  // work out where the drop items are going
  var targetNode = aNode;
  var dropOnto = true;
  if (aOrientation != 0) {
    dropOnto = false;
  }

  // work out what library resource is associated with the target node 
  var targetResource = this.getLibraryResourceForNode(targetNode);

  // check that the target list or library accepts drops
  // TODO, i can't believe we don't have a property to check for that :/
  // I'm told that the transfer policy will solve this, this might be a good
  // spot for querying it in the future... (?)

  // is the target a library
  var targetIsLibrary = (targetResource instanceof Ci.sbILibrary);

  // The Rules:
  //  * non-playlist items can be dropped on top of playlists and libraries
  //  * playlist items can be dropped on top of other libraries than their own
  //  * playlists can be dropped next to other playlists, either as part of a
  //      reordering, or as a playlist transfer to a different library, with a 
  //      specific position to drop to

  if (dropOnto) {
    if (!dropList) {
      // we are dropping onto a target node, and this is not a playlist, 
      // so return the target medialist that corresponds to the target node. 
      return targetResource;
    } else {
      // we are dropping a list onto a target node, we can only accept this drop
      // on a library, so if the target isnt one, refuse it
      // note: if we ever want to support dropping playlists on playlists, this
      // is the sport to add the new case
      if (!targetIsLibrary) {
        return null;
      }
        
      // we can only accept a list drop on a library different than its own 
      // (because its own library already has all of its tracks, so it makes no
      // sense to allow it), so extract the medialist that we are dropping, 
      // and check where it comes from
      var draggedList = DNDUtils.
        getInternalTransferDataForFlavour(aDragSession,
                                          TYPE_X_SB_TRANSFER_MEDIA_LIST,
                                          Ci.sbIMediaListTransferContext);
      if (targetResource == draggedList.list.library) {
        return null;
      }
        
      // we are indeed dropping the playlist onto a different library, accept
      // the drop
      return targetResource;
    }

  } else {

    // we are dropping in between two items
    if (dropList) {
      
      // we are dropping a playlist.
      
      // if we are trying to insert the playlist between two toplevel nodes,
      // refuse the drop
      if (targetNode.parentNode.id == "SB:Root") {
        return null;
      }
      
      // we now know that this is either a reorder inside a single container 
      // or a playlist transfer from one library to another, involving two
      // different containers. 
      
      // to be able to discriminate between those two cases, we need to know
      // where the playlist is going, as well as where it comes from. 
      
      // find where the playlist comes from
      var draggedList = DNDUtils.
        getInternalTransferDataForFlavour(aDragSession,
                                          TYPE_X_SB_TRANSFER_MEDIA_LIST,
                                          Ci.sbIMediaListTransferContext);
      var fromResource = draggedList.library;
      
      var toResource = null;

      // finding where the playlist is going however is much more complicated,
      // because we cannot rely on the fact that we can extract a library
      // resource from the parent container for the target list: we could be
      // dropping into a foreign container (eg. a device node), and the actual
      // library node could be a sibling of the target node.
      
      // so we'll first try to get a resource from the parent node, in case we
      // are dropping into a list of playlist that are children of their
      // library
      var parentNode = targetNode.parentNode;
      if (parentNode) {
        toResource = this.getLibraryResourceForNode(parentNode);
      }
      
      // ... and if there was no parent for the target node, or the parent had 
      // no library resource, we can still look around the drop target for 
      // libraries and playlists nodes. if we find any of these, we can assume 
      // that the resource targeted for drop is these item's library
      
      if (!toResource) {
        var siblingNode = targetNode.previousSibling;
        if (siblingNode) {
          toResource = this.getLibraryResourceForNode(siblingNode);
        }
      }
      if (!toResource) {
        var siblingNode = targetNode.nextSibling;
        if (siblingNode) {
          toResource = this.getLibraryResourceForNode(siblingNode);
        }
      }
        
      // if we have not found what the destination library is, refuse the drop
      if (!toResource) 
        return null;
        
      // get the library for the resource we found
      toResource = toResource.library;
        
      // if the source and destination library are the same, this is a reorder,
      // we can actually pretend to refuse it, because the servicePaneService
      // is going to accept it based on further rules (dndAcceptIn, dndAcceptNear)
      // and because there is no actual drop handling to be performed, the nodes
      // will be reordered in the tree and that is it.
      if (toResource == fromResource) 
        return null;
      
      // otherwise, this is a playlist transfer from one library to another,
      // we should accept the drop, but at the condition that we are not
      // dropping above a library, because we want to keep playlists either 
      // below or as children of their library nodes. 
      if (targetIsLibrary && aOrientation == 1) 
        return null;
      
      // the destination seems correct, accept the drop, the handler will
      // first copy the list to the new library, and then move the node
      // to where it belongs  
      return toResource;
    }
  }
  
  // default is refuse the drop
  return null;
}

sbLibraryServicePane.prototype.canDrop =
function sbLibraryServicePane_canDrop(aNode, aDragSession, aOrientation, aWindow) {
  // don't allow drop on read-only nodes
  if (aNode.getAttributeNS(LSP, "ReadOnly") == "true")
    return false;

  // if some of the items have disable download set then don't allow a drop 
  // on the service pane
  if (aDragSession.isDataFlavorSupported(TYPE_X_SB_TRANSFER_DISABLE_DOWNLOAD)) {
    return false;
  }

  var list = this._getMediaListForDrop(aNode, aDragSession, aOrientation);
  if (list) {
    
    // check if the list is in a readonly library
    if (!list.library.userEditable ||
        !list.library.userEditableContent) {
      // this is a list for a readonly library, can't drop
      return false;
    }
    
    // check if the list is itself readonly
    if (!list.userEditable ||
        !list.userEditableContent) {
      // this list content is readonly, can't drop
      return false;
    }
    
    // XXX Mook: hack for bug 4760 to do special handling for the download
    // playlist.  This will need to be expanded later to use IDLs on the
    // list so that things like extensions can do this too.
    var customType = list.getProperty(SBProperties.customType);
    if (customType == "download") {
      return this._canDownloadDrop(aDragSession);
    }
    // test whether the drop contains supported flavours
    return InternalDropHandler.isSupported(aDragSession) ||
           ExternalDropHandler.isSupported(aDragSession);
  } else {
    return false;
  }
}

sbLibraryServicePane.prototype.onDrop =
function sbLibraryServicePane_onDrop(aNode, aDragSession, aOrientation, aWindow) {
  // don't allow drop on read-only nodes
  if (aNode.getAttributeNS(LSP, "ReadOnly") == "true")
    return;

  // where are we dropping?
  var targetList = this._getMediaListForDrop(aNode, aDragSession, aOrientation);

  if (!targetList) {
    // don't know how to drop here
    return;
  }
  
  // perform this test now because the incoming new node makes it unreliable
  // to do in the onCopyMediaList callback
  var isLastSibling = (aNode.nextSibling == null);
  
  var dropHandlerListener = {
    libSPS: this,
    onDropComplete: function(aTargetList,
                             aImportedInLibrary,
                             aDuplicates,
                             aInsertedInMediaList,
                             aOtherDropsHandled) { 
      // show the standard report on the status bar
      return true; 
    },
    onFirstMediaItem: function(aTargetList, aFirstMediaItem) {},
    onCopyMediaList: function(aSourceList, aNewList) {
      // find the node that was created
      var newnode = 
        this.libSPS._servicePane.getNode(this.libSPS._itemURN(aNewList));
      // move the item to the right spot
      switch (aOrientation) {
        case -1:
          aNode.parentNode.insertBefore(newnode, aNode);
          break;
        case 1:
          if (!isLastSibling)
            aNode.parentNode.insertBefore(newnode, aNode.nextSibling);
          // else, the node has already been placed at the right spot by
          // the LibraryServicePaneService
          break;
      }
    }
  };

  if (InternalDropHandler.isSupported(aDragSession)) {
    // handle drop of internal items
    InternalDropHandler.dropOnList(aWindow, 
                                   aDragSession, 
                                   targetList, 
                                   -1, 
                                   dropHandlerListener);

  } else if (ExternalDropHandler.isSupported(aDragSession)) {
  
    // handle drop of external items
    ExternalDropHandler.dropOnList(aWindow, 
                                   aDragSession, 
                                   targetList, 
                                   -1, 
                                   dropHandlerListener);
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
    QueryInterface: XPCOMUtils.generateQI([Ci.sbIMediaListTransferContext])
  };
  
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

  // if no node was provided, then suggest the main library
  if (!aNode) 
    return this._libraryManager.mainLibrary;
  
  function checkNode(aNode, aLibServicePane) {
    // If this node is visible and belongs to the library
    // service pane service...
    if (aNode.contractid == CONTRACTID && !aNode.hidden) {
      // If this is a playlist and the playlist belongs
      // to a library that supports the given type,
      // then suggest that library
      var mediaItem = aLibServicePane._getItemForURN(aNode.id);
      if (mediaItem && mediaItem instanceof Ci.sbIMediaList &&
          aLibServicePane._doesLibrarySupportListType(mediaItem.library, aMediaListType))
      {
        return mediaItem.library;
      }

      // If this is a library that supports the given type,
      // then suggest the library
      var library = aLibServicePane._getLibraryForURN(aNode.id);
      if (library && library instanceof Ci.sbILibrary &&
          aLibServicePane._doesLibrarySupportListType(library, aMediaListType))
      {
        return library;
      }
    }

    return null;
  }
  
  // first, check if the given node is useable as a library...
  var lib = checkNode(aNode, this);
  if (lib)
    return lib;

  if (aNode.isContainer) {
    // check the children of the node (but not recursively) for a usable library
    for (var child = aNode.firstChild; child; child = child.nextSibling) {
      lib = checkNode(child, this);
      if (lib)
        return lib;
    }
  }
  
  // Move up the tree looking for libraries that support the
  // given media list type.
  aNode = aNode.parentNode;
  while (aNode && aNode != this._servicePane.root) {

    lib = checkNode(aNode, this);
    if (lib)
      return lib;

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

sbLibraryServicePane.prototype.createNodeForLibrary =
function sbLibraryServicePane_createNodeForLibrary(aLibrary) {
  if(aLibrary instanceof Ci.sbILibrary) {
    return this._libraryAdded(aLibrary);
  }

  return null;
}

/**
 * \brief Get the URN for a resource, to be used to look up the service pane node
 * \param aResource to resource to get the service pane URN for
 * \returns The expected URN for the resource
 */
sbLibraryServicePane.prototype._getURNForLibraryResource =
function sbLibraryServicePane_getURNForLibraryResource(aResource) {
  //logcall(arguments);

  // Must be initialized
  if (!this._libraryManager || !this._servicePane) {
    throw Components.results.NS_ERROR_NOT_INITIALIZED;
  }

  // If this is a library, get the library URN
  if (aResource instanceof Ci.sbILibrary) {
    return this._libraryURN(aResource);

  // If this is a mediaitem, get an item urn
  } else if (aResource instanceof Ci.sbIMediaItem) {
    // Check if this is a storage list for an outer list
    var outerListGuid = aResource.getProperty(SBProperties.outerGUID);
    if (outerListGuid) {
      var library = aResource.library;
      var outerList = library.getMediaItem(outerListGuid);
      if (outerList) {
        aResource = outerList;
      }
    }
    // cacluate the URN for the item
    return this._itemURN(aResource);

  // Else we don't know what to do, so
  // the arg must be invalid
  } else {
    throw Components.results.NS_ERROR_INVALID_ARG;
  }

  return node;
}

/* \brief Attempt to get a service pane node for the given library resource
 *
 * \param aResource an sbIMediaItem, sbIMediaItem, or sbILibrary
 * \return a service pane node that represents the given resource, if one
 *         exists. Note that in the case that more than one node related to a
 *         given library exists, it is not specified which node will be
 *         returned. However, nodes that are not hidden will be preferred.
 */
sbLibraryServicePane.prototype.getNodeForLibraryResource =
function sbLibraryServicePane_getNodeForLibraryResource(aResource) {
  //logcall(arguments);

  var urn = this._getURNForLibraryResource(aResource);
  var node = this._servicePane.getNode(urn);
  var hiddenNode = null;
  if (node && node.hasAttributeNS(LSP, "LibraryGUID")) {
    // this is a library node (and not a dummy playlist container)
    if (!node.hidden) {
      return node;
    }
    // this is a hidden node; not as good, but might be useful as a last-ditch
    hiddenNode = node;
  }
  for each (let type in ["audio", "video", "podcast"]) {
    let constrainedURN = urn + ":constraint(" + type + ")";
    node = this._servicePane.getNode(urn);
    if (!node.hidden) {
      return node;
    }
    // this node is hidden; remember it if nothing better comes along
    hiddenNode = hiddenNode || node;
  }
  return hiddenNode;
}

/**
 * \brief Attempt to get a service pane node for the given media list view
 *
 * \param aMediaListView the view for which to get the service pane node
 * \returns a service pane node that represents the given media list view
 */
sbLibraryServicePane.prototype.getNodeFromMediaListView =
function sbLibraryServicePane_getNodeFromMediaListView(aMediaListView) {
  //logcall(arguments);

  // get the base URN...
  var baseURN = this._getURNForLibraryResource(aMediaListView.mediaList);

  if ((aMediaListView instanceof Ci.sbIFilterableMediaListView) &&
      aMediaListView.filterConstraint)
  {
    // try to add constraints
    var urn = baseURN;

    // stash off the properties involved in the standard constraints,
    // so that we don't look at them
    var standardConstraintProperties = {};
    const standardConstraint = LibraryUtils.standardFilterConstraint
    for (let group in ArrayConverter.JSEnum(standardConstraint.groups)) {
      group.QueryInterface(Ci.sbILibraryConstraintGroup);
      for (let prop in ArrayConverter.JSEnum(group.properties)) {
        standardConstraintProperties[prop] = true;
      }
    }

    for (let group in ArrayConverter.JSEnum(aMediaListView.filterConstraint.groups)) {
      group.QueryInterface(Ci.sbILibraryConstraintGroup);
      for (let prop in ArrayConverter.JSEnum(group.properties)) {
        if (prop in standardConstraintProperties) {
          continue;
        }
        for (let value in ArrayConverter.JSEnum(group.getValues(prop))) {
          urn += ":constraint(" + value + ")";
        }
      }
    }

    let node = this._servicePane.getNode(urn);
    if (node) {
      return node;
    }
  }

  // lookup by constraints failed, use the base URN
  return this._servicePane.getNode(baseURN);
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


/* \brief Set node read-only property.
 *
 * \param aNode Node to set.
 * \param aReadOnly If true, node is read-only.
 */
sbLibraryServicePane.prototype.setNodeReadOnly =
function sbLibraryServicePane_setNodeReadOnly(aNode, aReadOnly) {
  if (aReadOnly) {
    aNode.editable = false;
    aNode.setAttributeNS(LSP, "ReadOnly", "true");
  } else {
    aNode.editable = true;
    aNode.setAttributeNS(LSP, "ReadOnly", "false");
  }
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

  // If the transfer policy indicates read only media lists, the library does
  // not support adding media lists of any type
  // XXXerik less than SUPER HACK to keep new playlists from being added to
  // device libraries.  This uses a hacked up policy system that will be
  // replaced by a real one.
  var transferPolicy = aLibrary.getProperty(SBProperties.transferPolicy);
  if (transferPolicy && transferPolicy.match(/readOnlyMediaLists/)) {
    return false;
  }

  // XXXben SUPER HACK to keep new playlists from being added to the web
  //        library. We should really fix this with our policy system.
  if (aLibrary.equals(LibraryUtils.webLibrary)) {
    return false;
  }

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
    onEnumerationBegin: function() { },
    onEnumerationEnd: function() { },
    onEnumeratedItem: function(list, item) {
      this.items.push(item);
    }
  };

  // Enumerate all lists in this library
  aLibrary.enumerateItemsByProperty(SBProperties.isList, "1",
                                    listener );

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
  var node = this._ensureLibraryNodeExists(aLibrary, true);

  // Listen to changes in the library so that we can display new playlists
  var filter = SBProperties.createArray([[SBProperties.hidden, null],
                                         [SBProperties.mediaListName, null]]);
  aLibrary.addListener(this,
                       false,
                       Ci.sbIMediaList.LISTENER_FLAGS_ALL &
                         ~Ci.sbIMediaList.LISTENER_FLAGS_AFTERITEMREMOVED,
                       filter);

  this._processListsInLibrary(aLibrary);

  return node;
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
  this._scanForRemovedItems(aLibrary);
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
  if (aID.substring(0, URN_PREFIX_LIBRARY.length) != URN_PREFIX_LIBRARY) {
    return null;
  }
  var id = aID.slice(URN_PREFIX_LIBRARY.length);
  id = id.replace(/:constraint\(.*?\)/g, '');
  return id;
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
        LOG("sbLibraryServicePane__getItemForURN: error trying to get medialist " +
             guid + " from library " + libraryGUID);
      }
    }

    // URNs of visible nodes in the servicetree should always refer
    // to an existing media item...
    LOG("sbLibraryServicePane__getItemForURN: could not find a mediaItem " +
         "for URN " + aID + ". The service pane must be out of sync with " +
         "the libraries!");
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
 * Ensure the library nodes for the given library exists, then return the
 * container node
 * @param aMove true to force move the nodes into the container if they
 *              already exist elsewhere
 */
sbLibraryServicePane.prototype._ensureLibraryNodeExists =
function sbLibraryServicePane__ensureLibraryNodeExists(aLibrary, aMove) {
  //logcall(arguments);
  var self = this;

  /**
   * Find or create a library node for the given library, for the given
   * constraint type
   * @param aLibrary the library to find / create a node for
   * @param aConstraintType the type of library node, e.g. "audio", "video",
   *                        "podcast" - if not given, no constraint applies
   * @returns the node (old or new) for the library of the given type
   */
  function makeNodeFromLibrary(aLibrary, aConstraintType, aParentNode) {
    const K_DEFAULT_WEIGHTS = {
      // weights to use by default, for a given constraint type
      "audio": 100,
      "video": 200
    };

    var id = self._libraryURN(aLibrary);
    if (aConstraintType) {
      // add a constraint only if the constraint type was specified
      id += ":constraint(" + aConstraintType + ")";
    }
    var node = self._servicePane.getNode(id);
    var newnode = false;
    if (!node) {
      // Create the node
      node = self._servicePane.addNode(id, aParentNode, true);
      newnode = true;
    }
    var customType = aLibrary.getProperty(SBProperties.customType);

    // Refresh the information just in case it is supposed to change
    // Don't set name if it hasn't changed to avoid a UI redraw
    if (node.name != aLibrary.name) {
      node.name = aLibrary.name;
    }
    node.url = null;
    node.contractid = CONTRACTID;
    node.editable = false;
    node.hidden = (aLibrary.getProperty(SBProperties.hidden) == "1");

    // Set properties for styling purposes
    self._mergeProperties(node,
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

    if (aConstraintType) {
        self._mergeProperties(node, [aConstraintType]);
        var builder = Cc["@songbirdnest.com/Songbird/Library/ConstraintBuilder;1"]
                        .createInstance(Ci.sbILibraryConstraintBuilder);
        builder.includeConstraint(LibraryUtils.standardFilterConstraint);
        builder.intersect();
        builder.include(SBProperties.contentType, aConstraintType);
        node.setAttributeNS(SP,
                            "mediaListViewConstraints",
                            builder.get());
        if (aConstraintType in K_DEFAULT_WEIGHTS) {
          node.setAttributeNS(SP,
                              "Weight",
                              K_DEFAULT_WEIGHTS[aConstraintType]);
        }
    }

    if (newnode) {
      // Position the node in the tree
      self._insertNodeAfter(aParentNode, node);
    }
    return node;
  }

  var customType = aLibrary.getProperty(SBProperties.customType);

  if (customType == 'web') {
    // the web library has no video/audio split, nor a parent, so we need to
    // special case it here and return early
    let node = makeNodeFromLibrary(aLibrary, null, self._servicePane.root);

    // Set the weight of the web library
    node.setAttributeNS(SP, 'Weight', 5);
    node.hidden = true;
    return node;
  }

  // make the parent node
  var id = self._libraryURN(aLibrary);
  var parentNode = self._servicePane.getNode(id);
  var newNode = false;
  if (!parentNode) {
    // Create the node
    parentNode = self._servicePane.addNode(id, self._servicePane.root, true);
    newNode = true;
  }
  // Refresh the information just in case it is supposed to change
  // Don't set name if it hasn't changed to avoid a UI redraw
  if (parentNode.name != aLibrary.name) {
    parentNode.name = aLibrary.name;
  }

  // uncomment this to cause clicks on the container node to load an unfiltered
  // library.  we need to think more about the ramifications for UE before
  // turning this on, so it's off for now.
  // (see also below on the migration part)
  //parentNode.contractid = CONTRACTID;

  // properties that should exist on the parent container node
  const K_PARENT_PROPS = ["folder", "library-container"];

  parentNode.url = null;
  parentNode.editable = false;
  parentNode.setAttributeNS(SP, 'Weight', -4);
  self._mergeProperties(parentNode, K_PARENT_PROPS);
  if (newNode) {
    // always create them as hidden
    parentNode.hidden = true;
  }

  if (aLibrary == this._libraryManager.mainLibrary) {
    var node = null;
    // note that |node| should be the audio node at the end of this loop,
    // for use by the migrate-from-before-split-views step
    for each (let type in ["video", "audio"]) {
      node = makeNodeFromLibrary(aLibrary, type, parentNode);
      node.name = '&servicesource.library.' + type;
      if (aMove || !node.parentNode) {
        this._insertNodeAfter(parentNode, node);
      }
    }
    this._servicePane.sortNode(parentNode);

    if (newNode) {
      parentNode.hidden = false;
      this._insertLibraryNode(parentNode, aLibrary);
    }
    else if (parentNode.properties.split(/\s/).indexOf('library') != -1) {
      // this came from before we had split views for the libraries
      // we need to migrate
      parentNode.hidden = false;
      let oldProps = parentNode.properties.split(/\s+/);
      let newProps = oldProps.filter(function(val) K_PARENT_PROPS.indexOf(val) == -1);
      this._mergeProperties(node, newProps);
      parentNode.properties = K_PARENT_PROPS.join(" ");

      // properties we know we want to delete
      for each (let i in ["url", "image", "tooltip", "contractid",
                          "dndDragTypes", "dndAcceptNear", "dndAcceptIn"])
      {
        parentNode[i] = null;
      }
    }

    // the main library uses a separate Playlists and Podcasts folder
    this._ensurePlaylistFolderExists();
    this._ensurePodcastFolderExists();

    // if the iTunes folder exists, then make it visible
    var fnode = this._servicePane.getNode('SB:iTunes');
    if (fnode)
      fnode.hidden = false;
  }
  else {
    for each (let type in ["video", "audio"]) {
      let node = makeNodeFromLibrary(aLibrary, type, parentNode);
      node.name = '&servicesource.library.' + type;
      if (aMove || !node.parentNode) {
        this._insertNodeAfter(parentNode, node);
      }

      // other libraries store the playlists under them, but only
      // assign the default value if they do not specifically tell
      // us not to do so

      if (node.getAttributeNS(SP,'dndCustomAccept') != 'true')
        node.dndAcceptIn = 'text/x-sb-playlist-'+aLibrary.guid;
    }
    this._servicePane.sortNode(parentNode);
  }

  return parentNode;
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
  // Don't set name if it hasn't changed to avoid a UI redraw
  if (node.name != aMediaList.name) {
    node.name = aMediaList.name;
  }
  node.url = null;
  node.contractid = CONTRACTID;
  if (customType == 'download') {
    // the download media list isn't editable
    node.editable = false;
    // set the weight of the downloads list
    node.setAttributeNS(SP, 'Weight', -3);
  } else {
    // the rest are, but only if the items themselves are not readonly
    node.editable = aMediaList.userEditable;
  }
  // Set properties for styling purposes
  if (aMediaList.getProperty(SBProperties.isSubscription) == "1") {
    this._mergeProperties(node, ["medialist", "medialisttype-dynamic"]);
    node.setAttributeNS(LSP, "ListSubscription", "1");
  } else {
    this._mergeProperties(node, ["medialist medialisttype-" + aMediaList.type]);
    node.setAttributeNS(LSP, "ListSubscription", "0");
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

  // if auto dndAcceptIn/Near hasn't been disabled, assign it now
  if (node.getAttributeNS(SP,'dndCustomAccept') != 'true') {
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
  }

  if (newnode) {
    // Place the node in the tree
    this._insertMediaListNode(node, aMediaList);
  }

  // Get hidden state from list, since we hide all list nodes on startup
  node.hidden = (aMediaList.getProperty(SBProperties.hidden) == "1");

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
  this._mergeProperties(fnode, ["folder", this._makeCSSProperty(fnode.name)]);
  fnode.hidden = false;
  fnode.contractid = CONTRACTID;
  fnode.dndAcceptIn = 'text/x-sb-playlist';
  fnode.editable = false;
  fnode.setAttributeNS(SP, 'Weight', 3);
  return fnode;
}

/**
 * Get the service pane node for the Podcasts folder (which contains all
 * the podcasts in the main library).
 */
sbLibraryServicePane.prototype._ensurePodcastFolderExists =
function sbLibraryServicePane__ensurePodcastFolderExists() {
  // Return null per bug 17607.  Return value should not get used since podcasts
  // can no longer be created.
  return null;

  var fnode = this._servicePane.getNode("SB:Podcasts");
  if (!fnode) {
    // make sure it exists
    var fnode = this._servicePane.addNode("SB:Podcasts",
                                          this._servicePane.root,
                                          true);
  }
  fnode.name = "&servicesource.podcasts";
  this._mergeProperties(fnode, ["folder", this._makeCSSProperty(fnode.name)]);
  fnode.hidden = false;
  fnode.contractid = CONTRACTID;
  fnode.editable = false;
  fnode.setAttributeNS(SP, "Weight", 2);
  return fnode;
}

/**
 * Get the service pane node for the iTunes folder (which contains all
 * the imported iTunes playlists in the main library).
 */
sbLibraryServicePane.prototype._ensureiTunesFolderExists =
function sbLibraryServicePane__ensureiTunesFolderExists() {
  var fnode = this._servicePane.getNode('SB:iTunes');
  if (!fnode) {
    // make sure it exists
    var fnode = this._servicePane.addNode('SB:iTunes',
        this._servicePane.root, true);
  }
  fnode.name = '&servicesource.itunes';
  this._mergeProperties(fnode, ["folder", this._makeCSSProperty(fnode.name)]);
  fnode.hidden = false;
  fnode.contractid = CONTRACTID;
  fnode.editable = false;
  fnode.setAttributeNS(SP, 'Weight', 3);
  return fnode;
}


sbLibraryServicePane.prototype._scanForRemovedItems =
function sbLibraryServicePane__scanForRemovedItems(aLibrary) {
  // Get the list of nodes for items within the library
  var libraryItemNodeList = this._servicePane.getNodesByAttributeNS
                                                (LSP,
                                                 "LibraryGUID",
                                                 aLibrary.guid);

  // Remove nodes whose items no longer exist
  var libraryItemNodeEnum = libraryItemNodeList.enumerate();
  while (libraryItemNodeEnum.hasMoreElements()) {
    // Get the library item node
    libraryItemNode =
      libraryItemNodeEnum.getNext().QueryInterface(Ci.sbIServicePaneNode);

    // Skip library nodes
    if (this._nodeIsLibrary(libraryItemNode))
      continue;

    // Remove node if item no longer exists
    var mediaItem = this._getItemForURN(libraryItemNode.id);
    if (!mediaItem)
      this._servicePane.removeNode(libraryItemNode);
  }
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
sbLibraryServicePane.prototype._insertNodeAfter =
function sbLibraryServicePane__insertNodeAfter(aParent, aNode, aPrecedingNode) {
  // don't be bad
  if (!aParent.isContainer) {
    LOG("sbLibraryServicePane__insertNodeAfter: cannot insert under non-container node.");
    return;
  }
  aParent.isOpen = true;

  // the node we'll use to insertBefore
  var next;
  // if the preceding node is not null, use its next sibling, otherwise
  // use the parent first child
  if (aPrecedingNode)
    next = aPrecedingNode.nextSibling;
  else
    next = aParent.firstChild;
    
  if (next) {
    // if the next node is same node as the one we are inserting, 
    // there is nothing to do
    if (next.id != aNode.id) {
      aParent.insertBefore(aNode, next);
    }
  } else {
    // there is no next item, the parent has no item at all, so use
    // append instead
    aParent.appendChild(aNode);
  }
}

/**
 * Logic to determine where a media list node should appear
 * in the service pane tree
 */
sbLibraryServicePane.prototype._insertMediaListNode =
function sbLibraryServicePane__insertMediaListNode(aNode, aMediaList) {
  //logcall(arguments);

  // If it is a main library media list, it belongs in either the
  // "Playlists" or "iTunes" folder, depending on whether it was imported
  // from iTunes or not
  if (aMediaList.library == this._libraryManager.mainLibrary)
  {
    // the download playlist is a special case
    if (aNode.getAttributeNS(LSP, 'ListCustomType') == 'download') {
      // FIXME: put it right after the library
      this._servicePane.root.appendChild(aNode);
    } else {
      var self = this;
      // this describes each type of playlist with the attributes/value pairs
      // that we can use to recognize it.
      // Note that the "type" field should be unique. it is not used anywhere
      // else than in this function, so you may use whatever you wish.
      var type_smart =        { type: "smart",
                                conditions: [ { attribute: "ListType",
                                                value    : "smart" } ]
                              };
      var type_simple =       { type: "simple",
                                conditions: [ { attribute: "ListType",
                                                value    : "simple" },
                                              { attribute: "ListSubscription",
                                                value    : "0" } ]
                              };
      var type_subscription = { type: "subscription",
                                conditions: [ { attribute: "ListType",
                                                value    : "simple" },
                                              { attribute: "ListSubscription",
                                                value    : "1" } ]
                              };
      
      // this is the order in which the playlists appear in the playlist folder
      var playlistTypes = [ type_smart, 
                            type_simple, 
                            type_subscription ];
      
      // create a search array to be used in _findLastNodeByAttributes
      // based on the type provided. If the type is not found, the search
      // array will contain all the types, so the list will go to the end
      // of the folder.
      function makeSearchArray(aPlaylistType) {
        var searchArray = [];
        for (var i=0;i<playlistTypes.length;i++) {
          var typeObject = playlistTypes[i];
          searchArray.push(typeObject.conditions);
          if (typeObject.type == aPlaylistType)
            break;
        }
        return searchArray;
      }
      
      // returns the playlist type for the given node
      function getNodeType(aNode) {
        for (var i=0;i<playlistTypes.length;i++) {
          var typeObject = playlistTypes[i];
          if (self._matchNode(aNode, typeObject.conditions))
            return typeObject.type;
        }
        return null;
      }

      // make sure the playlist folder exists
      var folder;
      // if it has an iTunesGUID property, it's imported from iTunes
      if (aMediaList.getProperty(SBProperties.iTunesGUID) != null) {
        folder = this._ensureiTunesFolderExists();
      } else if (aMediaList.getProperty(SBProperties.customType) == "podcast") {
        folder = this._ensurePodcastFolderExists();
      } else {
        folder = this._ensurePlaylistFolderExists();
      }
      this._servicePane.sortNode(folder);
      
      // get the type for this node
      var nodeType = getNodeType(aNode);
      
      // construct the search array
      var searchArray = makeSearchArray(nodeType);

      // look for the insertion point
      var insertionPoint = this._findLastNodeByAttributes(folder, 
                                                          searchArray,
                                                          aNode);
      
      // insert at the right spot (if insertionPoint is null, the node will
      // be inserted as the new first child)
      this._insertNodeAfter(folder, aNode, insertionPoint);
    }
  }
  // If it is a secondary library playlist, it should be
  // added as a child of that library
  else
  {
    // Find the parent libary in the tree
    var parentLibraryNode = this.getNodeForLibraryResource(aMediaList.library);

    // If the parent library is a top level node, and a container, 
    // make the playlist node its child
    
    if (parentLibraryNode) {
      if (parentLibraryNode.isContainer && 
          parentLibraryNode.parentNode.id == "SB:Root") {
        this._insertAfterLastOfSameType(aNode, parentLibraryNode);
      } else {
        this._insertAfterLastOfSameType(aNode, parentLibraryNode.parentNode);
      }
    } else {
      LOG("sbLibraryServicePane__insertMediaListNode: could not add media list to parent library");
      if (parentLibraryNode) {
        LOG(parentLibraryNode.name);
      }
      this._servicePane.root.appendChild(aNode);
    }
  }
}

/**
 * Match a node to a list of attribute/value pairs. All of the values for a 
 * node should match for this function to return true.
 */
sbLibraryServicePane.prototype._matchNode =
function sbLibraryServicePane__matchNode(aNode, aAttributeValues) {
  // it's a match if there are no attribute/value pairs in the list
  var match = true;
  
  // match each of the attribute/value pairs
  for each (var match in aAttributeValues) {
    var attribute = match.attribute;
    var value = match.value;
    // if this is not a match, skip this condition
    if (aNode.getAttributeNS(LSP, attribute) != value) {
      match = false;
      // don't test any more attribute/value pairs
      break;
    }    
  }
  
  return match;
}

/**
 * Finds the last node that matches any of the search criterions in a list,
 * with an optional excluded node.
 * Each search criterion is a list of attribute/pairs that must all match a node
 * for the condition to be fulfilled.
 */
sbLibraryServicePane.prototype._findLastNodeByAttributes =
function sbLibraryServicePane__findLastNodeByAttributes(aParent, 
                                                        aSearchArray,
                                                        aExcludedNode) {
  var children = aParent.childNodes;
  var lastMatch;
  while (children.hasMoreElements()) {
    var child = children.getNext().QueryInterface(Ci.sbIServicePaneNode);
    
    if (child.hidden)
      continue;

    // If this node belongs to the library service pane, and
    // is not the node we are trying to insert, then check
    // to see if this is a match
    if (child.contractid == CONTRACTID && 
        (!aExcludedNode || 
          child.id != aExcludedNode.id)) {
    
      // check each of the search criteria
      for each (var criterion in aSearchArray) {
        if (this._matchNode(child, criterion)) {
          // remember this node
          lastMatch = child;
          // don't test any more criteria
          break;
        }
      }
    }
  } // end of while
  return lastMatch;
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
    LOG("sbLibraryServicePane__insertAfterLastOfSameType: cannot insert under non-container node.");
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

/**
 * Turn a partial entity (&foo.bar) into a css property string (foo-bar),
 * but leaves other strings as they are.
 */
sbLibraryServicePane.prototype._makeCSSProperty = 
function sbLibraryServicePane__makeCSSProperty(aString) {
  if ( aString[0] == "&" ) {
    aString = aString.substr(1, aString.length);
    aString = aString.replace(/\./g, "-");
  }
  return aString;
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
function sbLibraryServicePane_onItemAdded(aMediaList, aMediaItem, aIndex) {
  //logcall(arguments);
  if (this._batch[aMediaList.guid] && this._batch[aMediaList.guid].isActive()) {
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
function sbLibraryServicePane_onBeforeItemRemoved(aMediaList, aMediaItem, aIndex) {
  //logcall(arguments);
  if (this._batch[aMediaList.guid] && this._batch[aMediaList.guid].isActive()) {
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
function sbLibraryServicePane_onAfterItemRemoved(aMediaList, aMediaItem, aIndex) {
}
sbLibraryServicePane.prototype.onItemUpdated =
function sbLibraryServicePane_onItemUpdated(aMediaList,
                                            aMediaItem,
                                            aProperties) {
  if (this._batch[aMediaList.guid] && this._batch[aMediaList.guid].isActive()) {
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
sbLibraryServicePane.prototype.onItemMoved =
function sbLibraryServicePane_onItemMoved(aMediaList,
                                          aFromIndex,
                                          aToIndex) {
}
sbLibraryServicePane.prototype.onBeforeListCleared =
function sbLibraryServicePane_onBeforeListCleared(aMediaList) {
}
sbLibraryServicePane.prototype.onListCleared =
function sbLibraryServicePane_onListCleared(aMediaList) {
  if (this._batch[aMediaList.guid] && this._batch[aMediaList.guid].isActive()) {
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
  if (!this._batch[aMediaList.guid]) {
    this._batch[aMediaList.guid] = new LibraryUtils.BatchHelper();
  }
  this._batch[aMediaList.guid].begin();
}
sbLibraryServicePane.prototype.onBatchEnd =
function sbLibraryServicePane_onBatchEnd(aMediaList) {
  //logcall(arguments);
  if (!this._batch[aMediaList.guid]) {
    return;
  }

  this._batch[aMediaList.guid].end();
  if (!this._batch[aMediaList.guid].isActive() && this._refreshPending) {
    this._refreshLibraryNodes(aMediaList);
    this._refreshPending = false;
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

var NSGetModule = XPCOMUtils.generateNSGetModule([sbLibraryServicePane]);
