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

  var library = createLibrary("test_null_properties", null, false);
  library.clear();

  testResource(library, library.guid);

  var item = library.createMediaItem(newURI("http://foo.com/"));
  library.sync();
  testResource(library, item.guid);

  var list = library.createMediaList("simple");
  library.sync();
  testResource(library, list.guid);
}

function testResource(library, guid) {

  var r = library.getItemByGuid(guid);
  var nullprop = "http://songbirdnest.com/data/1.0#foo";

  var originalLength = r.getProperties().length;
  var originalProps = countProperties(guid);

  assertEqual(r.getProperty(nullprop), null);

  r.setProperty(nullprop, null);
  assertEqual(r.getProperties().length, originalLength);
  assertEqual(r.getProperty(nullprop), null);
  library.sync();
  assertEqual(countProperties(guid), originalProps);

  var library2 = createLibrary("test_null_properties", null, false);
  var r2 = library2.getItemByGuid(guid);
  assertEqual(r2.getProperties().length, originalLength);
  assertEqual(r2.getProperty(nullprop), null);

  r.setProperty(nullprop, "");
  assertEqual(r.getProperties().length, originalLength + 1);
  assertEqual(r.getProperty(nullprop), "");
  library.sync();
  assertEqual(countProperties(guid), originalProps + 1);

  var library3 = createLibrary("test_null_properties", null, false);
  var r3 = library3.getItemByGuid(guid);
  assertEqual(r3.getProperties().length, originalLength + 1);
  assertEqual(r3.getProperty(nullprop), "");

  r.setProperty(nullprop, null);
  assertEqual(r.getProperties().length, originalLength);
  assertEqual(r.getProperty(nullprop), null);
  library.sync();
  assertEqual(countProperties(guid), originalProps);

  var library4 = createLibrary("test_null_properties", null, false);
  var r4 = library4.getItemByGuid(guid);
  assertEqual(r4.getProperties().length, originalLength);
  assertEqual(r4.getProperty(nullprop), null);

  // try with a top level property
  r.setProperty(SBProperties.contentMimeType, null);
  assertEqual(r.getProperties().length, originalLength);
  assertEqual(r.getProperty(SBProperties.contentMimeType), null);
  library.sync();

  var library5 = createLibrary("test_null_properties", null, false);
  var r5 = library5.getItemByGuid(guid);
  assertEqual(r5.getProperties().length, originalLength);
  assertEqual(r5.getProperty(SBProperties.contentMimeType), null);

  r.setProperty(SBProperties.contentMimeType, "");
  assertEqual(r.getProperties().length, originalLength + 1);
  assertEqual(r.getProperty(SBProperties.contentMimeType), "");

  var library6 = createLibrary("test_null_properties", null, false);
  var r6 = library6.getItemByGuid(guid);
  assertEqual(r6.getProperties().length, originalLength + 1);
  assertEqual(r6.getProperty(SBProperties.contentMimeType), "");

  r.setProperty(SBProperties.contentMimeType, null);
  assertEqual(r.getProperties().length, originalLength);
  assertEqual(r.getProperty(SBProperties.contentMimeType), null);
  library.sync();

  var library7 = createLibrary("test_null_properties", null, false);
  var r7 = library7.getItemByGuid(guid);
  assertEqual(r7.getProperties().length, originalLength);
  assertEqual(r7.getProperty(SBProperties.contentMimeType), null);

}

function countProperties(resourceGuid) {
  var dbq = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
              .createInstance(Ci.sbIDatabaseQuery);

  dbq.setDatabaseGUID("test_null_properties");
  dbq.setAsyncQuery(false);
  dbq.addQuery("select count(1) from resource_properties where guid = '" + resourceGuid + "'");
  dbq.execute();

  var dbr = dbq.getResultObject();
  return parseInt(dbr.getRowCell(0, 0));
}

