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

#include "sbLocalDatabaseLibrary.h"
#include "sbLocalDatabaseMediaItem.h"
#include "sbLocalDatabaseCID.h"

#include <sbIMediaListFactory.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbISQLBuilder.h>
#include <sbSQLBuilderCID.h>
#include <sbIDatabaseQuery.h>
#include <DatabaseQuery.h>
#include <sbIDatabaseResult.h>

#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsID.h>
#include <nsIFile.h>
#include <nsIURI.h>
#include <prtime.h>
#include <nsMemory.h>
#include <nsIUUIDGenerator.h>

#include <stdio.h>
#include <prprf.h>

NS_IMPL_ISUPPORTS_INHERITED2(sbLocalDatabaseLibrary,
                             sbLocalDatabaseResourceProperty,
                             sbILibrary,
                             sbILocalDatabaseLibrary)

// sbILocalDatabaseLibrary
NS_IMETHODIMP
sbLocalDatabaseLibrary::Init()
{
  nsresult rv;
  PRInt32 dbOk;

  // Maybe check to this that this db is valid, etc?
  // Check version and migrate if needed?

  mPropertyCache =
    do_CreateInstance(SB_LOCALDATABASE_PROPERTYCACHE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertyCache->SetDatabaseGUID(mDatabaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CreateQueries();
  NS_ENSURE_SUCCESS(rv, rv);

  // Read our library guid
  nsCOMPtr<sbISQLSelectBuilder> builder =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(),
                          NS_LITERAL_STRING("value"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableName(NS_LITERAL_STRING("library_metadata"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterion;
  rv = builder->CreateMatchCriterionString(EmptyString(),
                                           NS_LITERAL_STRING("name"),
                                           sbISQLSelectBuilder::MATCH_EQUALS,
                                           NS_LITERAL_STRING("resource-guid"),
                                           getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString sql;
  rv = builder->ToString(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(sql);
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

  NS_ENSURE_FALSE(rowCount == 0, NS_ERROR_UNEXPECTED);

  rv = result->GetRowCell(0, 0, mGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  sbLocalDatabaseResourceProperty::InitResourceProperty(mPropertyCache, mGuid);

  return NS_OK;
}

/* inline */ NS_METHOD
sbLocalDatabaseLibrary::MakeStandardQuery(sbIDatabaseQuery** _retval)
{
  nsresult rv;
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(mDatabaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Set the location (if it was specified in the constructor)
  if (mDatabaseLocation) {
    rv = query->SetDatabaseLocation(mDatabaseLocation);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = query->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = query);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::GetContractIdForGuid(const nsAString& aGuid,
                                             nsACString &aContractId)
{
  nsresult rv;
  PRInt32 dbOk;

  /*
   * We could probably cache the result of this method
   */
  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mGetContractIdForGuidQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, aGuid);
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

  if (rowCount == 1) {
    nsAutoString contractId;
    rv = result->GetRowCell(0, 0, contractId);
    NS_ENSURE_SUCCESS(rv, rv);

    aContractId = NS_ConvertUTF16toUTF8(contractId);
  }
  else {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::GetMediaItemIdForGuid(const nsAString& aGuid,
                                              PRUint32* aMediaItemId)
{
  nsresult rv;
  PRInt32 dbOk;

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mGetMediaItemIdForGuidQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, aGuid);
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

  if (rowCount == 1) {
    nsAutoString mediaItemIdStr;
    rv = result->GetRowCell(0, 0, mediaItemIdStr);
    NS_ENSURE_SUCCESS(rv, rv);

    *aMediaItemId = mediaItemIdStr.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::GetDatabaseGuid(nsAString& aDatabaseGuid)
{
  aDatabaseGuid = mDatabaseGuid;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::GetDatabaseLocation(nsIURI** aDatabaseLocation)
{
  NS_ENSURE_ARG_POINTER(aDatabaseLocation);

  if (!mDatabaseLocation) {
    *aDatabaseLocation = nsnull;
    return NS_OK;
  }

  nsresult rv = mDatabaseLocation->Clone(aDatabaseLocation);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* readonly attribute sbILocalDatabasePropertyCache propertyCache; */
NS_IMETHODIMP sbLocalDatabaseLibrary::GetPropertyCache(sbILocalDatabasePropertyCache * *aPropertyCache)
{
  NS_ENSURE_ARG_POINTER(aPropertyCache);
  NS_ADDREF(*aPropertyCache = mPropertyCache);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::GetPropertiesForGuid(const nsAString& aGuid,
                                             sbILocalDatabaseResourcePropertyBag** aPropertyBag)
{
  NS_ENSURE_ARG_POINTER(aPropertyBag);

  nsresult rv;

  const PRUnichar** guids = new const PRUnichar*[1];
  guids[0] = PromiseFlatString(aGuid).get();

  PRUint32 count;
  sbILocalDatabaseResourcePropertyBag** bags;
  rv = mPropertyCache->GetProperties(guids, 1, &count, &bags);
  NS_ENSURE_SUCCESS(rv, rv);

  *aPropertyBag = nsnull;
  if (count > 0 && bags[0]) {
    NS_ADDREF(*aPropertyBag = bags[0]);
  }

  nsMemory::Free(bags);
  delete[] guids;

  if (*aPropertyBag) {
    return NS_OK;
  }
  else {
    return NS_ERROR_NOT_AVAILABLE;
  }

}

// sbILibrary
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetDevice(sbIDevice** aDevice)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::GetSupportsForeignMediaItems(PRBool *aSupportsForeignMediaItems)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::Resolve(nsIURI *aUri, nsIChannel **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::CreateMediaItem(nsIURI *aUri,
                                        sbIMediaItem **_retval)
{
  NS_ENSURE_ARG_POINTER(aUri);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  PRInt32 dbOk;

  /*
   * Generate a new guid
   */
  nsCOMPtr<nsIUUIDGenerator> uuidGen =
    do_GetService("@mozilla.org/uuid-generator;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsID id;
  rv = uuidGen->GenerateUUIDInPlace(&id);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString guidUtf8(id.ToString());
  nsAutoString fullGuid;
  fullGuid = NS_ConvertUTF8toUTF16(guidUtf8);

  const nsAString& guid = Substring(fullGuid, 1, fullGuid.Length() - 2);

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mInsertMediaItemQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  /*
   * Set guid
   */
  rv = query->BindStringParameter(0, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  /*
   * Set create and update.  Make a string of the current time in milliseconds
   */
  char buf[20];
  int len = PR_snprintf(buf, sizeof(buf), "%lld",
                        PRUint64((PR_Now() / PR_USEC_PER_MSEC)));
  buf[sizeof(buf) - 1] = '\0';
  nsAutoString create = NS_ConvertASCIItoUTF16(buf, len);

  rv = query->BindStringParameter(1, create);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(2, create);
  NS_ENSURE_SUCCESS(rv, rv);

  /*
   * Set content url
   */
  nsCAutoString uriUtf8;
  rv = aUri->GetSpec(uriUtf8);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString uri;
  uri = NS_ConvertUTF8toUTF16(uriUtf8);

  rv = query->BindStringParameter(3, uri);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  sbLocalDatabaseMediaItem* item = new sbLocalDatabaseMediaItem(this, guid);
  NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = item);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::GetMediaItem(const nsAString& aGuid, 
                                     sbIMediaItem **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  /*
   * Get the contract id of the guid.  If the contract id is blank, we know
   * it is just a media item and we can simply create it.  Otherwise get the
   * guid's factory and create an instace
   */
  nsCAutoString contractId;
  rv = GetContractIdForGuid(aGuid, contractId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;
  if (contractId.IsEmpty()) {
    item = new sbLocalDatabaseMediaItem(this, aGuid);
    NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    nsCOMPtr<sbIMediaListFactory> factory = do_GetService(contractId.get(), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = factory->SetLibrary(this);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaList> list;
    rv = factory->InstantiateMediaList(aGuid, getter_AddRefs(list));
    NS_ENSURE_SUCCESS(rv, rv);

    item = do_QueryInterface(list, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF(*_retval = item);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::GetMediaListFactories(nsISimpleEnumerator** aMediaListFactories)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::GetMediaListFactory(const nsAString& aType,
                                            sbIMediaListFactory** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  // TODO: This is totally fake, needs to look up name in the list of
  // registered factories, etc.
  const char* contractid;

  if (aType.EqualsLiteral("view")) {
    contractid = SB_LOCALDATABASE_VIEWMEDIALISTFACTORY_CONTRACTID;
  }
  else {
    if (aType.EqualsLiteral("simple")) {
      contractid = SB_LOCALDATABASE_SIMPLEMEDIALISTFACTORY_CONTRACTID;
    }
    else {
      return NS_ERROR_INVALID_ARG;
    }
  }

  nsCOMPtr<sbIMediaListFactory> factory = do_GetService(contractid, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = factory->SetLibrary(this);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = factory);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::AddMediaListFactory(const nsAString& aType,
                                            const nsAString& aContractId)
{
  // insert new thing into db
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::BeginBatch(PRBool aIsAsync)
{
  // this should increment a counter to allow for nested batches

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::CancelBatch()
{
  // this should increment a counter to allow for nested batches
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::EndBatch()
{
  // this should decrement a counter to allow for nested batches
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::TidyUp()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::Shutdown()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
sbLocalDatabaseLibrary::CreateQueries()
{
  nsresult rv;

  // Build some queries
  nsCOMPtr<sbISQLSelectBuilder> builder =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(NS_LITERAL_STRING("_mlt"),
                          NS_LITERAL_STRING("factory_contractid"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableName(NS_LITERAL_STRING("media_items"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableAlias(NS_LITERAL_STRING("_mi"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddJoin(sbISQLSelectBuilder::JOIN_LEFT,
                        NS_LITERAL_STRING("media_list_types"),
                        NS_LITERAL_STRING("_mlt"),
                        NS_LITERAL_STRING("media_list_type_id"),
                        NS_LITERAL_STRING("_mi"),
                        NS_LITERAL_STRING("media_list_type_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterion;
  rv = builder->CreateMatchCriterionParameter(NS_LITERAL_STRING("_mi"),
                                              NS_LITERAL_STRING("guid"),
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->ToString(mGetContractIdForGuidQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(), NS_LITERAL_STRING("media_item_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableName(NS_LITERAL_STRING("media_items"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->CreateMatchCriterionParameter(EmptyString(),
                                              NS_LITERAL_STRING("guid"),
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->ToString(mGetMediaItemIdForGuidQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Build new media item query
  nsCOMPtr<sbISQLInsertBuilder> insert =
    do_CreateInstance(SB_SQLBUILDER_INSERT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->SetIntoTableName(NS_LITERAL_STRING("media_items"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("guid"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("created"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("updated"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("content_url"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->ToString(mInsertMediaItemQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

