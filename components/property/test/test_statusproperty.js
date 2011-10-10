/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

  var prop = "http://getnightingale.com/data/1.0#cdRipStatus";

  var builder =
    Cc["@getnightingale.com/Nightingale/Properties/Builder/StatusProperty;1"]
      .createInstance(Ci.sbIStatusPropertyBuilder);

  builder.propertyID = prop;
  builder.labelKey = "property.cdrip_status";
  builder.completedLabelKey = "property.cdrip_completed";
  builder.failedLabelKey = "property.cdrip_failed";
  var pi = builder.get();

  assertEqual(pi.type, "status");
  assertEqual(pi.id, prop);

  assertEqual(pi.format(null),       "Status");
  assertEqual(pi.format("0|50"), "Status");
  assertEqual(pi.format("1|50"), "");
  assertEqual(pi.format("2|100"), "Completed");
  assertEqual(pi.format("3|100"), "Unable to rip this CD track");

  var tvpi = pi.QueryInterface(Ci.sbITreeViewPropertyInfo);

  assertEqual(tvpi.getProgressMode(null),  Ci.nsITreeView.PROGRESS_NONE);
  assertEqual(tvpi.getProgressMode("0|0"), Ci.nsITreeView.PROGRESS_NONE);
  assertEqual(tvpi.getProgressMode("1|0"), Ci.nsITreeView.PROGRESS_NORMAL);
  assertEqual(tvpi.getProgressMode("2|0"), Ci.nsITreeView.PROGRESS_NONE);
  assertEqual(tvpi.getProgressMode("3|0"), Ci.nsITreeView.PROGRESS_NONE);

  assertEqual(tvpi.getCellValue(null),   "");
  assertEqual(tvpi.getCellValue("0|50"), "");
  assertEqual(tvpi.getCellValue("1|50"), "50");
  assertEqual(tvpi.getCellValue("2|100"), "");
  assertEqual(tvpi.getCellValue("3|100"), "");
}

