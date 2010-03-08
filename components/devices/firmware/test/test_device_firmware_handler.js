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
 * Unit test constants
 */

var TEST_FILE_LOCATION = "testharness/devicefirmware/files/";
var PORT_NUMBER        = getTestServerPortNumber();
var REMOTE_FILE_PREFIX = "http://localhost:" + PORT_NUMBER + "/files/";


/**
 * Get a temporary folder for use in metadata tests.
 * Will be removed in tail_metadatamanager.js
 */
var gTempFolder = null;
function getTempFolder() {
  if (gTempFolder) {
    return gTempFolder;
  }
  gTempFolder = Components.classes["@mozilla.org/file/directory_service;1"]
                       .getService(Components.interfaces.nsIProperties)
                       .get("TmpD", Components.interfaces.nsIFile);
  gTempFolder.append("songbird_metadata_tests.tmp");
  gTempFolder.createUnique(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0777);
  return gTempFolder;
}


/**
 * \brief Copy the given folder to tempName, returning an nsIFile
 * for the new location
 */
function getCopyOfFolder(folder, tempName) {
  assertNotEqual(folder, null);
  var tempFolder = getTempFolder();
  folder.copyTo(tempFolder, tempName);
  folder = tempFolder.clone();
  folder.append(tempName);
  assertEqual(folder.exists(), true);
  return folder;
}


/**
 * \brief Device Firmware Tests
 */

function runTest () {
  // Setup the JS server to serve up the mock test handler files.
  var testFolder = getCopyOfFolder(newAppRelativeFile(TEST_FILE_LOCATION,
                                                      "_temp_firmware_files"));

  var jsHttpServer = Cc["@mozilla.org/server/jshttp;1"]
                       .createInstance(Ci.nsIHttpServer);
  jsHttpServer.start(PORT_NUMBER);
  jsHttpServer.registerDirectory("/", testFolder.clone()); 
  
  // Set the remote URL's for the firmware files via the JS HTTP server.
  var handlerURLService = Cc["@songbirdnest.com/mock-firmware-url-handler;1"]
                            .getService(Ci.sbPIMockFirmwareHandlerURLService);

  handlerURLService.firmwareURL = REMOTE_FILE_PREFIX + "firmware.xml";
  handlerURLService.resetURL = REMOTE_FILE_PREFIX + "reset.html";
  handlerURLService.releaseNotesURL = REMOTE_FILE_PREFIX + "release_notes.html";
  handlerURLService.supportURL = REMOTE_FILE_PREFIX + "support.html";
  handlerURLService.registerURL = REMOTE_FILE_PREFIX + "register.html";
  
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

  jsHttpServer.stop();
  return;
}
