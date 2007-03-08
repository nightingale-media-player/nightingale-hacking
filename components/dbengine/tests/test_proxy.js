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

var gComplete = 0;

function dbqCallback() {

}

dbqCallback.prototype = {
  onQueryEnd: function(resultObject, dbGUID, query) {
    assertEqual(resultObject.getRowCount(), 2);
    
    gComplete++;
    if(gComplete > 1)
      testFinished();
  }
};

dbqCallback.prototype.constructor = dbqCallback;

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

  dbq.setDatabaseGUID("test_sync");
  dbq.addQuery("drop table proxy_test");
  dbq.addQuery("create table proxy_test (name text, value text)");
  dbq.addQuery("insert into proxy_test values ('test 0', 'testing... 0')");
  dbq.addQuery("insert into proxy_test values ('test 1', 'testing... 1')");
  
  dbq.execute();
  dbq.waitForCompletion();
  dbq.resetQuery();

  dbq.addSimpleQueryCallback(new dbqCallback());
  dbq.addSimpleQueryCallback(new dbqCallback());
  
  dbq.addQuery("select * from proxy_test");
  dbq.execute();
  
  testPending();
  
  return Components.results.NS_OK;
}

