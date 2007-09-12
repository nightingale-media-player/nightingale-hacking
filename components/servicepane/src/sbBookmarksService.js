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
 * \file sbBookmarks.js
 * \brief the service pane service manages the tree behind the service pane.
 * It provides an interface for creating bookmark nodes in the service pane tree.
 *
 * TODO:
 *  - move downloading to nsIChannel and friends for better chrome-proofing
 *  - send version and locale info when requesting new bookmarks
 *  - handle errors more elegantly in bookmarks downloading
 *  - handle the adding of bookmarks before the defaults are downloaded
 *  - allow the server bookmarks list to remove stale entries [bug 2352]
 *  - move the bookmarks.xml from ian.mckellar.org to songbirdnest.com
 * PERHAPS:
 *  - move default bookmarks downloading to after first-run so we can have
 *  per-locale bookmarks?
 *  - download and cache images
 *  - add a confirmation dialog for for bookmark deletion
 *  
 */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const CONTRACTID = "@songbirdnest.com/servicepane/bookmarks;1"
const ROOTNODE = "SB:Bookmarks"
const BOOKMARK_DRAG_TYPE = 'text/x-sb-bookmark';
const MOZ_URL_DRAG_TYPE = 'text/x-moz-url';
const BSP = 'http://songbirdnest.com/rdf/bookmarks#';

function SB_NewDataRemote(a,b) {
  return (new Components.Constructor("@songbirdnest.com/Songbird/DataRemote;1",
                    "sbIDataRemote", "init"))(a,b);
}



function sbBookmarks() {
  this._servicePane = null;
  this._stringBundle = null;
  
  // use the default stringbundle to translate tree nodes
  this.stringbundle = null;

  this._importAttempts = 5;
  this._importTimer = null;
}
sbBookmarks.prototype.QueryInterface = 
function sbBookmarks_QueryInterface(iid) {
  if (!iid.equals(Ci.nsISupports) &&
    !iid.equals(Ci.sbIBookmarks) &&
    !iid.equals(Ci.sbIServicePaneModule)) {
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
  return this;
}
sbBookmarks.prototype.servicePaneInit = 
function sbBookmarks_servicePaneInit(sps) {
  this._servicePane = sps;
  
  // if we don't have a bookmarks node in the tree that has the Imported
  // attribute...
  this._bookmarkNode = this._servicePane.getNode(ROOTNODE);
  if (!this._bookmarkNode ||
      this._bookmarkNode.getAttributeNS(BSP, 'Imported') != 'true') {
    // run the importer
    this.importBookmarks();
  }

  var sbSvc = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService);
  this._stringBundle = sbSvc.createBundle("chrome://songbird/locale/songbird.properties");
}

sbBookmarks.prototype.scheduleImportBookmarks =
function sbBookmarks_scheduleImportBookmarks() {
  this._importTimer = Cc['@mozilla.org/timer;1'].createInstance(Ci.nsITimer);
  this._importTimer.init(this, 5000, Ci.nsITimer.TYPE_ONE_SHOT);
}

sbBookmarks.prototype.observe = 
function sbBookmarks_observe(subject, topic, data) {
  if (topic == 'timer-callback' && subject == this._importTimer) {
    // the bookmarks import timer
    this._importTimer = null;
    if (!this._servicePane) {
      // hmm, we're shutting down, no need to import now
      return;
    }
    this.importBookmarks();
  }
}

sbBookmarks.prototype.importBookmarks =
function sbBookmarks_importBookmarks() {
  var prefsService =
      Components.classes["@mozilla.org/preferences-service;1"].
      getService(Components.interfaces.nsIPrefBranch);
  var bookmarksURL = prefsService.getCharPref("songbird.url.bookmarks");
  
  // fetch the default set of bookmarks through a series of tubes
  // FIXME: don't use XHR - use nsIChannel and friends
  // FIXME: send parameters and/or headers to indicate product version or something
  var xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
      .createInstance(Ci.nsIDOMEventTarget);
  var sps = this._servicePane;
  var service = this;
  function importError() {
    service._importAttempts--;
    if (service._importAttempts < 0) {
      // we tried, but it's time to give up till the next time the player starts
      // but first, let's create some default bookmarks to get us through
      if (!service._bookmarkNode) {
        // create bookmarks folder
        service._bookmarkNode = service.addFolderAt('SB:Bookmarks',
            '&servicesource.bookmarks', null, sps.root, null);
      }
      var default_bookmarks = [
        {name:'Add-ons', url:'http://addons.songbirdnest.com/'},
        {name:'Directory', url:'http://birdhouse.songbirdnest.com/directory'}];
      for (var i=0; i<default_bookmarks.length; i++) {
        var bm = default_bookmarks[i];
        var bnode = sps.getNode(bm.url);
        if (!bnode) {
          service.addBookmarkAt(bm.url, bm.name, 
            'chrome://songbird-branding/skin/logo_16.png', 
            service._bookmarkNode, null);
        }
      }
      
      return;
    }
    service.scheduleImportBookmarks();
  }
  xhr.addEventListener('load', function(evt) {
    var root = null;
    try {
      // a non-XML page will cause an exception here.
      root = xhr.responseXML.documentElement;
    } catch (e) {
      // catch it and try again
      importError();
      return;
    }
    var folders = root.getElementsByTagName('folder');
    for (var f=0; folders && f<folders.length; f++) {
      var folder = folders[f];
      if (!folder.hasAttribute('id') ||
          !folder.hasAttribute('name')) {
        // the folder is missing required attributes, we must ignore it
        continue;
      }

      var fnode = sps.getNode(folder.getAttribute('id'));
      
      if (!fnode) {
        // if the folder doesn't exist, create it
        fnode = service.addFolderAt(folder.getAttribute('id'),
            folder.getAttribute('name'), folder.getAttribute('image'),
            sps.root, null);
      }
      
      fnode.isOpen = (folder.getAttribute('open') == 'true');
      
      if (fnode && fnode.getAttributeNS(BSP, 'Imported')) {
        // don't reimport a folder that's already been imported
        continue;
      }
      
      if (fnode.id == ROOTNODE) {
        // we just created the default bookmarks root
        // we'll need that later if we want to let the user create
        // their own bookmarks
        service._bookmarkNode = fnode;
      }
      
      // now, let's create what goes in
      var bookmarks = folder.getElementsByTagName('bookmark');
      for (var b=0; bookmarks && b<bookmarks.length; b++) {
        var bookmark = bookmarks[b];
        if (!bookmark.hasAttribute('url') ||
            !bookmark.hasAttribute('name')) {
          // missing required attributes
          continue;
        }
        // If the bookmark already exists, then it's somewhere else
        // in the tree and we should leave it there untouched.
        // Except that it should be marked as imported now...
        var bnode = sps.getNode(bookmark.getAttribute('url'));
        if (!bnode) {
          // create the bookmark
          bnode = service.addBookmarkAt(bookmark.getAttribute('url'),
              bookmark.getAttribute('name'), bookmark.getAttribute('image'),
              fnode, null);
        }
        // remember we imported it.
        bnode.setAttributeNS(BSP, 'Imported', 'true');
      }
      
      fnode.setAttributeNS(BSP, 'Imported', 'true');
    }
    
    // try to import json bookmarks from 0.2.5
    try {
      service.migrateLegacyBookmarks();
    } catch (e) {
    }

  }, false);
  xhr.addEventListener('error', function(evt) {
    importError();
  }, false);
  xhr.QueryInterface(Ci.nsIXMLHttpRequest);
  xhr.open('GET', bookmarksURL, true);
  xhr.send(null);
}

sbBookmarks.prototype.shutdown = 
function sbBookmarks_shutdown() {
  this._bookmarkNode = null;
  this._servicePane = null;
  this._stringBundle = null;
  if (this._importTimer) {
    this._importTimer.cancel();
    this._importTimer = null;
  }
}

sbBookmarks.prototype.getString =
function sbBookmarks_getString(aStringId, aDefault) {
  try {
    return this._stringBundle.GetStringFromName(aStringId);
  } catch (e) {
    return aDefault;
  }
}
sbBookmarks.prototype.migrateLegacyBookmarks =
function sbBookmarks_migrateLegacyBookmarks() {
  try {
    var prefsService =
        Components.classes["@mozilla.org/preferences-service;1"].
        getService(Components.interfaces.nsIPrefBranch);
    var LEGACY_BOOKMARKS_PREF = 'songbird.bookmarks.serializedTree';
    if (prefsService.prefHasUserValue(LEGACY_BOOKMARKS_PREF)) {
      var json = '(' + prefsService.getCharPref(LEGACY_BOOKMARKS_PREF) + ')';
      var bms = eval(json);
      for (var i in bms.children) {
        var folder = bms.children[i];
        if (folder.label == '&servicesource.bookmarks') {
          for (var j in folder.children) {
            var bm = folder.children[j];
            if (bm.properties != 'bookmark') {
              continue;
            }
            var node = this._servicePane.getNode(bm.url);
            if (node) {
              // this bookmark already existed.
              // we want to set the title to the old
              node.name = bm.label;
            } else {
              // the bookmark does not exist. We need to create it
              var icon = null;
              // only import the icon if its from the web
              if (bm.icon.match(/^http/)) {
                icon = bm.icon;
              }
              this.addBookmark(bm.url, bm.label, icon);
            }
          }
          break;
        }
      }
      // let's clear that pref
      prefsService.clearUserPref(LEGACY_BOOKMARKS_PREF);
    }
  } catch (e) {
  }
}

sbBookmarks.prototype.addBookmark =
function sbBookmarks_addBookmark(aURL, aTitle, aIconURL) {
  dump('sbBookmarks.addBookmark('+aURL.toSource()+','+aTitle.toSource()+','+aIconURL.toSource()+')\n');
  if (!this._bookmarkNode) {
    // if we try to add a bookmark the defaults are loaded, lets create the default folder anyway
    this._bookmarkNode = this._servicePane.addNode(ROOTNODE,
        this._servicePane.root, true);
  }
  return this.addBookmarkAt(aURL, aTitle, aIconURL, this._bookmarkNode, null);
}

sbBookmarks.prototype.addBookmarkAt =
function sbBookmarks_addBookmarkAt(aURL, aTitle, aIconURL, aParent, aBefore) {
  var bnode = this._servicePane.addNode(aURL, aParent, false);
  if (!bnode) {
    return bnode;
  }
  
  bnode.url = aURL;
  bnode.name = aTitle;
  if (aBefore) {
    aBefore.parentNode.insertBefore(bnode, aBefore);
  }
  bnode.properties = "bookmark " + aTitle;
  bnode.hidden = false;
  bnode.contractid = CONTRACTID;
  bnode.dndDragTypes = BOOKMARK_DRAG_TYPE;
  bnode.dndAcceptNear = BOOKMARK_DRAG_TYPE;
  bnode.editable = true;
  
  if (aIconURL && aIconURL.match(/^https?:/)) {
    // check that the supplied image url works, otherwise use the default
    var checker = Components.classes["@mozilla.org/network/urichecker;1"]
      .createInstance(Components.interfaces.nsIURIChecker);
    var uri = Components.classes["@mozilla.org/network/standard-url;1"]
      .createInstance(Components.interfaces.nsIURI);
    uri.spec = aIconURL;
    checker.init(uri);
    checker.asyncCheck(new ImageUriCheckerObserver(bnode, aIconURL), null);
  }

  return bnode;
}

function ImageUriCheckerObserver(bnode, icon) {
  this._bnode = bnode;
  this._icon = icon;
}
ImageUriCheckerObserver.prototype.onStartRequest =
function ImageUriCheckerObserver_onStartRequest(aRequest, aContext)
{
}
ImageUriCheckerObserver.prototype.onStopRequest =
function ImageUriCheckerObserver_onStopRequest(aRequest, aContext, aStatusCode)
{
  // If the requested image exists, set it as the icon.
  if (aStatusCode == 0) {
    this._bnode.image = this._icon;
  }
  // Otherwise, we don't set the image property and we get the default from the skin.
}

sbBookmarks.prototype.addFolder =
function sbBookmarks_addFolder(aTitle) {
  return this.addFolderAt(null, aTitle, null, this._servicePane.root, null);
}

sbBookmarks.prototype.addFolderAt =
function sbBookmarks_addFolderAt(aId, aTitle, aIconURL, aParent, aBefore) {
  var  fnode = this._servicePane.addNode(aId, aParent, true);  
  fnode.name = aTitle;
  if (aIconURL != null) {
    fnode.image = aIconURL;
  }
  if (aBefore) {
    aBefore.parentNode.insertBefore(fnode, aBefore);
  }
  fnode.properties = "folder " + aTitle;
  fnode.hidden = false;
  fnode.contractid = CONTRACTID;
  fnode.dndDragTypes = 'text/x-sb-toplevel';
  fnode.dndAcceptNear = 'text/x-sb-toplevel';
  fnode.dndAcceptIn = BOOKMARK_DRAG_TYPE;
  fnode.editable = true; // folder names are editable
  
  return fnode;
}

sbBookmarks.prototype.bookmarkExists =
function sbBookmarks_bookmarkExists(aURL) {
  var node = this._servicePane.getNode(aURL);
  return (node != null);
}
sbBookmarks.prototype.fillContextMenu =
function sbBookmarks_fillContextMenu(aNode, aContextMenu, aParentWindow) {
  dump ('called fillContextMenu with node: '+aNode+'\n');
  
  if (aNode.contractid != CONTRACTID) {
    dump('or not...('+aNode.contractid+')\n');
    return;
  }
    
  var document = aContextMenu.ownerDocument;
    
  var item = document.createElement('menuitem');
  item.setAttribute('label',
            this.getString('bookmark.menu.edit', 'Edit'));
  var service = this; // so we can get to it from the callback
  item.addEventListener('command',
  function bookmark_edit_oncommand (event) {
    dump ('edit properties of: '+aNode.name+'\n');
    aParentWindow.QueryInterface(Ci.nsIDOMWindowInternal);
    /* begin code stolen from windowUtils.js:SBOpenModalDialog */
    /* FIXME: what's that BackscanPause stuff? */
    var chromeFeatures = "chrome,centerscreen,modal=yes,resizable=no";
    var accessibility = SB_NewDataRemote('accessibility.enabled', null).boolValue;
    chromeFeatures += (',titlebar='+accessibility?'yes':'no');
    aParentWindow.openDialog('chrome://songbird/content/xul/editBookmark.xul',
                 'edit_bookmark', chromeFeatures, aNode);
  }, false);
  aContextMenu.appendChild(item);
  
  item = document.createElement('menuitem');
  item.setAttribute('label',
            this.getString('bookmark.menu.remove', 'Remove'));
  item.addEventListener('command',
  function bookmark_delete_oncommand (event) {
    dump ('delete: '+aNode.name+'\n');
    // FIXME: confirmation dialog, eh??
    service._servicePane.removeNode(aNode);
  }, false);
  aContextMenu.appendChild(item);
}

sbBookmarks.prototype.fillNewItemMenu =
function sbBookmarks_fillNewItemMenu(aNode, aContextMenu, aParentWindow) {
  var stringBundle = this._stringBundle;
  function add(id, label, accesskey, oncommand) {
    var menuitem = aContextMenu.ownerDocument.createElement('menuitem');
    menuitem.setAttribute('id', id);
    menuitem.setAttribute('class', 'menuitem-iconic');
    menuitem.setAttribute('label', stringBundle.GetStringFromName(label));
    menuitem.setAttribute('accesskey', stringBundle.GetStringFromName(accesskey));
    menuitem.setAttribute('oncommand', oncommand);
    aContextMenu.appendChild(menuitem);
  }

  add('file.folder', 'menu.file.folder', 'menu.file.folder.accesskey', 'doMenu("file.folder")');
}

sbBookmarks.prototype.onSelectionChanged=
function sbBookmarks_onSelectionChanged(aNode, aContainer, aParentWindow) {
}

sbBookmarks.prototype.canDrop =
function sbBookmarks_canDrop(aNode, aDragSession, aOrientation) {
  if (aNode.isContainer) {
    if (aOrientation != 0) {
      // for folders you can only drop on the folder, not before or after
      return false;
    }
  }
  if (aDragSession.isDataFlavorSupported(MOZ_URL_DRAG_TYPE)) {
    return true;

  }
  return false;
}
sbBookmarks.prototype._getDragData =
function sbBookmarks__getDragData(aDragSession, aDataType) {
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
  return data.toString();
}
sbBookmarks.prototype.onDrop =
function sbBookmarks_onDrop(aNode, aDragSession, aOrientation) {
  if (aDragSession.isDataFlavorSupported(MOZ_URL_DRAG_TYPE)) {
    var data = this._getDragData(aDragSession, MOZ_URL_DRAG_TYPE).split('\n');
    var url = data[0];
    var text = data[1];
    var parent, before=null;
    if (aNode.isContainer) {
      parent = aNode;
    } else {
      parent = aNode.parentNode;
      before = aNode;
      if (aOrientation == 1) {
        // after
        before = aNode.nextSibling;
      }
    }
    this.addBookmarkAt(url, text, null, parent, before);
  }
}
sbBookmarks.prototype._addDragData =
function sbBookmarks__addDragData (aTransferable, aData, aDataType) {
  aTransferable.addDataFlavor(aDataType);
  var text = Components.classes["@mozilla.org/supports-string;1"].
     createInstance(Components.interfaces.nsISupportsString);
  text.data = aData;
  // double the length - it's unicode - this is stupid
  aTransferable.setTransferData(aDataType, text, text.data.length*2);
}
sbBookmarks.prototype.onDragGesture =
function sbBookmarks_onDragGesture(aNode, aTransferable) {
  if (aNode.isContainer) {
    // you can't drag folders - that should be handled at the
    // service pane service layer
    return false;
  }
  
  // attach a text/x-moz-url
  // this is for dragging to other places
  this._addDragData(aTransferable, aNode.url+'\n'+aNode.name, MOZ_URL_DRAG_TYPE);
  
  // and say yet - lets do this drag
  return true;
}

/**
 * Called when the user has attempted to rename a bookmark node
 */
sbBookmarks.prototype.onRename =
function sbBookmarks_onRename(aNode, aNewName) {
  if (aNode && aNewName) {
    aNode.name = aNewName;
  }
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
  sbBookmarks,
  Components.ID("{21e3e540-1191-46b5-a041-d0ab95d4c067}"),
  "Songbird Bookmarks Service",
  CONTRACTID,
  [{
    category: 'service-pane',
    entry: 'bookmarks',
    value: '@songbirdnest.com/servicepane/bookmarks;1'
  }]);

