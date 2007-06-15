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

function runTest() {

  var databaseGUID = "test_smartmedialist";
  var library = createLibrary(databaseGUID);

  testProperties(library);
  testConditions(library);
  testAll(library);
  testAny(library);
  testOperators(library);
  testItemLimit(library);
  testSerialize(library);
  testUsecsLimit(library);
  testBytesLimit(library);
  testRandom(library);
  testMatchTypeNoneItemLimit(library);
  testMatchTypeNoneUsecLimit(library)
  testMatchTypeNoneBytesLimit(library);
  testMatchTypeNoneRandom(library);
}

function testProperties(library) {

  var albumProp = SB_NS + "albumName";
  var list = library.createMediaList("smart");

  assertEqual(list.matchType, Ci.sbILocalDatabaseSmartMediaList.MATCH_TYPE_ANY);
  list.matchType = Ci.sbILocalDatabaseSmartMediaList.MATCH_TYPE_ALL;
  assertEqual(list.matchType, Ci.sbILocalDatabaseSmartMediaList.MATCH_TYPE_ALL);

  assertEqual(list.limitType, Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_NONE);
  list.limitType = Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_ITEMS;
  assertEqual(list.limitType, Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_ITEMS);

  assertEqual(list.limit, 0);
  list.limit = 20;
  assertEqual(list.limit, 20);

  assertEqual(list.selectPropertyName, "");
  list.selectPropertyName = albumProp;
  assertEqual(list.selectPropertyName, albumProp);

  assertEqual(list.selectDirection, true);
  list.selectDirection = false;
  assertEqual(list.selectDirection, false);

  assertEqual(list.randomSelection, false);
  list.randomSelection = true;
  assertEqual(list.randomSelection, true);

  assertEqual(list.liveUpdate, false);
  list.liveUpdate = true;
  assertEqual(list.liveUpdate, true);
}

function testConditions(library) {

  var albumProp = SB_NS + "albumName";

  var list = library.createMediaList("smart");

  assertEqual(list.conditionCount, 0);

  var op = getOperatorForProperty(albumProp, "=");
  var condition1 = list.appendCondition(albumProp, op, "Back In Black", null, false);
  assertEqual(list.conditionCount, 1);

  assertEqual(condition1.propertyName, albumProp);
  assertEqual(condition1.operator, op);
  assertEqual(condition1.leftValue, "Back In Black");
  assertEqual(condition1.rightValue, null);
  assertEqual(condition1.limit, false);

  var condition2 = list.appendCondition(albumProp, op, "The Life of Riley", null, false);
  assertEqual(list.conditionCount, 2);

  assertEqual(list.getConditionAt(0), condition1);
  assertEqual(list.getConditionAt(1), condition2);

  list.removeConditionAt(0);
  assertEqual(list.conditionCount, 1);
  assertEqual(list.getConditionAt(0), condition2);

  list.removeConditionAt(0);
  assertEqual(list.conditionCount, 0);
}

function testAll(library) {

  var albumProp = SB_NS + "albumName";
  var artistProp = SB_NS + "artistName";
  var contentLengthProp = SB_NS + "contentLength";

  var list = library.createMediaList("smart");
  list.matchType = Ci.sbILocalDatabaseSmartMediaList.MATCH_TYPE_ALL;

  assertEqual(list.length, 0);

  list.appendCondition(albumProp,
                       getOperatorForProperty(albumProp, "="),
                       "Back In Black",
                       null,
                       false);
  list.rebuild();

  assertEqual(list.length, 10);
  assertUnique(list);

  // Adding this condition should not change the result since
  // it overlaps with the first condition
  list.appendCondition(artistProp,
                       getOperatorForProperty(artistProp, "="),
                       "AC/DC",
                       null,
                       false);
  list.rebuild();
  assertEqual(list.length, 10);
  assertUnique(list);

  // Should result in 0 items
  list.appendCondition(artistProp,
                       getOperatorForProperty(artistProp, "="),
                       "a-ha",
                       null,
                       false);
  list.rebuild();

  assertEqual(list.length, 0);

  list.removeConditionAt(2);
  list.rebuild();
  assertEqual(list.length, 10);
  assertUnique(list);

  // Contrain the list on contnet length
  list.appendCondition(contentLengthProp,
                       getOperatorForProperty(contentLengthProp, "<"),
                       "1000",
                       null,
                       false);
  list.rebuild();
  assertEqual(list.length, 6);
  assertUnique(list);
}

function testAny(library) {

  var albumProp = SB_NS + "albumName";
  var artistProp = SB_NS + "artistName";
  var contentLengthProp = SB_NS + "contentLength";

  var list = library.createMediaList("smart");
  list.matchType = Ci.sbILocalDatabaseSmartMediaList.MATCH_TYPE_ANY;

  list.appendCondition(albumProp,
                       getOperatorForProperty(albumProp, "="),
                       "Back In Black",
                       null,
                       false);
  list.rebuild();

  assertEqual(list.length, 10);
  assertUnique(list);

  // Adding this condition should not change the result since
  // it overlaps with the first condition
  list.appendCondition(artistProp,
                       getOperatorForProperty(artistProp, "="),
                       "AC/DC",
                       null,
                       false);
  list.rebuild();

  assertEqual(list.length, 10);
  assertUnique(list);

  // Add another artist
  list.appendCondition(artistProp,
                       getOperatorForProperty(artistProp, "="),
                       "a-ha",
                       null,
                       false);
  list.rebuild();

  assertEqual(list.length, 20);
  assertUnique(list);
}

function testOperators(library) {

  var list = library.createMediaList("smart");
  var lastPlayTimeProp = SB_NS + "lastPlayTime";
  var albumProp = SB_NS + "albumName";

  function setConditions(prop, op, value) {
    list.clearConditions();
    list.appendCondition(prop,
                         getOperatorForProperty(prop, op),
                         value,
                         null,
                         false);
    list.rebuild();
  }

  var value = "1166399962000";
  setConditions(lastPlayTimeProp, "=", value);
  assertEqual(list.length, 4);
  assertUnique(list);

  setConditions(lastPlayTimeProp, "!=", value);
  assertEqual(list.length, 45);
  assertUnique(list);

  setConditions(lastPlayTimeProp, ">", value);
  assertEqual(list.length, 29);
  assertUnique(list);

  setConditions(lastPlayTimeProp, ">=", value);
  assertEqual(list.length, 33);
  assertUnique(list);

  setConditions(lastPlayTimeProp, "<", value);
  assertEqual(list.length, 16);
  assertUnique(list);

  setConditions(lastPlayTimeProp, "<=", value);
  assertEqual(list.length, 20);
  assertUnique(list);

  list.clearConditions();
  list.appendCondition(lastPlayTimeProp,
                       getOperatorForProperty(lastPlayTimeProp, "^"),
                       "1164844762000",
                       "1169855962000",
                       false);
  list.rebuild();
  assertEqual(list.length, 49);
  assertUnique(list);

  list.clearConditions();
  list.appendCondition(albumProp,
                       getOperatorForProperty(albumProp, "?%"),
                       "On",
                       null,
                       false);
  list.rebuild();
  assertEqual(list.length, 12);
  assertUnique(list);

  list.clearConditions();
  list.appendCondition(albumProp,
                       getOperatorForProperty(albumProp, "%?"),
                       "Black",
                       null,
                       false);
  list.rebuild();
  assertEqual(list.length, 22);
  assertUnique(list);

  list.clearConditions();
  list.appendCondition(albumProp,
                       getOperatorForProperty(albumProp, "%?%"),
                       "fat",
                       null,
                       false);
  list.rebuild();
  assertEqual(list.length, 12);
  assertUnique(list);
}

function testItemLimit(library) {

  var albumProp = SB_NS + "albumName";
  var trackProp = SB_NS + "trackNumber";

  var list = library.createMediaList("smart");

  list.appendCondition(albumProp,
                       getOperatorForProperty(albumProp, "="),
                       "Back In Black",
                       null,
                       false);
  list.rebuild();
  assertEqual(list.length, 10);
  assertUnique(list);

  list.limitType = Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_ITEMS;
  list.limit = 5;
  list.selectPropertyName = trackProp;
  list.selectDirection = true;

  list.rebuild();
  assertEqual(list.length, 5);

  // Should have tracks 1 to 5
  assertTrackNumbers(list, [1, 2, 3, 4, 5]);

  list.selectDirection = false;
  list.rebuild();

  // Should have tracks 6 to 10
  assertTrackNumbers(list, [6, 7, 8, 9, 10]);

  list.limitType = Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_NONE;
  list.rebuild();
  assertEqual(list.length, 10);
}

function testUsecsLimit(library) {

  var albumProp = SB_NS + "albumName";
  var trackProp = SB_NS + "trackNumber";

  var list = library.createMediaList("smart");

  list.appendCondition(albumProp,
                       getOperatorForProperty(albumProp, "="),
                       "Back In Black",
                       null,
                       false);

  list.limitType = Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_USECS;
  list.limit = 30 * 60 * 1000 * 1000;
  list.selectPropertyName = trackProp;
  list.selectDirection = true;

  list.rebuild();
  assertEqual(list.length, 3);
  assertUnique(list);

  // Should have tracks 1 to 3
  assertTrackNumbers(list, [1, 2, 3]);

  list.selectDirection = false;
  list.rebuild();
  assertEqual(list.length, 3);

  // Should have tracks 8 to 10
  assertTrackNumbers(list, [8, 9, 10]);
}

function testBytesLimit(library) {

  var albumProp = SB_NS + "albumName";
  var trackProp = SB_NS + "trackNumber";

  var list = library.createMediaList("smart");

  list.appendCondition(albumProp,
                       getOperatorForProperty(albumProp, "="),
                       "Back In Black",
                       null,
                       false);

  list.limitType = Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_BYTES;
  list.limit = 1500;
  list.selectPropertyName = trackProp;
  list.selectDirection = true;

  list.rebuild();
  assertEqual(list.length, 3);
  assertUnique(list);

  // Should have tracks 1 to 3
  assertTrackNumbers(list, [1, 2, 3]);

  list.selectDirection = false;
  list.rebuild();
  assertEqual(list.length, 3);

  // Should have tracks 8 to 10
  assertTrackNumbers(list, [8, 9, 10]);
}

function testMatchTypeNoneItemLimit(library) {

  var artistProp = SB_NS + "artistName";

  var list = library.createMediaList("smart");
  list.matchType = Ci.sbILocalDatabaseSmartMediaList.MATCH_TYPE_NONE;
  list.limitType = Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_ITEMS;
  list.limit = 25;
  list.selectPropertyName = artistProp;
  list.selectDirection = true;

  list.rebuild();
  assertEqual(list.length, 25);
  assertUnique(list);

  // First 12 tracks are "A House"
  assertPropertyRange(list, 0, 12, artistProp, "A House");

  // Next 13 tracks are "A Split Second"
  assertPropertyRange(list, 12, 13, artistProp, "A Split Second");

}

function testMatchTypeNoneUsecLimit(library) {

  var artistProp = SB_NS + "artistName";
  var durationProp = SB_NS + "duration";

  var list = library.createMediaList("smart");
  list.matchType = Ci.sbILocalDatabaseSmartMediaList.MATCH_TYPE_NONE;
  list.limitType = Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_USECS;

  // 2 hours of music
  list.limit = 120 * 60 * 1000 * 1000;
  list.selectPropertyName = artistProp;
  list.selectDirection = true;

  list.rebuild();
  assertEqual(list.length, 15);
  assertUnique(list);

  // Make sure the list's limit is correct
  var sum = sumProperty(list, durationProp);
  assertTrue(sum >= list.limit);

  // First 12 tracks are "A House"
  assertPropertyRange(list, 0, 12, artistProp, "A House");

  // Next 3 tracks are "A Split Second"
  assertPropertyRange(list, 12, 3, artistProp, "A Split Second");
}

function testMatchTypeNoneBytesLimit(library) {

  var artistProp = SB_NS + "artistName";
  var contentLengthProp = SB_NS + "contentLength";

  var list = library.createMediaList("smart");
  list.matchType = Ci.sbILocalDatabaseSmartMediaList.MATCH_TYPE_NONE;
  list.limitType = Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_BYTES;

  list.limit = 40000;
  list.selectPropertyName = artistProp;
  list.selectDirection = true;

  list.rebuild();
  assertEqual(list.length, 30);
  assertUnique(list);

  // Make sure the list's limit is correct
  var sum = sumProperty(list, contentLengthProp);
  assertTrue(sum >= list.limit);

  // First 12 tracks are "A House"
  assertPropertyRange(list, 0, 12, artistProp, "A House");

  // Next 15 tracks are "A Split Second"
  assertPropertyRange(list, 12, 15, artistProp, "A Split Second");

  // Next 3 tracks are "A Split Second"
  assertPropertyRange(list, 27, 3, artistProp, "a-ha");
}

function testRandom(library) {

  var albumProp = SB_NS + "albumName";
  var trackProp = SB_NS + "trackNumber";

  var list = library.createMediaList("smart");

  list.appendCondition(albumProp,
                       getOperatorForProperty(albumProp, "="),
                       "Back In Black",
                       null,
                       false);

  list.limitType = Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_ITEMS;
  list.limit = 5;
  list.randomSelection = true;

  list.rebuild();
  assertEqual(list.length, 5);
  assertUnique(list);

  // Test various high limits, should return all tracks in back in black
  list.limit = 200;
  list.rebuild();
  assertEqual(list.length, 10);
  assertUnique(list);

  list.limitType = Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_USECS;
  list.limit = 10 * 60 * 1000 * 1000;
  list.rebuild();
  assertUnique(list);

  list.limit = 240 * 60 * 1000 * 1000;
  list.rebuild();
  assertEqual(list.length, 10);
  assertUnique(list);

  list.limitType = Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_BYTES;
  list.limit = 10000;
  list.rebuild();
  assertUnique(list);

  list.limit = 1000000000;
  list.rebuild();
  assertEqual(list.length, 10);
  assertUnique(list);
}

function testMatchTypeNoneRandom(library) {

  var list = library.createMediaList("smart");
  var libraryMediaItems = countMediaItems(library);

  list.matchType = Ci.sbILocalDatabaseSmartMediaList.MATCH_TYPE_NONE;
  list.limitType = Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_ITEMS;
  list.limit = 5;
  list.randomSelection = true;

  list.rebuild();
  assertEqual(list.length, 5);
  assertUnique(list);

  // a really high limit should select the entire library
  list.limit = 100000;
  list.rebuild();
  assertEqual(list.length, libraryMediaItems);
  assertUnique(list);

  list.limitType = Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_USECS;
  list.limit = 10 * 60 * 1000 * 1000;
  list.rebuild();
  assertUnique(list);

  list.limit = 100 * 60 * 60 * 1000 * 1000;
  list.rebuild();
  assertEqual(list.length, libraryMediaItems);
  assertUnique(list);

  list.limitType = Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_BYTES;
  list.limit = 10000;
  list.rebuild();
  assertUnique(list);

  list.limit = 1000000000;
  list.rebuild();
  assertEqual(list.length, libraryMediaItems);
  assertUnique(list);
}

function testSerialize(library) {

  var lastPlayTimeProp = SB_NS + "lastPlayTime";
  var albumProp = SB_NS + "albumName";

  var list = library.createMediaList("smart");
  var guid = list.guid;
  list.matchType = Ci.sbILocalDatabaseSmartMediaList.MATCH_TYPE_ALL;
  list.limitType = Ci.sbILocalDatabaseSmartMediaList.LIMIT_TYPE_ITEMS;
  list.selectPropertyName = albumProp;
  list.selectDirection = false;
  list.limit = 123;
  list.randomSelection = true;
  list.liveUpdate = true;
  list.appendCondition(albumProp,
                       getOperatorForProperty(albumProp, "%?%"),
                       "fat",
                       "",
                       false);
  list.appendCondition(lastPlayTimeProp,
                       getOperatorForProperty(lastPlayTimeProp, ">"),
                       "1164844762000",
                       "",
                       false);

  // Create a second instance of the library and get the same list and see
  // if they match
  var databaseGUID = "test_smartmedialist";
  var library2 = createLibrary(databaseGUID, null, false);
  var restoredList = library2.getMediaItem(guid);

  assertEqual(list.matchType, restoredList.matchType);
  assertEqual(list.limitType, restoredList.limitType);
  assertEqual(list.selectPropertyName, restoredList.selectPropertyName);
  assertEqual(list.selectDirection, restoredList.selectDirection);
  assertEqual(list.limit, restoredList.limit);
  assertEqual(list.randomSelection, restoredList.randomSelection);
  assertEqual(list.liveUpdate, restoredList.liveUpdate);
  assertEqual(list.conditionCount, restoredList.conditionCount);

  for (var i = 0; i < list.conditionCount; i++) {
    assertCondition(list.getConditionAt(i), restoredList.getConditionAt(i));
  }
}

function getOperatorForProperty(propertyName, operator) {

  var propMan = Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
                  .getService(Ci.sbIPropertyManager);
  var info = propMan.getPropertyInfo(propertyName);
  var op = info.getOperator(operator);
  return op;
}

function assertCondition(a, b) {

  assertEqual(a.propertyName, b.propertyName);
  assertEqual(a.operator.operator, b.operator.operator);
  assertEqual(a.leftValue, b.leftValue);
  assertEqual(a.rightValue, b.rightValue);
  assertEqual(a.limit, b.limit);
}

function assertTrackNumbers(list, a) {

  for (var i = 0; i < a.length; i++) {
    assertEqual(list.getItemByIndex(i).getProperty(SB_NS + "trackNumber"), a[i]);
  }
}

function assertPropertyRange(list, start, length, prop, value) {

  for (var i = start; i < length; i++) {
    var item = list.getItemByIndex(i);
    assertEqual(item.getProperty(prop), value);
  }
}

function dumpList(list) {

  for (var i = 0; i < list.length; i++) {
    var item = list.getItemByIndex(i);
    var artist = item.getProperty(SB_NS + "artistName");
    var album = item.getProperty(SB_NS + "albumName");
    var track = item.getProperty(SB_NS + "trackNumber");
    log(artist + " " + album + " " + track);
  }
}

function assertUnique(list) {

  var guids = {};

  for (var i = 0; i < list.length; i++) {
    var item = list.getItemByIndex(i);
    if (item.guid in guids) {
      fail("list not unique, gud '" + item.guid + "' appears more than once");
    }
    guids[item.guid] = 1;
  }
}

function countMediaItems(library) {

  var PROP_ISLIST = "http://songbirdnest.com/data/1.0#isList";

  var listener = {
    length: 0,
    onEnumerationBegin: function() {
      return true;
    },
    onEnumeratedItem: function(list, item) {
      this.length++;
      return true;
    },
    onEnumerationEnd: function() {
      return true;
    }
  };

  library.enumerateItemsByProperty(PROP_ISLIST, "0",
                                   listener,
                                   Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

  return listener.length;
}

function sumProperty(list, prop) {

  var sum = 0;

  for (var i = 0; i < list.length; i++) {
    var item = list.getItemByIndex(i);
    sum += parseInt(item.getProperty(prop));
  }

  return sum;
}

