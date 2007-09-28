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
 * This test is not bundled with the XPI. When you build in debug mode
 * this test will be copied in to the compiled/dist/testharness/ directory,
 * and the extension will be installed to compiled/dist/extensions/.
 *
 * To run this unit test, execute:
 *   ./Songbird.exe -test xpcom_helloworld
 */
 
function runTest () {
  dump("\n\n\n\nTesting HelloWorld component\n\n\n\n");
  
  if (!Components.classes["@songbirdnest.com/Songbird/HelloWorld;1"]) {
    dump("\n\n\nWARNING:  The HelloWorld component is not installed.  Test aborted.\n\n\n");
    return Components.results.NS_OK;
  }

  var helloWorld = Components.classes["@songbirdnest.com/Songbird/HelloWorld;1"]
                   .createInstance(Components.interfaces.sbIHelloWorld);

  assertEqual(helloWorld.getMessage(), "Hello World");

  return Components.results.NS_OK;
}
