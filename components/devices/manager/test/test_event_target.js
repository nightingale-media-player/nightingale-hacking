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
 * \brief Device Manager tests - Device Event Target
 */

Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");

var hasEvent = false;

function onEvent(aEvent) {
  hasEvent = true;
}

function runTest () {
  var manager = Components.classes["@getnightingale.com/Nightingale/DeviceManager;2"]
                          .getService(Components.interfaces.sbIDeviceManager2);

  manager.addEventListener(onEvent);

  var event = manager.createEvent(Components.interfaces.sbIDeviceEvent.COMMAND_DEVICE_BASE);
  manager.dispatchEvent(event);
  assertTrue(hasEvent, "event listener not fired");
  
  manager.removeEventListener(onEvent);
  hasEvent = false;
  event = manager.createEvent(event.COMMAND_DEVICE_BASE);
  manager.dispatchEvent(event);
  assertFalse(hasEvent, "event listener not removed");
  
  // stress test, yay!
  var eventTest = Components.classes["@getnightingale.com/Nightingale/Device/EventTester/StressThreads;1"]
                            .createInstance(Components.interfaces.nsIRunnable);
  eventTest.run();
  
  // test the sequencing too
  var eventTest = Components.classes["@getnightingale.com/Nightingale/Device/EventTester/Removal;1"]
                            .createInstance(Components.interfaces.nsIRunnable);
  eventTest.run();
}
