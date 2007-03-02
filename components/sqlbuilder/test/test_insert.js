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

  var sql;
  var q;

  q = newInsert();
  q.intoTableName = "foo";
  q.addColumn("col1");
  q.addColumn("col2");
  q.addValueString("val1");
  q.addValueLong(123);
  q.addValueNull();
  q.addValueParameter();
  sql = "insert into foo (col1, col2) values ('val1', 123, null, ?)";
  assertEqual(sql, q.toString());

  q = newQuery();
  q.baseTableName = "bbc"
  q.addColumn(null, "name");
  q.addColumn(null, "region");

  var i = newInsert();
  i.intoTableName = "foo";
  i.addColumn("name");
  i.addColumn("region");
  i.select = q;
  sql = "insert into foo (name, region) select name, region from bbc";
  assertEqual(sql, i.toString());

  return Components.results.NS_OK;

}

function newInsert() {
  return Cc["@songbirdnest.com/Songbird/SQLBuilder/Insert;1"]
           .createInstance(Ci.sbISQLInsertBuilder);
}

function newQuery() {
  return Cc["@songbirdnest.com/Songbird/SQLBuilder/Select;1"]
           .createInstance(Ci.sbISQLSelectBuilder);
}

