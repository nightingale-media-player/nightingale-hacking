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
 * \brief Device Manager tests - Controller Registrar
 */

Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
var gUUIDGenerator = Components.classes["@mozilla.org/uuid-generator;1"]
                               .createInstance(Components.interfaces.nsIUUIDGenerator);


function DummyController(aParam) {
  this.wrappedJSObject = this;
  this.name = aParam;
  this.id = gUUIDGenerator.generateUUID();
}

function runTest () {
  var manager = Components.classes["@getnightingale.com/Nightingale/DeviceManager;2"]
                          .getService(Components.interfaces.sbIDeviceManager2);

  var params = [];

  var defaultControllers = ArrayConverter.JSArray(manager.controllers);
  var defaultControllerCount = defaultControllers.length;

  for each (defaultController in defaultControllers) {
    defaultController.QueryInterface(Components.interfaces.sbIDeviceController);
    params.push(defaultController.name);
  }
  
  // register some dummy controllers
  for (var i = 0; i < 1000; ++i) {
    // generate some sort of unique id
    var param = (params.length * 65537 + 7) % 3000;
    params.push("" + param);
    manager.registerController(new DummyController(param));
  }
  
  // put in a special controller to test getController
  var controller = new DummyController(0);
  manager.registerController(controller);

  // fill in some more dummy data
  for (var i = 0; i < 1000; ++i) {
    // generate some sort of unique id
    var param = (params.length * 65537 + 7) % 3000;
    params.push("" + param);
    manager.registerController(new DummyController(param));
  }

  // test getController
  var returnedController = manager.getController(controller.id);
  assertTrue(controller === returnedController.wrappedJSObject,
             "failed to get controller:" +
             controller.name + " / " + returnedController.name);
  manager.unregisterController(controller);
  
  // read them out and make sure everything's accounted for
  var controllers = ArrayConverter.JSArray(manager.controllers);
  assertTrue(controllers.length == params.length, "expected controllers to be registered");
  for each (controller in controllers) {
    controller.QueryInterface(Components.interfaces.sbIDeviceController);
    var index = params.indexOf(controller.name);
    assertTrue(index >= 0, "Looking for param " + controller.name + "\n" + uneval(params));
    params.splice(index, 1);
    manager.unregisterController(controller);
  }
  assertTrue(params.length == 0, "params not empty: " + uneval(params));
  assertTrue(manager.controllers.length == 0, "devices not unregistered");
}

