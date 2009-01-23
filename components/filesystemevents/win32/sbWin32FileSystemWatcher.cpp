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

#include "sbWin32FileSystemWatcher.h"

#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsIObserverService.h>
#include <nsMemory.h>
#include <nsAutoLock.h>
#include <sbStringUtils.h>

#define BUFFER_LEN 1024 * 64


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
    return;
  }

  // Extract the event paths
  FILE_NOTIFY_INFORMATION *fileInfo = 
    (FILE_NOTIFY_INFORMATION *)watcher->GetBuffer();
  if (fileInfo) {
    {
      nsAutoLock lock(watcher->GetEventPathsQueueLock());

      while (PR_TRUE) {
        // Push in the event path into the queue
        nsString curEventPath;
        curEventPath.Assign(fileInfo->FileName,
                            (fileInfo->FileNameLength / sizeof(WCHAR)));
        
        watcher->GetEventPathsQueue()->push(curEventPath);

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

  watcher->SetIsThreadRunning(PR_FALSE);
  return 0;
}

//------------------------------------------------------------------------------

NS_IMPL_ISUPPORTS_INHERITED2(sbWin32FileSystemWatcher,
                             sbBaseFileSystemWatcher,
                             nsIObserver,
                             nsITimerCallback)

sbWin32FileSystemWatcher::sbWin32FileSystemWatcher()
{
  mRootDirHandle = INVALID_HANDLE_VALUE;
  mWatcherThread = NULL;
  mBuffer = NULL;
  mIsThreadRunning = PR_FALSE;
  mShouldRunThread = PR_FALSE;
  mEventPathsQueueLock = 
    nsAutoLock::NewLock("sbWin32FileSystemWatcher::mEventPathsQueueLock");
}

sbWin32FileSystemWatcher::~sbWin32FileSystemWatcher()
{
  nsAutoLock::DestroyLock(mEventPathsQueueLock);

  // The thread was asked to terminate in the "quit-application" notification.
  // If it is still running, force kill it now.
  if (mWatcherThread && mIsThreadRunning) {
    TerminateThread(mWatcherThread, 0);
  }
  mWatcherThread = NULL;
}

NS_IMETHODIMP
sbWin32FileSystemWatcher::Init(sbIFileSystemListener *aListener, 
                               const nsAString & aRootPath, 
                               PRBool aIsRecursive)
{
  nsresult rv;
  nsCOMPtr<nsIObserverService> obsService = 
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = obsService->AddObserver(this, "quit-application-granted", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return sbBaseFileSystemWatcher::Init(aListener, aRootPath, aIsRecursive);
}

NS_IMETHODIMP
sbWin32FileSystemWatcher::StartWatching()
{
  if (mIsWatching) {
    return NS_OK;
  }

  mTree = new sbFileSystemTree();
  NS_ENSURE_TRUE(mTree, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = mTree->AddListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mTree->Init(mWatchPath, mIsRecursive);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbWin32FileSystemWatcher::StopWatching(PRBool aShouldSaveSession)
{
  if (!mIsWatching) {
    return NS_OK;
  }

  Cleanup();

  if (aShouldSaveSession) {
    nsresult rv = mTree->SaveTreeSession(mSessionGuid);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

void
sbWin32FileSystemWatcher::Cleanup()
{
  mShouldRunThread = PR_FALSE;
  mTimer->Cancel();

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
sbWin32FileSystemWatcher::WatchNextChange()
{
  DWORD flags =
    FILE_NOTIFY_CHANGE_FILE_NAME |
    FILE_NOTIFY_CHANGE_DIR_NAME |
    FILE_NOTIFY_CHANGE_LAST_WRITE |
    FILE_NOTIFY_CHANGE_CREATION;
  
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
    CloseHandle(mRootDirHandle);
    mRootDirHandle = INVALID_HANDLE_VALUE;
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

std::queue<nsString>* 
sbWin32FileSystemWatcher::GetEventPathsQueue()
{
  return &mEventPathsQueue;
}

PRLock* 
sbWin32FileSystemWatcher::GetEventPathsQueueLock()
{
  return mEventPathsQueueLock;
}

//------------------------------------------------------------------------------
// sbFileSystemTreeListener

NS_IMETHODIMP
sbWin32FileSystemWatcher::OnTreeReady(sbStringArray & aDirPathArray)
{
  // Setup the timer callback
  nsresult rv;
  mTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mTimer->InitWithCallback(this, 100, nsITimer::TYPE_REPEATING_SLACK);
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
  if (strcmp(aTopic, "quit-application-granted") == 0) {
    if (mIsWatching) {
      StopWatching();
    }

    // Remove observer hook
    nsresult rv;
    nsCOMPtr<nsIObserverService> obsService = 
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    obsService->RemoveObserver(this, "quit-application-granted");
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
// nsITimerCallback

NS_IMETHODIMP
sbWin32FileSystemWatcher::Notify(nsITimer *aTimer)
{
  {
    nsAutoLock lock(mEventPathsQueueLock);

    // Read the event paths out of the queue.
    while (mEventPathsQueue.size() > 0) {
      // The event paths that are in the queue are relative to the root folder.
      // These paths also contain both paths for folders and files. Since the
      // tree only wants to be informed about changes at the directory level, 
      // convert file paths to their parent folder.
      nsString curEventPath(mTree->EnsureTrailingPath(mWatchPath));
      curEventPath.Append(mEventPathsQueue.front());

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
      mTree->Update(curEventPath);

      mEventPathsQueue.pop();
    }
  }

  return NS_OK;
}

