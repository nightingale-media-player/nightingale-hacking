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
  Components.utils.import("resource://app/components/sbLibraryUtils.jsm");

  var values;
  var outValues;
  var outValuesCount = {};

  var builder = Cc["@songbirdnest.com/Songbird/Library/ConstraintBuilder;1"]
                  .createInstance(Ci.sbILibraryConstraintBuilder);

  try {
    builder.get();
    fail("did not throw");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_UNEXPECTED);
  }

  try {
    builder.intersect();
    fail("did not throw");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_UNEXPECTED);
  }

  try {
    builder.include(SBProperties.artistName, "The Beatles")
           .intersect()
           .get();
    fail("did not throw");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_UNEXPECTED);
  }

  builder = Cc["@songbirdnest.com/Songbird/Library/ConstraintBuilder;1"]
              .createInstance(Ci.sbILibraryConstraintBuilder);

  var c = builder.include(SBProperties.artistName, "The Beatles").get();
  assertEqual(c.groupCount, 1);
  var g = c.getGroup(0);
  assertEqual(g.properties.getNext(), SBProperties.artistName);
  assertEqual(g.getValues(SBProperties.artistName).getNext(), "The Beatles");

  c = builder.include(SBProperties.artistName, "The Beatles")
             .intersect()
             .include(SBProperties.albumName, "Abbey Road")
             .include(SBProperties.albumName, "Let It Be").get();

  assertEqual(c.groupCount, 2);
  var g = c.getGroup(0);
  assertEqual(g.properties.getNext(), SBProperties.artistName);
  assertEqual(g.getValues(SBProperties.artistName).getNext(), "The Beatles");
  g = c.getGroup(1);
  assertEqual(g.properties.getNext(), SBProperties.albumName);
  var values = g.getValues(SBProperties.albumName);
  assertEqual(values.getNext(), "Abbey Road");
  assertEqual(values.getNext(), "Let It Be");

  // Test equality
  var c1 = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["hello"]],
      [SBProperties.artistName, ["world"]]
    ]
  ]);

  var c2 = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["world", "hello"]]
    ]
  ]);
  assertTrue(c1.equals(c2));

  c1 = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["hello"]]
    ],
    [
      [SBProperties.albumName, ["world"]],
      [SBProperties.trackName, ["trackName"]],
      [SBProperties.genre, ["rock", "pop", "jazz"]]
    ]
  ]);

  c2 = LibraryUtils.createConstraint([
    [
      [SBProperties.genre, ["rock", "pop", "jazz"]],
      [SBProperties.albumName, ["world"]],
      [SBProperties.trackName, ["trackName"]]
    ],
    [
      [SBProperties.artistName, ["hello"]],
    ]
  ]);
  assertTrue(c1.equals(c2));

  c2 = LibraryUtils.createConstraint([
    [
      [SBProperties.genre, ["rock", "pop", "jazz"]],
      [SBProperties.trackName, ["trackName"]]
    ],
    [
      [SBProperties.artistName, ["hello"]],
    ]
  ]);
  assertFalse(c1.equals(c2));

  var newC2 = writeAndRead(c2);
  assertTrue(c2.equals(newC2));

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

