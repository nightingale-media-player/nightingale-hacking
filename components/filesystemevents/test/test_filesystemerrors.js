/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


function runTest()
{
  var fsWatcher = Cc["@songbirdnest.com/filesystem/watcher;1"]
                    .createInstance(Ci.sbIFileSystemWatcher);
  if (!fsWatcher.isSupported) {
    return;
  }

  // This test simply tries to bootstrap the file-system watcher at an 
  // invalid path.

  var watchDir = Cc["@mozilla.org/file/directory_service;1"]
                   .getService(Ci.nsIProperties)
                   .get("ProfD", Ci.nsIFile);

  watchDir.append("invalid_watch_dir");
  if (watchDir.exists()) {
    // Ensure that the folder does not exist for this test.
    watchDir.remove(true);
  }

  // Callback for the fs watcher error, the only method that should be 
  // implemented here is |onWatcherError()| since it should be the only
  // method called when the watcher is started with our fake path below.
  var listener = {
    onWatcherError: function(aErrorType, aDescription)
    {
      var isCorrectErrorCode = false;  // ensure |ROOT_PATH_MISSING|

      var message = "WATCHER ERROR RECIEVED: ";
      switch (aErrorType) {
        case Ci.sbIFileSystemListener.ROOT_PATH_MISSING:
          isCorrectErrorCode = true;
          message += "ROOT_PATH_MISSING ";
          break;

        case Ci.sbIFileSystemListener.INVALID_DIRECTORY:
          message += "INVALID_DIRECTORY ";
          break;

        case Ci.sbIFileSystemListener.SESSION_LOAD_ERROR:
          message += "SESSION_LOAD_ERROR ";
          break;
        
        default:
          message += "INVALID ERROR TYPE!!!! ";
      }

      LOG(message += aDescription);
      assertTrue(isCorrectErrorCode);

      fsWatcher = null;
      watchDir = null;
      testFinished();
    },
  };

  LOG("ROOT PATH TEST");

  // Start the fsWatcher at the invalid path.
  fsWatcher.init(listener, watchDir.path, true);
  fsWatcher.startWatching();
  testPending();
}


// \brief Console logging helper method.
function LOG(aMessage)
{
  dump("----------------------------------------------------------\n");
  dump(" " + aMessage + "\n");
  dump("----------------------------------------------------------\n");
}

