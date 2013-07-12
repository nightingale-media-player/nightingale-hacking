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

#include "sbPlaybackHistoryService.h"
#ifdef METRICS_ENABLED
#include "sbPlaybackMetricsKeys.h"
#endif

#include <nsIArray.h>
#include <nsICategoryManager.h>
#include <nsIConverterInputStream.h>
#include <nsIInputStream.h>
#include <nsILocalFile.h>
#include <nsIMutableArray.h>
#include <nsIObserverService.h>
#include <nsIURL.h>
#include <nsIVariant.h>
#include <nsIWeakReferenceUtils.h>

#include <nsArrayUtils.h>
#include <nsAutoLock.h>
#include <nsCOMArray.h>
#include <nsComponentManagerUtils.h>
#include <nsNetUtil.h>
#include <nsThreadUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOMCID.h>

#include <prprf.h>

#include <sbILibrary.h>
#include <sbILibraryManager.h>
#include <sbIMediacoreEvent.h>
#include <sbIMediacoreEventTarget.h>
#include <sbIMediacoreManager.h>
#include <sbIMediaList.h>
#include <sbIPlaybackHistoryEntry.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyInfo.h>
#include <sbIPropertyManager.h>
#include <sbISQLBuilder.h>

#include <DatabaseQuery.h>
#include <sbArray.h>
#include <sbLibraryCID.h>
#include <sbPropertiesCID.h>
#include <sbStandardProperties.h>
#include <sbProxiedComponentManager.h>
#include <sbSQLBuilderCID.h>
#include <sbStringUtils.h>
#include <sbDebugUtils.h>

#define NS_APPSTARTUP_CATEGORY           "app-startup"
#define NS_APPSTARTUP_TOPIC              "app-startup"

#define CONVERTER_BUFFER_SIZE 8192

#define PLAYBACKHISTORY_DB_GUID "playbackhistory@songbirdnest.com"
#define SCHEMA_URL \
  "chrome://songbird/content/mediacore/playback/history/playbackhistoryservice.sql"

#define PLAYBACKHISTORY_ENTRIES_TABLE     "playback_history_entries"
#define PLAYBACKHISTORY_ANNOTATIONS_TABLE "playback_history_entry_annotations"
#define PLAYBACKHISTORY_PROPERTIES_TABLE  "properties"
#define PLAYBACKHISTORY_COUNT_ENTRIES     "count(entry_id)"

#define ENTRY_ID_COLUMN         "entry_id"
#define LIBRARY_GUID_COLUMN     "library_guid"
#define MEDIA_ITEM_GUID_COLUMN  "media_item_guid"
#define PLAY_TIME_COLUMN        "play_time"
#define PLAY_DURATION_COLUMN    "play_duration"

#define ENTRY_ID_COLUMN       "entry_id"
#define PROPERTY_ID_COLUMN    "property_id"
#define PROPERTY_NAME_COLUMN  "property_name"
#define OBJ_COLUMN            "obj"
#define OBJ_SORTABLE          "obj_sortable"

//------------------------------------------------------------------------------
// Support Functions
//------------------------------------------------------------------------------
static already_AddRefed<nsILocalFile>
GetDBFolder()
{
  nsresult rv;
  nsCOMPtr<nsIProperties> ds =
    do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsILocalFile* file;
  rv = ds->Get("ProfD", NS_GET_IID(nsILocalFile), (void**)&file);
  NS_ENSURE_SUCCESS(rv, nsnull);

  rv = file->AppendRelativePath(NS_LITERAL_STRING("db"));
  if (NS_FAILED(rv)) {
    NS_WARNING("AppendRelativePath failed!");

    NS_RELEASE(file);
    return nsnull;
  }

  return file;
}

//-----------------------------------------------------------------------------
// sbPlaybackHistoryService
//-----------------------------------------------------------------------------
NS_IMPL_THREADSAFE_ISUPPORTS3(sbPlaybackHistoryService, 
                              sbIPlaybackHistoryService,
                              sbIMediacoreEventListener,
                              nsIObserver)

sbPlaybackHistoryService::sbPlaybackHistoryService()
: mCurrentlyTracking(PR_FALSE)
, mCurrentStartTime(0)
, mCurrentPauseTime(0)
, mCurrentDelta(0)
{
  MOZ_COUNT_CTOR(sbPlaybackHistoryService);
}

sbPlaybackHistoryService::~sbPlaybackHistoryService()
{
  MOZ_COUNT_DTOR(sbPlaybackHistoryService);
}

/*static*/ NS_METHOD 
sbPlaybackHistoryService::RegisterSelf(nsIComponentManager* aCompMgr,
                                       nsIFile* aPath,
                                       const char* aLoaderStr,
                                       const char* aType,
                                       const nsModuleComponentInfo *aInfo)
{
  NS_ENSURE_ARG_POINTER(aCompMgr);
  NS_ENSURE_ARG_POINTER(aPath);
  NS_ENSURE_ARG_POINTER(aLoaderStr);
  NS_ENSURE_ARG_POINTER(aType);
  NS_ENSURE_ARG_POINTER(aInfo);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = categoryManager->AddCategoryEntry(NS_APPSTARTUP_CATEGORY,
                                         SB_PLAYBACKHISTORYSERVICE_DESCRIPTION,
                                         "service,"
                                         SB_PLAYBACKHISTORYSERVICE_CONTRACTID,
                                         PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* static */ PLDHashOperator PR_CALLBACK
sbPlaybackHistoryService::AddListenersToCOMArrayCallback(nsISupportsHashKey::KeyType aKey,
                                                         sbIPlaybackHistoryListener* aEntry,
                                                         void* aUserData)
{
  NS_ASSERTION(aKey, "Null key in hashtable!");
  NS_ASSERTION(aEntry, "Null entry in hashtable!");

  nsCOMArray<sbIPlaybackHistoryListener>* array =
    static_cast<nsCOMArray<sbIPlaybackHistoryListener>*>(aUserData);

  PRBool success = array->AppendObject(aEntry);
  NS_ENSURE_TRUE(success, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

NS_METHOD 
sbPlaybackHistoryService::Init()
{
  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIObserverService> observerService = 
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, 
                                    SB_LIBRARY_MANAGER_READY_TOPIC, 
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);


  rv = observerService->AddObserver(this, 
                                    SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC, 
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  mMonitor = nsAutoMonitor::NewMonitor("sbPlaybackHistoryService::mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  rv = CreateQueries();
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mLibraries.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mListeners.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mPropertyDBIDToID.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mPropertyIDToDBID.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsISupportsWeakReference> weakRef = 
    do_GetService(SB_MEDIACOREMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = weakRef->GetWeakReference(getter_AddRefs(mMediacoreManager));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediacoreEventTarget> eventTarget = 
    do_QueryReferent(mMediacoreManager, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = eventTarget->AddListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef METRICS_ENABLED
  mMetrics = do_ProxiedCreateInstance("@songbirdnest.com/Songbird/Metrics;1", 
                                      &rv);
  NS_ENSURE_SUCCESS(rv, rv);
#endif

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::CreateQueries()
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  NS_NAMED_LITERAL_STRING(playbackHistoryEntriesTableName, 
                          PLAYBACKHISTORY_ENTRIES_TABLE);

  NS_NAMED_LITERAL_STRING(playbackHistoryAnnotationsTableName,
                          PLAYBACKHISTORY_ANNOTATIONS_TABLE);

    NS_NAMED_LITERAL_STRING(playbackHistoryCountEntries, 
                            PLAYBACKHISTORY_COUNT_ENTRIES);

  NS_NAMED_LITERAL_STRING(entryIdColumn, ENTRY_ID_COLUMN);
  NS_NAMED_LITERAL_STRING(libraryGuidColumn, LIBRARY_GUID_COLUMN);
  NS_NAMED_LITERAL_STRING(mediaItemGuidColumn, MEDIA_ITEM_GUID_COLUMN);
  NS_NAMED_LITERAL_STRING(playTimeColumn, PLAY_TIME_COLUMN);
  NS_NAMED_LITERAL_STRING(playDurationColumn, PLAY_DURATION_COLUMN);

  NS_NAMED_LITERAL_STRING(propertiesTableName, 
                          PLAYBACKHISTORY_PROPERTIES_TABLE);

  NS_NAMED_LITERAL_STRING(propertyIdColumn, PROPERTY_ID_COLUMN);
  NS_NAMED_LITERAL_STRING(propertyNameColumn, PROPERTY_NAME_COLUMN);
  NS_NAMED_LITERAL_STRING(objColumn, OBJ_COLUMN);
  NS_NAMED_LITERAL_STRING(objSortableColumn, OBJ_SORTABLE);

  nsCOMPtr<sbISQLInsertBuilder> insertBuilder = 
    do_CreateInstance(SB_SQLBUILDER_INSERT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for Inserting new Playback History Entries.
  rv = insertBuilder->SetIntoTableName(playbackHistoryEntriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->AddColumn(libraryGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->AddColumn(mediaItemGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->AddColumn(playTimeColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->AddColumn(playDurationColumn);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = insertBuilder->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = insertBuilder->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = insertBuilder->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = insertBuilder->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->ToString(mAddEntryQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for Inserting Annotations
  rv = insertBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->SetIntoTableName(playbackHistoryAnnotationsTableName);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = insertBuilder->AddColumn(entryIdColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->AddValueRaw(NS_LITERAL_STRING("(select entry_id from playback_history_entries where library_guid = ? AND media_item_guid = ? AND play_time = ?)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->AddColumn(propertyIdColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->AddColumn(objColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->AddColumn(objSortableColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->ToString(mAddAnnotationQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Query to insert annotations
  mInsertAnnotationQuery.AssignLiteral("insert into ");
  mInsertAnnotationQuery.Append(playbackHistoryAnnotationsTableName);
  mInsertAnnotationQuery.AppendLiteral(" (");
  
  mInsertAnnotationQuery.Append(entryIdColumn);
  mInsertAnnotationQuery.AppendLiteral(",");

  mInsertAnnotationQuery.Append(propertyIdColumn);
  mInsertAnnotationQuery.AppendLiteral(",");

  mInsertAnnotationQuery.Append(objColumn);
  mInsertAnnotationQuery.AppendLiteral(",");

  mInsertAnnotationQuery.Append(objSortableColumn);

  mInsertAnnotationQuery.AppendLiteral(" )");
  mInsertAnnotationQuery.AppendLiteral(" values (?, ?, ?, ?)");

  // Query to update an annotation
  mUpdateAnnotationQuery.AssignLiteral("update ");
  mUpdateAnnotationQuery.Append(playbackHistoryAnnotationsTableName);
  mUpdateAnnotationQuery.AppendLiteral(" set ");
  
  mUpdateAnnotationQuery.Append(propertyIdColumn);
  mUpdateAnnotationQuery.AppendLiteral(" = ?, ");

  mUpdateAnnotationQuery.Append(objColumn);
  mUpdateAnnotationQuery.AppendLiteral(" = ?, ");

  mUpdateAnnotationQuery.Append(objSortableColumn);
  mUpdateAnnotationQuery.AppendLiteral(" = ? ");
  
  mUpdateAnnotationQuery.AppendLiteral(" where entry_id = ?");

  // Query to remove an annotation
  mRemoveAnnotationQuery.AssignLiteral("delete from ");
  mRemoveAnnotationQuery.Append(playbackHistoryAnnotationsTableName);
  mRemoveAnnotationQuery.AppendLiteral(" where entry_id = ? AND property_id = ?");

  // Query to figure out if an annotation is present already or not.
  mIsAnnotationPresentQuery.AssignLiteral("select entry_id from ");
  mIsAnnotationPresentQuery.Append(playbackHistoryAnnotationsTableName);
  mIsAnnotationPresentQuery.AppendLiteral(" where entry_id = ? and property_id = ?");
  
  // Query for Entry Count
  nsCOMPtr<sbISQLSelectBuilder> selectBuilder = 
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetBaseTableName(playbackHistoryEntriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), playbackHistoryCountEntries);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->ToString(mGetEntryCountQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for inserting a property dbid for a property id.
  rv = insertBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->SetIntoTableName(propertiesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->AddColumn(propertyNameColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->ToString(mInsertPropertyIDQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for Getting Entries by Index
  rv = selectBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetBaseTableName(playbackHistoryEntriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), entryIdColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), libraryGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), mediaItemGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), playTimeColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), playDurationColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddOrder(EmptyString(), playTimeColumn, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetLimitIsParameter(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetOffsetIsParameter(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->ToString(mGetEntriesByIndexQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for Getting Entries by Index, Ascending.
  rv = selectBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetBaseTableName(playbackHistoryEntriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), entryIdColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), libraryGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), mediaItemGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), playTimeColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), playDurationColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddOrder(EmptyString(), playTimeColumn, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetLimitIsParameter(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetOffsetIsParameter(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->ToString(mGetEntriesByIndexQueryAscending);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for Getting Entries by Timestamp
  rv = selectBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetBaseTableName(playbackHistoryEntriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), entryIdColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), libraryGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), mediaItemGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), playTimeColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), playDurationColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddOrder(EmptyString(), playTimeColumn, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterionPlayTimeMin;
  rv = selectBuilder->CreateMatchCriterionParameter(
                                          EmptyString(), 
                                          playTimeColumn,
                                          sbISQLBuilder::MATCH_GREATEREQUAL,
                                          getter_AddRefs(criterionPlayTimeMin));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterionPlayTimeMax;
  rv = selectBuilder->CreateMatchCriterionParameter(
                                          EmptyString(), 
                                          playTimeColumn,
                                          sbISQLBuilder::MATCH_LESSEQUAL,
                                          getter_AddRefs(criterionPlayTimeMax));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterionPlayTimeRange;
  rv = selectBuilder->CreateAndCriterion(criterionPlayTimeMin,
                                         criterionPlayTimeMax,
                                         getter_AddRefs(criterionPlayTimeRange));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddCriterion(criterionPlayTimeRange);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->ToString(mGetEntriesByTimestampQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for Getting Entries by Timestamp, Ascending.
  rv = selectBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetBaseTableName(playbackHistoryEntriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), entryIdColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), libraryGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), mediaItemGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), playTimeColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), playDurationColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddOrder(EmptyString(), playTimeColumn, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddCriterion(criterionPlayTimeRange);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->ToString(mGetEntriesByTimestampQueryAscending);
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for Getting All Entries
  rv = selectBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetBaseTableName(playbackHistoryEntriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), entryIdColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), libraryGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), mediaItemGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), playTimeColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), playDurationColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddOrder(EmptyString(), playTimeColumn, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->ToString(mGetAllEntriesQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for Getting All Annotations Related to an Entry Id.
  rv = selectBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetBaseTableName(playbackHistoryAnnotationsTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), propertyIdColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), objColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterionEntryId;
  rv = selectBuilder->CreateMatchCriterionParameter(
                                            EmptyString(), 
                                            entryIdColumn,
                                            sbISQLBuilder::MATCH_EQUALS,
                                            getter_AddRefs(criterionEntryId));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddCriterion(criterionEntryId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->ToString(mGetAnnotationsForEntryQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for Deleting Entries by Index
  nsCOMPtr<sbISQLDeleteBuilder> deleteBuilder =
    do_CreateInstance(SB_SQLBUILDER_DELETE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteBuilder->SetTableName(playbackHistoryEntriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterionIn> criterionIn; 
  rv = deleteBuilder->CreateMatchCriterionIn(EmptyString(), 
                                            entryIdColumn,
                                            getter_AddRefs(criterionIn));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetBaseTableName(playbackHistoryEntriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), entryIdColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterionLibraryGuid;
  rv = selectBuilder->CreateMatchCriterionParameter(
                              EmptyString(), 
                              libraryGuidColumn,
                              sbISQLBuilder::MATCH_EQUALS,
                              getter_AddRefs(criterionLibraryGuid));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterionMediaItemGuid;
  rv = selectBuilder->CreateMatchCriterionParameter(
                              EmptyString(), 
                              mediaItemGuidColumn,
                              sbISQLBuilder::MATCH_EQUALS,
                              getter_AddRefs(criterionMediaItemGuid));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterionPlayTime;
  rv = selectBuilder->CreateMatchCriterionParameter(
                              EmptyString(), 
                              playTimeColumn,
                              sbISQLBuilder::MATCH_EQUALS,
                              getter_AddRefs(criterionPlayTime));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterionLeft;
  rv = selectBuilder->CreateAndCriterion(criterionLibraryGuid, 
                                         criterionMediaItemGuid,
                                         getter_AddRefs(criterionLeft));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterionRight;
  rv = selectBuilder->CreateAndCriterion(criterionLeft,
                                         criterionPlayTime,
                                         getter_AddRefs(criterionRight));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddCriterion(criterionRight);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = criterionIn->AddSubquery(selectBuilder);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteBuilder->AddCriterion(criterionIn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteBuilder->ToString(mRemoveEntriesQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for Deleting Annotations
  rv = deleteBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteBuilder->SetTableName(playbackHistoryEntriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->CreateMatchCriterionParameter(
                                        EmptyString(), 
                                        entryIdColumn,
                                        sbISQLBuilder::MATCH_EQUALS,
                                        getter_AddRefs(criterionEntryId));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteBuilder->AddCriterion(criterionEntryId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteBuilder->ToString(mRemoveAnnotationsQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for Deleting All Entries
  rv = deleteBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteBuilder->SetTableName(playbackHistoryEntriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteBuilder->ToString(mRemoveAllEntriesQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for Deleting All Annotations
  rv = deleteBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteBuilder->SetTableName(playbackHistoryAnnotationsTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteBuilder->ToString(mRemoveAllAnnotationsQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPlaybackHistoryService::CreateDefaultQuery(sbIDatabaseQuery **aQuery)
{
  NS_ENSURE_ARG_POINTER(aQuery);

  NS_NAMED_LITERAL_STRING(playbackHistoryDatabaseGUID, 
                          PLAYBACKHISTORY_DB_GUID);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<sbIDatabaseQuery> query = 
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(playbackHistoryDatabaseGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  query.forget(aQuery);

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::CreateAnnotationsFromEntryId(
                                      PRInt64 aEntryId, 
                                      sbIPropertyArray **aAnnotations)
{
  NS_ENSURE_ARG_POINTER(aAnnotations);
  NS_ENSURE_ARG(aEntryId != -1);

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mGetAnnotationsForEntryQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt64Parameter(0, aEntryId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk = 0;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  PRUint32 rowCount = 0;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMutablePropertyArray> annotations = 
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);  

  for(PRUint32 current = 0; current < rowCount; ++current) {
    nsString propertyDBIDStr;
    rv = result->GetRowCell(current, 0, propertyDBIDStr);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString annotationValue;
    rv = result->GetRowCell(current, 1, annotationValue);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyId = propertyDBIDStr.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString annotationId;
    PRBool success = mPropertyDBIDToID.Get(propertyId, &annotationId);
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

    rv = annotations->AppendProperty(annotationId, annotationValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbIPropertyArray> immutableAnnotations = 
    do_QueryInterface(annotations, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  immutableAnnotations.forget(aAnnotations);

  return NS_OK;
}


nsresult 
sbPlaybackHistoryService::CreateEntryFromResultSet(sbIDatabaseResult *aResult,
                                                   PRUint32 aRow,
                                                   sbIPlaybackHistoryEntry **aEntry)
{
  NS_ENSURE_ARG_POINTER(aResult);
  NS_ENSURE_ARG_POINTER(aEntry);

  PRUint32 rowCount = 0;
  nsresult rv = aResult->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(aRow < rowCount, NS_ERROR_INVALID_ARG);

  nsString entryIdStr;
  rv = aResult->GetRowCell(aRow, 0, entryIdStr);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString libraryGuid;
  rv = aResult->GetRowCell(aRow, 1, libraryGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString mediaItemGuid;
  rv = aResult->GetRowCell(aRow, 2, mediaItemGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString playTime;
  rv = aResult->GetRowCell(aRow, 3, playTime);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString playDuration;
  rv = aResult->GetRowCell(aRow, 4, playDuration);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 timestamp = nsString_ToUint64(playTime, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 duration = nsString_ToUint64(playDuration, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;
  rv = GetItem(libraryGuid, mediaItemGuid, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 entryId = nsString_ToUint64(entryIdStr, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyArray> annotations;
  rv = CreateAnnotationsFromEntryId(entryId, getter_AddRefs(annotations));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPlaybackHistoryEntry> entry;
  rv = CreateEntry(item, timestamp, duration, annotations, getter_AddRefs(entry));
  NS_ENSURE_SUCCESS(rv, rv);

  entry->SetEntryId(entryId);
  entry.forget(aEntry);

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::CreateEntriesFromResultSet(sbIDatabaseResult *aResult,
                                                     nsIArray **aEntries)
{
  NS_ENSURE_ARG_POINTER(aResult);
  NS_ENSURE_ARG_POINTER(aEntries);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIMutableArray> mutableArray = 
    do_CreateInstance(SB_THREADSAFE_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount = 0;
  rv = aResult->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRUint32 currentRow = 0; currentRow < rowCount; ++currentRow) {
    nsCOMPtr<sbIPlaybackHistoryEntry> entry;
    
    rv = CreateEntryFromResultSet(aResult, currentRow, getter_AddRefs(entry));
    // failing to find the item is not fatal, it only means that it has been
    // deleted from the library, so just skip it. Do not remove it from the
    // history though, so that if the same item is added again, we'll still
    // have its data.
    if (rv == NS_ERROR_NOT_AVAILABLE) 
      continue;

    rv = mutableArray->AppendElement(entry, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIArray> array = do_QueryInterface(mutableArray);
  array.forget(aEntries);

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::EnsureHistoryDatabaseAvailable()
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsILocalFile> file = GetDBFolder();
  NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

  nsString dbFileName = NS_LITERAL_STRING(PLAYBACKHISTORY_DB_GUID);
  dbFileName.AppendLiteral(".db");

  rv = file->AppendRelativePath(dbFileName);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists = PR_FALSE;
  rv = file->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if(exists) {
    return NS_OK;
  }

  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(NS_LITERAL_STRING(PLAYBACKHISTORY_DB_GUID));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> schemaURI;
  rv = NS_NewURI(getter_AddRefs(schemaURI), NS_LITERAL_CSTRING(SCHEMA_URL));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> input;
  rv = NS_OpenURI(getter_AddRefs(input), schemaURI);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIConverterInputStream> converterStream =
    do_CreateInstance("@mozilla.org/intl/converter-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = converterStream->Init(input,
                             "UTF-8",
                             CONVERTER_BUFFER_SIZE,
                             nsIConverterInputStream::DEFAULT_REPLACEMENT_CHARACTER);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIUnicharInputStream> unichar =
    do_QueryInterface(converterStream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 read = 0;
  nsString response;
  rv = unichar->ReadString(PR_UINT32_MAX, response, &read);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(read, "Schema file zero bytes?");

  rv = unichar->Close();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING(colonNewline, ";\n");

  PRInt32 posStart = 0;
  PRInt32 posEnd = response.Find(colonNewline, posStart);
  while (posEnd >= 0) {
    rv = query->AddQuery(Substring(response, posStart, posEnd - posStart));
    NS_ENSURE_SUCCESS(rv, rv);
    posStart = posEnd + 2;
    posEnd = response.Find(colonNewline, posStart);
  }

  PRInt32 dbOk = 0;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::FillAddQueryParameters(sbIDatabaseQuery *aQuery,
                                                 sbIPlaybackHistoryEntry *aEntry)
{
  NS_ENSURE_ARG_POINTER(aQuery);
  NS_ENSURE_ARG_POINTER(aEntry);

  nsCOMPtr<sbIMediaItem> item;
  nsresult rv = aEntry->GetItem(getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> library;
  rv = item->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString libraryGuid;
  rv = library->GetGuid(libraryGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aQuery->BindStringParameter(0, libraryGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString itemGuid;
  rv = item->GetGuid(itemGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aQuery->BindStringParameter(1, itemGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  PRTime timestamp = 0;
  rv = aEntry->GetTimestamp(&timestamp);
  NS_ENSURE_SUCCESS(rv, rv);

  char buf[30];
  PRUint32 len = PR_snprintf(buf, sizeof(buf), "%lld", timestamp);

  NS_ConvertASCIItoUTF16 timestampString(buf, len);
  rv = aQuery->BindStringParameter(2, timestampString);
  NS_ENSURE_SUCCESS(rv, rv);

  PRTime duration = 0;
  rv = aEntry->GetDuration(&duration);
  NS_ENSURE_SUCCESS(rv, rv);

  // duration is optional so if it's not present we bind to null instead of 0.
  if(duration) {
    len = PR_snprintf(buf, sizeof(buf), "%lld", duration);
    NS_ConvertASCIItoUTF16 durationString(buf, len);
    
    rv = aQuery->BindStringParameter(3, durationString);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = aQuery->BindNullParameter(3);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aQuery->AddQuery(NS_LITERAL_STRING("select last_insert_rowid()"));

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::FillAddAnnotationsQueryParameters(
                                           sbIDatabaseQuery *aQuery,
                                           sbIPlaybackHistoryEntry *aEntry)
{
  NS_ENSURE_ARG_POINTER(aQuery);
  NS_ENSURE_ARG_POINTER(aEntry);

  nsCOMPtr<sbIPropertyArray> annotations;
  nsresult rv = aEntry->GetAnnotations(getter_AddRefs(annotations));
  NS_ENSURE_SUCCESS(rv, rv);

  if(!annotations) {
    return NS_OK;
  }

  nsCOMPtr<sbIPropertyManager> propMan =
    do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;
  rv = aEntry->GetItem(getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> library;
  rv = item->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString libraryGuid;
  rv = library->GetGuid(libraryGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString itemGuid;
  rv = item->GetGuid(itemGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  PRTime timestamp = 0;
  rv = aEntry->GetTimestamp(&timestamp);
  NS_ENSURE_SUCCESS(rv, rv);

  char buf[30];
  PRUint32 len = PR_snprintf(buf, sizeof(buf), "%lld", timestamp);
  NS_ConvertASCIItoUTF16 timestampString(buf, len);
  
  PRUint32 length = 0;
  rv = annotations->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRUint32 current = 0; current < length; ++current) {
    nsCOMPtr<sbIProperty> annotation;
    rv = annotations->GetPropertyAt(current, getter_AddRefs(annotation));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString id;
    rv = annotation->GetId(id);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString value;
    rv = annotation->GetValue(value);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyId;
    rv = GetPropertyDBID(id, &propertyId);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIPropertyInfo> propertyInfo;
    rv = propMan->GetPropertyInfo(id, getter_AddRefs(propertyInfo));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString sortableValue;
    rv = propertyInfo->MakeSortable(value, sortableValue);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aQuery->AddQuery(mAddAnnotationQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aQuery->BindStringParameter(0, libraryGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aQuery->BindStringParameter(1, itemGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aQuery->BindStringParameter(2, timestampString);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aQuery->BindInt32Parameter(3, propertyId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aQuery->BindStringParameter(4, value);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aQuery->BindStringParameter(5, sortableValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::FillRemoveEntryQueryParameters(sbIDatabaseQuery *aQuery,
                                                         sbIPlaybackHistoryEntry *aEntry)
{
  NS_ENSURE_ARG_POINTER(aQuery);
  NS_ENSURE_ARG_POINTER(aEntry);

  nsCOMPtr<sbIMediaItem> item;
  nsresult rv = aEntry->GetItem(getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> library;
  rv = item->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString libraryGuid;
  rv = library->GetGuid(libraryGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aQuery->BindStringParameter(0, libraryGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString itemGuid;
  rv = item->GetGuid(itemGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aQuery->BindStringParameter(1, itemGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  PRTime timestamp = 0;
  rv = aEntry->GetTimestamp(&timestamp);
  NS_ENSURE_SUCCESS(rv, rv);

  char buf[30];
  PRUint32 len = PR_snprintf(buf, sizeof(buf), "%lld", timestamp);

  NS_ConvertASCIItoUTF16 timestampString(buf, len);
  rv = aQuery->BindStringParameter(2, timestampString);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::GetItem(const nsAString &aLibraryGuid,
                                  const nsAString &aItemGuid,
                                  sbIMediaItem **aItem)
{
  NS_ENSURE_ARG_POINTER(aItem);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<sbILibrary> library;
  if(!mLibraries.Get(aLibraryGuid, getter_AddRefs(library))) {
    nsCOMPtr<sbILibraryManager> libraryManager = 
      do_GetService(SONGBIRD_LIBRARYMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = libraryManager->GetLibrary(aLibraryGuid, getter_AddRefs(library));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool success = mLibraries.Put(aLibraryGuid, library);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }
  
  nsCOMPtr<sbIMediaItem> item;
  rv = library->GetMediaItem(aItemGuid, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  item.forget(aItem);

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::GetPropertyDBID(const nsAString &aPropertyID,
                                          PRUint32 *aPropertyDBID)
{
  NS_ENSURE_ARG_POINTER(aPropertyDBID);
  *aPropertyDBID = 0;

  if (!mPropertyIDToDBID.Get(aPropertyID, aPropertyDBID)) {
    nsresult rv = InsertPropertyID(aPropertyID, aPropertyDBID);
    NS_ENSURE_SUCCESS (rv, rv);
  }

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::InsertPropertyID(const nsAString &aPropertyID, 
                                           PRUint32 *aPropertyDBID)
{
  NS_ENSURE_ARG_POINTER(aPropertyDBID);

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mInsertPropertyIDQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, aPropertyID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(NS_LITERAL_STRING("select last_insert_rowid()"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk = 0;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  nsAutoString propertyDBIDStr;
  rv = result->GetRowCell(0, 0, propertyDBIDStr);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 propertyDBID = propertyDBIDStr.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  *aPropertyDBID = propertyDBID;

  mPropertyDBIDToID.Put(propertyDBID, nsAutoString(aPropertyID));
  mPropertyIDToDBID.Put(nsAutoString(aPropertyID), propertyDBID);

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::LoadPropertyIDs()
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<sbISQLSelectBuilder> builder =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableName(NS_LITERAL_STRING(PLAYBACKHISTORY_PROPERTIES_TABLE));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(), NS_LITERAL_STRING(PROPERTY_ID_COLUMN));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(), NS_LITERAL_STRING(PROPERTY_NAME_COLUMN));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString sql;
  rv = builder->ToString(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk = 0;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < rowCount; i++) {
    nsAutoString propertyDBIDStr;
    rv = result->GetRowCell(i, 0, propertyDBIDStr);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyDBID = propertyDBIDStr.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString propertyID;
    rv = result->GetRowCell(i, 1, propertyID);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool success = mPropertyDBIDToID.Put(propertyDBID, propertyID);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    success = mPropertyIDToDBID.Put(propertyID, propertyDBID);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::DoEntryAddedCallback(sbIPlaybackHistoryEntry *aEntry)
{
  nsCOMArray<sbIPlaybackHistoryListener> listeners;

  mListeners.EnumerateRead(AddListenersToCOMArrayCallback, 
                           (void *)&listeners);

  nsCOMPtr<sbIPlaybackHistoryListener> listener;
  PRInt32 count = listeners.Count();

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIMutableArray> array = 
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = array->AppendElement(aEntry, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRInt32 current = 0; current < count; ++current) {
    rv = listeners[current]->OnEntriesAdded(array);

#if defined(DEBUG)
    NS_ENSURE_SUCCESS(rv, rv);
#endif
  }

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::DoEntriesAddedCallback(nsIArray *aEntries)
{
  nsCOMArray<sbIPlaybackHistoryListener> listeners;

  mListeners.EnumerateRead(AddListenersToCOMArrayCallback, 
    (void *)&listeners);

  nsCOMPtr<sbIPlaybackHistoryListener> listener;
  PRInt32 count = listeners.Count();

  for(PRInt32 current = 0; current < count; ++current) {
    nsresult SB_UNUSED_IN_RELEASE(rv) =
        listeners[current]->OnEntriesAdded(aEntries);

#if defined(DEBUG)
    NS_ENSURE_SUCCESS(rv, rv);
#endif
  }

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::DoEntryUpdatedCallback(sbIPlaybackHistoryEntry *aEntry)
{
  nsCOMArray<sbIPlaybackHistoryListener> listeners;

  mListeners.EnumerateRead(AddListenersToCOMArrayCallback, 
    (void *)&listeners);

  nsCOMPtr<sbIPlaybackHistoryListener> listener;
  PRInt32 count = listeners.Count();

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIMutableArray> array = 
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = array->AppendElement(aEntry, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRInt32 current = 0; current < count; ++current) {
    rv = listeners[current]->OnEntriesUpdated(array);

#if defined(DEBUG)
    NS_ENSURE_SUCCESS(rv, rv);
#endif
  }

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::DoEntriesUpdatedCallback(nsIArray *aEntries)
{
  nsCOMArray<sbIPlaybackHistoryListener> listeners;

  mListeners.EnumerateRead(AddListenersToCOMArrayCallback, 
    (void *)&listeners);

  nsCOMPtr<sbIPlaybackHistoryListener> listener;
  PRInt32 count = listeners.Count();

  for(PRInt32 current = 0; current < count; ++current) {
    nsresult SB_UNUSED_IN_RELEASE(rv) =
        listeners[current]->OnEntriesUpdated(aEntries);

#if defined(DEBUG)
    NS_ENSURE_SUCCESS(rv, rv);
#endif
  }

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::DoEntryRemovedCallback(sbIPlaybackHistoryEntry *aEntry)
{
  nsCOMArray<sbIPlaybackHistoryListener> listeners;

  mListeners.EnumerateRead(AddListenersToCOMArrayCallback, 
    (void *)&listeners);

  nsCOMPtr<sbIPlaybackHistoryListener> listener;
  PRInt32 count = listeners.Count();

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIMutableArray> array = 
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = array->AppendElement(aEntry, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRInt32 current = 0; current < count; ++current) {
    rv = listeners[current]->OnEntriesRemoved(array);

#if defined(DEBUG)
    NS_ENSURE_SUCCESS(rv, rv);
#endif
  }

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::DoEntriesRemovedCallback(nsIArray *aEntries)
{
  nsCOMArray<sbIPlaybackHistoryListener> listeners;

  mListeners.EnumerateRead(AddListenersToCOMArrayCallback, 
    (void *)&listeners);

  nsCOMPtr<sbIPlaybackHistoryListener> listener;
  PRInt32 count = listeners.Count();

  for(PRInt32 current = 0; current < count; ++current) {
    nsresult SB_UNUSED_IN_RELEASE(rv) =
        listeners[current]->OnEntriesRemoved(aEntries);

#if defined(DEBUG)
    NS_ENSURE_SUCCESS(rv, rv);
#endif
  }

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::DoEntriesClearedCallback()
{
  nsCOMArray<sbIPlaybackHistoryListener> listeners;

  mListeners.EnumerateRead(AddListenersToCOMArrayCallback, 
    (void *)&listeners);

  nsCOMPtr<sbIPlaybackHistoryListener> listener;
  PRInt32 count = listeners.Count();

  for(PRInt32 current = 0; current < count; ++current) {
    nsresult SB_UNUSED_IN_RELEASE(rv) =
        listeners[current]->OnEntriesCleared();

#if defined(DEBUG)
    NS_ENSURE_SUCCESS(rv, rv);
#endif
  }

  return NS_OK;
}

nsresult
sbPlaybackHistoryService::UpdateTrackingDataFromEvent(sbIMediacoreEvent *aEvent)
{
  NS_ENSURE_ARG_POINTER(aEvent);

  nsCOMPtr<nsIVariant> variant;
  nsresult rv = aEvent->GetData(getter_AddRefs(variant));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> supports;
  rv = variant->GetAsISupports(getter_AddRefs(supports));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item = do_QueryInterface(supports, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentItem = item;
  mCurrentlyTracking = PR_TRUE;

  mCurrentStartTime = 0;
  mCurrentDelta = 0;

  return NS_OK;
}

nsresult
sbPlaybackHistoryService::UpdateCurrentViewFromEvent(sbIMediacoreEvent *aEvent)
{
  NS_ENSURE_ARG_POINTER(aEvent);

  nsCOMPtr<nsIVariant> variant;
  nsresult rv = aEvent->GetData(getter_AddRefs(variant));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> supports;
  rv = variant->GetAsISupports(getter_AddRefs(supports));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaListView> view = do_QueryInterface(supports, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  view.swap(mCurrentView);

  return NS_OK;
}

nsresult
sbPlaybackHistoryService::VerifyDataAndCreateNewEntry()
{
  nsAutoMonitor mon(mMonitor);

  NS_ENSURE_STATE(mCurrentlyTracking);
  NS_ENSURE_STATE(mCurrentItem);
  NS_ENSURE_STATE(mCurrentStartTime);

  if(!mCurrentlyTracking) {
    return NS_ERROR_UNEXPECTED;
  }

  // figure out actual playing time in milliseconds.
  PRTime actualPlayingTime = PR_Now() - mCurrentStartTime - mCurrentDelta;
  actualPlayingTime /= PR_USEC_PER_MSEC;
  
  NS_NAMED_LITERAL_STRING(PROPERTY_DURATION, SB_PROPERTY_DURATION);
  NS_NAMED_LITERAL_STRING(PROPERTY_PLAYCOUNT, SB_PROPERTY_PLAYCOUNT);
  NS_NAMED_LITERAL_STRING(PROPERTY_SKIPCOUNT, SB_PROPERTY_SKIPCOUNT);
  NS_NAMED_LITERAL_STRING(PROPERTY_LASTPLAYTIME, SB_PROPERTY_LASTPLAYTIME);
  NS_NAMED_LITERAL_STRING(PROPERTY_LASTSKIPTIME, SB_PROPERTY_LASTSKIPTIME);
  NS_NAMED_LITERAL_STRING(PROPERTY_EXCLUDE_FROM_HISTORY, 
      SB_PROPERTY_EXCLUDE_FROM_HISTORY);

  nsString durationStr;
  nsresult rv = mCurrentItem->GetProperty(PROPERTY_DURATION, durationStr);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 duration = nsString_ToInt64(durationStr, &rv);
  duration /= PR_USEC_PER_MSEC;
  NS_ENSURE_SUCCESS(rv, rv);

  nsString excludeFromHistoryStr;
  rv = mCurrentItem->GetProperty(PROPERTY_EXCLUDE_FROM_HISTORY, 
      excludeFromHistoryStr);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool excludeFromHistory = excludeFromHistoryStr.EqualsLiteral("1");

  // if we played for at least 240 seconds (matching audioscrobbler)
  // or more than half the track (matching audioscrobbler)
  if((duration && (actualPlayingTime >= (duration / 2))) ||
     actualPlayingTime >= (240 * 1000)) {

    // increment play count.
    nsString playCountStr;
    rv = mCurrentItem->GetProperty(PROPERTY_PLAYCOUNT, playCountStr);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint64 playCount = 0;
    if(!playCountStr.IsEmpty()) {
      playCount = nsString_ToUint64(playCountStr, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    playCount++;

    sbAutoString newPlayCountStr(playCount);
    rv = mCurrentItem->SetProperty(PROPERTY_PLAYCOUNT, newPlayCountStr);
    NS_ENSURE_SUCCESS(rv, rv);

    // update last time played
    sbAutoString newLastPlayTimeStr((PRUint64)(mCurrentStartTime / 
                                               PR_USEC_PER_MSEC));
    rv = mCurrentItem->SetProperty(PROPERTY_LASTPLAYTIME, newLastPlayTimeStr);
    NS_ENSURE_SUCCESS(rv, rv);

    // create new playback history entry, if appropriate
    if (!excludeFromHistory) {
      nsCOMPtr<sbIPlaybackHistoryEntry> entry;
      rv = CreateEntry(mCurrentItem, 
                       mCurrentStartTime, 
                       actualPlayingTime, 
                       nsnull, 
                       getter_AddRefs(entry));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = AddEntry(entry);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else {
    // update skip count.
    nsString skipCountStr;
    rv = mCurrentItem->GetProperty(PROPERTY_SKIPCOUNT, skipCountStr);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint64 skipCount = 0;
    if(!skipCountStr.IsEmpty()) {
      skipCount = nsString_ToUint64(skipCountStr, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    skipCount++;

    sbAutoString newSkipCountStr(skipCount);
    rv = mCurrentItem->SetProperty(PROPERTY_SKIPCOUNT, newSkipCountStr);
    NS_ENSURE_SUCCESS(rv, rv);

    // update last skip time.
    sbAutoString newLastSkipTimeStr((PRUint64) (mCurrentStartTime / 
                                                PR_USEC_PER_MSEC));
    rv = mCurrentItem->SetProperty(PROPERTY_LASTSKIPTIME, newLastSkipTimeStr);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  /* 
    Playback metrics are timing out causing the play count
    to fail to increment!
#ifdef METRICS_ENABLED
    // Regardless, we update playback metrics.
    // rv = UpdateMetrics();
    // NS_ENSURE_SUCCESS(rv, rv);
#endif
  */

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::ResetTrackingData()
{
  nsAutoMonitor mon(mMonitor);

  mCurrentlyTracking = PR_FALSE;
  mCurrentStartTime = 0;
  mCurrentPauseTime = 0;
  mCurrentDelta = 0;
  mCurrentItem = nsnull;

  return NS_OK;
}

#ifdef METRICS_ENABLED
nsresult 
sbPlaybackHistoryService::UpdateMetrics()
{
  nsAutoMonitor mon(mMonitor);

  NS_ENSURE_STATE(mCurrentView);
  NS_ENSURE_STATE(mCurrentItem);

  if (!mMetrics) {
    // metrics not available, e.g. after library shutdown
    return NS_OK;
  }

  // figure out actual playing time in seconds.
  PRTime actualPlayingTime = PR_Now() - mCurrentStartTime - mCurrentDelta;
  actualPlayingTime /= PR_USEC_PER_SEC;

  // increment total items played.
  nsresult rv = mMetrics->MetricsInc(NS_LITERAL_STRING(METRIC_MEDIACORE_ROOT),
                                     NS_LITERAL_STRING(METRIC_PLAY),
                                     NS_LITERAL_STRING(METRIC_ITEM));
  NS_ENSURE_SUCCESS(rv, rv);

  // increment total play time.
  rv = mMetrics->MetricsAdd(NS_LITERAL_STRING(METRIC_MEDIACORE_ROOT),
                                     NS_LITERAL_STRING(METRIC_PLAYTIME),
                                     EmptyString(),
                                     actualPlayingTime);
  NS_ENSURE_SUCCESS(rv, rv);

  // increment the appropriate bitrate bucket.
  nsString bitRateStr;
  rv = mCurrentItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_BITRATE), 
                                 bitRateStr);
  if(NS_FAILED(rv) || bitRateStr.IsEmpty()) {
    bitRateStr.AssignLiteral(METRIC_UNKNOWN);
  }

  nsString bitRateKey(NS_LITERAL_STRING(METRIC_BITRATE));
  bitRateKey.AppendLiteral(".");
  bitRateKey += bitRateStr;

  rv = mMetrics->MetricsInc(NS_LITERAL_STRING(METRIC_MEDIACORE_ROOT),
                            NS_LITERAL_STRING(METRIC_PLAY),
                            bitRateKey);
  NS_ENSURE_SUCCESS(rv, rv);

  // increment appropriate medialist total play time.
  nsCOMPtr<sbILibrary> library;
  rv = mCurrentItem->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> libraryList = do_QueryInterface(library, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> list;
  rv = mCurrentView->GetMediaList(getter_AddRefs(list));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString listCustomType, libraryCustomType;
  rv = list->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CUSTOMTYPE), 
                         listCustomType);
  if(NS_FAILED(rv) || listCustomType.IsEmpty()) {
    listCustomType.AssignLiteral("simple");
  }

  rv = library->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CUSTOMTYPE), 
                            libraryCustomType);
  if(NS_FAILED(rv) || listCustomType.IsEmpty()) {
    libraryCustomType.AssignLiteral("library");
  }

  nsString medialistKey;
  if(listCustomType.EqualsLiteral("dynamic")) {
    medialistKey.AssignLiteral(METRIC_SUBSCRIPTION);
  }
  else if(listCustomType.EqualsLiteral("local") ||
          listCustomType.EqualsLiteral("simple") ||
          libraryCustomType.EqualsLiteral("library")) {
    if(libraryList == list) {
      medialistKey.AssignLiteral(METRIC_LIBRARY);
    }
    else if(libraryCustomType.EqualsLiteral("web")) {
      medialistKey.AssignLiteral(METRIC_WEBPLAYLIST);
    }
    else {
      medialistKey.AssignLiteral("simple");
    }
  }
  else {
    medialistKey = listCustomType;
  }

  rv = mMetrics->MetricsInc(NS_LITERAL_STRING(METRIC_MEDIALIST_ROOT),
                            NS_LITERAL_STRING(METRIC_PLAY),
                            medialistKey);
  NS_ENSURE_SUCCESS(rv, rv);

  // increment appropriate play attempt bucket.
  nsCOMPtr<nsIURI> uri;
  rv = mCurrentItem->GetContentSrc(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString scheme;
  rv = uri->GetScheme(scheme);
  NS_ENSURE_SUCCESS(rv, rv);

  if(scheme.IsEmpty()) {
    scheme = METRIC_UNKNOWN;
  }

  nsCString extension;
  nsCOMPtr<nsIURL> url = do_QueryInterface(uri, &rv);
  if(NS_SUCCEEDED(rv)) {
    rv = url->GetFileExtension(extension);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if(extension.IsEmpty()) {
    extension = METRIC_UNKNOWN;
  }

  // key is attempt.<extension>.<scheme>
  nsString playAttemptKey(NS_LITERAL_STRING(METRIC_ATTEMPT));
  playAttemptKey.AppendLiteral(".");
  playAttemptKey += NS_ConvertUTF8toUTF16(extension);
  playAttemptKey.AppendLiteral(".");
  playAttemptKey += NS_ConvertUTF8toUTF16(scheme);

  rv = mMetrics->MetricsInc(NS_LITERAL_STRING(METRIC_MEDIACORE_ROOT),
                            NS_LITERAL_STRING(METRIC_PLAY),
                            playAttemptKey);
  NS_ENSURE_SUCCESS(rv, rv);

  // increment streaming bucket.
  if(scheme.EqualsLiteral("http") ||
     scheme.EqualsLiteral("https") ||
     scheme.EqualsLiteral("ftp") ||
     scheme.EqualsLiteral("rtsp")) {
    rv = mMetrics->MetricsInc(NS_LITERAL_STRING(METRIC_MEDIACORE_ROOT),
                              NS_LITERAL_STRING(METRIC_PLAY),
                              NS_LITERAL_STRING(METRIC_STREAMING));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
#endif

//-----------------------------------------------------------------------------
// nsIObserver
//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbPlaybackHistoryService::Observe(nsISupports* aSubject,
                                  const char* aTopic,
                                  const PRUnichar* aData)
{
  NS_ENSURE_ARG_POINTER(aTopic);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIObserverService> observerService = 
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if(strcmp(aTopic, SB_LIBRARY_MANAGER_READY_TOPIC) == 0) {
    rv = observerService->RemoveObserver(this, SB_LIBRARY_MANAGER_READY_TOPIC);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = EnsureHistoryDatabaseAvailable();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = LoadPropertyIDs();
    NS_ENSURE_SUCCESS(rv, rv);
  } 
  else if(strcmp(aTopic, SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC) == 0) {
    // Remove all library references
    mLibraries.Clear();
    mCurrentItem = nsnull;
    mCurrentView = nsnull;

#ifdef METRICS_ENABLED
    // Releasing metrics prevents a leak
    mMetrics = nsnull;
    rv = 
      observerService->RemoveObserver(this, 
                                      SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC);
    NS_ENSURE_SUCCESS(rv, rv);
#endif
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
// sbIMediacoreEventListener
//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbPlaybackHistoryService::OnMediacoreEvent(sbIMediacoreEvent *aEvent)
{
  NS_ENSURE_ARG_POINTER(aEvent);

  PRUint32 eventType = 0;
  nsresult rv = aEvent->GetType(&eventType);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoMonitor mon(mMonitor);

  switch(eventType) {
    case sbIMediacoreEvent::STREAM_START: {
      if(mCurrentlyTracking && !mCurrentStartTime) {
        mCurrentStartTime = PR_Now();
      }

      if(mCurrentlyTracking && mCurrentStartTime && mCurrentPauseTime) {
        mCurrentDelta = mCurrentDelta + (PR_Now() - mCurrentPauseTime);
        mCurrentPauseTime = 0;
      }
    }
    break;

    case sbIMediacoreEvent::STREAM_PAUSE: {
      if(mCurrentlyTracking && mCurrentStartTime && !mCurrentPauseTime) {
        mCurrentPauseTime = PR_Now();
      }
    }
    break;

    case sbIMediacoreEvent::STREAM_END:
    case sbIMediacoreEvent::STREAM_STOP: {
      if(mCurrentlyTracking && mCurrentStartTime) {
        rv = VerifyDataAndCreateNewEntry();
        NS_ASSERTION(NS_SUCCEEDED(rv), "Entry won't be created.");

        rv = ResetTrackingData();
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    break;

    case sbIMediacoreEvent::TRACK_CHANGE: {
      // already tracking, check to see if we should add an entry for the 
      // current item before starting to track the next one.
      if(mCurrentlyTracking) {
        rv = VerifyDataAndCreateNewEntry();
        if(NS_FAILED(rv)) {
          rv = ResetTrackingData();
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }

      rv = UpdateTrackingDataFromEvent(aEvent);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    case sbIMediacoreEvent::VIEW_CHANGE: {
      // we need to keep track of the current view for metrics
      rv = UpdateCurrentViewFromEvent(aEvent);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
// sbIPlaybackHistoryService
//-----------------------------------------------------------------------------
NS_IMETHODIMP 
sbPlaybackHistoryService::GetEntries(nsISimpleEnumerator * *aEntries)
{
  NS_ENSURE_ARG_POINTER(aEntries);

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mGetAllEntriesQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbError == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  nsCOMPtr<nsIArray> array;
  rv = CreateEntriesFromResultSet(result, getter_AddRefs(array));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = array->Enumerate(aEntries);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::GetEntryCount(PRUint64 *aEntryCount)
{
  NS_ENSURE_ARG_POINTER(aEntryCount);

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mGetEntryCountQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbError == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Make sure we get one row back
  NS_ENSURE_TRUE(rowCount == 1, NS_ERROR_UNEXPECTED);

  nsAutoString countStr;
  rv = result->GetRowCell(0, 0, countStr);
  NS_ENSURE_SUCCESS(rv, rv);

  *aEntryCount = nsString_ToUint64(countStr, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::CreateEntry(sbIMediaItem *aItem, 
                                      PRInt64 aTimestamp, 
                                      PRInt64 aDuration, 
                                      sbIPropertyArray *aAnnotations, 
                                      sbIPlaybackHistoryEntry **_retval)
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<sbIPlaybackHistoryEntry> entry = 
    do_CreateInstance(SB_PLAYBACKHISTORYENTRY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = entry->Init(aItem, aTimestamp, aDuration, aAnnotations);
  NS_ENSURE_SUCCESS(rv, rv);

  entry.forget(_retval);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::AddEntry(sbIPlaybackHistoryEntry *aEntry)
{
  NS_ENSURE_ARG_POINTER(aEntry);

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mAddEntryQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = FillAddQueryParameters(query, aEntry);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = FillAddAnnotationsQueryParameters(query, aEntry);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbError == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  nsAutoString entryIdStr;
  rv = result->GetRowCell(0, 0, entryIdStr);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 entryId = nsString_ToUint64(entryIdStr, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  aEntry->SetEntryId(entryId);

  rv = DoEntryAddedCallback(aEntry);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::AddEntries(nsIArray *aEntries)
{
  NS_ENSURE_ARG_POINTER(aEntries);

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(NS_LITERAL_STRING("BEGIN"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length = 0;
  rv = aEntries->GetLength(&length);

  for(PRUint32 current = 0; current < length; ++current) {
    nsCOMPtr<sbIPlaybackHistoryEntry> entry = 
      do_QueryElementAt(aEntries, current, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->AddQuery(mAddEntryQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = FillAddQueryParameters(query, entry);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = FillAddAnnotationsQueryParameters(query, entry);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = query->AddQuery(NS_LITERAL_STRING("COMMIT"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbError == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  PRUint32 rowCount = 0;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(length == rowCount, NS_ERROR_UNEXPECTED);

  for(PRUint32 currentRow = 0; currentRow < rowCount; ++currentRow) {
    nsString entryIdStr;
    
    rv = result->GetRowCell(currentRow, 0, entryIdStr);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIPlaybackHistoryEntry> entry = 
      do_QueryElementAt(aEntries, currentRow, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt64 entryId = nsString_ToUint64(entryIdStr, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    entry->SetEntryId(entryId);
  }

  rv = DoEntriesAddedCallback(aEntries);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::GetEntryByIndex(PRInt64 aIndex, 
                                          sbIPlaybackHistoryEntry **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 actualIndex = aIndex;
  if(actualIndex >= 0) {
    rv = query->AddQuery(mGetEntriesByIndexQuery);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = query->AddQuery(mGetEntriesByIndexQueryAscending);
    NS_ENSURE_SUCCESS(rv, rv);

    actualIndex = PR_ABS(actualIndex) - 1;
  }

  rv = query->BindInt64Parameter(0, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt64Parameter(1, actualIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbError == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);
  
  rv = CreateEntryFromResultSet(result, 0, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::GetEntriesByIndex(PRInt64 aStartIndex, 
                                            PRUint64 aCount, 
                                            nsIArray **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 actualIndex = aStartIndex;
  if( actualIndex >= 0) {
    rv = query->AddQuery(mGetEntriesByIndexQuery);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = query->AddQuery(mGetEntriesByIndexQueryAscending);
    NS_ENSURE_SUCCESS(rv, rv);

    actualIndex = PR_ABS(actualIndex) - 1;
  }

  rv = query->BindInt64Parameter(0, aCount);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt64Parameter(1, actualIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbError == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  rv = CreateEntriesFromResultSet(result, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::GetEntriesByTimestamp(PRInt64 aStartTimestamp, 
                                                PRInt64 aEndTimestamp, 
                                                nsIArray **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  PRBool isAscending = aStartTimestamp > aEndTimestamp;
  
  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  if(!isAscending) {
    rv = query->AddQuery(mGetEntriesByTimestampQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindInt64Parameter(0, aStartTimestamp);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindInt64Parameter(1, aEndTimestamp);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = query->AddQuery(mGetEntriesByTimestampQueryAscending);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindInt64Parameter(0, aEndTimestamp);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindInt64Parameter(1, aStartTimestamp);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbError == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  rv = CreateEntriesFromResultSet(result, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::RemoveEntry(sbIPlaybackHistoryEntry *aEntry)
{
  NS_ENSURE_ARG_POINTER(aEntry);

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mRemoveEntriesQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = FillRemoveEntryQueryParameters(query, aEntry);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mRemoveAnnotationsQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 entryId = -1;
  rv = aEntry->GetEntryId(&entryId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt64Parameter(0, entryId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbError == 0, NS_ERROR_FAILURE);

  rv = DoEntryRemovedCallback(aEntry);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::RemoveEntryByIndex(PRInt64 aIndex)
{
  nsCOMPtr<sbIPlaybackHistoryEntry> entry;
  
  nsresult rv = GetEntryByIndex(aIndex, getter_AddRefs(entry));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RemoveEntry(entry);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DoEntryRemovedCallback(entry);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::RemoveEntriesByIndex(PRInt64 aStartIndex, 
                                               PRUint64 aCount)
{
  nsCOMPtr<nsIArray> entries;
  
  nsresult rv = GetEntriesByIndex(aStartIndex, aCount, getter_AddRefs(entries));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RemoveEntries(entries);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DoEntriesRemovedCallback(entries);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::RemoveEntries(nsIArray *aEntries)
{
  NS_ENSURE_ARG_POINTER(aEntries);

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length = 0;
  rv = aEntries->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(NS_LITERAL_STRING("BEGIN"));
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRUint32 current = 0; current < length; ++current) {
    nsCOMPtr<sbIPlaybackHistoryEntry> entry = 
      do_QueryElementAt(aEntries, current, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->AddQuery(mRemoveEntriesQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = FillRemoveEntryQueryParameters(query, entry);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = query->AddQuery(NS_LITERAL_STRING("COMMIT"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbError == 0, NS_ERROR_FAILURE);

  rv = DoEntriesRemovedCallback(aEntries);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::GetEntriesByAnnotation(
                                  const nsAString & aAnnotationId, 
                                  const nsAString & aAnnotationValue, 
                                  PRUint32 aCount, 
                                  nsIArray **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsString sql;
  sql.AssignLiteral("select entry_id, library_guid, media_item_guid, play_time, play_duration from ");
  sql.AppendLiteral(PLAYBACKHISTORY_ENTRIES_TABLE);
  sql.AppendLiteral(" where entry_id in ( ");

  sql.AppendLiteral("select entry_id from ");
  sql.AppendLiteral(PLAYBACKHISTORY_ANNOTATIONS_TABLE);
  sql.AppendLiteral(" where property_id = ? and obj = ? ");

  if(aCount > 0) {
    sql.AppendLiteral(" limit ?");
  }

  sql.AppendLiteral(" ) ");
  sql.AppendLiteral("order by play_time desc");

  PRUint32 propertyId = 0;
  nsresult rv = GetPropertyDBID(aAnnotationId, &propertyId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt32Parameter(0, propertyId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(1, aAnnotationValue);
  NS_ENSURE_SUCCESS(rv, rv);

  if(aCount > 0) {
    rv = query->BindInt32Parameter(2, aCount);
  }

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbError == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  rv = CreateEntriesFromResultSet(result, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::GetEntriesByAnnotations(
                                            sbIPropertyArray *aAnnotations, 
                                            PRUint32 aCount, 
                                            nsIArray **_retval)
{
  NS_ENSURE_ARG_POINTER(aAnnotations);
  NS_ENSURE_ARG_POINTER(_retval);

  nsString sql;
  sql.AssignLiteral("select entry_id, library_guid, media_item_guid, play_time, play_duration from ");
  sql.AppendLiteral(PLAYBACKHISTORY_ENTRIES_TABLE);
  sql.AppendLiteral(" where entry_id in ( ");

  sql.AppendLiteral("select entry_id from ");
  sql.AppendLiteral(PLAYBACKHISTORY_ANNOTATIONS_TABLE);
  sql.AppendLiteral(" where property_id = ? and obj = ? ");

  PRUint32 length = 0;
  nsresult rv = aAnnotations->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRUint32 current = 0; current < length - 1; ++current) {
    sql.AppendLiteral(" or property_id = ? and obj = ? ");
  }

  if(aCount > 0) {
    sql.AppendLiteral(" limit ?");
  }

  sql.AppendLiteral(" ) ");
  sql.AppendLiteral(" order by play_time desc");

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRUint32 current = 0, currentEntry = 0; 
      current < length * 2; 
      current += 2, ++currentEntry) {
    
    nsCOMPtr<sbIProperty> property;
    rv = aAnnotations->GetPropertyAt(currentEntry, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString annotationId;
    rv = property->GetId(annotationId);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString annotationValue;
    rv = property->GetValue(annotationValue);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyId = 0;
    rv = GetPropertyDBID(annotationId, &propertyId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindInt32Parameter(current, propertyId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindStringParameter(current + 1, annotationValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if(aCount > 0) {
    rv = query->BindInt32Parameter(length * 2, aCount);
  }

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbError == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  rv = CreateEntriesFromResultSet(result, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::Clear()
{
  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mRemoveAllEntriesQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mRemoveAllAnnotationsQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbError == 0, NS_ERROR_FAILURE);

  rv = DoEntriesClearedCallback();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::AddListener(sbIPlaybackHistoryListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);

  // Make a proxy for the listener that will always send callbacks to the
  // current thread.
  nsCOMPtr<nsIThread> target;
  nsresult rv = NS_GetCurrentThread(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPlaybackHistoryListener> proxy;
  rv = do_GetProxyForObject(target,
                            NS_GET_IID(sbIPlaybackHistoryListener),
                            aListener,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the proxy to the hash table, using the listener as the key.
  PRBool success = mListeners.Put(aListener, proxy);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::RemoveListener(sbIPlaybackHistoryListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  mListeners.Remove(aListener);
  return NS_OK;
}

nsresult
sbPlaybackHistoryService::AddOrUpdateAnnotation(
                                              PRInt64 aEntryId,
                                              const nsAString &aAnnotationId,
                                              const nsAString &aAnnotationValue)
{
  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<sbIPropertyManager> propMan =
    do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyInfo> propertyInfo;
  rv = propMan->GetPropertyInfo(aAnnotationId, getter_AddRefs(propertyInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString sortableValue;
  rv = propertyInfo->MakeSortable(aAnnotationValue, sortableValue);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 propertyId = 0;
  rv = GetPropertyDBID(aAnnotationId, &propertyId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mIsAnnotationPresentQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt64Parameter(0, aEntryId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt32Parameter(1, propertyId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk = 0;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  PRUint32 rowCount = 0;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);

  if(rowCount == 1) {
    rv = query->AddQuery(mUpdateAnnotationQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindInt32Parameter(0, propertyId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindStringParameter(1, aAnnotationValue);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindStringParameter(2, sortableValue);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindInt64Parameter(3, aEntryId);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = query->AddQuery(mInsertAnnotationQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindInt64Parameter(0, aEntryId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindInt32Parameter(1, propertyId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindStringParameter(2, aAnnotationValue);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindStringParameter(3, sortableValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }


  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult
sbPlaybackHistoryService::RemoveAnnotation(PRInt64 aEntryId,
                                           const nsAString &aAnnotationId)
{
  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = query->AddQuery(mRemoveAnnotationQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt64Parameter(0, aEntryId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 propertyId = 0;
  rv = GetPropertyDBID(aAnnotationId, &propertyId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt32Parameter(1, propertyId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk = 0;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  return NS_OK;
}
