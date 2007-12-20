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

  var SB_NS = "http://songbirdnest.com/data/1.0#";

  var databaseGUID = "test_distinct";
  var library = createLibrary(databaseGUID);

  var enumerator = library.getDistinctValuesForProperty(SB_NS + "albumName");
  var expected = [
    "AC Black",
    "Back in Black",
    "From the Inside",
    "Hunting High and Low",
    "The Life of Riley",
    "Memorial Album",
    "On Our Big Fat Merry-Go-Round",
    "Restless and Wild/Balls to the Wall"
  ];

  var i = 0;
  while (enumerator.hasMore()) {
    assertEqual(enumerator.getNext(), expected[i]);
    i++;
  }

  var list = library.getMediaItem("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  enumerator = list.getDistinctValuesForProperty(SB_NS + "albumName");
  expected = [
    "Back in Black",
    "Hunting High and Low"
  ];
  i = 0;
  while (enumerator.hasMore()) {
    assertEqual(enumerator.getNext(), expected[i]);
    i++;
  }

}

