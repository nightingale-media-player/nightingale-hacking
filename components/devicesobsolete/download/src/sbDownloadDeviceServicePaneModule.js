/*
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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/components/sbProperties.jsm");

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

function sbDownloadDeviceServicePaneModule()
{
  this._servicePaneNode = null;
  this._downloadDevice = null;
  this._currentState = Ci.sbIDeviceBase.STATE_IDLE;
}

sbDownloadDeviceServicePaneModule.prototype =
{
  classDescription: "Songbird Download Device Service Pane Module",
  classID:          Components.ID("{ee93796b-090e-4703-982a-1d27bea552c3}"),
  contractID:       "@songbirdnest.com/Songbird/DownloadDeviceServicePaneModule;1",
  QueryInterface:   XPCOMUtils.generateQI([Ci.sbIServicePaneModule,
                                           Ci.sbIDeviceBaseCallback])
}

// sbIServicePaneModule
sbDownloadDeviceServicePaneModule.prototype.servicePaneInit =
function sbDownloadDeviceServicePaneModule_servicePaneInit(aServicePaneModule)
{
  var devMgr = Cc["@songbirdnest.com/Songbird/DeviceManager;1"]
                 .getService(Ci.sbIDeviceManager);
  var downloadCat = "Songbird Download Device";
  if (devMgr.hasDeviceForCategory(downloadCat)) {
    this._downloadDevice = devMgr.getDeviceByCategory(downloadCat)
                                 .QueryInterface(Ci.sbIDownloadDevice);
  }
  this._downloadDevice.addCallback(this);
  this._currentState = this._downloadDevice.getDeviceState("download");
  this._updateState();
}

sbDownloadDeviceServicePaneModule.prototype.fillContextMenu =
function sbDownloadDeviceServicePaneModule_fillContextMenu(aNode,
                                                           aContextMenu,
                                                           aParentWindow)
{
}

sbDownloadDeviceServicePaneModule.prototype.fillNewItemMenu =
function sbDownloadDeviceServicePaneModule_fillNewItemMenu(aNode,
                                                           aContextMenu,
                                                           aParentWindow)
{
}

sbDownloadDeviceServicePaneModule.prototype.onSelectionChanged =
function sbDownloadDeviceServicePaneModule_onSelectionChanged(aNode,
                                                              aContainer,
                                                              aParentWindow)
{
}

sbDownloadDeviceServicePaneModule.prototype.canDrop =
function sbDownloadDeviceServicePaneModule_canDrop(aNode,
                                                   aDragSession,
                                                   aOrientation)
{
}

sbDownloadDeviceServicePaneModule.prototype.onDrop =
function sbDownloadDeviceServicePaneModule_onDrop(aNode,
                                                  aDragSession,
                                                  aOrientation)
{
}

sbDownloadDeviceServicePaneModule.prototype.onDragGesture =
function sbDownloadDeviceServicePaneModule_onDragGesture(aNode,
                                                         aTransferable)
{
}

sbDownloadDeviceServicePaneModule.prototype.onRename =
function sbDownloadDeviceServicePaneModule_onRename(aNode,
                                                    aNewName)
{
}

sbDownloadDeviceServicePaneModule.prototype.__defineGetter__("stringBundle",
function sbDownloadDeviceServicePaneModule_get_stringBundle()
{
});

sbDownloadDeviceServicePaneModule.prototype.shutdown =
function sbDownloadDeviceServicePaneModule_shutdown()
{
  if (this._downloadDevice) {
    this._downloadDevice.removeCallback(this);
  }
}

// sbIDeviceBaseCallback
sbDownloadDeviceServicePaneModule.prototype.onDeviceConnect =
function sbDownloadDeviceServicePaneModule_onDeviceConnect(aDeviceIdentifier)
{
}

sbDownloadDeviceServicePaneModule.prototype.onDeviceDisconnect =
function sbDownloadDeviceServicePaneModule_onDeviceDisconnect(aDeviceIdentifier)
{
}

sbDownloadDeviceServicePaneModule.prototype.onTransferStart =
function sbDownloadDeviceServicePaneModule_onTransferStart(aSourceURL,
                                                           aDestinationURL)
{
}

sbDownloadDeviceServicePaneModule.prototype.onTransferComplete =
function sbDownloadDeviceServicePaneModule_onTransferComplete(aSourceURL,
                                                              aDestinationURL)
{
}

sbDownloadDeviceServicePaneModule.prototype.onStateChanged =
function sbDownloadDeviceServicePaneModule_onStateChanged(aDeviceIdentifier,
                                                          aState)
{
  this._currentState = aState;
  this._updateState();
}

sbDownloadDeviceServicePaneModule.prototype.__defineGetter__("_node",
function sbDownloadDeviceServicePaneModule_get_node()
{
  if (!this._servicePaneNode) {
    var lsps = Cc["@songbirdnest.com/servicepane/library;1"]
                 .getService(Ci.sbILibraryServicePaneService);
    this._servicePaneNode =
      lsps.getNodeForLibraryResource(this._downloadDevice.downloadMediaList);
  }
  return this._servicePaneNode;
});

sbDownloadDeviceServicePaneModule.prototype._updateState =
function sbDownloadDeviceServicePaneModule_onStateChanged()
{
  if (this._node) {
    var a = this._node.properties.split(" ");

    // Remove the "progress" property from the array, if any
    a = a.filter(function(property) {
      return property != "downloading";
    });

    if (this._currentState == Ci.sbIDeviceBase.STATE_DOWNLOADING) {
      a.push("downloading");
    }

    this._node.properties = a.join(" ");
  }
}

function NSGetModule(compMgr, fileSpec) {
  var module = XPCOMUtils.generateModule(
    [sbDownloadDeviceServicePaneModule],
      function(aCompMgr, aFileSpec, aLocation) {
      XPCOMUtils.categoryManager.addCategoryEntry(
        "service-pane",
        "z-download-device-service-pane-module",
        sbDownloadDeviceServicePaneModule.prototype.contractID,
        true,
        true);
      }
    );
  return module;
}

