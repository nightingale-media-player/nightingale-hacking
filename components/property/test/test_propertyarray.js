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

function assertPropertyData(property, id, value) {
  assertNotEqual(property, null);
  assertNotEqual(property, undefined);
  assertEqual(property.id, id);
  assertEqual(property.value, value);
}

function dumpArray(array) {
  var count = array.length;
  log ("dumpArray: " + array + " (count = " + count + ")");
  for (var index = 0; index < count; index++) {
    var prop = array.getPropertyAt(index);
    log("array[" + index + "] = {id: " + prop.id + ", value: " +
        prop.value + "}");
  }
}

function runTest() {
  var propertyArray =
    Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"].
    createInstance(Ci.sbIMutablePropertyArray);

  // Test appendProperty.
  for (var index = 0; index < 15; index++) {
    var id = "Property" + index;
    var value = "Value" + index;
    propertyArray.appendProperty(id, value);
  }
  
  // Test length.
  assertEqual(propertyArray.length, 15);
  
  // Test queryElementAt.
  var property = propertyArray.queryElementAt(0, Ci.sbIProperty);
  assertPropertyData(property, "Property0", "Value0");
  
  property = propertyArray.queryElementAt(1, Ci.nsISupports);
  assertPropertyData(property.QueryInterface(Ci.sbIProperty), "Property1",
                    "Value1");
  
  var qiException;
  try {
    property = propertyArray.queryElementAt(2, Ci.nsIFile);
  } catch (e) {
    qiException = true;
  }
  assertEqual(qiException, true);
  
  // Test getPropertyAt.
  property = propertyArray.getPropertyAt(5);
  assertPropertyData(property, "Property5", "Value5");
  
  // Test indexOf.
  var index = propertyArray.indexOf(0, property);
  assertEqual(index, 5);
  
  // Test enumerate.
  var enumerator = propertyArray.enumerate();
  var count = 0;
  while (enumerator.hasMoreElements()) {
    var property = enumerator.getNext().QueryInterface(Ci.sbIProperty);
    assertPropertyData(property, "Property" + count, "Value" + count);
    count++;
  }
  assertEqual(count, 15);
  
  // Test replaceElementAt.
  var factory = Cc["@songbirdnest.com/Songbird/Properties/PropertyFactory;1"].
                createInstance(Ci.sbIPropertyFactory);
  var newProperty = factory.createProperty("NewProperty", "NewValue");
  
  propertyArray.replaceElementAt(newProperty, 9, false);
  
  var replaceException;
  try {
    propertyArray.replaceElementAt(newProperty, 13, true);
  } catch (e) {
    replaceException = true;
  }
  assertEqual(replaceException, true);
  
  var testProperty = propertyArray.getPropertyAt(9);
  assertPropertyData(testProperty, "NewProperty", "NewValue");
  
  assertEqual(propertyArray.length, 15);
  
  // Test insertElementAt.
  newProperty = factory.createProperty("SuperNewProperty", "SuperNewValue");
  
  propertyArray.insertElementAt(newProperty, 10, false);
  
  var insertException;
  try {
    propertyArray.insertElementAt(newProperty, 13, true);
  } catch (e) {
    insertException = true;
  }
  assertEqual(insertException, true);
  
  testProperty = propertyArray.getPropertyAt(9);
  assertPropertyData(testProperty, "NewProperty", "NewValue");
  
  testProperty = propertyArray.getPropertyAt(11);
  assertPropertyData(testProperty, "Property10", "Value10");
  
  testProperty = propertyArray.getPropertyAt(10);
  assertPropertyData(testProperty, "SuperNewProperty", "SuperNewValue");
  
  assertEqual(propertyArray.length, 16);
  
  // Test removeElementAt.
  propertyArray.removeElementAt(10);
  
  testProperty = propertyArray.getPropertyAt(9);
  assertPropertyData(testProperty, "NewProperty", "NewValue");
  
  testProperty = propertyArray.getPropertyAt(10);
  assertPropertyData(testProperty, "Property10", "Value10");
  
  testProperty = propertyArray.getPropertyAt(11);
  assertPropertyData(testProperty, "Property11", "Value11");
  
  assertEqual(propertyArray.length, 15);
  
  // Test clear.
  propertyArray.clear();
  assertEqual(propertyArray.length, 0);
  
  var newTestProperty;
  var exception;
  try {
    newTestProperty = propertyArray.getPropertyAt(6);
  } catch (e) {
    exception = true;
  }
  assertEqual(newTestProperty, undefined);
  assertEqual(exception, true);

  // Test serialize
  var propertyArray =
    Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"].
    createInstance(Ci.sbIMutablePropertyArray);

  for (var index = 0; index < 10; index++) {
    var id = "Property" + index;
    var value = "Value" + index;
    propertyArray.appendProperty(id, value);
  }
  var pipe = Cc["@mozilla.org/pipe;1"].createInstance(Ci.nsIPipe);
  pipe.init(false, false, 0, 0xffffffff, null);

  var oos = Cc["@mozilla.org/binaryoutputstream;1"]
              .createInstance(Ci.nsIObjectOutputStream);
  oos.setOutputStream(pipe.outputStream);
  oos.writeObject(propertyArray, true);
  oos.close();

  var ois = Cc["@mozilla.org/binaryinputstream;1"]
              .createInstance(Ci.nsIObjectInputStream);
  ois.setInputStream(pipe.inputStream);
  propertyArray = ois.readObject(true)
  ois.close();

  for (var index = 0; index < 10; index++) {
    var property = propertyArray.queryElementAt(index, Ci.sbIProperty);
    assertPropertyData(property, "Property" + index, "Value" + index);
  }
}

function getTempFile(name) {
  var file = Cc["@mozilla.org/file/directory_service;1"]
               .getService(Ci.nsIProperties)
               .get("TmpD", Ci.nsIFile);
  file.append(name);
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0664);
  var registerFileForDelete = Cc["@mozilla.org/uriloader/external-helper-app-service;1"]
                                .getService(Ci.nsPIExternalAppLauncher);
  registerFileForDelete.deleteTemporaryFileOnExit(file);
  return file;
}

