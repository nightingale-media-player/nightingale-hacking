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

#include "sbLocalDatabaseSimpleMediaList.h"
#include "sbLocalDatabaseMediaListBase.h"
#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseLibrary.h"

#include <sbIMediaListListener.h>
#include <sbILibrary.h>
#include <sbISQLBuilder.h>
#include <sbSQLBuilderCID.h>
#include <sbIDatabaseQuery.h>
#include <DatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <pratom.h>
#include <nsComponentManagerUtils.h>
#include <nsAutoLock.h>
#include <nsISimpleEnumerator.h>

#define DEFAULT_SORT_PROPERTY \
  NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#ordinal")
#define DEFAULT_FETCH_SIZE 1000

NS_IMPL_ISUPPORTS1(sbSimpleMediaListEnumerationListener, sbIMediaListEnumerationListener)

/**
 * See sbIMediaListListener.idl
 */
NS_IMETHODIMP
sbSimpleMediaListEnumerationListener::OnEnumerationBegin(sbIMediaList* aMediaList,
                                                         PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  NS_ASSERTION(aMediaList != mFriendList,
               "Can't enumerate our friend media list!");

  NS_ENSURE_TRUE(mFriendList, NS_ERROR_FAILURE);

  nsresult rv = mFriendList->GetNextOrdinal(mOrdinal);
  NS_ENSURE_SUCCESS(rv, rv);

  // Prep the query
  mDBQuery = do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString databaseGuid;
  rv = mFriendList->mLibrary->GetDatabaseGuid(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBQuery->SetDatabaseGUID(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBQuery->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBQuery->AddQuery(NS_LITERAL_STRING("begin"));
  NS_ENSURE_SUCCESS(rv, rv);

  // All good for enumerating.
  *_retval = PR_TRUE;
  return NS_OK;
}

/**
 * See sbIMediaListListener.idl
 */
NS_IMETHODIMP
sbSimpleMediaListEnumerationListener::OnEnumeratedItem(sbIMediaList* aMediaList,
                                                       sbIMediaItem* aMediaItem,
                                                       PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  NS_ASSERTION(aMediaList != mFriendList,
               "Can't enumerate our friend media list!");

  nsresult rv = mDBQuery->AddQuery(mFriendList->mInsertIntoListQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabaseMediaItem> ldbmi =
    do_QueryInterface(aMediaItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mediaItemId;
  rv = ldbmi->GetMediaItemId(&mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBQuery->BindInt32Parameter(0, mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBQuery->BindStringParameter(1, mOrdinal);
  NS_ENSURE_SUCCESS(rv, rv);

  // Increment the ordinal
  rv = mFriendList->AddToLastPathSegment(mOrdinal, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  mItemsEnumerated++;

  *_retval = PR_TRUE;
  return NS_OK;
}

/**
 * See sbIMediaListListener.idl
 */
NS_IMETHODIMP
sbSimpleMediaListEnumerationListener::OnEnumerationEnd(sbIMediaList* aMediaList,
                                                       nsresult aStatusCode)
{
  NS_ASSERTION(aMediaList != mFriendList,
               "Can't enumerate our friend media list!");

  mResult = aStatusCode;

  NS_ENSURE_TRUE(mItemsEnumerated, NS_ERROR_UNEXPECTED);

  nsresult rv = mDBQuery->AddQuery(NS_LITERAL_STRING("commit"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbSuccess;
  rv = mDBQuery->Execute(&dbSuccess);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbSuccess, NS_ERROR_FAILURE);

  // Invalidate the cached list
  rv = mFriendList->mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


NS_IMPL_ISUPPORTS_INHERITED0(sbLocalDatabaseSimpleMediaList,
                             sbLocalDatabaseMediaListBase)

nsresult
sbLocalDatabaseSimpleMediaList::Init()
{
  nsresult rv;

  mFullArray = do_CreateInstance(SB_LOCALDATABASE_GUIDARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mediaItemId;
  rv = GetMediaItemId(&mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString databaseGuid;
  rv = mLibrary->GetDatabaseGuid(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetDatabaseGUID(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetBaseTable(NS_LITERAL_STRING("simple_media_lists"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetBaseConstraintColumn(NS_LITERAL_STRING("media_item_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetBaseConstraintValue(mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->AddSort(DEFAULT_SORT_PROPERTY, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetFetchSize(DEFAULT_FETCH_SIZE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->Clone(getter_AddRefs(mViewArray));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CreateQueries();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::GetItemByGuid(const nsAString& aGuid,
                                              sbIMediaItem** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::Contains(sbIMediaItem* aMediaItem,
                                         PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  PRInt32 dbOk;

  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString databaseGuid;
  rv = mLibrary->GetDatabaseGuid(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mGetMediaItemIdForGuidQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString guid;
  rv = aMediaItem->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = rowCount > 0;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::Add(sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  sbSimpleMediaListEnumerationListener listener(this);

  PRBool beginEnumeration;
  nsresult rv = listener.OnEnumerationBegin(nsnull, &beginEnumeration);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(beginEnumeration, NS_ERROR_ABORT);

  PRBool continueEnumerating;
  rv = listener.OnEnumeratedItem(nsnull, aMediaItem, &continueEnumerating);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = listener.OnEnumerationEnd(nsnull, NS_OK);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::AddAll(sbIMediaList* aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  sbSimpleMediaListEnumerationListener listener(this);
  nsresult rv =
    aMediaList->EnumerateAllItems(&listener,
                                  sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::AddSome(nsISimpleEnumerator* aMediaItems)
{
  NS_ENSURE_ARG_POINTER(aMediaItems);

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  sbSimpleMediaListEnumerationListener listener(this);

  PRBool beginEnumeration;
  nsresult rv = listener.OnEnumerationBegin(nsnull, &beginEnumeration);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(beginEnumeration, NS_ERROR_ABORT);

  PRBool hasMore;
  while (NS_SUCCEEDED(aMediaItems->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    rv = aMediaItems->GetNext(getter_AddRefs(supports));
    SB_CONTINUE_IF_FAILED(rv);

    nsCOMPtr<sbIMediaItem> item = do_QueryInterface(supports, &rv);
    SB_CONTINUE_IF_FAILED(rv);

    PRBool continueEnumerating;
    rv = listener.OnEnumeratedItem(nsnull, item, &continueEnumerating);
    if (NS_FAILED(rv) || !continueEnumerating) {
      break;
    }
  }

  rv = listener.OnEnumerationEnd(nsnull, NS_OK);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::InsertBefore(PRUint32 aIndex,
                                             sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv;
  PRInt32 dbOk;

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  PRUint32 length;
  rv = mFullArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG_MAX(aIndex, length - 1);

  nsAutoString ordinal;
  rv = GetBeforeOrdinal(aIndex, ordinal);
  NS_ENSURE_SUCCESS(rv, rv);

  // Run the insert query
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString databaseGuid;
  rv = mLibrary->GetDatabaseGuid(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mInsertIntoListQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabaseMediaItem> ldbmi =
    do_QueryInterface(aMediaItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mediaItemId;
  rv = ldbmi->GetMediaItemId(&mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt32Parameter(0, mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(1, ordinal);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  // Invalidate the cached list
  rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::MoveBefore(PRUint32 aFromIndex,
                                           PRUint32 aToIndex)
{
  // Do nothing if we don't have to move
  if (aFromIndex == aToIndex) {
    return NS_OK;
  }

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  nsresult rv;

  PRUint32 length;
  rv = mFullArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG_MAX(aFromIndex, length - 1);
  NS_ENSURE_ARG_MAX(aToIndex,   length - 1);

  // Get the ordinal of the space before the to index
  nsAutoString ordinal;
  rv = GetBeforeOrdinal(aToIndex, ordinal);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateOrdinalByIndex(aFromIndex, ordinal);
  NS_ENSURE_SUCCESS(rv, rv);

  // Invalidate the cached list
  rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::MoveLast(PRUint32 aIndex)
{
  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  nsresult rv;

  PRUint32 length;
  rv = mFullArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG_MAX(aIndex, length - 1);

  // Get the ordinal for the space after the last item in the list
  nsAutoString ordinal;
  rv = GetNextOrdinal(ordinal);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateOrdinalByIndex(aIndex, ordinal);
  NS_ENSURE_SUCCESS(rv, rv);

  // Invalidate the cached list
  rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::Remove(sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  nsresult rv;

  nsCOMPtr<sbILocalDatabaseMediaItem> ldbmi =
    do_QueryInterface(aMediaItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mediaItemId;
  rv = ldbmi->GetMediaItemId(&mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DeleteItemByMediaItemId(mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  // Invalidate the cached list
  rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::RemoveByIndex(PRUint32 aIndex)
{
  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  nsresult rv;

  PRUint32 mediaItemId;
  rv = mFullArray->GetMediaItemIdByIndex(aIndex, &mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DeleteItemByMediaItemId(mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  // Invalidate the cached list
  rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::RemoveSome(nsISimpleEnumerator* aMediaItems)
{
  NS_ENSURE_ARG_POINTER(aMediaItems);

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  nsresult rv;
  PRInt32 dbOk;

  // Prep the query
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString databaseGuid;
  rv = mLibrary->GetDatabaseGuid(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(NS_LITERAL_STRING("begin"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMoreElements;
  while (NS_SUCCEEDED(aMediaItems->HasMoreElements(&hasMoreElements)) &&
         hasMoreElements) {

    nsCOMPtr<nsISupports> supports;
    rv = aMediaItems->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaItem> item = do_QueryInterface(supports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->AddQuery(mDeleteListItemQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbILocalDatabaseMediaItem> ldbmi =
      do_QueryInterface(item, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 mediaItemId;
    rv = ldbmi->GetMediaItemId(&mediaItemId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindInt32Parameter(0, mediaItemId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = query->AddQuery(NS_LITERAL_STRING("commit"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  // Invalidate the cached list
  rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::Clear()
{
  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  nsresult rv;
  PRInt32 dbOk;

  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString databaseGuid;
  rv = mLibrary->GetDatabaseGuid(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mDeleteAllQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  // Invalidate the cached list
  rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseSimpleMediaList::ExecuteAggregateQuery(const nsAString& aQuery,
                                                      nsAString& aValue)
{
  nsresult rv;
  PRInt32 dbOk;

  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString databaseGuid;
  rv = mLibrary->GetDatabaseGuid(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(aQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (rowCount == 0) {
    return NS_ERROR_UNEXPECTED;
  }

  rv = result->GetRowCell(0, 0, aValue);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseSimpleMediaList::UpdateOrdinalByIndex(PRUint32 aIndex,
                                                     const nsAString& aOrdinal)
{
  nsresult rv;
  PRInt32 dbOk;

  // Get the media item id of the item we are moving
  PRUint32 mediaItemId;
  rv = mFullArray->GetMediaItemIdByIndex(aIndex, &mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  // Update the item at the from index with the new ordinal
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString databaseGuid;
  rv = mLibrary->GetDatabaseGuid(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mUpdateListItemOrdinalQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, aOrdinal);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt32Parameter(1, mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  return NS_OK;
}

nsresult
sbLocalDatabaseSimpleMediaList::DeleteItemByMediaItemId(PRUint32 aMediaItemId)
{
  nsresult rv;
  PRInt32 dbOk;

  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString databaseGuid;
  rv = mLibrary->GetDatabaseGuid(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mDeleteListItemQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt32Parameter(0, aMediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  return NS_OK;
}

nsresult
sbLocalDatabaseSimpleMediaList::GetNextOrdinal(nsAString& aValue)
{
  nsresult rv;

  PRUint32 length;
  rv = mFullArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  if (length == 0) {
    aValue.AssignLiteral("0");
    return NS_OK;
  }

  PRBool cached;
  rv = mFullArray->IsIndexCached(length - 1, &cached);
  NS_ENSURE_SUCCESS(rv, rv);

  if (cached) {
    rv = mFullArray->GetSortPropertyValueByIndex(length - 1, aValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = ExecuteAggregateQuery(mGetLastOrdinalQuery, aValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = AddToLastPathSegment(aValue, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseSimpleMediaList::GetBeforeOrdinal(PRUint32 aIndex,
                                                 nsAString& aValue)
{
  nsresult rv;

  // If we want to insert before the first index, get the ordinal of the
  // first index and trim off everything but the first path and subtract 1
  if (aIndex == 0) {
    PRBool cached;
    rv = mFullArray->IsIndexCached(0, &cached);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString ordinal;
    if (cached) {
      rv = mFullArray->GetSortPropertyValueByIndex(0, ordinal);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = ExecuteAggregateQuery(mGetFirstOrdinalQuery, ordinal);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Trim off additional path segments, if any
    PRUint32 pos = ordinal.FindChar('.');
    if (pos >= 0) {
      ordinal.SetLength(pos);
    }

    PRInt32 value = ordinal.ToInteger(&rv) - 1;
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString newOrdinal;
    newOrdinal.AppendInt(value);
    aValue = newOrdinal;

    return NS_OK;
  }

  // Find the ordinals before and after the place we want to insert
  nsAutoString aboveOrdinal;
  nsAutoString belowOrdinal;

  rv = mFullArray->GetSortPropertyValueByIndex(aIndex - 1, aboveOrdinal);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->GetSortPropertyValueByIndex(aIndex, belowOrdinal);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 aboveLevels = CountLevels(aboveOrdinal);
  PRUint32 belowLevels = CountLevels(belowOrdinal);

  // If the two paths are to the same level, create a new level on the
  // below path to make a path that sorts between
  if (aboveLevels == belowLevels) {
    belowOrdinal.AppendLiteral(".0");
    aValue = belowOrdinal;
    return NS_OK;
  }

  // Otherwise if the below path is longer than the above path, decrement the
  // last path segment of the below path
  if (belowLevels > aboveLevels) {
    rv = AddToLastPathSegment(belowOrdinal, -1);
    NS_ENSURE_SUCCESS(rv, rv);
    aValue = belowOrdinal;
    return NS_OK;
  }

  // If the above path is longer than the below, increment the last path
  // segment of the above path
  rv = AddToLastPathSegment(aboveOrdinal, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  aValue = aboveOrdinal;
  return NS_OK;
}

nsresult
sbLocalDatabaseSimpleMediaList::AddToLastPathSegment(nsAString& aPath,
                                                     PRInt32 aToAdd)
{

  PRUint32 startPos = aPath.RFindChar('.') + 1;
  PRUint32 length = aPath.Length() - startPos;

  nsresult rv;
  PRInt32 value = Substring(aPath, startPos, length).ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  value += aToAdd;

  nsAutoString newValue;
  newValue.AppendInt(value);
  aPath.Replace(startPos, length, newValue);

  return NS_OK;
}

PRUint32
sbLocalDatabaseSimpleMediaList::CountLevels(const nsAString& aPath)
{
  PRUint32 count = 0;
  PRInt32 foundpos = aPath.FindChar('.');
  while(foundpos >= 0) {
    count++;
    foundpos = aPath.FindChar('.', foundpos + 1);
  }
  return count;
}

nsresult
sbLocalDatabaseSimpleMediaList::CreateQueries()
{
  nsresult rv;

  PRUint32 mediaItemId;
  rv = GetMediaItemId(&mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  // The following builder is shared between queries that operate on the
  // items in this list
  nsCOMPtr<sbISQLSelectBuilder> builder =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterion;

  rv = builder->SetBaseTableName(NS_LITERAL_STRING("media_items"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableAlias(NS_LITERAL_STRING("_mi"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                        NS_LITERAL_STRING("simple_media_lists"),
                        NS_LITERAL_STRING("_sml"),
                        NS_LITERAL_STRING("member_media_item_id"),
                        NS_LITERAL_STRING("_mi"),
                        NS_LITERAL_STRING("media_item_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->CreateMatchCriterionLong(NS_LITERAL_STRING("_sml"),
                                         NS_LITERAL_STRING("media_item_id"),
                                         sbISQLSelectBuilder::MATCH_EQUALS,
                                         mediaItemId,
                                         getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create get last ordinal query
  rv = builder->ClearColumns();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(),
                          NS_LITERAL_STRING("max(ordinal)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->ToString(mGetLastOrdinalQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create get first ordinal query
  rv = builder->ClearColumns();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(),
                          NS_LITERAL_STRING("min(ordinal)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->ToString(mGetFirstOrdinalQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the query used by contains to see if a guid is in this list
  rv = builder->ClearColumns();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(NS_LITERAL_STRING("_mi"),
                          NS_LITERAL_STRING("media_item_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->CreateMatchCriterionParameter(NS_LITERAL_STRING("_mi"),
                                              NS_LITERAL_STRING("guid"),
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->ToString(mGetMediaItemIdForGuidQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create insertion query
  nsCOMPtr<sbISQLInsertBuilder> insert =
    do_CreateInstance(SB_SQLBUILDER_INSERT_CONTRACTID, &rv);

  rv = insert->SetIntoTableName(NS_LITERAL_STRING("simple_media_lists"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("media_item_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("member_media_item_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("ordinal"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueLong(mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->ToString(mInsertIntoListQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create ordinal update query
  nsCOMPtr<sbISQLUpdateBuilder> update =
    do_CreateInstance(SB_SQLBUILDER_UPDATE_CONTRACTID, &rv);

  rv = update->SetTableName(NS_LITERAL_STRING("simple_media_lists"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = update->AddAssignmentParameter(NS_LITERAL_STRING("ordinal"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = update->CreateMatchCriterionLong(EmptyString(),
                                        NS_LITERAL_STRING("media_item_id"),
                                        sbISQLSelectBuilder::MATCH_EQUALS,
                                        mediaItemId,
                                        getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = update->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = update->CreateMatchCriterionParameter(EmptyString(),
                                             NS_LITERAL_STRING("member_media_item_id"),
                                             sbISQLSelectBuilder::MATCH_EQUALS,
                                             getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = update->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = update->ToString(mUpdateListItemOrdinalQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create item delete query
  nsCOMPtr<sbISQLDeleteBuilder> deleteb =
    do_CreateInstance(SB_SQLBUILDER_DELETE_CONTRACTID, &rv);

  rv = deleteb->SetTableName(NS_LITERAL_STRING("simple_media_lists"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->CreateMatchCriterionLong(EmptyString(),
                                         NS_LITERAL_STRING("media_item_id"),
                                         sbISQLSelectBuilder::MATCH_EQUALS,
                                         mediaItemId,
                                         getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->CreateMatchCriterionParameter(EmptyString(),
                                              NS_LITERAL_STRING("member_media_item_id"),
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->ToString(mDeleteListItemQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create delete all query
  rv = deleteb->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->SetTableName(NS_LITERAL_STRING("simple_media_lists"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->CreateMatchCriterionLong(EmptyString(),
                                         NS_LITERAL_STRING("media_item_id"),
                                         sbISQLSelectBuilder::MATCH_EQUALS,
                                         mediaItemId,
                                         getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->ToString(mDeleteAllQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create distinct property values query
  rv = builder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetDistinct(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(NS_LITERAL_STRING("_rp"),
                          NS_LITERAL_STRING("obj"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableName(NS_LITERAL_STRING("media_items"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableAlias(NS_LITERAL_STRING("_mi"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                        NS_LITERAL_STRING("simple_media_lists"),
                        NS_LITERAL_STRING("_sml"),
                        NS_LITERAL_STRING("member_media_item_id"),
                        NS_LITERAL_STRING("_mi"),
                        NS_LITERAL_STRING("media_item_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->CreateMatchCriterionLong(NS_LITERAL_STRING("_sml"),
                                         NS_LITERAL_STRING("media_item_id"),
                                         sbISQLSelectBuilder::MATCH_EQUALS,
                                         mediaItemId,
                                         getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                        NS_LITERAL_STRING("resource_properties"),
                        NS_LITERAL_STRING("_rp"),
                        NS_LITERAL_STRING("guid"),
                        NS_LITERAL_STRING("_mi"),
                        NS_LITERAL_STRING("guid"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                        NS_LITERAL_STRING("properties"),
                        NS_LITERAL_STRING("_p"),
                        NS_LITERAL_STRING("property_id"),
                        NS_LITERAL_STRING("_rp"),
                        NS_LITERAL_STRING("property_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->CreateMatchCriterionParameter(NS_LITERAL_STRING("_p"),
                                              NS_LITERAL_STRING("property_name"),
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddOrder(NS_LITERAL_STRING("_rp"),
                         NS_LITERAL_STRING("obj_sortable"),
                         PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->ToString(mDistinctPropertyValuesQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::GetDefaultSortProperty(nsAString& aProperty)
{
  aProperty.Assign(DEFAULT_SORT_PROPERTY);
  return NS_OK;
}

