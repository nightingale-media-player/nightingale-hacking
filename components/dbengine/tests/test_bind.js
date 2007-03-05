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
  
  assertEqual(dbq.databaseLocation.spec, uri.spec);

  dbq.setDatabaseGUID("test_bind");
  dbq.addQuery("drop table bind_test");
  dbq.addQuery("create table bind_test (utf8_column text, " +
               "string_column text, double_column real, " +
               "int32_column integer, int64_column integer)");
  dbq.addQuery("insert into bind_test values " +
               "('foo', 'bar', 1234.567, 666, 9876543210)");
  dbq.addQuery("insert into bind_test values " +
               "('Sigur Rós', 'Ágætis Byrjun', -1234.567, -666, -9876543210)");
  dbq.execute();
  dbq.waitForCompletion();
  dbq.resetQuery();

  // Test bind when there is no query
  try {
    dbq.bindUTF8StringParameter(0, "hello world");
    fail("No exception thrown");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_INVALID_POINTER);
  }

  // Test each binding method
  dbq.addQuery("select * from bind_test where utf8_column = ?");
  dbq.bindUTF8StringParameter(0, "foo");
  execAndAssertCount(dbq, 1);

  dbq.resetQuery();
  dbq.addQuery("select * from bind_test where utf8_column = ?");
  dbq.bindUTF8StringParameter(0, "Sigur Rós");
  execAndAssertCount(dbq, 1);

  dbq.resetQuery();
  dbq.addQuery("select * from bind_test where string_column = ?");
  dbq.bindStringParameter(0, "bar");
  execAndAssertCount(dbq, 1);

  dbq.resetQuery();
  dbq.addQuery("select * from bind_test where string_column = ?");
  dbq.bindStringParameter(0, "Ágætis Byrjun");
  execAndAssertCount(dbq, 1);

  dbq.resetQuery();
  dbq.addQuery("select * from bind_test where double_column = ?");
  dbq.bindDoubleParameter(0, 1234.567);
  execAndAssertCount(dbq, 1);

  dbq.resetQuery();
  dbq.addQuery("select * from bind_test where int32_column = ?");
  dbq.bindInt32Parameter(0, 666);
  execAndAssertCount(dbq, 1);

  dbq.resetQuery();
  dbq.addQuery("select * from bind_test where int32_column = ?");
  dbq.bindInt32Parameter(0, -666);
  execAndAssertCount(dbq, 1);

  dbq.resetQuery();
  dbq.addQuery("select * from bind_test where int64_column = ?");
  dbq.bindInt64Parameter(0, 9876543210);
  execAndAssertCount(dbq, 1);

  dbq.resetQuery();
  dbq.addQuery("select * from bind_test where int64_column = ?");
  dbq.bindInt64Parameter(0, -9876543210);
  execAndAssertCount(dbq, 1);

  // Test an insert -- this lets us test the null binding method
  dbq.resetQuery();
  dbq.addQuery("insert into bind_test values (?, ?, ?, ?, ?)");
  dbq.bindNullParameter(0);
  dbq.bindStringParameter(1, "bar2");
  dbq.bindDoubleParameter(2, 567.0);
  dbq.bindInt32Parameter(3, 0);
  dbq.bindInt64Parameter(4, -1);
  dbq.execute();
  dbq.waitForCompletion();

  // Make sure that null was inserted
  dbq.resetQuery();
  dbq.addQuery("select * from bind_test where utf8_column is null");
  execAndAssertCount(dbq, 1);

  // Make sure that everything else was inserted
  dbq.resetQuery();
  dbq.addQuery("select * from bind_test where string_column = ? and " + 
               "double_column = ? and int32_column = ? and int64_column = ?");
  dbq.bindStringParameter(0, "bar2");
  dbq.bindDoubleParameter(1, 567.0);
  dbq.bindInt32Parameter(2, 0);
  dbq.bindInt64Parameter(3, -1);
  execAndAssertCount(dbq, 1);

  // Test binding multiple queries
  dbq.resetQuery();
  dbq.addQuery("select * from bind_test where utf8_column = ?");
  dbq.bindUTF8StringParameter(0, "foo");
  dbq.addQuery("select * from bind_test where string_column = ?");
  dbq.bindStringParameter(0, "bar");
  dbq.addQuery("select * from bind_test where int32_column = ?");
  dbq.bindInt32Parameter(0, 0);
  execAndAssertCount(dbq, 3);

  return Components.results.NS_OK;
}

function execAndAssertCount(dbq, numRows) {
  dbq.execute();
  dbq.waitForCompletion();
  assertEqual(dbq.getResultObject().getRowCount(), numRows);
}

