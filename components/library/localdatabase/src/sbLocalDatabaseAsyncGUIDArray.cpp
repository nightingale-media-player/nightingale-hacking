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

#include "sbLocalDatabaseAsyncGUIDArray.h"
#include "sbLocalDatabaseGUIDArray.h"

#include <nsComponentManagerUtils.h>
#include <nsIObserverService.h>
#include <nsIStringEnumerator.h>
#include <nsIThread.h>
#include <nsIURI.h>
#include <nsThreadUtils.h>
#include <prlog.h>
#include <sbILocalDatabasePropertyCache.h>
#include <sbLocalDatabaseCID.h>
#include <sbProxiedComponentManager.h>
#include <mozilla/Monitor.h>

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

// Shut down the thread after 45 seconds of inactivity
#define SB_LOCALDATABASE_ASYNCGUIDARRAY_THREAD_TIMEOUT (45)


static const char kShutdownMessage[] = "xpcom-shutdown-threads";

NS_IMPL_THREADSAFE_ISUPPORTS4(sbLocalDatabaseAsyncGUIDArray,
                              sbILocalDatabaseGUIDArray,
                              sbILocalDatabaseAsyncGUIDArray,
                              nsIObserver,
                              nsISupportsWeakReference)

sbLocalDatabaseAsyncGUIDArray::sbLocalDatabaseAsyncGUIDArray()
  : mSyncMonitor("sbLocalDatabaseAsyncGUIDArray::mSyncMonitor")
  , mQueueMonitor("sbLocalDatabaseAsyncGUIDArray::mQueueMonitor")
  , mThreadShouldExit(PR_FALSE)
{
#ifdef PR_LOGGING
  if (!gLocalDatabaseAsyncGUIDArrayLog) {
    gLocalDatabaseAsyncGUIDArrayLog =
      PR_NewLogModule("sbLocalDatabaseAsyncGUIDArray");
  }
#endif

  TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - Created", this));
  MOZ_COUNT_CTOR(sbLocalDatabaseAsyncGUIDArray);
}

sbLocalDatabaseAsyncGUIDArray::~sbLocalDatabaseAsyncGUIDArray() {

  TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - dtor", this));

  ShutdownThread();

  TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - Destroyed", this));
  MOZ_COUNT_DTOR(sbLocalDatabaseAsyncGUIDArray);
}

nsresult
sbLocalDatabaseAsyncGUIDArray::Init()
{
  nsresult rv;

  mInner = new sbLocalDatabaseGUIDArray();
  NS_ENSURE_TRUE(mInner, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, kShutdownMessage, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseAsyncGUIDArray::InitalizeThread()
{
  mThreadShouldExit = PR_FALSE;

  nsCOMPtr<nsIRunnable> runnable = new CommandProcessor(this);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  return NS_NewThread(getter_AddRefs(mThread), runnable);
}

nsresult
sbLocalDatabaseAsyncGUIDArray::ShutdownThread()
{
  // Note that the thread may have shut itself down due to inactivity

  if (mThread) {
    mozilla::MonitorAutoLock mon(mQueueMonitor);
    mThreadShouldExit = PR_TRUE;
    mon.Notify();

    // Join the thead
    mThread->Shutdown();
    mThread = nsnull;
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseAsyncGUIDArray::EnqueueCommand(CommandType aType,
                                              PRUint32 aIndex)
{
  nsresult rv;

  NS_ENSURE_STATE(mAsyncListenerArray.Length());

  TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - EnqueueCommand(%d, %d)",
         this, aType, aIndex));

  {
    mozilla::MonitorAutoLock mon(mQueueMonitor);

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
sbLocalDatabaseAsyncGUIDArray::AddAsyncListener
                            (sbILocalDatabaseAsyncGUIDArrayListener* aListener)
{
  TRACE(("LocalDatabaseAsyncGUIDArray[0x%.8x] - AddAsyncListener 0x%.8x",
         this, aListener));

  NS_ENSURE_ARG_POINTER(aListener);

  /* Acquiring this spins the event loop; do it before grabbing mSyncMonitor */
  nsresult rv;
  nsCOMPtr<nsIProxyObjectManager> proxyObjMgr =
      do_ProxiedGetService(NS_XPCOMPROXY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  // See if we have already added this listener.
  PRUint32 length = mAsyncListenerArray.Length();
  nsCOMPtr<nsISupports> ref = do_QueryInterface(aListener, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWeakReference> weak = do_GetWeakReference(aListener, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 i = 0; i < length; i++) {
    if(mAsyncListenerArray[i]->mRef == weak) {
      NS_WARNING("Attempted to add an async listener twice!");
      return NS_OK;
    }
  }

  sbLocalDatabaseAsyncGUIDArrayListenerInfoAutoPtr
                          info(new sbLocalDatabaseAsyncGUIDArrayListenerInfo());
  NS_ENSURE_TRUE(info, NS_ERROR_OUT_OF_MEMORY);

  rv = info->Init(proxyObjMgr, weak);
  NS_ENSURE_SUCCESS(rv, rv);

  sbLocalDatabaseAsyncGUIDArrayListenerInfoAutoPtr* added =
                              mAsyncListenerArray.AppendElement(info.forget());
  NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::RemoveAsyncListener
                            (sbILocalDatabaseAsyncGUIDArrayListener* aListener)
{
  TRACE(("LocalDatabaseAsyncGUIDArray[0x%.8x] - RemoveAsyncListener 0x%.8x",
         this, aListener));

  NS_ENSURE_ARG_POINTER(aListener);
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  nsresult rv;
  PRUint32 length = mAsyncListenerArray.Length();

  nsCOMPtr<nsIWeakReference> weak = do_GetWeakReference(aListener, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  if (NS_SUCCEEDED(rv)) {
    for (PRUint32 i = 0; i < length; i++) {
      if(mAsyncListenerArray[i]->mRef == weak) {
        mAsyncListenerArray.RemoveElementAt(i);
        return NS_OK;
      }
    }
    NS_WARNING("Attempted to remove an async listener that was never added!");
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetLengthAsync()
{
  return EnqueueCommand(eGetLength, 0);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetGuidByIndexAsync(PRUint32 aIndex)
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
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  NS_ENSURE_ARG_POINTER(_retval);

  nsRefPtr<sbLocalDatabaseAsyncGUIDArray> newArray =
    new sbLocalDatabaseAsyncGUIDArray();
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
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetDatabaseGUID(aDatabaseGUID);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetDatabaseGUID(const nsAString& aDatabaseGUID)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->SetDatabaseGUID(aDatabaseGUID);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetDatabaseLocation(nsIURI** aDatabaseLocation)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetDatabaseLocation(aDatabaseLocation);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetDatabaseLocation(nsIURI* aDatabaseLocation)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->SetDatabaseLocation(aDatabaseLocation);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetBaseTable(nsAString& aBaseTable)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetBaseTable(aBaseTable);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetBaseTable(const nsAString& aBaseTable)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->SetBaseTable(aBaseTable);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetBaseConstraintColumn(nsAString& aBaseConstraintColumn)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetBaseConstraintColumn(aBaseConstraintColumn);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetBaseConstraintColumn(const nsAString& aBaseConstraintColumn)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->SetBaseConstraintColumn(aBaseConstraintColumn);
}
     
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetBaseConstraintValue(PRUint32* aBaseConstraintValue)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetBaseConstraintValue(aBaseConstraintValue);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetBaseConstraintValue(PRUint32 aBaseConstraintValue)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->SetBaseConstraintValue(aBaseConstraintValue);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetFetchSize(PRUint32* aFetchSize)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetFetchSize(aFetchSize);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetFetchSize(PRUint32 aFetchSize)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->SetFetchSize(aFetchSize);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetIsDistinct(PRBool* aIsDistinct)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetIsDistinct(aIsDistinct);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetIsDistinct(PRBool aIsDistinct)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->SetIsDistinct(aIsDistinct);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetIsValid(PRBool *aIsValid)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetIsValid(aIsValid);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetDistinctWithSortableValues(PRBool *aDistinctWithSortableValues)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetDistinctWithSortableValues(aDistinctWithSortableValues);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetDistinctWithSortableValues(PRBool aDistinctWithSortableValues)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->SetDistinctWithSortableValues(aDistinctWithSortableValues);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetLength(PRUint32* aLength)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetLength(aLength);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetListener(sbILocalDatabaseGUIDArrayListener** aListener)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetListener(aListener);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetListener(sbILocalDatabaseGUIDArrayListener* aListener)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->SetListener(aListener);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetPropertyCache(sbILocalDatabasePropertyCache** aPropertyCache)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetPropertyCache(aPropertyCache);
}
NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetPropertyCache(sbILocalDatabasePropertyCache* aPropertyCache)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->SetPropertyCache(aPropertyCache);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SetLengthCache(
        sbILocalDatabaseGUIDArrayLengthCache *aLengthCache)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->SetLengthCache(aLengthCache);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetLengthCache(
        sbILocalDatabaseGUIDArrayLengthCache **aLengthCache)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetLengthCache(aLengthCache);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::MayInvalidate(PRUint32 * aDirtyPropIDs,
                                        PRUint32 aCount)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->MayInvalidate(aDirtyPropIDs, aCount);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::AddSort(const nsAString& aProperty,
                                       PRBool aAscending)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->AddSort(aProperty, aAscending);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::ClearSorts()
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->ClearSorts();
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetCurrentSort(sbIPropertyArray** aCurrentSort)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetCurrentSort(aCurrentSort);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::AddFilter(const nsAString& aProperty,
                                         nsIStringEnumerator* aValues,
                                         PRBool aIsSearch)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->AddFilter(aProperty, aValues, aIsSearch);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::ClearFilters()
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->ClearFilters();
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::IsIndexCached(PRUint32 aIndex,
                                             PRBool* _retval)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->IsIndexCached(aIndex, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetSortPropertyValueByIndex(PRUint32 aIndex,
                                                           nsAString& _retval)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetSortPropertyValueByIndex(aIndex, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetMediaItemIdByIndex(PRUint32 aIndex,
                                                     PRUint32* _retval)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetMediaItemIdByIndex(aIndex, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetOrdinalByIndex(PRUint32 aIndex,
                                                 nsAString& _retval)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetOrdinalByIndex(aIndex, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetGuidByIndex(PRUint32 aIndex,
                                         nsAString& _retval)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetGuidByIndex(aIndex, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetRowidByIndex(PRUint32 aIndex,
                                               PRUint64* _retval)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetRowidByIndex(aIndex, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetViewItemUIDByIndex(PRUint32 aIndex,
                                                     nsAString& _retval)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetViewItemUIDByIndex(aIndex, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::Invalidate(PRBool aInvalidateLength)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->Invalidate(aInvalidateLength);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::Clone(sbILocalDatabaseGUIDArray** _retval)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->Clone(_retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::CloneInto(sbILocalDatabaseGUIDArray* aDest)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->CloneInto(aDest);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetFirstIndexByPrefix(const nsAString& aValue,
                                                     PRUint32* _retval)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetFirstIndexByPrefix(aValue, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetFirstIndexByGuid(const nsAString& aGuid,
                                                   PRUint32* _retval)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetFirstIndexByGuid(aGuid, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::GetIndexByViewItemUID
                              (const nsAString& aViewItemUID,
                                               PRUint32* _retval)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->GetIndexByViewItemUID(aViewItemUID, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::ContainsGuid(const nsAString& aGuid,
                                            PRBool* _retval)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->ContainsGuid(aGuid, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::SuppressInvalidation(PRBool aSuppress)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->SuppressInvalidation(aSuppress);
}

NS_IMETHODIMP
sbLocalDatabaseAsyncGUIDArray::RemoveByIndex(PRUint32 aIndex)
{
  mozilla::MonitorAutoLock monitor(mSyncMonitor);

  return mInner->RemoveByIndex(aIndex);
}

nsresult
sbLocalDatabaseAsyncGUIDArray::SendOnGetLength(PRUint32 aLength,
                                               nsresult aResult)
{
  nsresult rv;
  PRBool listenSucceeded = PR_TRUE;

  PRUint32 length = mAsyncListenerArray.Length();
  for (PRUint32 i = 0; i < length; i++) {
    rv = mAsyncListenerArray[i]->mProxiedListener->OnGetLength(aLength,
                                                               aResult);
    if (NS_FAILED(rv))
      listenSucceeded = PR_FALSE;
  }
  NS_WARN_IF_FALSE(listenSucceeded, "Listener notification failed");

  return NS_OK;
}

nsresult
sbLocalDatabaseAsyncGUIDArray::SendOnGetGuidByIndex(PRUint32 aIndex,
                                                    const nsAString& aGUID,
                                                    nsresult aResult)
{
  nsresult rv;
  PRBool listenSucceeded = PR_TRUE;

  PRUint32 length = mAsyncListenerArray.Length();
  for (PRUint32 i = 0; i < length; i++) {
    rv = mAsyncListenerArray[i]->mProxiedListener->OnGetGuidByIndex(aIndex,
                                                                    aGUID,
                                                                    aResult);
    if (NS_FAILED(rv))
      listenSucceeded = PR_FALSE;
  }
  NS_WARN_IF_FALSE(listenSucceeded, "Listener notification failed");

  return NS_OK;
}

nsresult
sbLocalDatabaseAsyncGUIDArray::SendOnGetSortPropertyValueByIndex
                                                        (PRUint32 aIndex,
                                                         const nsAString& aGUID,
                                                         nsresult aResult)
{
  nsresult rv;
  PRBool listenSucceeded = PR_TRUE;

  PRUint32 length = mAsyncListenerArray.Length();
  for (PRUint32 i = 0; i < length; i++) {
    rv = mAsyncListenerArray[i]->mProxiedListener->
                          OnGetSortPropertyValueByIndex(aIndex, aGUID, aResult);
    if (NS_FAILED(rv))
      listenSucceeded = PR_FALSE;
  }
  NS_WARN_IF_FALSE(listenSucceeded, "Listener notification failed");

  return NS_OK;
}

nsresult
sbLocalDatabaseAsyncGUIDArray::SendOnGetMediaItemIdByIndex
                                                        (PRUint32 aIndex,
                                                         PRUint32 aMediaItemId,
                                                         nsresult aResult)
{
  nsresult rv;
  PRBool listenSucceeded = PR_TRUE;

  PRUint32 length = mAsyncListenerArray.Length();
  for (PRUint32 i = 0; i < length; i++) {
    rv = mAsyncListenerArray[i]->mProxiedListener->
                        OnGetMediaItemIdByIndex(aIndex, aMediaItemId, aResult);
    if (NS_FAILED(rv))
      listenSucceeded = PR_FALSE;
  }
  NS_WARN_IF_FALSE(listenSucceeded, "Listener notification failed");

  return NS_OK;
}

nsresult
sbLocalDatabaseAsyncGUIDArray::SendOnStateChange(PRUint32 aState)
{
  nsresult rv;
  PRBool listenSucceeded = PR_TRUE;

  PRUint32 length = mAsyncListenerArray.Length();
  for (PRUint32 i = 0; i < length; i++) {
    rv = mAsyncListenerArray[i]->mProxiedListener->OnStateChange(aState);
    if (NS_FAILED(rv))
      listenSucceeded = PR_FALSE;
  }
  NS_WARN_IF_FALSE(listenSucceeded, "Listener notification failed");

  return NS_OK;
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

  const PRIntervalTime timeout = 
    PR_SecondsToInterval(SB_LOCALDATABASE_ASYNCGUIDARRAY_THREAD_TIMEOUT);


  TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - Background Thread Start", mFriendArray));

  while (PR_TRUE) {

    CommandSpec cs;

    // Enter the monitor and wait for something to either appear on the queue
    // or for a request to exit.  When something shows up on the queue, pop
    // it off the top and exit the monitor.  If we time out without getting 
    // any work, shut down.
    {
      mozilla::MonitorAutoLock mon(mFriendArray->mQueueMonitor);
  
      while (mFriendArray->mQueue.Length() == 0 &&
            !mFriendArray->mThreadShouldExit) {
        rv = mon.Wait(timeout);
        
        if (NS_FAILED(rv)) {
          // Another thread had the monitor when we timed out, so
          // just go back and try again.
          continue;
        }
        
        if (mFriendArray->mQueue.Length() == 0 &&
            !mFriendArray->mThreadShouldExit) {
          // We timed out, and there is nothing in the queue, 
          // so might as well shut down.
          nsCOMPtr<nsIThread> doomed;
          // Try proxying to thread
          do_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD, NS_GET_IID(nsIThread), 
                mFriendArray->mThread, NS_PROXY_ASYNC, getter_AddRefs(doomed));
          if (doomed) {
            TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - Background Thread End Due To Timeout", mFriendArray));

            mFriendArray->mThread = nsnull;
            mFriendArray->mThreadShouldExit = PR_TRUE;
            doomed->Shutdown();
            
            // Return early to make sure nothing bad happens
            return NS_OK;
          } else {
            NS_WARNING("failed to construct proxy to main thread");
          }
        }
      }

      if (mFriendArray->mThreadShouldExit) {

        mozilla::MonitorAutoLock monitor(mFriendArray->mSyncMonitor);

        // For each remaining item on the queue, send errors
        for (PRUint32 i = 0; i < mFriendArray->mQueue.Length(); i++) {
          const CommandSpec& command = mFriendArray->mQueue[i];
          switch(command.type) {
            case eGetLength:
              rv = mFriendArray->SendOnGetLength(0, NS_ERROR_ABORT);
            break;

            case eGetByIndex:
              rv = mFriendArray->SendOnGetGuidByIndex(0,
                                                      EmptyString(),
                                                      NS_ERROR_ABORT);
            break;

            case eGetSortPropertyValueByIndex:
              rv = mFriendArray->SendOnGetSortPropertyValueByIndex(0,
                                                               EmptyString(),
                                                               NS_ERROR_ABORT);
            break;

            case eGetMediaItemIdByIndex:
              rv = mFriendArray->SendOnGetMediaItemIdByIndex(0,
                                                             0,
                                                             NS_ERROR_ABORT);
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
      mozilla::MonitorAutoLock monitor(mFriendArray->mSyncMonitor);

      // Just for convenience
      nsCOMPtr<sbILocalDatabaseGUIDArray> inner(mFriendArray->mInner);

      rv = mFriendArray->SendOnStateChange
                          (sbILocalDatabaseAsyncGUIDArrayListener::STATE_BUSY);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Listener notification failed");

      switch (cs.type) {
        case eGetLength:
          {
            TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - "
                   "Background GetLength", mFriendArray));

            PRUint32 length;
            nsresult innerResult = inner->GetLength(&length);
            rv = mFriendArray->SendOnGetLength(length, innerResult);
          }
        break;

        case eGetByIndex:
          {
            TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - "
                   "Background GetGuidByIndex", mFriendArray));

            nsAutoString guid;
            nsresult innerResult = inner->GetGuidByIndex(cs.index, guid);
            rv = mFriendArray->SendOnGetGuidByIndex(cs.index,
                                                    guid,
                                                    innerResult);
          }
        break;

        case eGetSortPropertyValueByIndex:
          {
            TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - "
                   "Background GetSortPropertyValueByIndex", mFriendArray));

            nsAutoString value;
            nsresult innerResult = inner->GetSortPropertyValueByIndex(cs.index,
                                                                      value);
            rv = mFriendArray->SendOnGetSortPropertyValueByIndex(cs.index,
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
            rv = mFriendArray->SendOnGetMediaItemIdByIndex(cs.index,
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

      rv = mFriendArray->SendOnStateChange
                          (sbILocalDatabaseAsyncGUIDArrayListener::STATE_IDLE);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Listener notification failed");
    }
  }

  TRACE(("sbLocalDatabaseAsyncGUIDArray[0x%x] - Background Thread End", mFriendArray));

  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(sbWeakAsyncListenerWrapper,
                              sbILocalDatabaseAsyncGUIDArrayListener)

sbWeakAsyncListenerWrapper::sbWeakAsyncListenerWrapper(nsIWeakReference* aWeakReference) :
  mWeakListener(aWeakReference)
{
  NS_ASSERTION(mWeakListener, "aWeakReference is null");
  MOZ_COUNT_CTOR(sbWeakAsyncListenerWrapper);
}

sbWeakAsyncListenerWrapper::~sbWeakAsyncListenerWrapper()
{
  MOZ_COUNT_DTOR(sbWeakAsyncListenerWrapper);
}

already_AddRefed<sbILocalDatabaseAsyncGUIDArrayListener>
sbWeakAsyncListenerWrapper::GetListener()
{
  nsCOMPtr<sbILocalDatabaseAsyncGUIDArrayListener> strongListener =
    do_QueryReferent(mWeakListener);
  if (!strongListener) {
    return nsnull;
  }

  sbILocalDatabaseAsyncGUIDArrayListener* listenerPtr = strongListener;
  NS_ADDREF(listenerPtr);
  return listenerPtr;
}

#define SB_TRY_NOTIFY(call)                                   \
  nsCOMPtr<sbILocalDatabaseAsyncGUIDArrayListener> listener = \
    GetListener();                                            \
  if (listener) {                                             \
    return listener->call;                                    \
  }                                                           \
  return NS_OK;

NS_IMETHODIMP
sbWeakAsyncListenerWrapper::OnGetLength(PRUint32 aLength,
                                        nsresult aResult)
{
  SB_TRY_NOTIFY(OnGetLength(aLength, aResult))
}

NS_IMETHODIMP
sbWeakAsyncListenerWrapper::OnGetGuidByIndex(PRUint32 aIndex,
                                             const nsAString& aGUID,
                                             nsresult aResult)
{
  SB_TRY_NOTIFY(OnGetGuidByIndex(aIndex, aGUID, aResult))
}

NS_IMETHODIMP
sbWeakAsyncListenerWrapper::OnGetSortPropertyValueByIndex(PRUint32 aIndex,
                                                          const nsAString& aPropertySortValue,
                                                          nsresult aResult)
{
  SB_TRY_NOTIFY(OnGetSortPropertyValueByIndex(aIndex, aPropertySortValue, aResult))
}

NS_IMETHODIMP
sbWeakAsyncListenerWrapper::OnGetMediaItemIdByIndex(PRUint32 aIndex,
                                                    PRUint32 aMediaItemId,
                                                    nsresult aResult)
{
  SB_TRY_NOTIFY(OnGetMediaItemIdByIndex(aIndex, aMediaItemId, aResult))
}

NS_IMETHODIMP
sbWeakAsyncListenerWrapper::OnStateChange(PRUint32 aState)
{
  SB_TRY_NOTIFY(OnStateChange(aState))
}

sbLocalDatabaseAsyncGUIDArrayListenerInfo::
                                    sbLocalDatabaseAsyncGUIDArrayListenerInfo()
{
  MOZ_COUNT_CTOR(sbLocalDatabaseAsyncGUIDArrayListenerInfo);
}

sbLocalDatabaseAsyncGUIDArrayListenerInfo::
                                    ~sbLocalDatabaseAsyncGUIDArrayListenerInfo()
{
  MOZ_COUNT_DTOR(sbLocalDatabaseAsyncGUIDArrayListenerInfo);
}

nsresult
sbLocalDatabaseAsyncGUIDArrayListenerInfo::Init(
        nsIProxyObjectManager *aProxyObjMgr,
        nsIWeakReference* aWeakListener)
{
  NS_ENSURE_ARG_POINTER(aProxyObjMgr);
  NS_ASSERTION(aWeakListener, "aWeakListener is null");
  nsresult rv;

  mRef = do_QueryInterface(aWeakListener, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mWeakListenerWrapper = new sbWeakAsyncListenerWrapper(aWeakListener);
  NS_ENSURE_TRUE(mWeakListenerWrapper, NS_ERROR_OUT_OF_MEMORY);

  /* Must not spin the event loop here - this is called with a lock held */
  rv = do_GetProxyForObjectWithManager(aProxyObjMgr,
                                       NS_PROXY_TO_CURRENT_THREAD,
                                       NS_GET_IID(
                                           sbILocalDatabaseAsyncGUIDArrayListener),
                                       mWeakListenerWrapper,
                                       NS_PROXY_ASYNC | NS_PROXY_ALWAYS,
                                       getter_AddRefs(mProxiedListener));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

