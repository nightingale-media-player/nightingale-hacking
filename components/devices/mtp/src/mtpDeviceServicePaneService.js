/**
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

Components.utils.import("resource://app/components/sbProperties.jsm");
Components.utils.import("resource://app/components/sbLibraryUtils.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;

var mtpServicePaneServiceConfig = {
  className:      "Songbird MTP Device Support Module",
  cid:            Components.ID("{e0296079-a13a-4f5d-94c6-8d7a8f5c3c91}"),
  contractID:     "@songbirdnest.com/servicepane/mtpDevice;1",
  
  ifList: [ Ci.sbIServicePaneModule,
            Ci.nsIObserver ],
            
  categoryList:
  [
    {
      category: "service-pane",
      entry: "mtp-device"
    }
  ],

  devCatName:     "Media Transfer Protocol Device",

  devURNPrefix:   "urn:device:",
  libURNPrefix:   "urn:library:",
  itemURNPrefix:  "urn:item:",

  appQuitTopic:       "quit-application",

  devImgURL:      "chrome://songbird/skin/icons/icon-device.png",
  devBusyImgURL:  "chrome://songbird/skin/icons/icon-busy.png",
  devMgrURL:      "chrome://mtp/content/xul/mtpDeviceSummaryPage.xul",
};

function mtpServicePaneService() {

}

mtpServicePaneService.prototype.constructor = mtpServicePaneService;

mtpServicePaneService.prototype = {
  classDescription: mtpServicePaneServiceConfig.className,
  classID: mtpServicePaneServiceConfig.cid,
  contractID: mtpServicePaneServiceConfig.contractID,

  _cfg: mtpServicePaneServiceConfig,
  _xpcom_categories: mtpServicePaneServiceConfig.categoryList,
  
  _deviceManagerSvc:      null,
  _deviceServicePaneSvc:  null,
  _observerSvc:           null,
  _servicePaneSvc:        null,

  // ************************************
  // sbIServicePaneService implementation
  // ************************************
  servicePaneInit: function mtpServicePaneService_servicePaneInit(aServicePaneService) {
    this._servicePaneSvc = aServicePaneService;
    this._initialize();
  },

  fillContextMenu: function mtpServicePaneService_fillContextMenu(aNode,
                                                                  aContextMenu,
                                                                  aParentWindow) {
  },

  fillNewItemMenu: function mtpServicePaneService_fillNewItemMenu(aNode,
                                                                  aContextMenu,
                                                                  aParentWindow) {
  },

  onSelectionChanged: function mtpServicePaneService_onSelectionChanged(aNode,
                                                          aContainer,
                                                          aParentWindow) {
  },

  canDrop: function mtpServicePaneService_canDrop(aNode, 
                                                  aDragSession, 
                                                  aOrientation, 
                                                  aWindow) {
    return false;
  },

  onDrop: function mtpServicePaneService_onDrop(aNode, 
                                                aDragSession, 
                                                aOrientation, 
                                                aWindow) {
  },

  onDragGesture: function mtpServicePaneService_onDragGesture(aNode, 
                                                              aTransferable) {
  },

  onRename: function mtpServicePaneService_onRename(aNode, 
                                                    aNewName) {
  },

  // ************************************
  // nsIObserver implementation
  // ************************************
  observe: function mtpServicePaneService_observe(aSubject, 
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
  QueryInterface: XPCOMUtils.generateQI(mtpServicePaneServiceConfig.ifList),
  
  // ************************************
  // Internal methods
  // ************************************
  _initialize: function mtpServicePaneService_initialize() {
    this._observerSvc = Cc["@mozilla.org/observer-service;1"]
                          .getService(Ci.nsIObserverService);
    
    this._observerSvc.addObserver(this, this._cfg.appQuitTopic, false);

    this._deviceServicePaneSvc = Cc["@songbirdnest.com/servicepane/device;1"]
                                   .getService(Ci.sbIDeviceServicePaneService);

    this._libServicePaneSvc = Cc["@songbirdnest.com/servicepane/library;1"]
                                .getService(Ci.sbILibraryServicePaneService);
                                
    this._deviceManagerSvc = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                               .getService(Ci.sbIDeviceManager2);
    
    var deviceEventListener = {
      mtpDeviceServicePaneSvc: this,
      
      onDeviceEvent: function deviceEventListener_onDeviceEvent(aDeviceEvent) {
        dump(aDeviceEvent);
      }
    };
    
    this._deviceEventListener = deviceEventListener;
    this._deviceManagerSvc.addEventListener(deviceEventListener);
  },
  
  _shutdown: function mtpServicePaneService_shutdown() {
    this._observerSvc.removeObserver(this, this._cfg.appQuitTopic);

    this._deviceManagerSvc.removeEventListener(this._deviceEventListener);
    this._deviceEventListener = null;
    
    this._deviceManagerSvc = null;
    this._deviceServicePaneSvc = null;
    this._libServicePaneSvc = null;
    this._observerSvc = null;
  },
  
  _processDeviceEvent: function mtpServicePaneService_processDeviceEvent(aDeviceEvent) {
    dump(aDeviceEvent);
    
    switch(aDeviceEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_ADDED: {
        this._addDevice();
      }
      break;
      
      default:
    }
  },
  
  _addDevice: function mtpServicePaneService_addDevice() {
    dump("XXXAus: addDevice!!!\n\n");
  },
  
  _removeDevice: function mtpServicePaneService_removeDevice() {
  
  }
};


function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([mtpServicePaneService]);
}
