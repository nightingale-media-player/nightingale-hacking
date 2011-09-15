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
  runPerfTest("guidarray default view", perfTest);
}

function perfTest(library, timer) {

  Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

  var array = newGuidArray(library);
  array.addSort(SBProperties.artistName, true);
  array.addFilter(SBProperties.hidden, new StringArrayEnumerator(["0"]), false);
  array.addFilter(SBProperties.isList, new StringArrayEnumerator(["0"]), false);

  timer.start();

  array.getGuidByIndex(0);
  array.getGuidByIndex(array.length / 2);
  array.getGuidByIndex(array.length - 1);

  timer.stop();

}
