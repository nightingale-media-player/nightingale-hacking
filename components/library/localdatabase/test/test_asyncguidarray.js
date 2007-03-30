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

function ArrayListener()
{
  this.gotLength = false;
  this.length = -1;

  this.gotGuid = false;
  this.guid = null;

  this.gotValue = false;
  this.value = null;

  this.gotMediaItemId = false;
  this.mediaItemId = null;

  this.rv = null;
}

ArrayListener.prototype.onGetLength = function(length, rv)
{
  this.length = length;
  this.rv = rv;
  this.gotLength = true;
}

ArrayListener.prototype.onGetByIndex = function(guid, rv)
{
  log("ArrayListener.prototype.onGetByIndex");
  this.guid = guid;
  this.rv = rv;
  this.gotGuid = true;
}

ArrayListener.prototype.onGetSortPropertyValueByIndex = function(value, rv)
{
  this.value = value;
  this.rv = rv;
  this.gotValue = true;
}

ArrayListener.prototype.onGetMediaItemIdByIndex = function(mediaItemId, rv)
{
  this.mediaItemId = mediaItemId;
  this.rv = rv;
  this.gotMediaItemId = true;
}

function doTest(array) {

  switch(phases[currentPhase]) {
    case "getLengthAsync":

      var listener = new ArrayListener();
      array.setListener(listener);
      array.getLengthAsync();

      var tester = function() {
        if (listener.gotLength) {
          if (listener.rv == Cr.NS_OK && listener.length == 102) {
            return 1;
          }
          else {
            return -1;
          }
        }
        return 0;
      };

      return tester;
    break;

    case "getByIndexAsync":

      var listener = new ArrayListener();
      array.setListener(listener);
      array.getByIndexAsync(0);

      var tester = function() {
        if (listener.gotGuid) {
          if (listener.rv == Cr.NS_OK &&
              listener.guid == "3E4FAFDA-AD99-11DB-9321-C22AB7121F49") {
            return 1;
          }
          else {
            return -1;
          }
        }
        return 0;
      };

      return tester;
    break;

    case "getSortPropertyValueByIndex":

      var listener = new ArrayListener();
      array.setListener(listener);
      array.getSortPropertyValueByIndexAsync(0);

      var tester = function() {
        if (listener.gotValue) {
          if (listener.rv == Cr.NS_OK &&
              listener.value == "AC Black") {
            return 1;
          }
          else {
            return -1;
          }
        }
        return 0;
      };

      return tester;
    break;

    case "getMediaItemIdByIndex":

      var listener = new ArrayListener();
      array.setListener(listener);
      array.getMediaItemIdByIndexAsync(0);

      var tester = function() {
        if (listener.gotMediaItemId) {
          if (listener.rv == Cr.NS_OK &&
              listener.mediaItemId == testMediaItemId) {
            return 1;
          }
          else {
            return -1;
          }
        }
        return 0;
      };

      return tester;
    break;
  }

  // Shouldn't get here
  var tester = function() { return -1; }
  return tester;
}

function TimerLoop(array) {
  this._array = array;
  this._timer = null;
}

TimerLoop.prototype.notify = function(timer)
{
  var shouldContinue = false;

  if (currentTester) {
    var result = currentTester.apply(this);
    if (result == 1) {
      log("phase '" + phases[currentPhase] + "' passed");
      currentTester = null;
      currentPhase++;
      shouldContinue = true;
    }

    if (result == -1) {
      currentTester = null;
      failed = true;
      shouldContinue = false;
    }

    if (result == 0) {
      shouldContinue = true;
    }
  }
  else {
    if (currentPhase < phases.length) {
      currentTester = doTest(this._array);
      shouldContinue = true;
    }
  }

  if (shouldContinue) {
    if (iterations < 1000) {
      iterations++;
      this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      this._timer.initWithCallback(this, 10, Ci.nsITimer.TYPE_ONE_SHOT);
    }
    else {
      this._array = null;
      loop = null;
      testFinished();
      fail("maximum iterations reached");
    }
  }
  else {
    this._array = null;
    loop = null;
    testFinished();
    if (failed) {
      fail("phase '" + phases[currentPhase] + "'");
    }
  }
}

var phases = ["getLengthAsync",
              "getByIndexAsync",
              "getSortPropertyValueByIndex",
              "getMediaItemIdByIndex"];
var currentPhase = 0;
var currentTester = null;
var failed = false;
var iterations = 0;
var loop;
var testMediaItemId;

function runTest () {

  // XXX: disable for now
  return;

  var databaseGUID = "test_asyncguidarray";
  var library = createLibrary(databaseGUID);

  testMediaItemId = library.QueryInterface(Ci.sbILocalDatabaseLibrary).getMediaItemIdForGuid("3E4FAFDA-AD99-11DB-9321-C22AB7121F49");

  var array = Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/AsyncGUIDArray;1"]
                .createInstance(Ci.sbILocalDatabaseAsyncGUIDArray);
  array.databaseGUID = databaseGUID;
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#albumName", true);
  array.addSort("http://songbirdnest.com/data/1.0#track", true);

  currentPhase = 0;

  loop = new TimerLoop(array);
  loop.notify();
  testPending();
}

