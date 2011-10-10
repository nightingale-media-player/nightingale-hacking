/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

/**
 * \brief Test file
 */

function runTest () {

  var SB_NS = "http://getnightingale.com/data/1.0#";
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);

  var databaseGUID = "test_batchcreate";
  var library = createLibrary(databaseGUID);

  var toAdd = Cc["@getnightingale.com/moz/xpcom/threadsafe-array;1"].createInstance(Ci.nsIMutableArray);
  var propertyArray = Cc["@getnightingale.com/moz/xpcom/threadsafe-array;1"].createInstance(Ci.nsIMutableArray);
  for (var i = 1; i < 101; i++) {
    toAdd.appendElement(newURI("file:///foo/" + i + ".mp3"), false);
    var props = Cc["@getnightingale.com/Nightingale/Properties/MutablePropertyArray;1"]
                  .createInstance(Ci.sbIMutablePropertyArray);
    props.appendProperty(SB_NS + "contentLength", i);
    props.appendProperty(SB_NS + "trackNumber", i);
    propertyArray.appendElement(props, false);
  }

  var libraryListener = new TestMediaListListener();
  library.addListener(libraryListener);

  var added = library.batchCreateMediaItems(toAdd, propertyArray);

  var e = added.enumerate();
  while(e.hasMoreElements()) {
    var item = e.getNext();

    var listener = {
      _item: null,
      onEnumerationBegin: function() {
      },
      onEnumeratedItem: function(list, item) {
        this._item = item;
        return Ci.sbIMediaListEnumerationListener.CANCEL;
      },
      onEnumerationEnd: function() {
      }
    };

    library.enumerateItemsByProperty("http://getnightingale.com/data/1.0#contentURL",
                                     item.contentSrc.spec,
                                     listener,
                                     Ci.sbIMediaList.ENUMERATIONTYPE_SNAPSHOT);

    assertEqual(listener._item, item);

    assertEqual(listener._item.getProperty(SB_NS + "contentLength"),
                item.getProperty(SB_NS + "contentLength"));
    assertEqual(listener._item.getProperty(SB_NS + "trackNumber"),
                item.getProperty(SB_NS + "trackNumber"));
  }

  // Check the order of the notifications
  assertEqual(libraryListener.added.length, 100);
  for (var i = 1; i < 101; i++) {
    assertEqual(libraryListener.added[i - 1].item.contentSrc.spec, "file:///foo/" + i + ".mp3");
  }
  libraryListener.reset();

  // Do it again with duplcate URLs
  toAdd = Cc["@getnightingale.com/moz/xpcom/threadsafe-array;1"].createInstance(Ci.nsIMutableArray);
  propertyArray = Cc["@getnightingale.com/moz/xpcom/threadsafe-array;1"].createInstance(Ci.nsIMutableArray);
  for (var i = 1; i < 101; i++) {
    toAdd.appendElement(newURI("file:///foo/duplicate.mp3"), false);
    var props = Cc["@getnightingale.com/Nightingale/Properties/MutablePropertyArray;1"]
                  .createInstance(Ci.sbIMutablePropertyArray);
    props.appendProperty(SB_NS + "contentLength", i);
    props.appendProperty(SB_NS + "trackNumber", i);
    propertyArray.appendElement(props, false);
  }

  added = library.batchCreateMediaItems(toAdd, propertyArray, true);
  // Check the order of the notifications
  assertEqual(libraryListener.added.length, 100);
  for (var i = 1; i < 101; i++) {
    assertEqual(libraryListener.added[i - 1].item.contentSrc.spec, "file:///foo/duplicate.mp3");
  }

  library.removeListener(libraryListener);
}
