/* vim: set sw=2 :miv */
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

/**
 * \brief Device Firmware Tests
 */

function runTest () {
  var device = Components.classes["@songbirdnest.com/Songbird/Device/DeviceTester/MockDevice;1"]
                         .createInstance(Components.interfaces.sbIDevice);
  assertEqual(device.name, "Bob's Mock Device");
  
  var updater = Components.classes["@songbirdnest.com/Songbird/Device/Firmware/Updater;1"]
                          .getService(Components.interfaces.sbIDeviceFirmwareUpdater);
  assertTrue(updater.hasHandler(device, 0, 0));

  var handler = updater.getHandler(device, 0, 0);
  assertNotEqual(handler, null);
  
  var listener = {
    op: "", 
    firmwareUpdate: null,
    onDeviceEvent: function(aEvent) {
      log("event type: " + aEvent.type);
      log("event origin: " + aEvent.origin);
      log("event target: " + aEvent.target);
      
      var data = aEvent.data;
      log("event data: " + data);
    
      switch(this.op) {
        case "cfu":
          if(aEvent.type == Ci.sbIDeviceEvent.EVENT_FIRMWARE_CFU_END) {
            log("is update available: " + data);
            testFinished();
          }
        break;
        
        case "du": {
          if(aEvent.type == Ci.sbIDeviceEvent.EVENT_FIRMWARE_DOWNLOAD_START) {
            log("firmware update download start");
          }
          else if(aEvent.type == Ci.sbIDeviceEvent.EVENT_FIRMWARE_DOWNLOAD_PROGRESS) {
            log("firmware update download progress: " + data);
          }
          if(aEvent.type == Ci.sbIDeviceEvent.EVENT_FIRMWARE_DOWNLOAD_END) {
            this.firmwareUpdate = data.QueryInterface(Ci.sbIDeviceFirmwareUpdate);
            log("firmware update version: " + this.firmwareUpdate.firmwareReadableVersion);
            log("firmware update file path: " + this.firmwareUpdate.firmwareImageFile.path);
            testFinished();
          }
        }
        break;
        
        case "au": {
          if(aEvent.type == Ci.sbIDeviceEvent.EVENT_FIRMWARE_UPDATE_START) {
            log("firmware apply update start");
          }
          else if(aEvent.type == Ci.sbIDeviceEvent.EVENT_FIRMWARE_WRITE_START) {
            log("firmware write start");
          }
          else if(aEvent.type == Ci.sbIDeviceEvent.EVENT_FIRWMARE_WRITE_PROGRESS) {
            log("firmware write progress: " + data);
          }
          else if(aEvent.type == Ci.sbIDeviceEvent.EVENT_FIRMWARE_WRITE_END) {
            log("firmware write end");
          }
          else if(aEvent.type == Ci.sbIDeviceEvent.EVENT_FIRMWARE_UPDATE_END) {
            log("firmware apply update end");
            testFinished();
          }          
        }
        break;
        
        default:
          log("unknown operation");
          testFinished();
      }
    }
  };
  
  try {
    log("Testing 'checkForUpdate'");
    listener.op = "cfu";
    
    var eventTarget = device.QueryInterface(Ci.sbIDeviceEventTarget);
    eventTarget.addEventListener(listener);
    
    updater.checkForUpdate(device, null);
    testPending();
      
    log("Testing 'downloadUpdate'");
    listener.op = "du";
    
    updater.downloadUpdate(device, false, null);
    testPending();
    
    log("Testing 'applyUpdate'");
    listener.op = "au";
    
    updater.applyUpdate(device, listener.firmwareUpdate, null);
    testPending();
      
    updater.finalizeUpdate(device);
  }
  finally {
    eventTarget.removeEventListener(listener);
  }
 
  return;
}
