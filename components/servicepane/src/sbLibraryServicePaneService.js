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

const CONTRACTID = "@songbirdnest.com/servicepane/library;1";
const ROOTNODE = "SB:Bookmarks";

const PROP_ISLIST = "http://songbirdnest.com/data/1.0#isList";

const URN_PREFIX_ITEM = 'urn:item:';
const URN_PREFIX_LIBRARY = 'urn:library:';

// TODO: Remove these... expose property so that images can be added via css?
const URL_ICON_PLAYLIST = 'chrome://songbird/skin/icons/icon_playlist_16x16.png';
const URL_ICON_LIBRARY = 'chrome://songbird/skin/icons/icon_lib_16x16.png';

// TODO: Remove this
const URL_PLAYLIST_DISPLAY = "chrome://songbird/content/xul/playlist_test2.xul?"

const LSP='http://songbirdnest.com/rdf/library-servicepane#';


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
 *
 * TODO: Remove debug dumps
 *
 * \sa sbIServicePaneService sbILibraryServicePaneService
 */
function sbLibraryServicePane() {
  this._servicePane = null;
  this._libraryManager = null;
  
  // use the default stringbundle to translate tree nodes
  this.stringbundle = null;
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
  logcall(arguments);

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
}

sbLibraryServicePane.prototype.fillContextMenu =
function sbLibraryServicePane_fillContextMenu(aNode, aContextMenu, aParentWindow) {
  // TODO
}
sbLibraryServicePane.prototype.canDrop =
function sbLibraryServicePane_canDrop(aNode, aDragSession, aOrientation) {
  return false;
}
sbLibraryServicePane.prototype.onDrop =
function sbLibraryServicePane_onDrop(aNode, aDragSession, aOrientation) {
}
sbLibraryServicePane.prototype.onDragGesture =
function sbLibraryServicePane_onDragGesture(aNode, aTransferable) {
  return false;
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
  logcall(arguments);
  
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
  logcall(arguments);
  
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
  logcall(arguments);
  
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
  logcall(arguments);
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
 * the libary service pane service
 */
sbLibraryServicePane.prototype._hideLibraryNodes =
function sbLibraryServicePane__hideLibraryNodes(aNode) {
  logcall(arguments);
  
  // If this is one of our nodes, hide it
  dump("sbLibraryServicePane__hideLibraryNodes testing " + aNode.name + " \n");
  if (aNode.contractid == CONTRACTID) {
    dump("sbLibraryServicePane__hideLibraryNodes Hiding " + aNode.name + " \n");
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
sbLibraryServicePane.prototype._processLibraries =
function sbLibraryServicePane__processLibraries() {
  logcall(arguments);
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
  logcall(arguments);

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
  logcall(arguments);  
  this._ensureLibraryNodeExists(aLibrary);
  this._processListsInLibrary(aLibrary);
}


/**
 * The given library has been removed.  Just hide the contents
 * rather than deleting so that if it is ever reattached
 * we will remember any ordering (drag-drop) information
 */
sbLibraryServicePane.prototype._libraryRemoved =
function sbLibraryServicePane__libraryRemoved(aLibrary) {
  logcall(arguments);
  
  // Find the node for this library
  // TODO needs testing
  var id = this._libraryURN(aLibrary);
  var node = this._servicePane.getNode(id);
  
  // Hide this node and everything below it
  if (node) {
    this._hideLibraryNodes(node);
  }
}


/** 
 * The given media list has been added. Show it in the service pane.
 */
sbLibraryServicePane.prototype._playlistAdded =
function sbLibraryServicePane__playlistAdded(aMediaList) {
  logcall(arguments);
  this._ensureMediaListNodeExists(aMediaList);
}


/** 
 * The given media list has been removed. Delete the node, as it 
 * is unlikely that the same playlist will come back again.
 */
sbLibraryServicePane.prototype._playlistRemoved =
function sbLibraryServicePane__playlistRemoved(aMediaList) {
  logcall(arguments);
  
  // TODO needs testing
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
sbLibraryServicePane.prototype._playlistUpdated =
function sbLibraryServicePane__playlistUpdated(aMediaList) {
  logcall(arguments);
  this._ensureMediaListNodeExists(aMediaList);
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
  logcall(arguments);
  var index = aID.indexOf(URN_PREFIX_ITEM);
  if (index >= 0) {
    return aID.slice(index);
  }
  return null;
}


/**
 * Given a resource id, attempt to extract the GUID of a library.
 */
sbLibraryServicePane.prototype._getLibraryGUIDForURN =
function sbLibraryServicePane___getLibraryGUIDForURN(aID) {
  logcall(arguments);
  var index = aID.indexOf(URN_PREFIX_LIBRARY);
  if (index >= 0) {
    return aID.slice(index);
  }
  return null;
}


/**
 * Given a resource id, attempt to get an sbIMediaItem.
 * This is the inverse of _itemURN
 */
sbLibraryServicePane.prototype._getItemForURN =
function sbLibraryServicePane__getItemForURN(aID) {
  logcall(arguments);
  var guid = this._getItemGUIDForURN(aID);
  if (guid) {
    var item = null;
    
    // First try the main library
    var mainLibrary = this._libraryManager.mainLibrary;
    item = mainLibrary.getMediaItem(guid);
    if (item) {
      return item;
    }
    
    // Well that didn't work... guess we have to look 
    // through all the libraries.
    var libraries = this._libraryManager.getLibraryEnumerator();
    while (libraries.hasMoreElements()) {
      var library = libraries.getNext();
      item = library.getMediaItem(guid);
      if (item) {
        return item;
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
  logcall(arguments);
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
  logcall(arguments);
  
  // Should really ask someone else... but for now just hardcode
  var url = URL_PLAYLIST_DISPLAY;
  if (aResource instanceof Ci.sbILibrary) {
    url += "library,"
  }
  return url + aResource.guid;
}



/**
 * Get the service pane node for the given library,
 * creating one if none exists.
 */
sbLibraryServicePane.prototype._ensureLibraryNodeExists =
function sbLibraryServicePane__ensureLibraryNodeExists(aLibrary) {
  logcall(arguments);

  var id = this._libraryURN(aLibrary);
  dump ('ensureLibraryNodeExists: '+id+'\n');
  var node = this._servicePane.getNode(id);
  if (!node) {
    // Create the node
    node = this._servicePane.addNode(id, this._servicePane.root, true);
    node.url = this._getDisplayURL(aLibrary);
    node.name = aLibrary.name;
    node.image = URL_ICON_LIBRARY;
    node.contractid = CONTRACTID;
    node.dndDragTypes = 'text/x-sb-toplevel';
    node.dndAcceptNear = 'text/x-sb-toplevel';
    node.properties = "library";
    node.editable = true;
    
    // Set properties for styling purposes
    node.properties = "library libraryguid_" + aLibrary.guid;
    
    // Save the type of media list so that we can group by type
    node.setAttributeNS(LSP, "ListType", aLibrary.type)    
    
    // Position the node in the tree
    this._insertLibraryNode(node, aLibrary);
  }
  
  // Refresh the name just in case it was localized
  node.name = aLibrary.name;  
  
  // Make sure the node is visible, since we hid all library nodes on startup
  node.hidden = false;
  
  // Listen to changes in the library so that we can display new playlists
  aLibrary.addListener(this);
  
  return node;
}

 
/**
 * Get the service pane node for the given media list,
 * creating one if none exists.
 */ 
sbLibraryServicePane.prototype._ensureMediaListNodeExists =
function sbLibraryServicePane__ensureMediaListNodeExists(aMediaList) {
  logcall(arguments);

  var id = this._itemURN(aMediaList);
  dump ('_ensureMediaListNodeExists: '+id+'\n');
  var node = this._servicePane.getNode(id);
  if (!node) {
    // Create the node
    node = this._servicePane.addNode(id, this._servicePane.root, false);
    node.url = this._getDisplayURL(aMediaList);
    node.image = URL_ICON_PLAYLIST;
    node.contractid = CONTRACTID;
    node.editable = true;
    
    // Set properties for styling purposes
    node.properties = "medialist medialisttype_" + aMediaList.type;
    
    // Save the type of media list so that we can group by type
    node.setAttributeNS(LSP, "ListType", aMediaList.type);
    
    // Place the node in the tree
    this._insertMediaListNode(node, aMediaList);
  }
  // Refresh the name just in case it was localized
  node.name = aMediaList.name;  
  
  // Make sure the node is visible, since we hid all library nodes on startup
  node.hidden = false;
      
  return node;
}


/**
 * Logic to determine where a library node should appear
 * in the service pane tree
 */ 
sbLibraryServicePane.prototype._insertLibraryNode =
function sbLibraryServicePane__insertLibraryNode(aNode, aLibrary) {
  logcall(arguments);
  
  // If this is the main library it should go at
  // the top of the list
  if (aLibrary == this._libraryManager.mainLibrary) {
    if (this._servicePane.root.firstChild) {
      this._servicePane.root.insertBefore(aNode, 
              this._servicePane.root.firstChild);    
    } else {
      this._servicePane.root.appendChild(aNode);    
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
  logcall(arguments);

  // If it is a main library media list, it belongs at 
  // the top level
  if (aMediaList.library == this._libraryManager.mainLibrary) 
  {  
    this._insertAfterLastOfSameType(aNode, this._servicePane.root);
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
      dump(parentLibraryNode.name + "\n");
      this._servicePane.root.appendChild(aNode);
    }
  }
}


 
/**
 * Inserts the given node under the given parent and attempts to keep
 * children grouped by type.  The node is inserted at the end of the list
 * or next to the last child of the same type as the given node.
 * TODO: Not fully tested!
 */ 
sbLibraryServicePane.prototype._insertAfterLastOfSameType =
function sbLibraryServicePane__insertAfterLastOfSameType(aNode, aParent) {
  logcall(arguments);
    
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
    
    // If we own this node...
    if (child.contractid == CONTRACTID) {
    
      // Keep track of last node of the same type as this one  
      if (nodeType == child.getAttributeNS(LSP, "ListType")) {
        lastOfSameType = child;
      }
      
    // When we get to the end of the library section...
    } else {        
      // Insert the new list after the last of the same type
      // or at the end of the list
      if (lastOfSameType && lastOfSameType.nextSibling) {
        aParent.insertBefore(aNode, lastOfSameType.nextSibling);
      } else {
        aParent.insertBefore(aNode, child);
      }
      
      inserted = true;            
      break;
    }     
  } // end of while
  
  // If for some reason we weren't able to insert the item
  // (eg no bookmarks, or a completely empty service tree), 
  // then just add it at the end.
  if (!inserted) {
    dump("sbLibraryServicePane__insertAfterLastOfSameType: Unable to find a ");
    dump("good place to insert a library resource node. Defaulting to end.\n");
    aParent.appendChild(aNode);
  }
}



///////////////////////////////
// sbILibraryManagerListener //
///////////////////////////////

sbLibraryServicePane.prototype.onLibraryRegistered =
function sbLibraryServicePane_onLibraryRegistered(aLibrary) {
  logcall(arguments);
  this._libraryAdded(aLibrary);
}
sbLibraryServicePane.prototype.onLibraryUnregistered =
function sbLibraryServicePane_onLibraryUnregistered(aLibrary) {
  logcall(arguments);
  this._libraryRemoved(aLibrary);
}

//////////////////////////
// sbIMediaListListener //
//////////////////////////

sbLibraryServicePane.prototype.onItemAdded =
function sbLibraryServicePane_onItemAdded(aMediaList, aMediaItem) {
  logcall(arguments);
  var isList = aMediaItem instanceof Ci.sbIMediaList;
  if (isList) {
    this._playlistAdded(aMediaItem);
  }
}
sbLibraryServicePane.prototype.onBeforeItemRemoved =
function sbLibraryServicePane_onBeforeItemRemoved(aMediaList, aMediaItem) {
  logcall(arguments);
  var isList = aMediaItem instanceof Ci.sbIMediaList;
  if (isList) {
    this._playlistRemoved(aMediaItem);
  }
}
sbLibraryServicePane.prototype.onAfterItemRemoved =
function sbLibraryServicePane_onAfterItemRemoved(aMediaList, aMediaItem) {
}
sbLibraryServicePane.prototype.onItemUpdated =
function sbLibraryServicePane_onItemUpdated(aMediaList, aMediaItem) {
  var isList = aMediaItem instanceof Ci.sbIMediaList;
  if (isList) {
    this._playlistUpdated(aMediaItem);
  }
}
sbLibraryServicePane.prototype.onListCleared =
function sbLibraryServicePane_onListCleared(aMediaList) {
}
sbLibraryServicePane.prototype.onBatchBegin =
function sbLibraryServicePane_onBatchBegin(aMediaList) {
  logcall(arguments);
}
sbLibraryServicePane.prototype.onBatchEnd =
function sbLibraryServicePane_onBatchEnd(aMediaList) {
  logcall(arguments);
}

/////////////////
// nsIObserver //
/////////////////

sbLibraryServicePane.prototype.observe = 
function sbLibraryServicePane_observe(subject, topic, data) {
  if (topic == "songbird-library-manager-ready") {
    var obs = Cc["@mozilla.org/observer-service;1"].
              getService(Ci.nsIObserverService);
    obs.removeObserver(this, "songbird-library-manager-ready");
    
    // get the library manager
    this._libraryManager =
        Components.classes['@songbirdnest.com/Songbird/library/Manager;1']
        .getService(Components.interfaces.sbILibraryManager);
    
    // register for notifications so that we can keep the service pane
    // in sync with the the libraries
    this._libraryManager.addListener(this);
    
    // FIXME: remove listener on destruction?  
    // Shouldn't have to once we get weak ref support in the library system
    // Show all existing libraries and playlists
    this._processLibraries();        
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
