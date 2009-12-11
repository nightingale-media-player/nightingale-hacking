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

var testWindow;
var testWindowFailed;
function beginWindowTest(url, continueFunction) {
  var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"]
             .getService(Ci.nsIWindowWatcher);

  testWindow = ww.openWindow(null, url, null, null, null);
  testWindow.addEventListener("load", function() {
    continueFunction.apply(this);
  }, false);
  testWindowFailed = false;
  testPending();
}

function endWindowTest(e) {
  if (!testWindowFailed) {
    testWindowFailed = true;
    if (testWindow) {
      testWindow.close();
      testWindow = null;
    }
    if (e) {
      fail(e);
    }
    else {
      testFinished();
    }
  }
}

function continueWindowTest(fn, parameters) {

  try {
    fn.apply(this, parameters);
  }
  catch(e) {
    endWindowTest();
    fail(e);
  }
}

function safeSetTimeout(closure, timeout) {
  testWindow.setTimeout(function() {
    try {
      closure.apply(this);
    }
    catch(e) {
      endWindowTest();
      fail(e);
    }
  }, timeout);

}
