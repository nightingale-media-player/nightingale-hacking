/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
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
 *=END SONGBIRD GPL
 */

/**
 * \brief Test file
 */

function runTest () {

  Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);

  var databaseGUID = "test_batchcreateifnotexist";
  var library = createLibrary(databaseGUID);

  var toAdd = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                .createInstance(Ci.nsIMutableArray);
  var propertyArray = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                        .createInstance(Ci.nsIMutableArray);
  for (var i = 1; i < 101; i++) {
    toAdd.appendElement(newURI("file:///foo/" + i + ".mp3"), false);
    var props =
          Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
            .createInstance(Ci.sbIMutablePropertyArray);
    props.appendProperty(SBProperties.contentLength, i);
    props.appendProperty(SBProperties.trackNumber, i);
    propertyArray.appendElement(props, false);
  }

  var libraryListener = new TestMediaListListener();
  library.addListener(libraryListener);

  var resultItemArray = {};
  var itemsCreated = library.batchCreateMediaItemsIfNotExist(toAdd,
                                                             propertyArray,
                                                             resultItemArray);
  resultItemArray = resultItemArray.value;

  // Check that all items were newly created
  var e = itemsCreated.enumerate();
  while(e.hasMoreElements()) {
    var itemCreated = e.getNext().QueryInterface(Ci.nsIVariant);
    assertTrue(itemCreated);
  }

  e = resultItemArray.enumerate();
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

    library.enumerateItemsByProperty(SBProperties.contentURL,
                                     item.contentSrc.spec,
                                     listener,
                                     Ci.sbIMediaList.ENUMERATIONTYPE_SNAPSHOT);

    assertEqual(listener._item, item);

    assertEqual(listener._item.getProperty(SBProperties.contentLength),
                item.getProperty(SBProperties.contentLength));
    assertEqual(listener._item.getProperty(SBProperties.trackNumber),
                item.getProperty(SBProperties.trackNumber));
  }

  // Check the order of the notifications
  assertEqual(libraryListener.added.length, 100);
  for (var i = 1; i < 101; i++) {
    assertEqual(libraryListener.added[i - 1].item.contentSrc.spec,
                "file:///foo/" + i + ".mp3");
  }
  libraryListener.reset();

  // Do it again with duplcate URLs
  toAdd = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
            .createInstance(Ci.nsIMutableArray);
  propertyArray = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                    .createInstance(Ci.nsIMutableArray);
  for (var i = 1; i < 101; i++) {
    toAdd.appendElement(newURI("file:///foo/duplicate.mp3"), false);
    var props =
          Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
            .createInstance(Ci.sbIMutablePropertyArray);
    props.appendProperty(SBProperties.contentLength, i);
    props.appendProperty(SBProperties.trackNumber, i);
    propertyArray.appendElement(props, false);
  }

  resultItemArray = {};
  itemsCreated = library.batchCreateMediaItemsIfNotExist(toAdd,
                                                         propertyArray,
                                                         resultItemArray);
  resultItemArray = resultItemArray.value;

  // Check that only the first item was newly created
  var firstItem = true;
  e = itemsCreated.enumerate();
  while(e.hasMoreElements()) {
    var itemCreated = e.getNext().QueryInterface(Ci.nsIVariant);
    if (firstItem)
      assertTrue(itemCreated);
    else
      assertFalse(itemCreated);
    firstItem = false;
  }

  // Check that all the items were properly returned
  assertEqual(resultItemArray.length, 100);
  e = resultItemArray.enumerate();
  while(e.hasMoreElements()) {
    var item = e.getNext();
    assertEqual(item.contentSrc.spec, "file:///foo/duplicate.mp3");
  }

  // Check the order of the notifications
  assertEqual(libraryListener.added.length, 1);
  assertEqual(libraryListener.added[0].item.contentSrc.spec,
              "file:///foo/duplicate.mp3");

  library.removeListener(libraryListener);
}
