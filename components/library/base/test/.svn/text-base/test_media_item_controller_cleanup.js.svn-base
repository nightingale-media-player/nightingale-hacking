/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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
 * \brief The tests the media item controller hiding code
 */

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");

const K_TRACKTYPE_REMOVED = "TEST_MEDIA_ITEM_CONTROLLER_REMOVED";
const K_TRACKTYPE_ADDED = "TEST_MEDIA_ITEM_CONTROLLER_ADDED";
const K_TRACKTYPE_SEPARATOR = '\x7F';
const K_COMPLETE_TOPIC = "songbird-media-item-controller-cleanup-complete";
const K_IDLE_TOPIC = "songbird-media-item-controller-cleanup-idle";
const K_TOTAL_ITEMS = 200;
const K_SEEN_PROP = SBProperties.base + "libraryItemControllerLastSeenTypes";
const K_HIDDEN_PROP = SBProperties.base + "libraryItemControllerTypeDisappeared";

var gLibrary = null;
var gCount = {};

function runTest () {
  gLibrary = createLibrary("test_media_item_controller_cleanup",
                           null,
                           false);


  // wait for things to clear out
  log("Processing pre-existing libraries...");
  var obs = Cc["@mozilla.org/observer-service;1"]
              .getService(Ci.nsIObserverService);
  var observer = {observe: function(aSubject, aTopic, aData) {
    obs.removeObserver(this, K_COMPLETE_TOPIC);
    obs.removeObserver(this, K_IDLE_TOPIC);
    log("Initialized: " + aTopic + " [" + aData + "]");
    // we need to sleep(0) to make sure we get out of the observer topic
    // callback loop, so that registering the next observer will not be called
    // immediately.
    sleep(0);
    testFinished();}
  };
  
  obs.addObserver(observer, K_COMPLETE_TOPIC, false);
  obs.addObserver(observer, K_IDLE_TOPIC, false);
  var cleanupSvc =
    Cc["@songbirdnest.com/Songbird/Library/MediaItemControllerCleanup;1"]
      .getService(Ci.nsIObserver);
  cleanupSvc.observe(null, "idle", null);
  testPending();
  log("Pre-existing libraries processed");

  setupForItemHidden();
}

/**
 * Counts the number of mediaitems in a medialist that match a set of properties
 * @param aList the list to act on
 * @param aFilter an array of [prop, val] pairs to filter by
 * @param aCheck an array of [prop, val] that must be true for each item
 */
function EnumeratorCounter(aList, aFilter, aCheck) {
  var count = 0;
  var exception = null;
  
  var listener = {
    onEnumerationBegin: function(aList) {},
    onEnumerationEnd: function(aList, aResult) {},
    onEnumeratedItem: function(aList, aItem) {
      try {
        for each (var prop in postFilter) {
          if (aItem.getProperty(prop[0]) !== null) {
            // this doesn't _actually_ match. yay?
            return;
          }
        }
        if (aCheck) {
          for each (var set in aCheck) {
            assertEqual(set[1],
                        aItem.getProperty(set[0]),
                        aItem.contentSrc.spec);
          }
        }
        ++count;
      } catch (e) {
        throw (exception = e);
      }
    },
    QueryInterface: XPCOMUtils.generateQI([Ci.sbIMediaListEnumerationListener])
  };
  
  // XXX: Note that we can't actually call enumerateItemsByProperties to look
  // for items that does _not_ have a given property (we can only look for items
  // with a given property to empty string); so here we split the set of
  // properties into two and do post-processing on the enumeration instead.

  var selector = aFilter.filter(function(f)f[1] !== null);
  var postFilter = aFilter.filter(function(f)f[1] === null);
  if (selector.length > 0) {
    aList.enumerateItemsByProperties(SBProperties.createArray(selector),
                                     listener);
  }
  else {
    aList.enumerateAllItems(listener);
  }

  if (exception) {
    throw exception;
  }
  return count;
}

function setupForItemHidden() {
  var obs = Cc["@mozilla.org/observer-service;1"]
              .getService(Ci.nsIObserverService);
  obs.addObserver({observe: checkItemsHidden}, K_COMPLETE_TOPIC, false);

  gLibrary.clear();
  
  gCount = {true:0, false:0};

  gLibrary.runInBatchMode(function() {
    for (var i = 0; i < K_TOTAL_ITEMS; ++i) {
      var hide = (Math.random() >= 0.5);
      ++gCount[hide];
      var uri = newURI("data:application/octet-stream," + i);
      var item = gLibrary.createMediaItem(uri);
      if (hide) {
        item.setProperty(SBProperties.trackType, K_TRACKTYPE_REMOVED);
      }
    }
  });
  
  assertEqual(K_TOTAL_ITEMS, gLibrary.length,
              "unexpected number of items added to library");
  assertEqual(gCount[true],
              gLibrary.getItemCountByProperty(SBProperties.trackType,
                                              K_TRACKTYPE_REMOVED),
              "unexpected number of items with testing track type");

  gLibrary.setProperty(K_SEEN_PROP,
                       [K_TRACKTYPE_REMOVED, K_TRACKTYPE_ADDED].join(K_TRACKTYPE_SEPARATOR));
  LibraryUtils.manager.registerLibrary(gLibrary, false);

  // since we don't actually want to wait for idle, just fire the event
  // at the cleanup component manually
  var cleanupSvc =
    Cc["@songbirdnest.com/Songbird/Library/MediaItemControllerCleanup;1"]
      .getService(Ci.nsIObserver);
  cleanupSvc.observe(null, "idle", null);
  testPending();
}

function checkItemsHidden() {
  var obs = Cc["@mozilla.org/observer-service;1"]
              .getService(Ci.nsIObserverService);
  obs.removeObserver(this, K_COMPLETE_TOPIC);

  assertEqual(K_TOTAL_ITEMS, gLibrary.length,
              "unexpected number of items added to library");
  assertEqual(gCount[true],
              gLibrary.getItemCountByProperty(SBProperties.hidden, "1"),
              "unexpected number of hidden items");
  assertEqual(gCount[true],
              gLibrary.getItemCountByProperty(K_HIDDEN_PROP, "1"),
              "unexpected number of marked items");
  var seenItems = 0;
  gLibrary.enumerateItemsByProperties(
    SBProperties.createArray([[SBProperties.trackType, K_TRACKTYPE_REMOVED],
                              [SBProperties.hidden, 1]]),
    {onEnumerationBegin: function(){},
     onEnumerationEnd: function(){},
     onEnumeratedItem: function(aList, aItem){
       assertEqual(K_TRACKTYPE_REMOVED,
                   aItem.getProperty(SBProperties.trackType),
                   "item with testing track type was not hidden");
       assertEqual("1",
                   aItem.getProperty(K_HIDDEN_PROP),
                   "item hidden incorrectly");
       ++seenItems;
     }
    });
  assertEqual(gCount[true],
              seenItems,
              "unexpected number of hidden items with testing track type");

  seenItems = 0;
  gLibrary.enumerateItemsByProperty(
    SBProperties.hidden,
    0, // not hidden
    {onEnumerationBegin: function(){},
     onEnumerationEnd: function(){},
     onEnumeratedItem: function(aList, aItem){
       assertNotEqual(K_TRACKTYPE_REMOVED,
                      aItem.getProperty(SBProperties.trackType),
                      "item with testing track type was not hidden");
       assertNotEqual("1",
                      aItem.getProperty(K_HIDDEN_PROP),
                      "item hidden incorrectly");
       ++seenItems;
     }
    });
  assertEqual(gCount[false],
              seenItems,
              "unexpected number of not hidden tracks");
  LibraryUtils.manager.unregisterLibrary(gLibrary);
  testFinished();
  
  setupForItemRestored();
}

function setupForItemRestored() {
  gLibrary.clear();
  
  gCount = {true:  {
              true:  { true:0, false: 0},
              false: { true:0, false: 0}},
            false: {
              true:  { true:0, false: 0},
              false: { true:0, false: 0}}
  };

  gLibrary.runInBatchMode(function() {
    for (var i = 0; i < K_TOTAL_ITEMS; ++i) {
      var type = (Math.random() > 0.5); // true = type_added, false = no value
      var hidden = (Math.random() > 0.5);
      var flagged = (hidden && Math.random() > 0.5); // true = has K_HIDDEN_PROP
      ++gCount[type][hidden][flagged];
  
      var uri = newURI("data:application/octet-stream," + i);
      var item = gLibrary.createMediaItem(uri);
      if (type)
        item.setProperty(SBProperties.trackType, K_TRACKTYPE_ADDED);
      if (hidden)
        item.setProperty(SBProperties.hidden, 1);
      if (flagged)
        item.setProperty(K_HIDDEN_PROP, 1);
    }
  });
  
  assertEqual(K_TOTAL_ITEMS, gLibrary.length,
              "unexpected number of items added to library");
  for each (var type in [false, true]) {
    for each (var hidden in [false, true]) {
      for each (var flagged in [false, true]) {
        var props = [
          [SBProperties.trackType,
           type ? K_TRACKTYPE_ADDED : null],
          [SBProperties.hidden,
           hidden ? 1 : 0],
          [K_HIDDEN_PROP,
           flagged ? 1 : null]];

        assertEqual(gCount[type][hidden][flagged],
                    EnumeratorCounter(gLibrary, props, props),
                    "unexpected number of initial " +
                    (type ? "custom controller" : "default controller") + "/" +
                    (hidden ? "hidden" : "visible") + "/" +
                    (flagged ? "flagged" : "unflagged") +
                    " items in library");
      }
    }
  }

  gLibrary.setProperty(K_SEEN_PROP, "");

  // trigger a restore and check again
  LibraryUtils.manager.registerLibrary(gLibrary, false);
  var obs = Cc["@mozilla.org/observer-service;1"]
              .getService(Ci.nsIObserverService);
  obs.addObserver({observe: checkItemsRestored}, K_COMPLETE_TOPIC, false);
  var cleanupSvc =
    Cc["@songbirdnest.com/Songbird/Library/MediaItemControllerCleanup;1"]
      .getService(Ci.nsIObserver);
  cleanupSvc.observe(null, "idle", null);
  
  testPending();
}

function checkItemsRestored() {
  var obs = Cc["@mozilla.org/observer-service;1"]
              .getService(Ci.nsIObserverService);
  obs.removeObserver(this, K_COMPLETE_TOPIC);

  assertEqual(K_TOTAL_ITEMS, gLibrary.length,
              "unexpected number of added items remaining library");
  
  // all items that 1) have custom controller, 2) are hidden, _AND_
  // 3) are flagged as having been hidden due to this, should be flipped back
  // to visible with the custom type still set (and unflagged)
  gCount[true][false][false] += gCount[true][true][true];
  gCount[true][true][true] = 0;
  
  for each (type in [false, true]) {
    for each (hidden in [false, true]) {
      for each (flagged in [false, true]) {
        var props = [
          [SBProperties.trackType,
           type ? K_TRACKTYPE_ADDED : null],
          [SBProperties.hidden,
           hidden ? 1 : 0],
          [K_HIDDEN_PROP,
           flagged ? 1 : null]];

        var config = [
          (type ? "custom controller" : "default controller"),
          (hidden ? "hidden" : "visible"),
          (flagged ? "flagged" : "unflagged")];
        try {
          assertEqual(gCount[type][hidden][flagged],
                      EnumeratorCounter(gLibrary, props, props),
                      "unexpected number of resulting " +
                        config.join("/") +
                        " items in library");
        } catch (e) {
          // eat the error so we can finish this test and see what other errors
          // show up
        }
      }
    }
  }

  LibraryUtils.manager.unregisterLibrary(gLibrary);
  testFinished();
}
