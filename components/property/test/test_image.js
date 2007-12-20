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

function runTest() {

  var prop = "http://songbirdnest.com/data/1.0#image";

  var builder =
    Cc["@songbirdnest.com/Songbird/Properties/Builder/Image;1"]
      .createInstance(Ci.sbIImagePropertyBuilder);

  builder.propertyID = prop;
  builder.displayName = "Display";

  var pi = builder.get();

  assertEqual(pi.type, "image");
  assertEqual(pi.id, prop);
  assertEqual(pi.displayName, "Display");
}

