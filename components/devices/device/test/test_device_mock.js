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
 * \brief Device tests - Mock device
 */

function runTest () {
  var device = Components.classes["@songbirdnest.com/Songbird/Device/DeviceTester/MockDevice;1"]
                         .createInstance(Components.interfaces.sbIDevice);
  assertEqual(device.name, "Bob's Mock Device");
  
  assertEqual("" + device.id, "" + device.id, "device ID not equal");
  assertEqual("" + device.controllerId, "" + device.controllerId, "controller ID not equal");
  
  assertFalse(device.connected);
  
  device.connect();
  assertTrue(device.connected);
  try {
    device.connect();
    fail("Re-connected device");
  } catch(e){
    /* expected to throw */
  }
  assertTrue(device.connected);

  device.disconnect();
  assertFalse(device.connected);
  try {
    device.disconnect();
    fail("Re-disconnected device");
  } catch(e) {
    /* expected to throw */
  }
  assertFalse(device.connected);
  
  assertFalse(device.threaded);
  
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
  
  /* TODO: device.capabilities */
  
  /* TODO: device.content */
  
  /* TODO: device.parameters */
 
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
