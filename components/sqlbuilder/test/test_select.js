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
  var c, c1, c2, c3;

  /*
   * Tests taken from http://sqlzoo.net/1.htm
   */

  q = newQuery();
  q.baseTableName = "bbc"
  q.addColumn(null, "name");
  q.addColumn(null, "region");
  q.addColumn(null, "population");
  sql = "select name, region, population from bbc";
  assertEqual(sql, q.toString());

  q = newQuery();
  q.baseTableName = "bbc"
  q.addColumn(null, "name");
  c = q.createMatchCriterionLong(null, "population",
                                 Ci.sbISQLBuilder.MATCH_GREATEREQUAL,
                                 200000000);
  q.addCriterion(c);
  sql = "select name from bbc where population >= 200000000";
  assertEqual(sql, q.toString());

  q = newQuery();
  q.baseTableName = "bbc"
  q.addColumn(null, "name");
  q.addColumn(null, "gdp / population");
  c = q.createMatchCriterionLong(null, "population",
                                 Ci.sbISQLBuilder.MATCH_GREATEREQUAL,
                                 200000000);
  q.addCriterion(c);
  sql = "select name, gdp / population from bbc where population >= 200000000";
  assertEqual(sql, q.toString());

  q = newQuery();
  q.baseTableName = "bbc"
  q.addColumn(null, "name");
  q.addColumn(null, "round(population / 1000000)");
  c = q.createMatchCriterionString(null, "region",
                                   Ci.sbISQLBuilder.MATCH_EQUALS,
                                   "South Asia");
  q.addCriterion(c);
  sql = "select name, round(population / 1000000) from bbc where region = 'South Asia'";
  assertEqual(sql, q.toString());

  q = newQuery();
  q.baseTableName = "bbc"
  q.addColumn(null, "name");
  q.addColumn(null, "population");
  c1 = q.createMatchCriterionString(null, "name",
                                    Ci.sbISQLBuilder.MATCH_EQUALS,
                                    "France");
  c2 = q.createMatchCriterionString(null, "name",
                                    Ci.sbISQLBuilder.MATCH_EQUALS,
                                    "Germany");
  c3 = q.createMatchCriterionString(null, "name",
                                    Ci.sbISQLBuilder.MATCH_EQUALS,
                                    "Italy");
  c = q.createOrCriterion(c1, c2);
  c = q.createOrCriterion(c, c3);
  q.addCriterion(c);
  sql = "select name, population from bbc where ((name = 'France' or name = 'Germany') or name = 'Italy')"
  assertEqual(sql, q.toString());

  q = newQuery();
  q.baseTableName = "bbc"
  q.addColumn(null, "name");
  c = q.createMatchCriterionString(null, "name",
                                   Ci.sbISQLBuilder.MATCH_LIKE,
                                   "%United%");
  q.addCriterion(c);
  sql = "select name from bbc where name like '%United%' ESCAPE '\\'";
  assertEqual(sql, q.toString());

  return Components.results.NS_OK;

}

function newQuery() {
  return Cc["@songbirdnest.com/Songbird/SQLBuilder/Select;1"]
           .createInstance(Ci.sbISQLSelectBuilder);
}

