/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbWin32FileSystemWatcher.h"

#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsIObserverService.h>
#include <nsMemory.h>
#include <nsThreadUtils.h>
#include <nsAutoLock.h>
#include <sbStringUtils.h>

#define BUFFER_LEN 1024 * 64

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbWin32FSWatcher:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gWin32FSWatcherLog = nsnull;
#define TRACE(args) PR_LOG(gWin32FSWatcherLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gWin32FSWatcherLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */


//------------------------------------------------------------------------------
// Async callback for ReadDirectoryChangesW()

VOID CALLBACK
ReadDirectoryChangesWCallbackRoutine(__in DWORD dwErrorCode,
                                     __in DWORD dwNumberOfBytesTransfered,
                                     __in LPOVERLAPPED lpOverlapped)
{
  
  sbWin32FileSystemWatcher *watcher =
    (sbWin32FileSystemWatcher *)lpOverlapped->hEvent;
  if (!watcher) {
    #if PR_LOGGING
      if (gWin32FSWatcherLog)
        LOG(("%s: no watcher!", __FUNCTION__));
    #endif /*(PR_LOGGING*/
    return;
  }

  if (FAILED(HRESULT_FROM_WIN32(dwErrorCode))) {
    return;
  }

  // Don't bother processing if no bytes were transfered.
  if (dwNumberOfBytesTransfered == 0) {
    return;
  }

  // Extract the event paths
  FILE_NOTIFY_INFORMATION *fileInfo =
    (FILE_NOTIFY_INFORMATION *)watcher->GetBuffer();
  if (fileInfo) {
    {
      #if PR_LOGGING
        if (gWin32FSWatcherLog)
          TRACE(("%s: found changes in %s",
                 __FUNCTION__, fileInfo->FileName));
      #endif /* PR_/LOGGING */
      nsAutoLock lock(watcher->GetEventPathsSetLock());

      while (PR_TRUE) {
        // Push in the event path into the queue
        nsString curEventPath;
        curEventPath.Assign(fileInfo->FileName,
                            (fileInfo->FileNameLength / sizeof(WCHAR)));

        watcher->GetEventPathsSet()->insert(curEventPath);

        if (fileInfo->NextEntryOffset == 0) {
          break;
        }
        fileInfo =
          (FILE_NOTIFY_INFORMATION *)((char *)fileInfo + fileInfo->NextEntryOffset);
      }
    }
  }

  watcher->WatchNextChange();
}

//------------------------------------------------------------------------------
// Background win32 thread to toggle the event chain

DWORD WINAPI BackgroundThreadProc(void *p)
{
  sbWin32FileSystemWatcher *watcher = (sbWin32FileSystemWatcher *)p;
  if (!watcher) {
    NS_WARNING("Could not get sbWin32FileSystemWatcher context in thread!");
    return 0;
  }

  while (watcher->GetShouldRunThread()) {
    // If the thread was just started, the event chain needs to be started for
    // the first time.
    if (!watcher->GetIsThreadRunning()) {
      watcher->WatchNextChange();
      watcher->SetIsThreadRunning(PR_TRUE);
    }

    SleepEx(100, TRUE);
  }
  watcher->Cleanup();

  watcher->SetIsThreadRunning(PR_FALSE);
  return 0;
}

//------------------------------------------------------------------------------

NS_IMPL_ISUPPORTS_INHERITED2(sbWin32FileSystemWatcher,
                             sbBaseFileSystemWatcher,
                             nsIObserver,
                             nsITimerCallback)

sbWin32FileSystemWatcher::sbWin32FileSystemWatcher() :
  mRootDirHandle(INVALID_HANDLE_VALUE),
  mWatcherThread(INVALID_HANDLE_VALUE),
  mBuffer(nsnull),
  mShouldRunThread(PR_FALSE),
  mIsThreadRunning(PR_FALSE),
  mShuttingDown(PR_FALSE)
{
#ifdef PR_LOGGING
  if (!gWin32FSWatcherLog) {
    gWin32FSWatcherLog = PR_NewLogModule("sbWin32FSWatcher");
  }
#endif
  mEventPathsSetLock =
    nsAutoLock::NewLock("sbWin32FileSystemWatcher::mEventPathsSetLock");
  NS_ASSERTION(mEventPathsSetLock, "Failed to create lock");
}

sbWin32FileSystemWatcher::~sbWin32FileSystemWatcher()
{
  TRACE(("%s", __FUNCTION__));
  nsAutoLock::DestroyLock(mEventPathsSetLock);

  // The thread was asked to terminate in the "quit-application" notification.
  // If it is still running, force kill it now.
  if ((mWatcherThread != INVALID_HANDLE_VALUE) && mIsThreadRunning) {
    TerminateThread(mWatcherThread, 0);
  }
  mWatcherThread = INVALID_HANDLE_VALUE;
}

NS_IMETHODIMP
sbWin32FileSystemWatcher::Init(sbIFileSystemListener *aListener,
                               const nsAString & aRootPath,
                               PRBool aIsRecursive)
{
  TRACE(("%s: root path = %s", __FUNCTION__,
         NS_ConvertUTF16toUTF8(aRootPath).get()));
  nsresult rv;
  nsCOMPtr<nsIObserverService> obsService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = obsService->AddObserver(this, "quit-application", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return sbBaseFileSystemWatcher::Init(aListener, aRootPath, aIsRecursive);
}

NS_IMETHODIMP
sbWin32FileSystemWatcher::StopWatching(PRBool aShouldSaveSession)
{
  TRACE(("%s", __FUNCTION__));

  mShuttingDown = PR_TRUE;

  if (!mIsWatching) {
    return NS_OK;
  }

  mShouldRunThread = PR_FALSE;

  if (mTimer) {
    mTimer->Cancel();
  }

  if (mRebuildThread) {
    mRebuildThread->Shutdown();
  }

  return sbBaseFileSystemWatcher::StopWatching(aShouldSaveSession);
}

void
sbWin32FileSystemWatcher::Cleanup()
{
  TRACE(("%s", __FUNCTION__));
  if (mBuffer) {
    nsMemory::Free(mBuffer);
    mBuffer = NULL;
  }

  if (mRootDirHandle != INVALID_HANDLE_VALUE) {
    CloseHandle(mRootDirHandle);
    mRootDirHandle = INVALID_HANDLE_VALUE;
  }

}

void
sbWin32FileSystemWatcher::InitRebuildThread()
{
  TRACE(("%s", __FUNCTION__));
  nsresult rv;
  
  NS_ENSURE_TRUE(!mShuttingDown, /* void */);
  
  mTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);

  rv = mTimer->InitWithCallback(this, 100, nsITimer::TYPE_REPEATING_SLACK);
  NS_ENSURE_SUCCESS(rv, /* void */);
}

void
sbWin32FileSystemWatcher::WatchNextChange()
{
  TRACE(("%s", __FUNCTION__));
  DWORD const flags =
    FILE_NOTIFY_CHANGE_FILE_NAME |
    FILE_NOTIFY_CHANGE_DIR_NAME |
    FILE_NOTIFY_CHANGE_LAST_WRITE |
    FILE_NOTIFY_CHANGE_CREATION;

  if (mRootDirHandle != INVALID_HANDLE_VALUE) {
    BOOL result =
      ReadDirectoryChangesW(mRootDirHandle,
                            mBuffer,
                            BUFFER_LEN,
                            TRUE,  // watch subdirs
                            flags,
                            NULL,
                            &mOverlapped,
                            ReadDirectoryChangesWCallbackRoutine);
    if (!result) {
      NS_WARNING("ERROR: Could not ReadDirectoryChangesW()");
      if (mRootDirHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(mRootDirHandle);
        mRootDirHandle = INVALID_HANDLE_VALUE;
      }
    }
  }
}

PRBool
sbWin32FileSystemWatcher::GetShouldRunThread()
{
  return mShouldRunThread;
}

PRBool
sbWin32FileSystemWatcher::GetIsThreadRunning()
{
  return mIsThreadRunning;
}

void
sbWin32FileSystemWatcher::SetIsThreadRunning(PRBool aIsThreadRunning)
{
  mIsThreadRunning = aIsThreadRunning;
}

void*
sbWin32FileSystemWatcher::GetBuffer()
{
  return mBuffer;
}

sbStringSet*
sbWin32FileSystemWatcher::GetEventPathsSet()
{
  return &mEventPathsSet;
}

PRLock*
sbWin32FileSystemWatcher::GetEventPathsSetLock()
{
  return mEventPathsSetLock;
}

//------------------------------------------------------------------------------
// sbFileSystemTreeListener

NS_IMETHODIMP
sbWin32FileSystemWatcher::OnTreeReady(const nsAString & aTreeRootPath,
                                      sbStringArray & aDirPathArray)
{
  TRACE(("%s: root path %s",
         __FUNCTION__, NS_ConvertUTF16toUTF8(aTreeRootPath).get()));
  if (mWatchPath.Equals(EmptyString())) {
    // If the watch path is empty here, this means that the tree was loaded
    // from a previous session. Set the watch path now.
    mWatchPath.Assign(aTreeRootPath);
  }

  // Setup the timer callback
  nsresult rv;
  NS_ENSURE_STATE(!mRebuildThread);

  // Setup the timer callback, on a background thread
  nsCOMPtr<nsIRunnable> initRebuildEvent =
    NS_NEW_RUNNABLE_METHOD(sbWin32FileSystemWatcher, this, InitRebuildThread);
  NS_ENSURE_TRUE(initRebuildEvent, NS_ERROR_OUT_OF_MEMORY);

  rv = NS_NewThread(getter_AddRefs(mRebuildThread), initRebuildEvent);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get a handle to the root directory.
  mRootDirHandle =
    CreateFileW(mWatchPath.get(),            // path
                GENERIC_READ,                // desired access
                FILE_SHARE_READ |            // shared mode
                FILE_SHARE_WRITE,
                NULL,                        // security attributes
                OPEN_EXISTING,               // creation disposition
                FILE_FLAG_BACKUP_SEMANTICS | // flags and attributes
                FILE_FLAG_OVERLAPPED,
                NULL);                       // template file

  NS_ENSURE_TRUE(mRootDirHandle != INVALID_HANDLE_VALUE, NS_ERROR_UNEXPECTED);

  memset(&mOverlapped, 0, sizeof(mOverlapped));
  mOverlapped.hEvent = (HANDLE)this;

  if (!mBuffer) {
    mBuffer = nsMemory::Alloc(BUFFER_LEN);
  }

  // Start the watcher thread.
  if (!mIsThreadRunning) {
    mShouldRunThread = PR_TRUE;
    mWatcherThread = CreateThread(NULL, 0, BackgroundThreadProc, this, 0, NULL);
    NS_ENSURE_TRUE(mWatcherThread != INVALID_HANDLE_VALUE, NS_ERROR_OUT_OF_MEMORY);
  }

  mIsWatching = PR_TRUE;

  rv = mListener->OnWatcherStarted();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//------------------------------------------------------------------------------
// nsIObserver

NS_IMETHODIMP
sbWin32FileSystemWatcher::Observe(nsISupports *aObject,
                                  const char *aTopic,
                                  const PRUnichar *aData)
{
  LOG(("%s: observing %s", __FUNCTION__, aTopic));
  if (strcmp(aTopic, "quit-application") == 0) {
    if (mIsWatching) {
      // Pass in PR_FALSE - the owner of the file system watcher should stop
      // the class themselves in order to save the current tree to disk.
      StopWatching(PR_FALSE);
    }

    // Remove observer hook
    nsresult rv;
    nsCOMPtr<nsIObserverService> obsService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    obsService->RemoveObserver(this, "quit-application");
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
// nsITimerCallback

NS_IMETHODIMP
sbWin32FileSystemWatcher::Notify(nsITimer *aTimer)
{
  TRACE(("%s", __FUNCTION__));
  {
    nsAutoLock lock(mEventPathsSetLock);

    sbStringSet folderEventPaths;

    // Read the event paths out of the set.
    // The event paths in |mEventPathsSet| contain the full absolute path
    // (including leaf name) of each event. Clear out the set and build a new
    // set of unique folder paths.
    while (!mEventPathsSet.empty()) {
      sbStringSetIter begin = mEventPathsSet.begin();

      nsString curEventPath(mTree->EnsureTrailingPath(mWatchPath));
      curEventPath.Append(*begin);

      // Remove the string and bump the iterator.
      mEventPathsSet.erase(begin);

      nsresult rv;
      nsCOMPtr<nsILocalFile> curEventFile =
        do_CreateInstance("@mozilla.org/file/local;1", &rv);
      if (NS_FAILED(rv)) {
        continue;
      }

      rv = curEventFile->InitWithPath(curEventPath);
      if (NS_FAILED(rv)) {
        continue;
      }

      // Since the file/folder might no longer be present on the file system,
      // manually cut out the leaf name for the file-spec to get the parent
      // folder path.
      nsString curEventFileLeafName;
      rv = curEventFile->GetLeafName(curEventFileLeafName);
      if (NS_FAILED(rv)) {
        continue;
      }

      curEventPath.Cut(curEventPath.Length() - curEventFileLeafName.Length(),
                       curEventFileLeafName.Length());
      folderEventPaths.insert(curEventPath);
    }

    // Now update the tree with the unique folder event paths.
    while (!folderEventPaths.empty()) {
      sbStringSetIter begin = folderEventPaths.begin();
      mTree->Update(*begin);
      folderEventPaths.erase(begin);
    }
  }

  return NS_OK;
}

