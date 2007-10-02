/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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
 * \brief Test file
 */

function runTest () {

  Components.utils.import("resource://app/components/sbProperties.jsm");

  var values;
  var outValues;
  var outValuesCount = {};

  var filter = Cc["@songbirdnest.com/Songbird/Library/Filter;1"]
                 .createInstance(Ci.sbILibraryFilter);

  try {
    filter.property;
    fail("Did not throw");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_UNEXPECTED);
  }

  values = ["value1", "value2", "value3"];
  filter.init(SBProperties.artistName, values.length, values);

  try {
    filter.init(SBProperties.artistName, values.length, values);
    fail("Did not throw");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_UNEXPECTED);
  }

  assertEqual(filter.property, SBProperties.artistName);
  assertArrays(filter.getValues(outValuesCount), values);

  var newFilter = writeAndRead(filter);
  assertEqual(newFilter.property, SBProperties.artistName);
  assertArrays(newFilter.getValues(outValuesCount), values);

  var search = Cc["@songbirdnest.com/Songbird/Library/Search;1"]
                 .createInstance(Ci.sbILibrarySearch);

  try {
    search.property;
    fail("Did not throw");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_UNEXPECTED);
  }

  values = ["value1", "value2", "value3"];
  search.init(SBProperties.artistName, true, values.length, values);

  try {
    search.init(SBProperties.artistName, true, values.length, values);
    fail("Did not throw");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_UNEXPECTED);
  }

  assertEqual(search.property, SBProperties.artistName);
  assertEqual(search.isAll, true);
  assertArrays(search.getValues(outValuesCount), values);

  var newSearch = writeAndRead(search);
  assertEqual(newSearch.property, SBProperties.artistName);
  assertEqual(newSearch.isAll, true);
  assertArrays(newSearch.getValues(outValuesCount), values);

  var sort = Cc["@songbirdnest.com/Songbird/Library/Sort;1"]
               .createInstance(Ci.sbILibrarySort);

  try {
    sort.property;
    fail("Did not throw");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_UNEXPECTED);
  }

  sort.init(SBProperties.artistName, true);

  try {
    sort.init(SBProperties.artistName, true);
    fail("Did not throw");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_UNEXPECTED);
  }

  assertEqual(sort.property, SBProperties.artistName);
  assertEqual(sort.isAscending, true);

  var newSort = writeAndRead(sort);
  assertEqual(newSort.property, SBProperties.artistName);
  assertEqual(sort.isAscending, true);

}

function assertArrays(a1, a2) {

  assertEqual(a1.length, a2.length);
  for (var i = 0; i < a1.length; i++) {
    assertEqual(a1[i], a2[i]);
  }

}

function writeAndRead(o) {
  var pipe = Cc["@mozilla.org/pipe;1"].createInstance(Ci.nsIPipe);
  pipe.init(false, false, 0, 0xffffffff, null);

  var oos = Cc["@mozilla.org/binaryoutputstream;1"]
              .createInstance(Ci.nsIObjectOutputStream);
  oos.setOutputStream(pipe.outputStream);
  oos.writeObject(o, true);
  oos.close();

  var ois = Cc["@mozilla.org/binaryinputstream;1"]
              .createInstance(Ci.nsIObjectInputStream);
  ois.setInputStream(pipe.inputStream);
  var read = ois.readObject(true)
  ois.close();

  return read;
}

