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

  var smartListFactory = Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/SmartMediaListFactory;1"]
                           .createInstance(Ci.sbIMediaListFactory);

  library.registerMediaListFactory(smartListFactory);

  testProperties(library);
  testConditions(library);
  testAll(library);
  testAny(library);
  testOperators(library);
  testItemLimit(library);
  testSerialize(library);
}

function testProperties(library) {

  var list = library.createMediaList("smart");

  assertEqual(list.match, Ci.sbILocalDatabaseSmartMediaList.MATCH_ANY);
  list.match = Ci.sbILocalDatabaseSmartMediaList.MATCH_ALL;
  assertEqual(list.match, Ci.sbILocalDatabaseSmartMediaList.MATCH_ALL);

  assertEqual(list.itemLimit, Ci.sbILocalDatabaseSmartMediaList.LIMIT_NONE);
  list.itemLimit = 20;
  assertEqual(list.itemLimit, 20);

  assertEqual(list.randomSelection, false);
  list.randomSelection = true;
  assertEqual(list.randomSelection, true);
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
  list.match = Ci.sbILocalDatabaseSmartMediaList.MATCH_ALL;

  list.appendCondition(albumProp,
                       getOperatorForProperty(albumProp, "="),
                       "Back In Black",
                       null,
                       false);
  list.rebuild();

  assertEqual(list.length, 10);

  // Adding this condition should not change the result since
  // it overlaps with the first condition
  list.appendCondition(artistProp,
                       getOperatorForProperty(artistProp, "="),
                       "AC/DC",
                       null,
                       false);
  list.rebuild();

  assertEqual(list.length, 10);

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

  // Contrain the list on contnet length
  list.appendCondition(contentLengthProp,
                       getOperatorForProperty(contentLengthProp, "<"),
                       "1000",
                       null,
                       false);
  list.rebuild();
  assertEqual(list.length, 6);

}

function testAny(library) {

  var albumProp = SB_NS + "albumName";
  var artistProp = SB_NS + "artistName";
  var contentLengthProp = SB_NS + "contentLength";

  var list = library.createMediaList("smart");
  list.match = Ci.sbILocalDatabaseSmartMediaList.MATCH_ANY;

  list.appendCondition(albumProp,
                       getOperatorForProperty(albumProp, "="),
                       "Back In Black",
                       null,
                       false);
  list.rebuild();

  assertEqual(list.length, 10);

  // Adding this condition should not change the result since
  // it overlaps with the first condition
  list.appendCondition(artistProp,
                       getOperatorForProperty(artistProp, "="),
                       "AC/DC",
                       null,
                       false);
  list.rebuild();

  assertEqual(list.length, 10);

  // Add another artist
  list.appendCondition(artistProp,
                       getOperatorForProperty(artistProp, "="),
                       "a-ha",
                       null,
                       false);
  list.rebuild();

  assertEqual(list.length, 20);
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

  setConditions(lastPlayTimeProp, "!=", value);
  assertEqual(list.length, 45);

  setConditions(lastPlayTimeProp, ">", value);
  assertEqual(list.length, 29);

  setConditions(lastPlayTimeProp, ">=", value);
  assertEqual(list.length, 33);

  setConditions(lastPlayTimeProp, "<", value);
  assertEqual(list.length, 16);

  setConditions(lastPlayTimeProp, "<=", value);
  assertEqual(list.length, 20);

  list.clearConditions();
  list.appendCondition(lastPlayTimeProp,
                       getOperatorForProperty(lastPlayTimeProp, "^"),
                       "1164844762000",
                       "1169855962000",
                       false);
  list.rebuild();
  assertEqual(list.length, 49);


  list.clearConditions();
  list.appendCondition(albumProp,
                       getOperatorForProperty(albumProp, "?%"),
                       "On",
                       null,
                       false);
  list.rebuild();
  assertEqual(list.length, 12);

  list.clearConditions();
  list.appendCondition(albumProp,
                       getOperatorForProperty(albumProp, "%?"),
                       "Black",
                       null,
                       false);
  list.rebuild();
  assertEqual(list.length, 22);

  list.clearConditions();
  list.appendCondition(albumProp,
                       getOperatorForProperty(albumProp, "%?%"),
                       "fat",
                       null,
                       false);
  list.rebuild();
  assertEqual(list.length, 12);
}

function testItemLimit(library) {

  var albumProp = SB_NS + "albumName";

  var list = library.createMediaList("smart");

  list.appendCondition(albumProp,
                       getOperatorForProperty(albumProp, "="),
                       "Back In Black",
                       null,
                       false);
  list.rebuild();
  assertEqual(list.length, 10);

  list.itemLimit = 5;
  list.rebuild();
  assertEqual(list.length, 5);

  list.itemLimit = Ci.sbILocalDatabaseSmartMediaList.LIMIT_NONE;
  list.rebuild();
  assertEqual(list.length, 10);
}

function testSerialize(library) {

  var lastPlayTimeProp = SB_NS + "lastPlayTime";
  var albumProp = SB_NS + "albumName";

  var list = library.createMediaList("smart");
  var guid = list.guid;
  list.match = Ci.sbILocalDatabaseSmartMediaList.MATCH_ALL;
  list.itemLimit = 123;
  list.randomSelection = true;
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

  var smartListFactory = Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/SmartMediaListFactory;1"]
                           .createInstance(Ci.sbIMediaListFactory);

  library2.registerMediaListFactory(smartListFactory);
  var restoredList = library2.getMediaItem(guid);

  assertEqual(list.match, restoredList.match);
  assertEqual(list.itemLimit, restoredList.itemLimit);
  assertEqual(list.randomSelection, restoredList.randomSelection);
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

