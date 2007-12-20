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
 * \brief Test file
 */

function runTest () {

  Components.utils.import("resource://app/components/sbProperties.jsm");
  Components.utils.import("resource://app/components/sbLibraryUtils.jsm");

  var library = createLibrary("test_viewdistinctvalues", null, false);
  library.clear();

  var items = [
    ["The Beatles", "Abbey Road", "Come Together",  "ROCK"],
    ["The Beatles", "Abbey Road", "Sun King",       "ROCK"],
    ["The Beatles", "Let It Be",  "Get Back",       "POP"],
    ["The Beatles", "Let It Be",  "Two Of Us",      "POP"],
    ["The Doors",   "L.A. Woman", "L.A. Woman",     "ROCK"],
    ["The Doors",   "L.A. Woman", "Love Her Madly", "ROCK"]
  ];

  function makeItem(i) {
    var item = library.createMediaItem(
      newURI("http://foo/" + i + ".mp3"),
      SBProperties.createArray([
        [SBProperties.artistName,items[i][0]],
        [SBProperties.albumName, items[i][1]],
        [SBProperties.trackName, items[i][2]],
        [SBProperties.genre,     items[i][3]]
     ]));
    return item;
  }

  var item1 = makeItem(0);
  var item2 = makeItem(1);
  var item3 = makeItem(2);
  var item4 = makeItem(3);
  var item5 = makeItem(4);
  var item6 = makeItem(5);

  var view = library.createView();

  var enumerator = view.getDistinctValuesForProperty(SBProperties.artistName);
  assertEqual(enumerator.getNext(), "The Beatles");
  assertEqual(enumerator.getNext(), "The Doors");
  assertFalse(enumerator.hasMore());

  enumerator = view.getDistinctValuesForProperty(SBProperties.albumName);
  assertEqual(enumerator.getNext(), "Abbey Road");
  assertEqual(enumerator.getNext(), "L.A. Woman");
  assertEqual(enumerator.getNext(), "Let It Be");
  assertFalse(enumerator.hasMore());

  view.filterConstraint = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["The Beatles"]]
    ]
  ]);

  enumerator = view.getDistinctValuesForProperty(SBProperties.artistName);
  assertEqual(enumerator.getNext(), "The Beatles");
  assertFalse(enumerator.hasMore());

  enumerator = view.getDistinctValuesForProperty(SBProperties.albumName);
  assertEqual(enumerator.getNext(), "Abbey Road");
  assertEqual(enumerator.getNext(), "Let It Be");
  assertFalse(enumerator.hasMore());

  view.filterConstraint = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["The Beatles"]]
    ],
    [
      [SBProperties.albumName, ["Let It Be"]]
    ],
  ]);

  enumerator = view.getDistinctValuesForProperty(SBProperties.artistName);
  assertEqual(enumerator.getNext(), "The Beatles");
  assertFalse(enumerator.hasMore());

  enumerator = view.getDistinctValuesForProperty(SBProperties.albumName);
  assertEqual(enumerator.getNext(), "Let It Be");
  assertFalse(enumerator.hasMore());
}

