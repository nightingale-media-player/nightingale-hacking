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

#include "sbSelectionListUtils.h"
#include "sbFilterTreeSelection.h"
#include "sbLocalDatabaseTreeView.h"

NS_IMPL_ISUPPORTS1(sbFilterTreeSelection,
                   nsITreeSelection)

sbFilterTreeSelection::sbFilterTreeSelection(nsITreeSelection* aTreeSelection,
                                             sbLocalDatabaseTreeView* aTreeView) :
  mTreeSelection(aTreeSelection),
  mTreeView(aTreeView)
{
  NS_ASSERTION(aTreeSelection, "Null passed to ctor");
}

NS_IMETHODIMP
sbFilterTreeSelection::GetTree(nsITreeBoxObject** aTree)
{
  return mTreeSelection->GetTree(aTree);
}
NS_IMETHODIMP
sbFilterTreeSelection::SetTree(nsITreeBoxObject* aTree)
{
  return mTreeSelection->SetTree(aTree);
}

NS_IMETHODIMP
sbFilterTreeSelection::GetSingle(PRBool* aSingle)
{
  return mTreeSelection->GetSingle(aSingle);
}

NS_IMETHODIMP
sbFilterTreeSelection::GetCount(PRInt32* aCount)
{
  return mTreeSelection->GetCount(aCount);
}

NS_IMETHODIMP
sbFilterTreeSelection::IsSelected(PRInt32 index, PRBool* _retval)
{
  return mTreeSelection->IsSelected(index, _retval);
}

NS_IMETHODIMP
sbFilterTreeSelection::Select(PRInt32 index)
{
  sbAutoSuppressSelectionEvents suppress(mTreeSelection);

  nsresult rv = mTreeSelection->Select(index);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CheckIsSelectAll();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbFilterTreeSelection::TimedSelect(PRInt32 index, PRInt32 delay)
{
  nsresult rv =  mTreeSelection->TimedSelect(index, delay);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CheckIsSelectAll();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbFilterTreeSelection::ToggleSelect(PRInt32 index)
{
  sbAutoSuppressSelectionEvents suppress(mTreeSelection);

  nsresult rv =  mTreeSelection->ToggleSelect(index);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CheckIsSelectAll();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbFilterTreeSelection::RangedSelect(PRInt32 startIndex,
                                    PRInt32 endIndex,
                                    PRBool augment)
{
  sbAutoSuppressSelectionEvents suppress(mTreeSelection);

  nsresult rv = mTreeSelection->RangedSelect(startIndex, endIndex, augment);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CheckIsSelectAll();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbFilterTreeSelection::ClearRange(PRInt32 startIndex,
                                  PRInt32 endIndex)
{
  sbAutoSuppressSelectionEvents suppress(mTreeSelection);

  nsresult rv =  mTreeSelection->ClearRange(startIndex, endIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CheckIsSelectAll();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbFilterTreeSelection::ClearSelection()
{
  sbAutoSuppressSelectionEvents suppress(mTreeSelection);

  nsresult rv =  mTreeSelection->ClearSelection();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CheckIsSelectAll();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbFilterTreeSelection::InvertSelection()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbFilterTreeSelection::SelectAll()
{
  sbAutoSuppressSelectionEvents suppress(mTreeSelection);

  nsresult rv =  mTreeSelection->Select(0);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CheckIsSelectAll();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbFilterTreeSelection::GetRangeCount(PRInt32* _retval)
{
  return mTreeSelection->GetRangeCount(_retval);
}

NS_IMETHODIMP
sbFilterTreeSelection::GetRangeAt(PRInt32 i,
                                  PRInt32* min,
                                  PRInt32* max)
{
  return mTreeSelection->GetRangeAt(i, min, max);
}

NS_IMETHODIMP
sbFilterTreeSelection::InvalidateSelection()
{
  return mTreeSelection->InvalidateSelection();
}

NS_IMETHODIMP
sbFilterTreeSelection::AdjustSelection(PRInt32 index, PRInt32 count)
{
  nsresult rv;

  sbAutoSuppressSelectionEvents suppress(mTreeSelection);

  // if index is 0 or count is -1, there is no need to pass
  // the call to the selection object
  if (index != 0 && count != -1) {
    rv =  mTreeSelection->AdjustSelection(index, count);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = CheckIsSelectAll();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbFilterTreeSelection::GetSelectEventsSuppressed(PRBool* aSelectEventsSuppressed)
{
  return mTreeSelection->GetSelectEventsSuppressed(aSelectEventsSuppressed);
}
NS_IMETHODIMP
sbFilterTreeSelection::SetSelectEventsSuppressed(PRBool aSelectEventsSuppressed)
{
  return mTreeSelection->SetSelectEventsSuppressed(aSelectEventsSuppressed);
}

NS_IMETHODIMP
sbFilterTreeSelection::GetCurrentIndex(PRInt32* aCurrentIndex)
{
  return mTreeSelection->GetCurrentIndex(aCurrentIndex);
}
NS_IMETHODIMP
sbFilterTreeSelection::SetCurrentIndex(PRInt32 aCurrentIndex)
{
  return mTreeSelection->SetCurrentIndex(aCurrentIndex);
}

NS_IMETHODIMP
sbFilterTreeSelection::GetCurrentColumn(nsITreeColumn** aCurrentColumn)
{
  return mTreeSelection->GetCurrentColumn(aCurrentColumn);
}
NS_IMETHODIMP
sbFilterTreeSelection::SetCurrentColumn(nsITreeColumn* aCurrentColumn)
{
  return mTreeSelection->SetCurrentColumn(aCurrentColumn);
}

NS_IMETHODIMP
sbFilterTreeSelection::GetShiftSelectPivot(PRInt32* aShiftSelectPivot)
{
  return mTreeSelection->GetShiftSelectPivot(aShiftSelectPivot);
}

nsresult
sbFilterTreeSelection::CheckIsSelectAll()
{
  nsresult rv;

  PRInt32 rowCount;
  rv = mTreeView->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 selectedCount;
  rv = mTreeSelection->GetCount(&selectedCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isAllRowSelected;
  rv = mTreeSelection->IsSelected(0, &isAllRowSelected);
  NS_ENSURE_SUCCESS(rv, rv);

  // We are select all if:
  // - The "all" row is selected
  // - If nothing is selected
  // - All rows are selected
  // Note that having all rows selected except for the "all" row is not
  // considered an "all" selection because it does not include items that
  // have no value for the filtered property.
  PRBool isSelectAll =
    isAllRowSelected ||
    selectedCount == 0 ||
    selectedCount == rowCount;

  if (isSelectAll) {
    // Note that all callers should have supressed selection
    rv = mTreeSelection->Select(0);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mTreeView->SetSelectionIsAll(isSelectAll);

  return NS_OK;
}
