/** vim: ts=4 sw=4 expandtab
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
const FOLDER_IMAGE = 'chrome://songbird/skin/icons/icon_folder.png';
const BOOKMARK_IMAGE = 'chrome://service-icons/skin/default.ico';


function SB_NewDataRemote(a,b) {
  return (new Components.Constructor("@songbirdnest.com/Songbird/DataRemote;1",
      "sbIDataRemote", "init"))(a,b);
}



function sbLibraryServicePane() {
  this._servicePane = null;
  this._libraryManager = null;
}
sbLibraryServicePane.prototype.QueryInterface = 
function sbLibraryServicePane_QueryInterface(iid) {
  if (!iid.equals(Ci.nsISupports) &&
    !iid.equals(Ci.sbIServicePaneModule) &&
    !iid.equals(Ci.sbILibraryManagerListener)) {
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
  return this;
}
sbLibraryServicePane.prototype.servicePaneInit = 
function sbLibraryServicePane_servicePaneInit(sps) {
  // keep track of the service pane service
  this._servicePane = sps;
  
  // get the library manager
  this._libraryManager =
      Components.classes['@songbirdnest.com/Songbird/library/Manager;1']
      .getService(Components.interfaces.sbILibraryManager);
  
  this._libraryManager.addListener(this);
  // FIXME: remove listener on destruction?
  
  var libraries = this._libraryManager.getLibraryEnumerator();
  while (libraries.hasMoreElements()) {
    var library = libraries.getNext();
    library.QueryInterface(Components.interfaces.sbILibrary);
    dump (' L: '+library+'\n');
    this._ensureLibraryNodeExists(library);
  }
}
sbLibraryServicePane.prototype.fillContextMenu =
function sbLibraryServicePane_fillContextMenu(aNode, aContextMenu, aParentWindow) {
}
sbLibraryServicePane.prototype._itemURN =
function sbLibraryServicePane__itemURN(aMediaItem) {
  // for a given media item get an urn we can use
  return 'urn:item:'+aMediaItem.guid;
}
sbLibraryServicePane.prototype._libraryURN =
function sbLibraryServicePane__libraryURN(aLibrary) {
  // for a given library object construct the urn to use internally in the
  // service pane
  return 'urn:library:'+aLibrary.guid;
}
sbLibraryServicePane.prototype._ensureLibraryNodeExists =
function sbLibraryServicePane__ensureLibraryNodeExists(aLibrary) {
  var id = this._libraryURN(aLibrary);
  dump ('ensureLibraryNodeExists: '+id+'\n');
  var node = this._servicePane.getNode(id);
  if (!node) {
    // create the node
    node = this._servicePane.addNode(id, this._servicePane.root, true);
    node.url = 'chrome://songbird/content/xul/playlist_test2.xul?library,'+aLibrary.guid;
    node.name = aLibrary.guid;
    node.image = 'chrome://songbird/skin/icons/icon_lib_16x16.png';
    node.hidden = false;
    node.contractid = CONTRACTID;
    node.hideURL = true;
    
    // we want to put it above the first non-library item
    // yes this rule is kind of arbitary.
    var childs = this._servicePane.root.childNodes;
    while (childs.hasMoreElements()) {
      var child = childs.getNext().QueryInterface(Ci.sbIServicePaneNode);
      if (child.contractid != CONTRACTID) {
        this._servicePane.root.insertBefore(node, child);
        break;
      }
    }
  }
  
  var e = {
    onEnumerationBegin: function(aMediaList) { return true; },
    onEnumerationEnd: function(aMediaList, aResultCode) { },
    onEnumeratedItem: function(aMediaList, aMediaItem) {
      dump('media item: '+aMediaItem+'\n');
      var is_list = aMediaItem instanceof Ci.sbIMediaList;
      dump(' is list: '+is_list+'\n');
      return true;
    }
  };
  // we need to treat the library like a media list for a moment
  aLibrary.QueryInterface(Ci.sbIMediaList);
  // we need to go through all the items and make sure we've got all the
  // required playlist service pane nodes.
  aLibrary.enumerateAllItems(e, Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);
  // then we need to know about any changes that will affect us
  aLibrary.addListener(this);
  
  return node;
}
// sbILibraryManagerListener
sbLibraryServicePane.prototype.onLibraryRegistered =
function sbLibraryServicePane_onLibraryRegistered(aLibrary) {
  this._ensureLibraryNodeExists(aLibrary);
}
sbLibraryServicePane.prototype.onLibraryUnregistered =
function sbLibraryServicePane_onLibraryUnregistered(aLibrary) {
  dump('onLibraryUnregistered\n');
}
// sbIMediaListListener
sbLibraryServicePane.prototype.onItemAdded =
function sbLibraryServicePane_onItemAdded(aMediaList, aMediaItem) {
  dump('onItemAdded: '+aMediaItem+'\n');
}
sbLibraryServicePane.prototype.onItemRemoved =
function sbLibraryServicePane_onItemRemoved(aMediaList, aMediaItem) {
}
sbLibraryServicePane.prototype.onItemUpdated =
function sbLibraryServicePane_onItemUpdated(aMediaList, aMediaItem) {
}
sbLibraryServicePane.prototype.onListCleared =
function sbLibraryServicePane_onListCleared(aMediaList) {
}
sbLibraryServicePane.prototype.onBatchBegin =
function sbLibraryServicePane_onBatchBegin(aMediaList) {
}
sbLibraryServicePane.prototype.onBatchEnd =
function sbLibraryServicePane_onBatchEnd(aMediaList) {
}

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
