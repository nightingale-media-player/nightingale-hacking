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

#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbIMediaListListener.h>

#include <nsIWeakReference.h>
#include <nsIWeakReferenceUtils.h>

#include <nsAutoLock.h>
#include <nsHashKeys.h>
#include <nsThreadUtils.h>
#include <sbProxyUtils.h>

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
: mListenerArrayLock(nsnull)
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
sbLocalDatabaseMediaListListener::AddListener(sbIMediaListListener* aListener,
                                              PRBool aOwnsWeak)
{
  TRACE(("LocalDatabaseMediaListListener[0x%.8x] - AddListener 0x%.8x %d",
         this, aListener, aOwnsWeak));
  NS_ASSERTION(mListenerArrayLock, "You haven't called Init yet!");
  NS_ENSURE_ARG_POINTER(aListener);

  nsresult rv;

  nsAutoLock lock(mListenerArrayLock);

  // See if we have already added this listener.
  PRUint32 length = mListenerArray.Length();
  for (PRUint32 i = 0; i < length; i++) {
    if(mListenerArray[i]->mRef == aListener) {
      // The listener has already been added, so do nothing more. But warn in
      // debug builds.
      NS_WARNING("Attempted to add a listener twice!");
      return NS_OK;
    }
  }

  sbListenerInfoAutoPtr info(new sbListenerInfo());
  NS_ENSURE_TRUE(info, NS_ERROR_OUT_OF_MEMORY);

  if (aOwnsWeak) {
    nsCOMPtr<nsIWeakReference> weak = do_GetWeakReference(aListener, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = info->Init(weak);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = info->Init(aListener);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  sbListenerInfoAutoPtr* added = mListenerArray.AppendElement(info.forget());
  NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);

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

  PRUint32 length = mListenerArray.Length();
  for (PRUint32 i = 0; i < length; i++) {
    if(mListenerArray[i]->mRef == aListener) {
      mListenerArray.RemoveElementAt(i);
      return NS_OK;
    }
  }

  NS_WARNING("Attempted to remove a listener that was never added!");

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
sbLocalDatabaseMediaListListener::SnapshotListenerArray(sbMediaListListenersArray& aArray)
{
  nsAutoLock lock(mListenerArrayLock);

  PRUint32 length = mListenerArray.Length();
  for (PRUint32 i = 0; i < length; i++) {
    PRBool success = aArray.AppendObject(mListenerArray[i]->mProxy);
    NS_ENSURE_SUCCESS(success, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

void
sbLocalDatabaseMediaListListener::SweepListenerArray()
{
  nsAutoLock lock(mListenerArrayLock);

  PRUint32 length = mListenerArray.Length();
  for (PRInt32 i = length - 1; i >= 0; --i) {
    if (mListenerArray[i]->mIsGone) {
      mListenerArray.RemoveElementAt(i);
    }
  }

}

/**
 * \brief Notifies all listeners that an item has been added to the list.
 */
void
sbLocalDatabaseMediaListListener::NotifyListenersItemAdded(sbIMediaList* aList,
                                                           sbIMediaItem* aItem)
{
  NS_ASSERTION(mListenerArrayLock, "You haven't called Init yet!");
  SB_ENSURE_TRUE_VOID(aList);
  SB_ENSURE_TRUE_VOID(aItem);

  sbMediaListListenersArray snapshot(mListenerArray.Length());
  nsresult rv = SnapshotListenerArray(snapshot);
  SB_ENSURE_TRUE_VOID(NS_SUCCEEDED(rv));

  for (PRInt32 i = 0; i < snapshot.Count(); i++) {
    rv = snapshot[i]->OnItemAdded(aList, aItem);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "OnItemAdded returned a failure code");
  }

  SweepListenerArray();
}

/**
 * \brief Notifies all listeners that an item is about to be removed from the
 *        list.
 */
void
sbLocalDatabaseMediaListListener::NotifyListenersBeforeItemRemoved(sbIMediaList* aList,
                                                                   sbIMediaItem* aItem)
{
  NS_ASSERTION(mListenerArrayLock, "You haven't called Init yet!");
  SB_ENSURE_TRUE_VOID(aList);
  SB_ENSURE_TRUE_VOID(aItem);

  sbMediaListListenersArray snapshot(mListenerArray.Length());
  nsresult rv = SnapshotListenerArray(snapshot);
  SB_ENSURE_TRUE_VOID(NS_SUCCEEDED(rv));

  for (PRInt32 i = 0; i < snapshot.Count(); i++) {
    rv = snapshot[i]->OnBeforeItemRemoved(aList, aItem);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "OnBeforeItemRemoved returned a failure code");
  }

  SweepListenerArray();
}

/**
 * \brief Notifies all listeners that an item has been removed from the list.
 */
void
sbLocalDatabaseMediaListListener::NotifyListenersAfterItemRemoved(sbIMediaList* aList,
                                                                  sbIMediaItem* aItem)
{
  NS_ASSERTION(mListenerArrayLock, "You haven't called Init yet!");
  SB_ENSURE_TRUE_VOID(aList);
  SB_ENSURE_TRUE_VOID(aItem);

  sbMediaListListenersArray snapshot(mListenerArray.Length());
  nsresult rv = SnapshotListenerArray(snapshot);
  SB_ENSURE_TRUE_VOID(NS_SUCCEEDED(rv));

  for (PRInt32 i = 0; i < snapshot.Count(); i++) {
    rv = snapshot[i]->OnAfterItemRemoved(aList, aItem);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "OnAfterItemRemoved returned a failure code");
  }

  SweepListenerArray();
}

/**
 * \brief Notifies all listeners that an item has been updated.
 */
void
sbLocalDatabaseMediaListListener::NotifyListenersItemUpdated(sbIMediaList* aList,
                                                             sbIMediaItem* aItem,
                                                             sbIPropertyArray* aProperties)
{
  NS_ASSERTION(mListenerArrayLock, "You haven't called Init yet!");
  SB_ENSURE_TRUE_VOID(aList);
  SB_ENSURE_TRUE_VOID(aItem);
  SB_ENSURE_TRUE_VOID(aProperties);

  sbMediaListListenersArray snapshot(mListenerArray.Length());
  nsresult rv = SnapshotListenerArray(snapshot);
  SB_ENSURE_TRUE_VOID(NS_SUCCEEDED(rv));

#ifdef PR_LOGGING
  nsAutoString list;
  aList->ToString(list);
  nsAutoString item;
  aItem->ToString(item);
  nsAutoString props;
  aProperties->ToString(props);
  TRACE(("LocalDatabaseMediaListListener[0x%.8x] - "
         "NotifyListenersItemUpdated %d %s %s %s", this, snapshot.Count(),
         NS_LossyConvertUTF16toASCII(list).get(),
         NS_LossyConvertUTF16toASCII(item).get(),
         NS_LossyConvertUTF16toASCII(props).get()));
#endif

  for (PRInt32 i = 0; i < snapshot.Count(); i++) {
    rv = snapshot[i]->OnItemUpdated(aList, aItem, aProperties);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "OnItemUpdated returned a failure code");
  }

  SweepListenerArray();
}

/**
 * \brief Notifies all listeners that the list has been cleared.
 */
void
sbLocalDatabaseMediaListListener::NotifyListenersListCleared(sbIMediaList* aList)
{
  NS_ASSERTION(mListenerArrayLock, "You haven't called Init yet!");
  SB_ENSURE_TRUE_VOID(aList);

  sbMediaListListenersArray snapshot(mListenerArray.Length());
  nsresult rv = SnapshotListenerArray(snapshot);
  SB_ENSURE_TRUE_VOID(NS_SUCCEEDED(rv));

  for (PRInt32 i = 0; i < snapshot.Count(); i++) {
    rv = snapshot[i]->OnListCleared(aList);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "OnListCleared returned a failure code");
  }

  SweepListenerArray();
}

/**
 * \brief Notifies all listeners that multiple items are about to be changed.
 */
void
sbLocalDatabaseMediaListListener::NotifyListenersBatchBegin(sbIMediaList* aList)
{
  NS_ASSERTION(mListenerArrayLock, "You haven't called Init yet!");
  SB_ENSURE_TRUE_VOID(aList);

  sbMediaListListenersArray snapshot(mListenerArray.Length());
  nsresult rv = SnapshotListenerArray(snapshot);
  SB_ENSURE_TRUE_VOID(NS_SUCCEEDED(rv));

  for (PRInt32 i = 0; i < snapshot.Count(); i++) {
    rv = snapshot[i]->OnBatchBegin(aList);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "OnBatchBegin returned a failure code");
  }

  SweepListenerArray();
}

/**
 * \brief Notifies all listeners that multiple items have been changed.
 */
void
sbLocalDatabaseMediaListListener::NotifyListenersBatchEnd(sbIMediaList* aList)
{
  NS_ASSERTION(mListenerArrayLock, "You haven't called Init yet!");
  SB_ENSURE_TRUE_VOID(aList);

  sbMediaListListenersArray snapshot(mListenerArray.Length());
  nsresult rv = SnapshotListenerArray(snapshot);
  SB_ENSURE_TRUE_VOID(NS_SUCCEEDED(rv));

  for (PRInt32 i = 0; i < snapshot.Count(); i++) {
    rv = snapshot[i]->OnBatchEnd(aList);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "OnBatchEnd returned a failure code");
  }

  SweepListenerArray();
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
sbListenerInfo::Init(sbIMediaListListener* aListener)
{
  NS_ASSERTION(aListener, "aListener is null");
  NS_ASSERTION(!mProxy, "Init called twice");
  nsresult rv;

  mRef = aListener;

  rv = SB_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                            NS_GET_IID(sbIMediaListListener),
                            aListener,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(mProxy));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbListenerInfo::Init(nsIWeakReference* aWeakListener)
{
  NS_ASSERTION(aWeakListener, "aWeakListener is null");
  NS_ASSERTION(!mProxy, "Init called twice");
  nsresult rv;

  mRef = aWeakListener;

  nsCOMPtr<sbIMediaListListener> wrapped =
    new sbWeakMediaListListenerWrapper(this);
  NS_ENSURE_TRUE(wrapped, NS_ERROR_OUT_OF_MEMORY);

  rv = SB_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                            NS_GET_IID(sbIMediaListListener),
                            wrapped,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(mProxy));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(sbWeakMediaListListenerWrapper,
                              sbIMediaListListener)

sbWeakMediaListListenerWrapper::sbWeakMediaListListenerWrapper(sbListenerInfo* aListenerInfo) :
  mListenerInfo(aListenerInfo)
{
  NS_ASSERTION(mListenerInfo, "aListenerInfo is null");
  MOZ_COUNT_CTOR(sbWeakMediaListListenerWrapper);
}

sbWeakMediaListListenerWrapper::~sbWeakMediaListListenerWrapper()
{
  MOZ_COUNT_DTOR(sbWeakMediaListListenerWrapper);
}

already_AddRefed<sbIMediaListListener>
sbWeakMediaListListenerWrapper::GetListener()
{
  nsresult rv;
  nsCOMPtr<nsIWeakReference> weak =
    do_QueryInterface(mListenerInfo->mRef, &rv);
  if (NS_FAILED(rv)) {
    NS_ERROR("Could no QI mRef to nsIWeakReference");
    mListenerInfo->mIsGone = PR_TRUE;
    return nsnull;
  }

  nsCOMPtr<sbIMediaListListener> strongListener = do_QueryReferent(weak);
  if (!strongListener) {
    TRACE(("sbWeakMediaListListenerWrapper[0x%.8x] - Weak listener is gone",
           this));
    mListenerInfo->mIsGone = PR_TRUE;
    return nsnull;
  }

  sbIMediaListListener* listenerPtr = strongListener;
  NS_ADDREF(listenerPtr);
  return listenerPtr;
}

#define SB_TRY_NOTIFY(call)                                \
  nsCOMPtr<sbIMediaListListener> listener = GetListener(); \
  if (listener) {                                          \
    return listener->call;                                 \
  }                                                        \
  return NS_OK;

NS_IMETHODIMP
sbWeakMediaListListenerWrapper::OnItemAdded(sbIMediaList* aMediaList,
                                            sbIMediaItem* aMediaItem)
{
  SB_TRY_NOTIFY(OnItemAdded(aMediaList, aMediaItem))
}

NS_IMETHODIMP
sbWeakMediaListListenerWrapper::OnBeforeItemRemoved(sbIMediaList* aMediaList,
                                                    sbIMediaItem* aMediaItem)
{
  SB_TRY_NOTIFY(OnBeforeItemRemoved(aMediaList, aMediaItem))
}

NS_IMETHODIMP
sbWeakMediaListListenerWrapper::OnAfterItemRemoved(sbIMediaList* aMediaList,
                                                   sbIMediaItem* aMediaItem)
{
  SB_TRY_NOTIFY(OnAfterItemRemoved(aMediaList, aMediaItem))
}

NS_IMETHODIMP
sbWeakMediaListListenerWrapper::OnItemUpdated(sbIMediaList* aMediaList,
                                              sbIMediaItem* aMediaItem,
                                              sbIPropertyArray* aProperties)
{
  SB_TRY_NOTIFY(OnItemUpdated(aMediaList, aMediaItem, aProperties))
}

NS_IMETHODIMP
sbWeakMediaListListenerWrapper::OnListCleared(sbIMediaList* aMediaList)
{
  SB_TRY_NOTIFY(OnListCleared(aMediaList))
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

