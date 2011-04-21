/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
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

const TEST_RUNNING = 0;
const TEST_COMPLETED = 1;
const TEST_FAILED = 2;

var testStatus = TEST_RUNNING;
var testFailMessage = "";

 
/**
 * \brief Device tests - Mock device
 */

function checkPropertyBag(aBag, aParams) {
  for (var name in aParams) {
    var val;
    try {
      val = aBag.getProperty(name);
    } catch (e) {
      log('Failed to get property "' + name + '"');
      throw(e);
    }
    assertTrue(val, 'Cannot find property "' + name + '"');
    if (typeof(aParams[name]) == "object" && "wrappedJSObject" in aParams[name])
      val = val.wrappedJSObject;
    assertEqual(aParams[name],
                val,
                'property "' + name + '" not equal');
    log('"' + name + '" is ' + val);
  }
}

function runTest () {
  let device = Components.classes["@songbirdnest.com/Songbird/Device/DeviceTester/MockDevice;1"]
                         .createInstance(Components.interfaces.sbIDevice);
  // Check basic device properties
  assertEqual(device.name, "Bob's Mock Device");
  assertEqual(device.productName, "Mock Device");
  
  assertEqual("" + device.id, "" + device.id, "device ID not equal");
  assertEqual("" + device.controllerId, "" + device.controllerId, "controller ID not equal");
  
  // Make sure device is initialized and disconnected on creation
  assertFalse(device.connected);
  
  // Check connect
  device.connect();
  assertTrue(device.connected);
  
  // Attempt a reconnect, this is expected to throw, if it doesn't then that
  // is a failure
  try {
    device.connect();
    fail("Re-connected device");
  } catch(e) {
    /* expected to throw */
  }
  
  // Make sure the device is still connected
  assertTrue(device.connected);

  // For the rest of these tests we need to hook up a listener.
  // If we shutdown before the device disconnects (disconnect is asynchronous)
  // then we'll get a crash in GC land.
  // This event handler checks testStatus to see if the test is currenly running
  // or has failed or is completed. It is at the failed or completed state that
  // the handler terminates the unit test.
  device.QueryInterface(Components.interfaces.sbIDeviceEventTarget);
  var handler = function handler(event) { 
    if (event.type == Ci.sbIDeviceEvent.EVENT_DEVICE_REMOVED) {
    
      // Check the current test status and end it if the test is done
      switch (testStatus) {
        case TEST_COMPLETED:
          device.removeEventListener(handler);
          doTimeout(500, function() {
            testFinished();
            return;
          });
          return;
        case TEST_FAILED:
          device.removeEventListener(handler);
          doTimeout(500, function() {
            fail(testFailMessage);
            return;
          });
          return;
        case TEST_RUNNING:
          // Continue and perform remaining tests
        break;
        default:
          fail("Unexpected testStatus value: " + testStatus);
          return;
      }
        
      log("Device removed event fired");
      try {
        // Device was disconnected, continue on testing
        assertFalse(device.connected);

        // Mock device is now threaded.
        assertFalse(device.threaded);

        log("Reconnecting");
        device.connect();

        test_prefs(device);

        /* TODO: device.capabilities */

        /* TODO: device.content */

        /* TODO: device.parameters */

        test_properties(device);

        test_event(device);

        /**
         * This is used as a callback for the request test because it needs
         * to asynchronously continue
         */
        function continueTest()
        {
          // Make sure we're connected
          if (!device.connected)
            device.connect();
         
          // Perform library and sync settings tests
          try {
            var operation = "test_library";
            test_library(device);
            operation = "test_sync_settings";
            test_sync_settings(device);
            testStatus = TEST_COMPLETED;
          }
          catch (e) {
            testFailMessage = operation + " failed with exception: " + e;
            log(testFailMessage);
            testStatus = TEST_FAILED;
            // stop a circular reference
            if (device.connected)
              device.disconnect();
          }
          finally {
            if (testStatus != TEST_FAILED) {
              testStatus = TEST_COMPLETED;
            }
            // stop a circular reference
            if (device.connected)
              device.disconnect();
          }
        }
        test_request(device, continueTest);
        return;
      }
      catch (e) {
        testFailMessage = "test_request failed with exception: " + e;
        log(testFailMessage);
        testStatus = TEST_FAILED;
        // stop a circular reference
        if (device.connected) {
           device.disconnect();
        }
        else {
          fail();
          return;
        }
      }    
    }
  }
  device.addEventListener(handler);

  device.disconnect();
  try {
    device.disconnect();
  } catch(e) {
    fail("Re-disconnected device should not fail");
  }
  testPending();
}

function test_prefs(device) {
  log("test_prefs");
  assertTrue(typeof(device.getPreference("hello")) == "undefined");
  
  device.setPreference("world", 3);
  assertEqual(device.getPreference("world"), 3);
  assertEqual(typeof(device.getPreference("world")), "number");
  device.setPreference("world", "goat");
  assertEqual(device.getPreference("world"), "goat");
  assertEqual(typeof(device.getPreference("world")), "string");
  device.setPreference("world", true);
  assertEqual(device.getPreference("world"), true);
  assertEqual(typeof(device.getPreference("world")), "boolean");
  
  with (Components.interfaces.sbIDevice) {
    device.setPreference("state", 0);
    assertEqual(device.state, STATE_IDLE);
    device.setPreference("state", 1);
    assertEqual(device.state, STATE_SYNCING);
    device.setPreference("state", 2);
    assertEqual(device.state, STATE_COPYING);
    device.setPreference("state", 3);
    assertEqual(device.state, STATE_DELETING);
    device.setPreference("state", 4);
    assertEqual(device.state, STATE_UPDATING);
    device.setPreference("state", 5);
    assertEqual(device.state, STATE_MOUNTING);
    device.setPreference("state", 6);
    assertEqual(device.state, STATE_DOWNLOADING);
    device.setPreference("state", 7);
    assertEqual(device.state, STATE_UPLOADING);
    device.setPreference("state", 8);
    assertEqual(device.state, STATE_DOWNLOAD_PAUSED);
    device.setPreference("state", 9);
    assertEqual(device.state, STATE_UPLOAD_PAUSED);
    device.setPreference("state", 10);
    assertEqual(device.state, STATE_DISCONNECTED);
  }
}

function test_event(device) {
  log("test_event");
  /* test as a event target */
  // I didn't bother with CI on the mock device
  device.QueryInterface(Components.interfaces.sbIDeviceEventTarget);
  var wasFired = false;
  var handler = function handler() { wasFired = true; }
  device.addEventListener(handler);
  var event = Components.classes["@songbirdnest.com/Songbird/DeviceManager;2"]
                        .getService(Components.interfaces.sbIDeviceManager2)
                        .createEvent(0);
  device.dispatchEvent(event);
  assertTrue(wasFired, "event handler not called");
  
  device.removeEventListener(handler);
}

function test_request(device, continueAction) {
  log("test_request");
  /* test as sbIMockDevice (request push/pop) */
  device.QueryInterface(Ci.sbIMockDevice);
  
  // test the properties
  var item = { QueryInterface:function(){return this} };
  item.wrappedJSObject = item;
  var list = { QueryInterface:function(){return this} };
  list.wrappedJSObject = list;
  var data = { /* nothing needed */ };
  data.wrappedJSObject = data;
  var params = { item: item,
                 list: list,
                 data: data,
                 index: 999,
                 otherIndex: 1024 };
  var retryCount = 0;
  device.submitRequest(0x201dbeef, createPropertyBag(params));
  // Wait for a request to come in using a timeout
  function requestCheck() {
    let request = null;
    try {
      request = device.popRequest();
    }
    catch (e)
    {
      // exception expected, fall through with request being null
    }
    if (request) {
      // Skip the thread start request and any other built in request
      if (request.getProperty("requestType") >= 0x20000000) {
        checkPropertyBag(request, params);
        log("item transfer ID: " + request.getProperty("itemTransferID"));
        assertTrue(request.getProperty("itemTransferID") > 3,
                     "Obviously bad item transfer ID");

        request = null; /* unleak */
        continueAction();
        return;
      }      
    }
    // Limit retries to 6 seconds
    if (++retryCount > 60) {
      fail("Failed in waiting for request");
    }
    // No request, continue to wait
    doTimeout(100, requestCheck);
    return;
  }
  doTimeout(100, requestCheck);
}

function test_library(device) {
  log("test_library");
  assertEqual(
        device,
        device.content.libraries.queryElementAt(0, Ci.sbIDeviceLibrary).device);
  assertTrue(device.defaultLibrary.equals(
               device.content.libraries.queryElementAt(0, Ci.sbIDeviceLibrary)));
}

function test_set_and_verify(device, audio, video, image)
{
  let syncSettings = device.defaultLibrary.syncSettings;

  log("Changing management types to " + audio + ", " + video + ", " + image);
  
  // Get all the media settings
  syncSettings = device.defaultLibrary.syncSettings;
  
  let audioSyncSettings = syncSettings.getMediaSettings(
                                           Ci.sbIDeviceLibrary.MEDIATYPE_AUDIO);
  let videoSyncSettings = syncSettings.getMediaSettings(
                                           Ci.sbIDeviceLibrary.MEDIATYPE_VIDEO);
  let imageSyncSettings = syncSettings.getMediaSettings(
                                           Ci.sbIDeviceLibrary.MEDIATYPE_IMAGE);
                                           
  // Set the management types and verify
  audioSyncSettings.mgmtType = audio;                             
  assertEqual(audioSyncSettings.mgmtType, audio);
  videoSyncSettings.mgmtType = video;
  assertEqual(videoSyncSettings.mgmtType, video);
  imageSyncSettings.mgmtType = image;
  assertEqual(imageSyncSettings.mgmtType, image);

  device.defaultLibrary.syncSettings = syncSettings;

  // Now retrieve the media settings again and verify them  
  syncSettings = device.defaultLibrary.syncSettings;
  assertEqual(syncSettings.getMediaSettings(
                                  Ci.sbIDeviceLibrary.MEDIATYPE_AUDIO).mgmtType,
              audio);
  assertEqual(syncSettings.getMediaSettings(
                                  Ci.sbIDeviceLibrary.MEDIATYPE_VIDEO).mgmtType,
              video);
  assertEqual(syncSettings.getMediaSettings(
                                  Ci.sbIDeviceLibrary.MEDIATYPE_IMAGE).mgmtType,
              image);
}

function test_sync_settings(device) {
  log("Testing initial mode");
  let syncSettings = device.defaultLibrary.syncSettings;

  log("Changing management type to all");
  
  test_set_and_verify(device,
                      Ci.sbIDeviceLibraryMediaSyncSettings.SYNC_MGMT_ALL,
                      Ci.sbIDeviceLibraryMediaSyncSettings.SYNC_MGMT_ALL,
                      Ci.sbIDeviceLibraryMediaSyncSettings.SYNC_MGMT_ALL);

  test_set_and_verify(device,
                      Ci.sbIDeviceLibraryMediaSyncSettings.SYNC_MGMT_NONE,
                      Ci.sbIDeviceLibraryMediaSyncSettings.SYNC_MGMT_NONE,
                      Ci.sbIDeviceLibraryMediaSyncSettings.SYNC_MGMT_NONE);
}

function test_properties(device) {
  var properties = device.properties;
  
  log("friendlyName: " + properties.friendlyName);
  assertEqual(properties.friendlyName, "Testing Device");
  log("serialNumber: " + properties.serialNumber);
  assertEqual(properties.serialNumber, "ACME-9000-0001-2000-3000");
  log("modelNumber: " + properties.modelNumber);
  assertEqual(properties.modelNumber, "ACME 9000");
  log("vendorName: " + properties.vendorName);
  assertEqual(properties.vendorName, "ACME Inc.");
  log("firmwareVersion: " + properties.firmwareVersion);
  assertEqual(properties.firmwareVersion, "1.0.0.0");
  
  properties.friendlyName = "New Friendly Test Device";
  assertEqual(properties.friendlyName, "New Friendly Test Device");
}

function createPropertyBag(aParams) {
  var bag = Cc["@mozilla.org/hash-property-bag;1"]
              .createInstance(Ci.nsIWritablePropertyBag);
  for (var name in aParams) {
    bag.setProperty(name, aParams[name]);
  }
  return bag;
}
