/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

//------------------------------------------------------------------------------
//
// Mock CD device test
//
//------------------------------------------------------------------------------


function runTest()
{
  var deviceName = "TestCD";
  var sbDevice = sbCreateDevice(deviceName, sbMakeInstantJunk());
  assertTrue(sbDevice);
  
  var deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                        .getService(Ci.sbIDeviceManager2);

  var props = Cc["@mozilla.org/hash-property-bag;1"]
                .createInstance(Ci.nsIWritablePropertyBag2);

  // XXX TODO finish out once we have the CD Addon working
  // sbDevice.submitRequest(Ci.sbIDevice.REQUEST_READ, 

  function onDeviceEvent(aEvent) {
    switch (aEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_READY:
        if (sbDevice.currentStatus.currentState !=
            Ci.sbIDevice.STATE_DISCONNECTED) {
          sbDevice.disconnect();
        }
        break;
      case Ci.sbIDeviceEvent.EVENT_DEVICE_REMOVED:
        deviceManager.removeEventListener(onDeviceEvent);
        sbDevice.removeEventListener(onDeviceEvent);
        testFinished();
        break;
    }
  }
  deviceManager.QueryInterface(Ci.sbIDeviceEventTarget)
               .addEventListener(onDeviceEvent);
  sbDevice.QueryInterface(Ci.sbIDeviceEventTarget)
          .addEventListener(onDeviceEvent);
  testPending();
}
