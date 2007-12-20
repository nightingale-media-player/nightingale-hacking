/**
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
 * \brief Advanced DataRemote unit tests
 * 
 * Initial file to test testharness architecture. Ultimately this
 *   file would hold more advanced unit tests.
 */

function runTest () {
  prepDataRemotes();

  const drConstructor = new Components.Constructor("@songbirdnest.com/Songbird/DataRemote;1", "sbIDataRemote", "init");
  var dr = new drConstructor("foo", null);
  dr.stringValue = "Success";

  // Add section to link dataremotes to dom nodes here

  var expression = /Success/;
  if ( !expression.test(dr.stringValue) )
    return Components.results.NS_ERROR_FAILURE;

  return Components.results.NS_OK;
}

