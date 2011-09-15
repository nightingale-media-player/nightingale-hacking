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

  var sql;
  var q;
  var c;

  /*
   * Tests taken from http://sqlzoo.net/3b.htm
   */

  q = newQuery();
  q.baseTableName = "ttms"
  q.addColumn(null, "who");
  q.addColumn("country", "name");
  q.addJoin(Ci.sbISQLBuilder.JOIN_INNER, "country", null, "id", "ttms",
            "country");
  c = q.createMatchCriterionLong(null, "games",
                                 Ci.sbISQLBuilder.MATCH_EQUALS,
                                 2000);
  q.addCriterion(c);
  sql = "select who, country.name from ttms join country on ttms.country = country.id where games = 2000";
  assertEqual(sql, q.toString());


  q = newQuery();
  q.baseTableName = "ttms"
  q.addColumn(null, "who");
  q.addColumn(null, "color");
  q.addJoin(Ci.sbISQLBuilder.JOIN_INNER, "country", null, "id", "ttms",
            "country");
  c = q.createMatchCriterionString(null, "name",
                                   Ci.sbISQLBuilder.MATCH_EQUALS,
                                   "Sweden");
  q.addCriterion(c);
  sql = "select who, color from ttms join country on ttms.country = country.id where name = 'Sweden'";
  assertEqual(sql, q.toString());

  q = newQuery();
  q.baseTableName = "ttms"
  q.addColumn(null, "games");
  q.addJoin(Ci.sbISQLBuilder.JOIN_INNER, "country", null, "id", "ttms",
            "country");
  c = q.createMatchCriterionString(null, "name",
                                   Ci.sbISQLBuilder.MATCH_EQUALS,
                                   "China");
  q.addCriterion(c);
  c = q.createMatchCriterionString(null, "color",
                                   Ci.sbISQLBuilder.MATCH_EQUALS,
                                   "gold");
  q.addCriterion(c);
  sql = "select games from ttms join country on ttms.country = country.id where name = 'China' and color = 'gold'";
  assertEqual(sql, q.toString());

  q = newQuery();
  q.baseTableName = "ttws"
  q.addColumn(null, "who");
  q.addJoin(Ci.sbISQLBuilder.JOIN_INNER, "games", null, "yr", "ttws", "games");
  c = q.createMatchCriterionString(null, "city",
                                   Ci.sbISQLBuilder.MATCH_EQUALS,
                                   "Barcelona");
  q.addCriterion(c);
  sql = "select who from ttws join games on ttws.games = games.yr where city = 'Barcelona'";
  assertEqual(sql, q.toString());

  /*
   * Test critera joins
   */
  q = newQuery();
  q.baseTableName = "leftTable"
  q.addColumn(null, "bar");
  c = q.createMatchCriterionTable("leftTable", "a",
                                   Ci.sbISQLBuilder.MATCH_EQUALS,
                                   "rightTable", "b")  ;
  q.addJoinWithCriterion(Ci.sbISQLBuilder.JOIN_INNER, "right", null, c);
  sql = "select bar from leftTable join right on leftTable.a = rightTable.b";
  assertEqual(sql, q.toString());

  return Components.results.NS_OK;

}

function newQuery() {
  return Cc["@songbirdnest.com/Songbird/SQLBuilder/Select;1"]
           .createInstance(Ci.sbISQLSelectBuilder);
}

