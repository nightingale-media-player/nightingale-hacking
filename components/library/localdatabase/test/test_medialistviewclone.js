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

  var library = createLibrary("test_medialistviewclone", null, false);
  library.clear();

  var view = library.createView();

  var cfs = view.cascadeFilterSet;

  view.setSort(SBProperties.createArray([
    [SBProperties.artistName, "a"]
  ]));
  view.filterConstraint = LibraryUtils.createConstraint([
    [
      [SBProperties.isList, ["0"]]
    ],
    [
      [SBProperties.hidden, ["0"]]
    ]
  ]);

  cfs.appendSearch([
    SBProperties.artistName,
    SBProperties.albumName,
    SBProperties.trackName
  ], 3);
  cfs.appendFilter(SBProperties.genre);
  cfs.appendFilter(SBProperties.artistName);
  cfs.appendFilter(SBProperties.albumName);

  cfs.set(0, ["Beat"], 1);
  cfs.set(1, ["ROCK"], 1);
  cfs.set(2, ["The Beatles"], 1);

  var clone = view.clone();

  assertPropertyArray(clone.currentSort, SBProperties.createArray([
    [SBProperties.artistName, "a"]
  ]));

  assertTrue(clone.searchConstraint.equals(LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["Beat"]],
      [SBProperties.albumName,  ["Beat"]],
      [SBProperties.trackName,  ["Beat"]]
    ]
  ])));

  assertTrue(clone.filterConstraint.equals(LibraryUtils.createConstraint([
    [
      [SBProperties.isList, ["0"]]
    ],
    [
      [SBProperties.hidden, ["0"]]
    ],
    [
      [SBProperties.genre, ["ROCK"]]
    ],
    [
      [SBProperties.artistName, ["The Beatles"]]
    ]
  ])));

  var cloneCfs = clone.cascadeFilterSet;
  assertEqual(cfs.length, cloneCfs.length);
  for (let i = 0; i < cfs.length; i++) {
    assertEqual(cfs.getProperty(i), cloneCfs.getProperty(i));
    assertEqual(cfs.isSearch(i), cloneCfs.isSearch(i));
  }

  var state = view.getState();
  var pipe = Cc["@mozilla.org/pipe;1"].createInstance(Ci.nsIPipe);
  pipe.init(false, false, 0, 0xffffffff, null);

  var oos = Cc["@mozilla.org/binaryoutputstream;1"]
              .createInstance(Ci.nsIObjectOutputStream);
  oos.setOutputStream(pipe.outputStream);
  oos.writeObject(state, true);
  oos.close();

  var ois = Cc["@mozilla.org/binaryinputstream;1"]
              .createInstance(Ci.nsIObjectInputStream);
  ois.setInputStream(pipe.inputStream);
  var newState = ois.readObject(true)
  ois.close();

  var newView = library.createView(newState);

  assertPropertyArray(newView.currentSort, SBProperties.createArray([
    [SBProperties.artistName, "a"]
  ]));
  assertTrue(newView.searchConstraint.equals(LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["Beat"]],
      [SBProperties.albumName,  ["Beat"]],
      [SBProperties.trackName,  ["Beat"]]
    ]
  ])));

  assertTrue(newView.filterConstraint.equals(LibraryUtils.createConstraint([
    [
      [SBProperties.isList, ["0"]]
    ],
    [
      [SBProperties.hidden, ["0"]]
    ],
    [
      [SBProperties.genre, ["ROCK"]]
    ],
    [
      [SBProperties.artistName, ["The Beatles"]]
    ]
  ])));

  var newCfs = newView.cascadeFilterSet;
  assertEqual(cfs.length, newCfs.length);
  for (let i = 0; i < cfs.length; i++) {
    assertEqual(cfs.getProperty(i), newCfs.getProperty(i));
    assertEqual(cfs.isSearch(i), newCfs.isSearch(i));
  }
}

function assertPropertyArray(a1, a2) {
  if (a1.length != a2.length) {
    fail("Property array lengths differ, " + a1 +
         " != " + a2);
  }

  for(var i = 0; i < a1.length; i++) {
    var prop1 = a1.getPropertyAt(i);
    var prop2 = a2.getPropertyAt(i);
    if (prop1.id != prop2.id) {
      fail("Names are different at index " + i + ", " +
           prop1 + " != " + prop2);
    }
    if (prop1.value != prop2.value) {
      fail("Values are different at index " + i + ", " +
           prop1 + " != " + prop2);
    }
  }

}

