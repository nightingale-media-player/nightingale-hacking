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
 * \brief Test file
 */

function runTest () {

  Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

  var library = createLibrary("test_fts", null, false);
  library.clear();
  library.flush();

  /* XXXAus: See bug 9488 and bug 9617
  assertEqual(countFtsRows("foo"), 0);
  assertEqual(countFtsRows("bar"), 0);
  */
  assertEqual(countFtsAllRows("foo"), 0);
  assertEqual(countFtsAllRows("bar"), 0);

  var item = library.createMediaItem(newURI("http://example.com/zoo.mp3"));
  item.setProperty(SBProperties.albumName, "foo");
  library.flush();

  /* XXXAus: See bug 9488 and bug 9617
  assertEqual(countFtsRows("foo"), 1);
  assertEqual(countFtsRows("bar"), 0);
  */
  assertEqual(countFtsAllRows("foo"), 1);
  assertEqual(countFtsAllRows("bar"), 0);

  item.setProperty(SBProperties.albumName, "bar");
  library.flush();

  /* XXXAus: See bug 9488 and bug 9617
  assertEqual(countFtsRows("foo"), 0);
  assertEqual(countFtsRows("bar"), 1);
  */
  assertEqual(countFtsAllRows("foo"), 0);
  assertEqual(countFtsAllRows("bar"), 1);

  item.setProperty(SBProperties.artistName, "foo");
  library.flush();

  /* XXXAus: See bug 9488 and bug 9617
  assertEqual(countFtsRows("foo"), 1);
  assertEqual(countFtsRows("bar"), 1);
  */
  assertEqual(countFtsAllRows("foo"), 1);
  assertEqual(countFtsAllRows("bar"), 1);

  library.remove(item);
  library.flush();

  /* XXXAus: See bug 9488 and bug 9617
  assertEqual(countFtsRows("foo"), 0);
  assertEqual(countFtsRows("bar"), 0);
  */
  assertEqual(countFtsAllRows("foo"), 0);
  assertEqual(countFtsAllRows("bar"), 0);
}

/*

XXXAus: Because of bug 9488, we had to disable search against specific
        properties. See bug 9617 for more info.

function countFtsRows(obj) {
  var dbq = Cc["@getnightingale.com/Nightingale/DatabaseQuery;1"]
              .createInstance(Ci.sbIDatabaseQuery);

  dbq.setDatabaseGUID("test_fts");
  dbq.addQuery("select count(1) from resource_properties_fts where obj = '" + obj + "'");
  dbq.execute();

  var dbr = dbq.getResultObject();
  return parseInt(dbr.getRowCell(0, 0));
}

*/

function countFtsAllRows(obj) {
  var dbq = Cc["@getnightingale.com/Nightingale/DatabaseQuery;1"]
              .createInstance(Ci.sbIDatabaseQuery);

  dbq.setDatabaseGUID("test_fts");
  dbq.addQuery("select count(1) from resource_properties_fts_all where alldata match '" + obj + "'");
  dbq.execute();

  var dbr = dbq.getResultObject();
  return parseInt(dbr.getRowCell(0, 0));
}
