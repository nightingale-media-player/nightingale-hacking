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

  var prop = "http://songbirdnest.com/data/1.0#button";

  var builder =
    Cc["@songbirdnest.com/Songbird/Properties/Builder/SimpleButton;1"]
      .createInstance(Ci.sbISimpleButtonPropertyBuilder);

  builder.propertyName = prop;
  builder.displayName = "Display";

  var pi = builder.get();

  assertEqual(pi.type, "button");
  assertEqual(pi.name, prop);
  assertEqual(pi.displayName, "Display");

  assertEqual(pi.format(null),    null);
  assertEqual(pi.format(""),      "");
  assertEqual(pi.format("foo"),   "foo");
  assertEqual(pi.format("|"),     "");
  assertEqual(pi.format("foo|"),  "foo");
  assertEqual(pi.format("foo|0"), "foo");

  var cpi = pi.QueryInterface(Ci.sbIClickablePropertyInfo);
  assertEqual(pi.isDisabled(null),    false);
  assertEqual(pi.isDisabled(""),      false);
  assertEqual(pi.isDisabled("foo"),   false);
  assertEqual(pi.isDisabled("|"),     false);
  assertEqual(pi.isDisabled("|1"),    true);
  assertEqual(pi.isDisabled("foo|"),  false);
  assertEqual(pi.isDisabled("foo|0"), false);
  assertEqual(pi.isDisabled("foo|x"), false);
  assertEqual(pi.isDisabled("foo|1"), true);
}

