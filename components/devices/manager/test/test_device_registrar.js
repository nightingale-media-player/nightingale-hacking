/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

/**
 * \brief Device Manager tests - Device Registrar
 */

Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
var gUUIDGenerator = Components.classes["@mozilla.org/uuid-generator;1"]
                               .createInstance(Components.interfaces.nsIUUIDGenerator);


function DummyDevice(aParam) {
  this.wrappedJSObject = this;
  this.param = aParam;
  this.id = gUUIDGenerator.generateUUID();
  this.connected = false;
  this.connect = function(){
    this.connected = true;
  }
}

function runTest () {
  var manager = Components.classes["@getnightingale.com/Nightingale/DeviceManager;2"]
                          .getService(Components.interfaces.sbIDeviceManager2);

  var params = [];
  
  // register some dummy devices
  for (var i = 0; i < 1000; ++i) {
    var param = Math.random();
    params.push(param);
    manager.registerDevice(new DummyDevice(param));
  }
  
  // put in a special device to test getDevice
  var device = new DummyDevice(0);
  manager.registerDevice(device);

  // fill in some more dummy data
  for (var i = 0; i < 1000; ++i) {
    param = Math.random();
    params.push(param);
    manager.registerDevice(new DummyDevice(param));
  }

  // test getDevice
  var returnedDevice = manager.getDevice(device.id);
  assertTrue(device === returnedDevice.wrappedJSObject,
             "failed to get device:" +
             device.id + " / " + returnedDevice.id);
  manager.unregisterDevice(device);
  assertTrue(returnedDevice.wrappedJSObject.connected);
  
  // read them out and make sure everything's accounted for
  var devices = ArrayConverter.JSArray(manager.devices);
  assertTrue(devices.length == params.length, "expected devices to be registered");
  for each (device in devices) {
    var index = params.indexOf(device.wrappedJSObject.param);
    assertTrue(index >= 0, "Looking for param " + device.wrappedJSObject.param);
    params.splice(index, 1);
    manager.unregisterDevice(device);
    assertTrue(device.wrappedJSObject.connected);
  }
  assertTrue(params.length == 0, "params not empty: " + uneval(params));
  assertTrue(manager.devices.length == 0, "devices not unregistered");
}

