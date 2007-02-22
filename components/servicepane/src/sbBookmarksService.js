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
 *    per-locale bookmarks?
 *  - download and cache images
 *  - add a confirmation dialog for for bookmark deletion
 *  
 */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const CONTRACTID = "@songbirdnest.com/servicepane/bookmarks;1"
const ROOTNODE = "SB:Bookmarks"
const FOLDER_IMAGE = 'chrome://songbird/skin/default/icon_folder.png';
const BOOKMARK_IMAGE = 'chrome://songbird/skin/serviceicons/default.ico';


function SB_NewDataRemote(a,b) {
    return (new Components.Constructor("@songbirdnest.com/Songbird/DataRemote;1",
                                      "sbIDataRemote", "init"))(a,b);
}



function sbBookmarks() {
    this._servicePane = null;
    this._stringBundle = null;
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
    
    var service = this;
    
    // if we don't have a bookmarks node in the tree
    this._bookmarkNode = this._servicePane.getNode(ROOTNODE);
    if (!this._bookmarkNode) {
        // fetch the default set of bookmarks through a series of tubes
        // FIXME: don't use XHR - use nsIChannel and friends
        // FIXME: send parameters and/or headers to indicate product version or something
        var xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                .createInstance(Ci.nsIDOMEventTarget);
        xhr.addEventListener('load', function(evt) {
            var root = xhr.responseXML.documentElement;
            var folders = root.getElementsByTagName('folder');
            for (var f=0; f<folders.length; f++) {
                var folder = folders[f];
                if (!folder.hasAttribute('id') ||
                        !folder.hasAttribute('name')) {
                    // the folder is missing required attributes, we must ignore it
                    continue;
                }
                // if this folder already exists, skip.
                // we'll want to change this behaviour later to support removals
                // see bug #2352
                var fnode = sps.getNode(folder.getAttribute('id'));
                if (fnode) {
                    // the folder exists, skip it
                    continue;
                }
                // create the folder
                fnode = sps.addNode(folder.getAttribute('id'), sps.root, true);
                fnode.name = folder.getAttribute('name');
                // attach an image
                if (folder.hasAttribute('image')) {
                    // if an icon was supplied use it
                    fnode.image = folder.getAttribute('image');
                } else {
                    // otherwise use a default
                    fnode.image = FOLDER_IMAGE;
                }
                fnode.hidden = false;
                fnode.contractid = CONTRACTID;
                
                if (fnode.id == ROOTNODE) {
                    // we just created the default bookmarks root
                    // we'll need that later if we want to let the user create
                    // their own bookmarks
                    service._bookmarkNode = fnode;
                }
                
                // now, let's create what goes in
                var bookmarks = folder.getElementsByTagName('bookmark');
                for (var b=0; b<bookmarks.length; b++) {
                    var bookmark = bookmarks[b];
                    if (!bookmark.hasAttribute('url') ||
                            !bookmark.hasAttribute('name')) {
                        // missing required attributes
                        continue;
                    }
                    // If the bookmark already exists, then it's somewhere else
                    // in the tree and we should leave it there untouched.
                    // I think.
                    var bnode = sps.getNode(bookmark.getAttribute('url'));
                    if (bnode) {
                        continue;
                    }
                    // create the bookmark
                    bnode = sps.addNode(bookmark.getAttribute('url'), fnode, false);
                    bnode.url = bookmark.getAttribute('url');
                    bnode.name = bookmark.getAttribute('name');
                    bnode.image = bookmark.hasAttribute('image')?
                            bookmark.getAttribute('image') : BOOKMARK_IMAGE;
                    bnode.hidden = false;
                    bnode.contractid = CONTRACTID;
                }
            }
            sps.save();
        }, false);
        xhr.addEventListener('error', function(evt) {
            // FIXME: handle errors. if we do nothing here then the user won't
            //        have bookmarks, but when they start the app again it
            //        will try to fetch them again.
        }, false);
        xhr.QueryInterface(Ci.nsIXMLHttpRequest);
        xhr.open('GET', 'http://ian.mckellar.org/bookmarks.xml', true);
        xhr.send(null);
    }
    
    var sbSvc = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService);
    this._stringBundle = sbSvc.createBundle("chrome://songbird/locale/songbird.properties");
}
sbBookmarks.prototype.getString =
function sbBookmarks_getString(aStringId, aDefault) {
    try {
        return this._stringBundle.GetStringFromName(aStringId);
    } catch (e) {
        return aDefault;
    }
}
sbBookmarks.prototype.addBookmark =
function sbBookmarks_addBookmark(aURL, aTitle, aIconURL) {
    dump('sbBookmarks.addBookmark('+aURL.toSource()+','+aTitle.toSource()+','+aIconURL.toSource()+')\n');
    var node = this._servicePane.addNode(aURL, this._bookmarkNode, false);
    dump ('addBookmark: addNode returned: '+node+'\n');
    if (node) {
        node.contractid = CONTRACTID;
        node.name = aTitle;
        node.url = aURL;
        /* FIXME: the should the icon be downloaded and cached as a data: url */
        node.image = aIconURL;
        
        node.hidden = false;
        
        this._servicePane.save();
        
        // check that the supplied image url works, otherwise use the default
        var observer = {
            service : null,
            onStartRequest : function(aRequest,aContext) {
            },
            onStopRequest : function(aRequest, aContext, aStatusCode) {
                if (aStatusCode != 0) {
                    node.icon = BOOKMARK_ICON;
                    this.service._servicePane.save();
                }
            }
        };
        observer.manager = this;
        var checker = Components.classes["@mozilla.org/network/urichecker;1"]
            .createInstance(Components.interfaces.nsIURIChecker);
        var uri = Components.classes["@mozilla.org/network/standard-url;1"]
            .createInstance(Components.interfaces.nsIURI);
        uri.spec = aIconURL;
        checker.init(uri);
        checker.asyncCheck(observer, null);
    }
    
    return node;
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
        /* begin code stolen from window_utils.js:SBOpenModalDialog */
        /* FIXME: what's that BackscanPause stuff? */
        var chromeFeatures = "chrome,centerscreen,modal=yes,resizable=no";
        var accessibility = SB_NewDataRemote('accessibility.enabled', null).boolValue;
        chromeFeatures += (',titlebar='+accessibility?'yes':'no');
        aParentWindow.openDialog('chrome://songbird/content/xul/editbookmark.xul',
                                 'edit_bookmark', chromeFeatures, aNode);

        service._servicePane.save();    
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
        service._servicePane.save();
    }, false);
    aContextMenu.appendChild(item);
    
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

