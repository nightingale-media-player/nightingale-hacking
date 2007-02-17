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
  var c;

  q = newQuery();
  q.baseTableName = "bbc"
  q.addColumn(null, "name");
  c = q.createMatchCriterionString(null, "name",
                                   Ci.sbISQLBuilder.MATCH_EQUALS,
                                   "o'boy");
  q.addCriterion(c);
  sql = "select name from bbc where name = 'o''boy'";
  assertEqual(sql, q.toString());

  q = newQuery();
  q.baseTableName = "bbc"
  q.addColumn(null, "name");
  c = q.createMatchCriterionNull(null, "name",
                                 Ci.sbISQLBuilder.MATCH_EQUALS);
  q.addCriterion(c);
  sql = "select name from bbc where name is null";
  assertEqual(sql, q.toString());

  q = newQuery();
  q.baseTableName = "bbc"
  q.addColumn(null, "name");
  q.addOrder(null, "name", true);
  sql = "select name from bbc order by name asc";
  assertEqual(sql, q.toString());

  q = newQuery();
  q.baseTableName = "bbc"
  q.addColumn(null, "name");
  q.addColumn(null, "region");
  q.addOrder(null, "name", true);
  q.addOrder(null, "region", false);
  sql = "select name, region from bbc order by name asc, region desc";
  assertEqual(sql, q.toString());

  q = newQuery();
  q.baseTableName = "bbc"
  q.addColumn(null, "name");
  c = q.createMatchCriterionIn(null, "name");
  c.addString("one");
  c.addString("two");
  q.addCriterion(c);
  sql = "select name from bbc where name in ('one', 'two')";
  assertEqual(sql, q.toString());

  q = newQuery();
  q.baseTableName = "bbc"
  q.addColumn(null, "name");
  q.limit = 10;
  q.offset = 20;
  sql = "select name from bbc limit 10 offset 20";
  assertEqual(sql, q.toString());

  q = newQuery();
  q.baseTableName = "bbc"
  q.addColumn(null, "name");
  q.limitIsParameter = true;
  q.offsetIsParameter = true;
  sql = "select name from bbc limit ? offset ?";
  assertEqual(sql, q.toString());

  q = newQuery();
  q.baseTableName = "bbc"
  q.addColumn(null, "name");
  try {
  q.addSubquery(q, null);
    fail("No exception thrown");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_ILLEGAL_VALUE);
  }

  return Components.results.NS_OK;

}

function newQuery() {
  return Cc["@songbirdnest.com/Songbird/SQLBuilder/Select;1"]
           .createInstance(Ci.sbISQLSelectBuilder);
}

