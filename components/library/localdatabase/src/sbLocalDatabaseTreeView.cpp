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

#include <nsIDOMElement.h>
#include <nsITreeBoxObject.h>
#include <nsITreeColumns.h>
#include <nsITreeSelection.h>
#include <sbILocalDatabasePropertyCache.h>
#include <sbIMediaListView.h>
#include <sbIPropertyArray.h>
#include <sbISortableMediaList.h>

#include <nsComponentManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsUnicharUtils.h>
#include <nsUnitConversion.h>
#include <prlog.h>
#include "sbLocalDatabasePropertyCache.h"
#include "sbLocalDatabaseCID.h"
#include <sbPropertiesCID.h>
#include <sbTArrayStringEnumerator.h>

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

NS_IMPL_ISUPPORTS3(sbLocalDatabaseTreeView,
                   nsITreeView,
                   sbILocalDatabaseAsyncGUIDArrayListener,
                   sbILocalDatabaseTreeView)

sbLocalDatabaseTreeView::sbLocalDatabaseTreeView() :
 mCachedRowCount(0),
 mNextGetByIndexAsync(-1),
 mCachedRowCountDirty(PR_TRUE),
 mCachedRowCountPending(PR_FALSE),
 mIsArrayBusy(PR_FALSE),
 mGetByIndexAsyncPending(PR_FALSE)
{
#ifdef PR_LOGGING
  if (!gLocalDatabaseTreeViewLog) {
    gLocalDatabaseTreeViewLog = PR_NewLogModule("sbLocalDatabaseTreeView");
  }
#endif
}

sbLocalDatabaseTreeView::~sbLocalDatabaseTreeView()
{
}

nsresult
sbLocalDatabaseTreeView::Init(sbIMediaListView* aMediaListView,
                              sbILocalDatabaseAsyncGUIDArray* aArray,
                              sbIPropertyArray* aCurrentSort)
{
  NS_ENSURE_ARG_POINTER(aArray);

  nsresult rv;

  // This can be null if we are not linked to a media list view
  mMediaListView = aMediaListView;

  mArray = aArray;

  rv = mArray->GetPropertyCache(getter_AddRefs(mPropertyCache));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->SetListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->GetFetchSize(&mFetchSize);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mRowCache.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  success = mPageCacheStatus.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::Rebuild()
{
  InvalidateCache();

  PRUint32 oldRowCount = mCachedRowCount;
  mCachedRowCount = 0;
  mCachedRowCountDirty = PR_TRUE;

  if (mTreeBoxObject) {
    nsresult rv = mTreeBoxObject->BeginUpdateBatch();
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mTreeBoxObject->RowCountChanged(0, -oldRowCount);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mTreeBoxObject->EndUpdateBatch();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::UpdateRowCount(PRUint32 aRowCount)
{
  if (mTreeBoxObject) {
    PRUint32 oldRowCount = mCachedRowCount;
    mCachedRowCount = aRowCount;
    mCachedRowCountDirty = PR_FALSE;
    mCachedRowCountPending = PR_FALSE;

    nsresult rv = mTreeBoxObject->BeginUpdateBatch();
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mTreeBoxObject->RowCountChanged(0, -oldRowCount);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mTreeBoxObject->RowCountChanged(0, mCachedRowCount);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mTreeBoxObject->EndUpdateBatch();
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
  const PRUnichar** guids = new const PRUnichar*[1];
  nsAutoString guid(aGuid);
  guids[0] = guid.get();

  PRUint32 count = 0;
  sbILocalDatabaseResourcePropertyBag** bags = nsnull;
  rv = mPropertyCache->GetProperties(guids, 1, &count, &bags);
  delete[] guids;
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
  if (count == 1 && bags[0]) {
    bag = bags[0];
  }
  NS_Free(bags);

  if (!bag) {
    NS_WARNING("Linked property cache did not cache row!");
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
sbLocalDatabaseTreeView::SetSort(const nsAString& aProperty, PRBool aDirection)
{
  nsresult rv;

  // If we are linked to a media list view, use its interfaces to manage
  // the sort
  if (mMediaListView) {
    // TODO: Get the sort profile from the property manager, if any
    nsCOMPtr<sbIPropertyArray> sort =
      do_CreateInstance(SB_PROPERTYARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = sort->AppendProperty(aProperty, aDirection ? NS_LITERAL_STRING("a") :
                                                      NS_LITERAL_STRING("d"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbISortableMediaList> sortable =
      do_QueryInterface(mMediaListView, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = sortable->SetSort(sort);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = mArray->ClearSorts();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mArray->AddSort(aProperty, aDirection);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  InvalidateCache();

  if (mTreeBoxObject) {
    rv = mTreeBoxObject->Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

void
sbLocalDatabaseTreeView::InvalidateCache()
{
  mRowCache.Clear();
  mPageCacheStatus.Clear();
}

// sbILocalDatabaseAsyncGUIDArrayListener
NS_IMETHODIMP
sbLocalDatabaseTreeView::OnGetLength(PRUint32 aLength,
                                     nsresult aResult)
{
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - OnGetLength(%d)", this, aLength));

  nsresult rv;

  if (NS_SUCCEEDED(aResult)) {
    rv = UpdateRowCount(aLength);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::OnGetByIndex(PRUint32 aIndex,
                                      const nsAString& aGUID,
                                      nsresult aResult)
{
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - OnGetByIndex(%d, %s)", this,
         aIndex, NS_LossyConvertUTF16toASCII(aGUID).get()));

  nsresult rv;

  if (NS_SUCCEEDED(aResult)) {

    // Now we know that the page this row is in has been fully cached by
    // the guid array, cache the entire page in our cache
    PRUint32 start = (aIndex / mFetchSize) * mFetchSize;
    PRUint32 end = start + mFetchSize;
    if (end > mCachedRowCount) {
      end = mCachedRowCount - 1;
    }
    PRUint32 length = end - start + 1;

    // Allocate an array of the guids we want
    const PRUnichar** guids = new const PRUnichar*[length];
    for (PRUint32 i = 0; i < length; i++) {
      nsAutoString guid;
      rv = mArray->GetByIndex(i + start, guid);
      NS_ENSURE_SUCCESS(rv, rv);
      guids[i] = ToNewUnicode(guid);
    }

    PRUint32 count = 0;
    sbILocalDatabaseResourcePropertyBag** bags = nsnull;
    rv = mPropertyCache->GetProperties(guids, length, &count, &bags);
    // Note that we check rv after we free guids array

    // Free the array and its contents
    for (PRUint32 i = 0; i < length; i++) {
      delete guids[i];
    }
    delete[] guids;
    NS_ENSURE_SUCCESS(rv, rv);

    // Cache each of the returned property bags
    for (PRUint32 i = 0; i < count; i++) {
      if (bags[i]) {
        PRBool success = mRowCache.Put(i + start, bags[i]);
        NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
      }
    }
    NS_Free(bags);

    rv = mTreeBoxObject->InvalidateRange(start, end);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = SetPageCachedStatus(aIndex, eCached);
    NS_ENSURE_SUCCESS(rv, rv);

    mGetByIndexAsyncPending = PR_FALSE;

    // If there was another request on deck, send it now
    if (mNextGetByIndexAsync > -1) {
      rv = mArray->GetByIndexAsync(mNextGetByIndexAsync);
      NS_ENSURE_SUCCESS(rv, rv);
      mNextGetByIndexAsync = -1;
      mGetByIndexAsyncPending = PR_TRUE;
    }

  }

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

  nsAutoString bind;
  rv = element->GetAttribute(NS_LITERAL_STRING("bind"), aProperty);
  NS_ENSURE_SUCCESS(rv, rv);

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

  nsAutoString bind;
  rv = GetPropertyForTreeColumn(col, bind);
  NS_ENSURE_SUCCESS(rv, rv);

  if (bind.Equals(NS_LITERAL_STRING("row"))) {
    _retval.AppendInt(row);
    return NS_OK;
  }

  // Check our local map cache of property bags and return the cell value if
  // we have it cached
  sbILocalDatabaseResourcePropertyBag* bag;
  if (mRowCache.Get(row, &bag)) {
    rv = bag->GetProperty(bind, _retval);
    if(rv == NS_ERROR_ILLEGAL_VALUE) {
      _retval.Assign(EmptyString());
    }
    else {
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return NS_OK;
  }

  PageCacheStatus status;
  rv = GetPageCachedStatus(row, &status);
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
      // clobber it and reset its page's cache status
      if (mGetByIndexAsyncPending) {

        if (mNextGetByIndexAsync > -1) {
          rv = SetPageCachedStatus(mNextGetByIndexAsync, eNotCached);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        mNextGetByIndexAsync = row;
        rv = SetPageCachedStatus(row, ePending);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        rv = SetPageCachedStatus(row, ePending);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mArray->GetByIndexAsync(row);
        NS_ENSURE_SUCCESS(rv, rv);
        mGetByIndexAsyncPending = PR_TRUE;
      }
      _retval.Assign(EmptyString());
    }
    break;

    // The page this row is in is pending, so just return a blank
    case ePending:
    {
      _retval.Assign(EmptyString());
    }
    break;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetSelection(nsITreeSelection** aSelection)
{
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
  NS_ENSURE_ARG_POINTER(aSelection);

  mSelection = aSelection;
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

  if (bind.Equals(NS_LITERAL_STRING("row"))) {
    return NS_OK;
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
sbLocalDatabaseTreeView::GetRowProperties(PRInt32 index,
                                          nsISupportsArray* properties)
{
  NS_ENSURE_ARG_POINTER(properties);

  // XXX: TODO
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetCellProperties(PRInt32 row,
                                           nsITreeColumn* col,
                                           nsISupportsArray* properties)
{
  NS_ENSURE_ARG_POINTER(col);
  NS_ENSURE_ARG_POINTER(properties);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetColumnProperties(nsITreeColumn* col,
                                             nsISupportsArray* properties)
{
  NS_ENSURE_ARG_POINTER(col);
  NS_ENSURE_ARG_POINTER(properties);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsContainer(PRInt32 index, PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsContainerOpen(PRInt32 index, PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsContainerEmpty(PRInt32 index, PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsSeparator(PRInt32 index, PRBool* _retval)
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
sbLocalDatabaseTreeView::CanDrop(PRInt32 index,
                                 PRInt32 orientation,
                                 PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::Drop(PRInt32 row, PRInt32 orientation)
{
  return NS_ERROR_NOT_IMPLEMENTED;
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
sbLocalDatabaseTreeView::GetLevel(PRInt32 index, PRInt32* _retval)
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

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetProgressMode(PRInt32 row,
                                         nsITreeColumn* col,
                                         PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(col);
  NS_ENSURE_ARG_POINTER(_retval);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetCellValue(PRInt32 row,
                                      nsITreeColumn* col,
                                      nsAString& _retval)
{
  NS_ENSURE_ARG_POINTER(col);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::SetTree(nsITreeBoxObject *tree)
{
  NS_ENSURE_ARG_POINTER(tree);

  mTreeBoxObject = tree;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::ToggleOpenState(PRInt32 index)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::SelectionChanged()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsEditable(PRInt32 row,
                                    nsITreeColumn* col,
                                    PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(col);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsAutoString bind;
  rv = GetPropertyForTreeColumn(col, bind);
  NS_ENSURE_SUCCESS(rv, rv);

  if (bind.Equals(NS_LITERAL_STRING("row"))) {
    *_retval = PR_FALSE;
  }
  else {
    // TODO: Talk to properties registry to find out of this should be
    // editable
    *_retval = PR_TRUE;
  }

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

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::SetCellText(PRInt32 row,
                                     nsITreeColumn* col,
                                     const nsAString& value)
{
  NS_ENSURE_ARG_POINTER(col);

  return NS_ERROR_NOT_IMPLEMENTED;
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
 * See sbILocalDatabaseTreeView
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
    if ((lastRow == index - 1) || (index == aStartFrom)) {
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
  rv = mArray->GetByIndexAsync(index);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "GetByIndexAsync");

  *_retval = (PRInt32)index;
  return NS_OK;
}
