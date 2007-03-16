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

#include "sbLocalDatabaseViewMediaList.h"
#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseLibrary.h"
#include "sbLocalDatabaseMediaListBase.h"

#include <sbIMediaListListener.h>
#include <sbILibrary.h>
#include <sbISQLBuilder.h>
#include <sbSQLBuilderCID.h>
#include <sbIDatabaseQuery.h>
#include <DatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <nsISimpleEnumerator.h>
#include <nsComponentManagerUtils.h>
#include <nsAutoLock.h>

#define DEFAULT_SORT_PROPERTY \
  NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#created")
#define DEFAULT_FETCH_SIZE 1000

NS_IMPL_ISUPPORTS_INHERITED0(sbLocalDatabaseViewMediaList,
                             sbLocalDatabaseMediaListBase)

sbLocalDatabaseViewMediaList::sbLocalDatabaseViewMediaList(sbILocalDatabaseLibrary* aLibrary,
                                                           const nsAString& aGuid) :
  sbLocalDatabaseMediaListBase(aLibrary, aGuid)
{
  mListUpdateLock = PR_NewLock();
  NS_ASSERTION(mListUpdateLock, "Failed to create lock!");
}

sbLocalDatabaseViewMediaList::~sbLocalDatabaseViewMediaList()
{
  if (mListUpdateLock) {
    PR_DestroyLock(mListUpdateLock);
    mListUpdateLock = nsnull;
  }
}

nsresult
sbLocalDatabaseViewMediaList::Init()
{
  nsresult rv;

  mFullArray = do_CreateInstance(SB_LOCALDATABASE_GUIDARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString databaseGuid;
  rv = mLibrary->GetDatabaseGuid(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetDatabaseGUID(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetBaseTable(NS_LITERAL_STRING("media_items"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->AddSort(DEFAULT_SORT_PROPERTY, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetFetchSize(DEFAULT_FETCH_SIZE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CreateQueries();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseViewMediaList::GetItemByGuid(const nsAString& aGuid,
                                            sbIMediaItem** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsCOMPtr<sbILibrary> library = do_QueryInterface(mLibrary, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;
  rv = library->GetMediaItem(aGuid, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = item);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseViewMediaList::Contains(sbIMediaItem* aMediaItem,
                                       PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  /*
   * Simply use the GetMediaItemIdForGuid method on the library to determine
   * if this item is in the database
   */
  nsAutoString guid;
  rv = aMediaItem->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mediaItemId;
  rv = mLibrary->GetMediaItemIdForGuid(guid, &mediaItemId);
  if (NS_SUCCEEDED(rv)) {
    *_retval = PR_TRUE;
  }
  else {
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      *_retval = PR_FALSE;
    }
    else {
      return rv;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseViewMediaList::Add(sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv;

  /*
   * If the media item's library and this list's library are the same, this
   * item must already be in this database
   */
  nsCOMPtr<sbILibrary> itemLibrary;
  rv = aMediaItem->GetLibrary(getter_AddRefs(itemLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> library = do_QueryInterface(mLibrary, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (itemLibrary == library) {
    return NS_ERROR_INVALID_ARG;
  }

  /*
   * Otherwise, we might want to transfer this item into this database, but
   * we need to figure out the rules for this in the future
   */
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseViewMediaList::AddAll(sbIMediaList* aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsresult rv;

  nsCOMPtr<nsISimpleEnumerator> items;
  rv = aMediaList->GetItems(getter_AddRefs(items));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddSome(items);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseViewMediaList::AddSome(nsISimpleEnumerator* aMediaItems)
{
  NS_ENSURE_ARG_POINTER(aMediaItems);

  nsresult rv;

  nsCOMPtr<sbILibrary> library = do_QueryInterface(mLibrary, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  /*
   * See documentation for Add()
   */
  PRBool hasMoreElements;
  while (NS_SUCCEEDED(aMediaItems->HasMoreElements(&hasMoreElements)) &&
         hasMoreElements) {

    nsCOMPtr<nsISupports> supports;
    rv = aMediaItems->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaItem> item = do_QueryInterface(supports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbILibrary> itemLibrary;
    rv = item->GetLibrary(getter_AddRefs(itemLibrary));
    NS_ENSURE_SUCCESS(rv, rv);

    if (itemLibrary == library) {
      return NS_ERROR_INVALID_ARG;
    }

  }

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseViewMediaList::InsertBefore(PRUint32 aIndex,
                                           sbIMediaItem* aMediaItem)
{
  // The view media list is not ordered so this method can not be implemented
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
sbLocalDatabaseViewMediaList::MoveBefore(PRUint32 aFromIndex,
                                         PRUint32 aToIndex)
{
  // The view media list is not ordered so this method can not be implemented
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
sbLocalDatabaseViewMediaList::MoveLast(PRUint32 aIndex)
{
  // The view media list is not ordered so this method can not be implemented
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
sbLocalDatabaseViewMediaList::Remove(sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  // Serialize list changes
  nsAutoLock lock(mListUpdateLock);

  nsresult rv;

  // TODO: If this item is a list, notify all list members that they are about
  // to be deleted

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
sbLocalDatabaseViewMediaList::RemoveByIndex(PRUint32 aIndex)
{
  // Serialize list changes
  nsAutoLock lock(mListUpdateLock);

  nsresult rv;

  // TODO: If this item is a list, notify all list members that they are about
  // to be deleted

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
sbLocalDatabaseViewMediaList::RemoveSome(nsISimpleEnumerator* aMediaItems)
{
  NS_ENSURE_ARG_POINTER(aMediaItems);

  // Serialize list changes
  nsAutoLock lock(mListUpdateLock);

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

    // TODO: If this item is a list, notify all list members that they are about
    // to be deleted

    rv = query->AddQuery(mDeleteItemQuery);
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
sbLocalDatabaseViewMediaList::Clear()
{
  // Serialize list changes
  nsAutoLock lock(mListUpdateLock);

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
sbLocalDatabaseViewMediaList::DeleteItemByMediaItemId(PRUint32 aMediaItemId)
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

  rv = query->AddQuery(mDeleteItemQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt32Parameter(0, aMediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  return NS_OK;
}

nsresult
sbLocalDatabaseViewMediaList::CreateQueries()
{
  nsresult rv;

  // Create item delete query
  nsCOMPtr<sbISQLDeleteBuilder> deleteb =
    do_CreateInstance(SB_SQLBUILDER_DELETE_CONTRACTID, &rv);

  rv = deleteb->SetTableName(NS_LITERAL_STRING("media_items"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterion;
  rv = deleteb->CreateMatchCriterionParameter(EmptyString(),
                                              NS_LITERAL_STRING("media_item_id"),
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->ToString(mDeleteItemQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create delete all query
  rv = deleteb->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->SetTableName(NS_LITERAL_STRING("media_items"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->ToString(mDeleteAllQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

