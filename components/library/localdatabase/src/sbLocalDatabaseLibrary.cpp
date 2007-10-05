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
#include <nsIClassInfoImpl.h>
#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsIMutableArray.h>
#include <nsIObserverService.h>
#include <nsIPrefBranch.h>
#include <nsIPrefService.h>
#include <nsIProgrammingLanguage.h>
#include <nsIPropertyBag2.h>
#include <nsISimpleEnumerator.h>
#include <nsIStringEnumerator.h>
#include <nsISupportsPrimitives.h>
#include <nsIThread.h>
#include <nsIURI.h>
#include <nsIUUIDGenerator.h>
#include <sbIDatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <sbILibraryFactory.h>
#include <sbILibraryManager.h>
#include <sbILibraryResource.h>
#include <sbILocalDatabaseGUIDArray.h>
#include <sbILocalDatabasePropertyCache.h>
#include <sbILocalDatabaseSimpleMediaList.h>
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
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsXPCOM.h>
#include <nsWeakReference.h>
#include <prinrval.h>
#include <prlog.h>
#include <prprf.h>
#include <prtime.h>
#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseMediaItem.h"
#include "sbLocalDatabaseMediaListView.h"
#include "sbLocalDatabasePropertyCache.h"
#include "sbLocalDatabaseSimpleMediaListFactory.h"
#include "sbLocalDatabaseSchemaInfo.h"
#include "sbLocalDatabaseSmartMediaListFactory.h"
#include "sbLocalDatabaseGUIDArray.h"
#include <sbStandardProperties.h>
#include <sbSQLBuilderCID.h>
#include <sbTArrayStringEnumerator.h>

#define NS_UUID_GENERATOR_CONTRACTID "@mozilla.org/uuid-generator;1"

#define SB_MEDIAITEM_TYPEID 0

#define DEFAULT_ANALYZE_COUNT_LIMIT 1000
#define ANALYZE_COUNT_PREF "songbird.library.localdatabase.analyzeCountLimit"

#define SB_MEDIALIST_FACTORY_DEFAULT_TYPE 1
#define SB_MEDIALIST_FACTORY_URI_PREFIX   "medialist('"
#define SB_MEDIALIST_FACTORY_URI_SUFFIX   "')"

// The number of milliseconds that the library could possibly wait before
// checking to see if all the async batch create timers have finished on
// shutdown. This number isn't exact, see the documentation for
// NS_ProcessPendingEvents.
#define SHUTDOWN_ASYNC_GRANULARITY_MS 1000

// These macros need to stay in sync with the QueryInterface macro
#define SB_ILIBRESOURCE_CAST(_ptr)                                             \
  static_cast<sbILibraryResource*>(static_cast<sbIMediaItem*>(static_cast<sbLocalDatabaseMediaItem*>(_ptr)))
#define SB_IMEDIALIST_CAST(_ptr)                                               \
  static_cast<sbIMediaList*>(static_cast<sbLocalDatabaseMediaListBase*>(_ptr))
#define SB_IMEDIAITEM_CAST(_ptr)                                               \
  static_cast<sbIMediaItem*>(static_cast<sbLocalDatabaseMediaItem*>(_ptr))

#define DEFAULT_SORT_PROPERTY SB_PROPERTY_CREATED

#define DEFAULT_FETCH_SIZE 1000

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbLocalDatabaseLibrary:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gLibraryLog = nsnull;
#endif /* PR_LOGGING */

#define TRACE(args) PR_LOG(gLibraryLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gLibraryLog, PR_LOG_WARN, args)

// Makes some of the logging a little easier to read
#define LOG_SUBMESSAGE_SPACE "                                 - "

static char* kInsertQueryColumns[] = {
  "guid",
  "created",
  "updated",
  "content_url",
  "hidden",
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

  // If the media item is not in the library, add it
  PRBool equals;
  rv = fromLibrary->Equals(SB_ILIBRESOURCE_CAST(mFriendLibrary),
                           &equals);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!equals) {
    nsCOMPtr<sbIMediaItem> newMediaItem;
    rv = mFriendLibrary->AddItemToLocalDatabase(aMediaItem,
                                                getter_AddRefs(newMediaItem));
    NS_ENSURE_SUCCESS(rv, rv);

    // Remember this media item for later so we can notify with it
    PRBool success = mNotificationList.AppendObject(newMediaItem);
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
  nsresult rv;

  if (mShouldInvalidate) {
    NS_ASSERTION(mFriendLibrary->mFullArray, "Uh, no full array?!");

    rv = mFriendLibrary->mFullArray->Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbIMediaList> libraryList =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbILocalDatabaseLibrary*, mFriendLibrary), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Notify our listeners
  PRUint32 count = mNotificationList.Count();
  for (PRUint32 i = 0; i < count; i++) {
    mFriendLibrary->NotifyListenersItemAdded(libraryList,
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

  nsresult rv;
  nsCOMPtr<sbIMediaList> libraryList =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbILocalDatabaseLibrary*, mFriendLibrary), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // First we notify any simple media lists that contain this item
  sbMediaItemToListsMap map;
  PRBool success = map.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  sbMediaListArray lists;
  rv = mFriendLibrary->GetContainingLists(&mNotificationList, &lists, &map);
  NS_ENSURE_SUCCESS(rv, rv);

  map.EnumerateRead(sbLocalDatabaseLibrary::NotifyListsBeforeItemRemoved,
                    libraryList);

  PRUint32 count = mNotificationList.Count();
  for (PRUint32 i = 0; i < count; i++) {
    nsCOMPtr<sbIMediaItem> item = do_QueryInterface(mNotificationList[i], &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Notify explicitly registered listeners for each item removal
    mFriendLibrary->NotifyListenersBeforeItemRemoved(libraryList, item);

    nsAutoString guid;
    rv = item->GetGuid(guid);
    NS_ENSURE_SUCCESS(rv, rv);

    // Remove from our cache.
    mFriendLibrary->mMediaItemTable.Remove(guid);

    // And set up the database query to actually remove the item
    rv = mDBQuery->AddQuery(mFriendLibrary->mDeleteItemQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBQuery->BindStringParameter(0, guid);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mDBQuery->AddQuery(NS_LITERAL_STRING("commit"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbSuccess;
  rv = mDBQuery->Execute(&dbSuccess);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbSuccess, NS_ERROR_FAILURE);

  // Invalidate our guid array
  rv = mFriendLibrary->mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  // Invalidate all of the simple media lists we notified
  for (PRInt32 i = 0; i < lists.Count(); i++) {
    nsCOMPtr<sbILocalDatabaseSimpleMediaList> simple =
      do_QueryInterface(lists[i], &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = simple->Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Notify simple media lists after removal
  map.EnumerateRead(sbLocalDatabaseLibrary::NotifyListsAfterItemRemoved,
                    libraryList);

  // Notify our listeners of after removal
  for (PRUint32 i = 0; i < count; i++) {
    mFriendLibrary->NotifyListenersAfterItemRemoved(libraryList,
                                                    mNotificationList[i]);
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED5(sbLocalDatabaseLibrary, sbLocalDatabaseMediaListBase,
                                                     nsIClassInfo,
                                                     nsIObserver,
                                                     sbIDatabaseSimpleQueryCallback,
                                                     sbILibrary,
                                                     sbILocalDatabaseLibrary)

NS_IMPL_CI_INTERFACE_GETTER8(sbLocalDatabaseLibrary,
                             nsIClassInfo,
                             nsIObserver,
                             nsISupportsWeakReference,
                             sbIDatabaseSimpleQueryCallback,
                             sbILibrary,
                             sbILibraryResource,
                             sbIMediaItem,
                             sbIMediaList);

sbLocalDatabaseLibrary::sbLocalDatabaseLibrary()
: mDirtyItemCount(0),
  mAnalyzeCountLimit(DEFAULT_ANALYZE_COUNT_LIMIT),
  mPreventAddedNotification(PR_FALSE)
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

  nsresult rv;

  // Maybe check to this that this db is valid, etc?
  // Check version and migrate if needed?

  mDatabaseGuid = aDatabaseGuid;
  mCreationParameters = aCreationParameters;
  mFactory = aFactory;

  // This may be null.
  mDatabaseLocation = aDatabaseLocation;

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

  nsAutoString guid;
  rv = result->GetRowCell(0, 0, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set up our property cache
  nsRefPtr<sbLocalDatabasePropertyCache>
    propCache(new sbLocalDatabasePropertyCache());
  NS_ENSURE_TRUE(propCache, NS_ERROR_OUT_OF_MEMORY);

  rv = propCache->Init(this, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  mPropertyCache = propCache;

  mFullArray = new sbLocalDatabaseGUIDArray();
  NS_ENSURE_TRUE(mFullArray, NS_ERROR_OUT_OF_MEMORY);

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

  rv = mFullArray->SetPropertyCache(mPropertyCache);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CreateQueries();
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize our base classes.  Note that we tell sbLocalDatabaseMediaListBase
  // that we do not want an owning reference to be kept to the library since
  // we are the library.  This is done to prevent a cycle.
  // XXXben You can't call Init here unless this library's mPropertyCache has
  //        been created.
  rv = sbLocalDatabaseMediaListBase::Init(this, guid, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitResourceProperty(mPropertyCache, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize the media list factory table.
  PRBool success = mMediaListFactoryTable.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  rv = RegisterDefaultMediaListFactories();
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize the media item table.
  success = mMediaItemTable.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  // See if the user has specified a different analyze count limit. We don't
  // care if any of this fails.
  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    PRInt32 prefValue;
    rv = prefBranch->GetIntPref(ANALYZE_COUNT_PREF, &prefValue);
    if (NS_SUCCEEDED(rv)) {
      mAnalyzeCountLimit = PR_MAX(1, prefValue);
    }
  }

  // Register for library manager shutdown so we know when to write all
  // pending changes.
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = observerService->AddObserver(this,
                                    SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC,
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

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

  PRUint32 columnCount = NS_ARRAY_LENGTH(kInsertQueryColumns);
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

  // Create mGetAllListsByTypeId
  rv = builder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(),
                          NS_LITERAL_STRING("guid"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableName(NS_LITERAL_STRING("media_items"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->CreateMatchCriterionParameter(EmptyString(),
                                              NS_LITERAL_STRING("media_list_type_id"),
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->ToString(mGetAllListsByTypeId);
  NS_ENSURE_SUCCESS(rv, rv);

  // Build mInsertPropertyQuery
  insert = do_CreateInstance(SB_SQLBUILDER_INSERT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->SetIntoTableName(NS_LITERAL_STRING("resource_properties"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("guid"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("property_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("obj"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("obj_sortable"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->ToString(mInsertPropertyQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Build mGetGuidsFromContentUrl
  rv = builder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(),
                          NS_LITERAL_STRING("guid"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableName(NS_LITERAL_STRING("media_items"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->CreateMatchCriterionParameter(EmptyString(),
                                              NS_LITERAL_STRING("content_url"),
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->ToString(mGetGuidsFromContentUrl);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * \brief A simple routine to construct a sbIDatabaseQuery object with some
 *        generic initialization.
 */
/* inline */ nsresult
sbLocalDatabaseLibrary::MakeStandardQuery(sbIDatabaseQuery** _retval,
                                          PRBool aRunAsync)
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

  rv = query->SetAsyncQuery(aRunAsync);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = query);
  return NS_OK;
}

/**
 * \brief Make a string of the current time in milliseconds.
 */
/* static inline */ void
sbLocalDatabaseLibrary::GetNowString(nsAString& _retval) {
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
 * \param aURISpec - The URI spec for media items (not used for media lists)
 * \param _retval - The GUID that was generated, set only if the function
 *                  succeeds.
 */
nsresult
sbLocalDatabaseLibrary::AddNewItemQuery(sbIDatabaseQuery* aQuery,
                                        const PRUint32 aMediaItemTypeID,
                                        const nsAString& aURISpec,
                                        nsAString& _retval)
{
  NS_ENSURE_ARG_POINTER(aQuery);

  TRACE(("LocalDatabaseLibrary[0x%.8x] - AddNewItemQuery(%d, %s)", this,
         aMediaItemTypeID, NS_LossyConvertUTF16toASCII(aURISpec).get()));

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
    rv = aQuery->BindStringParameter(3, aURISpec);
    NS_ENSURE_SUCCESS(rv, rv);

    // Not hidden by default
    rv = aQuery->BindInt32Parameter(4, 0);
    NS_ENSURE_SUCCESS(rv, rv);

    // Media items don't have a media_list_type_id.
    rv = aQuery->BindNullParameter(5);
    NS_ENSURE_SUCCESS(rv, rv);
 }
  else {
    // This is a media list, create its url in the form
    // songbird-medialist://<library guid>/<item guid>
    nsAutoString newSpec;
    newSpec.AssignLiteral("songbird-medialist://");
    newSpec.Append(mGuid);
    newSpec.AppendLiteral("/");
    newSpec.Append(guid);

    rv = aQuery->BindStringParameter(3, newSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    // Not hidden by default
    rv = aQuery->BindInt32Parameter(4, 0);
    NS_ENSURE_SUCCESS(rv, rv);

    // Record the media list type.
    rv = aQuery->BindInt32Parameter(5, aMediaItemTypeID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  _retval.Assign(guid);
  return NS_OK;
}

nsresult
sbLocalDatabaseLibrary::AddItemPropertiesQueries(sbIDatabaseQuery* aQuery,
                                                 const nsAString& aGuid,
                                                 sbIPropertyArray* aProperties,
                                                 PRUint32* aAddedQueryCount)
{
  NS_ASSERTION(aQuery, "aQuery is null");
  NS_ASSERTION(aProperties, "aProperties is null");
  NS_ASSERTION(aAddedQueryCount, "aAddedQueryCount is null");

  nsresult rv;

  nsCOMPtr<sbISQLUpdateBuilder> update;

  nsCOMPtr<sbIPropertyManager> propMan =
    do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length;
  rv = aProperties->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 count = 0;

  for (PRUint32 i = 0; i < length; i++) {
    nsCOMPtr<sbIProperty> property;
    rv = aProperties->GetPropertyAt(i, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString name;
    rv = property->GetName(name);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString value;
    rv = property->GetValue(value);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyId;
    rv = mPropertyCache->GetPropertyID(name, &propertyId);
    NS_ENSURE_SUCCESS(rv, rv);

    if (SB_IsTopLevelPropertyID(propertyId)) {
      if (!update) {
        update = do_CreateInstance(SB_SQLBUILDER_UPDATE_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = update->SetTableName(NS_LITERAL_STRING("media_items"));
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<sbISQLBuilderCriterion> criterion;
        rv = update->CreateMatchCriterionString(EmptyString(),
                                                NS_LITERAL_STRING("guid"),
                                                sbISQLBuilder::MATCH_EQUALS,
                                                aGuid,
                                                getter_AddRefs(criterion));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = update->AddCriterion(criterion);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      nsAutoString columnName;
      rv = SB_GetTopLevelPropertyColumn(name, columnName);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = update->AddAssignmentString(columnName, value);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      nsCOMPtr<sbIPropertyInfo> propertyInfo;
      rv = propMan->GetPropertyInfo(name, getter_AddRefs(propertyInfo));
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString sortableValue;
      rv = propertyInfo->MakeSortable(value, sortableValue);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = aQuery->AddQuery(mInsertPropertyQuery);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = aQuery->BindStringParameter(0, aGuid);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = aQuery->BindInt32Parameter(1, propertyId);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = aQuery->BindStringParameter(2, value);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = aQuery->BindStringParameter(3, sortableValue);
      NS_ENSURE_SUCCESS(rv, rv);

      count++;
    }
  }

  if (update) {
    nsAutoString sql;
    rv = update->ToString(sql);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aQuery->AddQuery(sql);
    NS_ENSURE_SUCCESS(rv, rv);

    count++;
  }

  *aAddedQueryCount = count;
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
  if (mMediaItemTable.Get(aGUID, &itemInfo) && itemInfo->hasListType) {
    LOG((LOG_SUBMESSAGE_SPACE "Found type in cache!"));

    _retval.Assign(itemInfo->listType);
    return NS_OK;
  }

  // Make sure this GUID actually belongs in our library.
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

  if (rowCount == 0) {
    // This GUID isn't part of our library.
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsString type;
  rv = result->GetRowCell(0, 0, type);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!itemInfo) {
    // Make a new itemInfo for this GUID.
    nsAutoPtr<sbMediaItemInfo> newItemInfo(new sbMediaItemInfo());
    NS_ENSURE_TRUE(newItemInfo, NS_ERROR_OUT_OF_MEMORY);

    PRBool success = mMediaItemTable.Put(aGUID, newItemInfo);
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

    itemInfo = newItemInfo.forget();
  }

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
    static_cast<nsTArray<nsString>*>(aUserData);
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

  NS_NEWXPCOM(factory, sbLocalDatabaseSmartMediaListFactory);
  NS_ENSURE_TRUE(factory, NS_ERROR_OUT_OF_MEMORY);

  rv = RegisterMediaListFactory(factory);
  NS_ENSURE_SUCCESS(rv, rv);

  factory =
    do_GetService(SB_LOCALDATABASE_DYNAMICMEDIALISTFACTORY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RegisterMediaListFactory(factory);
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
sbLocalDatabaseLibrary::AddItemToLocalDatabase(sbIMediaItem* aMediaItem,
                                               sbIMediaItem** _retval)
{
  NS_ASSERTION(aMediaItem, "Null pointer!");
  NS_ASSERTION(_retval, "Null pointer!");

  // Create a new media item and copy all the properties
  nsCOMPtr<nsIURI> contentUri;
  nsresult rv = aMediaItem->GetContentSrc(getter_AddRefs(contentUri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyArray> properties;
  rv = aMediaItem->GetProperties(nsnull, getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> newItem;

  // CreateMediaItem usually notifies listeners that an item was added to the
  // media list. That's not what we want in this case because our callers
  // (Add and OnEnumeratedItem) will notify instead. Unfortunately we have to
  // use a member variable because CreateMediaItem is an IDL method and
  // additional (optional) parameters are not allowed.
  mPreventAddedNotification = PR_TRUE;
  rv = CreateMediaItem(contentUri,
                       properties,
                       PR_TRUE,
                       getter_AddRefs(newItem));
  mPreventAddedNotification = PR_FALSE;
  NS_ENSURE_SUCCESS(rv, rv);

  newItem.swap(*_retval);
  return NS_OK;
}

void
sbLocalDatabaseLibrary::IncrementDatabaseDirtyItemCounter(PRUint32 aIncrement)
{
  mDirtyItemCount += aIncrement;

  if (mDirtyItemCount >= mAnalyzeCountLimit) {
    mDirtyItemCount = 0;

#ifdef DEBUG
    nsresult rv =
#endif
    RunAnalyzeQuery();
#ifdef DEBUG
    if (NS_FAILED(rv)) {
      NS_WARNING("RunAnalyzeQuery failed!");
    }
#endif
  }
}

nsresult
sbLocalDatabaseLibrary::RunAnalyzeQuery(PRBool aRunAsync)
{
  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = MakeStandardQuery(getter_AddRefs(query), aRunAsync);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(NS_LITERAL_STRING("ANALYZE"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbresult;
  rv = query->Execute(&dbresult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbresult, NS_ERROR_FAILURE);

  return NS_OK;
}

/**
 * \brief Given a list of media items, this method returns a mapping of media
 *        items to the media lists that contain them
 */
nsresult
sbLocalDatabaseLibrary::GetContainingLists(sbMediaItemArray* aItems,
                                           sbMediaListArray* aLists,
                                           sbMediaItemToListsMap* aMap)
{
  NS_ASSERTION(aItems, "aItems is null");
  NS_ASSERTION(aLists, "aLists is null");
  NS_ASSERTION(aMap,   "aMap is null");
  NS_ASSERTION(aMap->IsInitialized(), "aMap not initalized");

  NS_NAMED_LITERAL_STRING(kGuid,                  "guid");
  NS_NAMED_LITERAL_STRING(kMediaItemId,           "media_item_id");
  NS_NAMED_LITERAL_STRING(kMemberMediaItemId,     "member_media_item_id");
  NS_NAMED_LITERAL_STRING(kListAlias,             "list");
  NS_NAMED_LITERAL_STRING(kItemAlias,             "item");
  NS_NAMED_LITERAL_STRING(kSimpleMediaLists,      "simple_media_lists");
  NS_NAMED_LITERAL_STRING(kSimpleMediaListsAlias, "sml");
  NS_NAMED_LITERAL_STRING(kMediaItems,            "media_items");

  nsresult rv;

  nsTHashtable<nsISupportsHashKey> distinctLists;
  PRBool success = distinctLists.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query), PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLSelectBuilder> builder =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(kListAlias, kGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(kItemAlias, kGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableName(kSimpleMediaLists);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableAlias(kSimpleMediaListsAlias);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                        kMediaItems,
                        kListAlias,
                        kMediaItemId,
                        kSimpleMediaListsAlias,
                        kMediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                        kMediaItems,
                        kItemAlias,
                        kMediaItemId,
                        kSimpleMediaListsAlias,
                        kMemberMediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterionIn> inCriterion;
  rv = builder->CreateMatchCriterionIn(kSimpleMediaListsAlias,
                                       kMemberMediaItemId,
                                       getter_AddRefs(inCriterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddCriterion(inCriterion);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 count = aItems->Count();
  PRUint32 incount = 0;
  for (PRUint32 i = 0; i < count; i++) {

    // The library can never be a member of a simple media list, so skip it.
    // Without this, GetMediaItemId will fail on the library
    PRBool isLibrary;
    rv = this->Equals(aItems->ObjectAt(i), &isLibrary);
    NS_ENSURE_SUCCESS(rv, rv);
    if (isLibrary) {
      continue;
    }

    nsCOMPtr<sbILocalDatabaseMediaItem> item =
      do_QueryInterface(aItems->ObjectAt(i), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 mediaItemId;
    rv = item->GetMediaItemId(&mediaItemId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = inCriterion->AddLong(mediaItemId);
    NS_ENSURE_SUCCESS(rv, rv);

    incount++;
    if (incount > MAX_IN_LENGTH || i + 1 == count) {
      nsAutoString sql;
      rv = builder->ToString(sql);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = query->AddQuery(sql);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = inCriterion->Clear();
      NS_ENSURE_SUCCESS(rv, rv);

      incount = 0;
    }
  }

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

  for (PRUint32 i = 0; i < rowCount; i++) {
    nsAutoString listGuid;
    rv = result->GetRowCell(i, 0, listGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaItem> list;
    rv = GetMediaItem(listGuid, getter_AddRefs(list));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString itemGuid;
    rv = result->GetRowCell(i, 1, itemGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaItem> containedItem;
    rv = GetMediaItem(itemGuid, getter_AddRefs(containedItem));
    NS_ENSURE_SUCCESS(rv, rv);

    sbMediaItemArray* lists;
    if (!aMap->Get(containedItem, &lists)) {
      nsAutoPtr<sbMediaItemArray> newLists(new sbMediaItemArray());
      NS_ENSURE_TRUE(newLists, NS_ERROR_OUT_OF_MEMORY);
      PRBool success = aMap->Put(containedItem, newLists);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
      lists = newLists.forget();
    }

    success = lists->AppendObject(list);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    nsISupportsHashKey* addedList = distinctLists.PutEntry(list);
    NS_ENSURE_TRUE(addedList, NS_ERROR_OUT_OF_MEMORY);
  }

  distinctLists.EnumerateEntries(EntriesToMediaListArray, aLists);

  return NS_OK;
}

nsresult
sbLocalDatabaseLibrary::GetAllListsByType(const nsAString& aType,
                                          sbMediaListArray* aArray)
{
  NS_ASSERTION(aArray, "aArray is null");

  nsresult rv;

  sbMediaListFactoryInfo* factoryInfo;
  PRBool success = mMediaListFactoryTable.Get(aType, &factoryInfo);
  NS_ENSURE_TRUE(success, NS_ERROR_INVALID_ARG);

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query), PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mGetAllListsByTypeId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt32Parameter(0, factoryInfo->typeID);
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

  for (PRUint32 i = 0; i < rowCount; i++) {
    nsAutoString guid;
    rv = result->GetRowCell(i, 0, guid);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaItem> item;
    rv = GetMediaItem(guid, getter_AddRefs(item));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaList> list = do_QueryInterface(item, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool success = aArray->AppendObject(list);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseLibrary::FilterExistingURIs(nsIArray* aURIs,
                                           nsIArray** aFilteredURIs)
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - FilterExistingURIs()", this));

  NS_ASSERTION(aURIs, "aURIs is null");
  NS_ASSERTION(aFilteredURIs, "aFilteredURIs is null");

  nsresult rv;

  PRUint32 length;
  rv = aURIs->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMutableArray> filtered =
    do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // If the incoming array is empty, do nothing
  if (length == 0) {
    NS_ADDREF(*aFilteredURIs = filtered);
    return NS_OK;
  }

  nsInterfaceHashtable<nsCStringHashKey, nsIURI> uniques;
  PRBool success = uniques.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsTArray<nsCString> originalOrder;

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query), PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLSelectBuilder> builder =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(), NS_LITERAL_STRING("content_url"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableName(NS_LITERAL_STRING("media_items"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterionIn> inCriterion;
  rv = builder->CreateMatchCriterionIn(EmptyString(),
                                       NS_LITERAL_STRING("content_url"),
                                       getter_AddRefs(inCriterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddCriterion(inCriterion);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 incount = 0;
  for (PRUint32 i = 0; i < length; i++) {

    nsCOMPtr<nsIURI> uri = do_QueryElementAt(aURIs, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCString spec;
    rv = uri->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCString* appended = originalOrder.AppendElement(spec);
    NS_ENSURE_TRUE(appended, NS_ERROR_OUT_OF_MEMORY);

    // We want to build a list of unique URIs, and also only add these unique
    // URIs to the query
    if (!uniques.Get(spec, nsnull)) {

      success = uniques.Put(spec, uri);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

      rv = inCriterion->AddString(NS_ConvertUTF8toUTF16(spec));
      NS_ENSURE_SUCCESS(rv, rv);

      incount++;
    }

    if (incount > MAX_IN_LENGTH || i + 1 == length) {
      nsAutoString sql;
      rv = builder->ToString(sql);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = query->AddQuery(sql);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = inCriterion->Clear();
      NS_ENSURE_SUCCESS(rv, rv);

      incount = 0;
    }

  }

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

  // Remove any found URIs from the unique list since they are duplicates
  for (PRUint32 i = 0; i < rowCount; i++) {
    nsString existingSpec;
    rv = result->GetRowCell(i, 0, existingSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    uniques.Remove(NS_ConvertUTF16toUTF8(existingSpec));
  }

  // Finally, add the remaning URIs to the output array.  Use the original
  // order as a reference so we don't scramble things up
  for (PRUint32 i = 0; i < length; i++) {
    nsCOMPtr<nsIURI> uri;
    if (uniques.Get(originalOrder[i], getter_AddRefs(uri))) {
      rv = filtered->AppendElement(uri, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);

      // Remove them as we find them so we don't include duplicates from the
      // original array
      uniques.Remove(originalOrder[i]);
    }
  }

  NS_ADDREF(*aFilteredURIs = filtered);
  return NS_OK;
}

nsresult
sbLocalDatabaseLibrary::GetGuidFromContentURI(nsIURI* aURI, nsAString& aGUID)
{
  NS_ASSERTION(aURI, "aURIs is null");

  nsresult rv;

  nsCString spec;
  rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mGetGuidsFromContentUrl);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, NS_ConvertUTF8toUTF16(spec));
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

  if (rowCount == 0) {
    // The URI was not found
    return NS_ERROR_NOT_AVAILABLE;
  }

  rv = result->GetRowCell(0, 0, aGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseLibrary::Shutdown()
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - Shutdown()", this));

  nsresult rv;

  // Pump events until all of our async queries have returned.
  PRUint32 timerCount = (PRUint32)mBatchCreateTimers.Count();
  if (timerCount) {
    LOG((LOG_SUBMESSAGE_SPACE "waiting on %u timers", timerCount));

    nsCOMPtr<nsIThread> currentThread(do_GetCurrentThread());
    NS_ABORT_IF_FALSE(currentThread, "Failed to get current thread!");

    if (currentThread) {
      while (mBatchCreateTimers.Count()) {
        LOG((LOG_SUBMESSAGE_SPACE "processing events for %u milliseconds",
             SHUTDOWN_ASYNC_GRANULARITY_MS));
#ifdef DEBUG
        rv =
#endif
        NS_ProcessPendingEvents(currentThread,
                                PR_MillisecondsToInterval(SHUTDOWN_ASYNC_GRANULARITY_MS));
        NS_ASSERTION(NS_SUCCEEDED(rv), "NS_ProcessPendingEvents failed!");
      }
    }
    LOG((LOG_SUBMESSAGE_SPACE "all timers have died"));
  }

  // Explicitly release our property cache here so we make sure to write all
  // changes to disk (regardless of whether or not this library will be leaked)
  // to prevent data loss.
  mPropertyCache = nsnull;

  if (mDirtyItemCount) {
#ifdef DEBUG
    rv =
#endif
    RunAnalyzeQuery(PR_FALSE);
#ifdef DEBUG
    if (NS_FAILED(rv)) {
      NS_WARNING("RunAnalyzeQuery failed!");
    }
#endif
  }

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
sbLocalDatabaseLibrary::CreateQuery(sbIDatabaseQuery** _retval)
{
  return MakeStandardQuery(_retval);
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::NotifyListenersItemUpdated(sbIMediaItem* aItem,
                                                   sbIPropertyArray* aProperties)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(aProperties);

#ifdef PR_LOGGING
  PRTime timer = PR_Now();
#endif

  // First we notify any simple media lists that contain this item
  sbMediaItemArray singleItem;
  PRBool success = singleItem.AppendObject(aItem);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  sbMediaItemToListsMap map;
  success = map.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  sbMediaListArray lists;
  nsresult rv = GetContainingLists(&singleItem, &lists, &map);
  NS_ENSURE_SUCCESS(rv, rv);

  map.EnumerateRead(sbLocalDatabaseLibrary::NotifyListsItemUpdated,
                    aProperties);



  // Also notify explicity registered listeners
  sbLocalDatabaseMediaListListener::NotifyListenersItemUpdated(SB_IMEDIALIST_CAST(this),
                                                               aItem,
                                                               aProperties);

  TRACE(("LocalDatabaseLibrary[0x%.8x] - NotifyListenersItemUpdated %d usec",
         this, PR_Now() - timer));

  return NS_OK;
}

/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseLibrary::NotifyListsItemUpdated(nsISupportsHashKey::KeyType aKey,
                                               sbMediaItemArray* aEntry,
                                               void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in the hash?!");
  NS_ASSERTION(aUserData, "Null userData!");

  nsCOMPtr<sbIPropertyArray> properties =
    static_cast<sbIPropertyArray*>(aUserData);
  NS_ENSURE_TRUE(properties, PL_DHASH_STOP);

  nsresult rv;
  nsCOMPtr<sbIMediaItem> item = do_QueryInterface(aKey, &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  PRUint32 count = aEntry->Count();
  for (PRUint32 i = 0; i < count; i++) {
    nsCOMPtr<sbILocalDatabaseSimpleMediaList> simple =
      do_QueryInterface(aEntry->ObjectAt(i), &rv);
    NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

    rv = simple->NotifyListenersItemUpdated(item, properties);
    NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);
  }

  return PL_DHASH_NEXT;
}

/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseLibrary::NotifyListsBeforeItemRemoved(nsISupportsHashKey::KeyType aKey,
                                                     sbMediaItemArray* aEntry,
                                                     void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in the hash?!");
  NS_ASSERTION(aUserData, "Null userData!");

  nsCOMPtr<sbIMediaList> list = static_cast<sbIMediaList*>(aUserData);
  NS_ENSURE_TRUE(list, PL_DHASH_STOP);

  nsresult rv;
  nsCOMPtr<sbIMediaItem> item = do_QueryInterface(aKey, &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  PRUint32 count = aEntry->Count();
  for (PRUint32 i = 0; i < count; i++) {
    nsCOMPtr<sbILocalDatabaseSimpleMediaList> simple =
      do_QueryInterface(aEntry->ObjectAt(i), &rv);
    NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

    rv = simple->NotifyListenersBeforeItemRemoved(list, item);
    NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);
  }

  return PL_DHASH_NEXT;
}

/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseLibrary::NotifyListsAfterItemRemoved(nsISupportsHashKey::KeyType aKey,
                                                    sbMediaItemArray* aEntry,
                                                    void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in the hash?!");
  NS_ASSERTION(aUserData, "Null userData!");

  nsCOMPtr<sbIMediaList> list = static_cast<sbIMediaList*>(aUserData);
  NS_ENSURE_TRUE(list, PL_DHASH_STOP);

  nsresult rv;
  nsCOMPtr<sbIMediaItem> item = do_QueryInterface(aKey, &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  PRUint32 count = aEntry->Count();
  for (PRUint32 i = 0; i < count; i++) {
    nsCOMPtr<sbILocalDatabaseSimpleMediaList> simple =
      do_QueryInterface(aEntry->ObjectAt(i), &rv);
    NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

    rv = simple->NotifyListenersAfterItemRemoved(list, item);
    NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);
  }

  return PL_DHASH_NEXT;
}

/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseLibrary::EntriesToMediaListArray(nsISupportsHashKey* aEntry,
                                                void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in the hash?!");
  NS_ASSERTION(aUserData, "Null entry in the hash?!");

  sbMediaListArray* array =
    static_cast<sbMediaListArray*>(aUserData);

  nsresult rv;
  nsCOMPtr<sbIMediaList> list = do_QueryInterface(aEntry->GetKey(), &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  PRBool success = array->AppendObject(list);
  NS_ENSURE_TRUE(success, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
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
                                        sbIPropertyArray* aProperties,
                                        PRBool aAllowDuplicates,
                                        sbIMediaItem** _retval)
{
  NS_ENSURE_ARG_POINTER(aUri);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCAutoString spec;
  nsresult rv = aUri->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  TRACE(("LocalDatabaseLibrary[0x%.8x] - CreateMediaItem(%s)", this,
         spec.get()));


  // If we don't allow duplicates, check to see if there is already a media
  // item with this uri.  If so, return it.
  if (!aAllowDuplicates) {
    nsresult rv;
    nsCOMPtr<nsIMutableArray> array =
      do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = array->AppendElement(aUri, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIArray> filtered;
    rv = FilterExistingURIs(array, getter_AddRefs(filtered));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 length;
    rv = filtered->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    // The uri was filtered out, therefore it exists.  Get it and return it
    if (length == 0) {
      nsString guid;
      rv = GetGuidFromContentURI(aUri, guid);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = GetMediaItem(guid, _retval);
      NS_ENSURE_SUCCESS(rv, rv);

      return NS_OK;
    }
  }

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString guid;
  rv = AddNewItemQuery(query,
                       SB_MEDIAITEM_TYPEID,
                       NS_ConvertUTF8toUTF16(spec),
                       guid);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aProperties) {
    nsCOMPtr<sbIPropertyArray> filteredProperties;
    rv = GetFilteredPropertiesForNewItem(aProperties,
                                         getter_AddRefs(filteredProperties));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 junk;
    rv = AddItemPropertiesQueries(query, guid, filteredProperties, &junk);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  IncrementDatabaseDirtyItemCounter();

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

  // Invalidate our array
  rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  // Don't do this while we're receiving items through the Insertinglistener.
  if (!mPreventAddedNotification) {
    NotifyListenersItemAdded(SB_IMEDIALIST_CAST(this), mediaItem);
  }

  NS_ADDREF(*_retval = mediaItem);
  return NS_OK;
}

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::CreateMediaList(const nsAString& aType,
                                        sbIPropertyArray* aProperties,
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

  if (aProperties) {
    nsCOMPtr<sbIPropertyArray> filteredProperties;
    rv = GetFilteredPropertiesForNewItem(aProperties,
                                         getter_AddRefs(filteredProperties));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 junk;
    rv = AddItemPropertiesQueries(query, guid, filteredProperties, &junk);
    NS_ENSURE_SUCCESS(rv, rv);
  }

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

  // Invalidate our array
  rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  NotifyListenersItemAdded(SB_IMEDIALIST_CAST(this), mediaItem);

  nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(mediaItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = mediaList);
  return NS_OK;
}

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::CopyMediaList(const nsAString& aType,
                                      sbIMediaList* aSource,
                                      sbIMediaList** _retval)
{
  NS_ENSURE_FALSE(aType.IsEmpty(), NS_ERROR_INVALID_ARG);
  NS_ENSURE_ARG_POINTER(aSource);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIPropertyArray> properties;
  nsresult rv = aSource->GetProperties(nsnull, getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> newList;
  rv = CreateMediaList(aType, properties, getter_AddRefs(newList));
  NS_ENSURE_SUCCESS(rv, rv);

  // XXXben This will probably fail for types other than "simple"... For now
  //        we won't automatically lock other types out (by returning early)
  //        just in case another media list type implements this behavior.
  rv = newList->AddAll(aSource);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = newList);
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

  // If the requested guid is the library, return ourselves
  if (mGuid.Equals(aGUID)) {
    nsCOMPtr<sbIMediaItem> item = SB_IMEDIAITEM_CAST(this);
    NS_ADDREF(*_retval = item);
    return NS_OK;
  }

  sbMediaItemInfo* itemInfo;
  if (!mMediaItemTable.Get(aGUID, &itemInfo) ||
      !itemInfo->hasListType) {
    // Try to get the type from the database. This will also tell us if this
    // GUID actually belongs to this library.
    nsString type;
    rv = GetTypeForGUID(aGUID, type);
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      // Hmm, this GUID doesn't belong to this library. Someone passed in a bad
      // GUID.
      NS_WARNING("GetMediaItem called with a bad GUID!");
      return NS_ERROR_NOT_AVAILABLE;
    }
    NS_ENSURE_SUCCESS(rv, rv);

    if (!itemInfo) {
      // Now there should definitely be an entry for this GUID in our table.
      mMediaItemTable.Get(aGUID, &itemInfo);
      NS_ASSERTION(itemInfo, "No entry in the table for this GUID!");
    }
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

  NS_ASSERTION(itemInfo->hasListType, "Should have set a list type there!");

  nsRefPtr<sbLocalDatabaseMediaItem>
    newMediaItem(new sbLocalDatabaseMediaItem());
  NS_ENSURE_TRUE(newMediaItem, NS_ERROR_OUT_OF_MEMORY);

  rv = newMediaItem->Init(this, aGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  strongMediaItem = newMediaItem;

  if (!itemInfo->listType.IsEmpty()) {
    // This must be a media list, so get the appropriate factory.
    sbMediaListFactoryInfo* factoryInfo;
    PRBool success = mMediaListFactoryTable.Get(itemInfo->listType,
                                                &factoryInfo);
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
  nsresult rv = MakeStandardQuery(getter_AddRefs(query), PR_TRUE);
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
sbLocalDatabaseLibrary::Sync()
{
  TRACE(("LocalDatabaseLibrary[0x%.8x] - Sync()", this));

  nsresult rv = mPropertyCache->Write();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::BatchCreateMediaItems(nsIArray* aURIArray,
                                              nsIArray* aPropertyArrayArray,
                                              PRBool aAllowDuplicates,
                                              nsIArray** _retval)
{
  NS_ENSURE_ARG_POINTER(aURIArray);
  NS_ENSURE_ARG_POINTER(_retval);

  TRACE(("LocalDatabaseLibrary[0x%.8x] - BatchCreateMediaItems()", this));

  return BatchCreateMediaItemsInternal(aURIArray, aPropertyArrayArray,
                                       aAllowDuplicates, nsnull, _retval);
}

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::BatchCreateMediaItemsAsync(sbIBatchCreateMediaItemsListener* aListener,
                                                   nsIArray* aURIArray,
                                                   nsIArray* aPropertyArrayArray,
                                                   PRBool aAllowDuplicates)
{
  NS_ENSURE_ARG_POINTER(aListener);
  NS_ENSURE_ARG_POINTER(aURIArray);

  TRACE(("LocalDatabaseLibrary[0x%.8x] - BatchCreateMediaItemsAsync()", this));

  return BatchCreateMediaItemsInternal(aURIArray, aPropertyArrayArray,
                                       aAllowDuplicates, aListener, nsnull);
}

nsresult
sbLocalDatabaseLibrary::BatchCreateMediaItemsInternal(nsIArray* aURIArray,
                                                      nsIArray* aPropertyArrayArray,
                                                      PRBool aAllowDuplicates,
                                                      sbIBatchCreateMediaItemsListener* aListener,
                                                      nsIArray** _retval)
{
  NS_ASSERTION((aListener && !_retval) || (!aListener && _retval),
               "Only one of |aListener| and |_retval| should be set!");

  TRACE(("LocalDatabaseLibrary[0x%.8x] - BatchCreateMediaItemsInternal()",
         this));

  nsresult rv;

  nsCOMPtr<nsIArray> filteredArray;
  if (aAllowDuplicates) {
    filteredArray = aURIArray;
  }
  else {
    rv = FilterExistingURIs(aURIArray, getter_AddRefs(filteredArray));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRBool runAsync = aListener ? PR_TRUE : PR_FALSE;

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query), runAsync);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbBatchCreateTimerCallback> callback;
  nsRefPtr<sbBatchCreateHelper> helper;

  if (runAsync) {
    callback = new sbBatchCreateTimerCallback(this, aListener, query);
    NS_ENSURE_TRUE(callback, NS_ERROR_OUT_OF_MEMORY);

    rv = callback->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    helper = callback->BatchHelper();
  }
  else {
    helper = new sbBatchCreateHelper(this);
    NS_ENSURE_TRUE(helper, NS_ERROR_OUT_OF_MEMORY);
  }

  // Set up the batch add query
  rv = helper->InitQuery(query, filteredArray, aPropertyArrayArray);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbResult;
  rv = query->Execute(&dbResult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbResult == 0, NS_ERROR_FAILURE);

  if (runAsync) {
    // Start polling the query for completion
    nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Stick the timer into a member array so we keep it alive while it does
    // its thing.  It never gets removed from this array until this library
    // gets deleted.
    PRBool success = mBatchCreateTimers.AppendObject(timer);
    NS_ENSURE_TRUE(success, rv);

    // This value 333 is magical.  I copied it from the old media scan code.  I
    // assumed it was a number that was reached through experience tweaking the
    // callback frequency.
    rv = timer->InitWithCallback(callback, 333, nsITimer::TYPE_REPEATING_SLACK);
    if (NS_FAILED(rv)) {
      NS_WARNING("InitWithCallback failed!");
      success = mBatchCreateTimers.RemoveObject(timer);
      NS_ASSERTION(success, "Failed to remove a failed timer... Armageddon?");
      return rv;
    }
  }
  else {
    // Notify and get the new items
    nsCOMPtr<nsIArray> array;
    rv = helper->NotifyAndGetItems(getter_AddRefs(array));
    NS_ENSURE_SUCCESS(rv, rv);

    // Return the array of media items
    NS_ADDREF(*_retval = array);
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseLibrary::RemoveSelected(nsISimpleEnumerator* aSelection,
                                       sbLocalDatabaseMediaListView* aView)
{
  NS_ENSURE_ARG_POINTER(aSelection);
  NS_ENSURE_ARG_POINTER(aView);

  nsresult rv;

  nsRefPtr<sbLocalDatabaseMediaListBase> viewMediaList =
    aView->GetNativeMediaList();

  sbILocalDatabaseGUIDArray* viewArray = aView->GetGUIDArray();

  // Figure out if we have a library or a simple media list
  nsCOMPtr<sbIMediaList> list =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIMediaList*, viewMediaList), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isLibrary;
  rv = this->Equals(list, &isLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  // Start building a query that we will use to delete the items from the
  // database.  This query will be slightly different depending on if we are
  // deleting from the library or a list

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(NS_LITERAL_STRING("begin"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLDeleteBuilder> deleteb =
    do_CreateInstance(SB_SQLBUILDER_DELETE_CONTRACTID, &rv);
  nsCOMPtr<sbISQLBuilderCriterion> criterion;
  nsCOMPtr<sbISQLBuilderCriterionIn> inCriterion;

  // If we are a library, we can delete things by media item id
  if (isLibrary) {
    rv = deleteb->SetTableName(NS_LITERAL_STRING("media_items"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = deleteb->CreateMatchCriterionIn(EmptyString(),
                                         NS_LITERAL_STRING("media_item_id"),
                                         getter_AddRefs(inCriterion));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = deleteb->AddCriterion(inCriterion);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    // If we are a list, we will be deleting by the list's media item id and
    // the ordinal
    rv = deleteb->SetTableName(NS_LITERAL_STRING("simple_media_lists"));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 mediaItemId;
    rv = viewMediaList->GetMediaItemId(&mediaItemId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = deleteb->CreateMatchCriterionLong(EmptyString(),
                                           NS_LITERAL_STRING("media_item_id"),
                                           sbISQLSelectBuilder::MATCH_EQUALS,
                                           mediaItemId,
                                           getter_AddRefs(criterion));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = deleteb->AddCriterion(criterion);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = deleteb->CreateMatchCriterionIn(EmptyString(),
                                         NS_LITERAL_STRING("ordinal"),
                                         getter_AddRefs(inCriterion));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = deleteb->AddCriterion(inCriterion);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Loop through the selected media items and build the in criterion for the
  // query as well as keep a list of the items were deleting so we can send
  // notifications
  sbMediaItemArray selectedItems;
  nsCOMPtr<sbIIndexedMediaItem> indexedMediaItem;
  PRUint32 incount = 0;
  while (NS_SUCCEEDED(aSelection->GetNext(getter_AddRefs(indexedMediaItem)))) {

    nsCOMPtr<sbIMediaItem> item;
    PRUint32 index;

    rv = indexedMediaItem->GetMediaItem(getter_AddRefs(item));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = indexedMediaItem->GetIndex(&index);
    NS_ENSURE_SUCCESS(rv, rv);

    // Add a different id depending on where we are deleting from
    if (isLibrary) {
      PRUint32 mediaItemId;
      rv = viewArray->GetMediaItemIdByIndex(index, &mediaItemId);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = inCriterion->AddLong(mediaItemId);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      nsAutoString ordinal;
      rv = viewArray->GetOrdinalByIndex(index, ordinal);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = inCriterion->AddString(ordinal);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    incount++;

    // If we go over the number of IN items we allow, add it to the query
    if (incount > MAX_IN_LENGTH) {
      nsAutoString sql;
      rv = deleteb->ToString(sql);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = query->AddQuery(sql);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = inCriterion->Clear();
      NS_ENSURE_SUCCESS(rv, rv);

      incount = 0;
    }

    // Finally, remember this media item so we can send notifications
    PRBool success = selectedItems.AppendObject(item);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  // Add any left overs to the query
  if (incount > 0) {
    nsAutoString sql;
    rv = deleteb->ToString(sql);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->AddQuery(sql);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = query->AddQuery(NS_LITERAL_STRING("commit"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 count = selectedItems.Count();

  if (isLibrary) {

    // If we are removing from the library, we need to notify all the simple
    // media lists that contains the items as well as the library's listeners
    sbMediaItemToListsMap map;
    PRBool success = map.Init();
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    sbMediaListArray lists;

    rv = GetContainingLists(&selectedItems, &lists, &map);
    NS_ENSURE_SUCCESS(rv, rv);

    // Start a batch in both the library and all the lists that we are
    // removing from
    sbAutoBatchHelper batchHelper(this);
    sbAutoSimpleMediaListBatchHelper listsBatchHelper(&lists);

    map.EnumerateRead(NotifyListsBeforeItemRemoved, SB_IMEDIALIST_CAST(this));

    for (PRUint32 i = 0; i < count; i++) {
      sbLocalDatabaseMediaListListener::NotifyListenersBeforeItemRemoved(SB_IMEDIALIST_CAST(this),
                                                                         selectedItems[i]);
    }

    // Now actually delete the items
    PRInt32 dbSuccess;
    rv = query->Execute(&dbSuccess);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_SUCCESS(dbSuccess, NS_ERROR_FAILURE);

    rv = mFullArray->Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);

    // Invalidate all of the simple media lists we notified
    for (PRInt32 i = 0; i < lists.Count(); i++) {
      nsCOMPtr<sbILocalDatabaseSimpleMediaList> simple =
        do_QueryInterface(lists[i], &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = simple->Invalidate();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Notify simple media lists after removal
    map.EnumerateRead(NotifyListsAfterItemRemoved, SB_IMEDIALIST_CAST(this));

    // Notify our listeners of after removal
    for (PRUint32 i = 0; i < count; i++) {
      sbLocalDatabaseMediaListListener::NotifyListenersAfterItemRemoved(SB_IMEDIALIST_CAST(this),
                                                                        selectedItems[i]);
    }
  }
  else { // isLibrary
    sbAutoBatchHelper batchHelper(viewMediaList);

    // If this is a media list, just notify the list
    nsCOMPtr<sbILocalDatabaseSimpleMediaList> simple =
      do_QueryInterface(NS_ISUPPORTS_CAST(sbIMediaList*, viewMediaList), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 i = 0; i < count; i++) {
      rv = simple->NotifyListenersBeforeItemRemoved(viewMediaList,
                                                    selectedItems[i]);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Now actually delete the items
    PRInt32 dbSuccess;
    rv = query->Execute(&dbSuccess);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_SUCCESS(dbSuccess, NS_ERROR_FAILURE);

    rv = simple->Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 i = 0; i < count; i++) {
      rv = simple->NotifyListenersAfterItemRemoved(viewMediaList,
                                                   selectedItems[i]);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  for (PRUint32 i = 0; i < count; i++) {
    nsString guid;

    sbIMediaItem* mediaItem = selectedItems.ObjectAt(i);
    NS_ASSERTION(mediaItem, "Null in selectedItems!");

    rv = mediaItem->GetGuid(guid);
    NS_ENSURE_SUCCESS(rv, rv);

    // Remove from our cache.
    mMediaItemTable.Remove(guid);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseLibrary::GetNativeLibrary(sbLocalDatabaseLibrary** aLocalDatabaseLibrary)
{
  NS_ENSURE_ARG_POINTER(aLocalDatabaseLibrary);
  *aLocalDatabaseLibrary = this;
  return NS_OK;
}

/**
 * See sbIMediaItem
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::SetContentSrc(nsIURI* aContentSrc)
{
  // The contentSrc attribute cannot be modified on a library.
  return NS_ERROR_FAILURE;
}

/**
 * See sbIMediaList
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::GetType(nsAString& aType)
{
  aType.Assign(NS_LITERAL_STRING("library"));
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

  nsCOMPtr<sbIMediaItem> item;
  rv = GetMediaItem(aGuid, getter_AddRefs(item));
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

  nsCOMPtr<sbIMediaItem> newMediaItem;

  // Import this item into this library
  rv = AddItemToLocalDatabase(aMediaItem, getter_AddRefs(newMediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  // Invalidate the cached list
  rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  // And let everyone know about it.
  NotifyListenersItemAdded(SB_IMEDIALIST_CAST(this), newMediaItem);

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
  nsresult rv =
    sbLocalDatabaseMediaListBase::EnumerateAllItems(&listener,
                                                    sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
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
  nsresult rv = mFullArray->GetGuidByIndex(aIndex, guid);
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

  sbMediaListArray lists;
  nsresult rv = GetAllListsByType(NS_LITERAL_STRING("simple"), &lists);
  NS_ENSURE_SUCCESS(rv, rv);

  // Notify simple media lists that they are getting cleared
  for (PRInt32 i = 0; i < lists.Count(); i++) {
    nsCOMPtr<sbILocalDatabaseSimpleMediaList> simple =
      do_QueryInterface(lists[i], &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = simple->NotifyListenersListCleared(lists[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Clear our cache
  mMediaItemTable.Clear();

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
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

  // Invalidate all simple media lists
  for (PRInt32 i = 0; i < lists.Count(); i++) {
    nsCOMPtr<sbILocalDatabaseSimpleMediaList> simple =
      do_QueryInterface(lists[i], &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = simple->Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NotifyListenersListCleared(SB_IMEDIALIST_CAST(this));

  return NS_OK;
}

/**
 * See sbIMediaList
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::CreateView(sbIMediaListViewState* aState,
                                   sbIMediaListView** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsAutoString prop;
  prop.AssignLiteral(DEFAULT_SORT_PROPERTY);

  nsRefPtr<sbLocalDatabaseMediaListView>
    view(new sbLocalDatabaseMediaListView(this, this, prop, 0));
  NS_ENSURE_TRUE(view, NS_ERROR_OUT_OF_MEMORY);

  rv = view->Init(aState);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = view);
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

  // Keep this around to know when the vacuum call finishes... For UI
  // notification? Remove this whole mess if we never need it.

  return NS_OK;
}

/**
 * See nsIObserver
 */
NS_IMETHODIMP
sbLocalDatabaseLibrary::Observe(nsISupports *aSubject,
                                const char *aTopic,
                                const PRUnichar *aData)
{
  if (!strcmp(aTopic, SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC)) {
    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
      observerService->RemoveObserver(this, SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC);
    }

    rv = Shutdown();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    NS_NOTREACHED("Observing a topic we don't care about!");
  }

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

NS_IMPL_ISUPPORTS1(sbBatchCreateTimerCallback, nsITimerCallback);

sbBatchCreateTimerCallback::sbBatchCreateTimerCallback(sbLocalDatabaseLibrary* aLibrary,
                                                       sbIBatchCreateMediaItemsListener* aListener,
                                                       sbIDatabaseQuery* aQuery) :
  mLibrary(aLibrary),
  mListener(aListener),
  mQuery(aQuery),
  mTimer(nsnull),
  mQueryCount(0)
{
  NS_ASSERTION(aLibrary, "Null library!");
  NS_ASSERTION(aListener, "Null listener!");
  NS_ASSERTION(aQuery, "Null query!");
}

nsresult
sbBatchCreateTimerCallback::Init()
{
  PRBool success = mQueryToIndexMap.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  mBatchHelper = new sbBatchCreateHelper(mLibrary, this);
  NS_ENSURE_TRUE(mBatchHelper, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult
sbBatchCreateTimerCallback::AddMapping(PRUint32 aQueryIndex,
                                       PRUint32 aItemIndex)
{
  PRBool success = mQueryToIndexMap.Put(aQueryIndex, aItemIndex);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

sbBatchCreateHelper*
sbBatchCreateTimerCallback::BatchHelper()
{
  NS_ASSERTION(mBatchHelper, "This shouldn't be null, did you call Init?!");
  return mBatchHelper;
}


NS_IMETHODIMP
sbBatchCreateTimerCallback::Notify(nsITimer* aTimer)
{
  NS_ENSURE_ARG_POINTER(aTimer);

  PRBool complete;
  nsresult rv = NotifyInternal(aTimer, &complete);
  if (NS_SUCCEEDED(rv) && !complete) {
    // Everything looks fine, let the timer continue.
    return NS_OK;
  }

  // The library won't shut down until all the async timers are cleared, so
  // cancel this timer if we're done or if there was some kind of error.

  aTimer->Cancel();
  mLibrary->mBatchCreateTimers.RemoveObject(aTimer);

  // Report the earlier error, if any.
  NS_ENSURE_SUCCESS(rv, rv);

  // Gather the media items we added and call the listener.
  nsCOMPtr<nsIArray> array;
  rv = mBatchHelper->NotifyAndGetItems(getter_AddRefs(array));
  NS_ENSURE_SUCCESS(rv, rv);

  mListener->OnComplete(array);

  return NS_OK;
}

nsresult
sbBatchCreateTimerCallback::NotifyInternal(nsITimer* aTimer,
                                           PRBool* _retval)
{
  NS_ASSERTION(_retval, "Null retval!");

  nsresult rv;

  // Use mTimer as a "runonce" flag so that we can cache the query count.
  if (!mTimer) {
    rv = mQuery->GetQueryCount(&mQueryCount);
    NS_ENSURE_SUCCESS(rv, rv);

    mTimer = aTimer;
  }
#ifdef DEBUG
  else {
    // Make sure this is the timer we think it should be.
    NS_ASSERTION(mTimer == aTimer, "Not the timer we saw last time!");

    PRUint32 queryCount;
    rv = mQuery->GetQueryCount(&queryCount);
    if (NS_SUCCEEDED(rv)) {
      // Don't allow the query count to change! If we hit this assertion then
      // mQuery is being modified by somebody after they gave it to us!
      NS_ASSERTION(queryCount == mQueryCount, "mQuery should not be changed!");
    }
    else {
      NS_ERROR("Failed to get QueryCount!");
    }
  }
#endif

  // Exit early if there's nothing to do.
  if (!mQueryCount) {
    *_retval = PR_TRUE;
    return NS_OK;
  }

  // Check to see if the query is complete.
  PRUint32 currentQuery;
  rv = mQuery->CurrentQuery(&currentQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  currentQuery++;
  NS_ASSERTION(currentQuery <= mQueryCount, "Invalid position!");

  if (mQueryCount != currentQuery) {
    // Notify listener of progress.
    PRUint32 itemIndex;
    PRBool success = mQueryToIndexMap.Get(currentQuery, &itemIndex);
    NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);

    mListener->OnProgress(itemIndex);

    *_retval = PR_FALSE;
    return NS_OK;
  }

  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMPL_ADDREF(sbBatchCreateHelper)
NS_IMPL_RELEASE(sbBatchCreateHelper)

sbBatchCreateHelper::sbBatchCreateHelper(sbLocalDatabaseLibrary* aLibrary,
                                         sbBatchCreateTimerCallback* aCallback) :
  mLibrary(aLibrary),
  mCallback(aCallback)
{
  NS_ASSERTION(aLibrary, "Null library!");
}

nsresult
sbBatchCreateHelper::InitQuery(sbIDatabaseQuery* aQuery,
                               nsIArray* aURIArray,
                               nsIArray* aPropertyArrayArray)
{
  NS_ASSERTION(aQuery, "aQuery is null");

  nsresult rv = aQuery->AddQuery(NS_LITERAL_STRING("begin"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 listLength;
  rv = aURIArray->GetLength(&listLength);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 queryCount = 0;

  // Add a query to insert each new item and record the guids that were
  // generated for the future inserts
  for (PRUint32 i = 0; i < listLength; i++) {

    nsCOMPtr<nsIURI> uri = do_QueryElementAt(aURIArray, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCAutoString spec;
    rv = uri->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString guid;
    rv = mLibrary->AddNewItemQuery(aQuery,
                                   SB_MEDIAITEM_TYPEID,
                                   NS_ConvertUTF8toUTF16(spec),
                                   guid);
    NS_ENSURE_SUCCESS(rv, rv);

    if (mCallback) {
      rv = mCallback->AddMapping(queryCount, i);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    queryCount++;

    if (aPropertyArrayArray) {
      nsCOMPtr<sbIPropertyArray> properties =
        do_QueryElementAt(aPropertyArrayArray, i, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbIPropertyArray> filteredProperties;
      rv = mLibrary->GetFilteredPropertiesForNewItem(properties,
                                                     getter_AddRefs(filteredProperties));
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 addedCount;
      rv = mLibrary->AddItemPropertiesQueries(aQuery,
                                              guid,
                                              filteredProperties,
                                              &addedCount);
      NS_ENSURE_SUCCESS(rv, rv);

      for (PRUint32 j = 0; j < addedCount; j++) {
        if (mCallback) {
          rv = mCallback->AddMapping(queryCount, i);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        queryCount++;
      }

    }

    nsString* success = mGuids.AppendElement(guid);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  rv = aQuery->AddQuery(NS_LITERAL_STRING("commit"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBatchCreateHelper::NotifyAndGetItems(nsIArray** _retval)
{
  NS_ASSERTION(_retval, "_retval is null");

  nsresult rv;
  nsCOMPtr<nsIMutableArray> array =
    do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length = mGuids.Length();

  mLibrary->IncrementDatabaseDirtyItemCounter(length);


  {
    sbAutoBatchHelper batchHelper(mLibrary);

    // Bulk get all the property bags for the newly added items
    nsTArray<const PRUnichar*> guidArray(length);
    for (PRUint32 i = 0; i < length; i++) {
      const PRUnichar** addedPtr = guidArray.AppendElement(mGuids[i].get());
      NS_ENSURE_TRUE(addedPtr, NS_ERROR_OUT_OF_MEMORY);
    }

    PRUint32 count = 0;
    sbILocalDatabaseResourcePropertyBag** bags = nsnull;
    rv = mLibrary->mPropertyCache->GetProperties(guidArray.Elements(),
                                                 length,
                                                 &count,
                                                 &bags);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 i = 0; i < length; i++) {
      // We know the GUID and the type of these new media items so preload
      // the cache with this information
      nsAutoPtr<sbLocalDatabaseLibrary::sbMediaItemInfo>
        newItemInfo(new sbLocalDatabaseLibrary::sbMediaItemInfo());
      NS_ENSURE_TRUE(newItemInfo, NS_ERROR_OUT_OF_MEMORY);

      newItemInfo->hasListType = PR_TRUE;

      NS_ASSERTION(!mLibrary->mMediaItemTable.Get(mGuids[i], nsnull),
                   "Guid already exists!");

      PRBool success = mLibrary->mMediaItemTable.Put(mGuids[i], newItemInfo);
      NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

      newItemInfo.forget();

      nsCOMPtr<sbIMediaItem> mediaItem;
      rv = mLibrary->GetMediaItem(mGuids[i], getter_AddRefs(mediaItem));
      NS_ENSURE_SUCCESS(rv, rv);

      // Set the new media item with the property bag we got earlier
      nsCOMPtr<sbILocalDatabaseMediaItem> ldbmi =
        do_QueryInterface(mediaItem, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = ldbmi->SetPropertyBag(bags[i]);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = array->AppendElement(mediaItem, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mLibrary->mFullArray->Invalidate();
      NS_ENSURE_SUCCESS(rv, rv);

      mLibrary->NotifyListenersItemAdded(SB_IMEDIALIST_CAST(mLibrary),
                                         mediaItem);
    }

    NS_FREE_XPCOM_ISUPPORTS_POINTER_ARRAY(count, bags);

  }

  NS_ADDREF(*_retval = array);
  return NS_OK;
}
