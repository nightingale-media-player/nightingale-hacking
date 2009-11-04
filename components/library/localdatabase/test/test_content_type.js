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
 * \brief Test for the contentType property to be always set on item creation,
 *        defaulting to "audio" if not set
 */

Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

function runTest () {
try{
  var library = createLibrary("test_content_type", null, false);
  library.clear();
  testItem(library, null);

  // test making a media list and make sure we don't have any funny properties
  // set on it that we're not supposed to
  var list = library.createMediaList("simple");
  testItem(list, null);
  library.clear();

  // test createMediaItem creates the property when not set
  var item = library.createMediaItem(newURI("http://foo.example.com/"));
  testItem(item, "audio");
  library.clear();

  // test that we don't overwrite the property given
  item = library.createMediaItem(newURI("http://foo.example.com/"),
                                 makeProperties("audio"));
  testItem(item, "audio");
  library.clear();

  item = library.createMediaItem(newURI("http://foo.example.com/"),
                                 makeProperties("video"));
  testItem(item, "video");
  library.clear();

  // test that we don't accidentally not set it via other item-creating methods
  var retval = {}
  library.createMediaItemIfNotExist(newURI("http://foo.example.com/"),
                                    makeProperties("xyzzy"),
                                    retval);
  item = retval.value;
  testItem(item, "xyzzy");
  library.clear();

  var urls = ArrayConverter.nsIArray([newURI("http://foo.example.com/")]);
  var items = library.batchCreateMediaItems(urls);
  testItem(items.queryElementAt(0, Components.interfaces.nsISupports), "audio");
  library.clear();

  var props = ArrayConverter.nsIArray([makeProperties("moof")]);
  items = library.batchCreateMediaItems(urls, props);
  testItem(items.queryElementAt(0, Components.interfaces.nsISupports), "moof");
  library.clear();

  var listenerOne = {
    onProgress: function(){},
    onComplete: function(aItems, aResult) {
      assertTrue(Components.isSuccessCode(aResult),
                 "batch create async returned failure code");
      assertEqual(1,
                  aItems.length,
                  "missing or extra items via batch create async");
      testItem(aItems.queryElementAt(0, Components.interfaces.nsISupports),
               "audio");
      testFinished();
    }
  };
  library.batchCreateMediaItemsAsync(listenerOne, urls);
  testPending();
  library.clear();

  var listenerTwo = {
    onProgress: function(){},
    onComplete: function(aItems, aResult) {
      log("Second listener complete");
      assertTrue(Components.isSuccessCode(aResult),
                 "batch create async returned failure code");
      assertEqual(1,
                  aItems.length,
                  "missing or extra items via batch create async");
      testItem(aItems.queryElementAt(0, Components.interfaces.nsISupports),
               "moof");
      library.clear();
      testFinished();
    }
  };
  library.batchCreateMediaItemsAsync(listenerTwo, urls, props);
  testPending();
  library.clear();

} catch(e) { dump(e.stack + "\n"); throw(e); }
}

function testItem(aItem, aContentType) {
  __defineGetter__("contentType",
                   function () aItem.getProperty(SBProperties.contentType));
  if (aItem.library == aItem && null === aContentType) {
    // the library may have the content type be empty instead of null :(
    assertTrue(null === contentType || "" === contentType,
               "library content type is \"" + contentType + "\" instead of null");
  }
  else {
    assertEqual(aContentType, contentType, "contentType not set on created item");
  }
  aItem.setProperty(SBProperties.contentType, null);
  assertEqual(null, contentType, "contentType not cleared on explicit setting");
}

function makeProperties(aContentType) {
  var props = Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
                .createInstance(Ci.sbIMutablePropertyArray);
  props.appendProperty(SBProperties.contentType, aContentType);
  return props;
}
