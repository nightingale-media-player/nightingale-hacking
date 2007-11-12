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

  var prop = "http://songbirdnest.com/data/1.0#downloadButton";

  var builder =
    Cc["@songbirdnest.com/Songbird/Properties/Builder/DownloadButton;1"]
      .createInstance(Ci.sbIDownloadButtonPropertyBuilder);

  builder.propertyID = prop;
  builder.labelKey = "property.download_button";

  var pi = builder.get();

  assertEqual(pi.type, "downloadbutton");
  assertEqual(pi.id, prop);

  assertEqual(pi.format(null),       "");
  assertEqual(pi.format("0|100|50"), "");
  assertEqual(pi.format("1|100|50"), "Download");
  assertEqual(pi.format("2|100|50"), "");
  assertEqual(pi.format("3|100|50"), "");
  assertEqual(pi.format("4|100|50"), "");
  assertEqual(pi.format("5|100|50"), "");

  var tvpi = pi.QueryInterface(Ci.sbITreeViewPropertyInfo);

  assertEqual(tvpi.getProgressMode(null),    Ci.nsITreeView.PROGRESS_NONE);
  assertEqual(tvpi.getProgressMode("0|0|0"), Ci.nsITreeView.PROGRESS_NONE);
  assertEqual(tvpi.getProgressMode("1|0|0"), Ci.nsITreeView.PROGRESS_NONE);
  assertEqual(tvpi.getProgressMode("2|0|0"), Ci.nsITreeView.PROGRESS_UNDETERMINED);
  assertEqual(tvpi.getProgressMode("3|0|0"), Ci.nsITreeView.PROGRESS_NORMAL);
  assertEqual(tvpi.getProgressMode("4|0|0"), Ci.nsITreeView.PROGRESS_NORMAL);
  assertEqual(tvpi.getProgressMode("5|0|0"), Ci.nsITreeView.PROGRESS_NONE);

  assertEqual(tvpi.getCellValue(null),       "");
  assertEqual(tvpi.getCellValue("0|100|50"), "");
  assertEqual(tvpi.getCellValue("1|100|50"), "");
  assertEqual(tvpi.getCellValue("2|100|50"), "");
  assertEqual(tvpi.getCellValue("3|100|50"), "50");
  assertEqual(tvpi.getCellValue("4|100|50"), "50");
  assertEqual(tvpi.getCellValue("5|100|50"), "");
}

