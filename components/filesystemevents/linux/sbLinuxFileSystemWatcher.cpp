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

#include "sbLinuxFileSystemWatcher.h"

#include <nsComponentManagerUtils.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sbDebugUtils.h>

typedef sbFileDescMap::value_type sbFileDescPair;
typedef sbFileDescMap::const_iterator sbFileDescIter;

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbLinuxFSWatcher:5
 */

//------------------------------------------------------------------------------

// Inotify callback function
static gboolean
Inotify_Callback(GIOChannel *source, GIOCondition condition, gpointer data)
{
  sbLinuxFileSystemWatcher *watcher = 
    static_cast<sbLinuxFileSystemWatcher *>(data);
  if (watcher) {
    watcher->OnInotifyEvent();
  }

  return true;
}

//------------------------------------------------------------------------------

sbLinuxFileSystemWatcher::sbLinuxFileSystemWatcher()
{
  SB_PRLOG_SETUP(sbLinuxFSWatcher);

  mIsWatching = PR_FALSE;
}

sbLinuxFileSystemWatcher::~sbLinuxFileSystemWatcher()
{
  if (mIsWatching) {
    nsresult SB_UNUSED_IN_RELEASE(rv) = Cleanup();
    NS_ASSERTION(NS_SUCCEEDED(rv), "ERROR: Could not cleanup inotify!");
  }
}

nsresult
sbLinuxFileSystemWatcher::Cleanup()
{
  // Remove all the inotify file descriptor paths
  sbFileDescIter descBegin = mFileDescMap.begin();
  sbFileDescIter descEnd = mFileDescMap.end();
  sbFileDescIter descNext;
  for (descNext = descBegin; descNext != descEnd; ++descNext) {
    inotify_rm_watch(mInotifyFileDesc, descNext->first);
  }

  // Close the main inotify file descriptor
  close(mInotifyFileDesc);

  // Remove ourselves from the main glib thread.
  if (mInotifySource) {
    g_source_remove(mInotifySource);
  }

  return NS_OK;
}

nsresult
sbLinuxFileSystemWatcher::AddInotifyHook(const nsAString & aDirPath)
{
  PRUint32 watchFlags = 
    IN_MODIFY | IN_CREATE | IN_DELETE | IN_DELETE_SELF | 
    IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO;
  
  LOG("%s: adding inotify hook for [%s]",
       __PRETTY_FUNCTION__,
       NS_ConvertUTF16toUTF8(aDirPath).get());

  int pathFileDesc = inotify_add_watch(mInotifyFileDesc,
                                       NS_ConvertUTF16toUTF8(aDirPath).get(),
                                       watchFlags);
  if (pathFileDesc == -1) {
    #if PR_LOGGING
      int errnum = errno;
      char buf[0x1000];
      char *errorDesc = strerror_r(errnum, buf, sizeof(buf));
      LOG("%s: could not add inotify watch path [%s], error %d (%s)",
           NS_ConvertUTF16toUTF8(aDirPath).get(),
           errnum,
           errorDesc);
    #endif /* PR_LOGGING */
    NS_WARNING("Could not add a inotify watch path!!");
    
    // Notify the listener of an invalid path error.
    nsresult SB_UNUSED_IN_RELEASE(rv) = 
      mListener->OnWatcherError(sbIFileSystemListener::INVALID_DIRECTORY,
                                aDirPath);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "Could not notify listener of INVALID_DIRECTORY!");

    return NS_ERROR_UNEXPECTED;
  }

  mFileDescMap.insert(sbFileDescPair(pathFileDesc, 
                                     nsString(aDirPath)));

  return NS_OK;
}

NS_IMETHODIMP 
sbLinuxFileSystemWatcher::StopWatching(PRBool aShouldSaveSession)
{
  if (!mIsWatching) {
    return NS_OK;
  }

  nsresult rv = Cleanup();
  NS_ENSURE_SUCCESS(rv, rv);

  return sbBaseFileSystemWatcher::StopWatching(aShouldSaveSession);
}

nsresult
sbLinuxFileSystemWatcher::OnInotifyEvent()
{
  // This method is called when inotify tells us an event has happened.
  // Read the inotify file-descriptor to find out what changed.
  
  // The buffer will have enough room for at least one event.
  // FWIW, on my system PATH_MAX is 4k
  char buffer[sizeof(struct inotify_event) + PATH_MAX];

  PRInt32 n = read(mInotifyFileDesc, buffer, sizeof(buffer));
  if (n > 0) {
    int i = 0;

    // for each buffer
    while (i < n) {
      // find the event structure in the buffer
      struct inotify_event *event = (struct inotify_event *) &buffer[i];

      // Find the associated path in the map.
      sbFileDescIter curEventFileDesc = mFileDescMap.find(event->wd);
      if (curEventFileDesc != mFileDescMap.end()) {
        TRACE("%s: inotify event for %s length %u",
               __PRETTY_FUNCTION__,
               NS_ConvertUTF16toUTF8(curEventFileDesc->second).get(),
               event->len);
        // If the |event| has a |len| value, something has changed. Inform
        // the tree to update at the current path.
        if (event->len) {
          mTree->Update(curEventFileDesc->second);
        }

        // If the folder was deleted or moved, we want to remove the inotify
        // hook here. The tree will go ahead and inform us of changes in 
        // |OnChangeFound()|, but only with a native path. That unfortunately
        // requires a O(n) loop through the map to find the associated file
        // descriptor. So, to keep things simple - just remove the hook here.
        if (event->mask & IN_DELETE_SELF || event->mask & IN_MOVE_SELF) {
          mFileDescMap.erase(curEventFileDesc->first);
          inotify_rm_watch(mInotifyFileDesc, curEventFileDesc->first);
        }
      }
      else {
        NS_ASSERTION(PR_FALSE, 
                     "Error: Could not find a file desc for inotify event!");
      }

      // Get the next event.
      i += sizeof(struct inotify_event) + event->len; 
    }
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
// sbFileSystemTreeListener

NS_IMETHODIMP
sbLinuxFileSystemWatcher::OnChangeFound(const nsAString & aChangePath,
                                        EChangeType aChangeType)
{
  // Only setup the inotify hooks if this class is currently watching.
  // Events that were received from a previous session will be called
  // before |OnTreeReady()|.
  if (mIsWatching) {
    // Check to see if |aChangePath| represents a directory. If it does, add
    // a new inotify hook.
    nsresult rv;
    nsCOMPtr<nsILocalFile> curPathFile = 
      do_CreateInstance("@mozilla.org/file/local;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = curPathFile->InitWithPath(aChangePath);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool exists;
    rv = curPathFile->Exists(&exists);
    NS_ENSURE_SUCCESS(rv, rv);

    if (exists) {
      PRBool isDir;
      rv = curPathFile->IsDirectory(&isDir);

      NS_ENSURE_SUCCESS(rv, rv);

      if (isDir) {
        rv = AddInotifyHook(aChangePath);
        NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not add a inotify hook!");
      }
    }
  }

  return sbBaseFileSystemWatcher::OnChangeFound(aChangePath, aChangeType);
}

NS_IMETHODIMP 
sbLinuxFileSystemWatcher::OnTreeReady(const nsAString & aTreeRootPath,
                                      sbStringArray & aDirPathArray)
{
  TRACE("%s: adding root path [%s] and friends (previously watching [%s])",
         __PRETTY_FUNCTION__,
         NS_ConvertUTF16toUTF8(aTreeRootPath).get(),
         NS_ConvertUTF16toUTF8(mWatchPath).get());
  if (mWatchPath.Equals(EmptyString())) {
    // If the watch path is empty here, this means that the tree was loaded
    // from a previous session. Set the watch path now.
    mWatchPath.Assign(aTreeRootPath);
  }
  
  // Now that the tree has been built, start the inotify file-descriptor.
  mInotifyFileDesc = inotify_init();
  NS_ENSURE_TRUE(mInotifyFileDesc != -1, NS_ERROR_UNEXPECTED);

  // Add the inotify file descriptor to the glib mainloop.
  // TODO: Check the glib return values.
  GIOChannel *ioc = g_io_channel_unix_new(mInotifyFileDesc);
  mInotifySource = g_io_add_watch_full(ioc, 
                                       G_PRIORITY_DEFAULT, 
                                       G_IO_IN, 
                                       Inotify_Callback, 
                                       this, 
                                       nsnull);
  g_io_channel_unref(ioc);

  // Add the root path, it will not be included in the passed in array.
  
  // The tree gurantess that |mWatchPath| exists when this method is called.
  // However, if inotify fails to set itself up, report an invalid dir error.
  nsresult rv = AddInotifyHook(mWatchPath);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), 
                   "Could not add inotify hook for the root watch path!");

  // Add the other directories that were discovered in the tree build.
  PRUint32 pathCount = aDirPathArray.Length();
  for (PRUint32 i = 0; i < pathCount; i++) {
    rv = AddInotifyHook(aDirPathArray[i]);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "Could not add inotify hook for a directory path!");
  }

  mIsWatching = PR_TRUE;

  rv = mListener->OnWatcherStarted();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

