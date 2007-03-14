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

  dbq.setDatabaseGUID("test_tree_collate");
  dbq.addQuery("drop table test_collate");
  dbq.addQuery("create table test_collate (id text, ordinal text unique collate tree)");
  dbq.execute();
  dbq.waitForCompletion();
  dbq.resetQuery();

  // Test some junk data
  insert(dbq, "1", "");
  insert(dbq, "2", ".");
  insert(dbq, "3", "..");
  insert(dbq, "4", "......");
  insert(dbq, "5", null);

  clear(dbq);

  var a = [];
  insertAt(dbq, a, "a", 0);

  insertAt(dbq, a, "b", 1);

  insertAt(dbq, a, "c", 1);

  insertAt(dbq, a, "d", 0);

  insertAt(dbq, a, "e", 0);

  insertAt(dbq, a, "f", 2);

  insertAt(dbq, a, "g", 2);

  insertAt(dbq, a, "h", 2);

  return Components.results.NS_OK;
}

function insert(dbq, id, ordinal) {
  if (ordinal) {
    dbq.addQuery("insert into test_collate values ('" + id + "', '" + ordinal + "')");
  }
  else {
    dbq.addQuery("insert into test_collate values ('" + id + "', null)");
  }
  dbq.execute();
  dbq.waitForCompletion();
  dbq.resetQuery();
}

function clear(dbq) {
  dbq.addQuery("delete from test_collate");
  dbq.execute();
  dbq.waitForCompletion();
  dbq.resetQuery();
}

function assertTable(dbq, a) {
  dbq.addQuery("select id from test_collate order by ordinal asc");
  dbq.execute();
  dbq.waitForCompletion();
  var r = dbq.getResultObject();
  var rows = r.getRowCount();
  if (rows != a.length) {
    fail("query returned " + rows + " rows, array length is " + a.length);
  }

  for (var i = 0; i < rows; i++) {
    assertEqual(a[i], r.getRowCell(i, 0));
  }
  dbq.resetQuery();

}

function insertAt(dbq, a, id, index) {
  //log("insert at id = " + id + " index = " + index + "\n");
  if (a.length == 0) {
    insert(dbq, id, "0");
    a[0] = id;
  }
  else {
    if (index == a.length) {
      //log("****** last\n");
      insert(dbq, id, index);
      a.push(id);
    }
    else {
      var belowPath = getPathFromIndex(dbq, index);
      if (index == 0) {
        //log("****** first\n");
        insert(dbq, id, belowPath + ".0");
      }
      else {
        var abovePath = getPathFromIndex(dbq, index - 1);
        //log("****** between " + abovePath + " " + belowPath + "\n");
        var below = belowPath.split(".");
        var above = abovePath.split(".");
        if (below.length == above.length) {
          insert(dbq, id, belowPath + ".0");
        }
        else {
          if (below.length > above.length) {
            below[below.length - 1] = parseInt(below[below.length - 1]) - 1;
            insert(dbq, id, below.join("."));
          }
          else {
            above[above.length - 1] = parseInt(above[above.length - 1]) + 1;
            insert(dbq, id, above.join("."));
          }
        }
      }
      a.splice(index, 0, id);
    }
  }
  //log(a + "\n");
  assertTable(dbq, a);

}

function getPathFromIndex(dbq, index) {

  dbq.addQuery("select ordinal from test_collate order by ordinal asc limit 1 offset " + index);
  dbq.execute();
  dbq.waitForCompletion();
  var r = dbq.getResultObject();
  var path = r.getRowCell(0, 0);
  dbq.resetQuery();
  return path;
}

