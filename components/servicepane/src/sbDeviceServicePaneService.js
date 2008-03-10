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
 * \file sbDeviceServicePane.js
 */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const CONTRACTID = "@songbirdnest.com/servicepane/device;1";

const NC='http://home.netscape.com/NC-rdf#';
const SP = "http://songbirdnest.com/rdf/servicepane#";

const URN_PREFIX_DEVICE = "urn:device:";
const DEVICE_NODE_WEIGHT = -2

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
 * /class sbDeviceServicePane
 * /brief Provides device related nodes for the service pane
 * \sa sbIServicePaneService sbIDeviceServicePaneService
 */
function sbDeviceServicePane() {
  this._servicePane = null;

  // use the default stringbundle to translate tree nodes
  this.stringbundle = null;
}
sbDeviceServicePane.prototype.QueryInterface =
function sbDeviceServicePane_QueryInterface(iid) {
  if (!iid.equals(Ci.nsISupports) &&
    !iid.equals(Ci.sbIServicePaneModule) &&
    !iid.equals(Ci.sbIDeviceServicePaneService)) {
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
  return this;
}


//////////////////////////
// sbIServicePaneModule //
//////////////////////////

sbDeviceServicePane.prototype.servicePaneInit =
function sbDeviceServicePane_servicePaneInit(sps) {
  //logcall(arguments);

  // keep track of the service pane service
  this._servicePane = sps;
}

sbDeviceServicePane.prototype.shutdown =
function sbDeviceServicePane_shutdown() {
}

sbDeviceServicePane.prototype.fillContextMenu =
function sbDeviceServicePane_fillContextMenu(aNode, aContextMenu, aParentWindow) {
}

sbDeviceServicePane.prototype.fillNewItemMenu =
function sbDeviceServicePane_fillNewItemMenu(aNode, aContextMenu, aParentWindow) {
}

sbDeviceServicePane.prototype.onSelectionChanged =
function sbDeviceServicePane_onSelectionChanged(aNode, aContainer, aParentWindow) {
}

sbDeviceServicePane.prototype.canDrop =
function sbDeviceServicePane_canDrop(aNode, aDragSession, aOrientation, aWindow) {
  return false;
}

sbDeviceServicePane.prototype.onDrop =
function sbDeviceServicePane_onDrop(aNode, aDragSession, aOrientation, aWindow) {
}

sbDeviceServicePane.prototype.onDragGesture =
function sbDeviceServicePane_onDragGesture(aNode, aTransferable) {
  return false;
}


/**
 * Called when the user has attempted to rename a library/medialist node
 */
sbDeviceServicePane.prototype.onRename =
function sbDeviceServicePane_onRename(aNode, aNewName) {
}


//////////////////////////////////
// sbIDeviceServicePaneService //
//////////////////////////////////

sbDeviceServicePane.prototype.createNodeForDevice =
function sbDeviceServicePane_createNodeForDevice(aDevice, aDeviceIdentifier) {
  //logcall(arguments);

  // Get the Node.
  var id = this._deviceURN(aDevice, aDeviceIdentifier);
  var node = this._servicePane.getNode(id);
  if (!node) {
    // Create the node
    node = this._servicePane.addNode(id, this._servicePane.root, true);
  }

  // Refresh the information just in case it is supposed to change
  node.contractid = CONTRACTID;
  node.setAttributeNS(SP, "Weight", DEVICE_NODE_WEIGHT);
  node.contractid = CONTRACTID;
  node.editable = false;

  // Sort node into position.
  this._servicePane.sortNode(node);

  return node;
}

sbDeviceServicePane.prototype.createNodeForDevice2 =
function sbDeviceServicePane_createNodeForDevice2(aDevice) {
  // Get the Node.
  var id = this._deviceURN2(aDevice);
  
  var node = this._servicePane.getNode(id);
  if (!node) {
    // Create the node
    node = this._servicePane.addNode(id, this._servicePane.root, true);
  }

  // Refresh the information just in case it is supposed to change
  node.contractid = CONTRACTID;
  node.setAttributeNS(SP, "Weight", DEVICE_NODE_WEIGHT);
  node.contractid = CONTRACTID;
  node.editable = false;
  node.properties = "device";

  try {
    var iconUri = aDevice.properties.iconUri;
    if (iconUri) {
      var spec = iconUri.spec;
      if (iconUri.schemeIs("file") && /\.ico$/i(spec)) {
        // for *.ico, try to get the small icon
        spec = "moz-icon://" + spec + "?size=16";
      }
      node.image = spec;
    }
  } catch(ex) {
    /* we do not care if setting the icon fails */
  }

  // Sort node into position.
  this._servicePane.sortNode(node);

  return node;
}

/////////////////////
// Private Methods //
/////////////////////

/**
 * Get a service pane identifier for the given device
 */
sbDeviceServicePane.prototype._deviceURN =
function sbDeviceServicePane__deviceURN(aDevice, aDeviceIdentifier) {
  return URN_PREFIX_DEVICE + aDevice.deviceCategory + ":" + aDeviceIdentifier;
}

sbDeviceServicePane.prototype._deviceURN2 =
function sbDeviceServicePane__deviceURN2(aDevice) {
  var id = "" + aDevice.id;
  
  if(id.charAt(0) == "{" &&
     id.charAt(-1) == "}") {
    id = id.substring(1, -1);
  }
  
  
  return URN_PREFIX_DEVICE + id;
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
  sbDeviceServicePane,
  Components.ID("{845c31ee-c30e-4fb6-9667-0b10e58c7069}"),
  "Songbird Device Service Pane Service",
  CONTRACTID,
  [{
    category: 'service-pane',
    entry: 'device', // we want this to load first
    value: CONTRACTID
  }]);
