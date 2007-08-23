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
 * \brief Test file
 */

function runTest () {

  var SB_PROP_ARTISTNAME = "http://songbirdnest.com/data/1.0#artistName";

  var library = createLibrary("test_onitemupdated", null, true);

  var listener = {
    properties: null,
    onItemAdded: function(list, item) {},
    onBeforeItemRemoved: function(list, item) {},
    onAfterItemRemoved: function(list, item) {},
    onItemUpdated: function(list, item, properties) {
      this.properties = properties;
    },
    onListCleared: function(list) {},
    onBatchBegin: function(list) {},
    onBatchEnd: function(list) {}
  };

  library.addListener(listener, false);
  var item = library.createMediaItem(newURI("http://foo.com/foo.mp3"));

  var value = "The Rolling Stones";
  item.setProperty(SB_PROP_ARTISTNAME, value);

  // Item had no previous artist name property so the old value should be
  // an empty string.  This will change to null once we have IsVoid/SetVoid
  var prop = listener.properties.getPropertyAt(0);
  assertEqual(prop.name, SB_PROP_ARTISTNAME);
  assertEqual(prop.value, null);

  item.setProperty(SB_PROP_ARTISTNAME, "The Bealtes");

  prop = listener.properties.getPropertyAt(0);
  assertEqual(prop.name, SB_PROP_ARTISTNAME);
  assertEqual(prop.value, value);
}

