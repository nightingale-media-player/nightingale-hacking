/** vim: ts=2 sw=2 expandtab
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

  // get the library manager
  this._initLibraryManager();
}

sbLibraryServicePane.prototype.shutdown =
function sbLibraryServicePane_shutdown() {}

sbLibraryServicePane.prototype.fillContextMenu =
function sbLibraryServicePane_fillContextMenu(aNode, aContextMenu, aParentWindow) {

  // the playlists folder and the local library node get the "New Foo..." items
  if (aNode.id == 'SB:Playlists' ||
      aNode.className.match(/\blibrary\b/) ||
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

sbLibraryServicePane.prototype._canDropReorder =
function sbLibraryServicePane__canDropReorder(aNode, aDragSession, aOrientation) {
  // see if we can handle the drag and drop based on node properties
  let types = [];
  if (aOrientation == 0) {
    // drop in
    if (aNode.dndAcceptIn) {
      types = aNode.dndAcceptIn.split(',');
    }
  } else {
    // drop near
    if (aNode.dndAcceptNear) {
      types = aNode.dndAcceptNear.split(',');
    }
  }
  for each (let type in types) {
    if (aDragSession.isDataFlavorSupported(type)) {
      return type;
    }
  }
  return null;
}


sbLibraryServicePane.prototype.canDrop =
function sbLibraryServicePane_canDrop(aNode, aDragSession, aOrientation, aWindow) {
  // see if we can handle the drag and drop based on node properties
  if (this._canDropReorder(aNode, aDragSession, aOrientation)) {
    return true;
  }
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
  // see if this is a reorder we can handle based on node properties
  let type = this._canDropReorder(aNode, aDragSession, aOrientation);
  if (type) {
    // we're in business

    // do the dance to get our data out of the dnd system
    // create an nsITransferable
    let transferable = Cc["@mozilla.org/widget/transferable;1"]
                         .createInstance(Ci.nsITransferable);
    // specify what kind of data we want it to contain
    transferable.addDataFlavor(type);
    // ask the drag session to fill the transferable with that data
    aDragSession.getData(transferable, 0);
    // get the data from the transferable
    let data = {};
    transferable.getTransferData(type, data, {});
    // it's always a string. always.
    data = data.value.QueryInterface(Ci.nsISupportsString).data;

    // for drag and drop reordering the data is just the servicepane node id
    let droppedNode = this._servicePane.getNode(data);

    // fail if we can't get the node or it is the node we are over
    if (!droppedNode || aNode == droppedNode) {
      return;
    }

    if (aOrientation == 0) {
      // drop into
      aNode.appendChild(droppedNode);
    } else if (aOrientation > 0) {
      // drop after
      aNode.parentNode.insertBefore(droppedNode, aNode.nextSibling);
    } else {
      // drop before
      aNode.parentNode.insertBefore(droppedNode, aNode);
    }
    // work out what library resource is associated with the moved node 
    var medialist = this.getLibraryResourceForNode(aNode);
    
    this._saveListsOrder(medialist.library, aNode.parentNode);
    return;
  }
  
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

sbLibraryServicePane.prototype._saveListsOrder =
function sbLibraryServicePane__saveListsOrder(library, nodesParent) {
  var str = "";
  var children = nodesParent.childNodes;
  while (children.hasMoreElements()) {
    var child = children.getNext();
    var resource = this.getLibraryResourceForNode(child);
    if (resource instanceof Components.interfaces.sbIMediaList) {
      if (str != "")
        str += ",";
      str += resource.guid;
    }
  }
  var appPrefs = Cc["@mozilla.org/fuel/application;1"]
                   .getService(Ci.fuelIApplication).prefs;
  appPrefs.setValue("songbird.library_listorder." + library.guid, str);
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
 * Called when the user is about to attempt to rename a library/medialist node
 */
sbLibraryServicePane.prototype.onBeforeRename =
function sbLibraryServicePane_onBeforeRename(aNode) {
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

  // check the children of the node (but not recursively) for a usable library
  for (var child = aNode.firstChild; child; child = child.nextSibling) {
    lib = checkNode(child, this);
    if (lib)
      return lib;
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

/* \brief Suggest a unique name for creating a new playlist
 *
 * \param aLibrary an sbILibrary.
 * \return a unique playlist name.
 */
sbLibraryServicePane.prototype.suggestNameForNewPlaylist =
function sbLibraryServicePane_suggestNameForNewPlaylist(aLibrary) {
  // Give the playlist a default name
  // TODO: Localization should be done internally
  var name = SBString("playlist", "Playlist");
  var length = name.length;

  if(aLibrary instanceof Ci.sbILibrary) {
    // Build the existing IDs array
    let listIDs = [];
    let mediaLists = aLibrary.getItemsByProperty(SBProperties.isList, "1");
    for (let i = 0; i < mediaLists.length; ++i) {
      let mediaListName = mediaLists.queryElementAt(i, Ci.sbIMediaList).name;
      if (mediaListName && mediaListName.substr(0, length) == name) {
        if (mediaListName.length == length) {
          listIDs.push(1);
        }
        else if (mediaListName.length > length + 1) {
          listIDs.push(parseInt(mediaListName.substr(length + 1)));
        }
      }
    }

    let id = 1;
    while (1) {
      // The id is available.
      if (listIDs.indexOf(id) == -1)
        break;

      ++id;
    }

    if (id > 1)
      name = SBFormattedString("playlist.sequence", [id]);
  }

  return name;
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

  if (aResource instanceof Ci.sbILibrary) {
    // For backwards compatibility, prefer the audio/video node for libraries
    // over the container (if any of them is visible)
    for each (let type in ["audio", "video", "podcast"]) {
      let constrainedURN = urn + ":constraint(" + type + ")";
      let node = this._servicePane.getNode(constrainedURN);
      if (node && !node.hidden) {
        return node;
      }
    }
  }

  // For playlists and libraries that don't have any visible children, return
  // the main node - even if it is hidden
  return this._servicePane.getNode(urn);
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
  var urn = this._getURNForLibraryResource(aMediaListView.mediaList);

  var values = this._getConstraintsValueArrayFromMediaListView(aMediaListView);
  for (i = 0; i < values.length; ++i) {
    urn += ":constraint(" + values[i] + ")";
  }

  return this._servicePane.getNode(urn);
}

/**
 * \brief Attempt to get the content type of service pane node for the given
 *        media list view.
 *
 * \param aMediaListView the view for which to get the content type.
 * \returns the content type of service pane node that represents the given
 *          media list view or null if the type information is not available
 *          for the node.
 */
sbLibraryServicePane.prototype.getNodeContentTypeFromMediaListView =
function sbLibraryServicePane_getNodeContentTypeFromMediaListView(
                                  aMediaListView) {
  var values = this._getConstraintsValueArrayFromMediaListView(aMediaListView);

  const K_TYPES = ["audio", "video", "podcast"];

  for (i = 0; i < values.length; ++i) {
    if (K_TYPES.indexOf(values[i]) > -1)
      return values[i];
  }

  return null;
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

sbLibraryServicePane.prototype._getConstraintsValueArrayFromMediaListView =
function sbLibraryServicePane__getConstraintsValueArrayFromMediaListView(
                                   aMediaListView) {
  var values = [];

  if ((aMediaListView instanceof Ci.sbIFilterableMediaListView) &&
      aMediaListView.filterConstraint)
  {
    // stash off the properties involved in the standard constraints,
    // so that we don't look at them
    var standardConstraintProperties = {};
    const standardConstraint = LibraryUtils.standardFilterConstraint;
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
          values.push(value);
        }
      }
    }
  }

  return values;
}


/**
 * Return true if the given device supports playlist
 */
sbLibraryServicePane.prototype._doesDeviceSupportPlaylist =
function sbLibraryServicePane__doesDeviceSupportPlaylist(aDevice) {
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
}


/**
 * Return true if the given library supports the given list type
 */
sbLibraryServicePane.prototype._doesLibrarySupportListType =
function sbLibraryServicePane__doesLibrarySupportListType(aLibrary, aListType) {
  //logcall(arguments);

  var device;
  // Get device from non-device library will cause NS_ERROR_NOT_IMPLEMENTED.
  // Device library could also return NS_ERROR_UNEXPECTED on failure.
  try {
    device = aLibrary.device;
  } catch (e) {
    device = null;
  }

  // Check whether the device support playlist.
  if (device && !this._doesDeviceSupportPlaylist(device)) {
    return false;
  }

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
  
  // copy array of lists
  var remaining = listener.items.slice(0);
  
  // create nodes in the saved order, ignore guids with no
  // corresponding medialist and nodes whose guid isnt in the
  // saved order
  var appPrefs = Cc["@mozilla.org/fuel/application;1"]
                   .getService(Ci.fuelIApplication).prefs;
  var saved = appPrefs.getValue("songbird.library_listorder." + aLibrary.guid, "");
  var savedArray = saved.split(",");
  for each (var listguid in savedArray) {
    for (var i in remaining) {
      if (remaining[i].guid == listguid) {
        this._ensureMediaListNodeExists(remaining[i], true);
        remaining.slice(i, 1);
        break;
      }
    }
  }
  
  // Make sure we have a node for each remaining list. each node will be
  // inserted after the last node of the same type
  for (var i = 0; i < remaining.length; i++) {
    this._ensureMediaListNodeExists(remaining[i], false);
  }
}


/**
 * The given library has been added.  Show it in the service pane.
 */
sbLibraryServicePane.prototype._libraryAdded =
function sbLibraryServicePane__libraryAdded(aLibrary) {
  //logcall(arguments);
  var node = this._ensureLibraryNodeExists(aLibrary);

  // Listen to changes in the library so that we can display new playlists
  var filter = SBProperties.createArray([[SBProperties.hidden, null],
                                         [SBProperties.mediaListName, null]]);
  aLibrary.addListener(this,
                       false,
                       Ci.sbIMediaList.LISTENER_FLAGS_ALL,
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

  // Get the list of nodes for items within the library
  var libraryItemNodeList = this._servicePane.getNodesByAttributeNS
                                                (LSP,
                                                 "LibraryGUID",
                                                 aLibrary.guid);

  // Hide all nodes for items within the library
  var libraryItemNodeEnum = libraryItemNodeList.enumerate();
  while (libraryItemNodeEnum.hasMoreElements()) {
    // Hide the library item node
    libraryItemNode =
      libraryItemNodeEnum.getNext().QueryInterface(Ci.sbIServicePaneNode);
    this._servicePane.setNodeHidden(libraryItemNode, CONTRACTID, true);
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
    node.parentNode.removeChild(node);
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
    try {
      return this._libraryManager.getLibrary(guid);
    }
    catch (e) {
      LOG("sbLibraryServicePane__getLibraryForURN: error trying to get " +
          "library " + guid);
    }
  }
  return null;
}



/**
 * Ensure the library nodes for the given library exists, then return the
 * container node
 */
sbLibraryServicePane.prototype._ensureLibraryNodeExists =
function sbLibraryServicePane__ensureLibraryNodeExists(aLibrary) {
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
    var id = self._libraryURN(aLibrary);
    if (aConstraintType) {
      // add a constraint only if the constraint type was specified
      id += ":constraint(" + aConstraintType + ")";
    }
    var node = self._servicePane.getNode(id);
    if (!node) {
      // Create the node
      node = self._servicePane.createNode();
      node.id = id;
      node.contractid = CONTRACTID;
      node.editable = false;

      // Set properties for styling purposes
      self._addClassNames(node,
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
        self._addClassNames(node, [aConstraintType]);
        var builder = Cc["@songbirdnest.com/Songbird/Library/ConstraintBuilder;1"]
                        .createInstance(Ci.sbILibraryConstraintBuilder);
        builder.includeConstraint(LibraryUtils.standardFilterConstraint);
        builder.intersect();
        builder.include(SBProperties.contentType, aConstraintType);
        node.setAttributeNS(SP,
                            "mediaListViewConstraints",
                            builder.get());
      }
    }
    var customType = aLibrary.getProperty(SBProperties.customType);

    // Refresh the information just in case it is supposed to change
    // Don't set name if it hasn't changed to avoid a UI redraw
    let name = (aConstraintType ? '&servicesource.library.' + aConstraintType :
                                  aLibrary.name);
    if (node.name != name) {
      node.name = name;
    }
    var hidden = (aLibrary.getProperty(SBProperties.hidden) == "1");
    self._servicePane.setNodeHidden(node, CONTRACTID, hidden);

    if (aParentNode && node.parentNode != aParentNode) {
      // Insert the node as first child
      aParentNode.insertBefore(node, aParentNode.firstChild);
    }
    return node;
  }

  var customType = aLibrary.getProperty(SBProperties.customType);

  if (customType == 'web') {
    // the web library has no video/audio split, nor a parent, so we need to
    // special case it here and return early
    let node = makeNodeFromLibrary(aLibrary, null, null);

    // Set the weight of the web library
    node.setAttributeNS(SP, 'Weight', 5);
    self._servicePane.setNodeHidden(node, CONTRACTID, true);

    if (!node.parentNode)
      this._servicePane.root.appendChild(node);

    return node;
  }

  // make the parent node
  var id = self._libraryURN(aLibrary);
  var parentNode = self._servicePane.getNode(id);
  if (!parentNode) {
    // Create the node
    parentNode = this._servicePane.createNode();
    parentNode.id = id;
    parentNode.editable = false;
    parentNode.setAttributeNS(SP, 'Weight', -4);

    // uncomment this to cause clicks on the container node to load an unfiltered
    // library.  we need to think more about the ramifications for UE before
    // turning this on, so it's off for now.
    // (see also below on the migration part)
    //parentNode.contractid = CONTRACTID;

    // class names that should exist on the parent container node
    const K_PARENT_PROPS = ["folder", "library-container"];
  
    self._addClassNames(parentNode, K_PARENT_PROPS);

    if (aLibrary != this._libraryManager.mainLibrary) {
      // always create them as hidden
      self._servicePane.setNodeHidden(parentNode, CONTRACTID, true);
    }
  }

  // Refresh the information just in case it is supposed to change
  // Don't set name if it hasn't changed to avoid a UI redraw
  if (parentNode.name != aLibrary.name) {
    parentNode.name = aLibrary.name;
  }

  if (aLibrary == this._libraryManager.mainLibrary) {
    for each (let type in ["video", "audio"]) {
      let node = makeNodeFromLibrary(aLibrary, type, parentNode);
    }

    // the main library uses a separate Playlists and Podcasts folder
    this._ensurePlaylistFolderExists();
    this._ensurePodcastFolderExists();

    // if the iTunes folder exists, then make it visible
    var fnode = this._servicePane.getNode('SB:iTunes');
    if (fnode)
      this._servicePane.setNodeHidden(fnode, CONTRACTID, false);
  }
  else {
    for each (let type in ["video", "audio"]) {
      let node = makeNodeFromLibrary(aLibrary, type, parentNode);

      // other libraries store the playlists under them, but only
      // assign the default value if they do not specifically tell
      // us not to do so

      if (node.getAttributeNS(SP,'dndCustomAccept') != 'true')
        node.dndAcceptIn = 'text/x-sb-playlist-'+aLibrary.guid;
    }
  }

  if (!parentNode.parentNode) {
    // Append library to the root, service pane will sort by weight
    this._servicePane.root.appendChild(parentNode);
  }

  return parentNode;
}


/**
 * Get the service pane node for the given media list,
 * creating one if none exists.
 */
sbLibraryServicePane.prototype._ensureMediaListNodeExists =
function sbLibraryServicePane__ensureMediaListNodeExists(aMediaList, aAppend) {
  //logcall(arguments);

  var id = this._itemURN(aMediaList);
  var node = this._servicePane.getNode(id);

  var customType = aMediaList.getProperty(SBProperties.customType);
  var libCustomType = aMediaList.library.getProperty(SBProperties.customType);

  if (!node) {
    // Create the node
    // NOTE: it's a container for drag and drop purposes only.
    node = this._servicePane.createNode();
    node.id = id;
    node.contractid = CONTRACTID;

    if (customType == 'download') {
      // the download media list isn't editable
      node.editable = false;
      // set the weight of the downloads list
      node.setAttributeNS(SP, 'Weight', 999);
    } else {
      // the rest are, but only if the items themselves are not readonly
      node.editable = aMediaList.userEditable;
    }

    // Set properties for styling purposes
    if (aMediaList.getProperty(SBProperties.isSubscription) == "1") {
      this._addClassNames(node, ["medialist", "medialisttype-dynamic"]);
      node.setAttributeNS(LSP, "ListSubscription", "1");
    } else {
      this._addClassNames(node, ["medialist medialisttype-" + aMediaList.type]);
      node.setAttributeNS(LSP, "ListSubscription", "0");
    }
    // Add the customType to the properties to encourage people to set it for CSS
    this._addClassNames(node, [customType]);
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
        // (unless it's the download playlist)
        if (customType != 'download') {
          node.dndDragTypes = 'text/x-sb-playlist';
          node.dndAcceptNear = 'text/x-sb-playlist';
        }
      } else {
        // playlists in other libraries can only go into their libraries' nodes
        node.dndDragTypes = 'text/x-sb-playlist-'+aMediaList.library.guid;
        node.dndAcceptNear = 'text/x-sb-playlist-'+aMediaList.library.guid;
      }
    }
  }

  // Refresh the information just in case it is supposed to change
  // Don't set name if it hasn't changed to avoid a UI redraw
  if (node.name != aMediaList.name) {
    node.name = aMediaList.name;
  }

  // Get hidden state from list
  var hidden = (aMediaList.getProperty(SBProperties.hidden) == "1");
  this._servicePane.setNodeHidden(node, CONTRACTID, hidden);

  if (!node.parentNode) {
    // Place the node in the tree
    this._insertMediaListNode(node, aMediaList, aAppend);
  }

  return node;
}

/**
 * Get the service pane node for the Playlists folder (which contains all
 * the playlists in the main library).
 */
sbLibraryServicePane.prototype._ensurePlaylistFolderExists =
function sbLibraryServicePane__ensurePlaylistFolderExists() {
  let fnode = this._servicePane.getNode("SB:Playlists");
  if (!fnode) {
    // make sure it exists
    fnode = this._servicePane.createNode();
    fnode.id = "SB:Playlists";
    fnode.name = '&servicesource.playlists';
    this._addClassNames(fnode, ["folder", this._makeCSSProperty(fnode.name)]);
    fnode.contractid = CONTRACTID;
    fnode.dndAcceptIn = 'text/x-sb-playlist';
    fnode.editable = false;
    fnode.setAttributeNS(SP, 'Weight', 3);
    this._servicePane.root.appendChild(fnode);
  }

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

  let fnode = this._servicePane.getNode("SB:Podcasts");
  if (!fnode) {
    // make sure it exists
    fnode = this._servicePane.createNode();
    fnode.id = "SB:Podcasts";
    fnode.name = "&servicesource.podcasts";
    this._addClassNames(fnode, ["folder", this._makeCSSProperty(fnode.name)]);
    fnode.contractid = CONTRACTID;
    fnode.editable = false;
    fnode.setAttributeNS(SP, "Weight", 2);
    this._servicePane.root.appendChild(fnode);
  }
  return fnode;
}

/**
 * Get the service pane node for the iTunes folder (which contains all
 * the imported iTunes playlists in the main library).
 */
sbLibraryServicePane.prototype._ensureiTunesFolderExists =
function sbLibraryServicePane__ensureiTunesFolderExists() {
  let fnode = this._servicePane.getNode("SB:iTunes");
  if (!fnode) {
    // make sure it exists
    fnode = this._servicePane.createNode();
    fnode.id = "SB:iTunes";
    fnode.name = '&servicesource.itunes';
    this._addClassNames(fnode, ["folder", this._makeCSSProperty(fnode.name)]);
    fnode.contractid = CONTRACTID;
    fnode.editable = false;
    fnode.setAttributeNS(SP, 'Weight', 3);
    this._servicePane.root.appendChild(fnode);
  }
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
      libraryItemNode.parentNode.removeChild(libraryItemNode);
  }
}

/**
 * Logic to determine where a media list node should appear
 * in the service pane tree
 */
sbLibraryServicePane.prototype._insertMediaListNode =
function sbLibraryServicePane__insertMediaListNode(aNode, aMediaList, aAppend) {
  //logcall(arguments);

  // If it is a main library media list, it belongs in either the
  // "Playlists" or "iTunes" folder, depending on whether it was imported
  // from iTunes or not
  if (aMediaList.library == this._libraryManager.mainLibrary)
  {
    // the download playlist is a special case
    if (aNode.getAttributeNS(LSP, 'ListCustomType') == 'download') {
      // Fix it to the bottom of the library node
      var libraryNode = this.getNodeForLibraryResource(aMediaList.library);

      // getNodeForLibraryResource will usually return a child of the library
      // container
      if (libraryNode.parentNode != this._servicePane.root)
        libraryNode = libraryNode.parentNode;

      libraryNode.appendChild(aNode);
    } else {
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

      if (aAppend)
        folder.appendChild(aNode);
      else  
        this._insertAfterLastOfSameType(aNode, folder);
    }
  }
  // If it is a secondary library playlist, it should be
  // added as a child of that library
  else
  {
    // Find the parent libary in the tree
    var parentLibraryNode = this.getNodeForLibraryResource(aMediaList.library);

    // getNodeForLibraryResource will usually return a child of the library
    // container
    if (parentLibraryNode && parentLibraryNode.parentNode != this._servicePane.root)
      parentLibraryNode = parentLibraryNode.parentNode;

    // If we found a parent library node make the playlist node its child
    if (parentLibraryNode) {
      if (aAppend)
        parentLibraryNode.appendChild(aNode);
      else
        this._insertAfterLastOfSameType(aNode, parentLibraryNode);
    } else {
      LOG("sbLibraryServicePane__insertMediaListNode: could not add media list to parent library");
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

  function getNodeType(aNode) {
    let type = aNode.getAttributeNS(LSP, "ListType");
    if (type == "simple" && aNode.getAttributeNS(LSP, "ListSubscription") == "1")
      return "subscription";
    else
      return type;
  }

  // Try to insert after last of same type
  let nodeType = getNodeType(aNode);
  let insertionPoint = null;
  let found = false;
  for (let child = aParent.lastChild; child; child = child.previousSibling) {
    // Stop at a node that belongs to the library service pane and has the
    // same list type as the node we are inserting
    if (child.contractid == CONTRACTID && getNodeType(child) == nodeType) {
      found = true;
      break;
    }

    insertionPoint = child;
  }

  // Insert the new list after the last of the same type
  // or at the end of the list
  if (found) {
    aParent.insertBefore(aNode, insertionPoint);
  } else {
    aParent.appendChild(aNode);
  }

  // Ensure that all parent containers are open and the node is visible
  for (let parent = aParent; parent; parent = parent.parentNode)
    if (!parent.isOpen)
      parent.isOpen = true;
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

    this._removeListNodesForLibrary(node, aLibraryGUID);

    var nextSibling = node.nextSibling;

    if (this._getItemGUIDForURN(node.id)) {
      var nodeLibraryGUID = node.getAttributeNS(LSP, "LibraryGUID");
      if (nodeLibraryGUID == aLibraryGUID) {
        node.parentNode.removeChild(node);
      }
    }

    node = nextSibling;
  }
}

sbLibraryServicePane.prototype._addClassNames =
function sbLibraryServicePane__addClassNames(aNode, aList) {
  let className = aNode.className || "";
  let existing = {};
  for each (let name in className.split(" "))
    existing[name] = true;

  for each (let name in aList)
    if (!existing.hasOwnProperty(name))
      className += (className ? " " : "") + name;

  aNode.className = className;
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
  return true;
}
sbLibraryServicePane.prototype.onAfterItemRemoved =
function sbLibraryServicePane_onAfterItemRemoved(aMediaList, aMediaItem, aIndex) {
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
  return true;
}
sbLibraryServicePane.prototype.onBeforeListCleared =
function sbLibraryServicePane_onBeforeListCleared(aMediaList,
                                                  aExcludeLists) {
  return true;
}
sbLibraryServicePane.prototype.onListCleared =
function sbLibraryServicePane_onListCleared(aMediaList,
                                            aExcludeLists) {
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

  // Make sure to remove the library manager listener on shutdown
  var obs = Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService);
  obs.addObserver(this, "songbird-library-manager-before-shutdown", false);

  this._addAllLibraries();
}

/////////////////
// nsIObserver //
/////////////////

sbLibraryServicePane.prototype.observe =
function sbLibraryServicePane_observe(subject, topic, data) {

  var obs = Cc["@mozilla.org/observer-service;1"]
              .getService(Ci.nsIObserverService);

  if (topic == "songbird-library-manager-before-shutdown") {
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
