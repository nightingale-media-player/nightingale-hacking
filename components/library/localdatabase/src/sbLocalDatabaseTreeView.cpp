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
#include "sbLocalDatabaseCID.h"
#include <nsComponentManagerUtils.h>
#include <nsIDOMElement.h>
#include <nsITreeColumns.h>
#include <nsMemory.h>
#include <stdio.h>

#include "sbLocalDatabasePropertyCache.h"

NS_IMPL_ISUPPORTS2(sbLocalDatabaseTreeView,
                   sbILocalDatabaseTreeView,
                   nsITreeView)

sbLocalDatabaseTreeView::sbLocalDatabaseTreeView() :
 mSelection(nsnull),
 mFetchTimerScheduled(PR_FALSE)
{
}

sbLocalDatabaseTreeView::~sbLocalDatabaseTreeView()
{
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::Init()
{
  nsresult rv;

  NS_ENSURE_TRUE(mCache.Init(100), NS_ERROR_OUT_OF_MEMORY);

  mFetchTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mArray = do_CreateInstance(SB_LOCALDATABASE_GUIDARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->SetDatabaseGUID(NS_LITERAL_STRING("test_1000"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->SetBaseTable(NS_LITERAL_STRING("media_items"));
  NS_ENSURE_SUCCESS(rv, rv);

  mSortProperty = NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#artistName");
  mSortDirection = PR_TRUE;

  rv = mArray->AddSort(mSortProperty, mSortDirection);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->AddSort(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#albumName"), PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->AddSort(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#track"), PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->SetFetchSize(1000);
  NS_ENSURE_SUCCESS(rv, rv);

  mPropertyCache = do_CreateInstance(SB_LOCALDATABASE_PROPERTYCACHE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertyCache->SetDatabaseGUID(NS_LITERAL_STRING("test_1000"));
  NS_ENSURE_SUCCESS(rv, rv);

//  rv = mArray->SetPropertyCache(mPropertyCache);
//  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetSearch(nsAString& aSearch)
{
  aSearch = mSearch;
  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseTreeView::SetSearch(const nsAString& aSearch)
{
  nsresult rv;

  if (!aSearch.Equals(mSearch)) {
    mSearch = aSearch;
    rv = ResetFilters();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::FetchProperties()
{
  nsresult rv;
  PRUint32 length = mFetchList.Length();
  if (length == 0) {
    return NS_OK;
  }

  const PRUnichar** guids = new const PRUnichar*[length];
  for (PRUint32 i = 0; i < length; i++) {
    guids[i] = mFetchList[i].get();
  }

  PRUint32 count = 0;
  sbILocalDatabaseResourcePropertyBag** bags = nsnull;
  rv = mPropertyCache->GetProperties(guids, length, &count, &bags);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < count; i++) {
    if (bags[i]) {
      nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag(bags[i]);
      NS_ENSURE_TRUE(mCache.Put(mFetchList[i], bag),
                     NS_ERROR_OUT_OF_MEMORY);
      NS_RELEASE(bags[i]);
    }
  }

  nsMemory::Free(bags);
  delete guids;
  mFetchList.Clear();

  rv = mTreeBoxObject->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbLocalDatabaseTreeView::GetPropertyForTreeColumn(nsITreeColumn* aTreeColumn,
                                                  nsAString& aProperty)
{
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
sbLocalDatabaseTreeView::ResetFilters()
{
  nsresult rv;

  PRUint32 oldCount;
  rv = mArray->GetLength(&oldCount);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->ClearFilters();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mSearch.Equals(EmptyString())) {
    nsTArray<nsString> search;
    NS_ENSURE_TRUE(search.AppendElement(mSearch),
                   NS_ERROR_OUT_OF_MEMORY);
    nsCOMPtr<nsIStringEnumerator> searchEnum =
      new sbTArrayStringEnumerator(&search);
    NS_ENSURE_TRUE(searchEnum, NS_ERROR_OUT_OF_MEMORY);
    rv = mArray->AddFilter(EmptyString(), searchEnum, PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRUint32 newCount;
  rv = mArray->GetLength(&newCount);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mTreeBoxObject->BeginUpdateBatch();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mTreeBoxObject->RowCountChanged(0, -oldCount);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mTreeBoxObject->RowCountChanged(0, newCount);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mTreeBoxObject->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mTreeBoxObject->EndUpdateBatch();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetRowCount(PRInt32 *aRowCount)
{
  nsresult rv;
  PRUint32 count;

  rv = mArray->GetLength(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  *aRowCount = count;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetCellText(PRInt32 row,
                                     nsITreeColumn *col,
                                     nsAString& _retval)
{
  nsresult rv;

  nsAutoString bind;
  rv = GetPropertyForTreeColumn(col, bind);
  NS_ENSURE_SUCCESS(rv, rv);

  if (bind.Equals(NS_LITERAL_STRING("row"))) {
    _retval.AppendInt(row);
  }
  else {
    if (bind.Equals(NS_LITERAL_STRING("guid"))) {
      const PRUnichar* guid;
      rv = mArray->GetByIndexShared(row, &guid);
      NS_ENSURE_SUCCESS(rv, rv);
      _retval.Assign(guid);
    }
    else {
      nsAutoString guid;
      rv = mArray->GetByIndex(row, guid);
      NS_ENSURE_SUCCESS(rv, rv);

      sbILocalDatabaseResourcePropertyBag* bag;
      if (mCache.Get(guid, &bag)) {
        rv = bag->GetProperty(bind, _retval);
        if(rv == NS_ERROR_ILLEGAL_VALUE) {
          _retval.Assign(EmptyString());
        }
        else {
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
      else {
        // TODO: prevent duplicate guids here?
        NS_ENSURE_TRUE(mFetchList.AppendElement(guid),
                       NS_ERROR_OUT_OF_MEMORY);
        if (!mFetchTimerScheduled) {
          mFetchTimer->InitWithCallback(this, 10, nsITimer::TYPE_ONE_SHOT);
          NS_ENSURE_SUCCESS(rv, rv);
          mFetchTimerScheduled = PR_TRUE;
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetSelection(nsITreeSelection * *aSelection)
{
  if (!mSelection) {
    *aSelection = nsnull;
    return NS_ERROR_FAILURE;
  }
  NS_ADDREF(*aSelection = mSelection);
  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseTreeView::SetSelection(nsITreeSelection * aSelection)
{
  mSelection = aSelection;
  return NS_OK;
}


NS_IMETHODIMP
sbLocalDatabaseTreeView::CycleHeader(nsITreeColumn *col)
{
  nsresult rv;

  nsAutoString bind;
  rv = GetPropertyForTreeColumn(col, bind);
  NS_ENSURE_SUCCESS(rv, rv);

  if (bind.Equals(NS_LITERAL_STRING("row")) ||
      bind.Equals(NS_LITERAL_STRING("guid"))) {
    return NS_OK;
  }

  rv = mArray->ClearSorts();
  NS_ENSURE_SUCCESS(rv, rv);

  if (bind.Equals(mSortProperty)) {
    mSortDirection = !mSortDirection;
  }
  else {
    mSortProperty = bind;
    mSortDirection = PR_TRUE;
  }

  rv = mArray->AddSort(mSortProperty, mSortDirection);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mTreeBoxObject->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::CycleCell(PRInt32 row, nsITreeColumn *col)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetRowProperties(PRInt32 index, nsISupportsArray *properties)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetCellProperties(PRInt32 row, nsITreeColumn *col, nsISupportsArray *properties)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetColumnProperties(nsITreeColumn *col, nsISupportsArray *properties)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsContainer(PRInt32 index, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsContainerOpen(PRInt32 index, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsContainerEmpty(PRInt32 index, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsSeparator(PRInt32 index, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsSorted(PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::CanDrop(PRInt32 index, PRInt32 orientation, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::Drop(PRInt32 row, PRInt32 orientation)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetParentIndex(PRInt32 rowIndex, PRInt32 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::HasNextSibling(PRInt32 rowIndex, PRInt32 afterIndex, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetLevel(PRInt32 index, PRInt32 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetImageSrc(PRInt32 row, nsITreeColumn *col, nsAString & _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetProgressMode(PRInt32 row, nsITreeColumn *col, PRInt32 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::GetCellValue(PRInt32 row, nsITreeColumn *col, nsAString & _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::SetTree(nsITreeBoxObject *tree)
{
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
sbLocalDatabaseTreeView::IsEditable(PRInt32 row, nsITreeColumn *col, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::IsSelectable(PRInt32 row, nsITreeColumn *col, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::SetCellValue(PRInt32 row, nsITreeColumn *col, const nsAString & value)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::SetCellText(PRInt32 row, nsITreeColumn *col, const nsAString & value)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::PerformAction(const PRUnichar *action)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::PerformActionOnRow(const PRUnichar *action, PRInt32 row)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::PerformActionOnCell(const PRUnichar *action, PRInt32 row, nsITreeColumn *col)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseTreeView::Notify(nsITimer *timer)
{
  mFetchTimerScheduled = PR_FALSE;

  return FetchProperties();
}
