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
  var dbe = Cc["@songbirdnest.com/Songbird/DatabaseEngine;1"]
              .getService(Ci.sbIDatabaseEngine)

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

  dbq.setDatabaseGUID("test_closedatabase");

  dbq.addQuery("drop table test");
  dbq.addQuery("create table test (key text, value text)");
  dbq.addQuery("insert into test values ('mykey', 'myvalue')");
  dbq.execute();
  
  dbq.waitForCompletion();
  dbq.resetQuery();
  
  dbe.closeDatabase("test_closedatabase");
  
  var dbfile = actualdir.clone();
  dbfile.append("test_closedatabase.db")

  assertTrue(dbfile.exists());
  assertTrue(dbfile.isFile());
  
  dbfile.remove(false);
  
  assertFalse(dbfile.exists());

  return Components.results.NS_OK;
}
