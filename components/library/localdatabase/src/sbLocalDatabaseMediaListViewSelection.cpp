/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbLocalDatabaseMediaListViewSelection.h"
#include "sbLocalDatabaseMediaItem.h"

#include <sbStandardProperties.h>
#include <sbStringUtils.h>

#include <nsComponentManagerUtils.h>
#include <nsIClassInfoImpl.h>
#include <nsIObjectInputStream.h>
#include <nsIObjectOutputStream.h>
#include <nsIProgrammingLanguage.h>
#include <nsMemory.h>

// Uncomment this to log the selection list whenever it changes.  This is
// normally ifdef'd out because it can be really slow
//#define LOG_SELECTION

NS_IMPL_ISUPPORTS1(sbLocalDatabaseMediaListViewSelection,
                   sbIMediaListViewSelection)

#define NOTIFY_LISTENERS_INDIRECT(_obj, _method, _params)     \
  PR_BEGIN_MACRO                                              \
  if (!_obj->mSelectionNotificationsSuppressed) {             \
    sbObserverArray::ForwardIterator iter(_obj->mObservers);  \
    while (iter.HasMore()) {                                  \
      iter.GetNext()->_method _params;                        \
    }                                                         \
  }                                                           \
  PR_END_MACRO
#define NOTIFY_LISTENERS(_method, _params)                    \
  NOTIFY_LISTENERS_INDIRECT(this, _method, _params)

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbLocalDatabaseMediaListViewSelection:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gLocalDatabaseMediaListViewSelectionLog = nsnull;
#define TRACE(args) PR_LOG(gLocalDatabaseMediaListViewSelectionLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gLocalDatabaseMediaListViewSelectionLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

sbLocalDatabaseMediaListViewSelection::sbLocalDatabaseMediaListViewSelection()
  : mSelectionIsAll(PR_FALSE),
    mCurrentIndex(-1),
    mArray(nsnull),
    mIsLibrary(PR_FALSE),
    mLength(0),
    mSelectionNotificationsSuppressed(PR_FALSE)
{
#ifdef PR_LOGGING
  if (!gLocalDatabaseMediaListViewSelectionLog) {
    gLocalDatabaseMediaListViewSelectionLog =
      PR_NewLogModule("sbLocalDatabaseMediaListViewSelection");
  }
#endif
}

nsresult
sbLocalDatabaseMediaListViewSelection::Init(sbILibrary* aLibrary,
                                            const nsAString& aListGUID,
                                            sbILocalDatabaseGUIDArray* aArray,
                                            PRBool aIsLibrary,
                                            sbLocalDatabaseMediaListViewSelectionState* aState)
{
  NS_ASSERTION(aArray, "aArray is null");

  nsresult rv;

  mLibrary = aLibrary;
  mListGUID = aListGUID;
  mArray = aArray;
  mIsLibrary = aIsLibrary;

  PRBool success = mSelection.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  if (aState) {
    mCurrentIndex     = aState->mCurrentIndex;
    rv = GetUniqueIdForIndex(mCurrentIndex, mCurrentUID);
    NS_ENSURE_SUCCESS(rv, rv);
    mSelectionIsAll   = aState->mSelectionIsAll;

    if (!mSelectionIsAll) {
      aState->mSelectionList.EnumerateRead(SB_CopySelectionListCallback,
                                           &mSelection);
    }
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListViewSelection::ConfigurationChanged()
{
  nsresult rv = mArray->GetLength(&mLength);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the new current index from the current unique ID.
  if (!mCurrentUID.IsEmpty()) {
    PRUint32 index;
    rv = GetIndexForUniqueId(mCurrentUID, &index);
    if (NS_SUCCEEDED(rv)) {
      mCurrentIndex = index;
    } else if (rv == NS_ERROR_NOT_AVAILABLE) {
      mCurrentIndex = -1;
    } else {
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } else {
    mCurrentIndex = -1;
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListViewSelection::GetState(sbLocalDatabaseMediaListViewSelectionState** aState)
{
  NS_ENSURE_ARG_POINTER(aState);

  nsRefPtr<sbLocalDatabaseMediaListViewSelectionState> state =
    new sbLocalDatabaseMediaListViewSelectionState();
  NS_ENSURE_TRUE(state, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = state->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  state->mCurrentIndex     = mCurrentIndex;
  state->mSelectionIsAll   = mSelectionIsAll;

  if (!mSelectionIsAll) {
    mSelection.EnumerateRead(SB_CopySelectionListCallback,
                             &state->mSelectionList);
  }

  NS_ADDREF(*aState = state);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::GetCount(PRInt32* aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);

  if (mSelectionIsAll) {
    *aCount = (PRInt32) mLength;
  }
  else {
    *aCount = (PRInt32) mSelection.Count();
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::GetCurrentIndex(PRInt32* aCurrentIndex)
{
  NS_ENSURE_ARG_POINTER(aCurrentIndex);
  *aCurrentIndex = mCurrentIndex;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::SetCurrentIndex(PRInt32 aCurrentIndex)
{
  NS_ENSURE_ARG_RANGE(aCurrentIndex, -1, (PRInt32) mLength - 1);
  nsresult rv;
  mCurrentIndex = aCurrentIndex;
  rv = GetUniqueIdForIndex(mCurrentIndex, mCurrentUID);
  NS_ENSURE_SUCCESS(rv, rv);
  NOTIFY_LISTENERS(OnCurrentIndexChanged, ());
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::GetCurrentMediaItem(sbIMediaItem** aCurrentMediaItem)
{
  NS_ENSURE_ARG_POINTER(aCurrentMediaItem);
  nsresult rv;

  if (mCurrentIndex >= 0) {
    nsString guid;
    rv = mArray->GetGuidByIndex(mCurrentIndex, guid);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mLibrary->GetMediaItem(guid, aCurrentMediaItem);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    *aCurrentMediaItem = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::IsIndexSelected(PRInt32 aIndex,
                                                       PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv;

  // An invalid row is always not selected
  if (aIndex < 0 || aIndex > (PRInt32) mLength - 1) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  if (mSelectionIsAll) {
    *_retval = PR_TRUE;
    return NS_OK;
  }

  nsString uid;
  rv = GetUniqueIdForIndex((PRUint32) aIndex, uid);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = mSelection.Get(uid, nsnull);

  return NS_OK;
}

static nsresult IsContentTypeEqual(nsAString &aGuid,
                                   sbILibrary* aLibrary,
                                   const nsAString &aContentType,
                                   PRBool* aIsSame)
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(aIsSame);
  nsresult rv;

  nsCOMPtr<sbIMediaItem> item;
  rv = aLibrary->GetItemByGuid(aGuid, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString contentType;
  rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTTYPE),
                         contentType);
  NS_ENSURE_SUCCESS(rv, rv);

  *aIsSame = aContentType.Equals(contentType);
  return NS_OK;
}


NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::IsContentTypeSelected(
                                         const nsAString &aContentType,
                                         PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv;

  // If everything is selected, check our array
  if (mSelectionIsAll) {
    for (PRUint32 i = 0; i < mLength; i++) {
      nsAutoString guid;
      rv = mArray->GetGuidByIndex(i, guid);
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool isSame;
      rv = IsContentTypeEqual(guid, mLibrary, aContentType, &isSame);
      NS_ENSURE_SUCCESS(rv, rv);

      if (isSame) {
        *_retval = PR_TRUE;
        return NS_OK;
      }
    }
  }
  else {
    PRUint32 selectionCount = mSelection.Count();

    PRUint32 found = 0;
    for (PRUint32 i = 0; i < mLength && found < selectionCount; i++) {
      PRBool isIndexCached;
      rv = mArray->IsIndexCached(i, &isIndexCached);
      NS_ENSURE_SUCCESS(rv, rv);

      if (isIndexCached) {
        nsString uid;
        rv = GetUniqueIdForIndex(i, uid);
        NS_ENSURE_SUCCESS(rv, rv);

        nsString guid;
        if (mSelection.Get(uid, &guid)) {
          PRBool isSame;
          rv = IsContentTypeEqual(guid, mLibrary, aContentType, &isSame);
          NS_ENSURE_SUCCESS(rv, rv);

          if (isSame) {
            *_retval = PR_TRUE;
            return NS_OK;
          }

          found++;
        }
      }
    }

    // If we searched everything and no match, return
    if (found == selectionCount) {
      *_retval = PR_FALSE;
      return NS_OK;
    }

    // If we didn't find everything in the first pass of cached indexes,
    // do it again ignoring the cache (will cause database queries)
    for (PRUint32 i = 0; i < mLength; i++) {
      nsString uid;
      rv = GetUniqueIdForIndex(i, uid);
      NS_ENSURE_SUCCESS(rv, rv);

      nsString guid;
      if (mSelection.Get(uid, &guid)) {
        PRBool isSame;
        rv = IsContentTypeEqual(guid, mLibrary, aContentType, &isSame);
        NS_ENSURE_SUCCESS(rv, rv);

        if (isSame) {
          *_retval = PR_TRUE;
          return NS_OK;
        }
      }
    }
  }

  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::GetSelectedIndexedMediaItems(nsISimpleEnumerator** aSelectedMediaItems)
{
  NS_ENSURE_ARG_POINTER(aSelectedMediaItems);
  nsresult rv;

  // If everything is selected, simply return an enumerator over our array
  if (mSelectionIsAll) {
    *aSelectedMediaItems = new sbIndexedGUIDArrayEnumerator(mLibrary, mArray);
    NS_ENSURE_TRUE(*aSelectedMediaItems, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(*aSelectedMediaItems);
    return NS_OK;
  }

  // There is no way to determine the index of the items in the selection
  // list, so first walk through the cached indexes of the array and locate
  // the selected items
  nsRefPtr<sbGUIDArrayToIndexedMediaItemEnumerator>
    enumerator(new sbGUIDArrayToIndexedMediaItemEnumerator(mLibrary));
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

  PRUint32 selectionCount = mSelection.Count();

  PRUint32 found = 0;
  for (PRUint32 i = 0; i < mLength && found < selectionCount; i++) {
    PRBool isIndexCached;
    rv = mArray->IsIndexCached(i, &isIndexCached);
    NS_ENSURE_SUCCESS(rv, rv);

    if (isIndexCached) {
      nsString uid;
      rv = GetUniqueIdForIndex(i, uid);
      NS_ENSURE_SUCCESS(rv, rv);

      nsString guid;
      if (mSelection.Get(uid, &guid)) {
        rv = enumerator->AddGuid(guid, i);
        NS_ENSURE_SUCCESS(rv, rv);
        found++;
      }
    }
  }

  // If we found everything, return
  if (found == selectionCount) {
    NS_ADDREF(*aSelectedMediaItems = enumerator);
    return NS_OK;
  }

  // If we didn't find everything in the first pass of cached indexes,
  // do it again ignoring the cache (will cause database queries)
  enumerator = new sbGUIDArrayToIndexedMediaItemEnumerator(mLibrary);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

  for (PRUint32 i = 0; i < mLength; i++) {
    nsString uid;
    rv = GetUniqueIdForIndex(i, uid);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString guid;
    if (mSelection.Get(uid, &guid)) {
      rv = enumerator->AddGuid(guid, i);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  NS_ADDREF(*aSelectedMediaItems = enumerator);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::GetSelectedMediaItems(nsISimpleEnumerator * *aSelectedMediaItems)
{
  NS_ENSURE_ARG_POINTER(aSelectedMediaItems);
  nsresult rv;
  
  // just create an indexed enumerator and wrap it; this makes sure we reuse
  // the code and the two enumerator implementations are kept in sync.
  nsCOMPtr<nsISimpleEnumerator> indexedEnumerator;
  rv = GetSelectedIndexedMediaItems(getter_AddRefs(indexedEnumerator));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsRefPtr<sbIndexedToUnindexedMediaItemEnumerator> unwrapper =
    new sbIndexedToUnindexedMediaItemEnumerator(indexedEnumerator);
  NS_ENSURE_TRUE(unwrapper, NS_ERROR_OUT_OF_MEMORY);
  
  return CallQueryInterface(unwrapper.get(), aSelectedMediaItems);
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::Select(PRInt32 aIndex)
{
  NS_ENSURE_ARG_RANGE(aIndex, 0, (PRInt32) mLength - 1);
  nsresult rv;

  mCurrentIndex = aIndex;
  rv = GetUniqueIdForIndex(mCurrentIndex, mCurrentUID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddToSelection(aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  CheckSelectAll();

  NOTIFY_LISTENERS(OnSelectionChanged, ());

#ifdef DEBUG
  LogSelection();
#endif

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::SelectOnly(PRInt32 aIndex)
{
  NS_ENSURE_ARG_RANGE(aIndex, 0, (PRInt32) mLength - 1);
  nsresult rv;

  mCurrentIndex = aIndex;
  rv = GetUniqueIdForIndex(mCurrentIndex, mCurrentUID);
  NS_ENSURE_SUCCESS(rv, rv);

  mSelection.Clear();
  mSelectionIsAll = PR_FALSE;

  rv = AddToSelection(aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  CheckSelectAll();

  NOTIFY_LISTENERS(OnSelectionChanged, ());

#ifdef DEBUG
  LogSelection();
#endif

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::TimedSelectOnly(PRInt32 aIndex, PRInt32 aDelay)
{
  PRBool suppressed = mSelectionNotificationsSuppressed;
  if (aDelay != -1)
    mSelectionNotificationsSuppressed = PR_TRUE;
  nsresult rv = SelectOnly(aIndex);
  mSelectionNotificationsSuppressed = suppressed;
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDelay != -1 && !mSelectionNotificationsSuppressed) {
    if (mSelectTimer)
      mSelectTimer->Cancel();

    mSelectTimer = do_CreateInstance("@mozilla.org/timer;1");
    mSelectTimer->InitWithFuncCallback(DelayedSelectNotification, this, aDelay,
                                       nsITimer::TYPE_ONE_SHOT);
  }

  return NS_OK;
}

void
sbLocalDatabaseMediaListViewSelection::DelayedSelectNotification(nsITimer* aTimer, void* aClosure)
{
  nsRefPtr<sbLocalDatabaseMediaListViewSelection> self =
      static_cast<sbLocalDatabaseMediaListViewSelection*>(aClosure);
  if (self) {
    NOTIFY_LISTENERS_INDIRECT(self, OnSelectionChanged, ());
    aTimer->Cancel();
    self->mSelectTimer = nsnull;
  }
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::Toggle(PRInt32 aIndex)
{
  NS_ENSURE_ARG_RANGE((PRInt32) aIndex, 0, (PRInt32) mLength - 1);
  nsresult rv;

  mCurrentIndex = aIndex;
  rv = GetUniqueIdForIndex(mCurrentIndex, mCurrentUID);
  NS_ENSURE_SUCCESS(rv, rv);

  // If have an all selection, fill the selection with everything but the
  // toggled index
  if (mSelectionIsAll) {
    mSelectionIsAll = PR_FALSE;
    for (PRUint32 i = 0; i < mLength; i++) {
      if (i != (PRUint32) aIndex) {
        rv = AddToSelection(i);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    return NS_OK;
  }

  // Otherwise, simply do the toggle
  PRBool isSelected;
  rv = IsIndexSelected(aIndex, &isSelected);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isSelected) {
    rv = RemoveFromSelection(aIndex);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = AddToSelection(aIndex);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  CheckSelectAll();

  NOTIFY_LISTENERS(OnSelectionChanged, ());

#ifdef DEBUG
  LogSelection();
#endif

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::Clear(PRInt32 aIndex)
{
  NS_ENSURE_ARG_RANGE(aIndex, 0, (PRInt32) mLength - 1);

  nsresult rv;

  mCurrentIndex = aIndex;
  rv = GetUniqueIdForIndex(mCurrentIndex, mCurrentUID);
  NS_ENSURE_SUCCESS(rv, rv);

  // If have an all selection, fill the selection with everything but the
  // range we're clearing
  if (mSelectionIsAll) {
    mSelectionIsAll = PR_FALSE;
    for (PRUint32 i = 0; i < mLength; i++) {
      if (i != (PRUint32) aIndex) {
        rv = AddToSelection(i);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }

    NOTIFY_LISTENERS(OnSelectionChanged, ());

    return NS_OK;
  }

  rv = RemoveFromSelection((PRUint32) aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_LISTENERS(OnSelectionChanged, ());

#ifdef DEBUG
  LogSelection();
#endif

  return NS_OK;
}


NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::SelectRange(PRInt32 aStartIndex,
                                                   PRInt32 aEndIndex)
{
  TRACE(("sbLocalDatabaseMediaListViewSelection[0x%.8x] - SelectRange(%d, %d)",
         this, aStartIndex, aEndIndex));

  NS_ENSURE_ARG_RANGE(aStartIndex, 0, (PRInt32) mLength - 1);
  NS_ENSURE_ARG_RANGE(aEndIndex,   0, (PRInt32) mLength - 1);

  nsresult rv;

  if (mSelectionIsAll) {
    return NS_OK;
  }

  mCurrentIndex = aEndIndex;
  rv = GetUniqueIdForIndex(mCurrentIndex, mCurrentUID);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 start = PR_MIN(aStartIndex, aEndIndex);
  PRInt32 end   = PR_MAX(aStartIndex, aEndIndex);

  for (PRInt32 i = start; i <= end; i++) {
    rv = AddToSelection((PRUint32) i);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  CheckSelectAll();

  NOTIFY_LISTENERS(OnSelectionChanged, ());

#ifdef DEBUG
  LogSelection();
#endif

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::ClearRange(PRInt32 aStartIndex,
                                                  PRInt32 aEndIndex)
{
  NS_ENSURE_ARG_RANGE(aStartIndex, 0, (PRInt32) mLength - 1);
  NS_ENSURE_ARG_RANGE(aEndIndex,   0, (PRInt32) mLength - 1);

  nsresult rv;

  mCurrentIndex = aEndIndex;
  rv = GetUniqueIdForIndex(mCurrentIndex, mCurrentUID);
  NS_ENSURE_SUCCESS(rv, rv);

  // If have an all selection, fill the selection with everything but the
  // range we're clearing
  if (mSelectionIsAll) {
    mSelectionIsAll = PR_FALSE;
    for (PRUint32 i = 0; i < mLength; i++) {
      if (i < (PRUint32) aStartIndex || i > (PRUint32) aEndIndex) {
        rv = AddToSelection(i);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }

    NOTIFY_LISTENERS(OnSelectionChanged, ());

    return NS_OK;
  }

  for (PRInt32 i = aStartIndex; i <= aEndIndex; i++) {
    rv = RemoveFromSelection((PRUint32) i);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NOTIFY_LISTENERS(OnSelectionChanged, ());

#ifdef DEBUG
  LogSelection();
#endif

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::SelectNone()
{
  mSelection.Clear();
  mSelectionIsAll = PR_FALSE;
  mCurrentIndex = -1;
  mCurrentUID.Truncate();

  NOTIFY_LISTENERS(OnSelectionChanged, ());

#ifdef DEBUG
  LogSelection();
#endif

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::SelectAll()
{
  mSelection.Clear();
  mSelectionIsAll = PR_TRUE;

  NOTIFY_LISTENERS(OnSelectionChanged, ());

#ifdef DEBUG
  LogSelection();
#endif

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::AddListener(sbIMediaListViewSelectionListener* aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);

  PRBool success = mObservers.AppendElementUnlessExists(aListener);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::RemoveListener(sbIMediaListViewSelectionListener* aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  mObservers.RemoveElement(aListener);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::SetSelectionNotificationsSuppressed(PRBool aSelectionEventsSuppressed)
{
  mSelectionNotificationsSuppressed = aSelectionEventsSuppressed;

  // If this is being set to false, notify
  if (!mSelectionNotificationsSuppressed) {
    NOTIFY_LISTENERS(OnSelectionChanged, ());
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelection::GetSelectionNotificationsSuppressed(PRBool *aSelectionEventsSuppressed)
{
  NS_ENSURE_ARG_POINTER(aSelectionEventsSuppressed);
  *aSelectionEventsSuppressed = mSelectionNotificationsSuppressed;
  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListViewSelection::AddToSelection(PRUint32 aIndex)
{
  nsresult rv;

  nsString uid;
  rv = GetUniqueIdForIndex(aIndex, uid);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mSelection.Get(uid, nsnull)) {
    return NS_OK;
  }

  nsString guid;
  rv = mArray->GetGuidByIndex(aIndex, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mSelection.Put(uid, guid);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListViewSelection::RemoveFromSelection(PRUint32 aIndex)
{
  nsresult rv;

  nsString uid;
  rv = GetUniqueIdForIndex(aIndex, uid);
  NS_ENSURE_SUCCESS(rv, rv);

  mSelection.Remove(uid);

  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListViewSelection::GetUniqueIdForIndex(PRUint32 aIndex,
                                                           nsAString& aId)
{
  nsresult rv;

  /* For regular lists, the unique identifer is composed of the lists' guid
   * appended to the item's guid and viewItemUID (rowid-mediaitemid)
   * Thus the UniqueId is of the form:
   *   "listGUID|itemGUID|rowid-mediaitemid" */
  aId.Assign(mListGUID);
  aId.Append('|');

  nsString guid;
  rv = mArray->GetGuidByIndex(aIndex, guid);
  NS_ENSURE_SUCCESS(rv, rv);
  aId.Append(guid);
  aId.Append('|');

  // get the viewItemUID which is "rowid-mediaitemid" and append to the UniqueId
  nsString viewItemUID;
  rv = mArray->GetViewItemUIDByIndex(aIndex, viewItemUID);
  NS_ENSURE_SUCCESS(rv, rv);
  aId.Append(viewItemUID);

  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListViewSelection::GetUniqueIdForIndex(PRInt32    aIndex,
                                                           nsAString& aId)
{
  if (aIndex < 0) {
    aId.Truncate();
    return NS_OK;
  }
  return GetUniqueIdForIndex(static_cast<PRUint32>(aIndex), aId);
}

nsresult
sbLocalDatabaseMediaListViewSelection::GetIndexForUniqueId
                                         (const nsAString& aId,
                                          PRUint32*        aIndex)
{
  nsresult rv;

  // Split the unique ID into its components.
  nsTArray<nsString> idComponentList;
  nsString_Split(aId, NS_LITERAL_STRING("|"), idComponentList);
  if (idComponentList.Length() < 3)
    return NS_ERROR_NOT_AVAILABLE;

  /* use the unique id to get the viewItemUID which can help us get
   * the index of the item */
  nsString viewItemUID = idComponentList[2];

  // Get the index from the rowid and mediaitemid.
  rv = mArray->GetIndexByViewItemUID(viewItemUID, aIndex);
  if (rv == NS_ERROR_NOT_AVAILABLE)
    return NS_ERROR_NOT_AVAILABLE;
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

#ifdef DEBUG
void
sbLocalDatabaseMediaListViewSelection::LogSelection()
{
#ifdef LOG_SELECTION
  nsString list;

  if (mSelectionIsAll) {
    list.AssignLiteral("all");
  }
  else {
    PRUint32 oldFetchSize;
    nsresult rv = mArray->GetFetchSize(&oldFetchSize);
    NS_ENSURE_SUCCESS(rv, /* void */);

    rv = mArray->SetFetchSize(0);
    NS_ENSURE_SUCCESS(rv, /* void */);

    for (PRUint32 i = 0; i < mLength; i++) {
      nsString uid;
      GetUniqueIdForIndex(i, uid);

      nsString guid;
      if (mSelection.Get(uid, nsnull)) {
        list.AppendInt(i);
        list.Append(' ');
      }
    }

    rv = mArray->SetFetchSize(oldFetchSize);
    NS_ENSURE_SUCCESS(rv, /* void */);
  }

  TRACE(("sbLocalDatabaseMediaListViewSelection[0x%.8x] - LogSelection() "
         "length: %d, selection: %s",
         this, mLength, NS_LossyConvertUTF16toASCII(list).get()));
#endif
}
#endif

NS_IMPL_ISUPPORTS1(sbLocalDatabaseMediaListViewSelectionState,
                   nsISerializable);

sbLocalDatabaseMediaListViewSelectionState::sbLocalDatabaseMediaListViewSelectionState() :
  mCurrentIndex(-1),
  mSelectionIsAll(PR_FALSE)
{
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelectionState::Read(nsIObjectInputStream* aStream)
{
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  rv = aStream->Read32((PRUint32*) &mCurrentIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 selectionCount;
  rv = aStream->Read32(&selectionCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < selectionCount; i++) {
    nsString key;
    nsString entry;

    rv = aStream->ReadString(key);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aStream->ReadString(entry);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool success = mSelectionList.Put(key, entry);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  PRBool selectionIsAll;
  rv = aStream->ReadBoolean(&selectionIsAll);
  NS_ENSURE_SUCCESS(rv, rv);
  mSelectionIsAll = selectionIsAll;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListViewSelectionState::Write(nsIObjectOutputStream* aStream)
{
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  rv = aStream->Write32((PRUint32) mCurrentIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aStream->Write32(mSelectionList.Count());
  NS_ENSURE_SUCCESS(rv, rv);

  mSelectionList.EnumerateRead(SB_SerializeSelectionListCallback, aStream);

  rv = aStream->WriteBoolean(mSelectionIsAll);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListViewSelectionState::Init()
{
  PRBool success = mSelectionList.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListViewSelectionState::ToString(nsAString& aStr)
{
  nsString buff;

  buff.AppendLiteral(" currentIndex ");
  buff.AppendInt(mCurrentIndex);

  buff.AppendLiteral(" selection ");
  if (mSelectionIsAll) {
    buff.AppendLiteral("is all");
  }
  else {
    buff.AppendInt(mSelectionList.Count());
    buff.AppendLiteral(" items");
  }

  aStr = buff;

  return NS_OK;
}


NS_IMPL_THREADSAFE_ISUPPORTS2(sbGUIDArrayToIndexedMediaItemEnumerator,
                              nsISimpleEnumerator,
                              nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER2(sbGUIDArrayToIndexedMediaItemEnumerator,
                             nsISimpleEnumerator,
                             nsIClassInfo)

NS_DECL_CLASSINFO(sbGUIDArrayToIndexedMediaItemEnumerator)
NS_IMPL_THREADSAFE_CI(sbGUIDArrayToIndexedMediaItemEnumerator)

/**
 * \class sbGuidArrayToMediaItemEnumerator
 *
 * \brief A "pessimistic" enumerator that always looks ahead to make sure
 *        a next media item is available.
 */
sbGUIDArrayToIndexedMediaItemEnumerator::sbGUIDArrayToIndexedMediaItemEnumerator(sbILibrary* aLibrary) :
  mInitalized(PR_FALSE),
  mLibrary(aLibrary),
  mNextIndex(0),
  mNextItemIndex(0)
{
}

nsresult
sbGUIDArrayToIndexedMediaItemEnumerator::AddGuid(const nsAString& aGuid,
                                                 PRUint32 aIndex)
{
  Item* item = mItems.AppendElement();
  NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);

  item->index = aIndex;
  item->guid = aGuid;

  return NS_OK;
}

nsresult
sbGUIDArrayToIndexedMediaItemEnumerator::GetNextItem()
{
  if (!mInitalized) {
    mInitalized = PR_TRUE;
  }

  mNextItem = nsnull;
  PRUint32 length = mItems.Length();

  while (mNextIndex < length) {
    nsresult rv = mLibrary->GetMediaItem(mItems[mNextIndex].guid,
                                         getter_AddRefs(mNextItem));
    mNextItemIndex = mItems[mNextIndex].index;
    mNextIndex++;
    if (NS_SUCCEEDED(rv)) {
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
sbGUIDArrayToIndexedMediaItemEnumerator::HasMoreElements(PRBool *_retval)
{
  if (!mInitalized) {
    GetNextItem();
  }

  *_retval = mNextItem != nsnull;

  return NS_OK;
}

NS_IMETHODIMP
sbGUIDArrayToIndexedMediaItemEnumerator::GetNext(nsISupports **_retval)
{
  if (!mInitalized) {
    GetNextItem();
  }

  if (!mNextItem) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<sbLocalDatabaseIndexedMediaItem> indexedItem
    (new sbLocalDatabaseIndexedMediaItem(mNextItemIndex, mNextItem));
  NS_ENSURE_TRUE(indexedItem, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = NS_ISUPPORTS_CAST(sbIIndexedMediaItem*,
                                         indexedItem.get()));

  GetNextItem();

  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS2(sbIndexedGUIDArrayEnumerator, 
                              nsISimpleEnumerator,
                              nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER2(sbIndexedGUIDArrayEnumerator,
                             nsISimpleEnumerator,
                             nsIClassInfo)

NS_DECL_CLASSINFO(sbIndexedGUIDArrayEnumerator)
NS_IMPL_THREADSAFE_CI(sbIndexedGUIDArrayEnumerator)

sbIndexedGUIDArrayEnumerator::sbIndexedGUIDArrayEnumerator(sbILibrary* aLibrary,
                                                           sbILocalDatabaseGUIDArray* aArray) :
  mLibrary(aLibrary),
  mArray(aArray),
  mNextIndex(0),
  mInitalized(PR_FALSE)
{
  NS_ASSERTION(aLibrary, "aLibrary is null");
  NS_ASSERTION(aArray, "aArray is null");
}

nsresult
sbIndexedGUIDArrayEnumerator::Init()
{
  PRUint32 length;
  nsresult rv = mArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; i++) {
    nsAutoString guid;
    rv = mArray->GetGuidByIndex(i, guid);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString* added = mGUIDArray.AppendElement(guid);
    NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
  }

  mInitalized = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
sbIndexedGUIDArrayEnumerator::HasMoreElements(PRBool *_retval)
{
  if (!mInitalized) {
    nsresult rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *_retval = mNextIndex < mGUIDArray.Length();
  return NS_OK;
}

NS_IMETHODIMP
sbIndexedGUIDArrayEnumerator::GetNext(nsISupports **_retval)
{
  nsresult rv;

  if (!mInitalized) {
    rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!(mNextIndex < mGUIDArray.Length())) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<sbIMediaItem> item;
  rv = mLibrary->GetMediaItem(mGUIDArray[mNextIndex], getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbLocalDatabaseIndexedMediaItem> indexedItem
    (new sbLocalDatabaseIndexedMediaItem(mNextIndex, item));
  NS_ENSURE_TRUE(indexedItem, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = NS_ISUPPORTS_CAST(sbIIndexedMediaItem*,
                                         indexedItem.get()));

  mNextIndex++;
  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS2(sbIndexedToUnindexedMediaItemEnumerator,
                              nsISimpleEnumerator,
                              nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER2(sbIndexedToUnindexedMediaItemEnumerator,
                             nsISimpleEnumerator,
                             nsIClassInfo)

NS_DECL_CLASSINFO(sbIndexedToUnindexedMediaItemEnumerator)
NS_IMPL_THREADSAFE_CI(sbIndexedToUnindexedMediaItemEnumerator)

/**
 * \class sbIndexedToUnindexedMediaItemEnumerator
 *
 * \brief An unwrapper for sbGUIDArrayToIndexedMediaItemEnumerator to return
 *        unwrapped sbIMediaItems with no indices instead
 */
sbIndexedToUnindexedMediaItemEnumerator::sbIndexedToUnindexedMediaItemEnumerator(nsISimpleEnumerator* aEnumerator) :
  mIndexedEnumerator(aEnumerator)
{
}

NS_IMETHODIMP
sbIndexedToUnindexedMediaItemEnumerator::HasMoreElements(PRBool *_retval)
{
  NS_ENSURE_TRUE(mIndexedEnumerator, NS_ERROR_NOT_INITIALIZED);
  return mIndexedEnumerator->HasMoreElements(_retval);
}

NS_IMETHODIMP
sbIndexedToUnindexedMediaItemEnumerator::GetNext(nsISupports **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mIndexedEnumerator, NS_ERROR_NOT_INITIALIZED);
  
  nsresult rv;
  
  nsCOMPtr<sbIIndexedMediaItem> indexedItem;
  rv = mIndexedEnumerator->GetNext(getter_AddRefs(indexedItem));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<sbIMediaItem> item;
  rv = indexedItem->GetMediaItem(getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);
  
  return CallQueryInterface(item, _retval);
}
