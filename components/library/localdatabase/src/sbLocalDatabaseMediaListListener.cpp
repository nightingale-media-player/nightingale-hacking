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

#include "sbLocalDatabaseMediaListListener.h"
#include "sbLocalDatabaseMediaListBase.h"

#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbIMediaListListener.h>

#include <nsIWeakReference.h>
#include <nsIWeakReferenceUtils.h>

#include <nsAutoLock.h>
#include <nsHashKeys.h>
#include <nsThreadUtils.h>
#include <sbProxyUtils.h>

#ifdef DEBUG
#include <nsIXPConnect.h>
#endif
#ifdef PR_LOGGING
#include <prprf.h>
#endif

#define SB_ENSURE_TRUE_VOID(_arg) \
  NS_ENSURE_TRUE((_arg), /* void */)

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbLocalDatabaseMediaListListener:5
 */
#ifdef PR_LOGGING

static PRLogModuleInfo* gMediaListListenerLog = nsnull;
#define TRACE(args) if (gMediaListListenerLog) PR_LOG(gMediaListListenerLog, PR_LOG_DEBUG, args)
#define LOG(args)   if (gMediaListListenerLog) PR_LOG(gMediaListListenerLog, PR_LOG_WARN, args)

#else /* PR_LOGGING */

#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */

#endif /* PR_LOGGING */

sbLocalDatabaseMediaListListener::sbLocalDatabaseMediaListListener()
: mListenerArrayLock(nsnull),
  mBatchDepth(0)
{
  MOZ_COUNT_CTOR(sbLocalDatabaseMediaListListener);
#ifdef PR_LOGGING
  if (!gMediaListListenerLog) {
    gMediaListListenerLog = PR_NewLogModule("sbLocalDatabaseMediaListListener");
  }
#endif
  TRACE(("LocalDatabaseMediaListListener[0x%.8x] - Constructed", this));
}

sbLocalDatabaseMediaListListener::~sbLocalDatabaseMediaListListener()
{
  if (mListenerArrayLock) {
    nsAutoLock::DestroyLock(mListenerArrayLock);
  }
  TRACE(("LocalDatabaseMediaListListener[0x%.8x] - Destructed", this));
  MOZ_COUNT_DTOR(sbLocalDatabaseMediaListListener);
}

nsresult
sbLocalDatabaseMediaListListener::Init()
{
  NS_WARN_IF_FALSE(!mListenerArrayLock, "You're calling Init more than once!");

  // Initialize our lock.
  if (!mListenerArrayLock) {
    mListenerArrayLock =
      nsAutoLock::NewLock("sbLocalDatabaseMediaListListener::"
                          "mListenerArrayLock");
    NS_ENSURE_TRUE(mListenerArrayLock, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListListener::AddListener(sbLocalDatabaseMediaListBase* aList,
                                              sbIMediaListListener* aListener,
                                              PRBool aOwnsWeak,
                                              PRUint32 aFlags,
                                              sbIPropertyArray* aPropertyFilter)
{
#ifdef DEBUG
  nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID());
  if(xpc) {
    nsCAutoString objtype;
    nsCOMPtr<nsIXPConnectWrappedJS> wrapped = do_QueryInterface(aListener);
    if (wrapped) {
      objtype.AssignLiteral("wrapped");
    }
    else {
      objtype.AssignLiteral("native");
    }

    nsCAutoString stack;
    stack.AssignLiteral("no js stack");

    nsCOMPtr<nsIStackFrame> current;
    xpc->GetCurrentJSStack(getter_AddRefs(current));
    if (current) {
      current->ToString(getter_Copies(stack));
    }
    TRACE(("LocalDatabaseMediaListListener[0x%.8x] - "
           "AddListener 0x%.8x %d %s %s",
           this, aListener, aOwnsWeak, objtype.get(), stack.get()));
  }
#endif

  NS_ASSERTION(mListenerArrayLock, "You haven't called Init yet!");
  NS_ENSURE_ARG_POINTER(aListener);

  nsresult rv;

  // When aFlags is 0, this means that it was unspecified by the caller and
  // should get the default value
  if (aFlags == 0) {
    aFlags = sbIMediaList::LISTENER_FLAGS_ALL;
  }

  nsAutoLock lock(mListenerArrayLock);

  // See if we have already added this listener.
  PRUint32 length = mListenerArray.Length();
  nsCOMPtr<nsISupports> ref = do_QueryInterface(aListener, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aOwnsWeak) {
    nsCOMPtr<nsIWeakReference> weak = do_GetWeakReference(aListener, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    for (PRUint32 i = 0; i < length; i++) {
      if(mListenerArray[i]->mRef == weak) {
        NS_WARNING("Attempted to add a weak listener twice!");
        return NS_OK;
      }
    }
  }
  else {
    for (PRUint32 i = 0; i < length; i++) {
      if(mListenerArray[i]->mRef == ref) {
        NS_WARNING("Attempted to add a strong listener twice!");
        return NS_OK;
      }
    }
  }

  sbListenerInfoAutoPtr info(new sbListenerInfo());
  NS_ENSURE_TRUE(info, NS_ERROR_OUT_OF_MEMORY);

  if (aOwnsWeak) {
    nsCOMPtr<nsIWeakReference> weak = do_GetWeakReference(aListener, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = info->Init(weak, mBatchDepth, aFlags, aPropertyFilter);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = info->Init(aListener, mBatchDepth, aFlags, aPropertyFilter);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  sbListenerInfoAutoPtr* added = mListenerArray.AppendElement(info.forget());
  NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);

  // If this list is in a batch, the newly added listener should be notified
  // immediately of this fact
  if (mBatchDepth > 0) {
    nsCOMPtr<sbIMediaList> list =
      do_QueryInterface(NS_ISUPPORTS_CAST(sbIMediaList*, aList), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 i = 0; i < mBatchDepth; i++) {
      rv = aListener->OnBatchBegin(aList);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "OnBatchBegin returned a failure code");
    }
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListListener::RemoveListener(sbIMediaListListener* aListener)
{
  TRACE(("LocalDatabaseMediaListListener[0x%.8x] - RemoveListener 0x%.8x",
         this, aListener));
  NS_ASSERTION(mListenerArrayLock, "You haven't called Init yet!");
  NS_ENSURE_ARG_POINTER(aListener);

  nsAutoLock lock(mListenerArrayLock);

  nsresult rv;
  PRUint32 length = mListenerArray.Length();

  nsCOMPtr<nsISupports> ref = do_QueryInterface(aListener, &rv);
  for (PRUint32 i = 0; i < length; i++) {
    if(mListenerArray[i]->mRef == ref) {
      mListenerArray.RemoveElementAt(i);
      return NS_OK;
    }
  }

  nsCOMPtr<nsIWeakReference> weak = do_GetWeakReference(aListener, &rv);
  if (NS_SUCCEEDED(rv)) {
    for (PRUint32 i = 0; i < length; i++) {
      if(mListenerArray[i]->mRef == weak) {
        mListenerArray.RemoveElementAt(i);
        return NS_OK;
      }
    }
    NS_WARNING("Attempted to remove a weak listener that was never added!");
  }
  else {
    NS_WARNING("Attempted to remove a strong listener that was never added!");
  }

  return NS_OK;
}

PRUint32
sbLocalDatabaseMediaListListener::ListenerCount()
{
  NS_ASSERTION(mListenerArrayLock, "You haven't called Init yet!");

  nsAutoLock lock(mListenerArrayLock);
  return mListenerArray.Length();
}

nsresult
sbLocalDatabaseMediaListListener::SnapshotListenerArray(sbMediaListListenersArray& aArray,
                                                        PRUint32 aFlags,
                                                        sbIPropertyArray* aProperties)
{
  nsAutoLock lock(mListenerArrayLock);

  PRUint32 length = mListenerArray.Length();
  for (PRUint32 i = 0; i < length; i++) {
    if (mListenerArray[i]->ShouldNotify(aFlags, aProperties)) {
      nsString debugAddress;
      mListenerArray[i]->GetDebugAddress(debugAddress);
      ListenerAndDebugAddress* added =
        aArray.AppendElement(ListenerAndDebugAddress(mListenerArray[i]->mProxy,
                                                     debugAddress));
      NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
    }
  }

  return NS_OK;
}

void
sbLocalDatabaseMediaListListener::SweepListenerArray(sbStopNotifyArray& aStopNotifying)
{
  nsAutoLock lock(mListenerArrayLock);

  PRUint32 numStopNotifying = aStopNotifying.Length();
  PRUint32 numListeners     = mListenerArray.Length();

  // The aStopNotifying array tells us what listeners are "gone" (weak
  // listeners that have gone away) as well as which listeners should no
  // longer be notified in the current batch.  Match up the listeners in this
  // array with the main listener array by comparing the listener comptr

  for (PRUint32 i = 0; i  < numStopNotifying; i++) {
    const StopNotifyFlags& stop = aStopNotifying[i];
    for (PRUint32 j = 0; j < numListeners; j++) {
      if (stop.listener == mListenerArray[j]->mProxy) {
        if (stop.isGone) {
          mListenerArray.RemoveElementAt(j);
        }
        else {
          if (stop.listenerFlags > 0) {
            mListenerArray[j]->SetShouldStopNotifying(stop.listenerFlags);
          }
        }
      }
    }
  }

}

#ifdef PR_LOGGING

#define SB_NOTIFY_LOG_COUNT(method)                                       \
  nsString __notifiedList;                                                \
  TRACE(("LocalDatabaseMediaListListener[0x%.8x] - " #method " %d of %d", \
         this, snapshot.Length(), mListenerArray.Length()));

#define SB_NOTIFY_LOG_REMEMBER_NOTIFIED                                   \
  PR_BEGIN_MACRO                                                          \
    __notifiedList.AppendLiteral(" ");                                    \
    __notifiedList.Append(snapshot[i].debugAddress);                      \
    __notifiedList.AppendLiteral(" (");                                   \
    __notifiedList.AppendInt(__delta);                                    \
    __notifiedList.AppendLiteral(")");                                    \
  PR_END_MACRO

#define SB_NOTIFY_LOG_NOTIFIED                                            \
  TRACE(("LocalDatabaseMediaListListener[0x%.8x] - Notified%s",           \
         this, NS_LossyConvertUTF16toASCII(__notifiedList).get()));

#define SB_NOTIFY_LOG_START_TIMER                                         \
  PRTime __now = PR_Now();

#define SB_NOTIFY_LOG_STOP_TIMER                                          \
  PRUint32 __delta = PR_Now() - __now;
#else

#define SB_NOTIFY_LOG_COUNT(method)
#define SB_NOTIFY_LOG_REMEMBER_NOTIFIED
#define SB_NOTIFY_LOG_NOTIFIED
#define SB_NOTIFY_LOG_START_TIMER
#define SB_NOTIFY_LOG_STOP_TIMER

#endif

#define SB_NOTIFY_LISTENERS_HEAD                                          \
  NS_ASSERTION(mListenerArrayLock, "You haven't called Init yet!");       \
                                                                          \
  sbMediaListListenersArray snapshot;

#define SB_NOTIFY_LISTENERS_TAIL(method, call, flag)                      \
  SB_ENSURE_TRUE_VOID(NS_SUCCEEDED(rv));                                  \
                                                                          \
  PRUint32 length = snapshot.Length();                                    \
  sbStopNotifyArray stopNotifying(length);                                \
                                                                          \
  SB_NOTIFY_LOG_COUNT(method)                                             \
                                                                          \
  for (PRUint32 i = 0; i < length; i++) {                                 \
    PRBool noMoreForBatch = PR_FALSE;                                     \
    SB_NOTIFY_LOG_START_TIMER                                             \
    rv = snapshot[i].listener->call;                                      \
    SB_NOTIFY_LOG_STOP_TIMER                                              \
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), #call " returned a failure code"); \
    PRBool isGone = rv == NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;          \
    PRUint32 listenerFlags = 0;                                           \
    if (noMoreForBatch) {                                                 \
      listenerFlags = sbIMediaList::flag;                                 \
    }                                                                     \
    StopNotifyFlags* added =                                              \
      stopNotifying.AppendElement(StopNotifyFlags(snapshot[i].listener,   \
                                                  listenerFlags,          \
                                                  isGone));               \
    SB_ENSURE_TRUE_VOID(added);                                           \
    SB_NOTIFY_LOG_REMEMBER_NOTIFIED;                                      \
  }                                                                       \
                                                                          \
  SB_NOTIFY_LOG_NOTIFIED                                                  \
  SweepListenerArray(stopNotifying);

#define SB_NOTIFY_LISTENERS(method, call, flag)                           \
  SB_NOTIFY_LISTENERS_HEAD                                                \
  nsresult rv = SnapshotListenerArray(snapshot, sbIMediaList::flag);      \
  SB_NOTIFY_LISTENERS_TAIL(method, call, flag)

#define SB_NOTIFY_LISTENERS_PROPERTIES(method, call, flag)                \
  SB_NOTIFY_LISTENERS_HEAD                                                \
  nsresult rv = SnapshotListenerArray(snapshot,                           \
                                      sbIMediaList::flag,                 \
                                      aProperties);                       \
  SB_NOTIFY_LISTENERS_TAIL(method, call, flag)

/**
 * \brief Notifies all listeners that an item has been added to the list.
 */
void
sbLocalDatabaseMediaListListener::NotifyListenersItemAdded(sbIMediaList* aList,
                                                           sbIMediaItem* aItem)
{
  SB_ENSURE_TRUE_VOID(aList);
  SB_ENSURE_TRUE_VOID(aItem);

  SB_NOTIFY_LISTENERS(NotifyListenersItemAdded,
                      OnItemAdded(aList, aItem, &noMoreForBatch),
                      LISTENER_FLAGS_ITEMADDED);
}

/**
 * \brief Notifies all listeners that an item is about to be removed from the
 *        list.
 */
void
sbLocalDatabaseMediaListListener::NotifyListenersBeforeItemRemoved(sbIMediaList* aList,
                                                                   sbIMediaItem* aItem)
{
  SB_ENSURE_TRUE_VOID(aList);
  SB_ENSURE_TRUE_VOID(aItem);

  SB_NOTIFY_LISTENERS(NotifyListenersBeforeItemRemoved,
                      OnBeforeItemRemoved(aList, aItem, &noMoreForBatch),
                      LISTENER_FLAGS_BEFOREITEMREMOVED);
}

/**
 * \brief Notifies all listeners that an item has been removed from the list.
 */
void
sbLocalDatabaseMediaListListener::NotifyListenersAfterItemRemoved(sbIMediaList* aList,
                                                                  sbIMediaItem* aItem)
{
  SB_ENSURE_TRUE_VOID(aList);
  SB_ENSURE_TRUE_VOID(aItem);

  SB_NOTIFY_LISTENERS(NotifyListenersAfterItemRemoved,
                      OnAfterItemRemoved(aList, aItem, &noMoreForBatch),
                      LISTENER_FLAGS_AFTERITEMREMOVED);
}

/**
 * \brief Notifies all listeners that an item has been updated.
 */
void
sbLocalDatabaseMediaListListener::NotifyListenersItemUpdated(sbIMediaList* aList,
                                                             sbIMediaItem* aItem,
                                                             sbIPropertyArray* aProperties)
{
  SB_ENSURE_TRUE_VOID(aList);
  SB_ENSURE_TRUE_VOID(aItem);
  SB_ENSURE_TRUE_VOID(aProperties);

#if 0
  nsAutoString list;
  aList->ToString(list);
  nsAutoString item;
  aItem->ToString(item);
  nsAutoString props;
  aProperties->ToString(props);
  TRACE(("LocalDatabaseMediaListListener[0x%.8x] - "
         "NotifyListenersItemUpdated %s %s %s", this,
         NS_LossyConvertUTF16toASCII(list).get(),
         NS_LossyConvertUTF16toASCII(item).get(),
         NS_LossyConvertUTF16toASCII(props).get()));
#endif

  SB_NOTIFY_LISTENERS_PROPERTIES(NotifyListenersItemUpdated,
                                 OnItemUpdated(aList, aItem, aProperties, &noMoreForBatch),
                                 LISTENER_FLAGS_ITEMUPDATED);
}

/**
 * \brief Notifies all listeners that the list has been cleared.
 */
void
sbLocalDatabaseMediaListListener::NotifyListenersListCleared(sbIMediaList* aList)
{
  SB_ENSURE_TRUE_VOID(aList);

  SB_NOTIFY_LISTENERS(NotifyListenersListCleared,
                      OnListCleared(aList, &noMoreForBatch),
                      LISTENER_FLAGS_LISTCLEARED);
}

/**
 * \brief Notifies all listeners that multiple items are about to be changed.
 */
void
sbLocalDatabaseMediaListListener::NotifyListenersBatchBegin(sbIMediaList* aList)
{
  SB_ENSURE_TRUE_VOID(aList);

  mBatchDepth++;

  // Tell all of our listener infos that we have started a batch
  {
    nsAutoLock lock(mListenerArrayLock);
    PRUint32 length = mListenerArray.Length();
    for (PRUint32 i = 0; i < length; i++) {
      mListenerArray[i]->BeginBatch();
    }
  }

  SB_NOTIFY_LISTENERS(NotifyListenersBatchBegin,
                      OnBatchBegin(aList),
                      LISTENER_FLAGS_BATCHBEGIN);
}

/**
 * \brief Notifies all listeners that multiple items have been changed.
 */
void
sbLocalDatabaseMediaListListener::NotifyListenersBatchEnd(sbIMediaList* aList)
{
  SB_ENSURE_TRUE_VOID(aList);

  if (mBatchDepth == 0) {
    NS_ERROR("Batch begin/end imbalance");
    return;
  }

  mBatchDepth--;

  // Tell all of our listener infos that we have ended a batch
  {
    nsAutoLock lock(mListenerArrayLock);
    PRUint32 length = mListenerArray.Length();
    for (PRUint32 i = 0; i < length; i++) {
      mListenerArray[i]->EndBatch();
    }
  }

  SB_NOTIFY_LISTENERS(NotifyListenersBatchEnd,
                      OnBatchEnd(aList),
                      LISTENER_FLAGS_BATCHEND);
}

sbListenerInfo::sbListenerInfo() :
  mIsGone(PR_FALSE)
{
  MOZ_COUNT_CTOR(sbListenerInfo);
}

sbListenerInfo::~sbListenerInfo()
{
  MOZ_COUNT_DTOR(sbListenerInfo);
}

nsresult
sbListenerInfo::Init(sbIMediaListListener* aListener,
                     PRUint32 aCurrentBatchDepth,
                     PRUint32 aFlags,
                     sbIPropertyArray* aPropertyFilter)
{
  NS_ASSERTION(aListener, "aListener is null");
  NS_ASSERTION(!mProxy, "Init called twice");
  nsresult rv;

  mRef = do_QueryInterface(aListener, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mFlags = aFlags;

  PRBool success = mStopNotifiyingStack.SetLength(aCurrentBatchDepth);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  for (PRUint32 i = 0; i < aCurrentBatchDepth; i++) {
    mStopNotifiyingStack[i] = 0;
  }

  InitPropertyFilter(aPropertyFilter);

  rv = SB_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                            NS_GET_IID(sbIMediaListListener),
                            aListener,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(mProxy));
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef PR_LOGGING
  char buf[256];
  PRUint32 len = PR_snprintf(buf, sizeof(buf), "0x%.8x", aListener);
  mDebugAddress.Assign(NS_ConvertASCIItoUTF16(buf, len));
#endif
  return NS_OK;
}

nsresult
sbListenerInfo::Init(nsIWeakReference* aWeakListener,
                     PRUint32 aCurrentBatchDepth,
                     PRUint32 aFlags,
                     sbIPropertyArray* aPropertyFilter)
{
  NS_ASSERTION(aWeakListener, "aWeakListener is null");
  NS_ASSERTION(!mProxy, "Init called twice");
  nsresult rv;

  mRef = do_QueryInterface(aWeakListener, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mWeak  = aWeakListener;
  mFlags = aFlags;

  PRBool success = mStopNotifiyingStack.SetLength(aCurrentBatchDepth);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  for (PRUint32 i = 0; i < aCurrentBatchDepth; i++) {
    mStopNotifiyingStack[i] = 0;
  }

  InitPropertyFilter(aPropertyFilter);

  nsCOMPtr<sbIMediaListListener> wrapped =
    new sbWeakMediaListListenerWrapper(mWeak);
  NS_ENSURE_TRUE(wrapped, NS_ERROR_OUT_OF_MEMORY);

  rv = SB_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                            NS_GET_IID(sbIMediaListListener),
                            wrapped,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(mProxy));
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef PR_LOGGING
  nsCOMPtr<sbIMediaListListener> listener = do_QueryReferent(aWeakListener);
  char buf[256];
  PRUint32 len = PR_snprintf(buf, sizeof(buf), "0x%.8x",
                             listener ? listener.get() : 0);
  mDebugAddress.Assign(NS_ConvertASCIItoUTF16(buf, len));
#endif

  return NS_OK;
}

void
sbListenerInfo::GetDebugAddress(nsAString& aDebugAddress)
{
  aDebugAddress = mDebugAddress;
}

PRBool
sbListenerInfo::ShouldNotify(PRUint32 aFlag, sbIPropertyArray* aProperties)
{
  // Check the flags to see if we should be notifying
  if ((mFlags & aFlag) == 0) {
    return PR_FALSE;
  }

  // Check if we are set to stop notifying for this batch
  if (mStopNotifiyingStack.Length() > 0 && mStopNotifiyingStack[0] & aFlag) {
    return PR_FALSE;
  }

  // If there are properties, check them
  if (mHasPropertyFilter && aProperties) {
    nsresult rv;

    PRUint32 length;
    rv = aProperties->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    for (PRUint32 i = 0; i < length; i++) {
      nsCOMPtr<sbIProperty> property;
      rv = aProperties->GetPropertyAt(i, getter_AddRefs(property));
      NS_ENSURE_SUCCESS(rv, PR_FALSE);

      nsString id;
      rv = property->GetId(id);
      NS_ENSURE_SUCCESS(rv, PR_FALSE);

      if (mPropertyFilter.GetEntry(id)) {
        // Found, we should notify
        return PR_TRUE;
      }
    }

    // Not found, don't notify
    return PR_FALSE;
  }

  // If we get here, this means we should notify
  return PR_TRUE;
}

void
sbListenerInfo::BeginBatch()
{
  PRUint32* success = mStopNotifiyingStack.InsertElementAt(0, 0);
  SB_ENSURE_TRUE_VOID(success);
}

void
sbListenerInfo::EndBatch()
{
  if (mStopNotifiyingStack.Length() == 0) {
    NS_ERROR("BeginBatch/EndBatch out of balance");
    return;
  }

  mStopNotifiyingStack.RemoveElementAt(0);
}

void
sbListenerInfo::SetShouldStopNotifying(PRUint32 aFlag)
{
  // If there is no batch, just ignore this
  if (mStopNotifiyingStack.Length() == 0) {
    return;
  }

  mStopNotifiyingStack[0] |= aFlag;
}

nsresult
sbListenerInfo::InitPropertyFilter(sbIPropertyArray* aPropertyFilter)
{
  nsresult rv;

  if (aPropertyFilter) {
    mHasPropertyFilter = PR_TRUE;

    PRUint32 length = 0;
    rv = aPropertyFilter->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool success = mPropertyFilter.Init(length);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    for (PRUint32 i = 0; i < length; i++) {
      nsCOMPtr<sbIProperty> property;
      rv = aPropertyFilter->GetPropertyAt(i, getter_AddRefs(property));
      NS_ENSURE_SUCCESS(rv, rv);

      nsString id;
      rv = property->GetId(id);
      NS_ENSURE_SUCCESS(rv, rv);

      nsStringHashKey* added = mPropertyFilter.PutEntry(id);
      NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
    }
  }
  else {
    mHasPropertyFilter = PR_FALSE;
  }

  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(sbWeakMediaListListenerWrapper,
                              sbIMediaListListener)

sbWeakMediaListListenerWrapper::sbWeakMediaListListenerWrapper(nsIWeakReference* aWeakListener) :
  mWrappedWeak(aWeakListener)
{
  NS_ASSERTION(mWrappedWeak, "aWeakListener is null");
  MOZ_COUNT_CTOR(sbWeakMediaListListenerWrapper);
}

sbWeakMediaListListenerWrapper::~sbWeakMediaListListenerWrapper()
{
  MOZ_COUNT_DTOR(sbWeakMediaListListenerWrapper);
}

already_AddRefed<sbIMediaListListener>
sbWeakMediaListListenerWrapper::GetListener()
{
  nsCOMPtr<sbIMediaListListener> strongListener = do_QueryReferent(mWrappedWeak); 
  if (!strongListener) {
    LOG(("sbWeakMediaListListenerWrapper[0x%.8x] - Weak listener is gone",
         this));
    return nsnull;
  }

  sbIMediaListListener* listenerPtr = strongListener;
  NS_ADDREF(listenerPtr);
  return listenerPtr;
}

// Try to notify the listener if it still exists.  If it no longer exists,
// return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA as a sentinel to report this
// situation (see SB_NOTIFY_LISTENERS_TAIL)
#define SB_TRY_NOTIFY(call)                                \
  nsCOMPtr<sbIMediaListListener> listener = GetListener(); \
  if (listener) {                                          \
    return listener->call;                                 \
  }                                                        \
  return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;

NS_IMETHODIMP
sbWeakMediaListListenerWrapper::OnItemAdded(sbIMediaList* aMediaList,
                                            sbIMediaItem* aMediaItem,
                                            PRBool* aNoMoreForBatch)
{
  SB_TRY_NOTIFY(OnItemAdded(aMediaList, aMediaItem, aNoMoreForBatch))
}

NS_IMETHODIMP
sbWeakMediaListListenerWrapper::OnBeforeItemRemoved(sbIMediaList* aMediaList,
                                                    sbIMediaItem* aMediaItem,
                                                    PRBool* aNoMoreForBatch)
{
  SB_TRY_NOTIFY(OnBeforeItemRemoved(aMediaList, aMediaItem, aNoMoreForBatch))
}

NS_IMETHODIMP
sbWeakMediaListListenerWrapper::OnAfterItemRemoved(sbIMediaList* aMediaList,
                                                   sbIMediaItem* aMediaItem,
                                                   PRBool* aNoMoreForBatch)
{
  SB_TRY_NOTIFY(OnAfterItemRemoved(aMediaList, aMediaItem, aNoMoreForBatch))
}

NS_IMETHODIMP
sbWeakMediaListListenerWrapper::OnItemUpdated(sbIMediaList* aMediaList,
                                              sbIMediaItem* aMediaItem,
                                              sbIPropertyArray* aProperties,
                                              PRBool* aNoMoreForBatch)
{
  SB_TRY_NOTIFY(OnItemUpdated(aMediaList,
                              aMediaItem,
                              aProperties,
                              aNoMoreForBatch))
}

NS_IMETHODIMP
sbWeakMediaListListenerWrapper::OnListCleared(sbIMediaList* aMediaList,
                                              PRBool* aNoMoreForBatch)
{
  SB_TRY_NOTIFY(OnListCleared(aMediaList, aNoMoreForBatch))
}

NS_IMETHODIMP
sbWeakMediaListListenerWrapper::OnBatchBegin(sbIMediaList* aMediaList)
{
  SB_TRY_NOTIFY(OnBatchBegin(aMediaList))
}

NS_IMETHODIMP
sbWeakMediaListListenerWrapper::OnBatchEnd(sbIMediaList* aMediaList)
{
  SB_TRY_NOTIFY(OnBatchEnd(aMediaList))
}
