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
 * \file ServicePaneService.js
 * \brief the service pane service manages the tree behind the service pane
 *
 * TODO:
 *  - implement lazy saving of dirty datasources.
 *  - handle module removal
 *  
 */

function DEBUG(msg) {
    dump('sbServicePaneService.js: '+msg+'\n');
}

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ce = Components.Exception;

const RDFSVC = Cc['@mozilla.org/rdf/rdf-service;1'].getService(Ci.nsIRDFService);
const RDFCU = Cc['@mozilla.org/rdf/container-utils;1'].getService(Ci.nsIRDFContainerUtils);
const IOSVC = Cc['@mozilla.org/network/io-service;1'].getService(Ci.nsIIOService);

const NC='http://home.netscape.com/NC-rdf#';
const SP='http://songbirdnest.com/rdf/servicepane#';

function ServicePaneNode (ds, resource) {
    this.__defineGetter__('resource', function () { return resource });
    
    this._dataSource = ds;
    this._dataSource.QueryInterface(Ci.nsIRDFRemoteDataSource);
    
    // set up this._container if this is a Seq
    if (RDFCU.IsSeq(ds, resource)) {
        this._container = Cc['@mozilla.org/rdf/container;1'].createInstance(Ci.nsIRDFContainer);
        this._container.Init(ds, resource);
    } else {
        this._container = null;
    }
}

ServicePaneNode.prototype.QueryInterface = function (iid) {
    if (!iid.equals(Ci.sbIServicePaneNode) &&
        !iid.equals(Ci.sbIServicePaneNodeInternal) &&
        !iid.equals(Ci.nsIClassInfo) &&
        !iid.equals(Ci.nsISupports)) {
        throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
}
ServicePaneNode.prototype.classDescription = 'Songbird Service Pane Node';
ServicePaneNode.prototype.classID = null;
ServicePaneNode.prototype.contractID = null;
ServicePaneNode.prototype.flags = Ci.nsIClassInfo.MAIN_THREAD_ONLY;
ServicePaneNode.prototype.implementationLanguage = Ci.nsIProgrammingLanguage.JAVASCRIPT;
ServicePaneNode.prototype.getHelperForLanguage = function(aLanguage) { return null; }
ServicePaneNode.prototype.getInterfaces = function(count) {
    var interfaces = [Ci.sbIServicePaneNode,
                      Ci.sbIServicePaneNodeInternal,
                      Ci.nsIClassInfo,
                      Ci.nsISupports,
                     ];
    count.value = interfaces.length;
    return interfaces;
}


ServicePaneNode.prototype.__defineGetter__ ('id',
        function () { return this.resource.ValueUTF8 });

ServicePaneNode.prototype.__defineGetter__ ('isContainer',
        function () { return this._container != null });

ServicePaneNode.prototype.setAttributeNS = function (aNamespace, aName, aValue) {
    var property = RDFSVC.GetResource(aNamespace+aName);
    var target = this._dataSource.GetTarget(this.resource, property, true);
    if (target) {
        this._dataSource.Unassert(this.resource, property, target);
    }
    if (aValue != null) {
        this._dataSource.Assert(this.resource, property,
                                RDFSVC.GetLiteral(aValue), true);
    }
}

ServicePaneNode.prototype.getAttributeNS = function (aNamespace, aName) {
    var property = RDFSVC.GetResource(aNamespace+aName);
    var target = this._dataSource.GetTarget(this.resource, property, true);
    if (target) {
        var value = target.QueryInterface(Ci.nsIRDFLiteral).Value
        return value;
    } else {
        return null;
    }
}

ServicePaneNode.prototype.__defineGetter__ ('url', function () {
    return this.getAttributeNS(NC,'URL'); })
ServicePaneNode.prototype.__defineSetter__ ('url', function (aValue) {
    this.setAttributeNS(NC,'URL', aValue); });

ServicePaneNode.prototype.__defineGetter__ ('image', function () {
    return this.getAttributeNS(NC,'Icon'); })
ServicePaneNode.prototype.__defineSetter__ ('image', function (aValue) {
    this.setAttributeNS(NC,'Icon', aValue); });

ServicePaneNode.prototype.__defineGetter__ ('name', function () {
    return this.getAttributeNS(NC,'Name'); })
ServicePaneNode.prototype.__defineSetter__ ('name', function (aValue) {
    this.setAttributeNS(NC,'Name', aValue); });

ServicePaneNode.prototype.__defineGetter__ ('tooltip', function () {
    return this.getAttributeNS(NC,'Description'); })
ServicePaneNode.prototype.__defineSetter__ ('tooltip', function (aValue) {
    this.setAttributeNS(NC,'Description', aValue); });

ServicePaneNode.prototype.__defineGetter__ ('properties', function () {
    return this.getAttributeNS(NC,'Properties'); })
ServicePaneNode.prototype.__defineSetter__ ('properties', function (aValue) {
    this.setAttributeNS(NC,'Properties', aValue); });

ServicePaneNode.prototype.__defineGetter__ ('contractid', function () {
    return this.getAttributeNS(SP,'contractid'); })
ServicePaneNode.prototype.__defineSetter__ ('contractid', function (aValue) {
    this.setAttributeNS(SP,'contractid', aValue); });

ServicePaneNode.prototype.__defineGetter__ ('hidden', function () {
    return this.getAttributeNS(SP,'Hidden') == 'true'; })
ServicePaneNode.prototype.__defineSetter__ ('hidden', function (aValue) {
    this.setAttributeNS(SP,'Hidden', aValue?'true':'false'); });

ServicePaneNode.prototype.__defineGetter__ ('editable', function () {
    return this.getAttributeNS(SP,'Editable') == 'true'; })
ServicePaneNode.prototype.__defineSetter__ ('editable', function (aValue) {
    this.setAttributeNS(SP,'Editable', aValue?'true':'false'); });

ServicePaneNode.prototype.__defineGetter__ ('isOpen', function () {
    return this.getAttributeNS(SP,'Open') == 'true'; })
ServicePaneNode.prototype.__defineSetter__ ('isOpen', function (aValue) {
    this.setAttributeNS(SP,'Open', aValue?'true':'false'); });

ServicePaneNode.prototype.__defineGetter__ ('dndDragTypes', function () {
    return this.getAttributeNS(NC,'dndDragTypes'); })
ServicePaneNode.prototype.__defineSetter__ ('dndDragTypes', function (aValue) {
    this.setAttributeNS(NC,'dndDragTypes', aValue); });

ServicePaneNode.prototype.__defineGetter__ ('dndAcceptNear', function () {
    return this.getAttributeNS(NC,'dndAcceptNear'); })
ServicePaneNode.prototype.__defineSetter__ ('dndAcceptNear', function (aValue) {
    this.setAttributeNS(NC,'dndAcceptNear', aValue); });

ServicePaneNode.prototype.__defineGetter__ ('dndAcceptIn', function () {
    return this.getAttributeNS(NC,'dndAcceptIn'); })
ServicePaneNode.prototype.__defineSetter__ ('dndAcceptIn', function (aValue) {
    this.setAttributeNS(NC,'dndAcceptIn', aValue); });



ServicePaneNode.prototype.__defineGetter__('childNodes',
        function() {
            if (!this.isContainer) {
                throw this.id+' is not a container';
            }
            var e = this._container.GetElements();
            var ds = this._dataSource;
            return {
                hasMoreElements: function() { return e.hasMoreElements(); },
                getNext: function() {
                    if (!e.hasMoreElements()) { return null; }
                    var resource = e.getNext();
                    if (!resource) { return null; }
                    return new ServicePaneNode(ds,
                            resource.QueryInterface(Ci.nsIRDFResource));
                }
            };
        });

ServicePaneNode.prototype.__defineGetter__('firstChild',
        function () {
            return this.childNodes.getNext();
        });

ServicePaneNode.prototype.__defineGetter__('lastChild',
        function () {
            var enumerator = this.childNodes;
            var node = null;
            while (enumerator.hasMoreElements()) {
                node = enumerator.getNext();
            }
            return node;
        });

ServicePaneNode.prototype.__defineGetter__('nextSibling',
        function () {
            var parent = this.parentNode;
            if (!parent) {
                // can't have siblings without parents
                return null;
            }
            var nodes = parent.childNodes;
            while (nodes.hasMoreElements()) {
                var node = nodes.getNext();
                if (node.id == this.id) {
                    return nodes.getNext();
                }
            }
            return null;
        });

ServicePaneNode.prototype.__defineGetter__('parentNode',
        function () {
            var labels = this._dataSource.ArcLabelsIn(this.resource);
            while (labels.hasMoreElements()) {
                var label = labels.getNext().QueryInterface(Ci.nsIRDFResource);
                if (RDFCU.IsOrdinalProperty(label)) {
                    return new ServicePaneNode(this._dataSource,
                            this._dataSource.GetSource(label, this.resource, true));
                }
            }
            return null;
        });

ServicePaneNode.prototype.__defineGetter__('previousSibling',
        function () {
            var parent = this.parentNode;
            if (!parent) {
                // can't have siblings without parents
                return null;
            }
            var prev = null;
            var nodes = parent.childNodes;
            while (nodes.hasMoreElements()) {
                var node = nodes.getNext();
                if (node.id == this.id) {
                    return prev;
                }
                prev = node;
            }
            return null;
        });

ServicePaneNode.prototype.appendChild = function(aChild) {
    if (!this.isContainer) {
        throw this.id+' is not a container';
    }

    aChild.unlinkNode();

    this._container.AppendElement(aChild.resource);
    return aChild;
};

ServicePaneNode.prototype.insertBefore = function(aNewNode, aAdjacentNode) {
    if (!this.isContainer) {
        throw this.id + ' is not a container';
    }
    
    if (this.id != aAdjacentNode.parentNode.id) {
        throw aAdjacentNode.id + ' is not in ' + this.id;
    }
    
    // unlink the node we're moving from where it is currently
    aNewNode.unlinkNode();
    
    // work out where it should go
    var index = this._container.IndexOf(aAdjacentNode.resource);
    
    // add it back in there
    this._container.InsertElementAt(aNewNode.resource, index, true);
}

ServicePaneNode.prototype.removeChild = function(aChild) {
    if (!this.isContainer) {
        throw this.id + ' is not a container';
    }

    if (this._container.IndexOf(aChild.resource)< 0) {
        throw aChild.id + ' is not in ' + this.id;
    }
    
    this._container.RemoveElement(aChild.resource, true);
    
    aChild.clearNode();
}

ServicePaneNode.prototype.replaceChild = function(aNewNode, aOldNode) {
    if (!this.isContainer) {
        throw this.id + ' is not a container';
    }

    var index = this._container.IndexOf(aOldNode.resource);
    if (index < 0) {
        throw aOldNode.id + ' is not in ' + this.id;
    }
    
    this.insertBefore(aNewNode, aOldNode);
    this.removeChild(aOldNode);
}

ServicePaneNode.prototype.unlinkChild = function (aChild) {
    // fail silently
    if (this._container.IndexOf(aChild.resource) < 0) {
        return;
    }
    this._container.RemoveElement(aChild.resource, true);
}

ServicePaneNode.prototype.unlinkNode = function () {
    var parent = this.parentNode;
    if (parent) {
        parent.unlinkChild(this);
    }
}

ServicePaneNode.prototype.clearNode = function () {
    if (this.isContainer) {
        // first we need to remove all our children from the graph
        var children = this.childNodes;
        while (children.hasMoreElements()) {
            this.removeChild(children.getNext());
        }
    }
    
    // then we need to find all our outgoing arcs
    var arcs = this._dataSource.ArcLabelsOut(this.resource);
    while (arcs.hasMoreElements()) {
        var arc = arcs.getNext().QueryInterface(Ci.nsIRDFResource);
        // get the targets
        var targets = this._dataSource.GetTargets(this.resource, arc, true);
        while (targets.hasMoreElements()) {
            var target = targets.getNext().QueryInterface(Ci.nsIRDFNode);
            // and remove them
            this._dataSource.Unassert(this.resource, arc, target);
        }
    }
}





function ServicePaneService () {
    debug('ServicePaneService() starts\n');
    // we want to wait till profile-after-change to initialize
    var obsSvc = Cc['@mozilla.org/observer-service;1']
        .getService(Ci.nsIObserverService);
    obsSvc.addObserver(this, 'profile-after-change', false);
    debug('ServicePaneService() ends\n');
}
ServicePaneService.prototype.observe = /* for nsIObserver */
function ServicePaneService_observe(subject, topic, data) {
    debug('ServicePaneService.observe() begins\n');
    if (topic == 'profile-after-change') {
        // bwe only want this topic once so remove the observer
        var obsSvc = Cc['@mozilla.org/observer-service;1']
            .getService(Ci.nsIObserverService);
        obsSvc.removeObserver(this, 'profile-after-change');
        // the profile is loaded - initialize the service
        this.init();
    }
    debug('ServicePaneService.observe() ends\n');
}
ServicePaneService.prototype.init = function ServicePaneService_init() {
    debug('ServicePaneService.init() begins\n');

    var dirsvc = Cc['@mozilla.org/file/directory_service;1'].getService(Ci.nsIProperties);

    // get the profile directory
    var bResult = {};
    var path = dirsvc.get('ProfD', Ci.nsIFile, bResult);
    path.QueryInterface(Ci.nsIFile);
    // the file where we store our pane's state
    path.append('service-pane.rdf');

    var uri = IOSVC.newFileURI(path);
    // FIXME: does this handle non-latin paths on non-linux?
    this._dataSource = RDFSVC.GetDataSourceBlocking(uri.spec);

    // for sbIServicePaneService.dataSource
    this.__defineGetter__('dataSource', 
        function () { return this._dataSource });


    // for sbIServicePaneService.root
    this.__defineGetter__('root', 
        function () { return this._root });

    // the root of the tree
    this._root = this.getNode('SB:Root');
    if (!this._root) {
        // looks like we need to bootstrap the tree
        // create the root node
        this._dataSource.Assert(RDFSVC.GetResource('SB:Root'),
                                RDFSVC.GetResource(SP+'Hidden'),
                                RDFSVC.GetLiteral('true'), true);
        // make it a sequence
        RDFCU.MakeSeq(this._dataSource, RDFSVC.GetResource('SB:Root'));
        this._root = this.getNode('SB:Root');
        this._root.hidden = false;
        this.save();
    }

    // okay, lets get all the keys
    this._modules = [];
    var module_keys = [];
    var catMgr = Cc["@mozilla.org/categorymanager;1"]
            .getService(Ci.nsICategoryManager);
    var e = catMgr.enumerateCategory('service-pane');
    while (e.hasMoreElements()) {
        var key = e.getNext().QueryInterface(Ci.nsISupportsCString);
        module_keys.push(key);
    }
    
    // let's sort the list
    module_keys.sort()
    
    // and lets create and init the modules in order
    for(var i=0; i<module_keys.length; i++) {
        var key = module_keys[i];
        var contractid = catMgr.getCategoryEntry('service-pane', key);
        debug ('sbSPS: trying to load contractid: '+contractid+'\n');
        debug ('sbSPS: Cc[contractid]: '+Cc[contractid]+'\n');
        try {
            var module = Cc[contractid].getService(Ci.sbIServicePaneModule);
            module.servicePaneInit(this);
            this._modules.push(module);
        } catch (e) {
            debug('ERROR CREATING SERVICE PANE MODULE: '+e);
        }
    }

    debug('ServicePaneService.init() ends\n');
}
// FIXME: implement nsIClassInfo
ServicePaneService.prototype.QueryInterface = 
function ServicePaneService_QueryInterface(iid) {
    if (!iid.equals(Ci.sbIServicePaneService) &&
        !iid.equals(Ci.nsIObserver) &&
        !iid.equals(Ci.nsISupports)) {
        throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
}
ServicePaneService.prototype.addNode =
function ServicePaneService_addNode(aId, aParent, aContainer) {
    DEBUG ('ServicePaneService.addNode('+aId+','+aParent+')');

    /* ensure the parent is supplied */
    if (aParent == null) {
        throw Ce('you need to supply a parent to addNode');
    }
    
    var resource;
    if (aId != null) {
        resource = RDFSVC.GetResource(aId);
        /* ensure the node doesn't already exist */
        if (this._dataSource.GetTargets(resource,
                RDFSVC.GetResource(SP+'Hidden'), true).hasMoreElements()) {
            /* the node has a "hidden" attribute so it must exist */
            return null;
        }
    } else {
        /* no id supplied - make it anonymous */
        resource = RDFSVC.GetAnonymousResource();
    }
    
    /* create an arc making the node hidden, thus creating the object in RDF */
    this._dataSource.Assert(resource,
                            RDFSVC.GetResource(SP+'Hidden'),
                            RDFSVC.GetLiteral('true'), true);
    if (aContainer) {
        RDFCU.MakeSeq(this._dataSource, resource);
    }
    
    /* create the javascript proxy object */
    var node = new ServicePaneNode(this._dataSource, resource);
    
    DEBUG ('ServicePaneService.addNode: node.hidden='+node.hidden);
    
    /* add the node to the parent */
    aParent.appendChild(node)
    
    /* if the node is a container, make it open */
    if (aContainer) {
        node.isOpen = true;
    }
    
    // by default nothing is editable
    node.editable = false;
    
    return node;
}
ServicePaneService.prototype.removeNode =
function ServicePaneService_removeNode(aNode) {
    /* ensure the node is supplied*/
    if (aNode == null) {
        throw Ce('you need to supply a node to removeNode');
    }
    
    if (aNode.id == this._root.id) {
        throw Ce("you can't remove the root node");
    }
    
    if(aNode.parentNode) {
        // remove the node from it's parent
        aNode.parentNode.removeChild(aNode);
    } else {
        // or if it's an orphan call the internal function to clear it out
        aNode.clearNode();
    }
}
ServicePaneService.prototype.getNode =
function ServicePaneService_getNode(aId) {
    var resource = RDFSVC.GetResource(aId);
    /* ensure the node already exists */
    if (!this._dataSource.GetTargets(resource,
            RDFSVC.GetResource(SP+'Hidden'), true).hasMoreElements()) {
        /* the node has a "hidden" attribute so it must exist */
        return null;
    }
    
    return new ServicePaneNode(this._dataSource, resource);
    
}

ServicePaneService.prototype.save =
function ServicePaneService_save() {
    /* FIXME: this should really be rate-limited */
    this._dataSource.QueryInterface(Ci.nsIRDFRemoteDataSource).Flush();
}
ServicePaneService.prototype.fillContextMenu =
function ServicePaneService_fillContextMenu(aId, aContextMenu, aParentWindow) {
    var node = this.getNode(aId);
    for (var i=0; i<this._modules.length; i++) {
        this._modules[i].fillContextMenu(node, aContextMenu, aParentWindow);
    }
}
ServicePaneService.prototype._canDropReorder =
function ServicePaneService__canDropReorder(aNode, aDragSession, aOrientation) {
    // see if we can handle the drag and drop based on node properties
    var types = [];
    if (aOrientation == 0) {
        // drop in
        if (!aNode.isContainer) {
            // not a container - don't even try
            return null;
        }
        if (aNode.dndAcceptIn) {
            types = aNode.dndAcceptIn.split(',');
        }
    } else {
        // drop near
        if (aNode.dndAcceptNear) {
            types = aNode.dndAcceptNear.split(',');
        }
    }
    for (var i=0; i<types.length; i++) {
        if (aDragSession.isDataFlavorSupported(types[i])) {
            return types[i];
        }
    }
    return null;
}
ServicePaneService.prototype.canDrop =
function ServicePaneService_canDrop(aId, aDragSession, aOrientation) {
    var node = this.getNode(aId);
    if (!node) {
        return false;
    }
    
    // see if we can handle the drag and drop based on node properties
    if (this._canDropReorder(node, aDragSession, aOrientation)) {
        return true;
    }
    
    // let the module that owns this node handle this
    if (node.contractid && Cc[node.contractid]) {
        var module = Cc[node.contractid].getService(Ci.sbIServicePaneModule);
        if (module) {
            return module.canDrop(node, aDragSession, aOrientation);
        }
    }
    return false;
}
ServicePaneService.prototype.onDrop =
function ServicePaneService_onDrop(aId, aDragSession, aOrientation) {
    var node = this.getNode(aId);
    if (!node) {
        return false;
    }
    // see if this is a reorder we can handle based on node properties
    var type = this._canDropReorder(node, aDragSession, aOrientation);
    if (type) {
        // we're in business
        
        // do the dance to get our data out of the dnd system
        // create an nsITransferable
        var transferable = Components.classes["@mozilla.org/widget/transferable;1"].
                createInstance(Components.interfaces.nsITransferable);
        // specify what kind of data we want it to contain
        transferable.addDataFlavor(type);
        // ask the drag session to fill the transferable with that data
        aDragSession.getData(transferable, 0);
        // get the data from the transferable
        var data = {};
        var dataLength = {};
        transferable.getTransferData(type, data, dataLength);
        // it's always a string. always.
        data = data.value.QueryInterface(Components.interfaces.nsISupportsString);
        data = data.toString();
        
        // for drag and drop reordering the data is just the servicepane uri
        var droppedNode = this.getNode(data);
        
        // fail if we can't get the node
        if (!droppedNode) {
            return;
        }
        
        if (aOrientation == 0) {
            // drop into
            // we should append the new node to the current node
            node.appendChild(droppedNode);
        } else if (aOrientation > 0) {
            // drop after
            if (node.nextSibling) {
                // not the last node
                node.parentNode.insertBefore(droppedNode, node.nextSibling);
            } else {
                // the last node
                node.parentNode.appendChild(droppedNode);
            }
        } else {
            // drop before
            node.parentNode.insertBefore(droppedNode, node);
        }
        return;
    }
    
    // or let the module that owns this node handle it
    if (node.contractid) {
        var module = Cc[node.contractid].getService(Ci.sbIServicePaneModule);
        if (module) {
            module.onDrop(node, aDragSession, aOrientation);
        }
    }
}
ServicePaneService.prototype.onDragGesture =
function ServicePaneService_onDragGesture(aId, aTransferable) {
    DEBUG('onDragGesture('+aId+', '+aTransferable+')');
    var node = this.getNode(aId);
    if (!node) {
        return false;
    }
    
    var success = false;
    
    // create a transferable
    var transferable = Components.classes["@mozilla.org/widget/transferable;1"].
        createInstance(Components.interfaces.nsITransferable);
        
    // get drag types from the node data
    if (node.dndDragTypes) {
        var types = node.dndDragTypes.split(',');
        for (var i=0; i<types.length; i++) {
            var type = types[i];
            transferable.addDataFlavor(type);
            var text = Components.classes["@mozilla.org/supports-string;1"].
               createInstance(Components.interfaces.nsISupportsString);
            text.data = node.id;
            // double the length - it's unicode - this is stupid
            transferable.setTransferData(type, text, text.data.length*2);
            success = true;
        }
    }
        
    if (node.contractid && Cc[node.contractid]) {
        var module = Cc[node.contractid].getService(Ci.sbIServicePaneModule);
        if (module) {
            if (module.onDragGesture(node, transferable)) {
                if (!success) {
                    success = true;
                }
            }
        }
    }
    
    if (success) {
        aTransferable.value = transferable;
    }
    
    DEBUG(' success='+success);
    
    return success;
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
    ServicePaneService,
    Components.ID("{eb5c665a-bfe2-49f1-a747-cd3554e55606}"),
    "Songbird Service Pane Service",
    "@songbirdnest.com/servicepane/service;1",
    [{
        category: 'app-startup',
        entry: 'service-pane-service',
        value: 'service,@songbirdnest.com/servicepane/service;1'
    }]);
