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

#include "sbLocalDatabaseTreeView.h"
#include "sbPlaylistTreeSelection.h"
#include "sbFilterTreeSelection.h"

#include <nsIAtom.h>
#include <nsIAtomService.h>
#include <nsIClassInfoImpl.h>
#include <nsIDOMElement.h>
#include <nsIObjectOutputStream.h>
#include <nsIObjectInputStream.h>
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
#include <sbILibraryConstraints.h>
#include <sbIMediacoreEvent.h>
#include <sbIMediacoreEventTarget.h>
#include <sbIMediacoreManager.h>
#include <sbIMediacoreSequencer.h>
#include <sbIMediacoreStatus.h>
#include <sbIMediaListView.h>
#include <sbIMediaList.h>
#include <sbIMediaItem.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <sbIPropertyUnitConverter.h>
#include <sbISortableMediaListView.h>
#include <sbITreeViewPropertyInfo.h>

#include <nsComponentManagerUtils.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsUnicharUtils.h>
#include <nsCoord.h>
#include <prlog.h>
#include "sbLocalDatabasePropertyCache.h"
#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseGUIDArray.h"
#include "sbLocalDatabaseMediaItem.h"
#include "sbLocalDatabaseMediaListView.h"
#include <sbLibraryCID.h>
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
 */

class sbAutoUpdateBatch
{
public:
  sbAutoUpdateBatch(nsITreeBoxObject* aTreeBoxObject) :
    mTreeBoxObject(aTreeBoxObject)
  {
    NS_ASSERTION(aTreeBoxObject, "aTreeBoxObject is null");
#ifdef DEBUG
    nsresult rv =
#endif
    mTreeBoxObject->BeginUpdateBatch();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to begin");
  }

  ~sbAutoUpdateBatch()
  {
#ifdef DEBUG
    nsresult rv =
#endif
    mTreeBoxObject->EndUpdateBatch();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to end");
  }

private:
  nsITreeBoxObject* mTreeBoxObject;
};

/* static */ nsresult PR_CALLBACK
sbLocalDatabaseTreeView::SelectionListSavingEnumeratorCallback(PRUint32 aIndex,
                                                               const nsAString& aId,
                                                               const nsAString& aGuid,
                                                               void* aUserData)
{
  NS_ENSURE_ARG_POINTER(aUserData);

  sbSelectionList* list = static_cast<sbSelectionList*>(aUserData);
  NS_ENSURE_STATE(list);

  nsString guid(aGuid);
  PRBool success = list->Put(aId, guid);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

/* static */ nsresult PR_CALLBACK
sbLocalDatabaseTreeView::SelectionListGuidsEnumeratorCallback(PRUint32 aIndex,
                                                              const nsAString& aId,
                                                              const nsAString& aGuid,
                                                              void* aUserData)
{
  NS_ENSURE_ARG_POINTER(aUserData);

  nsTArray<nsString>* list = static_cast<nsTArray<nsString>*>(aUserData);
  NS_ENSURE_STATE(list);

  nsString* appended = list->AppendElement(aGuid);
  NS_ENSURE_TRUE(appended, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMPL_ISUPPORTS8(sbLocalDatabaseTreeView,
                   nsIClassInfo,
                   nsISupportsWeakReference,
                   nsITreeView,
                   sbILocalDatabaseGUIDArrayListener,
                   sbILocalDatabaseTreeView,
                   sbIMediaListViewTreeView,
                   sbIMediacoreEventListener,
                   sbIMediaListViewSelectionListener)

NS_IMPL_CI_INTERFACE_GETTER7(sbLocalDatabaseTreeView,
                             nsIClassInfo,
                             nsITreeView,
                             sbILocalDatabaseGUIDArrayListener,
                             sbILocalDatabaseTreeView,
                             sbIMediaListViewTreeView,
                             sbIMediacoreEventListener,
                             sbIMediaListViewSelectionListener)

sbLocalDatabaseTreeView::sbLocalDatabaseTreeView() :
 mListType(eLibrary),
 mMediaListView(nsnull),
 mViewSelection(nsnull),
 mArrayLength(0),
 mManageSelection(PR_FALSE),
 mHaveSavedSelection(PR_FALSE),
 mMouseState(sbILocalDatabaseTreeView::MOUSE_STATE_NONE),
 mMouseStateRow(-1),
 mSelectionIsAll(PR_FALSE),
 mFakeAllRow(PR_FALSE),
 mIsListeningToPlayback(PR_FALSE),
 mFirstCachedRow(NOT_SET),
 mLastCachedRow(NOT_SET)
{
#ifdef PR_LOGGING
  if (!gLocalDatabaseTreeViewLog) {
    gLocalDatabaseTreeViewLog = PR_NewLogModule("sbLocalDatabaseTreeView");
  }
#endif
}

sbLocalDatabaseTreeView::~sbLocalDatabaseTreeView()
{
  nsresult rv;

  if (mViewSelection) {
    nsCOMPtr<sbIMediaListViewSelectionListener> selectionListener =
      do_QueryInterface(NS_ISUPPORTS_CAST(sbILocalDatabaseTreeView*, this),
                        &rv);
    if (NS_SUCCEEDED(rv))
      mViewSelection->RemoveListener(selectionListener);
  }

  NS_ASSERTION(!mIsListeningToPlayback, "Still listening when dtor called");
  // declare ourselves dead, in case the async listener is actually in the
  // process of waiting to fire. yay.
  ClearWeakReferences();
}

nsresult
sbLocalDatabaseTreeView::Init(sbLocalDatabaseMediaListView* aMediaListView,
                              sbILocalDatabaseGUIDArray* aArray,
                              sbIPropertyArray* aCurrentSort,
                              sbLocalDatabaseTreeViewState* aState)
{
  NS_ENSURE_ARG_POINTER(aMediaListView);
  NS_ENSURE_ARG_POINTER(aArray);

  nsresult rv;

  // Make sure we actually are getting a sort or a state
  if (aCurrentSort) {
    NS_ENSURE_TRUE(!aState, NS_ERROR_INVALID_ARG);
    PRUint32 arrayLength;
    rv = aCurrentSort->GetLength(&arrayLength);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_STATE(arrayLength);
  }
  else {
    NS_ENSURE_ARG_POINTER(aState);
  }

  mPropMan = do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mMediaListView = aMediaListView;
  mArray = aArray;

  // Determine the list type
  PRBool isDistinct;
  rv = mArray->GetIsDistinct(&isDistinct);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isDistinct) {
    mListType = eDistinct;
    mManageSelection = PR_TRUE;
    mFakeAllRow = PR_TRUE;
    mSelectionIsAll = PR_TRUE;
  }
  else {
    mManageSelection = PR_FALSE;
    nsString baseTable;
    rv = mArray->GetBaseTable(baseTable);
    NS_ENSURE_SUCCESS(rv, rv);

    if (baseTable.EqualsLiteral("media_items")) {
      mListType = eLibrary;
    }
    else {
      if (baseTable.EqualsLiteral("simple_media_lists")) {
        mListType = eSimple;
      }
      else {
        return NS_ERROR_UNEXPECTED;
      }
    }
  }

  // If we are not managing our selection, we rely on our view for selection
  // management.  Add a listener to the view's selection so we can track
  // changes
  if (!mManageSelection) {
    nsCOMPtr<sbIMediaListViewSelection> viewSelection;
    rv = mMediaListView->GetSelection(getter_AddRefs(viewSelection));
    NS_ENSURE_TRUE(viewSelection, NS_ERROR_UNEXPECTED);
    mViewSelection = viewSelection;

    nsCOMPtr<sbIMediaListViewSelectionListener> selectionListener =
      do_QueryInterface(NS_ISUPPORTS_CAST(sbILocalDatabaseTreeView*, this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mViewSelection->AddListener(selectionListener);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mArray->GetPropertyCache(getter_AddRefs(mPropertyCache));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabaseGUIDArrayListener> listener =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbILocalDatabaseTreeView*, this), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->SetListener(listener);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->GetFetchSize(&mFetchSize);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mSelectionList.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  mHaveSavedSelection = PR_FALSE;

  if (aState) {
#ifdef DEBUG
  {
    nsString buff;
    aState->ToString(buff);
    LOG(("sbLocalDatabaseTreeView::Init[0x%.8x] - restoring %s",
          this, NS_LossyConvertUTF16toASCII(buff).get()));
  }
#endif
    rv = aState->mSort->GetProperty(mCurrentSortProperty);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool isAscending;
    rv = aState->mSort->GetIsAscending(&isAscending);
    NS_ENSURE_SUCCESS(rv, rv);
    mCurrentSortDirectionIsAscending = isAscending;

    if (mManageSelection) {
      mSelectionIsAll = aState->mSelectionIsAll;
      if (!mSelectionIsAll) {
        aState->mSelectionList.EnumerateRead(SB_CopySelectionListCallback,
                                             &mSelectionList);
        mHaveSavedSelection = PR_TRUE;
      }
    }
  }
  else {
    // Grab the top level sort property from the bag
    nsCOMPtr<sbIProperty> property;
    rv = aCurrentSort->GetPropertyAt(0, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetId(mCurrentSortProperty);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString value;
    rv = property->GetValue(value);
    NS_ENSURE_SUCCESS(rv, rv);

    mCurrentSortDirectionIsAscending = value.EqualsLiteral("a");
  }

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

  // If this is not a distinct list, we will want a listener to track playback
  // changes.  This listener should be set up in SetTree.
  if (mListType != eDistinct) {
    nsCOMPtr<nsISupportsWeakReference> weakRef =
      do_GetService("@songbirdnest.com/Songbird/Mediacore/Manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = weakRef->GetWeakReference(getter_AddRefs(mMediacoreManager));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::Rebuild()
{
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - Rebuild()", this));

  nsresult rv;

  // Check to see if the sort of the underlying array has changed
  nsCOMPtr<sbIPropertyArray> sort;
  rv = mArray->GetCurrentSort(getter_AddRefs(sort));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIProperty> property;
  rv = sort->GetPropertyAt(0, getter_AddRefs(property));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString arraySortProperty;
  rv = property->GetId(arraySortProperty);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool arraySortDirectionIsAscending;
  nsString direction;
  rv = property->GetValue(direction);
  NS_ENSURE_SUCCESS(rv, rv);
  arraySortDirectionIsAscending = direction.EqualsLiteral("a");

  if (!arraySortProperty.Equals(mCurrentSortProperty) ||
      arraySortDirectionIsAscending != mCurrentSortDirectionIsAscending) {

    mCurrentSortProperty = arraySortProperty;
    mCurrentSortDirectionIsAscending = arraySortDirectionIsAscending;

    rv = UpdateColumnSortAttributes(arraySortProperty,
                                    arraySortDirectionIsAscending);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Update the number of rows in the tree widget
  PRInt32 oldRowCount;
  rv = GetRowCount(&oldRowCount);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mArray->GetLength(&mArrayLength);
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt32 newRowCount;
  rv = GetRowCount(&newRowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mTreeBoxObject) {
    // Change the number of rows in the tree by the difference between the
    // new row count and the old cached row count.  We still need to invalidate
    // the whole tree since we don't know where the row changes took place.
    PRInt32 delta = newRowCount - oldRowCount;

    sbAutoUpdateBatch autoBatch(mTreeBoxObject);
    if (delta != 0) {
      rv = mTreeBoxObject->RowCountChanged(oldRowCount, delta);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    rv = mTreeBoxObject->Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // If we're managing the selection, restore it after adjusting the tree box
  // row count
  if (mManageSelection) {
    RestoreSelection();
  }

  return NS_OK;
}

void
sbLocalDatabaseTreeView::ClearMediaListView()
{
  mMediaListView = nsnull;
  mViewSelection = nsnull;
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

    nsString token(Substring(firstChar, current));

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

inline
PRBool intersection(PRInt32 start1, PRInt32 end1, PRInt32 start2, PRInt32 end2, PRInt32 & intersectStart, PRInt32 & intersectEnd) {
  PRBool const result = end1 >= start2 && start1 <= end2;
  if (result) {
    intersectStart = start2;
    intersectEnd = end2;
    if (start1 > start2)
      intersectStart = start1;
    if (end1 < end2)
      intersectEnd = end1;

  }
  return result;
}

nsresult
sbLocalDatabaseTreeView::GetCellPropertyValue(PRInt32 aIndex,
                                              nsITreeColumn *aTreeColumn,
                                              nsAString& _retval)
{
  NS_ENSURE_ARG_POINTER(aTreeColumn);

  nsresult rv;

  //////////////////////////////////////////////////////////////////////////
  // WARNING: This method is called during Paint. DO NOT MODIFY THE TREE, //
  // cause events to be fired, or use synchronous proxies, as you risk    //
  // crashing in recursive painting/frame construction.                   //
  //////////////////////////////////////////////////////////////////////////

  nsString bind;
  rv = GetPropertyForTreeColumn(aTreeColumn, bind);
  NS_ENSURE_SUCCESS(rv, rv);

  // If this is an ordinal column, return just the row number
  if (bind.EqualsLiteral(SB_PROPERTY_ORDINAL)) {
    _retval.AppendInt(aIndex + 1);
    return NS_OK;
  }

  // Explicitly cache all of the properties of all the visible rows
  if (mTreeBoxObject) {
    PRInt32 first;
    PRInt32 last;
    rv = mTreeBoxObject->GetFirstVisibleRow(&first);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mTreeBoxObject->GetLastVisibleRow(&last);
    NS_ENSURE_SUCCESS(rv, rv);

    // If we have a valid first/last and they're different
    if (first >= 0 && last >= 0) {

      // Calculate the number of rows we're going process
      PRInt32 length = last - first + 1;

      if (mFirstCachedRow != NOT_SET && mFirstCachedRow > first) {
        // User scrolled up, make sure to cache at least a page
        first = std::max(std::min(first, mFirstCachedRow - length + 1),
                         0);
      }
      if (mLastCachedRow != NOT_SET && mLastCachedRow < last) {
        // User scrolled down, make sure to cache at least a page
        last = std::min(std::max(last, mLastCachedRow + length - 1),
                        (PRInt32)mArrayLength + (mFakeAllRow ? 1 : 0) - 1);
      }
      length = last - first + 1;

      PRInt32 intersectStart;
      PRInt32 intersectEnd;
      // Get the intersection between the current first and last and previous
      PRBool const intersects = intersection(first, last,
                                             mFirstCachedRow, mLastCachedRow,
                                             intersectStart, intersectEnd);
      // If they intersect then remove the intersecting rows from the length
      if (intersects) {
        length -= (intersectEnd - intersectStart + 1);
      }
      if (length > 0) {
        // Set the capacity
        mGuidWorkArray.SetCapacity(length);
        mGuidWorkArray.Reset();
        for (PRInt32 row = first;
             row <= last && static_cast<PRUint32>(row) < mArrayLength;
             row++) {

          // Skip the fake row and any row's we have already cached
          if ((row >= mFirstCachedRow && row <= mLastCachedRow) ||
              (mFakeAllRow && row == 0)) {
            continue;
          }

          nsString guid;
          rv = mArray->GetGuidByIndex(TreeToArray(row), guid);
          NS_ENSURE_SUCCESS(rv, rv);

          rv = mGuidWorkArray.Append(guid);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        rv = mPropertyCache->CacheProperties(const_cast<PRUnichar const **>(
                                               mGuidWorkArray.AsCharArray()),
                                             mGuidWorkArray.Length());
        NS_ENSURE_SUCCESS(rv, rv);

        mFirstCachedRow = first;
        mLastCachedRow = last;
      }
    }
  }

  nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
  rv = GetBag(aIndex, getter_AddRefs(bag));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString value;
  rv = bag->GetProperty(bind, value);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the property info
  nsCOMPtr<sbIPropertyInfo> info;
  rv = mPropMan->GetPropertyInfo(bind, getter_AddRefs(info));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the unit converter
  nsCOMPtr<sbIPropertyUnitConverter> converter;
  rv = info->GetUnitConverter(getter_AddRefs(converter));
  NS_ENSURE_SUCCESS(rv, rv);

  // Format the value for display
  if (converter) {
    // unit converter present, ask it to format the value using the best unit,
    // using at most 1 decimal digit
    rv = converter->AutoFormat(value,
                               -1, // no min decimals
                               1,  // 1 decimal max
                               _retval);
  } else {
    // no unit converter, just use propertyinfo.format
    rv = info->Format(value, _retval);
  }

  if (NS_FAILED(rv)) {
    _retval.Truncate();
  }

  return NS_OK;
}


nsresult
sbLocalDatabaseTreeView::SaveSelectionList()
{
  NS_ASSERTION(mManageSelection,
               "SaveSelectionList() called but we're not managing selection");

  // Do nothing if selection already saved
  if (mHaveSavedSelection)
    return NS_OK;

  // If the current selection is everything, don't save anything
  if (mSelectionIsAll) {
    return NS_OK;
  }

  nsresult rv = EnumerateSelection(SelectionListSavingEnumeratorCallback,
                                   &mSelectionList);

  NS_ENSURE_SUCCESS(rv, rv);

  mHaveSavedSelection = PR_TRUE;

  return NS_OK;
}


nsresult
sbLocalDatabaseTreeView::RestoreSelection()
{
  NS_ASSERTION
    (mManageSelection,
     "RestoreSelection() called but we're not managing selection");

  // If no selection is set, don't restore anything
  if (!mRealSelection) {
    return NS_OK;
  }

  nsresult rv;
  if (mSelectionIsAll) {
    rv = mRealSelection->Select(0);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (mHaveSavedSelection) {
    // Start with an empty selection
    rv = mRealSelection->ClearSelection();
    NS_ENSURE_SUCCESS(rv, rv);

    // Check each item to see if it needs to be selected.  Enumerating the
    // selection list can't be done because it's not possible to get the array
    // index from a unique ID.
    for (PRUint32 i = 0; i < mArrayLength; i++) {
      // Stop if nothing more to select
      if (!mSelectionList.Count()) {
        break;
      }

      // Get the unique ID for the current item
      nsString id;
      rv = GetUniqueIdForIndex(i, id);
      NS_ENSURE_SUCCESS(rv, rv);

      // Select item if it's in the selection list
      if (mSelectionList.Get(id, nsnull)) {
        // Don't apply this selection any more
        mSelectionList.Remove(id);

        // Select row in the real selection
        PRInt32 row = ArrayToTree(i);
        rv = mRealSelection->ToggleSelect(row);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    // Normally we have reselected everything, and therefore, mSelectionList is
    // empty. However if an item has either disappeared or been renamed, it'll
    // still be in the list at this point, and this will interfere with
    // GetSelectedValues (it'll think those values are still selected). So force
    // the array to be empty now.
    mSelectionList.Clear();
    mHaveSavedSelection = PR_FALSE;
  }

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

          nsString id;
          PRUint32 index = TreeToArray(j);
          rv = GetUniqueIdForIndex(index, id);
          NS_ENSURE_SUCCESS(rv, rv);

          nsString guid;
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

  aId.Truncate();

  // For distinct lists, the sort key works as a unique id
  if (mListType == eDistinct) {
    rv = mArray->GetSortPropertyValueByIndex(aIndex, aId);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    // For regular lists, the unique identifer is composed of the lists' guid
    // appended to the item's guid appeneded to the item's database rowid.
    nsCOMPtr<sbIMediaList> list;
    rv = mMediaListView->GetMediaList(getter_AddRefs(list));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString guid;
    rv = list->GetGuid(guid);
    NS_ENSURE_SUCCESS(rv, rv);
    aId.Append(guid);
    aId.Append('|');

    guid.Truncate();
    rv = mArray->GetGuidByIndex(aIndex, guid);
    NS_ENSURE_SUCCESS(rv, rv);
    aId.Append(guid);
    aId.Append('|');

    PRUint64 rowid;
    rv = mArray->GetRowidByIndex(aIndex, &rowid);
    NS_ENSURE_SUCCESS(rv, rv);
    AppendInt(aId, rowid);
  }

  return NS_OK;
}

void
sbLocalDatabaseTreeView::SetSelectionIsAll(PRBool aSelectionIsAll) {

  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - SetSelectionIsAll(%d)",
         this, aSelectionIsAll));
  NS_ASSERTION(mManageSelection,
               "SetSelectionIsAll() called but we're not managing selection");

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
  NS_ASSERTION(mManageSelection,
               "ClearSelectionList() called but we're not managing selection");

  mSelectionList.Clear();
  mHaveSavedSelection = PR_FALSE;
}

nsresult
sbLocalDatabaseTreeView::UpdateColumnSortAttributes(const nsAString& aProperty,
                                                    PRBool aDirection)
{
  nsresult rv;

  if (!mTreeBoxObject) {
    NS_WARNING("Unable to update column sort attributes, no box object!");
    return NS_OK;
  }

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

    nsString bind;
    rv = element->GetAttribute(NS_LITERAL_STRING("bind"), bind);
    NS_ENSURE_SUCCESS(rv, rv);

    if (bind.Equals(aProperty)) {
      rv = element->SetAttribute(NS_LITERAL_STRING("sortActive"),
                                 NS_LITERAL_STRING("true"));
      NS_ENSURE_SUCCESS(rv, rv);

      nsString direction;
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

nsresult
sbLocalDatabaseTreeView::GetState(sbLocalDatabaseTreeViewState** aState)
{
  NS_ENSURE_ARG_POINTER(aState);

  nsresult rv;

  nsRefPtr<sbLocalDatabaseTreeViewState> state =
    new sbLocalDatabaseTreeViewState();
  NS_ENSURE_TRUE(state, NS_ERROR_OUT_OF_MEMORY);

  rv = state->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  state->mSort = do_CreateInstance(SONGBIRD_LIBRARYSORT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = state->mSort->Init(mCurrentSortProperty,
                          mCurrentSortDirectionIsAscending);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mSelectionIsAll) {
    state->mSelectionIsAll = PR_TRUE;
  }
  else {
    mSelectionList.EnumerateRead(SB_CopySelectionListCallback,
                                 &state->mSelectionList);
    rv = EnumerateSelection(SelectionListSavingEnumeratorCallback,
                            &state->mSelectionList);
    NS_ENSURE_SUCCESS(rv, rv);
  }

#ifdef DEBUG
  {
    nsString buff;
    state->ToString(buff);
    LOG(("sbLocalDatabaseTreeView::GetState[0x%.8x] - returning %s",
          this, NS_LossyConvertUTF16toASCII(buff).get()));
  }
#endif

  state.forget(aState);
  return NS_OK;
}

// sbILocalDatabaseTreeView
NS_IMETHODIMP
sbLocalDatabaseTreeView::SetSort(const nsAString& aProperty, PRBool aDirection)
{
  LOG(("sbLocalDatabaseTreeView[0x%.8x] - SetSort(%s, %d)",
       this, NS_LossyConvertUTF16toASCII(aProperty).get(), aDirection));

  nsresult rv;

  nsCOMPtr<sbIMediaList> list;
  rv = mMediaListView->GetMediaList(getter_AddRefs(list));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString isSortable;
  rv = list->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ISSORTABLE), isSortable);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isSortable.Equals(NS_LITERAL_STRING("0"))) {
    // list is not sortable
    return NS_ERROR_FAILURE;
  }

  // If this the library, a sort on ordinal is replaced with a create date
  // sort since the library does not have an ordinal.
  nsString sortProperty;
  sortProperty = aProperty;
  if (mListType == eLibrary && aProperty.EqualsLiteral(SB_PROPERTY_ORDINAL)) {
    sortProperty.AssignLiteral(SB_PROPERTY_CREATED);
  }

  // If we are linked to a media list view, use its interfaces to manage
  // the sort
  if (mListType != eDistinct) {
    NS_ENSURE_STATE(mMediaListView);

    nsCOMPtr<sbIMutablePropertyArray> newSort =
      do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = newSort->SetStrict(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = newSort->AppendProperty(aProperty,
                                  aDirection ?
                                    NS_LITERAL_STRING("a") :
                                    NS_LITERAL_STRING("d"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbISortableMediaListView> sortable =
      do_QueryInterface(NS_ISUPPORTS_CAST(sbISortableMediaListView*, mMediaListView), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = sortable->SetSort(newSort);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = mArray->ClearSorts();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mArray->AddSort(sortProperty, aDirection);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mArray->Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = Rebuild();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mCurrentSortProperty = aProperty;
  mCurrentSortDirectionIsAscending = aDirection;

  rv = UpdateColumnSortAttributes(aProperty, aDirection);
  NS_ENSURE_SUCCESS(rv, rv);

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
        nsString guid;
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

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetSelectionIsAll(PRBool* aSelectionIsAll)
{
  NS_ENSURE_ARG_POINTER(aSelectionIsAll);
  NS_ASSERTION(mManageSelection,
               "GetSelectionIsAll() called but we're not managing selection");

  *aSelectionIsAll = mSelectionIsAll;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetSelectedValues(nsIStringEnumerator** aValues)
{
  NS_ENSURE_ARG_POINTER(aValues);
  NS_ASSERTION(mManageSelection,
               "GetSelectedValues() called but we're not managing selection");
  nsresult rv;

  if (mSelectionIsAll) {
    nsTArray<nsString> empty;
    nsCOMPtr<nsIStringEnumerator> enumerator =
      new sbTArrayStringEnumerator(&empty);
    NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

    enumerator.forget(aValues);
    return NS_OK;
  }

  nsTArray<nsString> guids;
  rv = EnumerateSelection(SelectionListGuidsEnumeratorCallback, &guids);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mSelectionList.EnumerateRead(SB_SelectionListGuidsToTArrayCallback,
                                    &guids);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length = guids.Length();
  nsTArray<nsString> values(length);
  for (PRUint32 i = 0; i < length; i++) {

    nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
    rv = GetBag(guids[i], getter_AddRefs(bag));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString value;
    rv = bag->GetProperty(mCurrentSortProperty, value);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString* appended = values.AppendElement(value);
    NS_ENSURE_TRUE(appended, NS_ERROR_OUT_OF_MEMORY);
  }

  nsCOMPtr<nsIStringEnumerator> enumerator =
    new sbTArrayStringEnumerator(&values);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

  enumerator.forget(aValues);
  return NS_OK;
}

// sbILocalDatabaseGUIDArrayListener
NS_IMETHODIMP
sbLocalDatabaseTreeView::OnBeforeInvalidate()
{
  // array modified so reset everything
  mGuidWorkArray.Reset();
  mLastCachedRow = mFirstCachedRow = NOT_SET;

  if (mManageSelection) {
    nsresult rv = SaveSelectionList();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::OnAfterInvalidate()
{
  nsresult rv;

  rv = Rebuild();
  NS_ENSURE_SUCCESS(rv, rv);

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

    nsString bind;
    rv = element->GetAttribute(NS_LITERAL_STRING("bind"), bind);
    NS_ENSURE_SUCCESS(rv, rv);

    if (bind.Equals(aProperty)) {
      column.forget(aTreeColumn);
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

/* inline */ nsresult
sbLocalDatabaseTreeView::GetColumnPropertyInfo(nsITreeColumn* aColumn,
                                               sbIPropertyInfo** aPropertyInfo)
{
  nsString propertyID;
  nsresult rv = GetPropertyForTreeColumn(aColumn, propertyID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropMan->GetPropertyInfo(propertyID, aPropertyInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::GetPropertyInfoAndValue(PRInt32 aRow,
                                                 nsITreeColumn* aColumn,
                                                 nsAString& aValue,
                                                 sbIPropertyInfo** aPropertyInfo)
{
  NS_ASSERTION(aColumn, "aColumn is null");
  NS_ASSERTION(aPropertyInfo, "aPropertyInfo is null");
  nsresult rv;

  PRUint32 index = TreeToArray(aRow);
  nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
  rv = GetBag(index, getter_AddRefs(bag));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyInfo> pi;
  rv = GetColumnPropertyInfo(aColumn, getter_AddRefs(pi));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString propertyID;
  rv = pi->GetId(propertyID);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString value;
  rv = bag->GetProperty(propertyID, aValue);
  NS_ENSURE_SUCCESS(rv, rv);

  pi.forget(aPropertyInfo);
  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::GetPlayingProperty(PRUint32 aIndex,
                                            nsISupportsArray* properties)
{
  NS_ASSERTION(properties, "properties is null");

  nsresult rv;

  //////////////////////////////////////////////////////////////////////////
  // WARNING: This method is called during Paint. DO NOT MODIFY THE TREE, //
  // cause events to be fired, or use synchronous proxies, as you risk    //
  // crashing in recursive painting/frame construction.                   //
  //////////////////////////////////////////////////////////////////////////

  if (!mPlayingItemUID.IsEmpty()) {
    nsString uid;
    rv = GetUniqueIdForIndex(aIndex, uid);
    NS_ENSURE_SUCCESS(rv, rv);

    if (mPlayingItemUID.Equals(uid)) {
      rv = TokenizeProperties(NS_LITERAL_STRING("playing"), properties);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::GetIsListReadOnly(PRBool *aOutIsReadOnly)
{
  NS_ENSURE_ARG_POINTER(aOutIsReadOnly);

  nsresult rv;

  //////////////////////////////////////////////////////////////////////////
  // WARNING: This method is called during Paint. DO NOT MODIFY THE TREE, //
  // cause events to be fired, or use synchronous proxies, as you risk    //
  // crashing in recursive painting/frame construction.                   //
  //////////////////////////////////////////////////////////////////////////

  nsCOMPtr<sbIMediaList> mediaList;
  rv = mMediaListView->GetMediaList(getter_AddRefs(mediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString isReadOnly;
  rv = mediaList->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ISREADONLY),
                              isReadOnly);
  NS_ENSURE_SUCCESS(rv, rv);

  *aOutIsReadOnly = isReadOnly.EqualsLiteral("1");
  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::GetBag(PRUint32 aIndex,
                                sbILocalDatabaseResourcePropertyBag** aBag)
{
  NS_ASSERTION(aBag, "aBag is null!");
  nsresult rv;

  nsString guid;
  rv = mArray->GetGuidByIndex(aIndex, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetBag(guid, aBag);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::GetBag(const nsAString& aGuid,
                                sbILocalDatabaseResourcePropertyBag** aBag)
{
  NS_ASSERTION(aBag, "aBag is null!");
  nsresult rv;

  const PRUnichar* guidptr = aGuid.BeginReading();

  PRUint32 count = 0;
  sbILocalDatabaseResourcePropertyBag** bags = nsnull;
  rv = mPropertyCache->GetProperties(&guidptr, 1, &count, &bags);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
  if (count == 1 && bags[0]) {
    bag = bags[0];
  }
  NS_FREE_XPCOM_ISUPPORTS_POINTER_ARRAY(count, bags);

  if (!bag) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  bag.forget(aBag);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetRowCount(PRInt32 *aRowCount)
{
  NS_ENSURE_ARG_POINTER(aRowCount);

  //////////////////////////////////////////////////////////////////////////
  // WARNING: This method is called during Paint. DO NOT MODIFY THE TREE, //
  // cause events to be fired, or use synchronous proxies, as you risk    //
  // crashing in recursive painting/frame construction.                   //
  //////////////////////////////////////////////////////////////////////////

  *aRowCount = mArrayLength + (mFakeAllRow ? 1 : 0);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetCellText(PRInt32 row,
                                     nsITreeColumn *col,
                                     nsAString& _retval)
{
  NS_ENSURE_ARG_POINTER(col);

  nsresult rv;

  //////////////////////////////////////////////////////////////////////////
  // WARNING: This method is called during Paint. DO NOT MODIFY THE TREE, //
  // cause events to be fired, or use synchronous proxies, as you risk    //
  // crashing in recursive painting/frame construction.                   //
  //////////////////////////////////////////////////////////////////////////

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

  // this matches the MediaListView
  NS_ENSURE_ARG_POINTER(aSelection);
  NS_IF_ADDREF(*aSelection = mSelection);
  return NS_OK;

}
NS_IMETHODIMP
sbLocalDatabaseTreeView::SetSelection(nsITreeSelection* aSelection)
{
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - SetSelection(0x%.8x)", this, aSelection));

  NS_ENSURE_ARG_POINTER(aSelection);

  // Wrap the selection given to us with our own proxy implementation so
  // we can get useful notifications on how the selection is changing
  if (mListType == eDistinct) {
    mSelection = new sbFilterTreeSelection(aSelection, this);
    NS_ENSURE_TRUE(mSelection, NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    NS_ASSERTION(mViewSelection, "SetSelection called but no mViewSelection");
    mSelection = new sbPlaylistTreeSelection(aSelection, mViewSelection, this);
    NS_ENSURE_TRUE(mSelection, NS_ERROR_OUT_OF_MEMORY);
  }

  // Keep a ref to the real selection so we can modify it without going
  // through our proxy
  mRealSelection = aSelection;

  // If we're managing the selection, restore it
  if (mManageSelection) {
    RestoreSelection();
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::CycleHeader(nsITreeColumn* col)
{
  NS_ENSURE_ARG_POINTER(col);
  nsresult rv;

  nsCOMPtr<sbIMediaList> mediaList;
  rv = mMediaListView->GetMediaList(getter_AddRefs(mediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  // If the list is not sortable, ignore the click
  nsString isSortable;
  rv = mediaList->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ISSORTABLE),
                              isSortable);
  NS_ENSURE_SUCCESS(rv, rv);
  if (isSortable.EqualsLiteral("0")) {
    return NS_OK;
  }

  nsString bind;
  rv = GetPropertyForTreeColumn(col, bind);
  NS_ENSURE_SUCCESS(rv, rv);

  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - CycleHeader %s", this,
         NS_LossyConvertUTF16toASCII(bind).get()));

  PRBool directionIsAscending = PR_TRUE;
  if (bind.Equals(mCurrentSortProperty)) {
    directionIsAscending = !mCurrentSortDirectionIsAscending;
  }

  rv = SetSort(bind, directionIsAscending);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mObserver) {
    nsCOMPtr<sbIMediaListViewTreeViewObserver> observer =
      do_QueryReferent(mObserver);
    if (observer) {
      rv = observer->CycleHeader(col);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

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

  PRUint32 index = TreeToArray(row);

  //////////////////////////////////////////////////////////////////////////
  // WARNING: This method is called during Paint. DO NOT MODIFY THE TREE, //
  // cause events to be fired, or use synchronous proxies, as you risk    //
  // crashing in recursive painting/frame construction.                   //
  //////////////////////////////////////////////////////////////////////////

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

  rv = GetPlayingProperty(index, properties);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
  rv = GetBag(index, getter_AddRefs(bag));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringEnumerator> propertyEnumerator;
  rv = mPropMan->GetPropertyIDs(getter_AddRefs(propertyEnumerator));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString propertyID;
  while (NS_SUCCEEDED(propertyEnumerator->GetNext(propertyID))) {

    nsString value;
    nsresult rv = bag->GetProperty(propertyID, value);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIPropertyInfo> propInfo;
    rv = mPropMan->GetPropertyInfo(propertyID, getter_AddRefs(propInfo));
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

  //////////////////////////////////////////////////////////////////////////
  // WARNING: This method is called during Paint. DO NOT MODIFY THE TREE, //
  // cause events to be fired, or use synchronous proxies, as you risk    //
  // crashing in recursive painting/frame construction.                   //
  //////////////////////////////////////////////////////////////////////////

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

  rv = GetPlayingProperty(TreeToArray(row), properties);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyInfo> pi;
  nsString value;
  rv = GetPropertyInfoAndValue(row, col, value, getter_AddRefs(pi));
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

  // If the current medialist is readonly, be set the "readonly" property.
  PRBool isMediaListReadOnly;
  rv = GetIsListReadOnly(&isMediaListReadOnly);
  if (NS_SUCCEEDED(rv) && isMediaListReadOnly) {
    rv = TokenizeProperties(NS_LITERAL_STRING("readonly"), properties);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetColumnProperties(nsITreeColumn* col,
                                             nsISupportsArray* properties)
{
  NS_ENSURE_ARG_POINTER(col);
  NS_ENSURE_ARG_POINTER(properties);

  nsString propertyID;
  nsresult rv = GetPropertyForTreeColumn(col, propertyID);
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

  for (PRUint32 index = 0; index < propertyID.Length(); index++) {
    PRUnichar testChar = propertyID.CharAt(index);

    // Short circuit for ASCII alphanumerics.
    if ((testChar >= 97 && testChar <= 122) || // a-z
        (testChar >= 65 && testChar <= 90) ||  // A-Z
        (testChar >= 48 && testChar <= 57)) {  // 0-9
      continue;
    }

    PRInt32 badCharIndex= badChars.FindChar(testChar);
    if (badCharIndex > -1) {
      if (index > 0 && propertyID.CharAt(index - 1) == kHyphenChar) {
        propertyID.Replace(index, 1, nsnull, 0);
        index--;
      }
      else {
        propertyID.Replace(index, 1, kHyphenChar);
      }
    }
  }

  rv = TokenizeProperties(propertyID, properties);
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

    nsCOMPtr<sbIMediaListViewTreeViewObserver> observer =
      do_QueryReferent(mObserver);
    if (observer) {
      nsresult rv = observer->CanDrop(TreeToArray(row),
                                      orientation,
                                      _retval);
      NS_ENSURE_SUCCESS(rv, rv);
    }
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

    nsCOMPtr<sbIMediaListViewTreeViewObserver> observer =
      do_QueryReferent(mObserver);
    if (observer) {
      nsresult rv = observer->Drop(TreeToArray(row), orientation);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetParentIndex(PRInt32 rowIndex, PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  // we never have parent indices, return -1 as specified in the IDL
  *_retval = -1;
  return NS_OK;
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

  *_retval = 0;
  return NS_OK;
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
  rv = GetPropertyInfoAndValue(row, col, value, getter_AddRefs(pi));
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
  rv = GetPropertyInfoAndValue(row, col, value, getter_AddRefs(pi));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbITreeViewPropertyInfo> tvpi = do_QueryInterface(pi, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = tvpi->GetProgressMode(value, _retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - GetMode(%d, %d) = %d", this,
         row, col, _retval));

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

  //////////////////////////////////////////////////////////////////////////
  // WARNING: This method is called during Paint. DO NOT MODIFY THE TREE, //
  // cause events to be fired, or use synchronous proxies, as you risk    //
  // crashing in recursive painting/frame construction.                   //
  //////////////////////////////////////////////////////////////////////////

  if (IsAllRow(row)) {
    _retval.Truncate();
    return NS_OK;
  }

  nsresult rv;

  nsCOMPtr<sbIPropertyInfo> pi;
  nsString value;
  rv = GetPropertyInfoAndValue(row, col, value, getter_AddRefs(pi));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbITreeViewPropertyInfo> tvpi = do_QueryInterface(pi, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = tvpi->GetCellValue(value, _retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - GetCellValue(%d, %d) = %s", this,
         row, colIndex, NS_LossyConvertUTF16toASCII(_retval).get()));

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::SetTree(nsITreeBoxObject *tree)
{
  TRACE(("sbLocalDatabaseTreeView[0x%.8x] - SetTree(0x%.8x)", this, tree));
  nsresult rv;

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
  // If we are detaching from the tree, save
  // the selection so that it can be
  // restored if we are ever rebound.
  else if (mManageSelection) {
    nsresult rv = SaveSelectionList();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Manage our listener to the playback service.  Attach the listener when
  // we are bound to a tree and remove it when we are unbound.  We do this
  // because we need to remove our listener when we go away, but we can't do
  // it in the destructor because we wind up creating a wrapper that eventually
  // points to a deleted this.
  if (mMediacoreManager) {
    nsCOMPtr<sbIMediacoreEventListener> eventListener =
      do_QueryInterface(NS_ISUPPORTS_CAST(sbIMediacoreEventListener*, this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediacoreEventTarget> target =
      do_QueryReferent(mMediacoreManager, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (tree) {
      if (!mIsListeningToPlayback) {

        rv = target->AddListener(eventListener);
        NS_ENSURE_SUCCESS(rv, rv);
        mIsListeningToPlayback = PR_TRUE;

        // Initialize the playing indicator
        rv = OnTrackChange();
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        NS_WARNING("Tree set but we're already listening to playback!");
      }
    }
    else {
      if (mIsListeningToPlayback) {
        rv = target->RemoveListener(eventListener);
        NS_ENSURE_SUCCESS(rv, rv);
        mIsListeningToPlayback = PR_FALSE;
      }
      else {
        NS_WARNING("Tree nulled but we're not listening to playback!");
      }
    }
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

  nsCOMPtr<sbIPropertyInfo> info;
  rv = GetColumnPropertyInfo(col, getter_AddRefs(info));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString bind;
  rv = GetPropertyForTreeColumn(col, bind);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString guid;
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

  nsString oldValue;
  rv = item->GetProperty(bind, oldValue);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!value.Equals(oldValue)) {
    rv = item->SetProperty(bind, value);
    // We need to handle the ILLEGAL_VALUE case specially or the
    // tree.xml::stopEditing() errors out and leaves the inline
    // editor showing on deselect.
    if (rv == NS_ERROR_ILLEGAL_VALUE) {
      // We do not notify any observers that the cell has been edited
      // in the event of illegal data, however, we don't want to throw
      // an error exception, so we'll throw this sorta-success in case
      // anyone cares.
      return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;
    }
    NS_ENSURE_SUCCESS(rv, rv);

    if (mObserver) {
      nsCOMPtr<sbIMediaListViewTreeViewObserver> observer =
        do_QueryReferent(mObserver);
      if (observer) {
        rv = observer->OnCellEdited(item, bind, oldValue);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
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

  nsString keyString(aKeyString);
  PRUint32 keyStringLength = keyString.Length();

  // Make sure that this is lowercased. The obj_searchable table should always be
  // lowercased.
  ToLowerCase(keyString);

  // Most folks will use this function to check the very next row in the tree.
  // Try that before doing the costly database search.
  nsString testString;
  nsresult rv = mArray->GetSortPropertyValueByIndex(aStartFrom, testString);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!keyString.Compare(Substring(testString, 0, keyStringLength))) {
    *_retval = aStartFrom;
    return NS_OK;
  }

  PRUint32 index;
  rv = mArray->GetFirstIndexByPrefix(keyString, &index);
  if (NS_FAILED(rv) || (index < aStartFrom)) {
    *_retval = -1;
    return NS_OK;
  }

  // If we have a fake all row, we have to add 1 to the index we return.
  if (mFakeAllRow) {
    index++;
  }

  *_retval = (PRInt32)index;
  return NS_OK;
}

/**
 * See sbIMediaListViewTreeView
 */
NS_IMETHODIMP
sbLocalDatabaseTreeView::SetObserver(sbIMediaListViewTreeViewObserver* aObserver)
{
  nsresult rv;

  if (aObserver) {
    mObserver = do_GetWeakReference(aObserver, &rv);;
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    mObserver = nsnull;
  }

  return NS_OK;
}

/**
 * See sbIMediaListViewTreeView
 */
NS_IMETHODIMP
sbLocalDatabaseTreeView::GetObserver(sbIMediaListViewTreeViewObserver** aObserver)
{
  NS_ENSURE_ARG_POINTER(aObserver);

  *aObserver = nsnull;
  if (mObserver) {
    nsCOMPtr<sbIMediaListViewTreeViewObserver> observer =
      do_QueryReferent(mObserver);
    if (observer) {
      observer.forget(aObserver);
    }
  }

  return NS_OK;
}

// -----------------------------------------------------------------------------
// sbIMediacoreEventListener
// -----------------------------------------------------------------------------
NS_IMETHODIMP
sbLocalDatabaseTreeView::OnMediacoreEvent(sbIMediacoreEvent *aEvent)
{
  NS_ENSURE_ARG_POINTER(aEvent);

  PRUint32 eventType = 0;
  nsresult rv = aEvent->GetType(&eventType);
  NS_ENSURE_SUCCESS(rv, rv);

  switch(eventType) {
    case sbIMediacoreEvent::STREAM_START:
    case sbIMediacoreEvent::TRACK_CHANGE:
    case sbIMediacoreEvent::TRACK_INDEX_CHANGE: {
      rv = OnTrackChange();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    case sbIMediacoreEvent::STREAM_END:
    case sbIMediacoreEvent::STREAM_STOP: {
      rv = OnStop();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;
  }

  return NS_OK;
}

// -----------------------------------------------------------------------------
// Event Listener Handlers
// -----------------------------------------------------------------------------
nsresult
sbLocalDatabaseTreeView::OnStop()
{
  nsresult rv;

  mPlayingItemUID = EmptyString();

  if (mTreeBoxObject) {
    rv = mTreeBoxObject->Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::OnTrackChange()
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<sbIMediacoreManager> manager =
    do_QueryReferent(mMediacoreManager, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediacoreSequencer> sequencer;
  rv = manager->GetSequencer(getter_AddRefs(sequencer));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaListView> playingView;
  rv = sequencer->GetView(getter_AddRefs(playingView));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 playingViewIndex = 0;
  rv = sequencer->GetViewPosition(&playingViewIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = OnTrackChange(playingView, playingViewIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::OnTrackChange(sbIMediaListView *aView,
                                       PRUint32 aIndex)
{
  nsresult rv;

  if (aView && mMediaListView) {
    nsCOMPtr<sbIMediaList> viewList;
    rv = mMediaListView->GetMediaList(getter_AddRefs(viewList));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaList> playingList;
    rv = aView->GetMediaList(getter_AddRefs(playingList));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool equals;
    rv = viewList->Equals(playingList, &equals);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediacoreManager> manager =
                                    do_QueryReferent(mMediacoreManager, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediacoreStatus> status;
    rv = manager->GetStatus(getter_AddRefs(status));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 state = 0;
    rv = status->GetState(&state);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool isPlaying = (state == sbIMediacoreStatus::STATUS_PLAYING ||
                        state == sbIMediacoreStatus::STATUS_PAUSED ||
                        state == sbIMediacoreStatus::STATUS_BUFFERING);

    if (equals && isPlaying) {
      nsString uid;
      rv = aView->GetViewItemUIDForIndex(aIndex, uid);
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 index;
      rv = mMediaListView->GetIndexForViewItemUID(uid, &index);
      if (NS_SUCCEEDED(rv)) {
        rv = GetUniqueIdForIndex(index, mPlayingItemUID);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        mPlayingItemUID = EmptyString();
      }
    }
    else {
      mPlayingItemUID = EmptyString();
    }
  }
  else {
    mPlayingItemUID = EmptyString();
  }

  if (mTreeBoxObject) {
    rv = mTreeBoxObject->Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeView::OnTrackIndexChange(sbIMediacoreEvent *aEvent)
{
  NS_ENSURE_ARG_POINTER(aEvent);

  // The index of the playing track has changed. This is either due to the
  // user moving the track around (in which case the UID of the item has not
  // changed) or to the whole content of the list being replaced (eg, a smart
  // playlist being rebuilt), in which case the currently playing item (if it
  // is still there) has a brand new UID. In both cases it is safe to just
  // forward the arguments to OnTrackChange in order to use the currently
  // playing item's UID.
  nsresult rv = OnTrackChange();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// sbIMediaListViewSelectionListener
NS_IMETHODIMP
sbLocalDatabaseTreeView::OnSelectionChanged()
{
  LOG(("sbLocalDatabaseTreeView[0x%.8x] - OnSelectionChanged", this));
  if (mListType != eDistinct && mTreeBoxObject && mRealSelection) {
    // Current index may have changed as well
    OnCurrentIndexChanged();
    nsresult rv = mTreeBoxObject->Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::OnCurrentIndexChanged()
{
  nsresult rv;
  PRInt32 currentIndex;

  if (mRealSelection && mViewSelection) {
    rv = mViewSelection->GetCurrentIndex(&currentIndex);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool treeIsSelected;
    rv = mRealSelection->IsSelected(currentIndex, &treeIsSelected);
    NS_ENSURE_SUCCESS(rv, rv);

    // If this row is not selected, do a toggle, which
    // will fire a select event and set the current index
    if (!treeIsSelected) {
      rv = mRealSelection->ToggleSelect(currentIndex);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      // Otherwise, already part of the selection, so just change current index
      rv = mRealSelection->SetCurrentIndex(currentIndex);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

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

NS_IMPL_ISUPPORTS1(sbLocalDatabaseTreeViewState,
                   nsISerializable);

sbLocalDatabaseTreeViewState::sbLocalDatabaseTreeViewState() :
  mSelectionIsAll(PR_FALSE)
{
}

NS_IMETHODIMP
sbLocalDatabaseTreeViewState::Read(nsIObjectInputStream* aStream)
{
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  rv = aStream->ReadObject(PR_TRUE, getter_AddRefs(mSort));
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
sbLocalDatabaseTreeViewState::Write(nsIObjectOutputStream* aStream)
{
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  rv = aStream->WriteObject(mSort, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aStream->Write32(mSelectionList.Count());
  NS_ENSURE_SUCCESS(rv, rv);

  mSelectionList.EnumerateRead(SB_SerializeSelectionListCallback, aStream);

  rv = aStream->WriteBoolean(mSelectionIsAll);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeViewState::Init()
{
  PRBool success = mSelectionList.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult
sbLocalDatabaseTreeViewState::ToString(nsAString& aStr)
{
  nsresult rv;
  nsString buff;
  nsString temp;

  rv = mSort->ToString(temp);
  NS_ENSURE_SUCCESS(rv, rv);

  buff.Assign(temp);

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
