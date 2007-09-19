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

#include "sbLocalDatabaseTreeView.h"

#include <nsIAtom.h>
#include <nsIAtomService.h>
#include <nsIClassInfoImpl.h>
#include <nsIDOMElement.h>
#include <nsIProgrammingLanguage.h>
#include <nsIStringBundle.h>
#include <nsIStringEnumerator.h>
#include <nsITreeBoxObject.h>
#include <nsITreeColumns.h>
#include <nsIVariant.h>
#include <sbIClickablePropertyInfo.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbILocalDatabasePropertyCache.h>
#include <sbILibrary.h>
#include <sbIMediaListView.h>
#include <sbIMediaList.h>
#include <sbIMediaItem.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <sbISortableMediaListView.h>
#include <sbITreeViewPropertyInfo.h>

#include <nsComponentManagerUtils.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsUnicharUtils.h>
#include <nsUnitConversion.h>
#include <prlog.h>
#include "sbLocalDatabasePropertyCache.h"
#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseGUIDArray.h"
#include "sbLocalDatabaseMediaItem.h"
#include "sbLocalDatabaseMediaListView.h"
#include <sbPropertiesCID.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>
#include <sbTArrayStringEnumerator.h>

#ifdef DEBUG
#include <prprf.h>
#endif

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbLocalDatabaseTreeView:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gLocalDatabaseTreeViewLog = nsnull;
#define TRACE(args) PR_LOG(gLocalDatabaseTreeViewLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gLocalDatabaseTreeViewLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

#define PROGRESS_VALUE_UNSET -1
#define PROGRESS_VALUE_COMPLETE 101

#define SB_STRING_BUNDLE_CHROME_URL "chrome://songbird/locale/songbird.properties"

#define BAD_CSS_CHARS "/.:# !@$%^&*(),?;'\"<>~=+`\\|[]{}"

/*
 * There are two distinct coordinate systems used in this file, one for the
 * underlying GUID array and one for the rows of the tree.  The term "index"
 * always refers to an index into the guid array, and the term "length" refers
 * to the length of the guid array.  The term "row" always refers to a row
 * in the tree, and the term "row count" refers to the total number of rows
 * in the tree.
 *
 * The TreeToArray and ArrayToTree methods are used to convert between the
 * two coordinate systems.
 *
 * mRowCache and mDiryRowCache use GUID array indexes for the keys.
 */


/* static */ nsresult PR_CALLBACK
sbLocalDatabaseTreeView::SelectionListSavingEnumeratorCallback(PRUint32 aIndex,
                                                               const nsAString& aId,
                                                               const nsAString& aGuid,
                                                               void* aUserData)
{
  NS_ENSURE_ARG_POINTER(aUserData);

  sbSelectionList* list = static_cast<sbSelectionList*>(aUserData);
  NS_ENSURE_STATE(list);

  nsAutoString guid(aGuid);
  PRBool success = list->Put(aId, guid);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

/* static */ nsresult PR_CALLBACK
sbLocalDatabaseTreeView::SelectionToArrayEnumeratorCallback(PRUint32 aIndex,
                                                            const nsAString& aId,
                                                            const nsAString& aGuid,
                                                            void* aUserData)
{
  NS_ENSURE_ARG_POINTER(aUserData);

  sbGUIDArrayToIndexedMediaItemEnumerator* enumerator =
    static_cast<sbGUIDArrayToIndexedMediaItemEnumerator*>(aUserData);
  NS_ENSURE_STATE(enumerator);

  nsresult rv = enumerator->AddGuid(aGuid, aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* static */ nsresult PR_CALLBACK
sbLocalDatabaseTreeView::SelectionIndexEnumeratorCallback(PRUint32 aIndex,
                                                          const nsAString& aId,
                                                          const nsAString& aGuid,
                                                          void* aUserData)
{
  NS_ENSURE_ARG_POINTER(aUserData);

  nsTHashtable<nsUint32HashKey>* selectedIndexes =
    static_cast<nsTHashtable<nsUint32HashKey>*>(aUserData);
  NS_ENSURE_STATE(selectedIndexes);

  nsUint32HashKey* success = selectedIndexes->PutEntry(aIndex);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMPL_ISUPPORTS7(sbLocalDatabaseTreeView,
                   nsIClassInfo,
                   nsISupportsWeakReference,
                   nsITreeView,
                   sbILocalDatabaseAsyncGUIDArrayListener,
                   sbILocalDatabaseGUIDArrayListener,
                   sbILocalDatabaseTreeView,
                   sbIMediaListViewTreeView)

NS_IMPL_CI_INTERFACE_GETTER6(sbLocalDatabaseTreeView,
                             nsIClassInfo,
                             nsITreeView,
                             sbILocalDatabaseAsyncGUIDArrayListener,
                             sbILocalDatabaseGUIDArrayListener,
                             sbILocalDatabaseTreeView,
                             sbIMediaListViewTreeView)

sbLocalDatabaseTreeView::sbLocalDatabaseTreeView() :
 mListType(eLibrary),
 mCachedRowCount(0),
 mNextGetByIndexAsync(-1),
 mMouseState(sbILocalDatabaseTreeView::MOUSE_STATE_NONE),
 mMouseStateRow(-1),
 mSelectionIsAll(PR_FALSE),
 mCachedRowCountDirty(PR_TRUE),
 mCachedRowCountPending(PR_FALSE),
 mIsArrayBusy(PR_FALSE),
 mGetByIndexAsyncPending(PR_FALSE),
 mClearSelectionPending(PR_FALSE),
 mFakeAllRow(PR_FALSE),
 mSelectionChanging(PR_FALSE)
{
  MOZ_COUNT_CTOR(sbLocalDatabaseTreeView);
#ifdef PR_LOGGING
  if (!gLocalDatabaseTreeViewLog) {
    gLocalDatabaseTreeViewLog = PR_NewLogModule("sbLocalDatabaseTreeView");
  }
#endif
}

sbLocalDatabaseTreeView::~sbLocalDatabaseTreeView()
{
  MOZ_COUNT_DTOR(sbLocalDatabaseTreeView);
  nsresult rv;
  if (mArray) {
    nsCOMPtr<sbILocalDatabaseAsyncGUIDArrayListener> asyncListener =
      do_QueryInterface(NS_ISUPPORTS_CAST(sbILocalDatabaseTreeView*, this),
                        &rv);
    if (NS_SUCCEEDED(rv))
      mArray->RemoveAsyncListener(asyncListener);
  }
}

nsresult
sbLocalDatabaseTreeView::Init(sbLocalDatabaseMediaListView* aMediaListView,
                              sbILocalDatabaseAsyncGUIDArray* aArray,
                              sbIPropertyArray* aCurrentSort)
{
  NS_ENSURE_ARG_POINTER(aMediaListView);
  NS_ENSURE_ARG_POINTER(aArray);
  NS_ENSURE_ARG_POINTER(aCurrentSort);

  // Make sure we actually are getting a sort
  PRUint32 arrayLength;
  nsresult rv = aCurrentSort->GetLength(&arrayLength);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_STATE(arrayLength);

  mPropMan = do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mMediaListView = aMediaListView;

  mArray = aArray;

  rv = mArray->GetPropertyCache(getter_AddRefs(mPropertyCache));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabaseGUIDArrayListener> listener =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbILocalDatabaseTreeView*, this), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->SetListener(listener);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabaseAsyncGUIDArrayListener> asyncListener =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbILocalDatabaseTreeView*, this), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->AddAsyncListener(asyncListener);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->GetFetchSize(&mFetchSize);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mRowCache.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  success = mPageCacheStatus.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  success = mDirtyRowCache.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  success = mSelectionList.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  // Determine the list type
  PRBool isDistinct;
  rv = mArray->GetIsDistinct(&isDistinct);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isDistinct) {
    mListType = eDistinct;
    mFakeAllRow = PR_TRUE;
    mSelectionIsAll = PR_TRUE;
  }
  else {
    nsAutoString baseTable;
    rv = mArray->GetBaseTable(baseTable);
    NS_ENSURE_SUCCESS(rv, rv);

    if (baseTable.EqualsLiteral("media_items")) {
      mListType = eLibrary;
    }
    else {
      mListType = eSimple;
    }
  }

  // Grab the top level sort property from the bag
  nsCOMPtr<sbIProperty> property;
  rv = aCurrentSort->GetPropertyAt(0, getter_AddRefs(property));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = property->GetName(mCurrentSortProperty);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString value;
  rv = property->GetValue(value);
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentSortDirectionIsAscending = value.EqualsLiteral("a");

  // Get our localized strings
  nsCOMPtr<nsIStringBundleService> bundleService =
    do_GetService("@mozilla.org/intl/stringbundle;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringBundle> stringBundle;
  rv = bundleService->CreateBundle(SB_STRING_BUNDLE_CHROME_URL,
                                   getter_AddRefs(stringBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stringBundle->GetStringFromName(NS_LITERAL_STRING("library.all").get(),
                                       getter_Copies(mLocalizedAll));
  if (NS_FAILED(rv)) {
    mLocalizedAll.AssignLiteral("library.all");
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::Rebuild()
{
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - Rebuild()", this));

  nsresult rv;

  // Clear the selection from the tree only if we don't have everything
  // selected.  The selection will be resored when the cached pages are read
  // back
  if (mRealSelection && !mSelectionIsAll) {
    mClearSelectionPending = PR_TRUE;
  }

  // Invalidate our cache
  rv = InvalidateCache();
  NS_ENSURE_SUCCESS(rv, rv);

  // Force a row recount
  mCachedRowCountDirty = PR_TRUE;
  mCachedRowCountPending = PR_FALSE;

  PRInt32 count;
  rv = GetRowCount(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
sbLocalDatabaseTreeView::ClearMediaListView()
{
  mMediaListView = nsnull;
}

/**
 * \brief Parses a string and separates space-delimited substrings into nsIAtom
 *        elements.
 *
 * Shamelessly adapted from nsTreeUtils.cpp.
 */
nsresult
sbLocalDatabaseTreeView::TokenizeProperties(const nsAString& aProperties,
                                            nsISupportsArray* aAtomArray)
{
  NS_ASSERTION(!aProperties.IsEmpty(), "Don't give this an empty string");
  NS_ASSERTION(aAtomArray, "Null pointer!");

  const PRUnichar* current, *end;
  aProperties.BeginReading(&current, &end);

  static const PRUnichar sSpaceChar = ' ';

  nsresult rv;
  nsCOMPtr<nsIAtomService> atomService =
    do_GetService(NS_ATOMSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  do {

    // Skip whitespace
    while (current < end && *current == sSpaceChar) {
      ++current;
    }

    // If only whitespace, we're done
    if (current == end) {
      break;
    }

    // Note the first non-whitespace character
    const PRUnichar* firstChar = current;

    // Advance to the next whitespace character
    while (current < end && *current != sSpaceChar) {
      ++current;
    }

    nsAutoString token(Substring(firstChar, current));

    // Make an atom
    nsCOMPtr<nsIAtom> atom;
    rv = atomService->GetAtom(token.get(), getter_AddRefs(atom));
    NS_ENSURE_SUCCESS(rv, rv);

    // Don't encourage people to step on each other's toes.
    if (aAtomArray->IndexOf(atom) != -1) {
      continue;
    }

    rv = aAtomArray->AppendElement(atom);
    NS_ENSURE_SUCCESS(rv, rv);

  } while (current < end);

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::UpdateRowCount(PRUint32 aRowCount)
{
  nsresult rv;

  if (mTreeBoxObject) {
    PRUint32 oldRowCount = mCachedRowCount;
    mCachedRowCount = aRowCount;
    mCachedRowCountDirty = PR_FALSE;
    mCachedRowCountPending = PR_FALSE;

    // Change the number of rows in the tree by the difference between the
    // new row count and the old cached row count.  We still need to invalidate
    // the whole tree since we don't know where the row changes too place.
    PRInt32 delta = mCachedRowCount - oldRowCount;
    rv = mTreeBoxObject->BeginUpdateBatch();
    NS_ENSURE_SUCCESS(rv, rv);
    if (delta != 0) {
      rv = mTreeBoxObject->RowCountChanged(oldRowCount, delta);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    rv = mTreeBoxObject->Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mTreeBoxObject->EndUpdateBatch();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // This is the first method that gets called after the tree has been rebuilt.
  // If we have a fake all row and the selection is "all", select it here.
  // Note that we do this here rather than in the code that restores selection
  // because we never get a callback for this fake row when repopulating the
  // tree.
  if (mFakeAllRow && mSelectionIsAll) {
    mSelectionChanging = PR_TRUE;
    rv = mRealSelection->Select(0);
    mSelectionChanging = PR_FALSE;
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::GetCellPropertyValue(PRInt32 aIndex,
                                              nsITreeColumn *aTreeColumn,
                                              nsAString& _retval)
{
  NS_ENSURE_ARG_POINTER(aTreeColumn);

  nsresult rv;

  nsAutoString bind;
  rv = GetPropertyForTreeColumn(aTreeColumn, bind);
  NS_ENSURE_SUCCESS(rv, rv);

  // If this is an ordinal column, return just the row number
  if (bind.EqualsLiteral(SB_PROPERTY_ORDINAL)) {
    _retval.AppendInt(aIndex + 1);
    return NS_OK;
  }

  nsCOMPtr<sbIPropertyInfo> info;
  rv = mPropMan->GetPropertyInfo(bind, getter_AddRefs(info));
  NS_ENSURE_SUCCESS(rv, rv);

  // Check our local map cache of property bags and return the cell value if
  // we have it cached
  nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
  if (mRowCache.Get(aIndex, getter_AddRefs(bag))) {
    nsAutoString value;
    rv = bag->GetProperty(bind, value);
    NS_ENSURE_SUCCESS(rv, rv);

    // Format the value for display
    rv = info->Format(value, _retval);
    if (NS_FAILED(rv)) {
      _retval.Truncate();
    }

    return NS_OK;
  }

  PageCacheStatus status;
  rv = GetPageCachedStatus(aIndex, &status);
  NS_ENSURE_SUCCESS(rv, rv);

  switch(status) {

    // Since we cache each page's data in mRowCache, this should never be
    // called.  Warn and fall through so we can still process the request.
    case eCached:
      NS_WARNING("Not cached in mRowCache but page cached?");

    // The age this row is in is not yet cached, so async request it and
    // return blank
    case eNotCached:
    {

      // If we already have a pending request, don't start another, just put
      // this new request on deck.  If there is already a next request on deck,
      // clobber it and reset its page's cache status.  The purpose of this is
      // to prevent a lot of requests from getting stacked up if the user
      // scrolls the list around a lot

      if (mGetByIndexAsyncPending) {

        if (mNextGetByIndexAsync > -1) {
          rv = SetPageCachedStatus(mNextGetByIndexAsync, eNotCached);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        mNextGetByIndexAsync = aIndex;
        rv = SetPageCachedStatus(aIndex, ePending);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {

        rv = SetPageCachedStatus(aIndex, ePending);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mArray->GetGuidByIndexAsync(aIndex);
        NS_ENSURE_SUCCESS(rv, rv);
        mGetByIndexAsyncPending = PR_TRUE;
      }

      // Try our dirty cache so we can show something
      if (mDirtyRowCache.Get(aIndex, getter_AddRefs(bag))) {
        rv = bag->GetProperty(bind, _retval);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        _retval.Truncate();
      }
    }
    break;

    // The page this row is in is pending, so just return a blank
    case ePending:
    {
      // Try our dirty cache so we can show something
      if (mDirtyRowCache.Get(aIndex, getter_AddRefs(bag))) {
        nsString value;
        rv = bag->GetProperty(bind, value);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = info->Format(value, _retval);
        if (NS_FAILED(rv)) {
          _retval.Truncate();
        }

      }
      else {
        _retval.Truncate();
      }
    }
    break;
  }

  if (!_retval.Equals(EmptyString())) {
    // Format the value for display
    rv = info->Format(_retval, _retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::GetPropertyBag(const nsAString& aGuid,
                                        PRUint32 aIndex,
                                        sbILocalDatabaseResourcePropertyBag** _retval)
{
  nsresult rv;

  // Since we've linked a property cache with the array, this guid
  // should be cached
  const PRUnichar* guid = aGuid.BeginReading();

  PRUint32 count = 0;
  sbILocalDatabaseResourcePropertyBag** bags = nsnull;
  rv = mPropertyCache->GetProperties(&guid, 1, &count, &bags);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
  if (count == 1 && bags[0]) {
    bag = bags[0];
  }
  NS_FREE_XPCOM_ISUPPORTS_POINTER_ARRAY(count, bags);

  if (!bag) {
    NS_WARNING("Could not find properties for guid!");
    return NS_ERROR_UNEXPECTED;
  }

  PRBool success = mRowCache.Put(aIndex, bag);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = bag);
  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::GetPageCachedStatus(PRUint32 aIndex,
                                             PageCacheStatus* aStatus)
{
  PRUint32 cell = aIndex / mFetchSize;
  PRUint32 status;
  if (mPageCacheStatus.Get(cell, &status)) {
    *aStatus = PageCacheStatus(status);
  }
  else {
    *aStatus = eNotCached;
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::SetPageCachedStatus(PRUint32 aIndex,
                                             PageCacheStatus aStatus)
{
  PRUint32 cell = aIndex / mFetchSize;
  PRBool success = mPageCacheStatus.Put(cell, (PRUint32) aStatus);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - "
         "SetPageCachedStatus(%d, %d) cell = %d", this, aIndex, aStatus, cell));

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::InvalidateCache()
{
  // Copy the visible pages into our temporary dirty row cache so we have
  // something to show the user while the tree is rebuilding
  if (mTreeBoxObject) {
    PRInt32 first;
    PRInt32 last;
    nsresult rv = mTreeBoxObject->GetFirstVisibleRow(&first);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mTreeBoxObject->GetLastVisibleRow(&last);
    NS_ENSURE_SUCCESS(rv, rv);

    if (first >= 0 && last >= 0) {
      for (PRUint32 row = first; row <= (PRUint32) last; row++) {
        nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
        if (mRowCache.Get(TreeToArray(row), getter_AddRefs(bag))) {
          PRBool success = mDirtyRowCache.Put(TreeToArray(row), bag);
          NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
        }
      }
    }

  }
  mRowCache.Clear();
  mPageCacheStatus.Clear();

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::SaveSelectionList()
{
  // If the current selection is everything, don't save anything
  if (mSelectionIsAll) {
    return NS_OK;
  }

  nsresult rv = EnumerateSelection(SelectionListSavingEnumeratorCallback,
                                   &mSelectionList);

  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult
sbLocalDatabaseTreeView::EnumerateSelection(sbSelectionEnumeratorCallbackFunc aFunc,
                                            void* aUserData)
{
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - EnumerateSelection()", this));

  if (mRealSelection) {

    // Iterate through the selection's ranges and store the unique identifier
    // for each row in the selection hash table.  Note that we don't want
    // to clear the selection list first as to keep items that were previously
    // selected but were never re-selected by the background cache fill
    PRInt32 rangeCount;
    nsresult rv = mRealSelection->GetRangeCount(&rangeCount);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRInt32 i = 0; i < rangeCount; i++) {
      PRInt32 min;
      PRInt32 max;
      rv = mRealSelection->GetRangeAt(i, &min, &max);
      NS_ENSURE_SUCCESS(rv, rv);

      if (min >= 0 && max >= 0) {
        for (PRInt32 j = min; j <= max; j++) {

          if (IsAllRow(j)) {
            continue;
          }

          nsAutoString id;
          PRUint32 index = TreeToArray(j);
          rv = GetUniqueIdForIndex(index, id);
          NS_ENSURE_SUCCESS(rv, rv);

          nsAutoString guid;
          rv = mArray->GetGuidByIndex(index, guid);
          NS_ENSURE_SUCCESS(rv, rv);

          TRACE(("sbLocalDatabaseTreeView[0x%.8x] - SaveSelectionList() - "
                 "saving %s from index %d guid %s",
                 this, NS_ConvertUTF16toUTF8(id).get(), index,
                 NS_ConvertUTF16toUTF8(guid).get()));

          rv = aFunc(index, id, guid, aUserData);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
      else {
        NS_WARNING("Bad value returned from nsTreeSelection::GetRangeAt");
      }
    }

  }

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::GetUniqueIdForIndex(PRUint32 aIndex, nsAString& aId)
{
  nsresult rv;

  // For distinct lists, the sort key works as a unique id, otherwise we can
  // use the rowid
  if (mListType == eDistinct) {
    rv = mArray->GetSortPropertyValueByIndex(aIndex, aId);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    PRUint64 rowid;
    rv = mArray->GetRowidByIndex(aIndex, &rowid);
    NS_ENSURE_SUCCESS(rv, rv);

    aId.Truncate();
    AppendInt(aId, rowid);
  }

  return NS_OK;
}

void
sbLocalDatabaseTreeView::SetSelectionIsAll(PRBool aSelectionIsAll) {

  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - SetSelectionIsAll(%d)",
         this, aSelectionIsAll));

  mSelectionIsAll = aSelectionIsAll;

  // An all selection means everything in our selection list was implicitly
  // selected so we can clear it
  if (mSelectionIsAll) {
    ClearSelectionList();
  }

}

void
sbLocalDatabaseTreeView::ClearSelectionList() {

  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - ClearSelectionList()", this));

  mSelectionList.Clear();
}

nsresult
sbLocalDatabaseTreeView::UpdateColumnSortAttributes(const nsAString& aProperty,
                                                    PRBool aDirection)
{
  nsresult rv;

  nsCOMPtr<nsITreeColumns> columns;
  rv = mTreeBoxObject->GetColumns(getter_AddRefs(columns));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 count;
  rv = columns->GetCount(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRInt32 i = 0; i < count; i++) {
    nsCOMPtr<nsITreeColumn> column;
    rv = columns->GetColumnAt(i, getter_AddRefs(column));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!column) {
      NS_WARNING("Failed to find column!");
      continue;
    }

    nsCOMPtr<nsIDOMElement> element;
    rv = column->GetElement(getter_AddRefs(element));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString bind;
    rv = element->GetAttribute(NS_LITERAL_STRING("bind"), bind);
    NS_ENSURE_SUCCESS(rv, rv);

    if (bind.Equals(aProperty)) {
      rv = element->SetAttribute(NS_LITERAL_STRING("sortActive"),
                                 NS_LITERAL_STRING("true"));
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString direction;
      if (aDirection) {
        direction.AssignLiteral("ascending");
      }
      else {
        direction.AssignLiteral("descending");
      }
      rv = element->SetAttribute(NS_LITERAL_STRING("sortDirection"),
                                 direction);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = element->RemoveAttribute(NS_LITERAL_STRING("sortActive"));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = element->RemoveAttribute(NS_LITERAL_STRING("sortDirection"));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

// sbILocalDatabaseTreeView
NS_IMETHODIMP
sbLocalDatabaseTreeView::SetSort(const nsAString& aProperty, PRBool aDirection)
{
  nsresult rv;

  // If we are linked to a media list view, use its interfaces to manage
  // the sort
  if (mListType != eDistinct) {
    NS_ENSURE_STATE(mMediaListView);

    // TODO: Get the sort profile from the property manager, if any
    nsCOMPtr<sbIMutablePropertyArray> sort =
      do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = sort->SetStrict(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = sort->AppendProperty(aProperty, aDirection ? NS_LITERAL_STRING("a") :
                                                      NS_LITERAL_STRING("d"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbISortableMediaListView> sortable =
      do_QueryInterface(NS_ISUPPORTS_CAST(sbISortableMediaListView*, mMediaListView), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = sortable->SetSort(sort);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = mArray->ClearSorts();
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mArray->AddSort(aProperty, aDirection);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mArray->Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = Rebuild();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = UpdateColumnSortAttributes(aProperty, aDirection);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetSelectionChanging(PRBool* aSelectionChanging)
{
  NS_ENSURE_ARG_POINTER(aSelectionChanging);

  *aSelectionChanging = mSelectionChanging;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::InvalidateRowsByGuid(const nsAString& aGuid)
{
  LOG(("sbLocalDatabaseTreeView[0x%.8x] - InvalidateRowsByGuid(%s)",
       this, NS_LossyConvertUTF16toASCII(aGuid).get()));

  if (mTreeBoxObject) {
    PRInt32 first;
    PRInt32 last;
    nsresult rv = mTreeBoxObject->GetFirstVisibleRow(&first);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mTreeBoxObject->GetLastVisibleRow(&last);
    NS_ENSURE_SUCCESS(rv, rv);

    if (first >= 0 && last >= 0) {
      PRUint32 length;
      rv = mArray->GetLength(&length);
      NS_ENSURE_SUCCESS(rv, rv);

      PRInt32 rowCount = ArrayToTree(length);

      if (last >= rowCount)
        last = rowCount - 1;

      for (PRInt32 row = first; row <= last; row++) {
        nsAutoString guid;
        rv = mArray->GetGuidByIndex(TreeToArray(row), guid);
        NS_ENSURE_SUCCESS(rv, rv);
        if (guid.Equals(aGuid)) {
          rv = mTreeBoxObject->InvalidateRow(row);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
    }

  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::SetMouseState(PRInt32 aRow,
                                       nsITreeColumn* aColumn,
                                       PRUint32 aState)
{
#ifdef PR_LOGGING
  PRInt32 colIndex = -1;
  if (aColumn) {
    aColumn->GetIndex(&colIndex);
  }
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - SetMouseState(%d, %d, %d)", this,
         aRow, colIndex, aState));
#endif

  nsresult rv;

  if (mMouseState && mMouseStateRow >= 0 && mMouseStateColumn) {
    mMouseState = sbILocalDatabaseTreeView::MOUSE_STATE_NONE;
    if (mTreeBoxObject) {
      rv = mTreeBoxObject->InvalidateCell(mMouseStateRow, mMouseStateColumn);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  
  mMouseState       = aState;
  mMouseStateRow    = aRow;
  mMouseStateColumn = aColumn;

  if (mMouseStateRow >= 0 && mMouseStateColumn && mTreeBoxObject) {
    rv = mTreeBoxObject->InvalidateCell(mMouseStateRow, mMouseStateColumn);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

// sbILocalDatabaseGUIDArrayListener
NS_IMETHODIMP
sbLocalDatabaseTreeView::OnBeforeInvalidate()
{
  nsresult rv = SaveSelectionList();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// sbILocalDatabaseAsyncGUIDArrayListener
NS_IMETHODIMP
sbLocalDatabaseTreeView::OnGetLength(PRUint32 aLength,
                                     nsresult aResult)
{
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - OnGetLength(%d)", this, aLength));

  nsresult rv;

  if (NS_SUCCEEDED(aResult)) {
    rv = UpdateRowCount(ArrayToTree(aLength));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::OnGetGuidByIndex(PRUint32 aIndex,
                                          const nsAString& aGUID,
                                          nsresult aResult)
{
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - OnGetGuidByIndex(%d, %s)", this,
         aIndex, NS_LossyConvertUTF16toASCII(aGUID).get()));

  nsresult rv;

  if (NS_SUCCEEDED(aResult) && !mCachedRowCountDirty) {

    // If there is a clear selection pending, clear the selection
    if (mClearSelectionPending) {
      mSelectionChanging = PR_TRUE;
      if (mSelectionIsAll) {
        rv = mRealSelection->Select(0);
      }
      else {
        rv = mRealSelection->ClearSelection();
      }
      mSelectionChanging = PR_FALSE;
      NS_ENSURE_SUCCESS(rv, rv);
      mClearSelectionPending = PR_FALSE;
    }

    // Now we know that the page this row is in has been fully cached by
    // the guid array, cache the entire page in our cache
    PRUint32 start = (aIndex / mFetchSize) * mFetchSize;
    PRUint32 end = start + mFetchSize - 1;
    if (end > TreeToArray(mCachedRowCount)) {
      end = TreeToArray(mCachedRowCount) - 1;
    }
    PRUint32 length = end - start + 1;

    // Suppress select events
    rv = mRealSelection->SetSelectEventsSuppressed(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    // Build an array of guids we want.  We build both a string array and a
    // pointer array because we need the former to make sure the pointers
    // remain valid.
    nsTArray<nsString> guids(length);
    nsTArray<const PRUnichar*> guidArray(length);

    for (PRUint32 i = 0; i < length; i++) {
      PRUint32 index = i + start;
      nsAutoString guid;
      rv = mArray->GetGuidByIndex(index, guid);
      if (NS_FAILED(rv)) {

        // If this fails, this means that the underlying array has changed and
        // we don't know it yet.  This is pretty bad and I would like to
        // figure out a nicer way to detect besides just handing the error.

        // Mark this page as not cached
        rv = SetPageCachedStatus(aIndex, eNotCached);
        NS_ENSURE_SUCCESS(rv, rv);
        mGetByIndexAsyncPending = PR_FALSE;

        NS_WARNING("Array changed out from under us - need to make this better");
        return NS_OK;
      }

      // XXXben Check to make sure that we have a nsITreeBoxObject here before
      //        modifying the selection. This was crashing occasionally, see
      //        bug 3533.
      nsCOMPtr<nsITreeBoxObject> boxObject;
      rv = mRealSelection->GetTree(getter_AddRefs(boxObject));
      NS_ENSURE_SUCCESS(rv, rv);

      NS_ASSERTION(boxObject, "No box object?! Fix bug 3533 correctly!");

      // Should this row be selected?
      if (mSelectionList.Count() && boxObject) {
        nsAutoString id;
        rv = GetUniqueIdForIndex(index, id);
        NS_ENSURE_SUCCESS(rv, rv);

        nsAutoString selectedGuid;
        if (mSelectionList.Get(id, &selectedGuid)) {
          TRACE(("sbLocalDatabaseTreeView[0x%.8x] - OnGetGuidByIndex() - "
                 "restoring selection %s at %d", this,
                 NS_ConvertUTF16toUTF8(id).get(), ArrayToTree(index)));

          mSelectionList.Remove(id);
          if (mRealSelection) {
            mSelectionChanging = PR_TRUE;
            PRInt32 row = ArrayToTree(index);
            rv = mRealSelection->RangedSelect(row, row, PR_TRUE);
            mSelectionChanging = PR_FALSE;
            NS_ENSURE_SUCCESS(rv, rv);
          }
        }
      }

      nsString* addedGuid = guids.AppendElement(guid);
      NS_ENSURE_TRUE(addedGuid, NS_ERROR_OUT_OF_MEMORY);

      const PRUnichar** addedPtr = guidArray.AppendElement(guid.get());
      NS_ENSURE_TRUE(addedPtr, NS_ERROR_OUT_OF_MEMORY);
    }

    // Unsuppress select events
    mSelectionChanging = PR_TRUE;
    rv = mRealSelection->SetSelectEventsSuppressed(PR_FALSE);
    mSelectionChanging = PR_FALSE;
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 count = 0;
    sbILocalDatabaseResourcePropertyBag** bags = nsnull;
    rv = mPropertyCache->GetProperties(guidArray.Elements(), length, &count, &bags);
    NS_ENSURE_SUCCESS(rv, rv);

    // Cache each of the returned property bags
    for (PRUint32 i = 0; i < count; i++) {
      if (bags[i]) {
        PRBool success = mRowCache.Put(i + start, bags[i]);
        if (!success) {
          NS_FREE_XPCOM_ISUPPORTS_POINTER_ARRAY(count, bags);
          return NS_ERROR_OUT_OF_MEMORY;
        }

        // Now that we have the real data we can remove our temp entry from
        // mDirtyRowCache.
        mDirtyRowCache.Remove(i + start);
      }
    }

    NS_FREE_XPCOM_ISUPPORTS_POINTER_ARRAY(count, bags);

    TRACE(("sbLocalDatabaseTreeView[0x%.8x] - OnGetGuidByIndex - "
           "InvalidateRange(%d, %d)", this, start, end));

    if (mTreeBoxObject) {
      rv = mTreeBoxObject->InvalidateRange(ArrayToTree(start),
                                           ArrayToTree(end));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = SetPageCachedStatus(aIndex, eCached);
    NS_ENSURE_SUCCESS(rv, rv);

    mGetByIndexAsyncPending = PR_FALSE;

    // If there was another request on deck, send it now
    if (mNextGetByIndexAsync > -1) {
      rv = mArray->GetGuidByIndexAsync(mNextGetByIndexAsync);
      NS_ENSURE_SUCCESS(rv, rv);
      mNextGetByIndexAsync = -1;
      mGetByIndexAsyncPending = PR_TRUE;
    }

    return NS_OK;
  }

  // If we get here, there was some kind of error getting the rows from the
  // array.  Mark the requested row as not cached.
  rv = SetPageCachedStatus(aIndex, eNotCached);
  NS_ENSURE_SUCCESS(rv, rv);

  mGetByIndexAsyncPending = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::OnGetSortPropertyValueByIndex(PRUint32 aIndex,
                                                       const nsAString& aGUID,
                                                       nsresult aResult)
{
  NS_NOTREACHED("Shouldn't get called");
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::OnGetMediaItemIdByIndex(PRUint32 aIndex,
                                                 PRUint32 aMediaItemId,
                                                 nsresult aResult)
{
  NS_NOTREACHED("Shouldn't get called");
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::OnStateChange(PRUint32 aState)
{
  if (aState == sbILocalDatabaseAsyncGUIDArrayListener::STATE_BUSY) {
    mIsArrayBusy = PR_TRUE;
  }
  else if (aState == sbILocalDatabaseAsyncGUIDArrayListener::STATE_IDLE) {
    mIsArrayBusy = PR_FALSE;
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::GetPropertyForTreeColumn(nsITreeColumn* aTreeColumn,
                                                  nsAString& aProperty)
{
  NS_ENSURE_ARG_POINTER(aTreeColumn);

  nsresult rv;

  nsCOMPtr<nsIDOMElement> element;
  rv = aTreeColumn->GetElement(getter_AddRefs(element));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = element->GetAttribute(NS_LITERAL_STRING("bind"), aProperty);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::GetTreeColumnForProperty(const nsAString& aProperty,
                                                  nsITreeColumn** aTreeColumn)
{
  NS_ENSURE_ARG_POINTER(aTreeColumn);
  NS_ENSURE_STATE(mTreeBoxObject);

  nsCOMPtr<nsITreeColumns> columns;
  nsresult rv = mTreeBoxObject->GetColumns(getter_AddRefs(columns));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 count;
  rv = columns->GetCount(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRInt32 i = 0; i < count; i++) {
    nsCOMPtr<nsITreeColumn> column;
    rv = columns->GetColumnAt(i, getter_AddRefs(column));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMElement> element;
    rv = column->GetElement(getter_AddRefs(element));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString bind;
    rv = element->GetAttribute(NS_LITERAL_STRING("bind"), bind);
    NS_ENSURE_SUCCESS(rv, rv);

    if (bind.Equals(aProperty)) {
      NS_ADDREF(*aTreeColumn = column);
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

/* inline */ nsresult
sbLocalDatabaseTreeView::GetColumnPropertyInfo(nsITreeColumn* aColumn,
                                               sbIPropertyInfo** aPropertyInfo)
{
  nsAutoString propertyName;
  nsresult rv = GetPropertyForTreeColumn(aColumn, propertyName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropMan->GetPropertyInfo(propertyName, aPropertyInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::GetPropertyInfoAndCachedValue(PRInt32 aRow,
                                                       nsITreeColumn* aColumn,
                                                       nsAString& aValue,
                                                       sbIPropertyInfo** aPropertyInfo)
{
  NS_ASSERTION(aColumn, "aColumn is null");
  NS_ASSERTION(aPropertyInfo, "aPropertyInfo is null");

  PRUint32 index = TreeToArray(aRow);
  nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
  if (!mRowCache.Get(index, getter_AddRefs(bag)) &&
      !mDirtyRowCache.Get(index, getter_AddRefs(bag))) {
    // Don't bother to do anything else if this row isn't cached yet.
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<sbIPropertyInfo> pi;
  nsresult rv = GetColumnPropertyInfo(aColumn, getter_AddRefs(pi));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString propertyName;
  rv = pi->GetName(propertyName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString value;
  rv = bag->GetProperty(propertyName, aValue);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aPropertyInfo = pi);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetRowCount(PRInt32 *aRowCount)
{
  NS_ENSURE_ARG_POINTER(aRowCount);

  nsresult rv;

  if (mCachedRowCountDirty) {
    if (!mCachedRowCountPending) {
      rv = mArray->GetLengthAsync();
      NS_ENSURE_SUCCESS(rv, rv);
      mCachedRowCountPending = PR_TRUE;
    }
  }

  *aRowCount = mCachedRowCount;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetCellText(PRInt32 row,
                                     nsITreeColumn *col,
                                     nsAString& _retval)
{
  NS_ENSURE_ARG_POINTER(col);

  nsresult rv;

  if (IsAllRow(row)) {
    _retval.Assign(mLocalizedAll);
    return NS_OK;
  }

  rv = GetCellPropertyValue(TreeToArray(row), col, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetSelection(nsITreeSelection** aSelection)
{
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - GetSelection()", this));

  NS_ENSURE_ARG_POINTER(aSelection);

  if (!mSelection) {
    return NS_ERROR_FAILURE;
  }

  NS_ADDREF(*aSelection = mSelection);
  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseTreeView::SetSelection(nsITreeSelection* aSelection)
{
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - SetSelection(0x%.8x)", this, aSelection));

  NS_ENSURE_ARG_POINTER(aSelection);

  // Wrap the selection given to us with our own proxy implementation so
  // we can get useful notifications on how the selection is changing
  mSelection = new sbLocalDatabaseTreeSelection(aSelection, this, mFakeAllRow);
  NS_ENSURE_TRUE(mSelection, NS_ERROR_OUT_OF_MEMORY);

  // Keep a ref to the real selection so we can modify it without going
  // through our proxy
  mRealSelection = aSelection;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::CycleHeader(nsITreeColumn* col)
{
  NS_ENSURE_ARG_POINTER(col);

  nsresult rv;

  // If our guid array is busy, ignore the click
  if (mIsArrayBusy) {
    return NS_OK;
  }

  nsAutoString bind;
  rv = GetPropertyForTreeColumn(col, bind);
  NS_ENSURE_SUCCESS(rv, rv);

  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - CycleHeader %s", this,
         NS_LossyConvertUTF16toASCII(bind).get()));

  if (bind.EqualsLiteral(SB_PROPERTY_ORDINAL)) {
    if (mListType == eLibrary) {
      rv = SetSort(NS_LITERAL_STRING(SB_PROPERTY_CREATED), PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);
      return NS_OK;
    }
  }

  if (bind.Equals(mCurrentSortProperty)) {
    mCurrentSortDirectionIsAscending = !mCurrentSortDirectionIsAscending;
  }
  else {
    mCurrentSortProperty = bind;
    mCurrentSortDirectionIsAscending = PR_TRUE;
  }

  rv = SetSort(mCurrentSortProperty, mCurrentSortDirectionIsAscending);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::CycleCell(PRInt32 row,
                                   nsITreeColumn* col)
{
  NS_ENSURE_ARG_POINTER(col);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetRowProperties(PRInt32 row,
                                          nsISupportsArray* properties)
{
  NS_ENSURE_ARG_MIN(row, 0);
  NS_ENSURE_ARG_POINTER(properties);

  nsresult rv;

  PRUint32 count;
  properties->Count(&count);
  nsString props;
  for (PRUint32 i = 0; i < count; i++) {
    nsCOMPtr<nsIAtom> atom;
    properties->QueryElementAt(i, NS_GET_IID(nsIAtom), getter_AddRefs(atom));
    if (atom) {
      nsString s;
      atom->ToString(s);
      props.Append(s);
      props.AppendLiteral(" ");
    }
  }

  if (IsAllRow(row)) {
    rv = TokenizeProperties(NS_LITERAL_STRING("all"), properties);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  PRUint32 index = TreeToArray(row);

  nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
  if (!mRowCache.Get(index, getter_AddRefs(bag)) &&
      !mDirtyRowCache.Get(index, getter_AddRefs(bag))) {
    // Don't bother to do anything else if this row isn't cached yet.
    return NS_OK;
  }

  nsCOMPtr<nsIStringEnumerator> propertyEnumerator;
  rv = mPropMan->GetPropertyNames(getter_AddRefs(propertyEnumerator));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString propertyName;
  while (NS_SUCCEEDED(propertyEnumerator->GetNext(propertyName))) {

    nsAutoString value;
    nsresult rv = bag->GetProperty(propertyName, value);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIPropertyInfo> propInfo;
    rv = mPropMan->GetPropertyInfo(propertyName, getter_AddRefs(propInfo));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbITreeViewPropertyInfo> tvpi = do_QueryInterface(propInfo, &rv);
    if (NS_SUCCEEDED(rv)) {
      nsString propertiesString;
      rv = tvpi->GetRowProperties(value, propertiesString);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!propertiesString.IsEmpty()) {
        rv = TokenizeProperties(propertiesString, properties);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }


  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetCellProperties(PRInt32 row,
                                           nsITreeColumn* col,
                                           nsISupportsArray* properties)
{
  NS_ENSURE_ARG_MIN(row, 0);
  NS_ENSURE_ARG_POINTER(col);
  NS_ENSURE_ARG_POINTER(properties);

#ifdef PR_LOGGING
  PRInt32 colIndex = -1;
  col->GetIndex(&colIndex);

  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - GetCellProperties(%d, %d)", this,
         row, colIndex));
#endif

  if (IsAllRow(row)) {
    return NS_OK;
  }

  nsresult rv;

  rv = GetColumnProperties(col, properties);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the mouse states
  if (mMouseStateRow == row && mMouseStateColumn == col) {
    switch(mMouseState) {
      case sbILocalDatabaseTreeView::MOUSE_STATE_HOVER:
        rv = TokenizeProperties(NS_LITERAL_STRING("cell-hover"), properties);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      case sbILocalDatabaseTreeView::MOUSE_STATE_DOWN:
        rv = TokenizeProperties(NS_LITERAL_STRING("cell-active"), properties);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
    }
  }

  nsCOMPtr<sbIPropertyInfo> pi;
  nsString value;
  rv = GetPropertyInfoAndCachedValue(row, col, value, getter_AddRefs(pi));
  // Returns NS_ERROR_NOT_AVAILABLE when the row is not cached
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbITreeViewPropertyInfo> tvpi = do_QueryInterface(pi, &rv);
  if (NS_SUCCEEDED(rv)) {
    nsString propertiesString;
    rv = tvpi->GetCellProperties(value, propertiesString);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!propertiesString.IsEmpty()) {
      rv = TokenizeProperties(propertiesString, properties);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  nsCOMPtr<sbIClickablePropertyInfo> cpi = do_QueryInterface(pi, &rv);
  if (NS_SUCCEEDED(rv)) {
    PRBool isDisabled;
    rv = cpi->IsDisabled(value, &isDisabled);
    NS_ENSURE_SUCCESS(rv, rv);

    if (isDisabled) {
      rv = TokenizeProperties(NS_LITERAL_STRING("disabled"), properties);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetColumnProperties(nsITreeColumn* col,
                                             nsISupportsArray* properties)
{
  NS_ENSURE_ARG_POINTER(col);
  NS_ENSURE_ARG_POINTER(properties);

  nsAutoString propertyName;
  nsresult rv = GetPropertyForTreeColumn(col, propertyName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Turn the property name into something that CSS can handle.  For example
  //
  //   http://songbirdnest.com/data/1.0#rating
  //
  // becomes:
  //
  //   http-songbirdnest-com-data-1-0-rating

  NS_NAMED_LITERAL_STRING(badChars, BAD_CSS_CHARS);
  static const PRUnichar kHyphenChar = '-';

  for (PRUint32 index = 0; index < propertyName.Length(); index++) {
    PRUnichar testChar = propertyName.CharAt(index);

    // Short circuit for ASCII alphanumerics.
    if ((testChar >= 97 && testChar <= 122) || // a-z
        (testChar >= 65 && testChar <= 90) ||  // A-Z
        (testChar >= 48 && testChar <= 57)) {  // 0-9
      continue;
    }

    PRInt32 badCharIndex= badChars.FindChar(testChar);
    if (badCharIndex > -1) {
      if (index > 0 && propertyName.CharAt(index - 1) == kHyphenChar) {
        propertyName.Replace(index, 1, nsnull, 0);
        index--;
      }
      else {
        propertyName.Replace(index, 1, kHyphenChar);
      }
    }
  }

  rv = TokenizeProperties(propertyName, properties);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsContainer(PRInt32 row, PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsContainerOpen(PRInt32 row, PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsContainerEmpty(PRInt32 row, PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsSeparator(PRInt32 row, PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsSorted(PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  // Media lists are always sorted
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::CanDrop(PRInt32 row,
                                 PRInt32 orientation,
                                 PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - CanDrop(%d, %d)", this,
         row, orientation));

  if (!IsAllRow(row) && mObserver) {
    nsresult rv = mObserver->CanDrop(TreeToArray(row),
                                     orientation,
                                     _retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    *_retval = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::Drop(PRInt32 row, PRInt32 orientation)
{
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - Drop(%d, %d)", this,
         row, orientation));

  if (!IsAllRow(row) && mObserver) {
    nsresult rv = mObserver->Drop(TreeToArray(row), orientation);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetParentIndex(PRInt32 rowIndex, PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::HasNextSibling(PRInt32 rowIndex,
                                        PRInt32 afterIndex,
                                        PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetLevel(PRInt32 row, PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetImageSrc(PRInt32 row,
                                     nsITreeColumn* col,
                                     nsAString& _retval)
{
  NS_ENSURE_ARG_POINTER(col);

  if (IsAllRow(row)) {
    return NS_OK;
  }

  nsresult rv;

  nsString value;
  nsCOMPtr<sbIPropertyInfo> pi;
  rv = GetPropertyInfoAndCachedValue(row, col, value, getter_AddRefs(pi));
  // Returns NS_ERROR_NOT_AVAILABLE when the row is not cached
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbITreeViewPropertyInfo> tvpi = do_QueryInterface(pi, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = tvpi->GetImageSrc(value, _retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetProgressMode(PRInt32 row,
                                         nsITreeColumn* col,
                                         PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(col);
  NS_ENSURE_ARG_POINTER(_retval);

  if (IsAllRow(row)) {
    *_retval = nsITreeView::PROGRESS_NONE;
    return NS_OK;
  }

  nsresult rv;

  nsString value;
  nsCOMPtr<sbIPropertyInfo> pi;
  rv = GetPropertyInfoAndCachedValue(row, col, value, getter_AddRefs(pi));
  // Returns NS_ERROR_NOT_AVAILABLE when the row is not cached
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbITreeViewPropertyInfo> tvpi = do_QueryInterface(pi, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = tvpi->GetProgressMode(value, _retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetCellValue(PRInt32 row,
                                      nsITreeColumn* col,
                                      nsAString& _retval)
{
#ifdef PR_LOGGING
  PRInt32 colIndex = -1;
  col->GetIndex(&colIndex);

  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - GetCellValue(%d, %d)", this,
         row, colIndex));
#endif

  if (IsAllRow(row)) {
    _retval.Truncate();
    return NS_OK;
  }

  nsresult rv;

  nsCOMPtr<sbIPropertyInfo> pi;
  nsString value;
  rv = GetPropertyInfoAndCachedValue(row, col, value, getter_AddRefs(pi));
  // Returns NS_ERROR_NOT_AVAILABLE when the row is not cached
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbITreeViewPropertyInfo> tvpi = do_QueryInterface(pi, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = tvpi->GetCellValue(value, _retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::SetTree(nsITreeBoxObject *tree)
{
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - SetTree(0x%.8x)", this, tree));

  // XXX: nsTreeBoxObject calls this method with a null to break a cycle so
  // we can't NS_ENSURE_ARG_POINTER(tree)

  mTreeBoxObject = tree;

  if (tree) {
    nsresult rv = UpdateColumnSortAttributes(mCurrentSortProperty,
                                             mCurrentSortDirectionIsAscending);
    NS_ENSURE_SUCCESS(rv, rv);

    // Rebuild view with the new tree
    rv = Rebuild();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::ToggleOpenState(PRInt32 row)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::SelectionChanged()
{
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - SelectionChanged()", this));

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsEditable(PRInt32 row,
                                    nsITreeColumn* col,
                                    PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(col);
  NS_ENSURE_ARG_POINTER(_retval);

  if (IsAllRow(row)) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  nsresult rv;

  nsCOMPtr<sbIPropertyInfo> propInfo;
  nsAutoString bind;
  rv = GetColumnPropertyInfo(col, getter_AddRefs(propInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propInfo->GetUserEditable(_retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsSelectable(PRInt32 row,
                                      nsITreeColumn* col,
                                      PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(col);
  NS_ENSURE_ARG_POINTER(_retval);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::SetCellValue(PRInt32 row,
                                      nsITreeColumn* col,
                                      const nsAString& value)
{
  NS_ENSURE_ARG_POINTER(col);

  nsresult rv;
#ifdef PR_LOGGING
  PRInt32 colIndex;
  rv = col->GetIndex(&colIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - SetCellValue(%d, %d %s)", this,
         row, colIndex, NS_LossyConvertUTF16toASCII(value).get()));
#endif
  rv = SetCellText(row, col, value);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::SetCellText(PRInt32 row,
                                     nsITreeColumn* col,
                                     const nsAString& value)
{
  NS_ENSURE_ARG_POINTER(col);
  NS_ENSURE_STATE(mMediaListView);

  if (IsAllRow(row)) {
    return NS_OK;
  }

  nsresult rv;
#ifdef PR_LOGGING
  PRInt32 colIndex;
  rv = col->GetIndex(&colIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - SetCellText(%d, %d %s)", this,
         row, colIndex, NS_LossyConvertUTF16toASCII(value).get()));
#endif

  nsAutoString bind;
  rv = GetPropertyForTreeColumn(col, bind);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString guid;
  rv = mArray->GetGuidByIndex(TreeToArray(row), guid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> mediaList;
  rv = mMediaListView->GetMediaList(getter_AddRefs(mediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> library;
  rv = mediaList->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;
  rv = library->GetMediaItem(guid, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString oldValue;
  rv = item->GetProperty(bind, oldValue);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!value.Equals(oldValue)) {
    rv = item->SetProperty(bind, value);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::PerformAction(const PRUnichar* action)
{
  NS_ENSURE_ARG_POINTER(action);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::PerformActionOnRow(const PRUnichar* action,
                                            PRInt32 row)
{
  NS_ENSURE_ARG_POINTER(action);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::PerformActionOnCell(const PRUnichar* action,
                                             PRInt32 row,
                                             nsITreeColumn* col)
{
  NS_ENSURE_ARG_POINTER(action);
  NS_ENSURE_ARG_POINTER(col);

  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 * See sbIMediaListViewTreeView
 */
NS_IMETHODIMP
sbLocalDatabaseTreeView::GetNextRowIndexForKeyNavigation(const nsAString& aKeyString,
                                                         PRUint32 aStartFrom,
                                                         PRInt32* _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "wrong thread");
  NS_ENSURE_FALSE(aKeyString.IsEmpty(), NS_ERROR_INVALID_ARG);
  NS_ENSURE_ARG_POINTER(_retval);

  // If we don't know how many rows should be in this table then bail.
  // Otherwise the algorithm below can loop forever.
  if (mCachedRowCountDirty) {
    NS_WARNING("We don't know how many rows are available. Bailing.");
    *_retval = -1;
    return NS_OK;
  }

  nsAutoString keyString(aKeyString);
  PRUint32 keyStringLength = keyString.Length();

  // Make sure that this is lowercased. The obj_sortable table should always be
  // lowercased.
  ToLowerCase(keyString);

  // Most folks will use this function to check the very next row in the tree.
  // Try that before doing the cache search or the costly database search.
  nsAutoString testString;
  nsresult rv = mArray->GetSortPropertyValueByIndex(aStartFrom, testString);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!keyString.Compare(Substring(testString, 0, keyStringLength))) {
    *_retval = aStartFrom;
    return NS_OK;
  }

  // Now search the cache. This will be way faster than the database search.
  PRInt32 comparison, lastRow;
  PRUint32 rowsSeen, index;

  // Three conditions to bail:
  //   1. The test string is no longer larger than the key string.
  //   2. We have iterated past the maximum number of rows in the list.
  //   3. We have passed all the rows currently cached.
  for (comparison = 1, lastRow = -1, rowsSeen = 0, index = aStartFrom + 1;
       comparison == 1 && index < mCachedRowCount && rowsSeen < mRowCache.Count();
       index++) {

    if (!mRowCache.Get(index, nsnull)) {
      // We haven't cached this row yet so continue.
      continue;
    }

    // Get the sort property value for the row.
    rv = mArray->GetSortPropertyValueByIndex(index, testString);
    NS_ENSURE_SUCCESS(rv, rv);

    // See how it compares to our key string.
    comparison = keyString.Compare(Substring(testString, 0, keyStringLength));

    if (comparison != 0) {
      // The string didn't match, but remember that this row was cached.
      lastRow = index;
      rowsSeen++;

      // And keep churning.
      continue;
    }

    // We know that we've found the right spot if the row immediately
    // preceeding this one was cached yet did not match or if this is the
    // first row to be searched.
    if ((lastRow == ((PRInt32) index) - 1) || (index == aStartFrom)) {
      *_retval = index;
      return NS_OK;
    }
  }

  // We didn't find a sure spot in the cache, so ask the database for the real
  // answer.
  rv = mArray->GetFirstIndexByPrefix(keyString, &index);
  if (NS_FAILED(rv) || (index < aStartFrom)) {
    *_retval = -1;
    return NS_OK;
  }

  // We're going to assume that the caller wants to look at the row we're about
  // to return. Trick the GUID array into caching the page that it's on so that
  // the data will arrive faster.
  rv = mArray->GetGuidByIndexAsync(index);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "GetGuidByIndexAsync");

  *_retval = (PRInt32)index;
  return NS_OK;
}

/**
 * See sbIMediaListViewTreeView
 */
NS_IMETHODIMP
sbLocalDatabaseTreeView::SetObserver(sbIMediaListViewTreeViewObserver* aObserver)
{
  mObserver = aObserver;

  return NS_OK;
}

/**
 * See sbIMediaListViewTreeView
 */
NS_IMETHODIMP
sbLocalDatabaseTreeView::GetObserver(sbIMediaListViewTreeViewObserver** aObserver)
{
  NS_ENSURE_ARG_POINTER(aObserver);

  NS_IF_ADDREF(*aObserver = mObserver);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetSelectedMediaItems(nsISimpleEnumerator** aSelection)
{
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - GetSelectedMediaItems()", this));

  NS_ENSURE_ARG_POINTER(aSelection);
  NS_ENSURE_STATE(mMediaListView);

  nsCOMPtr<sbIMediaList> list;
  nsresult rv = mMediaListView->GetMediaList(getter_AddRefs(list));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> library;
  rv = list->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  // If everything is selected, simply return an enumerator over our array
  if (mSelectionIsAll) {
    TRACE(("sbLocalDatabaseTreeView[0x%.8x] - "
           "GetSelectedMediaItems() - all select", this));

    *aSelection = new sbIndexedGUIDArrayEnumerator(library, mArray);
    NS_ENSURE_TRUE(*aSelection, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(*aSelection);
    return NS_OK;
  }

  // If our saved selection is empty, we know the index of every selected
  // item, so just build the list and return an enumerator
  if (mSelectionList.Count() == 0) {
    TRACE(("sbLocalDatabaseTreeView[0x%.8x] - "
           "GetSelectedMediaItems() - no saved selection", this));

    nsRefPtr<sbGUIDArrayToIndexedMediaItemEnumerator>
      enumerator(new sbGUIDArrayToIndexedMediaItemEnumerator(library));
    NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

    // Fill the enumerator with guids
    rv = EnumerateSelection(SelectionToArrayEnumeratorCallback, enumerator);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*aSelection = enumerator);
    return NS_OK;
  }

  // There is no way to determine the index of the items in the saved selection
  // list, so we need to synchronously (ew) force all the pages into cache
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - "
         "GetSelectedMediaItems() - have saved selection", this));

  // Get all the indexes from the selection
  nsTHashtable<nsUint32HashKey> selectedIndexes;
  PRBool success = selectedIndexes.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  rv = EnumerateSelection(SelectionIndexEnumeratorCallback, &selectedIndexes);
  NS_ENSURE_SUCCESS(rv, rv);

  // Iterate through the enitre array synchronously, adding guids to the
  // enumerator if they are either found in the selectedIndexes or our saved
  // selection
  PRUint32 length;
  rv = mArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbGUIDArrayToIndexedMediaItemEnumerator>
    enumerator(new sbGUIDArrayToIndexedMediaItemEnumerator(library));
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

  PRUint32 count = 0;
  for (PRUint32 i = 0; i < length; i++) {

    if (selectedIndexes.GetEntry(i)) {
      nsAutoString guid;
      rv = mArray->GetGuidByIndex(i, guid);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = enumerator->AddGuid(guid, i);
      NS_ENSURE_SUCCESS(rv, rv);
      count++;
    }
    else {
      nsAutoString id;
      rv = GetUniqueIdForIndex((PRUint32) i, id);
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString guid;
      if (mSelectionList.Get(id, &guid)) {
        rv = enumerator->AddGuid(guid, i);
        NS_ENSURE_SUCCESS(rv, rv);
        count++;
      }
    }
  }

  NS_ADDREF(*aSelection = enumerator);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetSelectionCount(PRUint32* aSelectionLength)
{
  NS_ENSURE_ARG_POINTER(aSelectionLength);

  nsresult rv;

  if (mSelectionIsAll) {
    rv = mArray->GetLength(aSelectionLength);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    // Add the saved selection list with the number of selected rows
    PRUint32 length = mSelectionList.Count();

    PRInt32 rangeCount;
    nsresult rv = mRealSelection->GetRangeCount(&rangeCount);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRInt32 i = 0; i < rangeCount; i++) {
      PRInt32 min;
      PRInt32 max;
      rv = mRealSelection->GetRangeAt(i, &min, &max);
      NS_ENSURE_SUCCESS(rv, rv);

      length += max - min + 1;

      if (IsAllRow(min)) {
        length--;
      }
    }

    *aSelectionLength = length;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::RemoveSelectedMediaItems()
{
  NS_ENSURE_STATE(mListType != eDistinct);
  NS_ENSURE_STATE(mMediaListView);

  nsresult rv;

  nsRefPtr<sbLocalDatabaseMediaListBase> list =
    mMediaListView->GetNativeMediaList();

  nsRefPtr<sbLocalDatabaseLibrary> library = list->GetNativeLibrary();

  nsCOMPtr<nsISimpleEnumerator> selection;
  rv = GetSelectedMediaItems(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = library->RemoveSelected(selection, mMediaListView);
  NS_ENSURE_SUCCESS(rv, rv);

  // Invalidate current index as its row has been removed
  if (mRealSelection) {
    rv = mRealSelection->SetCurrentIndex(-1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetCurrentMediaItem(sbIMediaItem** aCurrentMediaItem)
{
  NS_ENSURE_ARG_POINTER(aCurrentMediaItem);
  NS_ENSURE_STATE(mRealSelection);
  NS_ENSURE_STATE(mMediaListView);

  PRInt32 currentRow;
  nsresult rv = mRealSelection->GetCurrentIndex(&currentRow);
  NS_ENSURE_SUCCESS(rv, rv);

  // If there is no current index, return null
  if (currentRow < 0) {
    *aCurrentMediaItem = nsnull;
    return NS_OK;
  }

  nsAutoString guid;
  rv = mArray->GetGuidByIndex(TreeToArray(currentRow), guid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> list;
  rv = mMediaListView->GetMediaList(getter_AddRefs(list));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = list->GetItemByGuid(guid, aCurrentMediaItem);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// nsIClassInfo
NS_IMETHODIMP
sbLocalDatabaseTreeView::GetInterfaces(PRUint32* count, nsIID*** array)
{
  return NS_CI_INTERFACE_GETTER_NAME(sbLocalDatabaseTreeView)(count, array);
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetHelperForLanguage(PRUint32 language,
                                              nsISupports** _retval)
{
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetContractID(char** aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetClassDescription(char** aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetClassID(nsCID** aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetImplementationLanguage(PRUint32* aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetFlags(PRUint32 *aFlags)
{
  *aFlags = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  return NS_ERROR_NOT_AVAILABLE;
}

// nsITreeSelection
//
// Note that this is mostly a proxy to our inner mSelection.  It is used to
// to get a detailed look at how the user is changing the selection.  We are
// only concerned if the user changes the selection in such a way that would
// affect our selection list, that is, the list of selected items that have
// not been pulled into the page cache.

NS_IMPL_ISUPPORTS1(sbLocalDatabaseTreeSelection,
                   nsITreeSelection)

sbLocalDatabaseTreeSelection::sbLocalDatabaseTreeSelection(nsITreeSelection* aSelection,
                                                           sbLocalDatabaseTreeView* aTreeView,
                                                           PRBool aAllRow) :
  mSelection(aSelection),
  mTreeView(aTreeView),
  mAllRow(aAllRow)
{
  NS_ASSERTION(aSelection && aTreeView, "Null passed to ctor");
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::GetTree(nsITreeBoxObject** aTree)
{
  TRACE(("sbLocalDatabaseTreeSelection[0x%.8x] - GetTree()", this));

  return mSelection->GetTree(aTree);
}
NS_IMETHODIMP
sbLocalDatabaseTreeSelection::SetTree(nsITreeBoxObject* aTree)
{
  TRACE(("sbLocalDatabaseTreeSelection[0x%.8x] - SetTree(0x%.8x)",
         this, aTree));

  return mSelection->SetTree(aTree);
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::GetSingle(PRBool* aSingle)
{
  return mSelection->GetSingle(aSingle);
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::GetCount(PRInt32* aCount)
{
  return mSelection->GetCount(aCount);
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::IsSelected(PRInt32 index, PRBool* _retval)
{
  return mSelection->IsSelected(index, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::Select(PRInt32 index)
{
  TRACE(("sbLocalDatabaseTreeSelection[0x%.8x] - Select(%d)", this, index));

  nsresult rv = mSelection->Select(index);
  NS_ENSURE_SUCCESS(rv, rv);

  // This method causes all other rows to be de-selected, so we need to clear
  // the selection list
  mTreeView->ClearSelectionList();
  mTreeView->SetSelectionIsAll(PR_FALSE);

  // This method could have caused all rows to be selected, so check
  rv = CheckIsSelectAll();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::TimedSelect(PRInt32 index, PRInt32 delay)
{
  TRACE(("sbLocalDatabaseTreeSelection[0x%.8x] - TimedSelect(%d, %d)",
         this, index, delay));

  nsresult rv = mSelection->TimedSelect(index, delay);
  NS_ENSURE_SUCCESS(rv, rv);

  // This method calls Select() internally, so treat it the same
  mTreeView->ClearSelectionList();
  mTreeView->SetSelectionIsAll(PR_FALSE);

  // This method could have caused all rows to be selected, so check
  rv = CheckIsSelectAll();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::ToggleSelect(PRInt32 index)
{
  TRACE(("sbLocalDatabaseTreeSelection[0x%.8x] - ToggleSelect(%d)",
         this, index));

  nsresult rv = mSelection->ToggleSelect(index);
  NS_ENSURE_SUCCESS(rv, rv);

  // This method could have caused all rows to be selected, so check
  rv = CheckIsSelectAll();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::RangedSelect(PRInt32 startIndex,
                                           PRInt32 endIndex,
                                           PRBool augment)
{
  TRACE(("sbLocalDatabaseTreeSelection[0x%.8x] - RangedSelect(%d, %d, %d)",
         this, startIndex, endIndex, augment));

  nsresult rv = mSelection->RangedSelect(startIndex, endIndex, augment);
  NS_ENSURE_SUCCESS(rv, rv);

  // If the ranged select caused a select all, just tell our tree and return
  PRBool isSelectAll;
  rv = CheckIsSelectAll(&isSelectAll);
  NS_ENSURE_SUCCESS(rv, rv);
  if (isSelectAll) {
    return NS_OK;
  }

  // If we are not augmenting the current selection (which means we have an
  // entirely new selection), clear the selection list
  if (!augment) {
    mTreeView->ClearSelectionList();
    return NS_OK;
  }

  // A startIndex of -1 has some special meaning, see:
  // http://mxr.mozilla.org/seamonkey/source/layout/xul/base/src/tree/src/nsTreeSelection.cpp#445
  if (startIndex == -1) {
    PRInt32 shiftSelectPivot;
    rv = mSelection->GetShiftSelectPivot(&shiftSelectPivot);
    NS_ENSURE_SUCCESS(rv, rv);

    if (shiftSelectPivot != -1) {
      startIndex = shiftSelectPivot;
    }
    else {
      PRInt32 currentIndex;
      rv = mSelection->GetCurrentIndex(&currentIndex);
      NS_ENSURE_SUCCESS(rv, rv);

      if (currentIndex != -1) {
        startIndex = currentIndex;
      }
      else {
        startIndex = endIndex;
      }
    }
  }

  // Check to see if the selection range includes pages that we have not cached
  // yet.
  PRBool includesNotCachedPage = RangeIncludesNotCachedPage(startIndex,
                                                            endIndex);

  // If the selection range does not include any non-cached pages, we know it
  // does not affect our selection list, so just return.  We also know there
  // can't be any more non-cached pages after this range if the end of the
  // range is at the end of the list.
  if (!includesNotCachedPage ||
      ((PRUint32) endIndex + 1) == mTreeView->mCachedRowCount) {
    TRACE(("sbLocalDatabaseTreeSelection[0x%.8x] - RangedSelect() - "
           "Ranged select over cached pages", this));
    return NS_OK;
  }

  // Now see if there are any other non-cached pages after the end of the
  // range
  includesNotCachedPage = RangeIncludesNotCachedPage(endIndex + 1,
                                                     mTreeView->mCachedRowCount - 1);

  // If there are no more non-cached pages in the list, we know that the
  // ranged selection contains our entire selection list, so we can clear it
  if (!includesNotCachedPage) {
    mTreeView->ClearSelectionList();
    TRACE(("sbLocalDatabaseTreeSelection[0x%.8x] - RangedSelect() - "
           "Ranged select over all non-cached pages", this));
    return NS_OK;
  }

  // This is where things get a little hairy.  We need to manually knock out
  // the items that were selected by this selection range from the selection
  // list
  for (PRUint32 i = startIndex; i <= (PRUint32) endIndex; i++) {
    nsAutoString id;
    rv = mTreeView->GetUniqueIdForIndex(i, id);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString guid;
    if (mTreeView->mSelectionList.Get(id, &guid)) {
      mTreeView->mSelectionList.Remove(id);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::ClearRange(PRInt32 startIndex,
                                         PRInt32 endIndex)
{
  TRACE(("sbLocalDatabaseTreeSelection[0x%.8x] - ClearRange(%d, %d)",
         this, startIndex, endIndex));

  nsresult rv = mSelection->ClearRange(startIndex, endIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  // This would affect our selection list, but the xul tree binding does not
  // use this method so we won't bother implementing it for now.

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::ClearSelection()
{
  TRACE(("sbLocalDatabaseTreeSelection[0x%.8x] - ClearSelection()", this));

  mTreeView->mSelectionChanging = PR_TRUE;
  nsresult rv = mSelection->ClearSelection();
  mTreeView->mSelectionChanging = PR_FALSE;
  NS_ENSURE_SUCCESS(rv, rv);

  // Simple clear the selection list when the entire selection is cleared
  mTreeView->ClearSelectionList();

  // This method could have caused all rows to be selected, so check
  rv = CheckIsSelectAll();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::InvertSelection()
{
  // This would affect our selection list, but since the moz tree selection
  // does not implement it, no worries!
  return mSelection->InvertSelection();
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::SelectAll()
{
  TRACE(("sbLocalDatabaseTreeSelection[0x%.8x] - SelectAll()", this));

  nsresult rv = mSelection->SelectAll();
  NS_ENSURE_SUCCESS(rv, rv);

  // Simply tell the view that everything was selected
  mTreeView->SetSelectionIsAll(PR_TRUE);

  if (mAllRow) {
    rv = mSelection->Select(0);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::GetRangeCount(PRInt32* _retval)
{
  return mSelection->GetRangeCount(_retval);
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::GetRangeAt(PRInt32 i,
                                         PRInt32* min,
                                         PRInt32* max)
{
  return mSelection->GetRangeAt(i, min, max);
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::InvalidateSelection()
{
  return mSelection->InvalidateSelection();
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::AdjustSelection(PRInt32 index, PRInt32 count)
{
  TRACE(("sbLocalDatabaseTreeSelection[0x%.8x] - AdjustSelection(%d, %d)",
         this, index, count));

  nsresult rv = mSelection->AdjustSelection(index, count);
  NS_ENSURE_SUCCESS(rv, rv);

  // If the current index is past the last row, invalidate it.
  PRInt32 currentIndex;
  rv = this->GetCurrentIndex(&currentIndex);
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt32 rowCount;
  rv = mTreeView->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);
  if (currentIndex >= rowCount) {
    rv = this->SetCurrentIndex(-1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // This would affect our selection list, but the xul tree binding does not
  // use this method so we won't bother implementing it for now.
  // XXX: Not true, I think it gets called when the tree grows?

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::GetSelectEventsSuppressed(PRBool* aSelectEventsSuppressed)
{
  return mSelection->GetSelectEventsSuppressed(aSelectEventsSuppressed);
}
NS_IMETHODIMP
sbLocalDatabaseTreeSelection::SetSelectEventsSuppressed(PRBool aSelectEventsSuppressed)
{
  return mSelection->SetSelectEventsSuppressed(aSelectEventsSuppressed);
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::GetCurrentIndex(PRInt32* aCurrentIndex)
{
  return mSelection->GetCurrentIndex(aCurrentIndex);
}
NS_IMETHODIMP
sbLocalDatabaseTreeSelection::SetCurrentIndex(PRInt32 aCurrentIndex)
{
  return mSelection->SetCurrentIndex(aCurrentIndex);
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::GetCurrentColumn(nsITreeColumn** aCurrentColumn)
{
  return mSelection->GetCurrentColumn(aCurrentColumn);
}
NS_IMETHODIMP
sbLocalDatabaseTreeSelection::SetCurrentColumn(nsITreeColumn* aCurrentColumn)
{
  return mSelection->SetCurrentColumn(aCurrentColumn);
}

NS_IMETHODIMP
sbLocalDatabaseTreeSelection::GetShiftSelectPivot(PRInt32* aShiftSelectPivot)
{
  return mSelection->GetShiftSelectPivot(aShiftSelectPivot);
}

nsresult
sbLocalDatabaseTreeSelection::CheckIsSelectAll(PRBool* _retval)
{
  PRInt32 rangeCount;
  nsresult rv = mSelection->GetRangeCount(&rangeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isSelectAll = PR_TRUE;
  PRBool isFirstRowSelected = PR_FALSE;
  if (rangeCount > 0) {
    PRInt32 nextRow = 0;
    for (PRInt32 i = 0; isSelectAll && i < rangeCount; i++) {
      PRInt32 min;
      PRInt32 max;
      rv = mSelection->GetRangeAt(i, &min, &max);
      NS_ENSURE_SUCCESS(rv, rv);

      if (min == 0) {
        isFirstRowSelected = PR_TRUE;
      }

      for (PRInt32 j = min; j <= max; j++) {
        if (j != nextRow) {
          isSelectAll = PR_FALSE;
        }
        nextRow++;
      }
    }

    if ((PRUint32) nextRow != mTreeView->mCachedRowCount) {
      isSelectAll = PR_FALSE;
    }

  }
  else {
    isSelectAll = PR_FALSE;
  }

  // If we are maintaining an all row, and if the user has selected all rows,
  // no rows, or only the first row, this is an all selection
  if (mAllRow) {
    if (isSelectAll || rangeCount == 0 || isFirstRowSelected) {
      PRInt32 count;
      rv = mTreeView->GetRowCount(&count);
      NS_ENSURE_SUCCESS(rv, rv);

      if (count) {
        mTreeView->mSelectionChanging = PR_TRUE;
        rv = mSelection->Select(0);
        mTreeView->mSelectionChanging = PR_FALSE;
        NS_ENSURE_SUCCESS(rv, rv);
      }

      isSelectAll = PR_TRUE;
    }
  }

  mTreeView->SetSelectionIsAll(isSelectAll);
  if (_retval) {
    *_retval = isSelectAll;
  }
  return NS_OK;
}

PRBool
sbLocalDatabaseTreeSelection::RangeIncludesNotCachedPage(PRUint32 startIndex,
                                                         PRUint32 endIndex)
{
  PRUint32 startPage = startIndex / mTreeView->mFetchSize;
  PRUint32 endPage   = endIndex   / mTreeView->mFetchSize;
  PRBool includesNotCachedPage = PR_FALSE;
  for (PRUint32 i = startPage; !includesNotCachedPage && i <= endPage; i++) {

    // A page is not cached if it is not found in page cache status, or if its
    // status is explicitly set to eNotCached
    PRUint32 status;
    if (!mTreeView->mPageCacheStatus.Get(i, &status) ||
        status == sbLocalDatabaseTreeView::eNotCached) {
      includesNotCachedPage = PR_TRUE;
    }
  }

  return includesNotCachedPage;
}

NS_IMPL_ISUPPORTS1(sbGUIDArrayToIndexedMediaItemEnumerator, nsISimpleEnumerator)

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

NS_IMPL_ISUPPORTS1(sbIndexedGUIDArrayEnumerator, nsISimpleEnumerator)

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
