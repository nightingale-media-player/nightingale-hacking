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

#ifndef __SBLOCALDATABASEASYNCGUIDARRAY_H__
#define __SBLOCALDATABASEASYNCGUIDARRAY_H__

#include <sbILocalDatabaseAsyncGUIDArray.h>
#include <sbILocalDatabaseGUIDArray.h>

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsIObserver.h>
#include <nsIRunnable.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <prlock.h>
#include <prmon.h>

class nsIThread;
class nsIWeakReference;

// These need compilation unit scoping so both classes can use them
enum CommandType {
  eNone,
  eGetLength,
  eGetByIndex,
  eGetSortPropertyValueByIndex,
  eGetMediaItemIdByIndex
};

struct CommandSpec {
  CommandSpec() :
    type(eNone),
    index(0)
  {
  }
  CommandType type;
  PRUint32 index;
};

typedef nsTArray<CommandSpec> sbCommandQueue;

class sbLocalDatabaseAsyncGUIDArray : public sbILocalDatabaseAsyncGUIDArray,
                                      public nsIObserver
{
public:
  friend class CommandProcessor;

  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASEGUIDARRAY
  NS_DECL_SBILOCALDATABASEASYNCGUIDARRAY
  NS_DECL_NSIOBSERVER

  nsresult Init();
  nsresult InitalizeThread();
  nsresult ShutdownThread();
  nsresult EnqueueCommand(CommandType aType, PRUint32 aIndex);

  sbLocalDatabaseAsyncGUIDArray();

  ~sbLocalDatabaseAsyncGUIDArray();
private:
  class sbWeakAsyncListenerWrapper : public sbILocalDatabaseAsyncGUIDArrayListener
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_SBILOCALDATABASEASYNCGUIDARRAYLISTENER

    sbWeakAsyncListenerWrapper(nsIWeakReference* aWeakListener);
    ~sbWeakAsyncListenerWrapper();

    already_AddRefed<sbILocalDatabaseAsyncGUIDArrayListener> GetListener();

  private:
    nsCOMPtr<nsIWeakReference> mWeakListener;
  };

  // GUID array this class wraps
  nsCOMPtr<sbILocalDatabaseGUIDArray> mInner;

  // The weak listener wrapper
  nsRefPtr<sbWeakAsyncListenerWrapper> mWeakListenerWrapper;

  // Listener that gets notified when commands complete
  nsCOMPtr<sbILocalDatabaseAsyncGUIDArrayListener> mProxiedListener;

  // Queue of pending commands
  sbCommandQueue mQueue;

  // This monitor protects methods that are called synchronously
  PRMonitor* mSyncMonitor;

  // Monitor over mQueue and calls to InitalizeThread()
  PRMonitor* mQueueMonitor;

  // Background thread
  nsCOMPtr<nsIThread> mThread;

  // Tell our background thread it should exit
  PRPackedBool mThreadShouldExit;

  // True if the thread has been explictly shut down
  PRPackedBool mThreadShutDown;
};

class CommandProcessor : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  CommandProcessor(sbLocalDatabaseAsyncGUIDArray* aFriendArray);

  ~CommandProcessor();

protected:
  sbLocalDatabaseAsyncGUIDArray* mFriendArray;
};

#endif /* __SBLOCALDATABASEASYNCGUIDARRAY_H__ */

