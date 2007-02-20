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
 *  - make the "Bookmarks" name localizable
 *  - load default bookmarks off the internet, somehow
 *  - download and cache images
 *  
 */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const CONTRACTID = "@songbirdnest.com/servicepane/bookmarks;1"
const ROOTNODE = "SB:Bookmarks"

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
    this._bookmarkNode = this._servicePane.getNode(ROOTNODE);
    if (!this._bookmarkNode) {
        this._bookmarkNode = this._servicePane.addNode(ROOTNODE, sps.root, true);
        this._bookmarkNode.hidden = false;
        this._bookmarkNode.name = 'Bookmarks'; // FIXME: get from locale?
        
        // FIXME: ADD DEFAULT BOOKMARKS FROM THE INTERNETS
        
        // for the time being, lets just add a few hardcoded bookmarks
        this.addBookmark('http://www.songbirdnest.com/', 'Songbird', 'http://www.songbirdnest.com/favicon.ico');
        this.addBookmark('http://www.google.com/', 'Google', 'http://www.google.com/favicon.ico');
        this.addBookmark('http://www.creativecommons.org/', 'Creative Commons', 'http://www.creativecommons.org/favicon.ico');
        
        sps.save();
    }
    dump ('this._bookmarksNode.name='+this._bookmarkNode.name+'\n');
    
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
        /* FIXME: the icon should be downloaded and cached as a data: url */
        node.image = aIconURL;
        
        node.hidden = false;
        
        this._servicePane.save();
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
    item.addEventListener('command',
    function bookmark_menu_oncommand (event) {
        dump ('edit properties of: '+aNode.name+'\n');
        var data = {};
        data.in_url = aNode.url;
        data.in_label = aNode.label;
        data.in_icon= aNode.icon;
        aParentWindow.QueryInterface(Ci.nsIDOMWindowInternal);
        /* begin code stolen from window_utils.js:SBOpenModalDialog */
        /* FIXME: what's that BackscanPause stuff? */
        var chromeFeatures = "chrome,centerscreen,modal=yes,resizable=no";
        var accessibility = SB_NewDataRemote('accessibility.enabled', null).boolValue;
        chromeFeatures += (',titlebar='+accessibility?'yes':'no');
        aParentWindow.openDialog('chrome://songbird/content/xul/editbookmark.xul',
                                 'edit_bookmark', chromeFeatures, data);
        /* end stolen code */
        if (data.result) {
            aNode.url = data.out_url;
            aNode.label = data.out_icon;
            aNode.icon = data.out_label;
            this._servicePane.save();
      }         
    }, false);
    aContextMenu.appendChild(item);
    
    item = document.createElement('menuitem');
    item.setAttribute('label',
                      this.getString('bookmark.menu.remove', 'Remove'));
    item.addEventListener('command', function (event) {
        dump ('delete: '+aNode.name+'\n');
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

