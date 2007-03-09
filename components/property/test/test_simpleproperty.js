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

function runTest() {
  var factory = Cc["@songbirdnest.com/Songbird/Properties/PropertyFactory;1"].
                createInstance(Ci.sbIPropertyFactory);
                
  var property = factory.createProperty("Foo", "Bar");

  assertEqual(property.name, "Foo");
  assertEqual(property.value, "Bar");
  assertEqual(typeof(property.value), "string");
  
  property = factory.createProperty("Baz", 200);
  
  assertEqual(property.name, "Baz");
  assertEqual(property.value, 200);
  assertEqual(typeof(property.value), "number");
}
