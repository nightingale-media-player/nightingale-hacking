/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/sbStorageFormatter.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://app/jsmodules/WindowUtils.jsm");

const CDRIPNS = 'http://songbirdnest.com/rdf/servicepane/cdrip#';
const SPNS = 'http://songbirdnest.com/rdf/servicepane#';

const TYPE_X_SB_TRANSFER_MEDIA_LIST = "application/x-sb-transfer-media-list";

// Internal defines for the state of the cd for the context menu display.
const DEVICE_STATE_IDLE     = 0; // CD Inserted but nothing is happening
const DEVICE_STATE_LOOKUP   = 1; // A lookup for cd information in progress
const DEVICE_STATE_RIPPING  = 2; // Ripping tracks from a CD
const DEVICE_STATE_RIP_OK   = 3; // All selected tracks ripped from CD
const DEVICE_STATE_RIP_FAIL = 4; // Some selected tracks failed to rip from CD

var sbCDRipServicePaneServiceConfig = {
  className:      "Songbird CD Rip Device Support Module",
  cid:            Components.ID("{5f96ae26-3c6e-41d1-80de-d174b3b8673a}"),
  contractID:     "@songbirdnest.com/servicepane/cdRipDevice;1",
  
  ifList: [ Ci.sbIServicePaneModule,
            Ci.nsIObserver ],
            
  categoryList:
  [
    {
      category: "service-pane",
      entry: "cdrip-device"
    }
  ],

  devCatName:     "CD Rip Device",

  devURNPrefix:   "urn:device:",
  libURNPrefix:   "urn:library:",
  itemURNPrefix:  "urn:item:",

  appQuitTopic:   "quit-application",

  devMgrURL:      "chrome://songbird/content/mediapages/cdRipMediaView.xul"
};

function sbCDRipServicePaneService() {

}

sbCDRipServicePaneService.prototype.constructor = sbCDRipServicePaneService;

sbCDRipServicePaneService.prototype = {
  classDescription: sbCDRipServicePaneServiceConfig.className,
  classID: sbCDRipServicePaneServiceConfig.cid,
  contractID: sbCDRipServicePaneServiceConfig.contractID,

  _cfg: sbCDRipServicePaneServiceConfig,
  _xpcom_categories: sbCDRipServicePaneServiceConfig.categoryList,

  // Services to use.
  _observerSvc:           null,
  _servicePaneSvc:        null,
  _cdSvc:                 null,

  // ************************************
  // sbIServicePaneService implementation
  // ************************************
  servicePaneInit: function sbCDRipServicePaneService_servicePaneInit(aServicePaneService) {
    this._servicePaneSvc = aServicePaneService;
    this._initialize();
  },

  fillContextMenu: function sbCDRipServicePaneService_fillContextMenu(aNode,
                                                                  aContextMenu,
                                                                  aParentWindow) {
  },

  fillNewItemMenu: function sbCDRipServicePaneService_fillNewItemMenu(aNode,
                                                                  aContextMenu,
                                                                  aParentWindow) {
  },

  onSelectionChanged: function sbCDRipServicePaneService_onSelectionChanged(aNode,
                                                          aContainer,
                                                          aParentWindow) {
  },

  canDrop: function sbCDRipServicePaneService_canDrop(aNode, 
                                                  aDragSession, 
                                                  aOrientation, 
                                                  aWindow) {
    // Currently no drag and drop allowed
    return false;
  },

  onDrop: function sbCDRipServicePaneService_onDrop(aNode, 
                                                aDragSession, 
                                                aOrientation, 
                                                aWindow) {
    // Currently no drag and drop allowed
  },
  
  onDragGesture: function sbCDRipServicePaneService_onDragGesture(aNode, 
                                                              aTransferable) {
    // Currently no drag and drop allowed
  },

  onRename: function sbCDRipServicePaneService_onRename(aNode, 
                                                    aNewName) {
    // Rename is not allowed for CD Devices
  },

  shutdown: function sbCDRipServicePaneService_shutdown() {
    // Do nothing, since we shut down on quit-application
  },

  // ************************************
  // nsIObserver implementation
  // ************************************
  observe: function sbCDRipServicePaneService_observe(aSubject, 
                                                  aTopic, 
                                                  aData) {
    switch (aTopic) {
      case this._cfg.appQuitTopic :
        this._shutdown();
      break;
    }

  },

  // ************************************
  // nsISupports implementation
  // ************************************
  QueryInterface: XPCOMUtils.generateQI(sbCDRipServicePaneServiceConfig.ifList),

  // ************************************
  // Internal methods
  // ************************************
  
  /**
   * \brief Initialize the CD Device nodes.
   */
  _initialize: function sbCDRipServicePaneService_initialize() {
    this._observerSvc = Cc["@mozilla.org/observer-service;1"]
                          .getService(Ci.nsIObserverService);
    
    this._observerSvc.addObserver(this, this._cfg.appQuitTopic, false);

    // Remove all stale nodes.
    this._removeCDDeviceNodes(this._servicePaneSvc.root);
    
    // TODO: Get the CD Service
    // this._cdSvc = Cc[""]
    //                 .getService(Ci.sbICD);
    
    // TODO: Query CD Service for existing devices
    //       For each device that has a valid CD in it we will create a node
    
    // TODO: Hook up with the cd service and listen for add/remove events
    
    // Test add a device node
    this._addCDDevice(null);
  },
  
  _shutdown: function sbCDRipServicePaneService_shutdown() {
    this._observerSvc.removeObserver(this, this._cfg.appQuitTopic);

    // Purge all device nodes before shutdown.
    this._removeCDDeviceNodes(this._servicePaneSvc.root);
    
    this._servicePaneSvc = null;
    this._observerSvc = null;
  },

  /**
   * \brief Adds a CD Device node to the service pane.
   * \param aCDDevice - CD Device information to add a node with.
   */
  _addCDDevice: function sbCDRipServicePaneService_addDevice(aCDDevice) {
    // Not available until sbICD is implemented
    //var device = aCDDevice.QueryInterface(Ci.sbICDDevice);
    
    /*
     * Currently this is only a test node until the CD service is in place.
     * The following code will be replaced with proper node creation based
     * on a proper cd device.
     */
    // Add a cd rip node in the service pane.
    //var devNode = this._deviceServicePaneSvc.createNodeForDevice2(device);
    var node = this._servicePaneSvc.addNode("test-cdrip",
                                            this._servicePaneSvc.root,
                                            false);
    if (!node) {
      // Already exists so return
      return;
    }
    
    // Set up the node properties
    node.setAttributeNS(SPNS, "Weight", 5);
    node.properties = "device";
    node.setAttributeNS(CDRIPNS, "CDId", 0);
    node.contractid = this._cfg.contractID;
    node.url = this._cfg.devMgrURL + "?cd-id=" + 0;
    node.editable = false;
    node.name = "CD Rip"; // TODO: Localize.
    node.hidden = false;
  },
  
  /**
   * \brief Removes all the CD Device nodes from the service pane.
   * \param aNode - Root node to remove, if it is a container the children will
   *                be checked first. Nodes are only removed if they match this
   *                services contract id.
   */
  _removeCDDeviceNodes: function sbCDRipServicePaneService_removeCDDeviceNodes(
                                                                        aNode) {
    // Remove child device nodes.
    if (aNode.isContainer) {
      var childEnum = aNode.childNodes;
      while (childEnum.hasMoreElements()) {
        var child = childEnum.getNext().QueryInterface(Ci.sbIServicePaneNode);
        this._removeCDDeviceNodes(child);
      }
    }

    // Remove cd device node.
    if (aNode.contractid == this._cfg.contractID)
      aNode.hidden = true;
  }
};

// Instantiate an XPCOM module.
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbCDRipServicePaneService]);
}
