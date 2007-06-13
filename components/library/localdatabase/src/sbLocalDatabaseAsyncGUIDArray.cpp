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

#include "sbLocalDatabaseAsyncGUIDArray.h"

#include <nsAutoLock.h>
#include <nsComponentManagerUtils.h>
#include <nsIObserverService.h>
#include <nsIStringEnumerator.h>
#include <nsIThread.h>
#include <nsIURI.h>
#include <nsThreadUtils.h>
#include <prlog.h>
#include <sbILocalDatabasePropertyCache.h>
#include <sbLocalDatabaseCID.h>
#include <sbProxyUtils.h>

#include "sbLocalDatabaseGUIDArray.h"

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbLocalDatabaseAsyncGUIDArray:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gLocalDatabaseAsyncGUIDArrayLog = nsnull;
#define TRACE(args) PR_LOG(gLocalDatabaseAsyncGUIDArrayLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gLocalDatabaseAsyncGUIDArrayLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

static const char kShutdownMessage[] = "xpcom-shutdown-threads";

NS_IMPL_THREADSAFE_ISUPPORTS3(sbLocalDatabaseAsyncGUIDArray,
                              sbILocalDatabaseGUIDArray,
                              sbILocalDatabaseAsyncGUIDArray,
                              nsIObserver)

sbLocalDatabaseAsyncGUIDArray::sbLocalDatabaseAsyncGUIDArray() :
  mThreadShouldExit(PR_FALSE),
  mThreadShutDown(PR_FALSE)
{
#ifdef PR_LOGGING
  if (!gLocalDatabaseAsyncGUIDArrayLog) {
    gLocalDatabaseAsyncGUIDArrayLog =
      PR_NewLogModule("sbLocalDatabaseAsyncGUIDArray");
  }
#endif

  TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - Created", this));
}

sbLocalDatabaseAsyncGUIDArray::~sbLocalDatabaseAsyncGUIDArray() {

  TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - dtor", this));

  ShutdownThread();

  if (mSyncMonitor) {
    nsAutoMonitor::DestroyMonitor(mSyncMonitor);
  }

  if (mQueueMonitor) {
    nsAutoMonitor::DestroyMonitor(mQueueMonitor);
  }

  TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - Destroyed", this));
}

nsresult
sbLocalDatabaseAsyncGUIDArray::Init()
{
  nsresult rv;

  mInner = new sbLocalDatabaseGUIDArray();
  NS_ENSURE_TRUE(mInner, NS_ERROR_OUT_OF_MEMORY);

  mSyncMonitor = nsAutoMonitor::NewMonitor("sbLocalDatabaseAsyncGUIDArray::mSyncMonitor");
  NS_ENSURE_TRUE(mSyncMonitor, NS_ERROR_OUT_OF_MEMORY);

  mQueueMonitor = nsAutoMonitor::NewMonitor("sbLocalDatabaseAsyncGUIDArray::mQueueMonitor");
  NS_ENSURE_TRUE(mQueueMonitor, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, kShutdownMessage, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseAsyncGUIDArray::InitalizeThread()
{
  nsCOMPtr<nsIRunnable> runnable = new CommandProcessor(this);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  return NS_NewThread(getter_AddRefs(mThread), runnable);
}

nsresult
sbLocalDatabaseAsyncGUIDArray::ShutdownThread()
{
  if (mThread) {
    if (mQueueMonitor) {
      nsAutoMonitor mon(mQueueMonitor);
      mThreadShouldExit = PR_TRUE;
      mon.Notify();
    }

    // Join the thead
    mThread->Shutdown();
    mThread = nsnull;
  }

  mThreadShutDown = PR_TRUE;

  return NS_OK;
}

nsresult
sbLocalDatabaseAsyncGUIDArray::EnqueueCommand(CommandType aType,
                                              PRUint32 aIndex)
{
  nsresult rv;

  NS_ENSURE_STATE(mProxiedListener);
  NS_ENSURE_FALSE(mThreadShutDown, NS_ERROR_ABORT);

  TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - EnqueueCommand(%d, %d)",
         this, aType, aIndex));

  {
    nsAutoMonitor mon(mQueueMonitor);

    CommandSpec* cs = mQueue.AppendElement();
    NS_ENSURE_TRUE(cs, NS_ERROR_OUT_OF_MEMORY);
    cs->type  = aType;
    cs->index = aIndex;

    if (!mThread) {
      rv = InitalizeThread();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    mon.Notify();
  }

  return NS_OK;
}

// sbILocalDatabaseAsyncGUIDArray
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetAsyncListener(sbILocalDatabaseAsyncGUIDArrayListener* aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  nsAutoMonitor monitor(mSyncMonitor);

  nsresult rv;
  rv = SB_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                            NS_GET_IID(sbILocalDatabaseAsyncGUIDArrayListener),
                            aListener,
                            NS_PROXY_ASYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(mProxiedListener));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetAsyncListener(sbILocalDatabaseAsyncGUIDArrayListener** aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  nsAutoMonitor monitor(mSyncMonitor);

  NS_IF_ADDREF(*aListener = mProxiedListener);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetLengthAsync()
{
  return EnqueueCommand(eGetLength, 0);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetByIndexAsync(PRUint32 aIndex)
{
  return EnqueueCommand(eGetByIndex, aIndex);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetSortPropertyValueByIndexAsync(PRUint32 aIndex)
{
  return EnqueueCommand(eGetSortPropertyValueByIndex, aIndex);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetMediaItemIdByIndexAsync(PRUint32 aIndex)
{
  return EnqueueCommand(eGetMediaItemIdByIndex, aIndex);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::CloneAsyncArray(sbILocalDatabaseAsyncGUIDArray** _retval)
{
  nsAutoMonitor monitor(mSyncMonitor);

  NS_ENSURE_ARG_POINTER(_retval);

  sbLocalDatabaseAsyncGUIDArray* newArray;
  NS_NEWXPCOM(newArray, sbLocalDatabaseAsyncGUIDArray);
  NS_ENSURE_TRUE(newArray, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = newArray->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mInner->CloneInto(newArray);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = newArray);
  return NS_OK;
}

// nsIObserver
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::Observe(nsISupports *aSubject,
                                       const char *aTopic,
                                       const PRUnichar *aData)
{
  if (strcmp(aTopic, kShutdownMessage) == 0) {
    TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - Observe", this));

    ShutdownThread();

    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    observerService->RemoveObserver(this, kShutdownMessage);
  }
  return NS_OK;
}

// sbILocalDatabaseGUIDArray
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetDatabaseGUID(nsAString& aDatabaseGUID)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->GetDatabaseGUID(aDatabaseGUID);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetDatabaseGUID(const nsAString& aDatabaseGUID)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->SetDatabaseGUID(aDatabaseGUID);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetDatabaseLocation(nsIURI** aDatabaseLocation)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->GetDatabaseLocation(aDatabaseLocation);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetDatabaseLocation(nsIURI* aDatabaseLocation)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->SetDatabaseLocation(aDatabaseLocation);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetBaseTable(nsAString& aBaseTable)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->GetBaseTable(aBaseTable);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetBaseTable(const nsAString& aBaseTable)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->SetBaseTable(aBaseTable);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetBaseConstraintColumn(nsAString& aBaseConstraintColumn)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->GetBaseConstraintColumn(aBaseConstraintColumn);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetBaseConstraintColumn(const nsAString& aBaseConstraintColumn)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->SetBaseConstraintColumn(aBaseConstraintColumn);
}
     
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetBaseConstraintValue(PRUint32* aBaseConstraintValue)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->GetBaseConstraintValue(aBaseConstraintValue);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetBaseConstraintValue(PRUint32 aBaseConstraintValue)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->SetBaseConstraintValue(aBaseConstraintValue);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetFetchSize(PRUint32* aFetchSize)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->GetFetchSize(aFetchSize);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetFetchSize(PRUint32 aFetchSize)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->SetFetchSize(aFetchSize);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetIsDistinct(PRBool* aIsDistinct)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->GetIsDistinct(aIsDistinct);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetIsDistinct(PRBool aIsDistinct)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->SetIsDistinct(aIsDistinct);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetLength(PRUint32* aLength)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->GetLength(aLength);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetListener(sbILocalDatabaseGUIDArrayListener** aListener)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->GetListener(aListener);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetListener(sbILocalDatabaseGUIDArrayListener* aListener)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->SetListener(aListener);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetPropertyCache(sbILocalDatabasePropertyCache** aPropertyCache)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->GetPropertyCache(aPropertyCache);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetPropertyCache(sbILocalDatabasePropertyCache* aPropertyCache)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->SetPropertyCache(aPropertyCache);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::AddSort(const nsAString& aProperty,
                                       PRBool aAscending)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->AddSort(aProperty, aAscending);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::ClearSorts()
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->ClearSorts();
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetCurrentSort(sbIPropertyArray** aCurrentSort)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->GetCurrentSort(aCurrentSort);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::AddFilter(const nsAString& aProperty,
                                         nsIStringEnumerator* aValues,
                                         PRBool aIsSearch)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->AddFilter(aProperty, aValues, aIsSearch);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::ClearFilters()
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->ClearFilters();
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::IsIndexCached(PRUint32 aIndex,
                                             PRBool* _retval)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->IsIndexCached(aIndex, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetByIndex(PRUint32 aIndex,
                                          nsAString& _retval)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->GetByIndex(aIndex, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetSortPropertyValueByIndex(PRUint32 aIndex,
                                                           nsAString& _retval)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->GetSortPropertyValueByIndex(aIndex, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetMediaItemIdByIndex(PRUint32 aIndex,
                                                     PRUint32* _retval)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->GetMediaItemIdByIndex(aIndex, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetOrdinalByIndex(PRUint32 aIndex,
                                                 nsAString& _retval)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->GetOrdinalByIndex(aIndex, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetGuidByIndex(PRUint32 aIndex,
                                         nsAString& _retval)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->GetGuidByIndex(aIndex, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::Invalidate()
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->Invalidate();
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::Clone(sbILocalDatabaseGUIDArray** _retval)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->Clone(_retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::CloneInto(sbILocalDatabaseGUIDArray* aDest)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->CloneInto(aDest);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetFirstIndexByPrefix(const nsAString& aValue,
                                                     PRUint32* _retval)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->GetFirstIndexByPrefix(aValue, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::RemoveByIndex(PRUint32 aIndex)
{
  nsAutoMonitor monitor(mSyncMonitor);

  return mInner->RemoveByIndex(aIndex);
}

NS_IMPL_THREADSAFE_ISUPPORTS1(CommandProcessor, nsIRunnable)

CommandProcessor::CommandProcessor(sbLocalDatabaseAsyncGUIDArray* aFriendArray) :
    mFriendArray(aFriendArray)
{
  TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - CommandProcessor ctor", this));
}

CommandProcessor::~CommandProcessor()
{
  TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - CommandProcessor dtor", this));
}

NS_IMETHODIMP
CommandProcessor::Run()
{
  nsresult rv;

  TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - Background Thread Start", mFriendArray));

  while (PR_TRUE) {

    CommandSpec cs;

    // Enter the monitor and wait for something to either appear on the queue
    // or for a request to exit.  When something shows up on the queue, pop
    // it off the top and exit the monitor.
    {
      NS_ENSURE_TRUE(mFriendArray->mQueueMonitor, NS_ERROR_FAILURE);
      nsAutoMonitor mon(mFriendArray->mQueueMonitor);
  
      while (mFriendArray->mQueue.Length() == 0 &&
            !mFriendArray->mThreadShouldExit) {
        mon.Wait();
      }

      if (mFriendArray->mThreadShouldExit) {

        nsAutoMonitor monitor(mFriendArray->mSyncMonitor);

        // For each remaining item on the queue, send errors
        nsCOMPtr<sbILocalDatabaseAsyncGUIDArrayListener> listener(mFriendArray->mProxiedListener);
        for (PRUint32 i = 0; i < mFriendArray->mQueue.Length(); i++) {
          const CommandSpec& command = mFriendArray->mQueue[i];
          switch(command.type) {
            case eGetLength:
              rv = listener->OnGetLength(0, NS_ERROR_ABORT);
            break;

            case eGetByIndex:
              rv = listener->OnGetByIndex(0, EmptyString(), NS_ERROR_ABORT);
            break;

            case eGetSortPropertyValueByIndex:
              rv = listener->OnGetSortPropertyValueByIndex(0,
                                                           EmptyString(),
                                                           NS_ERROR_ABORT);
            break;

            case eGetMediaItemIdByIndex:
              rv = listener->OnGetMediaItemIdByIndex(0, 0, NS_ERROR_ABORT);
            break;

            default:
              NS_NOTREACHED("Invalid command type");
              rv = NS_ERROR_UNEXPECTED;
            break;
          }
          NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Listener notification failed");
        }

        // Break out of the loop
        break;
      }

      // Pop the next command off the top of the queue
      const CommandSpec& top = mFriendArray->mQueue[0];
      cs.type  = top.type;
      cs.index = top.index;
      mFriendArray->mQueue.RemoveElementAt(0);
    }

    // Sync lock here so we don't run over synchronous usage of the array
    {
      nsAutoMonitor monitor(mFriendArray->mSyncMonitor);

      // Just for convenience
      nsCOMPtr<sbILocalDatabaseGUIDArray> inner(mFriendArray->mInner);
      nsCOMPtr<sbILocalDatabaseAsyncGUIDArrayListener> listener(mFriendArray->mProxiedListener);

      rv = listener->OnStateChange(sbILocalDatabaseAsyncGUIDArrayListener::STATE_BUSY);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Listener notification failed");

      switch (cs.type) {
        case eGetLength:
          {
            TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - "
                   "Background GetLength", mFriendArray));

            PRUint32 length;
            nsresult innerResult = inner->GetLength(&length);
            rv = listener->OnGetLength(length, innerResult);
          }
        break;

        case eGetByIndex:
          {
            TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - "
                   "Background GetByIndex", mFriendArray));

            nsAutoString guid;
            nsresult innerResult = inner->GetByIndex(cs.index, guid);
            rv = listener->OnGetByIndex(cs.index, guid, innerResult);
          }
        break;

        case eGetSortPropertyValueByIndex:
          {
            TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - "
                   "Background GetSortPropertyValueByIndex", mFriendArray));

            nsAutoString value;
            nsresult innerResult = inner->GetSortPropertyValueByIndex(cs.index,
                                                                      value);
            rv = listener->OnGetSortPropertyValueByIndex(cs.index,
                                                         value,
                                                         innerResult);
          }
        break;

        case eGetMediaItemIdByIndex:
          {
            TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - "
                   "Background GetMediaItemIdByIndex", mFriendArray));

            PRUint32 mediaItemId;
            nsresult innerResult = inner->GetMediaItemIdByIndex(cs.index,
                                                                &mediaItemId);
            rv = listener->OnGetMediaItemIdByIndex(cs.index,
                                                   mediaItemId,
                                                   innerResult);
          }
        break;

        default:
          NS_NOTREACHED("Invalid command type");
          rv = NS_ERROR_UNEXPECTED;
        break;
      }
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Listener notification failed");

      rv = listener->OnStateChange(sbILocalDatabaseAsyncGUIDArrayListener::STATE_IDLE);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Listener notification failed");
    }
  }

  TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - Background Thread End", mFriendArray));

  return NS_OK;
}

