/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END NIGHTINGALE GPL
 */

function runTest() {

  // from sbRatingPropertyInfo
  var STAR_WIDTH = 14;
  var ZERO_HIT_WIDTH = 4;

  Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

  var propMan = Cc["@getnightingale.com/Nightingale/Properties/PropertyManager;1"]
                  .getService(Ci.sbIPropertyManager);
  var info = propMan.getPropertyInfo(SBProperties.rating);

  // Test that a far left click nulls the value
  var clickable = info.QueryInterface(Ci.sbIClickablePropertyInfo);
  var value = clickable.getValueForClick("4", 0, 0, 0, 0);
  assertEqual(value, null);

  // Test toggle
  value = clickable.getValueForClick("4", 0, 0, ZERO_HIT_WIDTH + (STAR_WIDTH * 4) - 1, 0);
  assertEqual(value, null);
  value = clickable.getValueForClick("1", 0, 0, ZERO_HIT_WIDTH + (STAR_WIDTH * 1) - 1, 0);
  assertEqual(value, null);

  // A zero rating is not valid
  // XXXlone yes it is now, it is treated as null, remove when bug 8033 is fixed
  assertTrue(info.validate(null));
  assertTrue(info.validate("1"));
  assertTrue(info.validate("5"));
  assertTrue(info.validate("0"));
  assertFalse(info.validate("1000"));
  assertFalse(info.validate("foo"));

  // XXXsteve zeros should be treated as null.  Remove when bug 8033 is fixed
  assertEqual(info.format("0"), null);

}
