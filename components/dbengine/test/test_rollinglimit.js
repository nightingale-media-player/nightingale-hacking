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

  var dbq = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
              .createInstance(Ci.sbIDatabaseQuery);

  var ios = Cc["@mozilla.org/network/io-service;1"]
              .createInstance(Ci.nsIIOService);

  var dir = Cc["@mozilla.org/file/directory_service;1"]
              .createInstance(Ci.nsIProperties);
              
  var testdir = dir.get("ProfD", Ci.nsIFile);

  var actualdir = testdir.clone();
  actualdir.append("db_tests");

  if(!actualdir.exists())
  {
    try {
      actualdir.create(Ci.nsIFile.DIRECTORY_TYPE, 0700);
    } catch(e) {
      //Some failures might be handled later. Some might be ignored.
      throw e;
    }
  }
  
  var uri = ios.newFileURI(actualdir);
  dbq.databaseLocation = uri;

  dbq.setDatabaseGUID("test_rollinglimit");
  dbq.addQuery("drop table test_rollinglimit");
  dbq.addQuery("create table test_rollinglimit (size integer, name text)");
  dbq.execute();
  dbq.resetQuery();

  insert(dbq, "1", "bart");
  insert(dbq, "2", "homer");
  insert(dbq, "3", "lisa");
  insert(dbq, "4", "maggie");
  insert(dbq, "5", "marge");

  // A rolling limit of 5 should finish at the third row
  dbq.resetQuery();
  dbq.rollingLimit = 5;
  dbq.rollingLimitColumnIndex = 0;
  dbq.addQuery("select size, name from test_rollinglimit order by name");
  dbq.execute();

  var r = dbq.getResultObject();
  var rows = r.getRowCount();
  assertEqual(rows, 1);
  assertEqual(r.getRowCell(0, 0), "3");
  assertEqual(r.getRowCell(0, 1), "lisa");
  assertEqual(dbq.rollingLimitResult, "3");

  // A rolling limit of 100 should not return any rows
  dbq.resetQuery();
  dbq.rollingLimit = 100;
  dbq.rollingLimitColumnIndex = 0;
  dbq.addQuery("select size, name from test_rollinglimit order by name");
  dbq.execute();

  var r = dbq.getResultObject();
  var rows = r.getRowCount();
  assertEqual(rows, 0);

  // Rolling limit of 7 descending, should stop at maggie
  dbq.resetQuery();
  dbq.rollingLimit = 7;
  dbq.rollingLimitColumnIndex = 0;
  dbq.addQuery("select size, name from test_rollinglimit order by name desc");
  dbq.execute();

  var r = dbq.getResultObject();
  var rows = r.getRowCount();
  assertEqual(rows, 1);
  assertEqual(r.getRowCell(0, 0), "4");
  assertEqual(r.getRowCell(0, 1), "maggie");
  assertEqual(dbq.rollingLimitResult, "2");
}

function insert(dbq, size, name) {
  dbq.addQuery("insert into test_rollinglimit values ('" + size + "', '" + name + "')");
  dbq.execute();
  dbq.resetQuery();
}

