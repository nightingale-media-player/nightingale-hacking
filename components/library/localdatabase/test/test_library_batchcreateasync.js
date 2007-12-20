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

  var SB_NS = "http://songbirdnest.com/data/1.0#";
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);

  var databaseGUID = "test_batchcreateasync";
  var library = createLibrary(databaseGUID, null, true);

  var toAdd = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
  var propertyArray = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
  for (var i = 0; i < 100; i++) {
    toAdd.appendElement(newURI("file:///foo/" + i + ".mp3"), false);
    var props = Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
                  .createInstance(Ci.sbIMutablePropertyArray);
    props.appendProperty(SB_NS + "contentLength", i);
    props.appendProperty(SB_NS + "trackNumber", i + 1);
    propertyArray.appendElement(props, false);
  }

  var listener = {
    onProgress: function(index) {
    },
    onComplete: function(array) {
      for (var i = 0; i < array.length; i++) {
        var item = array.queryElementAt(i, Ci.sbIMediaItem);
        assertTrue(item.contentSrc.equals(newURI("file:///foo/" + i + ".mp3")));
        assertEqual(item.getProperty(SB_NS + "contentLength"), i);
        assertEqual(item.getProperty(SB_NS + "trackNumber"), i + 1);
      }
      testFinished();
    }
  };

  library.batchCreateMediaItemsAsync(listener, toAdd, propertyArray, true);
  testPending();
}
