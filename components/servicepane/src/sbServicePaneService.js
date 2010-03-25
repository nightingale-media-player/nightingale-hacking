/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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
 * \file ServicePaneService.js
 * \brief the service pane service manages the tree behind the service pane
 *
 * TODO:
 *  - implement lazy saving of dirty datasources.
 *  - handle module removal
 *
 */

function DEBUG(msg) {
  //dump('sbServicePaneService.js: '+msg+'\n');
}

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ce = Components.Exception;
const Cu = Components.utils;

Cu.import("resource://app/jsmodules/RDFHelper.jsm");

const RDFSVC = Cc['@mozilla.org/rdf/rdf-service;1'].getService(Ci.nsIRDFService);
const RDFCU = Cc['@mozilla.org/rdf/container-utils;1'].getService(Ci.nsIRDFContainerUtils);
const IOSVC = Cc['@mozilla.org/network/io-service;1'].getService(Ci.nsIIOService);
const gPrefs = Cc['@mozilla.org/preferences-service;1'].getService(Ci.nsIPrefBranch);

const NC='http://home.netscape.com/NC-rdf#';
const SP='http://songbirdnest.com/rdf/servicepane#';
const DEVICESP_NS = "http://songbirdnest.com/rdf/device-servicepane#";

const STRINGBUNDLE='chrome://songbird/locale/songbird.properties';

function ServicePaneNode (ds, resource) {
  this._resource = resource;

  this._dataSource = ds;
  this._dataSource.QueryInterface(Ci.nsIRDFRemoteDataSource);

  // set up this._container if this is a Seq
  if (RDFCU.IsSeq(ds, resource)) {
    this._container = Cc['@mozilla.org/rdf/container;1'].createInstance(Ci.nsIRDFContainer);
    this._container.Init(ds, resource);
    this._resource = null;
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

ServicePaneNode.prototype.__defineGetter__ ('resource',
  function () {
    if (this._container) {
      return this._container.Resource;
    }
    else {
      return this._resource;
    }
  }
);

ServicePaneNode.prototype.__defineGetter__ ('id',
    function () { return this.resource.ValueUTF8; });

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

ServicePaneNode.prototype._getAttributeNS = function (aNamespace, aName, aDS) {
  var property = RDFSVC.GetResource(aNamespace+aName);
  var target = aDS.GetTarget(this.resource, property, true);
  if (target) {
    var value = target.QueryInterface(Ci.nsIRDFLiteral).Value
    return value;
  } else {
    return null;
  }
}
ServicePaneNode.prototype.getAttributeNS = function (aNamespace, aName) {
  return this._getAttributeNS(aNamespace, aName, this._dataSource);
};
/**
 * This is the same as .getAttributeNS, but used with the localization-wrapped
 * datasource instead, so the string will be localized to the display value if
 * appropriate.
 */
ServicePaneNode.prototype._getLocalizedAttributeNS = function (aNamespace, aName) {
  // this could be better :(
  var ds = Cc['@songbirdnest.com/servicepane/service;1']
             .getService(Ci.sbIServicePaneService)
             .dataSource;
  return this._getAttributeNS(aNamespace, aName, ds);
};

ServicePaneNode.prototype.hasAttributeNS =
  function SPN_hasAttributeNS(aNamespace, aName)
{
  var property = RDFSVC.GetResource(aNamespace+aName);
  return this._dataSource.hasArcOut(this.resource, property);
}

ServicePaneNode.prototype.__defineGetter__ ('url', function () {
  return this.getAttributeNS(NC,'URL'); })
ServicePaneNode.prototype.__defineSetter__ ('url', function (aValue) {
  this.setAttributeNS(NC,'URL', aValue); });

ServicePaneNode.prototype.__defineGetter__ ('image', function () {
  return this.getAttributeNS(NC,'Icon');
})
ServicePaneNode.prototype.__defineSetter__ ('image', function (aValue) {
  var self = this;
  if ((this.getAttributeNS(DEVICESP_NS, "deviceNodeType") == "device") &&
      (this.properties.split(/\s/).indexOf("read-only") >= 0))
  {
    // Create a read-only representation of the device icon
    // Since we need a document to create the canvas element from, we'll
    // grab the appShellService's hidden window
    var appShellService = Cc["@mozilla.org/appshell/appShellService;1"]
                            .getService(Ci.nsIAppShellService);
    var parentDoc = appShellService.hiddenWindow.docShell
                    .QueryInterface(Ci.nsIInterfaceRequestor)
                    .getInterface(Ci.nsIDOMWindow).document;
    var canvas = parentDoc.createElementNS(
                     "http://www.w3.org/1999/xhtml", 'canvas');
    canvas.width = 16;
    canvas.height = 16;
    var ctx = canvas.getContext('2d');

    // Create the Image elements and draw them to the canvas
    var deviceImage = parentDoc.createElementNS(
                     "http://www.w3.org/1999/xhtml", 'img');
    var roImage = parentDoc.createElementNS(
                     "http://www.w3.org/1999/xhtml", 'img');
    roImage.onload = function() {
      ctx.drawImage(roImage, 0, 0);

      // Get the data URI representation of the canvas:
      var dataURI= canvas.toDataURL("image/png", "");

      self.setAttributeNS(NC,'Icon', dataURI);
    }
    deviceImage.onload = function() {
      ctx.drawImage(deviceImage, 0, 0);
      roImage.src = "chrome://songbird/skin/service-pane/read-only.png";
    }

    // Set the source for the images, thus loading them
    this.setAttributeNS(NC,'Icon', aValue);
    deviceImage.src = aValue;
  } else {
    this.setAttributeNS(NC,'Icon', aValue);
  }
});

ServicePaneNode.prototype.__defineGetter__ ('name', function () {
  return this.getAttributeNS(NC,'Name'); })
ServicePaneNode.prototype.__defineSetter__ ('name', function (aValue) {
  this.setAttributeNS(NC,'Name', aValue); });

ServicePaneNode.prototype.__defineGetter__ ('displayName', function () {
  return this._getLocalizedAttributeNS(NC,'Name'); });

ServicePaneNode.prototype.__defineGetter__ ('tooltip', function () {
  return this.getAttributeNS(NC,'Description'); })
ServicePaneNode.prototype.__defineSetter__ ('tooltip', function (aValue) {
  this.setAttributeNS(NC,'Description', aValue); });

ServicePaneNode.prototype.__defineGetter__ ('properties', function () {
  return this.getAttributeNS(SP,'Properties'); })
ServicePaneNode.prototype.__defineSetter__ ('properties', function (aValue) {
 this.setAttributeNS(SP,'Properties', aValue); });

ServicePaneNode.prototype.__defineGetter__ ('contractid', function () {
  return this.getAttributeNS(SP,'contractid'); })
ServicePaneNode.prototype.__defineSetter__ ('contractid', function (aValue) {
  this.setAttributeNS(SP,'contractid', aValue); });

ServicePaneNode.prototype.__defineGetter__ ('hidden', function () {
  return this.getAttributeNS(SP,'Hidden') == 'true'; })
ServicePaneNode.prototype.__defineSetter__ ('hidden', function (aValue) {
  this.setAttributeNS(SP,'Hidden', aValue?'true':'false'); });

ServicePaneNode.prototype.__defineGetter__ ('hasNoChildren', function () {
  return this._container.GetCount() == 0; })

ServicePaneNode.prototype.__defineGetter__ ('editable', function () {
  return this.getAttributeNS(SP,'Editable') == 'true'; })
ServicePaneNode.prototype.__defineSetter__ ('editable', function (aValue) {
  this.setAttributeNS(SP,'Editable', aValue?'true':'false'); });

ServicePaneNode.prototype.__defineGetter__ ('isOpen', function () {
  return this.getAttributeNS(SP,'Open') == 'true'; })
ServicePaneNode.prototype.__defineSetter__ ('isOpen', function (aValue) {
  this.setAttributeNS(SP,'Open', aValue?'true':'false'); });

ServicePaneNode.prototype.__defineGetter__ ('dndDragTypes', function () {
  return this.getAttributeNS(SP,'dndDragTypes'); })
ServicePaneNode.prototype.__defineSetter__ ('dndDragTypes', function (aValue) {
  this.setAttributeNS(SP,'dndDragTypes', aValue); });

ServicePaneNode.prototype.__defineGetter__ ('dndAcceptNear', function () {
  return this.getAttributeNS(SP,'dndAcceptNear'); })
ServicePaneNode.prototype.__defineSetter__ ('dndAcceptNear', function (aValue) {
  this.setAttributeNS(SP,'dndAcceptNear', aValue); });

ServicePaneNode.prototype.__defineGetter__ ('dndAcceptIn', function () {
  return this.getAttributeNS(SP,'dndAcceptIn'); })
ServicePaneNode.prototype.__defineSetter__ ('dndAcceptIn', function (aValue) {
  this.setAttributeNS(SP,'dndAcceptIn', aValue); });

ServicePaneNode.prototype.__defineGetter__ ('stringbundle', function () {
  return this.getAttributeNS(SP,'stringbundle'); })
ServicePaneNode.prototype.__defineSetter__ ('stringbundle', function (aValue) {
  this.setAttributeNS(SP,'stringbundle', aValue); });

ServicePaneNode.prototype.__defineGetter__ ('shouldHideIfEmpty', function() {
  return (this.properties && this.properties.split(/\s/).indexOf("hide-if-empty") > -1);
});

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

  // Show this node if the "hide-if-empty" property is set
  if (this.shouldHideIfEmpty && this.hasNoChildren) {
    this.hidden = false;
  } 

  aChild.unlinkNode();

  this._container.AppendElement(aChild.resource);

  return aChild;
};

ServicePaneNode.prototype.insertBefore = function(aNewNode, aAdjacentNode) {
  DEBUG('insertBefore('+aNewNode.id+', '+aAdjacentNode.id+')');

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

  DEBUG(' index='+index);

  // Show this node if the "hide-if-empty" property is set
  if (this.shouldHideIfEmpty && this.hasNoChildren) {
    this.hidden = false;  
  }

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

  // Hide this node if the "hide-if-empty" property is set. 
  if (this.shouldHideIfEmpty && this.hasNoChildren) {
    this.hidden = true;
  }

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
  DEBUG('ServicePaneService() starts');
  this._initialized = false;
  var obsSvc = Cc['@mozilla.org/observer-service;1']
    .getService(Ci.nsIObserverService);
  obsSvc.addObserver(this, 'quit-application', false);
  DEBUG('ServicePaneService() ends');
}
ServicePaneService.prototype.observe = /* for nsIObserver */
function ServicePaneService_observe(subject, topic, data) {
  DEBUG('ServicePaneService.observe() begins');
  if (topic == 'quit-application') {
    var obsSvc = Cc['@mozilla.org/observer-service;1']
      .getService(Ci.nsIObserverService);
    obsSvc.removeObserver(this, 'quit-application');
    this.shutdown();
  }
  DEBUG('ServicePaneService.observe() ends');
}
ServicePaneService.prototype.init = function ServicePaneService_init() {
  DEBUG('ServicePaneService.init() begins');

  if (this._initialized) {
    // already initialized
    DEBUG('already initialized');
    return;
  }
  this._initialized = true;

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
  this._dataSourceWrapped = new dsTranslator(this._dataSource, this);
  this._dsSaver = new dsSaver(this._dataSource, 30*1000); // try to save every thirty seconds

  // listen to changes in the datasource
  this._dataSource.AddObserver(this);

  // track service pane listeners
  this._listeners = [];

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
    DEBUG ('trying to load contractid: '+contractid);
    DEBUG ('Cc[contractid]: '+Cc[contractid]);
    try {
      var module = Cc[contractid].getService(Ci.sbIServicePaneModule);
      module.servicePaneInit(this);
      this._modules.push(module);
    } catch (e) {
      DEBUG('ERROR CREATING SERVICE PANE MODULE: '+e);
    }
  }

  // HACK ALERT
  // now, let's sort the top-level nodes by their weight.
  // this idea of node weight is stolen from Drupal menus
  var node = this._root.firstChild;
  while (node) {
    var value = parseInt(node.getAttributeNS(SP, 'Weight'));
    while (node.previousSibling) {
      var prev_value =
          parseInt(node.previousSibling.getAttributeNS(SP, 'Weight'));
      if (prev_value > value) {
        this._root.insertBefore(node, node.previousSibling);
      } else {
        break;
      }
    }

    node = node.nextSibling;
  }

  // Check addon nodes.
  this.checkAddonNodes();

  DEBUG('ServicePaneService.init() ends');
}

ServicePaneService.prototype.checkAddonNodes =
  function ServicePaneService_checkAddonNodes(aNode) {
  // Get node.  Start at root if none specified.
  var node = aNode;
  if (!node)
    node = this._root;

  // Check node if it's an addon node.
  if (node.hasAttributeNS(SP, "addonID")) {
    // If node addon is not installed, remove node and return.  If node addon is
    // disabled, hide node.
    var addonID = node.getAttributeNS(SP, "addonID");
    if (!this.isAddonInstalled(addonID)) {
      this.removeNode(node);
      return;
    }
    else if (this.isAddonDisabled(addonID)) {
      node.hidden = true;
    }
  }

  // Check node children.
  if (node.isContainer) {
    var child = node.firstChild;
    while (child) {
      var nextSibling = child.nextSibling;
      this.checkAddonNodes(child);
      child = nextSibling;
    }
  }
}

ServicePaneService.prototype.isAddonInstalled =
  function ServicePaneService_isAddonInstalled(aAddonID) {
  // Check if addon is installed.
  var extensionManager = Cc["@mozilla.org/extensions/manager;1"]
                           .getService(Ci.nsIExtensionManager);
  var addonInstalled = extensionManager.getItemForID(aAddonID) ? true : false;

  return addonInstalled;
}

ServicePaneService.prototype.isAddonDisabled =
  function ServicePaneService_isAddonDisabled(aAddonID) {
  // Get the addon info.
  var extensionManager = Cc["@mozilla.org/extensions/manager;1"]
                           .getService(Ci.nsIExtensionManager);
  addonInfo = RDFHelper.help(extensionManager.datasource.URI,
                             "urn:mozilla:item:" + aAddonID,
                             RDFHelper.DEFAULT_RDF_NAMESPACES);

  // Check if addon is disabled.
  var addonDisabled = false;
  if ((addonInfo.userDisabled == "true") ||
      (addonInfo.userDisabled == "needs-enable") ||
      (addonInfo.appDisabled == "true") ||
      (addonInfo.appDisabled == "needs-enable")) {
    addonDisabled = true;
  }

  return addonDisabled;
}

ServicePaneService.prototype.shutdown = function ServicePaneService_shutdown() {
  DEBUG('ServicePaneService.shutdown() begins');

  if (!this._initialized) {
    // if we werent initialized, there's no need to shutdown
    return;
  }

  // Clear our array of service pane providers.  This is needed to break a
  // reference cycle between the provider and this service
  for (let i = 0; i < this._modules.length; i++) {
    try {
      this._modules[i].shutdown();
    }
    catch (e) {
      // Ignore errors
      Components.utils.reportError(e);
    }
    this._modules[i] = null;
  }

  // remove ourselves as an RDF observer
  this._dataSource.RemoveObserver(this);

  // finalize the data source saver
  this._dsSaver.finalize();
  this._dsSaver = null;

  this._dataSource = null;
  this._dataSourceWrapped = null;
  this._root = null;

  DEBUG('ServicePaneService.shutdown() ends');
}

// for sbIServicePaneService.dataSource
ServicePaneService.prototype.__defineGetter__('dataSource', function () {
  if (!this._initialized) { this.init(); }
  return this._dataSourceWrapped;
});

// for sbIServicePaneService.root
ServicePaneService.prototype.__defineGetter__('root', function () {
  if (!this._initialized) { this.init(); }
  return this._root;
});

// FIXME: implement nsIClassInfo
ServicePaneService.prototype.QueryInterface =
function ServicePaneService_QueryInterface(iid) {
  if (!iid.equals(Ci.sbIServicePaneService) &&
    !iid.equals(Ci.nsIRDFObserver) &&
    !iid.equals(Ci.nsIObserver) &&
    !iid.equals(Ci.nsISupports)) {
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
  return this;
}
ServicePaneService.prototype.addNode =
function ServicePaneService_addNode(aId, aParent, aContainer) {
  if (!this._initialized) { this.init(); }
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

  // by default nothing is editable and everything is very heavy
  node.editable = false;
  node.setAttributeNS(SP, 'Weight', 9999);
  return node;
}
ServicePaneService.prototype.removeNode =
function ServicePaneService_removeNode(aNode) {
  if (!this._initialized) { this.init(); }
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
  if (!this._initialized) { this.init(); }
  try {
    var resource = RDFSVC.GetResource(aId);
    /* ensure the node already exists */
    if (!this._dataSource.GetTargets(resource,
        RDFSVC.GetResource(SP+'Hidden'), true).hasMoreElements()) {
      /* the node has a "hidden" attribute so it must exist */
      return null;
    }
  } catch (e) {
    // if we failed, report the error
    Components.utils.reportError(e);
    // and return null, caller may be able to get back on his feet
    return null;
  }
  return new ServicePaneNode(this._dataSource, resource);
}

ServicePaneService.prototype.getNodeForURL =
function ServicePaneService_getNodeForURL(aURL) {
  if (!this._initialized) { this.init(); }

  var ncURL = RDFSVC.GetResource(NC+"URL");
  var url = RDFSVC.GetLiteral(aURL);
  var target = this._dataSource.GetSource(ncURL, url, true);
  return (target) ? this.getNode(target.Value) : null;
}

ServicePaneService.prototype.getNodesByAttributeNS =
function ServicePaneService_getNodesByAttributeNS(aNamespace, aName, aValue) {
  if (!this._initialized) { this.init(); }

  var property = RDFSVC.GetResource(aNamespace+aName);
  var target = RDFSVC.GetLiteral(aValue);
  var sourceEnum = this._dataSource.GetSources(property, target, true);

  var nodeList = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"].createInstance(Ci.nsIMutableArray);
  while (sourceEnum.hasMoreElements()) {
    var source = sourceEnum.getNext().QueryInterface(Ci.nsIRDFResource);
    if (source) {
      var node = this.getNode(source.Value);
      if (node) {
        nodeList.appendElement(node, false);
      }
    }
  }

  return nodeList.QueryInterface(Ci.nsIArray);
}

ServicePaneService.prototype.sortNode =
function ServicePaneService_sortNode(aNode) {
  // Determine the correct position of the node by its weight
  var value = parseInt(aNode.getAttributeNS(SP, 'Weight'));
  var insertBefore = null;
  var prevNode = null;
  var enumerator = this._root.childNodes;
  while (enumerator.hasMoreElements()) {
    var current = enumerator.getNext();
    var currentValue = parseInt(current.getAttributeNS(SP, 'Weight'));
    if (currentValue > value) {
      insertBefore = current;
      break;
    }
    prevNode = current;
  }

  if (prevNode && prevNode.id == aNode.id) {
    // The node is already in the right place, nothing to do
    return;
  }

  // Add the node
  if (insertBefore)
    this._root.insertBefore(aNode, insertBefore);
  else
    this._root.appendChild(aNode);
}

ServicePaneService.prototype.save =
function ServicePaneService_save() {
  /* FIXME: this function should go away */
}

ServicePaneService.prototype.fillContextMenu =
function ServicePaneService_fillContextMenu(aId, aContextMenu, aParentWindow) {
  if (!this._initialized) { this.init(); }
  var node = aId ? this.getNode(aId) : null;
  for (var i=0; i<this._modules.length; i++) {
    try {
      this._modules[i].fillContextMenu(node, aContextMenu, aParentWindow);
    } catch (ex) {
      if (gPrefs.getPrefType('javascript.options.showInConsole') &&
          gPrefs.getBoolPref('javascript.options.showInConsole')) {
        Components.utils.reportError(ex);
      }
    }
  }
}

ServicePaneService.prototype.fillNewItemMenu =
function ServicePaneService_fillNewItemMenu(aId, aContextMenu, aParentWindow) {
  if (!this._initialized) { this.init(); }
  var node = aId ? this.getNode(aId) : null;
  for (var i=0; i<this._modules.length; i++) {
    try {
      this._modules[i].fillNewItemMenu(node, aContextMenu, aParentWindow);
    } catch (ex) {
      if (gPrefs.getPrefType('javascript.options.showInConsole') &&
          gPrefs.getBoolPref('javascript.options.showInConsole')) {
        Components.utils.reportError(ex);
      }
    }
  }
}

ServicePaneService.prototype.onSelectionChanged =
function ServicePaneService_onSelectionChanged(aId, aContainer, aParentWindow) {
  if (!this._initialized) { this.init(); }
  var node = aId ? this.getNode(aId) : null;
  for (var i=0; i<this._modules.length; i++) {
    try {
      this._modules[i].onSelectionChanged(node, aContainer, aParentWindow);
    } catch (ex) {
      if (gPrefs.getPrefType('javascript.options.showInConsole') &&
          gPrefs.getBoolPref('javascript.options.showInConsole')) {
        Components.utils.reportError(ex);
      }
    }
  }
}

ServicePaneService.prototype._canDropReorder =
function ServicePaneService__canDropReorder(aNode, aDragSession, aOrientation) {
  if (!this._initialized) { this.init(); }
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
function ServicePaneService_canDrop(aId, aDragSession, aOrientation, aWindow) {
  if (!this._initialized) { this.init(); }
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
      return module.canDrop(node, aDragSession, aOrientation, aWindow);
    }
  }
  return false;
}
ServicePaneService.prototype.onDrop =
function ServicePaneService_onDrop(aId, aDragSession, aOrientation, aWindow) {
  if (!this._initialized) { this.init(); }
  var node = this.getNode(aId);
  if (!node) {
    return;
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
        if (droppedNode.id == node.nextSibling.id) {
          return; // can't insert after ourselves
        }
        node.parentNode.insertBefore(droppedNode, node.nextSibling);
      } else {
        // the last node
        node.parentNode.appendChild(droppedNode);
      }
    } else {
      // drop before
      if (droppedNode.id == node.id) {
        return; // can't insert before ourselves
      }
      node.parentNode.insertBefore(droppedNode, node);
    }
    return;
  }

  // or let the module that owns this node handle it
  if (node.contractid) {
    var module = Cc[node.contractid].getService(Ci.sbIServicePaneModule);
    if (module) {
      module.onDrop(node, aDragSession, aOrientation, aWindow);
    }
  }
}
ServicePaneService.prototype.onDragGesture =
function ServicePaneService_onDragGesture(aId, aTransferable) {
  if (!this._initialized) { this.init(); }
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
 * Called before a node is renamed by the user.
 * Delegates to the module that owns the given node.
 */
ServicePaneService.prototype.onBeforeRename =
function ServicePaneService_onBeforeRename(aID) {
  if (!this._initialized) {
    this.init();
  }
  var node = this.getNode(aID);
  if (!node || !node.editable) {
    return;
  }

  // Pass the message on to the node owner
  if (node.contractid) {
    var module = Cc[node.contractid].getService(Ci.sbIServicePaneModule);
    if (module) {
      module.onBeforeRename(node);
    }
  }
}

/**
 * Called when a node is renamed by the user.
 * Delegates to the module that owns the given node.
 */
ServicePaneService.prototype.onRename =
function ServicePaneService_onRename(aID, aNewName) {
  if (!this._initialized) { this.init(); }
  var node = this.getNode(aID);
  if (!node || !node.editable) {
    return;
  }

  // Pass the message on to the node owner
  if (node.contractid) {
    var module = Cc[node.contractid].getService(Ci.sbIServicePaneModule);
    if (module) {
      module.onRename(node, aNewName);
    }
  }
}

ServicePaneService.prototype.addListener =
function ServicePaneService_addListener(aListener) {
  this._listeners.push(aListener);
}

ServicePaneService.prototype.removeListener =
function ServicePaneService_removeListener(aListener) {
  for (var i=0; i<this._listeners.length; i++) {
    if (this._listeners[i] == aListener) {
      this._listeners.splice(i,1);
    }
  }
}

/* nsIRDFObserver -> sbIServicePaneListener */
ServicePaneService.prototype._notifyListeners =
function ServicePaneService__notifyListeners(aSource, aProperty) {
  DEBUG('_notifyListeners aSource='+aSource.Value+' aProperty='+aProperty.Value);
  var nodeid = aSource.Value;
  var property = aProperty.Value;
  for (var i=0; i<this._listeners.length; i++) {
    this._listeners[i].nodePropertyChanged(nodeid, property);
  }
}

ServicePaneService.prototype.onAssert =
function ServicePaneService_onAssert(aDataSource, aSource, aProperty, aTarget) {
  this._notifyListeners(aSource, aProperty);
},
ServicePaneService.prototype.onUnassert =
function ServicePaneService_onUnassert(aDataSource, aSource, aProperty, aTarget) {
  this._notifyListeners(aSource, aProperty);
},
ServicePaneService.prototype.onChange =
function ServicePaneService_onChange(aDataSource, aSource, aProperty, aOldTarget, aNewTarget) {
  this._notifyListeners(aSource, aProperty);
},
ServicePaneService.prototype.onMove =
function ServicePaneService_onMove(aDataSource, aOldSource, aNewSource, aProperty, aTarget) {
  this._notifyListeners(aOldSource, aProperty);
  this._notifyListeners(aNewSource, aProperty);
},
ServicePaneService.prototype.onBeginUpdateBatch =
function ServicePaneService_onBeginUpdateBatch(a) { },
ServicePaneService.prototype.onEndUpdateBatch =
function ServicePaneService_onEndUpdateBatch(a) { }


/* this is a wrapper for the nsIRDFDataSource that will translate some of the
  properties via stringbundles */
function dsTranslator(inner, sps) {
  this.inner = inner;
  this.sps = sps;
  this.properties = {};
  this.properties[NC+'Name'] = true;
  this.stringBundleService = Cc['@mozilla.org/intl/stringbundle;1']
      .getService(Ci.nsIStringBundleService);
  this._observers = [];
}
dsTranslator.prototype = {
  // impl methods

  // Try to translate a key based on a string bundle uri. Either return an
  // nsIRDFLiteral on success or null on failure.
  translateKey: function translateKey(stringbundleuri, key) {
    var bundle = this.stringBundleService.createBundle(stringbundleuri);
    if (!bundle) { return null; }
    var message = null;
    try {
      message = bundle.GetStringFromName(key);
    } catch (e) { }
    if (!message) { return null; }
    return RDFSVC.GetLiteral(message);
  },

  // Try to translate an RDF triple. Either return either the original or
  // the translated target
  translateTriple: function translateTriple(aSource, aProperty, aTarget) {
    // if there's no target, fail
    if (!aTarget) { return null; }

    // if the target is not an nsIRDFLiteral, return it
    if (!(aTarget instanceof Ci.nsIRDFLiteral)) { return aTarget; }

    // if the target does not begin with "&", return it
    if (aTarget.Value.length < 1 ||
        aTarget.Value[0] != '&') {
      return aTarget;
    }

    // the key is the rest of aTarget.Value
    var key = aTarget.Value.substring(1);

    // we need to find the string bundle we look in these places in this
    // order:
    //   a property on the node (http://songbirdnest.com/rdf/servicepane#stringbundle)
    //   the .stringbundle attribute on the associated module service
    //   the default stringbundle on the service pane service

    // FIXME: after testing wrap each of these steps in try/catch

    var node = this.sps.getNode(aSource.Value);

    // the node's stringbundle
    if (node && node.stringbundle) {
      var translated = this.translateKey(node.stringbundle, key);
      if (translated) {
        return translated;
      }
    }

    // the module's stringbundle
    if (node && node.contractid && Cc[node.contractid]) {
      var m = Cc[node.contractid].getService(Ci.sbIServicePaneModule);
      if (m && m.stringbundle) {
        var translated = this.translateKey(m.stringbundle, key);
        if (translated) {
          return translated
        }
      }
    }

    // the app's default stringbundle
    var translated = this.translateKey(STRINGBUNDLE, key)
    if (translated) {
      return translated;
    }

    // couldn't translate it - let's just return the raw target
    return aTarget;
  },

  // nsISupports
  QueryInterface: function QueryInterface(iid) {
    if (iid.equals(Ci.nsIRDFObserver)) {
      return this;
    }
    return this.inner.QueryInterface(iid);
  },

  // nsIRDFDataSource
  get URI() { return this.inner.URI; },
  GetSource: function GetSource(a,b,c) { return this.inner.GetSource(a,b,c); },
  GetSources: function GetSources(a,b,c) { return this.inner.GetSources(a,b,c); },
  GetTarget: function GetTarget(aSource, aProperty, aTruthValue) {
    var target = this.inner.GetTarget(aSource, aProperty, aTruthValue);

    // is this property translatable
    if (this.properties[aProperty.Value]) {
      return this.translateTriple(aSource, aProperty, target);
    } else {
      return target;
    }
  },
  // FIXME do we need to translate GetTargets too?
  GetTargets: function GetTargets(aSource, aProperty, aTruthValue) {
    var targets = this.inner.GetTargets(aSource, aProperty, aTruthValue);

    // is this property translatable?
    if (this.properties[aProperty.Value]) {
      // we need to create a proxy enumerator object
      var self = this;
      var enumerator = {
        hasMoreElements: function hasMoreElements() {
          return targets.hasMoreElements();
        },
        getNext: function getNext() {
          return self.translateTriple(aSource, aProperty,
              targets.getNext());
        },
        QueryInterface: function QueryInterface(iid) {
          return targets.QueryInterface(iid);
        }
      };
      return enumerator
    } else {
      return targets;
    }
  },
  Assert: function Assert(a,b,c,d) { this.inner.Assert(a,b,c,d); },
  Unassert: function Unassert(a,b,c) { this.inner.Unassert(a,b,c); },
  Change: function Change(a,b,c,d) { this.inner.Change(a,b,c,d); },
  Move: function Move(a,b,c,d) { this.inner.Move(a,b,c,d); },
  HasAssertion: function HasAssertion(a,b,c,d) { return this.inner.HasAssertion(a,b,c,d); },
  ArcLabelsIn: function ArcLabelsIn(a) { return this.inner.ArcLabelsIn(a); },
  ArcLabelsOut: function ArcLabelsOut(a) { return this.inner.ArcLabelsOut(a); },
  GetAllResources: function GetAllResources() { return this.inner.GetAllResourvces(); },
  IsCommandEnabled: function IsCommandEnabled(a,b,c) { return this.inner.IsCommandEnabled(a,b,c); },
  DoCommand: function DoCommand(a,b,c) { this.inner.DoCommand(a,b,c); },
  IsCommandEnabled: function IsCommandEnabled(a,b,c) { return this.inner.IsCommandEnabled(a,b,c); },
  GetAllCmds: function GetAllCmds() { return this.inner.GetAllCmds(); },
  hasArcIn: function hasArcIn(a,b) { this.inner.hasArcIn(a,b); },
  hasArcOut: function hasArcOut(a,b) { this.inner.hasArcOut(a,b); },
  beginUpdateBatch: function beginUpdateBatch() { this.inner.beginUpdateBatch(); },
  endUpdateBatch: function endUpdateBatch() { this.inner.endUpdateBatch(); },

  // nsIRDFDataSource - observers
  _observers: [],
  AddObserver: function AddObserver(a) {
    if (this._observers.length == 0)
      this.inner.AddObserver(this);
    this._observers.push(a);
  },
  RemoveObserver: function RemoveObserver(a) {
    this._observers = this._observers.filter(function(e) e != a);
    if (this._observers.length < 1)
      this.inner.removeObserver(this);
  },

  // nsIRDFObserver
  _callObserver: function(name, args) {
    this._observers.forEach(function(callback) {
      args = [this].concat(Array.prototype.concat.apply([], args).slice(1));
      try {
        callback[name].apply(callback, args);
      } catch(e) { /* ignore all errors */ }
    }, this);
  },
  onAssert:           function() {this._callObserver("onAssert",           arguments)},
  onUnassert:         function() {this._callObserver("onUnassert",         arguments)},
  onChange:           function() {this._callObserver("onChange",           arguments)},
  onMove:             function() {this._callObserver("onMove",             arguments)},
  onBeginUpdateBatch: function() {this._callObserver("onBeginUpdateBatch", arguments)},
  onEndUpdateBatch:   function() {this._callObserver("onEndUpdateBatch",   arguments)},

  // nsIRDFRemoteDataSource
  get loaded() { return this.inner.loaded; },
  Init: function Init(a) { this.inner.Init(a); },
  Refresh: function Refresh(a) { this.inner.Refresh(a); },
  Flush: function Flush() { this.inner.Flush(); },
  FlushTo: function FlushTo(a) { this.inner.FlushTo(a); }
};


/* an RDF observer that handles automatic saving of RDF state */
function dsSaver(ds, frequency) {
  this.ds = ds.QueryInterface(Ci.nsIRDFDataSource);
  this.ds.QueryInterface(Ci.nsIRDFRemoteDataSource);

  this.dirty = false; // has the DS changed since it was last saved?

  this.timer = Cc['@mozilla.org/timer;1'].createInstance(Ci.nsITimer);
  this.timer.init(this, frequency, Ci.nsITimer.TYPE_REPEATING_SLACK);

  this.ds.AddObserver(this);
}
dsSaver.prototype = {
  save: function dsSaver_save() {
    if (this.dirty) {
      this.ds.Flush();
      this.dirty = false;
    }
  },

  finalize: function dsSaver_finalize() {
    this.save();
    this.ds.RemoveObserver(this);
    this.timer.cancel();
    this.timer = null;
  },

  /* nsISupports */
  QueryInterface: function dsSaver_QueryInterface(iid) {
    if (!iid.equals(Ci.nsIRDFObserver) &&
        !iid.equals(Ci.nsIObserver) &&
        !iid.equals(Ci.nsISupports)) {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
  },

  /* nsIObserver */
  observe: function dsSaver_observe(subject, topic, data) {
    switch (topic) {
      case 'timer-callback':
        if (subject == this.timer) {
          this.save();
        }
        break;
    }
  },

  /* nsIRDFObserver */
  onAssert: function dsSaver_onAssert(a, b, c, d) {
    this.dirty = true;
  },
  onUnassert: function dsSaver_onUnassert(a, b, c, d) {
    this.dirty = true;
  },
  onChange: function dsSaver_onChange(a, b, c, d, e) {
    this.dirty = true;
  },
  onMove: function dsSaver_onMove(a, b, c, d, e) {
    this.dirty = true;
  },
  onBeginUpdateBatch: function dsSaver_onBeginUpdateBatch(a) { },
  onEndUpdateBatch: function dsSaver_onEndUpdateBatch(a) { }
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
