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

#ifndef sbWin32FileSystemWatcher_h_
#define sbWin32FileSystemWatcher_h_

#include <sbBaseFileSystemWatcher.h>
#include <sbFileSystemTree.h>
#include <nsStringAPI.h>
#include <nsIObserver.h>
#include <nsCOMPtr.h>
#include <nsITimer.h>
#include <windows.h>
#include <set>

typedef std::set<nsString>          sbStringSet;
typedef sbStringSet::iterator sbStringSetIter; 


class sbWin32FileSystemWatcher : public sbBaseFileSystemWatcher,
                                 public nsIObserver,
                                 public nsITimerCallback
{
public:
  sbWin32FileSystemWatcher();
  virtual ~sbWin32FileSystemWatcher();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIOBSERVER
  NS_DECL_NSITIMERCALLBACK

  // sbBaseFileSystemWatcher
  NS_IMETHOD Init(sbIFileSystemListener *aListener,
                  const nsAString & aRootPath,
                  PRBool aIsRecursive);
  NS_IMETHOD StopWatching(PRBool aShouldSaveSession);

  // sbIFileSystemTreeListener
  NS_IMETHOD OnTreeReady(const nsAString & aTreeRootPath,
                         sbStringArray & aDirPathArray);

  //
  // \brief Accessor for the thread should run boolean. This returns
  //        true when the thread should be running, and false when the
  //        thread should stop.
  //
  PRBool GetShouldRunThread();
  
  //
  // \brief Accessor for the thread is running boolean. This
  //        returns true when the thread is up and running.
  //
  PRBool GetIsThreadRunning();

  //
  // \brief Setter to specify if the thread is up and running. 
  //        Note: This method should only be called by the background thread.
  //
  void SetIsThreadRunning(PRBool aIsThreadRunning);

  //
  // \brief Accessor for the event structure buffer.
  //        Note: This method should only be called by the background thread. 
  //
  void* GetBuffer();

  //
  // \brief Accessor for the event paths set.
  //
  sbStringSet* GetEventPathsSet();

  //
  // \brief Accessor for the event paths set lock.
  //
  PRLock* GetEventPathsSetLock();

  //
  // \brief Method to setup the chained call to |ReadDirectoryChangesW|.
  //        Note: This method should only be called by the background thread.
  //
  void WatchNextChange();

protected:
  friend DWORD WINAPI BackgroundThreadProc(void *p);
  void Cleanup();
  void InitRebuildThread();

private:
  nsCOMPtr<nsITimer>   mTimer;
  HANDLE               mRootDirHandle;
  HANDLE               mWatcherThread;
  nsCOMPtr<nsIThread>  mRebuildThread;
  void                 *mBuffer;
  OVERLAPPED           mOverlapped;
  PRBool               mShouldRunThread;
  PRBool               mIsThreadRunning;
  sbStringSet          mEventPathsSet;
  PRLock               *mEventPathsSetLock;
  PRBool               mShuttingDown;
};

#endif  // sbWin32FileSystemWatcher_h_

