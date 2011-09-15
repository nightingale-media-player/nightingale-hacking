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
 * \brief Test file
 */

function runTest () {
  var gLib = null;  // test lib
  var gPQS = null;  // play queue service
  var gMM = null;   // mediacore manager
  var gEventTarget = null;

  var testFixture = {
    _testURIArray: [],
    _testIndex: 0,
    _prevTestIndex: 0,
    _indexChange: false,
    _nextUri: 0,

    // call this between each test
    reset: function () {
      this._testURIArray = [];
      this._testIndex = 0;
      this._nextUri = 0;
      gPQS.clearAll();
    },

    onIndexUpdated: function (index) {
      this._indexChange = true;
    },

    onQueueOperationStarted: function() {},

    onQueueOperationCompleted: function(aCount) {
      testFinished();
    },

    // sbIMediaList methods
    testAdd: function (num) {
      var uris = this._generateURIs(num);
      for (let i = 0; i < uris.length; i++)
        this._testURIArray.push(uris[i]);
      var items = this._generateItems(uris);
      if (num == 1) {
        gPQS.mediaList.add(items[0]);
      } else {
        gPQS.mediaList.addSome(ArrayConverter.enumerator(items));
      }
      this._verifyList();
    },

    testRemove: function (indexArray) {
      // go through the list backwards so indexes are valid after splice
      indexArray.sort();
      for (let i = indexArray.length - 1; i > -1 ; i--) {
        if (indexArray[i] < this._testIndex)
          this._testIndex--;
        this._testURIArray.splice(indexArray[i], 1);
      }
      var items = this._getItemsByIndexes(indexArray);
      if (indexArray.length == 1) {
        gPQS.mediaList.remove(items[0]);
      } else {
        gPQS.mediaList.removeSome(ArrayConverter.enumerator(items));
      }
      this._verifyList();
    },

    // play queue service methods
    testQueueNext: function (num) {
      var uris = this._generateURIs(num);
      for (let i = 0; i < uris.length; i++)
        this._testURIArray.splice(this._testIndex + i, 0, uris[i]);
      var items = this._generateItems(uris);
      if (num == 1) {
        gPQS.queueNext(items[0]);
      } else {
        let simpleList = gLib.createMediaList("simple");
        simpleList.addSome(ArrayConverter.enumerator(items));
        gPQS.queueNext(simpleList);
      }
      this._verifyList();
    },

    testQueueLast: function (num) {
      var uris = this._generateURIs(num);
      for (let i = 0; i < uris.length; i++)
        this._testURIArray.push(uris[i]);
      var items = this._generateItems(uris);
      if (num == 1) {
        gPQS.queueLast(items[0]);
      } else {
        let simpleList = gLib.createMediaList("simple");
        simpleList.addSome(ArrayConverter.enumerator(items));
        gPQS.queueLast(simpleList);
      }
      this._verifyList();
    },

    testQueueSomeBefore: function(num, pos) {
      var uris = this._generateURIs(num);
      for (let i = 0; i < uris.length; i++)
        this._testURIArray.splice(pos + i, 0, uris[i]);
      if (pos <= this._testIndex)
        this._testIndex += num;
      var items = this._generateItems(uris);
      gPQS.queueSomeBefore(pos, ArrayConverter.enumerator(items));
      testPending();
      this._verifyList();
    },

    testQueueSomeNext: function(num) {
      var uris = this._generateURIs(num);
      for (let i = 0; i < uris.length; i++)
        this._testURIArray.splice(this._testIndex + i, 0, uris[i]);
      var items = this._generateItems(uris);
      gPQS.queueSomeNext(ArrayConverter.enumerator(items));
      testPending();
      this._verifyList();
    },

    testQueueSomeLast: function(num) {
      var uris = this._generateURIs(num);
      for (let i = 0; i < uris.length; i++)
        this._testURIArray.push(uris[i]);
      var items = this._generateItems(uris);
      gPQS.queueSomeLast(ArrayConverter.enumerator(items));
      testPending();
      this._verifyList();
    },

    testClearHistory: function () {
      if (this._testIndex > 0)
        this._testURIArray.splice(0, this._testIndex);
      this._testIndex = 0;
      gPQS.clearHistory();
      this._verifyList();
    },

    testClearAll: function () {
      this._testIndex = 0;
      this._testURIArray.length = 0;
      gPQS.clearAll();
      assertEqual(gPQS.index, 0);
      this._verifyList();
    },

    testSetIndex: function (pos) {
      gPQS.index = pos;
      this._testIndex = Math.min(pos, gPQS.mediaList.length);
      this._verifyList();
    },

    // sequencer methods
    testSetSequencer: function (pos) {
      return this.testSetIndex(pos);
      // initialize the sequencer view
      gMM.sequencer.view = gPQS.mediaList.createView();
      while (gMM.sequencer.viewPosition < pos)
        gMM.sequencer.next();
      while (gMM.sequencer.viewPosition > pos)
        gMM.sequencer.previous();
      gMM.sequencer.stop();
      this._testIndex = pos;
      this._verifyList();
    },

    // sbIOrderableMediaList methods
    testInsertBefore: function (num, pos) {
      var uris = this._generateURIs(num);
      for (let i = 0; i < uris.length; i++)
        this._testURIArray.splice(pos + i, 0, uris[i]);
      if (pos <= this._testIndex)
        this._testIndex += num;
      var items = this._generateItems(uris);
      if (num == 1) {
        gPQS.mediaList.insertBefore(pos, items[0]);
      } else {
        gPQS.mediaList.insertSomeBefore(pos, ArrayConverter.enumerator(items));
      }
      this._verifyList();
    },

    testMoveBefore: function (indexArray, pos) {
      indexArray.sort();
      var uris = new Array(indexArray.length);
      var movingIndex = -1;
      var newURIArray = [];
      var finalIndex = -1;
      for (let i = 0; i < indexArray.length; i++) {
        if (indexArray[i] == this._testIndex) {
          movingIndex = i;
          this._testIndex = -1;
        }
        uris[i] = this._testURIArray[indexArray[i]];
        this._testURIArray[indexArray[i]] = null;
      }
      for (let i = 0; i < pos; i++) {
        if (i == this._testIndex)
          finalIndex = newURIArray.length;
        if (this._testURIArray[i])
          newURIArray.push(this._testURIArray[i]);
      }
      for (let i = 0; i < uris.length; i++) {
        if (i == movingIndex)
          finalIndex = newURIArray.length;
        newURIArray.push(uris[i]);
      }
      for (let i = pos; i < this._testURIArray.length; i++) {
        if (i == this._testIndex)
          finalIndex = newURIArray.length;
        if (this._testURIArray[i])
          newURIArray.push(this._testURIArray[i]);
      }
      this._testURIArray = newURIArray;
      this._testIndex = finalIndex;
      if (indexArray.length == 1) {
        gPQS.mediaList.moveBefore(indexArray[0], pos);
      } else {
        gPQS.mediaList.moveSomeBefore(indexArray, indexArray.length, pos);
      }
      this._verifyList();
    },

    testMoveLast: function (indexArray) {
      indexArray.sort();
      var uris = new Array(indexArray.length);
      var movingIndex = -1;
      var newURIArray = [];
      var finalIndex = -1;
      for (let i = 0; i < indexArray.length; i++) {
        if (indexArray[i] == this._testIndex) {
          movingIndex = i;
          this._testIndex = -1;
        }
        uris[i] = this._testURIArray[indexArray[i]];
        this._testURIArray[indexArray[i]] = null;
      }
      for (let i = 0; i < this._testURIArray.length; i++) {
        if (i == this._testIndex)
          finalIndex = newURIArray.length;
        if (this._testURIArray[i])
          newURIArray.push(this._testURIArray[i]);
      }
      for (let i = 0; i < uris.length; i++) {
        if (i == movingIndex)
          finalIndex = newURIArray.length;
        newURIArray.push(uris[i]);
      }
      this._testURIArray = newURIArray;
      this._testIndex = finalIndex;
      if (indexArray.length == 1) {
        gPQS.mediaList.moveLast(indexArray[0]);
      } else {
        gPQS.mediaList.moveSomeLast(indexArray, indexArray.length);
      }
      this._verifyList();
    },

    _generateItems: function (uris) {
      var items = new Array(uris.length);
      for (var i = 0; i < uris.length; i++)
        items[i] = gLib.createMediaItem(uris[i]);
      return items;
    },

    _generateURIs: function (num) {
      var uris = new Array(num);
      for (var i = 0; i < num; i++)
        uris[i] = newURI("http://test.com/test" + this._nextUri++);
      return uris;
    },

    _getItemsByIndexes: function (indexArray) {
      var items = new Array(indexArray.length)
      for (let i = 0; i < indexArray.length; i++)
        items[i] = gPQS.mediaList.getItemByIndex(indexArray[i]);
      return items;
    },

    _verifyList: function () {
      assertEqual(gPQS.mediaList.length, this._testURIArray.length);
      assertEqual(gPQS.index, this._testIndex);
      if (this._testIndex != this._prevTestIndex) {
        assertTrue(this._indexChange);
        this._indexChange = false;
        this._prevTestIndex = this._testIndex;
      }
      for (var i = 0; i < this._testURIArray.length; i++) {
        assertTrue( this._testURIArray[i].equals(
                      gPQS.mediaList.getItemByIndex(i)["contentSrc"]) );
      }
    },
  };

  // Import some services.
  Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");

  // register a test library.
  gLib = createLibrary("test_playqueueservice");  // test library
  var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                         .getService(Ci.sbILibraryManager);
  libraryManager.registerLibrary(gLib, false);


  gPQS = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
           .getService(Ci.sbIPlayQueueService);
  assertNotEqual(gPQS, null, "Failed to get play queue service");

  if (!gPQS.mediaList.isEmpty) {
    var backupList = gLib.copyMediaList("simple",
                                        gPQS.mediaList,
                                        false);
    var backupIndex = gPQS.index;
  }

  gPQS.addListener(testFixture);

  gMM = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
          .getService(Ci.sbIMediacoreManager);

  // single item tests
  testFixture.reset();
  testFixture.testAdd(1);
  testFixture.testQueueNext(1);
  testFixture.testSetSequencer(1);
  
  testFixture.testQueueLast(1);
  testFixture.testClearHistory();
  testFixture.testSetIndex(gPQS.index + 1);
  testFixture.testRemove([1]);
  testFixture.testSetIndex(gPQS.index + 1);
  testFixture.testQueueNext(1);
  testFixture.testClearAll();

  // addSome/removeSome
  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(2);
  testFixture.testRemove([0,1,2]);

  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(2);
  testFixture.testRemove([0,2,3]);

  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(2);
  testFixture.testRemove([1,2,3]);

  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(2);
  testFixture.testRemove([2,3,4]);

  // queueNext/Last with mediaLists
  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(4);
  testFixture.testQueueNext(5);

  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(4);
  testFixture.testQueueLast(5);

  // Test queueSomeBefore
  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(3);
  testFixture.testQueueSomeBefore(5, 2);

  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(3);
  testFixture.testQueueSomeBefore(5, 3);

  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(3);
  testFixture.testQueueSomeBefore(5, 4);

  // Test queueSomeNext/Last
  testFixture.reset();
  testFixture.testQueueSomeNext(3);  // gPQS.index == gPQS.mediaList.length
  testFixture.testSetIndex(2);
  testFixture.testQueueSomeNext(1);
  testFixture.testQueueSomeLast(3);
  testFixture.testSetIndex(7);
  testFixture.testQueueSomeLast(1);  // gPQS.index == gPQS.mediaList.length
  
  // sbIOrderableMediaList
  assertTrue(gPQS.mediaList instanceof Ci.sbIOrderableMediaList);

  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(3);
  testFixture.testInsertBefore(5, 2);

  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(3);
  testFixture.testInsertBefore(5, 3);

  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(3);
  testFixture.testInsertBefore(5, 4);

  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(2);
  testFixture.testMoveBefore([0,1,2], 2);

  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(2);
  testFixture.testMoveBefore([0,1,3], 2);

  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(2);
  testFixture.testMoveBefore([2,3,4], 0);

  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(2);
  testFixture.testMoveLast([0,1]);

  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(2);
  testFixture.testMoveLast([0,1,2,3]);

  testFixture.reset();
  testFixture.testAdd(5);
  testFixture.testSetIndex(2);
  testFixture.testMoveLast([2,3,4]);


  // clean up
  testFixture.reset();

  gPQS.removeListener(testFixture);

  if (backupList)
  {
    gPQS.mediaList.addAll(backupList);
    gPQS.index = backupIndex;
  }
  libraryManager.unregisterLibrary(gLib);
}

