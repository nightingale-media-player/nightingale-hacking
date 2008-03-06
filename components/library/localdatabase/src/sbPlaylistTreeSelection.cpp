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

#include "sbPlaylistTreeSelection.h"
#include <sbLocalDatabaseMediaListViewSelection.h>

NS_IMPL_ISUPPORTS1(sbPlaylistTreeSelection,
                   nsITreeSelection)

sbPlaylistTreeSelection::sbPlaylistTreeSelection(nsITreeSelection* aTreeSelection,
                                                 sbIMediaListViewSelection* aViewSelection,
                                                 sbLocalDatabaseTreeView* aTreeView) :
  mTreeSelection(aTreeSelection),
  mViewSelection(aViewSelection),
  mTreeView(aTreeView),
  mShiftSelectPivot(-1)
{
  NS_ASSERTION(aTreeSelection && aViewSelection, "Null passed to ctor");
}

NS_IMETHODIMP
sbPlaylistTreeSelection::GetTree(nsITreeBoxObject** aTree)
{
  return mTreeSelection->GetTree(aTree);
}
NS_IMETHODIMP
sbPlaylistTreeSelection::SetTree(nsITreeBoxObject* aTree)
{
  return mTreeSelection->SetTree(aTree);
}

NS_IMETHODIMP
sbPlaylistTreeSelection::GetSingle(PRBool* aSingle)
{
  return mTreeSelection->GetSingle(aSingle);
}

NS_IMETHODIMP
sbPlaylistTreeSelection::GetCount(PRInt32* aCount)
{
  return mViewSelection->GetCount(aCount);
}

NS_IMETHODIMP
sbPlaylistTreeSelection::IsSelected(PRInt32 index, PRBool* _retval)
{
  // The tree body frame uses IsSelected to determine if rows are selected.  If
  // we are in the middle of a row count requery, there is a brief point in
  // time where the tree body's idea of how many rows the tree has differs from
  // the number of rows in the view.  Return false when this is the case.
  if (mTreeView->mCachedRowCountDirty) {
    *_retval = PR_FALSE;
    return NS_OK;
  }
  return mViewSelection->IsIndexSelected(index, _retval);
}

NS_IMETHODIMP
sbPlaylistTreeSelection::Select(PRInt32 index)
{
  nsresult rv;
  mShiftSelectPivot = -1;

  return mViewSelection->SelectOnly(index);
}

NS_IMETHODIMP
sbPlaylistTreeSelection::TimedSelect(PRInt32 index, PRInt32 delay)
{
  nsresult rv;
  mShiftSelectPivot = -1;

  return mViewSelection->SelectOnly(index);
}

NS_IMETHODIMP
sbPlaylistTreeSelection::ToggleSelect(PRInt32 index)
{
  mShiftSelectPivot = -1;
  return mViewSelection->Toggle(index);
}

NS_IMETHODIMP
sbPlaylistTreeSelection::RangedSelect(PRInt32 startIndex,
                                      PRInt32 endIndex,
                                      PRBool augment)
{
  nsresult rv;
  sbAutoSelectNotificationsSuppressed autoSelection(mViewSelection);

  // Save the current index here since we need it later and it may be
  // changed if we need to clear the selection
  PRInt32 currentIndex;
  rv = mViewSelection->GetCurrentIndex(&currentIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!augment) {
    rv = mViewSelection->SelectNone();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (startIndex == -1) {
    if (mShiftSelectPivot != -1) {
      startIndex = mShiftSelectPivot;
    }
    else {
      if (currentIndex != -1) {
        startIndex = currentIndex;
      }
      else {
        startIndex = endIndex;
      }
    }
  }

  mShiftSelectPivot = startIndex;

  return mViewSelection->SelectRange(startIndex, endIndex);
}

NS_IMETHODIMP
sbPlaylistTreeSelection::ClearRange(PRInt32 startIndex,
                                    PRInt32 endIndex)
{
  mShiftSelectPivot = -1;
  return mViewSelection->ClearRange(startIndex, endIndex);
}

NS_IMETHODIMP
sbPlaylistTreeSelection::ClearSelection()
{
  mShiftSelectPivot = -1;
  return mViewSelection->SelectNone();
}

NS_IMETHODIMP
sbPlaylistTreeSelection::InvertSelection()
{
  // XXX not implemented by nsTreeSelection either
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbPlaylistTreeSelection::SelectAll()
{
  mShiftSelectPivot = -1;
  return mViewSelection->SelectAll();
}

NS_IMETHODIMP
sbPlaylistTreeSelection::GetRangeCount(PRInt32* _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbPlaylistTreeSelection::GetRangeAt(PRInt32 i,
                                    PRInt32* min,
                                    PRInt32* max)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbPlaylistTreeSelection::InvalidateSelection()
{
  return mTreeSelection->InvalidateSelection();
}

NS_IMETHODIMP
sbPlaylistTreeSelection::AdjustSelection(PRInt32 index, PRInt32 count)
{
  nsresult rv;
  PRInt32 currentIndex;
  rv = mViewSelection->GetCurrentIndex(&currentIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  // adjust current index, if necessary
  if ((currentIndex != -1) && (index <= currentIndex)) {
    // if we are deleting and the delete includes the current index, reset it
    if (count < 0 && (currentIndex <= (index - count -1))) {
        currentIndex = -1;
    }
    else {
        currentIndex += count;
    }

    rv = mViewSelection->SetCurrentIndex(currentIndex);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // adjust mShiftSelectPivot, if necessary
  if ((mShiftSelectPivot != 1) && (index <= mShiftSelectPivot)) {
    // if we are deleting and the delete includes the shift select pivot, reset it
    if (count < 0 && (mShiftSelectPivot <= (index - count - 1))) {
        mShiftSelectPivot = -1;
    }
    else {
        mShiftSelectPivot += count;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistTreeSelection::GetSelectEventsSuppressed(PRBool* aSelectEventsSuppressed)
{
  return mTreeSelection->GetSelectEventsSuppressed(aSelectEventsSuppressed);
}
NS_IMETHODIMP
sbPlaylistTreeSelection::SetSelectEventsSuppressed(PRBool aSelectEventsSuppressed)
{
  return mTreeSelection->SetSelectEventsSuppressed(aSelectEventsSuppressed);
}

NS_IMETHODIMP
sbPlaylistTreeSelection::GetCurrentIndex(PRInt32* aCurrentIndex)
{
  return mViewSelection->GetCurrentIndex(aCurrentIndex);
}
NS_IMETHODIMP
sbPlaylistTreeSelection::SetCurrentIndex(PRInt32 aCurrentIndex)
{
  return mViewSelection->SetCurrentIndex(aCurrentIndex);
}

NS_IMETHODIMP
sbPlaylistTreeSelection::GetCurrentColumn(nsITreeColumn** aCurrentColumn)
{
  return mTreeSelection->GetCurrentColumn(aCurrentColumn);
}
NS_IMETHODIMP
sbPlaylistTreeSelection::SetCurrentColumn(nsITreeColumn* aCurrentColumn)
{
  return mTreeSelection->SetCurrentColumn(aCurrentColumn);
}

NS_IMETHODIMP
sbPlaylistTreeSelection::GetShiftSelectPivot(PRInt32* aShiftSelectPivot)
{
  *aShiftSelectPivot = mShiftSelectPivot;
  return NS_OK;
}
