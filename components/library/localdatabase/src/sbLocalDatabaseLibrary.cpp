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

#include <nsArrayUtils.h>
#include <nsIClassInfo.h>
#include <nsIFile.h>
#include <nsIMutableArray.h>
#include <nsIProgrammingLanguage.h>
#include <nsIPropertyBag2.h>
#include <nsISimpleEnumerator.h>
#include <nsIStringEnumerator.h>
#include <nsISupportsPrimitives.h>
#include <nsIURI.h>
#include <nsIUUIDGenerator.h>
#include <sbIDatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <sbILibraryFactory.h>
#include <sbILibraryResource.h>
#include <sbILocalDatabaseGUIDArray.h>
#include <sbILocalDatabasePropertyCache.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbIMediaListView.h>
#include <sbISQLBuilder.h>
#include <nsITimer.h>

#include <DatabaseQuery.h>
#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsHashKeys.h>
#include <nsID.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOM.h>
#include <nsWeakReference.h>
#include <prlog.h>
#include <prprf.h>
#include <prtime.h>
#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseMediaItem.h"
#include "sbLocalDatabaseMediaListView.h"
#include "sbLocalDatabasePropertyCache.h"
#include "sbLocalDatabaseSimpleMediaListFactory.h"
#include <sbSQLBuilderCID.h>
#include <sbTArrayStringEnumerator.h>

#define NS_UUID_GENERATOR_CONTRACTID "@mozilla.org/uuid-generator;1"

#define SB_MEDIAITEM_TYPEID 0

#define SB_MEDIALIST_FACTORY_DEFAULT_TYPE 1
#define SB_MEDIALIST_FACTORY_URI_PREFIX   "medialist('"
#define SB_MEDIALIST_FACTORY_URI_SUFFIX   "')"

#define SB_ILIBRESOURCE_CAST(_ptr)                                             \
  NS_STATIC_CAST(sbILibraryResource*,                                          \
                 NS_STATIC_CAST(sbILibrary*, _ptr))

#define DEFAULT_SORT_PROPERTY "http://songbirdnest.com/data/1.0#created"

#define DEFAULT_FETCH_SIZE 1000

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbLocalDatabaseLibrary:5
 */
#ifdef PR_LOGGING

static PRLogModuleInfo* gLibraryLog = nsnull;
#define TRACE(args) if (gLibraryLog) PR_LOG(gLibraryLog, PR_LOG_DEBUG, args)
#define LOG(args)   if (gLibraryLog) PR_LOG(gLibraryLog, PR_LOG_WARN, args)

#else /* PR_LOGGING */

#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */

#endif /* PR_LOGGING */

// Makes some of the logging a little easier to read
#define LOG_SUBMESSAGE_SPACE "                                 - "

#define countof(_array) (sizeof(_array) / sizeof(_array[0]))

static char* kInsertQueryColumns[] = {
  "guid",
  "created",
  "updated",
  "content_url",
  "media_list_type_id"
};

NS_IMPL_ISUPPORTS1(sbLibraryInsertingEnumerationListener,
                   sbIMediaListEnumerationListener)

/**
 * See sbIMediaListListener.idl
 */
NS_IMETHODIMP
sbLibraryInsertingEnumerationListener::OnEnumerationBegin(sbIMediaList* aMediaList,
                                                          PRBool* _retval)
{
  if (_retval) {
    *_retval = PR_TRUE;
  }

  return NS_OK;
}

/**
 * See sbIMediaListListener.idl
 */
NS_IMETHODIMP
sbLibraryInsertingEnumerationListener::OnEnumeratedItem(sbIMediaList* aMediaList,
                                                        sbIMediaItem* aMediaItem,
                                                        PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv;
  nsCOMPtr<sbILibrary> fromLibrary;
  rv = aMediaItem->GetLibrary(getter_AddRefs(fromLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  // If the media item is not in the view, add it
  PRBool equals;
  rv = fromLibrary->Equals(SB_ILIBRESOURCE_CAST(mFriendLibrary), &equals);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!equals) {
    rv = mFriendLibrary->AddItemToLocalDatabase(aMediaItem);
    NS_ENSURE_SUCCESS(rv, rv);

    // Remember this media item for later so we can notify with it
    PRBool success = mNotificationList.AppendObject(aMediaItem);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    mShouldInvalidate = PR_TRUE;
  }

  if (_retval) {
    *_retval = PR_TRUE;
  }

  return NS_OK;
}

/**
 * See sbIMediaListListener.idl
 */
NS_IMETHODIMP
sbLibraryInsertingEnumerationListener::OnEnumerationEnd(sbIMediaList* aMediaList,
                                                        nsresult aStatusCode)
{
  if (mShouldInvalidate) {
    NS_ASSERTION(mFriendLibrary->mFullArray, "Uh, no full array?!");

    nsresult rv = mFriendLibrary->mFullArray->Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Notify our listeners
  PRUint32 count = mNotificationList.Count();
  for (PRUint32 i = 0; i < count; i++) {
    mFriendLibrary->NotifyListenersItemAdded(mFriendLibrary,
                                             mNotificationList[i]);
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS1(sbLibraryRemovingEnumerationListener,
                   sbIMediaListEnumerationListener)

/**
 * See sbIMediaListListener.idl
 */
NS_IMETHODIMP
sbLibraryRemovingEnumerationListener::OnEnumerationBegin(sbIMediaList* aMediaList,
                                                         PRBool* _retval)
{
  // Prep the query
  nsresult rv = mFriendLibrary->MakeStandardQuery(getter_AddRefs(mDBQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBQuery->AddQuery(NS_LITERAL_STRING("begin"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbIMediaListListener.idl
 */
NS_IMETHODIMP
sbLibraryRemovingEnumerationListener::OnEnumeratedItem(sbIMediaList* aMediaList,
                                                       sbIMediaItem* aMediaItem,
                                                       PRBool* _retval)
{
  NS_ASSERTION(aMediaItem, "Null pointer!");

  // Remember this media item for later so we can notify with it
  PRBool success = mNotificationList.AppendObject(aMediaItem);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString guid;
  nsresult rv = aMediaItem->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Remove from our cache.
  mFriendLibrary->mMediaItemTable.Remove(guid);

  rv = mDBQuery->AddQuery(mFriendLibrary->mDeleteItemQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBQuery->BindStringParameter(0, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  mItemEnumerated = PR_TRUE;
  return NS_OK;
}

/**
 * See sbIMediaListListener.idl
 */
NS_IMETHODIMP
sbLibraryRemovingEnumerationListener::OnEnumerationEnd(sbIMediaList* aMediaList,
                                                       nsresult aStatusCode)
{
  if (!mItemEnumerated) {
    NS_WARNING("OnEnumerationEnd called with no items enumerated");
    return NS_OK;
  }

  nsresult rv = mDBQuery->AddQuery(NS_LITERAL_STRING("commit"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Notify our listeners of before removal
  PRUint32 count = mNotificationList.Count();
  for (PRUint32 i = 0; i < count; i++) {
    mFriendLibrary->NotifyListenersBeforeItemRemoved(mFriendLibrary,
                                                     mNotificationList[i]);
  }

  PRInt32 dbSuccess;
  rv = mDBQuery->Execute(&dbSuccess);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbSuccess, NS_ERROR_FAILURE);

  rv = mFriendLibrary->mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  // Notify our listeners of after removal
  for (PRUint32 i = 0; i < count; i++) {
    mFriendLibrary->NotifyListenersAfterItemRemoved(mFriendLibrary,
                                               mNotificationList[i]);
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED4(sbLocalDatabaseLibrary, sbLocalDatabaseMediaListBase,
                                                     nsIClassInfo,
                                                     sbIDatabaseSimpleQueryCallback,
                                                     sbILibrary,
                                                     sbILocalDatabaseLibrary)

NS_IMPL_CI_INTERFACE_GETTER7(sbLocalDatabaseLibrary,
                             nsIClassInfo,
                             nsISupportsWeakReference,
                             sbIDatabaseSimpleQueryCallback,
                             sbILibrary,
                             sbILibraryResource,
                             sbIMediaItem,
                             sbIMediaList);

sbLocalDatabaseLibrary::sbLocalDatabaseLibrary()
{
#ifdef PR_LOGGING
  if (!gLibraryLog) {
    gLibraryLog = PR_NewLogModule("sbLocalDatabaseLibrary");
  }
#endif
  TRACE(("LocalDatabaseLibrary[0x%.8x] - Constructed", this));
}

sbLocalDatabaseLibrary::~sbLocalDatabaseLibrary()
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - Destructed", this));
}

nsresult
sbLocalDatabaseLibrary::Init(const nsAString& aDatabaseGuid,
                             nsIPropertyBag2* aCreationParameters,
                             sbILibraryFactory* aFactory,
                             nsIURI* aDatabaseLocation)
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - Init()", this));
  NS_ENSURE_FALSE(aDatabaseGuid.IsEmpty(), NS_ERROR_INVALID_ARG);
  NS_ENSURE_ARG_POINTER(aCreationParameters);
  NS_ENSURE_ARG_POINTER(aFactory);

  // Maybe check to this that this db is valid, etc?
  // Check version and migrate if needed?

  mDatabaseGuid.Assign(aDatabaseGuid);

  mCreationParameters = aCreationParameters;

  mFactory = aFactory;

  // This may be null.
  mDatabaseLocation = aDatabaseLocation;

  nsresult rv;
  mFullArray = do_CreateInstance(SB_LOCALDATABASE_GUIDARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetDatabaseGUID(aDatabaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mDatabaseLocation) {
    rv = mFullArray->SetDatabaseLocation(aDatabaseLocation);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mFullArray->SetBaseTable(NS_LITERAL_STRING("media_items"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->AddSort(NS_LITERAL_STRING(DEFAULT_SORT_PROPERTY), PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetFetchSize(DEFAULT_FETCH_SIZE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoPtr<sbLocalDatabasePropertyCache>
    propCache(new sbLocalDatabasePropertyCache());
  NS_ENSURE_TRUE(propCache, NS_ERROR_OUT_OF_MEMORY);

  rv = propCache->Init(this);
  NS_ENSURE_SUCCESS(rv, rv);

  mPropertyCache = propCache.forget();

  rv = CreateQueries();
  NS_ENSURE_SUCCESS(rv, rv);

  // Find our resource GUID. This identifies us within the library (as opposed
  // to the database file used by the DBEngine).
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

  rv = builder->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString sql;
  rv = builder->ToString(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(rowCount == 1, NS_ERROR_UNEXPECTED);

  rv = result->GetRowCell(0, 0, mGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize our base classes
  // XXXben You can't call Init here unless this library's mPropertyCache has
  //        been created.
  rv = sbLocalDatabaseMediaListBase::Init(this, aDatabaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitResourceProperty(mPropertyCache, mGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize the media list factory table.
  PRBool success = mMediaListFactoryTable.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  rv = RegisterDefaultMediaListFactories();
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize the media item table.
  success = mMediaItemTable.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult
sbLocalDatabaseLibrary::CreateQueries()
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - CreateQueries()", this));
  nsresult rv;

  // Build some queries
  nsCOMPtr<sbISQLSelectBuilder> builder =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(NS_LITERAL_STRING("_mlt"),
                          NS_LITERAL_STRING("type"));
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

  rv = builder->ToString(mGetTypeForGUIDQuery);
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

  rv = builder->ToString(mGetMediaItemIdForGUIDQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Build new media item query
  nsCOMPtr<sbISQLInsertBuilder> insert =
    do_CreateInstance(SB_SQLBUILDER_INSERT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->SetIntoTableName(NS_LITERAL_STRING("media_items"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 columnCount = countof(kInsertQueryColumns);
  for (PRUint32 index = 0; index < columnCount; index++) {
    nsCAutoString columnName(kInsertQueryColumns[index]);
    rv = insert->AddColumn(NS_ConvertUTF8toUTF16(columnName));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  for (PRUint32 index = 0; index < columnCount; index++) {
    rv = insert->AddValueParameter();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = insert->ToString(mInsertMediaItemQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Build a query to get all the registered media list factories.
  rv = builder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);
   
  rv = builder->AddColumn(EmptyString(),
                          NS_LITERAL_STRING("media_list_type_id"));
  NS_ENSURE_SUCCESS(rv, rv);
   
  rv = builder->AddColumn(EmptyString(),
                          NS_LITERAL_STRING("type"));
  NS_ENSURE_SUCCESS(rv, rv);
   
  rv = builder->AddColumn(EmptyString(),
                          NS_LITERAL_STRING("factory_contractid"));
  NS_ENSURE_SUCCESS(rv, rv);
   
  rv = builder->SetBaseTableName(NS_LITERAL_STRING("media_list_types"));
  NS_ENSURE_SUCCESS(rv, rv);
   
  rv = builder->AddOrder(EmptyString(),
                         NS_LITERAL_STRING("media_list_type_id"),
                         PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->ToString(mMediaListFactoriesQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Make mInsertMediaListFactoryQuery.
  rv = insert->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->SetIntoTableName(NS_LITERAL_STRING("media_list_types"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("type"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("factory_contractid"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->ToString(mInsertMediaListFactoryQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create item delete query
  nsCOMPtr<sbISQLDeleteBuilder> deleteb =
    do_CreateInstance(SB_SQLBUILDER_DELETE_CONTRACTID, &rv);

  rv = deleteb->SetTableName(NS_LITERAL_STRING("media_items"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->CreateMatchCriterionParameter(EmptyString(),
                                              NS_LITERAL_STRING("guid"),
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

  // Create mGetFactoryIDForTypeQuery
  rv = builder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(),
                          NS_LITERAL_STRING("media_list_type_id"));
  NS_ENSURE_SUCCESS(rv, rv);
   
  rv = builder->SetBaseTableName(NS_LITERAL_STRING("media_list_types"));
  NS_ENSURE_SUCCESS(rv, rv);
   
  rv = builder->CreateMatchCriterionParameter(EmptyString(),
                                              NS_LITERAL_STRING("type"),
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);
   
  rv = builder->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);
   
  rv = builder->ToString(mGetFactoryIDForTypeQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * \brief A simple routine to construct a sbIDatabaseQuery object with some
 *        generic initialization.
 */
/* inline */ nsresult
sbLocalDatabaseLibrary::MakeStandardQuery(sbIDatabaseQuery** _retval)
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - MakeStandardQuery()", this));
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

/**
 * \brief Make a string of the current time in milliseconds.
 */
/* inline */ void
sbLocalDatabaseLibrary::GetNowString(nsAString& _retval) {
  TRACE(("LocalDatabaseLibrary[0x%.8x] - GetNowString()", this));
  char buf[30];
  PRUint32 len = PR_snprintf(buf, sizeof(buf), "%lld",
                             (PRUint64)(PR_Now() / PR_USEC_PER_MSEC));
  _retval.Assign(NS_ConvertASCIItoUTF16(buf, len));
}

/**
 * \brief Creates a new item in the database for the CreateMediaItem and
 *        CreateMediaList methods.
 *
 * \param aItemType - An integer specifying the type of item to create. This
 *                    will be zero for normal media items and nonzero for all
 *                    media lists.
 * \param aURISpecOrPrefix - The URI spec for media items or the URI prefix for
 *                           media lists.
 * \param _retval - The GUID that was generated, set only if the function
 *                  succeeds.
 */
nsresult
sbLocalDatabaseLibrary::AddNewItemQuery(sbIDatabaseQuery* aQuery,
                                        const PRUint32 aMediaItemTypeID,
                                        const nsAString& aURISpecOrPrefix,
                                        nsAString& _retval)
{
  NS_ENSURE_ARG_POINTER(aQuery);

  TRACE(("LocalDatabaseLibrary[0x%.8x] - AddNewItemQuery(%d, %s)", this,
         aMediaItemTypeID, NS_LossyConvertUTF16toASCII(aURISpecOrPrefix).get()));

  nsresult rv = aQuery->AddQuery(mInsertMediaItemQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Make a new GUID for the new media list.
  nsCOMPtr<nsIUUIDGenerator> uuidGen =
    do_GetService(NS_UUID_GENERATOR_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsID id;
  rv = uuidGen->GenerateUUIDInPlace(&id);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString guid(NS_ConvertUTF8toUTF16(id.ToString()));
  guid.Assign(Substring(guid, 1, guid.Length() - 2));

  // ToString adds curly braces to the GUID which we don't want.
  rv = aQuery->BindStringParameter(0, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set created and updated timestamps.
  nsAutoString createdTimeString;
  GetNowString(createdTimeString);

  rv = aQuery->BindStringParameter(1, createdTimeString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aQuery->BindStringParameter(2, createdTimeString);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the new URI spec and media item type.
  if (aMediaItemTypeID == SB_MEDIAITEM_TYPEID) {
    // This is a regular media item so use the spec that was passed in.
    rv = aQuery->BindStringParameter(3, aURISpecOrPrefix);
    NS_ENSURE_SUCCESS(rv, rv);

    // Media items don't have a media_list_type_id.
    rv = aQuery->BindNullParameter(4);
    NS_ENSURE_SUCCESS(rv, rv);
 }
  else {
    // This is a media list, so use the value that was passed in as a prefix
    // for the GUID.
    nsAutoString newSpec;
    newSpec.Assign(aURISpecOrPrefix);
    newSpec.Append(guid);

    rv = aQuery->BindStringParameter(3, newSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    // Record the media list type.
    rv = aQuery->BindInt32Parameter(4, aMediaItemTypeID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  _retval.Assign(guid);
  return NS_OK;
}

/**
 * \brief Returns the media list type for the given GUID.
 *
 * \param aGUID - The GUID to look up.
 *
 * \return The media list type. May return an empty string if the GUID
 *         represents a non-media list entry.
 */
nsresult
sbLocalDatabaseLibrary::GetTypeForGUID(const nsAString& aGUID,
                                       nsAString& _retval)
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - GetTypeForGUID(%s)", this,
         NS_LossyConvertUTF16toASCII(aGUID).get()));

  // See if we have already cached this GUID's type.
  sbMediaItemInfo* itemInfo;
  if (!mMediaItemTable.Get(aGUID, &itemInfo)) {
    // Make a new itemInfo for this GUID.
    nsAutoPtr<sbMediaItemInfo> newItemInfo(new sbMediaItemInfo());
    NS_ENSURE_TRUE(newItemInfo, NS_ERROR_OUT_OF_MEMORY);

    PRBool success = mMediaItemTable.Put(aGUID, newItemInfo);
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

    itemInfo = newItemInfo.forget();
  }
  else if (itemInfo->hasListType) {
    LOG((LOG_SUBMESSAGE_SPACE "Found type in cache!"));

    _retval.Assign(itemInfo->listType);
    return NS_OK;
  }

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mGetTypeForGUIDQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, aGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbresult;
  rv = query->Execute(&dbresult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbresult, dbresult);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(rowCount, NS_ERROR_NOT_AVAILABLE);

  nsString type;
  rv = result->GetRowCell(0, 0, type);
  NS_ENSURE_SUCCESS(rv, rv);

  itemInfo->listType.Assign(type);
  itemInfo->hasListType = PR_TRUE;

  _retval.Assign(type);
  return NS_OK;
}

/**
 * \brief Adds the types of all registered media list factories to an array.
 */
/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseLibrary::AddTypesToArrayCallback(nsStringHashKey::KeyType aKey,
                                                sbMediaListFactoryInfo* aEntry,
                                                void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in the hash?!");
  NS_ASSERTION(aUserData, "Null userData!");

#ifdef PR_LOGGING
  NS_ASSERTION(aEntry->factory, "Null factory!");

  nsCAutoString contractID;
  aEntry->factory->GetContractID(contractID);

  TRACE(("LocalDatabaseLibrary - AddTypesToArrayCallback(%s, %s)",
         NS_LossyConvertUTF16toASCII(aKey).get(), contractID.get()));
#endif
  
  // Make a string enumerator for the string array.
  nsTArray<nsString>* array =
    NS_STATIC_CAST(nsTArray<nsString>*, aUserData);
  NS_ENSURE_TRUE(array, PL_DHASH_STOP);
  
  nsString* newElement = array->AppendElement(aKey);
  NS_ENSURE_TRUE(newElement, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

/**
 * \brief Registers this library's default media list factories.
 */
nsresult
sbLocalDatabaseLibrary::RegisterDefaultMediaListFactories()
{
  nsCOMPtr<sbIMediaListFactory> factory;
  NS_NEWXPCOM(factory, sbLocalDatabaseSimpleMediaListFactory);
  NS_ENSURE_TRUE(factory, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = RegisterMediaListFactory(factory);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * \brief Removes an item from the database.
 */
nsresult
sbLocalDatabaseLibrary::DeleteDatabaseItem(const nsAString& aGuid)
{
  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mDeleteItemQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, aGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  return NS_OK;
}

/**
 * \brief
 */
nsresult
sbLocalDatabaseLibrary::AddItemToLocalDatabase(sbIMediaItem* aMediaItem)
{
  nsresult rv;

  // Create a new media item and copy all the properties
  nsCOMPtr<nsIURI> contentUri;
  rv = aMediaItem->GetContentSrc(getter_AddRefs(contentUri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> newItem;
  rv = CreateMediaItem(contentUri, getter_AddRefs(newItem));
  NS_ENSURE_SUCCESS(rv, rv);

  // TODO: Copy properties

  return NS_OK;
}

/**
 * See sbILocalDatabaseLibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetContractIdForGuid(const nsAString& aGUID,
                                             nsACString& aContractID)
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - GetContractIdForGuid(%s)", this,
         NS_LossyConvertUTF16toASCII(aGUID).get()));
  nsAutoString mediaType;
  nsresult rv = GetTypeForGUID(aGUID, mediaType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mediaType.IsEmpty()) {
    // This is a regular media item with no contractID.
    aContractID.Truncate();
    return NS_OK;
  }

  sbMediaListFactoryInfo* factoryInfo;
  PRBool typeRegistered = mMediaListFactoryTable.Get(mediaType, &factoryInfo);
  NS_ENSURE_TRUE(typeRegistered, NS_ERROR_UNEXPECTED);

  NS_ASSERTION(factoryInfo->factory, "Null factory pointer!");
  nsCAutoString contractID;
  rv = factoryInfo->factory->GetContractID(contractID);
  NS_ENSURE_SUCCESS(rv, rv);

  aContractID.Assign(contractID);
  return NS_OK;
}

/**
 * See sbILocalDatabaseLibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetMediaItemIdForGuid(const nsAString& aGUID,
                                              PRUint32* aMediaItemID)
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - GetMediaItemIdForGuid(%s)", this,
         NS_LossyConvertUTF16toASCII(aGUID).get()));
  NS_ENSURE_ARG_POINTER(aMediaItemID);

  sbMediaItemInfo* itemInfo;
  if (!mMediaItemTable.Get(aGUID, &itemInfo)) {
    // Make a new itemInfo for this GUID.
    nsAutoPtr<sbMediaItemInfo> newItemInfo(new sbMediaItemInfo());
    NS_ENSURE_TRUE(newItemInfo, NS_ERROR_OUT_OF_MEMORY);

    PRBool success = mMediaItemTable.Put(aGUID, newItemInfo);
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

    itemInfo = newItemInfo.forget();
  }
  else if (itemInfo->hasItemID) {
    LOG(("                                 - Found ID in cache!"));
    *aMediaItemID = itemInfo->itemID;
    return NS_OK;
  }

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mGetMediaItemIdForGUIDQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, aGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbresult;
  rv = query->Execute(&dbresult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbresult, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Some callers (e.g. Contains) listen specifically for
  // NS_ERROR_NOT_AVAILABLE to mean that the item does not exist in the table.
  NS_ENSURE_TRUE(rowCount, NS_ERROR_NOT_AVAILABLE);

  nsAutoString idString;
  rv = result->GetRowCell(0, 0, idString);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 id = idString.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  itemInfo->itemID = id;
  itemInfo->hasItemID = PR_TRUE;

  *aMediaItemID = id;
  return NS_OK;
}

/**
 * See sbILocalDatabaseLibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetDatabaseGuid(nsAString& aDatabaseGuid)
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - GetDatabaseGuid()", this));
  aDatabaseGuid = mDatabaseGuid;
  return NS_OK;
}

/**
 * See sbILocalDatabaseLibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetDatabaseLocation(nsIURI** aDatabaseLocation)
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - GetDatabaseLocation()", this));
  NS_ENSURE_ARG_POINTER(aDatabaseLocation);

  if (!mDatabaseLocation) {
    *aDatabaseLocation = nsnull;
    return NS_OK;
  }

  nsresult rv = mDatabaseLocation->Clone(aDatabaseLocation);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbILocalDatabaseLibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetPropertyCache(sbILocalDatabasePropertyCache** aPropertyCache)
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - GetPropertyCache()", this));
  NS_ENSURE_ARG_POINTER(aPropertyCache);
  NS_ADDREF(*aPropertyCache = mPropertyCache);
  return NS_OK;
}

/**
 * See sbILocalDatabaseLibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetPropertiesForGuid(const nsAString& aGuid,
                                             sbILocalDatabaseResourcePropertyBag** aPropertyBag)
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - GetPropertiesForGuid(%s)", this,
         NS_LossyConvertUTF16toASCII(aGuid).get()));
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

  NS_Free(bags);
  delete[] guids;

  if (*aPropertyBag) {
    return NS_OK;
  }
  else {
    return NS_ERROR_NOT_AVAILABLE;
  }

}

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetDevice(sbIDevice** aDevice)
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - GetDevice()", this));
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetSupportsForeignMediaItems(PRBool* aSupportsForeignMediaItems)
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - GetSupportsForeignMediaItems()", this));
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetCreationParameters(nsIPropertyBag2** aCreationParameters)
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - GetCreationParameters()", this));
  NS_ENSURE_ARG_POINTER(aCreationParameters);
  NS_ENSURE_STATE(mCreationParameters);

  NS_ADDREF(*aCreationParameters = mCreationParameters);
  return NS_OK;
}

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetFactory(sbILibraryFactory** aFactory)
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - GetFactory()", this));
  NS_ENSURE_ARG_POINTER(aFactory);
  NS_ENSURE_STATE(mFactory);

  NS_ADDREF(*aFactory = mFactory);
  return NS_OK;
}

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::Resolve(nsIURI* aUri,
                                nsIChannel** _retval)
{
  NS_ENSURE_ARG_POINTER(aUri);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCAutoString spec;
  nsresult rv = aUri->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  TRACE(("LocalDatabaseLibrary[0x%.8x] - Resolve(%s)", this, spec.get()));
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::CreateMediaItem(nsIURI* aUri,
                                        sbIMediaItem** _retval)
{
  NS_ENSURE_ARG_POINTER(aUri);
  NS_ENSURE_ARG_POINTER(_retval);

  // TODO: Check for duplicates?

  nsCAutoString spec;
  nsresult rv = aUri->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  TRACE(("LocalDatabaseLibrary[0x%.8x] - CreateMediaItem(%s)", this,
         spec.get()));

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString guid;
  rv = AddNewItemQuery(query,
                       SB_MEDIAITEM_TYPEID,
                       NS_ConvertUTF8toUTF16(spec),
                       guid);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  // Add the new media item into cache
  nsAutoPtr<sbMediaItemInfo> newItemInfo(new sbMediaItemInfo());
  NS_ENSURE_TRUE(newItemInfo, NS_ERROR_OUT_OF_MEMORY);

  newItemInfo->hasListType = PR_TRUE;

  NS_ASSERTION(!mMediaItemTable.Get(guid, nsnull),
               "Guid already exists!");

  PRBool success = mMediaItemTable.Put(guid, newItemInfo);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  newItemInfo.forget();

  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = GetMediaItem(guid, getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  NotifyListenersItemAdded(this, mediaItem);

  NS_ADDREF(*_retval = mediaItem);
  return NS_OK;
}

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::CreateMediaList(const nsAString& aType,
                                        sbIMediaList** _retval)
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - CreateMediaList(%s)", this,
         NS_LossyConvertUTF16toASCII(aType).get()));
  NS_ENSURE_ARG_POINTER(_retval);

  sbMediaListFactoryInfo* factoryInfo;
  PRBool validType = mMediaListFactoryTable.Get(aType, &factoryInfo);
  NS_ENSURE_TRUE(validType, NS_ERROR_INVALID_ARG);

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString guid;
  rv = AddNewItemQuery(query, factoryInfo->typeID, aType, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  // Add the new media list into cache
  nsAutoPtr<sbMediaItemInfo> newItemInfo(new sbMediaItemInfo());
  NS_ENSURE_TRUE(newItemInfo, NS_ERROR_OUT_OF_MEMORY);

  newItemInfo->listType.Assign(aType);
  newItemInfo->hasListType = PR_TRUE;

  NS_ASSERTION(!mMediaItemTable.Get(guid, nsnull),
               "Guid already exists!");

  PRBool success = mMediaItemTable.Put(guid, newItemInfo);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  newItemInfo.forget();

  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = GetMediaItem(guid, getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  NotifyListenersItemAdded(this, mediaItem);

  nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(mediaItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = mediaList);
  return NS_OK;
}

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetMediaItem(const nsAString& aGUID,
                                     sbIMediaItem** _retval)
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - GetMediaItem(%s)", this,
         NS_LossyConvertUTF16toASCII(aGUID).get()));
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  nsCOMPtr<sbIMediaItem> strongMediaItem;

  sbMediaItemInfo* itemInfo;
  if (!mMediaItemTable.Get(aGUID, &itemInfo)) {
    // Make a new itemInfo for this GUID.
    nsAutoPtr<sbMediaItemInfo> newItemInfo(new sbMediaItemInfo());
    NS_ENSURE_TRUE(newItemInfo, NS_ERROR_OUT_OF_MEMORY);

    PRBool success = mMediaItemTable.Put(aGUID, newItemInfo);
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

    itemInfo = newItemInfo.forget();
  }
  else if (itemInfo->weakRef) {
    strongMediaItem = do_QueryReferent(itemInfo->weakRef, &rv);
    if (NS_SUCCEEDED(rv)) {
      // This item is still owned by someone so we don't have to create it
      // again. Add a ref and let it live a little longer.
      LOG((LOG_SUBMESSAGE_SPACE "Found live weak reference in cache"));
      NS_ADDREF(*_retval = strongMediaItem);

      // That's all we have to do here.
      return NS_OK;
    }

    // Our cached item has died... Remove it from the hash table and create it
    // all over again.
    LOG((LOG_SUBMESSAGE_SPACE "Found dead weak reference in cache"));
    itemInfo->weakRef = nsnull;
  }

  nsAutoPtr<sbLocalDatabaseMediaItem>
    newMediaItem(new sbLocalDatabaseMediaItem());
  NS_ENSURE_TRUE(newMediaItem, NS_ERROR_OUT_OF_MEMORY);

  rv = newMediaItem->Init(this, aGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  strongMediaItem = newMediaItem.forget();

  // Get the type for the guid.
  nsAutoString type;
  if (itemInfo->hasListType) {
    type.Assign(itemInfo->listType);
  }
  else {
    rv = GetTypeForGUID(aGUID, type);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ASSERTION(itemInfo->hasListType, "Should have set a list type there!");

  if (!type.IsEmpty()) {
    // This must be a media list, so get the appropriate factory.
    sbMediaListFactoryInfo* factoryInfo;
    PRBool success = mMediaListFactoryTable.Get(type, &factoryInfo);
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

    NS_ASSERTION(factoryInfo->factory, "Null factory!");

    nsCOMPtr<sbIMediaList> mediaList;
    rv = factoryInfo->factory->CreateMediaList(strongMediaItem,
                                               getter_AddRefs(mediaList));
    NS_ENSURE_SUCCESS(rv, rv);

    // Replace the new media item. If the media list cared about it then it
    // should have added a reference to keep it alive.
    strongMediaItem = do_QueryInterface(mediaList, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  itemInfo->weakRef = do_GetWeakReference(strongMediaItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = strongMediaItem);
  return NS_OK;
}

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetMediaListTypes(nsIStringEnumerator** aMediaListTypes)
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - GetMediaListTypes(%s)", this));
  NS_ENSURE_ARG_POINTER(aMediaListTypes);

  // Array to hold all the registered types.
  nsTArray<nsString> typeArray;

  PRUint32 keyCount = mMediaListFactoryTable.Count();
  PRUint32 enumCount =
    mMediaListFactoryTable.EnumerateRead(AddTypesToArrayCallback, &typeArray);
  NS_ENSURE_TRUE(enumCount == keyCount, NS_ERROR_FAILURE);

  nsCOMPtr<nsIStringEnumerator> enumerator =
    new sbTArrayStringEnumerator(&typeArray);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aMediaListTypes = enumerator);
  return NS_OK;
}

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::RegisterMediaListFactory(sbIMediaListFactory* aFactory)
{
  NS_ENSURE_ARG_POINTER(aFactory);

  nsAutoString type;
  nsresult rv = aFactory->GetType(type);
  NS_ENSURE_SUCCESS(rv, rv);

  TRACE(("LocalDatabaseLibrary[0x%.8x] - RegisterMediaListFactory(%s)", this,
         NS_LossyConvertUTF16toASCII(type).get()));

  // Bail if we've already registered this type.
  PRBool alreadyRegistered = mMediaListFactoryTable.Get(type, nsnull);
  if (alreadyRegistered) {
    NS_WARNING("Registering a media list factory that was already registered!");
    return NS_OK;
  }

  // See if this type has already been registered into the database.
  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mGetFactoryIDForTypeQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, type);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbresult;
  rv = query->Execute(&dbresult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbresult, dbresult);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Make sure that there is no more than one entry for this factory type.
  NS_ASSERTION(rowCount <= 1,
               "More than one entry for this type in the database!");

  if (!rowCount) {
    // This is a brand new media list type that our database has never seen.
    // Make a query to insert it into the database.
    rv = query->ResetQuery();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->AddQuery(mInsertMediaListFactoryQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindStringParameter(0, type);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCAutoString contractID;
    rv = aFactory->GetContractID(contractID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindStringParameter(1, NS_ConvertASCIItoUTF16(contractID));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->Execute(&dbresult);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_SUCCESS(dbresult, dbresult);

    // Get the newly created typeID for the factory.
    rv = query->ResetQuery();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->AddQuery(NS_LITERAL_STRING("select last_insert_rowid()"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->Execute(&dbresult);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_SUCCESS(dbresult, dbresult);

    nsCOMPtr<sbIDatabaseResult> result;
    rv = query->GetResultObject(getter_AddRefs(result));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsAutoString newTypeIDString;
  rv = result->GetRowCell(0, 0, newTypeIDString);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 newTypeID = newTypeIDString.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Make a new object to hold this info.
  nsAutoPtr<sbMediaListFactoryInfo>
    factoryInfo(new sbMediaListFactoryInfo(newTypeID, aFactory));
  NS_ENSURE_TRUE(factoryInfo, NS_ERROR_OUT_OF_MEMORY);

  // And add it to our hash table.
  PRBool success = mMediaListFactoryTable.Put(type, factoryInfo);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  factoryInfo.forget();
  return NS_OK;
}

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::Optimize()
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - Optimize()", this));

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetAsyncQuery(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(NS_LITERAL_STRING("VACUUM"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(NS_LITERAL_STRING("ANALYZE"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddSimpleQueryCallback(this);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbresult;
  rv = query->Execute(&dbresult);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(dbresult == 0, NS_ERROR_FAILURE);
  
  return NS_OK;
}

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::Shutdown()
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - Shutdown()", this));
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::BatchCreateMediaItems(nsIArray* aURIList,
                                              nsIArray** _retval)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(aURIList);
  NS_ENSURE_ARG_POINTER(_retval);

  TRACE(("LocalDatabaseLibrary[0x%.8x] - BatchCreateMediaItems()", this));
  
  // How many URI?
  PRUint32 listLength;
  rv = aURIList->GetLength(&listLength);
  NS_ENSURE_SUCCESS(rv, rv);

  // Make an array of their specs
  nsTArray<nsString> specs;
  for (PRUint32 i = 0; i < listLength; i++) {

    nsCOMPtr<nsIURI> uri = do_QueryElementAt(aURIList, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCAutoString spec;
    rv = uri->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsString* success = specs.AppendElement(NS_ConvertUTF8toUTF16(spec));
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  // Convert that into an array of PRUnichar.  PLEAH!!!!
  const PRUnichar **URLArray = (const PRUnichar **)nsMemory::Alloc( sizeof( PRUnichar * ) * listLength );
  for (PRUint32 i = 0; i < listLength; i++)
    URLArray[ i ] = PromiseFlatString( specs[ i ] ).get();
  
  // Setup the outparams
  nsCOMPtr<nsIArray> addedGuids, newItems;
  nsCOMPtr<sbIDatabaseQuery> query;

  // Compose the query
  rv = BatchCreateMediaItemsQuery(listLength, URLArray, getter_AddRefs(addedGuids), getter_AddRefs(query));
  nsMemory::Free( URLArray ); // Free the memory before checking the return value!!!!!!!
  NS_ENSURE_SUCCESS(rv, rv);

  // Execute the query
  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  // Get the media items after the database changes commit.
  rv = BatchGetMediaItems( addedGuids, getter_AddRefs(newItems) );
  NS_ENSURE_SUCCESS(rv, rv);

  // Tell the world they were created.
  rv = BatchNotifyAdded( newItems );
  NS_ENSURE_SUCCESS(rv, rv);

  // Return the array of media items
  NS_ADDREF(*_retval = newItems);
  return NS_OK;
}

/**
 * See sbILibrary
 */

/* sbIDatabaseQuery batchCreateMediaItemsQuery (in unsigned long aURLCount, [array, size_is (aURLCount)] in wstring aURLArray, out nsIArray aGUIDArray); */
NS_IMETHODIMP 
sbLocalDatabaseLibrary::BatchCreateMediaItemsQuery(PRUint32 aURLCount, const PRUnichar **aURLArray, nsIArray **aGUIDArray, sbIDatabaseQuery **_retval)
{
  NS_ENSURE_ARG_POINTER(aURLArray);
  NS_ENSURE_ARG_POINTER(_retval);

  TRACE(("LocalDatabaseLibrary[0x%.8x] - BatchCreateMediaItems()", this));

  // Create the query that we shall return
  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  // Begin a batch
  rv = query->AddQuery(NS_LITERAL_STRING("begin"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the array of strings that we shall return
  nsCOMPtr<nsIMutableArray> array = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // For all urls in the array
  for (PRUint32 i = 0; i < aURLCount; i++) {
    // Compose the query to add the given url
    nsAutoString guid;
    rv = AddNewItemQuery(query,
                         SB_MEDIAITEM_TYPEID,
                         nsAutoString(aURLArray[i]),
                         guid); 
    NS_ENSURE_SUCCESS(rv, rv);

    TRACE(("LocalDatabaseLibrary[0x%.8x] - Load() Loaded(%s, %s)", this,
           aURLArray[i], NS_LossyConvertUTF16toASCII(guid).get()));

    if ( aGUIDArray ) // Optional paramater if you don't care.
    {
      // And add the returned guid string to the outparam array
      nsCOMPtr<nsISupportsString> guidStr(do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID));
      NS_ENSURE_TRUE(guidStr, NS_ERROR_FAILURE);
      guidStr->SetData(guid);
      rv = array->AppendElement(guidStr, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Flush the database transaction every 500 inserts.
    // This value was empirically tested to give the fastest results on a 100k medialist.
    if ( ( i + 1 ) % 500 == 0 ) {
      rv = query->AddQuery(NS_LITERAL_STRING("commit"));
      rv = query->AddQuery(NS_LITERAL_STRING("begin"));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // End the batch
  rv = query->AddQuery(NS_LITERAL_STRING("commit"));
  NS_ENSURE_SUCCESS(rv, rv);

  // And return what we need to
  if ( aGUIDArray )
    NS_ADDREF(*aGUIDArray = array);
  NS_ADDREF(*_retval = query);
  return NS_OK;
}

/* nsIArray batchGetMediaItems (in nsIArray aGUIDArray); */
NS_IMETHODIMP 
sbLocalDatabaseLibrary::BatchGetMediaItems(nsIArray *aGUIDArray, nsIArray **_retval) {
  nsresult rv;
  // Generate the return array from the addedGuids
  nsCOMPtr<nsIMutableArray> array = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  {
    sbAutoBatchHelper batchHelper(this);

    // How many GUID?
    PRUint32 length;
    rv = aGUIDArray->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    // Copy them over, please.
    for (PRUint32 i = 0; i < length; i++) {
      // Break out the lame guid string
      nsCOMPtr<nsISupportsString> guidStr = do_QueryElementAt(aGUIDArray, i, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      // To the autostring
      nsAutoString guid;
      rv = guidStr->GetData( guid );
      NS_ENSURE_SUCCESS(rv, rv);

      // We know the GUID and the type of these new media items so preload
      // the cache with this information, so getting the media item is faster.
      nsAutoPtr<sbMediaItemInfo> newItemInfo(new sbMediaItemInfo());
      NS_ENSURE_TRUE(newItemInfo, NS_ERROR_OUT_OF_MEMORY);
      newItemInfo->hasListType = PR_TRUE;
      NS_ASSERTION(!mMediaItemTable.Get(guid, nsnull),
                    "Guid already exists!");
      PRBool success = mMediaItemTable.Put(guid, newItemInfo);
      NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
      newItemInfo.forget();

      // Then make a media item
      nsCOMPtr<sbIMediaItem> mediaItem;
      rv = GetMediaItem(guid, getter_AddRefs(mediaItem));
      NS_ENSURE_SUCCESS(rv, rv);

      // Add that to the return array
      rv = array->AppendElement(mediaItem, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  NS_ADDREF(*_retval = array);
  return NS_OK;
}

/* void batchNotifyAdded (in nsIArray aMediaItemArray); */
NS_IMETHODIMP 
sbLocalDatabaseLibrary::BatchNotifyAdded(nsIArray *aMediaItemArray) {
  nsresult rv;
  // Generate the notifications from the aMediaItemArray
  sbAutoBatchHelper batchHelper(this);

  // How many GUID?
  PRUint32 length;
  rv = aMediaItemArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  // Copy them over, please.
  for (PRUint32 i = 0; i < length; i++) {
    // Break out the media item
    nsCOMPtr<sbIMediaItem> mediaItem = do_QueryElementAt(aMediaItemArray, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // And notify the world that we changed this.
    NotifyListenersItemAdded(this, mediaItem);
  }

  return NS_OK;
}

/**
 * See sbIMediaList
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetItemByGuid(const nsAString& aGuid,
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

/**
 * See sbIMediaList
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::Contains(sbIMediaItem* aMediaItem,
                                 PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsAutoString guid;
  nsresult rv = aMediaItem->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mediaItemId;
  rv = GetMediaItemIdForGuid(guid, &mediaItemId);
  if (NS_FAILED(rv) && (rv != NS_ERROR_NOT_AVAILABLE)) {
    NS_WARNING("GetMediaItemIdForGuid failed");
    return rv;
  }

  *_retval = NS_SUCCEEDED(rv);
  return NS_OK;
}

/**
 * See sbIMediaList
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::Add(sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  // If the media item's library and this list's library are the same, this
  // item must already be in this database.
  nsCOMPtr<sbILibrary> itemLibrary;
  nsresult rv = aMediaItem->GetLibrary(getter_AddRefs(itemLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool equals;
  rv = itemLibrary->Equals(SB_ILIBRESOURCE_CAST(this), &equals);
  NS_ENSURE_SUCCESS(rv, rv);

  if (equals) {
    return NS_OK;
  }

  // Import this item into this library
  rv = AddItemToLocalDatabase(aMediaItem);
  NS_ENSURE_SUCCESS(rv, rv);

  // Invalidate the cached list
  rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  // And let everyone know about it.
  NotifyListenersItemAdded(this, aMediaItem);

  return NS_OK;
}

/**
 * See sbIMediaList
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::AddAll(sbIMediaList* aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  sbAutoBatchHelper batchHelper(this);

  sbLibraryInsertingEnumerationListener listener(this);
  nsresult rv = EnumerateAllItems(&listener, sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbIMediaList
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::AddSome(nsISimpleEnumerator* aMediaItems)
{
  NS_ENSURE_ARG_POINTER(aMediaItems);

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  sbLibraryInsertingEnumerationListener listener(this);

  PRBool beginEnumeration;
  nsresult rv = listener.OnEnumerationBegin(nsnull, &beginEnumeration);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(beginEnumeration, NS_ERROR_ABORT);

  sbAutoBatchHelper batchHelper(this);

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

/**
 * See sbIMediaList
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::Remove(sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  // Use our enumeration listener to make this use the same code as all the
  // other remove methods.
  sbLibraryRemovingEnumerationListener listener(this);

  nsresult rv = listener.OnEnumerationBegin(nsnull, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = listener.OnEnumeratedItem(nsnull, aMediaItem, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = listener.OnEnumerationEnd(nsnull, NS_OK);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbIMediaList
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::RemoveByIndex(PRUint32 aIndex)
{
  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  // Hrm, we actually have to instantiate an item to notify the listeners here.
  // Hopefully it's cached.
  nsAutoString guid;
  nsresult rv = mFullArray->GetByIndex(aIndex, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = GetMediaItem(guid, getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = Remove(mediaItem);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbIMediaList
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::RemoveSome(nsISimpleEnumerator* aMediaItems)
{
  NS_ENSURE_ARG_POINTER(aMediaItems);

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  sbAutoBatchHelper batchHelper(this);

  // Use our enumeration listener to make this use the same code as all the
  // other remove methods.
  sbLibraryRemovingEnumerationListener listener(this);

  nsresult rv = listener.OnEnumerationBegin(nsnull, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore;
  while (NS_SUCCEEDED(aMediaItems->HasMoreElements(&hasMore)) && hasMore) {

    nsCOMPtr<nsISupports> supports;
    rv = aMediaItems->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaItem> item = do_QueryInterface(supports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = listener.OnEnumeratedItem(nsnull, item, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = listener.OnEnumerationEnd(nsnull, NS_OK);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbIMediaList
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::Clear()
{
  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  NotifyListenersListCleared(this);

  // Clear our cache
  mMediaItemTable.Clear();

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mDeleteAllQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  // Invalidate the cached list
  rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbIMediaList
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::CreateView(sbIMediaListView** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsCOMPtr<sbIMediaList> mediaList =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbILocalDatabaseLibrary*, this), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString prop;
  prop.AssignLiteral(DEFAULT_SORT_PROPERTY);

  nsAutoPtr<sbLocalDatabaseMediaListView>
    view(new sbLocalDatabaseMediaListView(this, mediaList, prop, 0));
  NS_ENSURE_TRUE(view, NS_ERROR_OUT_OF_MEMORY);

  rv = view->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = view.forget());
  return NS_OK;
}

/**
 * See 
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetDefaultSortProperty(nsAString& aProperty)
{
  aProperty.AssignLiteral(DEFAULT_SORT_PROPERTY);
  return NS_OK;
}

/**
 * See sbIDatabaseSimpleQueryCallback
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::OnQueryEnd(sbIDatabaseResult* aDBResultObject,
                                   const nsAString& aDBGUID,
                                   const nsAString& aQuery)
{
  NS_ASSERTION(aQuery.Find("VACUUM") != -1, "Got the wrong callback!");

  return NS_OK;
}

/**
 * See nsIClassInfo
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetInterfaces(PRUint32* count, nsIID*** array)
{
  return NS_CI_INTERFACE_GETTER_NAME(sbLocalDatabaseLibrary)(count, array);
}

/**
 * See nsIClassInfo
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetHelperForLanguage(PRUint32 language,
                                             nsISupports** _retval)
{
  *_retval = nsnull;
  return NS_OK;
}

/**
 * See nsIClassInfo
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetContractID(char** aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

/**
 * See nsIClassInfo
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetClassDescription(char** aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

/**
 * See nsIClassInfo
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetClassID(nsCID** aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

/**
 * See nsIClassInfo
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetImplementationLanguage(PRUint32* aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

/**
 * See nsIClassInfo
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetFlags(PRUint32 *aFlags)
{
  *aFlags = 0;
  return NS_OK;
}

/**
 * See nsIClassInfo
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  return NS_ERROR_NOT_AVAILABLE;
}

