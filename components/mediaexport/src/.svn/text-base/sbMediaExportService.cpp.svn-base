/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */


#include "sbMediaExportService.h"

#include <sbILibraryManager.h>
#include <sbILibrary.h>
#include <sbIMediaExportAgentService.h>

#include <nsICategoryManager.h>
#include <nsIObserverService.h>
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsIThread.h>
#include <nsIProxyObjectManager.h>
#include <nsIRunnable.h>
#include <nsIThreadManager.h>
#include <nsThreadUtils.h>
#include <nsIThreadPool.h>
#include <nsIUpdateService.h>

#include <nsAutoPtr.h>
#include <nsArrayUtils.h>
#include <nsDirectoryServiceUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsMemory.h>

#include <sbLibraryUtils.h>
#include <sbPropertiesCID.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>
#include <sbThreadPoolService.h>
#include <sbMediaListEnumArrayHelper.h>
#include <sbDebugUtils.h>

#include <algorithm>

//------------------------------------------------------------------------------
// To log this module, set the following environment variable:
//   NSPR_LOG_MODULES=sbMediaExportService:5

template <class T>
static nsresult
EnumerateItemsByGuids(typename T::const_iterator const aGuidStringListBegin,
                      typename T::const_iterator const aGuidStringListEnd,
                      sbIMediaList *aMediaList,
                      nsIArray **aRetVal)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aRetVal);

  nsresult rv;

#ifdef PR_LOGGING
  nsString mediaListName;
  rv = aMediaList->GetName(mediaListName);
  NS_ENSURE_SUCCESS(rv, rv);
  LOG("%s: Enumerate items by guids on media list '%s'",
        __FUNCTION__, NS_ConvertUTF16toUTF8(mediaListName).get());
#endif

  nsCOMPtr<sbIMutablePropertyArray> properties =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING(guidProperty, SB_PROPERTY_GUID);

  typename T::const_iterator iter;
  for (iter = aGuidStringListBegin; iter != aGuidStringListEnd; ++iter) {
    rv = properties->AppendProperty(guidProperty, *iter);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsRefPtr<sbMediaListEnumArrayHelper> enumHelper =
    sbMediaListEnumArrayHelper::New();
  NS_ENSURE_TRUE(enumHelper, NS_ERROR_OUT_OF_MEMORY);

  rv = aMediaList->EnumerateItemsByProperties(
      properties,
      enumHelper,
      sbIMediaList::ENUMERATIONTYPE_LOCKING);
  NS_ENSURE_SUCCESS(rv, rv);

  return enumHelper->GetMediaItemsArray(aRetVal);
}

//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS7(sbMediaExportService,
                              sbIMediaExportService,
                              nsIClassInfo,
                              nsIObserver,
                              sbIMediaListListener,
                              sbILocalDatabaseSmartMediaListListener,
                              sbIJobProgress,
                              sbIShutdownJob)

NS_IMPL_CI_INTERFACE_GETTER5(sbMediaExportService,
                             sbIMediaExportService,
                             nsIClassInfo,
                             nsIObserver,
                             sbIJobProgress,
                             sbIShutdownJob)

NS_DECL_CLASSINFO(sbMediaExportService)
NS_IMPL_THREADSAFE_CI(sbMediaExportService)

//------------------------------------------------------------------------------

sbMediaExportService::sbMediaExportService()
  : mIsRunning(PR_FALSE)
  , mTotal(0)
  , mProgress(0)
{
  SB_PRLOG_SETUP(sbMediaExportService);
}

sbMediaExportService::~sbMediaExportService()
{
}

nsresult
sbMediaExportService::Init()
{
  LOG("%s: Starting the export service", __FUNCTION__);

  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this,
                                    SB_LIBRARY_MANAGER_READY_TOPIC,
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this,
                                    SB_LIBRARY_MANAGER_SHUTDOWN_TOPIC,
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, "songbird-shutdown", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create and start the pref controller
  mPrefController = new sbMediaExportPrefController();
  NS_ENSURE_TRUE(mPrefController, NS_ERROR_OUT_OF_MEMORY);

  rv = mPrefController->Init(this);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediaExportService::InitInternal()
{
  LOG("%s: Internal initializing the export service", __FUNCTION__);

  // Don't bother starting any listeners when the service should not run.
  if (!mPrefController->GetShouldExportAnyMedia()) {
    return NS_OK;
  }

  mIsRunning = PR_TRUE;

  // At least one item needs to be exported, setup the library listener.
  nsresult rv;
  nsCOMPtr<sbILibrary> mainLibrary;
  rv = GetMainLibrary(getter_AddRefs(mainLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ListenToMediaList(mainLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  // Find and listen to all playlists as needed.
  if (mPrefController->GetShouldExportPlaylists() ||
      mPrefController->GetShouldExportSmartPlaylists())
  {
    nsCOMPtr<nsIArray> foundPlaylists;
    rv = mainLibrary->GetItemsByProperty(
        NS_LITERAL_STRING(SB_PROPERTY_ISLIST),
        NS_LITERAL_STRING("1"),
        getter_AddRefs(foundPlaylists));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 length;
    rv = foundPlaylists->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 i = 0; i < length; i++) {
      nsCOMPtr<sbIMediaList> curMediaList =
        do_QueryElementAt(foundPlaylists, i, &rv);
      if (NS_FAILED(rv) || !curMediaList) {
        NS_WARNING("ERROR: Could not get the current playlist from an array!");
        continue;
      }

      PRBool shouldWatch = PR_FALSE;
      rv = GetShouldWatchMediaList(curMediaList, &shouldWatch);
      if (NS_SUCCEEDED(rv) && shouldWatch) {
        rv = ListenToMediaList(curMediaList);
        NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                         "ERROR: Could not start listening to a media list!");
      }
    }
  }

  return NS_OK;
}

nsresult
sbMediaExportService::Shutdown()
{
  LOG("%s: Shutting down export service", __FUNCTION__);

  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->RemoveObserver(this, SB_LIBRARY_MANAGER_SHUTDOWN_TOPIC);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = StopListeningMediaLists();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPrefController->Shutdown();
  NS_ENSURE_SUCCESS(rv, rv);

  // Let's ask the update service if there are updates that need to be applied.
  nsCOMPtr<nsIUpdateManager> updateMgr =
    do_GetService("@mozilla.org/updates/update-manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasPendingUpdates = PR_FALSE;
  PRInt32 updateCount;
  rv = updateMgr->GetUpdateCount(&updateCount);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG("%s: Found %i updates", __FUNCTION__, updateCount);

  if (updateCount > 0) {
    for (PRInt32 i = 0; i < updateCount && !hasPendingUpdates; i++) {
      nsCOMPtr<nsIUpdate> curUpdate;
      rv = updateMgr->GetUpdateAt(i, getter_AddRefs(curUpdate));
      if (NS_FAILED(rv)) {
        continue;
      }

      nsString state;
      rv = curUpdate->GetState(state);
      if (NS_FAILED(rv)) {
        continue;
      }

      // According to |nsIUpdate| in "nsIUpdateService.idl", the |state| field
      // will contain the literal string "pending" when there are updates that
      // will be applied.
      if (state.EqualsLiteral("pending")) {
        hasPendingUpdates = PR_TRUE;
      }
    }

    if (hasPendingUpdates) {
      // When there is at least one update according to the update manager, the
      // updater will run the next time Songbird is launched. In order to assist
      // with updates, any currently running instances of the agent need to be
      // killed.
      nsCOMPtr<sbIMediaExportAgentService> agentService =
        do_GetService(SB_MEDIAEXPORTAGENTSERVICE_CONTRACTID, &rv);
      if (NS_SUCCEEDED(rv) && agentService) {
        // First, kill the agent(s).
        rv = agentService->KillActiveAgents();
        NS_ENSURE_SUCCESS(rv, rv);

        // Next, unregister the agent. The agent will re-register for user login
        // the next time it is run.
        rv = agentService->UnregisterExportAgent();
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  return NS_OK;
}

nsresult
sbMediaExportService::StopListeningMediaLists()
{
  LOG("%s: Removing listeners from all media lists", __FUNCTION__);

  nsresult rv;

  if (mIsRunning) {
    // Removing listener references from all the observed media lists.
    for (PRInt32 i = 0; i < mObservedMediaLists.Count(); i++) {
      nsCOMPtr<sbIMediaList> curMediaList = mObservedMediaLists[i];
      if (!curMediaList) {
        NS_WARNING("Could not get a the media list reference!");
        continue;
      }

      rv = curMediaList->RemoveListener(this);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
          "Could not remove media list listener!");

#if PR_LOGGING
      nsString listGuid;
      rv = curMediaList->GetGuid(listGuid);
      NS_ENSURE_SUCCESS(rv, rv);
      LOG("%s: Removing listener for media list '%s'",
            __FUNCTION__, NS_ConvertUTF16toUTF8(listGuid).get());
#endif
    }

    // Remove smart media list listener references from all the observed smart
    // media lists.
    for (PRInt32 i = 0; i < mObservedSmartMediaLists.Count(); i++) {
      nsCOMPtr<sbILocalDatabaseSmartMediaList> curSmartList =
        mObservedSmartMediaLists[i];
      if (!curSmartList) {
        NS_WARNING("Could not get a smart media list reference!");
        continue;
      }

      rv = curSmartList->RemoveSmartMediaListListener(this);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
          "Could not remove the smart media list listener!");

#if PR_LOGGING
      nsString listGuid;
      rv = curSmartList->GetGuid(listGuid);
      NS_ENSURE_SUCCESS(rv, rv);
      LOG("%s: Removing listener for smart media list '%s'",
            __FUNCTION__, NS_ConvertUTF16toUTF8(listGuid).get());
#endif
    }

    mObservedMediaLists.Clear();
    mObservedSmartMediaLists.Clear();

    // Clean up the exported data lists
    mAddedItemsMap.clear();
    mAddedMediaList.clear();
    mRemovedMediaLists.clear();

    mIsRunning = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::OnBoolPrefChanged(const nsAString & aPrefName,
                                        const PRBool aNewPrefValue)
{
  LOG("%s: '%s' pref changed : %s",
        __FUNCTION__,
        NS_ConvertUTF16toUTF8(aPrefName).get(),
        (aNewPrefValue ? "true" : "false"));

  nsresult rv;

  // If the user turned on some export prefs and the service is not running
  // it is now time to start running.
  if (!mIsRunning && mPrefController->GetShouldExportAnyMedia()) {
    rv = InitInternal();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  // Shutdown if the service is currently running
  else if (mIsRunning && !mPrefController->GetShouldExportAnyMedia()) {
    rv = StopListeningMediaLists();
    NS_ENSURE_SUCCESS(rv, rv);

    // Since the user has turned off the media exporting prefs, tell the
    // export-agent to unregister itself. This call will remove any
    // startup/login hooks that the agent has setup.
    nsCOMPtr<sbIMediaExportAgentService> agentService =
      do_GetService(SB_MEDIAEXPORTAGENTSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv) && agentService) {
      rv = agentService->UnregisterExportAgent();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
sbMediaExportService::ListenToMediaList(sbIMediaList *aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsresult rv;

#ifdef PR_LOGGING
  nsString listGuid;
  rv = aMediaList->GetGuid(listGuid);
  NS_ENSURE_SUCCESS(rv, rv);
  LOG("%s: Adding listener for media list '%s",
        __FUNCTION__, NS_ConvertUTF16toUTF8(listGuid).get());
#endif

  nsString listType;
  rv = aMediaList->GetType(listType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (listType.EqualsLiteral("smart")) {
    // Listen to rebuild notices only for smart playlists.
    nsCOMPtr<sbILocalDatabaseSmartMediaList> smartList =
      do_QueryInterface(aMediaList, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = smartList->AddSmartMediaListListener(this);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ENSURE_TRUE(mObservedSmartMediaLists.AppendObject(smartList),
                   NS_ERROR_FAILURE);
  }
  else {
    // Use the regular medialist listener hook for regular playlists.
    if (!mFilteredProperties) {
      mFilteredProperties =
        do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mFilteredProperties->SetStrict(PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);

      // Content URL
      rv = mFilteredProperties->AppendProperty(
          NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
          EmptyString());
      NS_ENSURE_SUCCESS(rv, rv);

      // GUID
      rv = mFilteredProperties->AppendProperty(
          NS_LITERAL_STRING(SB_PROPERTY_GUID),
          EmptyString());
      NS_ENSURE_SUCCESS(rv, rv);

      // Playlist Name
      rv = mFilteredProperties->AppendProperty(
          NS_LITERAL_STRING(SB_PROPERTY_MEDIALISTNAME),
          EmptyString());
      NS_ENSURE_SUCCESS(rv, rv);
    }

    PRUint32 flags = sbIMediaList::LISTENER_FLAGS_ITEMADDED |
                     sbIMediaList::LISTENER_FLAGS_AFTERITEMREMOVED |
                     sbIMediaList::LISTENER_FLAGS_ITEMUPDATED |
                     sbIMediaList::LISTENER_FLAGS_LISTCLEARED;

    rv = aMediaList->AddListener(this, PR_FALSE, flags, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ENSURE_TRUE(mObservedMediaLists.AppendObject(aMediaList),
                   NS_ERROR_FAILURE);
  }

  return NS_OK;
}

nsresult
sbMediaExportService::GetShouldWatchMediaList(sbIMediaList *aMediaList,
                                              PRBool *aShouldWatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aShouldWatch);

  *aShouldWatch = PR_FALSE;
  nsresult rv;

#ifdef PR_LOGGING
  nsString mediaListName;
  rv = aMediaList->GetName(mediaListName);
  NS_ENSURE_SUCCESS(rv, rv);
  LOG("%s: Deciding if medialist '%s' should be watched.",
        __FUNCTION__, NS_ConvertUTF16toUTF8(mediaListName).get());
#endif

  nsString propValue;

  // Don't watch the Downloads folder
  rv = aMediaList->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CUSTOMTYPE),
                               propValue);
  if (NS_FAILED(rv) || propValue.EqualsLiteral("download")) {
    return NS_OK;
  }

  // Don't watch playlists that are owned by iTunes
  rv = aMediaList->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ITUNES_GUID),
                               propValue);
  if (NS_SUCCEEDED(rv) && !propValue.IsEmpty()) {
    return NS_OK;
  }

  // Don't watch subscriptions
  rv = aMediaList->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ISSUBSCRIPTION),
                               propValue);
  if (NS_FAILED(rv) || propValue.EqualsLiteral("1")) {
    return NS_OK;
  }

  // Make sure this list isn't supposed to be hidden
  rv = aMediaList->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                               propValue);
  if (NS_FAILED(rv) || propValue.EqualsLiteral("1")) {
    return NS_OK;
  }

  // Filter out the users choice for playlist exporting
  rv = aMediaList->GetType(propValue);
  NS_ENSURE_SUCCESS(rv, rv);

  if (propValue.EqualsLiteral("simple") &&
      !mPrefController->GetShouldExportPlaylists())
  {
    return NS_OK;
  }

  if (propValue.EqualsLiteral("smart") &&
      !mPrefController->GetShouldExportSmartPlaylists())
  {
    return NS_OK;
  }

  // Passed all tests, this is a playlist that should be watched
  *aShouldWatch = PR_TRUE;
  return NS_OK;
}

void
sbMediaExportService::WriteExportData()
{
  nsresult SB_UNUSED_IN_RELEASE(rv) = WriteChangesToTaskFile();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                   "ERROR: Could not begin exporting songbird data!");
}

void
sbMediaExportService::ProxyNotifyListeners()
{
  nsresult rv;
  rv = NotifyListeners();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                   "ERROR: Could not notify the listeners!");
}

nsresult
sbMediaExportService::WriteChangesToTaskFile()
{
  LOG("%s: Writing recorded media changes to task file", __FUNCTION__);
  nsresult rv;

  if (GetHasRecordedChanges()) {
    LOG("%s: Starting to export data", __FUNCTION__);

    // The order of exported data looks like this
    // 1.) Added media lists
    // 2.) Removed media lists
    // 3.) Medialist added items

    mTaskWriter = new sbMediaExportTaskWriter();
    NS_ENSURE_TRUE(mTaskWriter, NS_ERROR_OUT_OF_MEMORY);

    rv = mTaskWriter->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    // Export media lists first
    if (mPrefController->GetShouldExportAnyPlaylists()) {
      rv = WriteAddedMediaLists();
      NS_ENSURE_SUCCESS(rv, rv);

      rv = NotifyListeners();
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not notify listeners!");

      rv = WriteRemovedMediaLists();
      NS_ENSURE_SUCCESS(rv, rv);

      rv = NotifyListeners();
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not notify listeners!");

      if (mPrefController->GetShouldExportSmartPlaylists()) {
        rv = WriteUpdatedSmartPlaylists();
        NS_ENSURE_SUCCESS(rv, rv);

        rv = NotifyListeners();
        NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not notify listeners!");
      }
    }

    // Export media items next
    if (mPrefController->GetShouldExportTracks()) {
      rv = WriteAddedMediaItems();
      NS_ENSURE_SUCCESS(rv, rv);
      rv = WriteUpdatedMediaItems();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = NotifyListeners();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not notify listeners!");

    rv = mTaskWriter->Finish();
    NS_ENSURE_SUCCESS(rv, rv);
  } // end should write data.

  LOG("%s: Done exporting data", __FUNCTION__);

  mAddedMediaList.clear();
  mAddedItemsMap.clear();
  mRemovedMediaLists.clear();

  mStatus = sbIJobProgress::STATUS_SUCCEEDED;
  rv = NotifyListeners();
  NS_ENSURE_SUCCESS(rv, rv);

  // Only start the export agent if we are supposed to.
  if (mPrefController->GetShouldStartExportAgent()) {
    nsCOMPtr<sbIMediaExportAgentService> agentService =
      do_GetService(SB_MEDIAEXPORTAGENTSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv) && agentService) {
      rv = agentService->StartExportAgent();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
sbMediaExportService::WriteAddedMediaLists()
{
  if (mAddedMediaList.size() == 0) {
    return NS_OK;
  }

  LOG("%s: Writing added media lists", __FUNCTION__);

  NS_ENSURE_TRUE(mTaskWriter, NS_ERROR_UNEXPECTED);

  nsresult rv;
  rv = mTaskWriter->WriteAddedMediaListsHeader();
  NS_ENSURE_SUCCESS(rv, rv);

  sbStringListIter begin = mAddedMediaList.begin();
  sbStringListIter end = mAddedMediaList.end();
  sbStringListIter next;

  for (next = begin; next != end; ++next) {
    nsCOMPtr<sbIMediaList> curMediaList;
    rv = GetMediaListByGuid(*next, getter_AddRefs(curMediaList));
    if (NS_FAILED(rv) || !curMediaList) {
      NS_WARNING("ERROR: Could not get a media list by GUID!");
      continue;
    }

    rv = mTaskWriter->WriteMediaListName(curMediaList);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "ERROR: Could not write the added media list name!");

    ++mProgress;
  }

  return NS_OK;
}

nsresult
sbMediaExportService::WriteRemovedMediaLists()
{
  if (mRemovedMediaLists.size() == 0) {
    return NS_OK;
  }

  LOG("%s: Writing removed media lists", __FUNCTION__);

  NS_ENSURE_TRUE(mTaskWriter, NS_ERROR_UNEXPECTED);

  nsresult rv;
  rv = mTaskWriter->WriteRemovedMediaListsHeader();
  NS_ENSURE_SUCCESS(rv, rv);

  sbStringListIter begin = mRemovedMediaLists.begin();
  sbStringListIter end = mRemovedMediaLists.end();
  sbStringListIter next;

  for (next = begin; next != end; ++next) {
    rv = mTaskWriter->WriteEscapedString(*next);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "ERROR: Could not write the removed media list name!");

    ++mProgress;
  }

  return NS_OK;
}

nsresult
sbMediaExportService::WriteAddedMediaItems()
{
  if (mAddedItemsMap.size() == 0) {
    return NS_OK;
  }

  LOG("%s: Writing %i media lists with added media items",
        __FUNCTION__, mAddedItemsMap.size());

  NS_ENSURE_TRUE(mTaskWriter, NS_ERROR_UNEXPECTED);

  nsresult rv;
  nsCOMPtr<sbILibrary> mainLibrary;
  rv = GetMainLibrary(getter_AddRefs(mainLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString mainLibraryGuid;
  rv = mainLibrary->GetGuid(mainLibraryGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  sbMediaListItemMapIter begin = mAddedItemsMap.begin();
  sbMediaListItemMapIter end = mAddedItemsMap.end();
  sbMediaListItemMapIter next;

  for (next = begin; next != end; ++next) {
    nsString curMediaListGuid(next->first);
    nsCOMPtr<sbIMediaList> curParentList;
    rv = GetMediaListByGuid(curMediaListGuid, getter_AddRefs(curParentList));
    if (NS_FAILED(rv) || !curParentList) {
      NS_WARNING("ERROR: Could not look up a media list by guid!");
      continue;
    }

    // pass the flag if this list is the main Songbird library
    if (mainLibraryGuid.Equals(curMediaListGuid)) {
      rv = mTaskWriter->WriteAddedMediaItemsListHeader(curParentList, PR_TRUE);
    } else {
      rv = mTaskWriter->WriteAddedMediaItemsListHeader(curParentList);
    }
    if (NS_FAILED(rv)) {
      NS_WARNING("ERROR: Could not write the media item list header!");
      continue;
    }

    nsCOMPtr<nsIArray> addedMediaItems;
    rv = EnumerateItemsByGuids<sbStringList>(next->second.begin(),
                                             next->second.end(),
                                             curParentList,
                                             getter_AddRefs(addedMediaItems));
    if (NS_FAILED(rv) || !addedMediaItems) {
      NS_WARNING("ERROR: Could not enumerate media items by guid!");
      continue;
    }

    // Print out all the tracks in the media list.
    rv = WriteMediaItemsArray(addedMediaItems);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "ERROR: Could not write out the media items!");
  }

  return NS_OK;
}

nsresult
sbMediaExportService::WriteUpdatedMediaItems()
{
  TRACE("%s: Writing %i updated media items",
         __FUNCTION__,
         mUpdatedItems.size());
  if (mUpdatedItems.empty()) {
    return NS_OK;
  }

  NS_ENSURE_STATE(mTaskWriter);

  nsresult rv;

  nsCOMPtr<sbILibrary> mainLibrary;
  rv = GetMainLibrary(getter_AddRefs(mainLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> updatedMediaItems;
  rv = EnumerateItemsByGuids<sbStringSet>(mUpdatedItems.begin(),
                                          mUpdatedItems.end(),
                                          mainLibrary,
                                          getter_AddRefs(updatedMediaItems));
  if (NS_FAILED(rv) || !updatedMediaItems) {
    NS_WARNING("ERROR: Could not enumerate media items by guid!");
    return NS_ERROR_FAILURE;;
  }

  rv = mTaskWriter->WriteUpdatedMediaItemsListHeader();
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length = 0;
  rv = updatedMediaItems->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 updatedtemDelta = 0;

  for (PRUint32 i = 0; i < length; i++) {
    nsCOMPtr<sbIMediaItem> curMediaItem =
      do_QueryElementAt(updatedMediaItems, i, &rv);
    if (NS_FAILED(rv) || !curMediaItem) {
      NS_WARNING("ERROR: Could not get a mediaitem from an array!");
      continue;
    }

    rv = mTaskWriter->WriteUpdatedTrack(curMediaItem);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
        "ERROR: Could not write a smartlist mediaitem!");

    ++mProgress;
    ++updatedtemDelta;

    // Go ahead and notify the listener after each write.
    if (updatedtemDelta == LISTENER_NOTIFY_ITEM_DELTA) {
      rv = NotifyListeners();
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
          "ERROR: Could not notify the listeners!");
      updatedtemDelta = 0;
    }
  }

  if (updatedtemDelta > 0) {
    rv = NotifyListeners();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
        "ERROR: Could not notify the listeners!");
  }

  return NS_OK;
}

nsresult
sbMediaExportService::WriteUpdatedSmartPlaylists()
{
  if (mUpdatedSmartMediaLists.size() == 0) {
    return NS_OK;
  }

  LOG("%s: Writing updated smart playlists", __FUNCTION__);

  NS_ENSURE_TRUE(mTaskWriter, NS_ERROR_UNEXPECTED);

  nsresult rv;
  sbStringListIter begin = mUpdatedSmartMediaLists.begin();
  sbStringListIter end = mUpdatedSmartMediaLists.end();
  sbStringListIter next;
  for (next = begin; next != end; ++next) {
    // Get the smart playlist as a medialist to write the update header with
    // the task writer. NOTE: This guid is always going to be a smart list guid.
    nsCOMPtr<sbIMediaList> curSmartMediaList;
    rv = GetMediaListByGuid(*next, getter_AddRefs(curSmartMediaList));
    if (NS_FAILED(rv) || !curSmartMediaList) {
      NS_WARNING("ERROR: Could not get a smart list by GUID!");
      continue;
    }

    nsRefPtr<sbMediaListEnumArrayHelper> enumHelper =
      sbMediaListEnumArrayHelper::New();
    NS_ENSURE_TRUE(enumHelper, NS_ERROR_OUT_OF_MEMORY);

    rv = curSmartMediaList->EnumerateAllItems(
        enumHelper,
        sbIMediaList::ENUMERATIONTYPE_LOCKING);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIArray> mediaItems;
    rv = enumHelper->GetMediaItemsArray(getter_AddRefs(mediaItems));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 length = 0;
    rv = mediaItems->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    if (length == 0) {
      continue;
    }

    rv = mTaskWriter->WriteUpdatedSmartPlaylistHeader(curSmartMediaList);
    if (NS_FAILED(rv)) {
      NS_WARNING("ERROR: Could not write the updated smartlist header!");
      continue;
    }

    // Print out all the tracks in the media list.
    rv = WriteMediaItemsArray(mediaItems);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "ERROR: Could not write out the media items!");
  }

  return NS_OK;
}

nsresult
sbMediaExportService::WriteMediaItemsArray(nsIArray *aItemsArray)
{
  NS_ENSURE_ARG_POINTER(aItemsArray);

  PRUint32 length = 0;
  nsresult rv = aItemsArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 addedItemDelta = 0;

  for (PRUint32 i = 0; i < length; i++) {
    nsCOMPtr<sbIMediaItem> curMediaItem =
      do_QueryElementAt(aItemsArray, i, &rv);
    if (NS_FAILED(rv) || !curMediaItem) {
      NS_WARNING("ERROR: Could not get a mediaitem from an array!");
      continue;
    }

    rv = mTaskWriter->WriteAddedTrack(curMediaItem);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
        "ERROR: Could not write a smartlist mediaitem!");

    ++mProgress;
    ++addedItemDelta;

    // Go ahead and notify the listener after each write.
    if (addedItemDelta == LISTENER_NOTIFY_ITEM_DELTA) {
      rv = NotifyListeners();
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
          "ERROR: Could not notify the listeners!");
      addedItemDelta = 0;
    }
  }

  if (addedItemDelta > 0) {
    rv = NotifyListeners();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
        "ERROR: Could not notify the listeners!");
  }

  return NS_OK;
}

nsresult
sbMediaExportService::GetMediaListByGuid(const nsAString & aItemGuid,
                                         sbIMediaList **aMediaList)
{
  LOG("%s: Getting item by GUID '%s'",
        __FUNCTION__, NS_ConvertUTF16toUTF8(aItemGuid).get());

  nsresult rv;
  nsCOMPtr<sbILibrary> mainLibrary;
  rv = GetMainLibrary(getter_AddRefs(mainLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString mainLibraryGuid;
  rv = mainLibrary->GetGuid(mainLibraryGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> itemAsList;

  // If this is the main library GUID, return that.
  if (mainLibraryGuid.Equals(aItemGuid)) {
    itemAsList = do_QueryInterface(mainLibrary, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    nsCOMPtr<sbIMediaItem> mediaItem;
    rv = mainLibrary->GetItemByGuid(aItemGuid, getter_AddRefs(mediaItem));
    NS_ENSURE_SUCCESS(rv, rv);

    itemAsList = do_QueryInterface(mediaItem, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  itemAsList.swap(*aMediaList);
  return NS_OK;
}

nsresult
sbMediaExportService::NotifyListeners()
{
  LOG("%s: updating listeners of job progress", __FUNCTION__);

  nsresult rv;
  if (!NS_IsMainThread()) {
    // Since all of the listeners expect to be notified on the main thread,
    // proxy back to this method on the main thread.
    nsCOMPtr<nsIThread> mainThread;
    rv = NS_GetMainThread(getter_AddRefs(mainThread));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIRunnable> runnable =
      NS_NEW_RUNNABLE_METHOD(sbMediaExportService, this, ProxyNotifyListeners);
    NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

    return mainThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
  }

  for (PRInt32 i = 0; i < mJobListeners.Count(); i++) {
    rv = mJobListeners[i]->OnJobProgress(this);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
        "Could not notify job progress listener!");
  }

  return NS_OK;
}

PRBool
sbMediaExportService::GetHasRecordedChanges()
{
  return !mAddedItemsMap.empty() ||
         !mUpdatedItems.empty() ||
         !mAddedMediaList.empty() ||
         !mRemovedMediaLists.empty() ||
         !mUpdatedSmartMediaLists.empty();
}

//------------------------------------------------------------------------------
// sbIMediaExportService

NS_IMETHODIMP
sbMediaExportService::GetHasPendingChanges(PRBool *aHasPendingChanges)
{
  NS_ENSURE_ARG_POINTER(aHasPendingChanges);
  *aHasPendingChanges = GetHasRecordedChanges();

  LOG("%s: Media-export service has pending changes == %s",
        __FUNCTION__, (*aHasPendingChanges ? "true" : "false"));

  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::ExportSongbirdData()
{
  LOG("%s: Exporting songbird data (manual trigger)", __FUNCTION__);

  nsresult rv;
  mStatus = sbIJobProgress::STATUS_RUNNING;
  rv = NotifyListeners();
  NS_ENSURE_SUCCESS(rv, rv);

  // Since looking up the stashed data locks the main thread, write out the
  // export data on a background thread.
  nsCOMPtr<nsIThreadPool> threadPoolService =
    do_GetService(SB_THREADPOOLSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRunnable> runnable =
    NS_NEW_RUNNABLE_METHOD(sbMediaExportService, this, WriteExportData);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  return threadPoolService->Dispatch(runnable, NS_DISPATCH_NORMAL);
}

//------------------------------------------------------------------------------
// nsIObserver

NS_IMETHODIMP
sbMediaExportService::Observe(nsISupports *aSubject,
                              const char *aTopic,
                              const PRUnichar *aData)
{
  NS_ENSURE_ARG_POINTER(aTopic);

  LOG("%s: Topic '%s' observed", __FUNCTION__, aTopic);

  nsresult rv;
  if (strcmp(aTopic, SB_LIBRARY_MANAGER_READY_TOPIC) == 0) {
    // No need to listen for the library manager ready notice
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->RemoveObserver(this, SB_LIBRARY_MANAGER_READY_TOPIC);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = InitInternal();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (strcmp(aTopic, SB_LIBRARY_MANAGER_SHUTDOWN_TOPIC) == 0) {
    rv = Shutdown();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIMediaListListener

NS_IMETHODIMP
sbMediaExportService::OnItemAdded(sbIMediaList *aMediaList,
                                  sbIMediaItem *aMediaItem,
                                  PRUint32 aIndex,
                                  PRBool *aRetVal)
{
  LOG("%s: Media Item Added!", __FUNCTION__);

  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv;

  nsString itemGuid;
  rv = aMediaItem->GetGuid(itemGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Handle the mediaitem if it is a playlist (and exporting for lists is on).
  nsCOMPtr<sbIMediaList> itemAsList = do_QueryInterface(aMediaItem, &rv);
  if (itemAsList && NS_SUCCEEDED(rv)) {
    if (mPrefController->GetShouldExportPlaylists() ||
        mPrefController->GetShouldExportSmartPlaylists())
    {
      // Only worry if this is a list that we should be watching
      PRBool shouldWatchList = PR_FALSE;
      rv = GetShouldWatchMediaList(itemAsList, &shouldWatchList);
      if (NS_SUCCEEDED(rv) && shouldWatchList) {
        rv = ListenToMediaList(itemAsList);
        NS_ENSURE_SUCCESS(rv, rv);

        mAddedMediaList.push_back(itemGuid);
      }
    }
  }
  // Handle the added mediaitem track.
  else {
    nsCOMPtr<sbILibrary> mainLibrary;
    rv = GetMainLibrary(getter_AddRefs(mainLibrary));
    NS_ENSURE_SUCCESS(rv, rv);
    if (SameCOMIdentity(mainLibrary, aMediaList)) {
      // this item was added to the main library; check if it already has an
      // itunes persistent id, and ignore it if so (because that means we just
      // imported the track from itunes)
      nsString itunesGuid;
      rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ITUNES_GUID),
                                   itunesGuid);
      if (NS_SUCCEEDED(rv) && !itunesGuid.IsEmpty()) {
        // this item already exists
        return NS_OK;
      }
    }

    nsString listGuid;
    rv = aMediaList->GetGuid(listGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    // Add this mediaitem's guid to the parent medialist's guid string set
    // in the added items map.
    sbMediaListItemMapIter guidIter = mAddedItemsMap.find(listGuid);
    if (guidIter == mAddedItemsMap.end()) {
      // The media list isn't represented in the map, add a new entry for this
      // media list using it's guid.
      std::pair<sbMediaListItemMapIter, bool> insertedPair =
        mAddedItemsMap.insert(sbMediaListItemMapPair(listGuid, sbStringList()));

      NS_ENSURE_TRUE(insertedPair.second, NS_ERROR_FAILURE);
      guidIter = insertedPair.first;
    }

    guidIter->second.push_back(itemGuid);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::OnBeforeItemRemoved(sbIMediaList *aMediaList,
                                          sbIMediaItem *aMediaItem,
                                          PRUint32 aIndex,
                                          PRBool *aRetVal)
{
  *aRetVal = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::OnAfterItemRemoved(sbIMediaList *aMediaList,
                                         sbIMediaItem *aMediaItem,
                                         PRUint32 aIndex, PRBool *_retval)
{
  LOG("%s: After Media Item Removed!!", __FUNCTION__);

  if (mPrefController->GetShouldExportPlaylists() ||
      mPrefController->GetShouldExportSmartPlaylists())
  {
    nsresult rv;
    nsCOMPtr<sbIMediaList> itemAsList = do_QueryInterface(aMediaItem, &rv);
    if (NS_SUCCEEDED(rv) && itemAsList) {
      // If this is a list that is currently being observed by this service,
      // then remove the listener hook and add the list name to the removed
      // media lists string list.

      nsString type;
      rv = itemAsList->GetType(type);
      NS_ENSURE_SUCCESS(rv, rv);

      if (type.EqualsLiteral("smart")) {
        nsCOMPtr<sbILocalDatabaseSmartMediaList> itemAsSmartList =
          do_QueryInterface(itemAsList, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        PRInt32 index = mObservedSmartMediaLists.IndexOf(itemAsSmartList);
        if (index > -1) {
          nsString listName;
          rv = itemAsList->GetName(listName);
          NS_ENSURE_SUCCESS(rv, rv);

          rv = itemAsSmartList->RemoveSmartMediaListListener(this);
          NS_ENSURE_SUCCESS(rv, rv);

          mRemovedMediaLists.push_back(listName);
        }
      }
      else {
        PRInt32 index = mObservedMediaLists.IndexOf(itemAsList);
        if (index > -1) {
          nsString listName;
          rv = itemAsList->GetName(listName);
          NS_ENSURE_SUCCESS(rv, rv);

          mRemovedMediaLists.push_back(listName);

          rv = itemAsList->RemoveListener(this);
          NS_ENSURE_SUCCESS(rv, rv);

          NS_WARN_IF_FALSE(mObservedMediaLists.RemoveObjectAt(index),
              "Could not remove the media list from the observed lists array!");
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::OnItemUpdated(sbIMediaList *aMediaList,
                                    sbIMediaItem *aMediaItem,
                                    sbIPropertyArray *aProperties,
                                    PRBool *aRetVal)
{
  LOG("%s: Media Item Updated!!", __FUNCTION__);

  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aProperties);
  NS_ENSURE_ARG_POINTER(aRetVal);

  // we always want more notifications
  *aRetVal = PR_FALSE;

  nsresult rv;
  #if DEBUG
  {
    nsCOMPtr<sbILibrary> mainLib;
    rv = GetMainLibrary(getter_AddRefs(mainLib));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ASSERTION(SameCOMIdentity(mainLib, aMediaList),
                 "sbMediaExportService::OnItemUpdated: unexpected media list");
  }
  #endif

  nsCOMPtr<sbIMediaList> itemAsList = do_QueryInterface(aMediaItem, &rv);
  if (itemAsList && NS_SUCCEEDED(rv)) {
    // this is a media list; we don't care about those
    return NS_OK;
  }

  nsString contentUrl;
  rv = aProperties->GetPropertyValue(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                                     contentUrl);
  if (NS_FAILED(rv)) {
    // the content url didn't change, something else did
    // we don't need to update the export
    return NS_OK;
  }

  nsString itunesGuid;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ITUNES_GUID),
                               itunesGuid);
  if (NS_FAILED(rv)) {
    // no itunes guid, new item?
    NS_NOTREACHED("not implemented");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsString itemGuid;
  rv = aMediaItem->GetGuid(itemGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add this item's guid to the list of things that were changed
  mUpdatedItems.insert(itemGuid);

  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::OnItemMoved(sbIMediaList *aMediaList,
                                  PRUint32 aFromIndex,
                                  PRUint32 aToIndex,
                                  PRBool *aRetVal)
{
  LOG("%s: Media Item Moved!", __FUNCTION__);
  return NS_OK;
}
NS_IMETHODIMP
sbMediaExportService::OnBeforeListCleared(sbIMediaList *aMediaList,
                                          PRBool aExcludeLists,
                                          PRBool *aRetVal)
{
  LOG("%s: Media List Before Cleared!", __FUNCTION__);
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::OnListCleared(sbIMediaList *aMediaList,
                                    PRBool aExcludeLists,
                                    PRBool *aRetVal)
{
  LOG("%s: Media List Cleared!", __FUNCTION__);
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::OnBatchBegin(sbIMediaList *aMediaList)
{
  LOG("%s: Media List Batch Begin!", __FUNCTION__);
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::OnBatchEnd(sbIMediaList *aMediaList)
{
  LOG("%s: Media List Batch End!", __FUNCTION__);
  return NS_OK;
}

//------------------------------------------------------------------------------
// sbILocalDatabaseSmartMediaListListener

NS_IMETHODIMP
sbMediaExportService::OnRebuild(sbILocalDatabaseSmartMediaList *aSmartMediaList)
{
  NS_ENSURE_ARG_POINTER(aSmartMediaList);

  nsresult rv;
  nsString listGuid;
  rv = aSmartMediaList->GetGuid(listGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG("%s: Observing updated smart media list for '%s'",
        __FUNCTION__, NS_ConvertUTF16toUTF8(listGuid).get());

  sbStringListIter result = std::find(mUpdatedSmartMediaLists.begin(),
                                      mUpdatedSmartMediaLists.end(),
                                      listGuid);
  if (result == mUpdatedSmartMediaLists.end()) {
    mUpdatedSmartMediaLists.push_back(listGuid);
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIJobProgress

NS_IMETHODIMP
sbMediaExportService::GetStatus(PRUint16 *aStatus)
{
  NS_ENSURE_ARG_POINTER(aStatus);
  *aStatus = mStatus;
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::GetBlocked(PRBool *aBlocked)
{
  NS_ENSURE_ARG_POINTER(aBlocked);
  *aBlocked = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::GetStatusText(nsAString & aStatusText)
{
  return SBGetLocalizedString(aStatusText,
      NS_LITERAL_STRING("mediaexport.shutdowntaskname"));
}

NS_IMETHODIMP
sbMediaExportService::GetTitleText(nsAString & aTitleText)
{
  // Not used by the shutdown service
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::GetProgress(PRUint32 *aProgress)
{
  NS_ENSURE_ARG_POINTER(aProgress);
  *aProgress = mProgress;
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::GetTotal(PRUint32 *aTotal)
{
  NS_ENSURE_ARG_POINTER(aTotal);
  *aTotal = mTotal;
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::GetErrorCount(PRUint32 *aErrorCount)
{
  NS_ENSURE_ARG_POINTER(aErrorCount);
  // Not used by the shutdown service
  *aErrorCount = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::GetErrorMessages(nsIStringEnumerator **aRetVal)
{
  NS_ENSURE_ARG_POINTER(aRetVal);
  // Not used by the shutdown service
  *aRetVal = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::AddJobProgressListener(sbIJobProgressListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  NS_ENSURE_TRUE(mJobListeners.AppendObject(aListener), NS_ERROR_FAILURE);
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::RemoveJobProgressListener(sbIJobProgressListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  NS_WARN_IF_FALSE(mJobListeners.RemoveObject(aListener),
      "Could not remove the job progress listener!");
  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIShutdownTask

NS_IMETHODIMP
sbMediaExportService::GetNeedsToRunTask(PRBool *aNeedsToRunTask)
{
  NS_ENSURE_ARG_POINTER(aNeedsToRunTask);

  nsresult rv;
  *aNeedsToRunTask = PR_FALSE;

  rv = GetHasPendingChanges(aNeedsToRunTask);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!*aNeedsToRunTask && mPrefController->GetShouldExportAnyMedia()) {
    // Since there isn't any pending changes, check to see if the agent should
    // be started. Only start the agent if it is not running and there is at
    // least one pending export task file on disk.
    nsCOMPtr<nsIFile> taskFileParentFolder;
    rv = NS_GetSpecialDirectory(NS_APP_APPLICATION_REGISTRY_DIR,
                                getter_AddRefs(taskFileParentFolder));
    NS_ENSURE_SUCCESS(rv, rv);

    // Loop through this folders children to see if we can find a task file
    // who's name matches the export task filename.
    nsCOMPtr<nsISimpleEnumerator> dirEnum;
    rv = taskFileParentFolder->GetDirectoryEntries(getter_AddRefs(dirEnum));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasTaskFile = PR_FALSE;
    PRBool hasMore = PR_FALSE;
    while ((NS_SUCCEEDED(dirEnum->HasMoreElements(&hasMore))) && hasMore) {
      nsCOMPtr<nsISupports> curItem;
      rv = dirEnum->GetNext(getter_AddRefs(curItem));
      if (NS_FAILED(rv)) {
        continue;
      }

      nsCOMPtr<nsIFile> curFile = do_QueryInterface(curItem, &rv);
      if (NS_FAILED(rv) || !curFile) {
        continue;
      }

      nsString curFileName;
      rv = curFile->GetLeafName(curFileName);
      if (NS_FAILED(rv)) {
        continue;
      }

      if (curFileName.Equals(NS_LITERAL_STRING(TASKFILE_NAME))) {
        hasTaskFile = PR_TRUE;
        // Don't bother iterating through the rest of the children.
        break;
      }
    }

    if (hasTaskFile) {
      nsCOMPtr<sbIMediaExportAgentService> agentService =
        do_GetService(SB_MEDIAEXPORTAGENTSERVICE_CONTRACTID, &rv);
      if (NS_SUCCEEDED(rv) && agentService) {
        PRBool isAgentRunning = PR_FALSE;
        rv = agentService->GetIsAgentRunning(&isAgentRunning);
        if (NS_SUCCEEDED(rv) && !isAgentRunning) {
          *aNeedsToRunTask = PR_TRUE;
        }
      }
    }
  }

  if (*aNeedsToRunTask) {
    // Since we are about to run a job, calculate the total item count now.
    mProgress = 0;
    mTotal = mAddedMediaList.size();
    mTotal += mRemovedMediaLists.size();

    sbStringListIter listBegin = mUpdatedSmartMediaLists.begin();
    sbStringListIter listEnd = mUpdatedSmartMediaLists.end();
    sbStringListIter listNext;
    for (listNext = listBegin; listNext != listEnd; ++listNext) {
      nsCOMPtr<sbIMediaList> curUpdatedList;
      rv = GetMediaListByGuid(*listNext, getter_AddRefs(curUpdatedList));
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 length;
      rv = curUpdatedList->GetLength(&length);
      NS_ENSURE_SUCCESS(rv, rv);

      mTotal += length;
    }

    sbMediaListItemMapIter begin = mAddedItemsMap.begin();
    sbMediaListItemMapIter end = mAddedItemsMap.end();
    sbMediaListItemMapIter next;
    for (next = begin; next != end; ++next) {
      mTotal += next->second.size();
    }
  }

  LOG("%s: Export service needs to run at shutdown == %s",
        __FUNCTION__, (*aNeedsToRunTask ? "true" : "false"));

  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::StartTask()
{
  LOG("%s: Starting export service shutdown task...", __FUNCTION__);

  // This method gets called by the shutdown service when it is our turn to
  // begin processing. Simply start the export data process here.
  return ExportSongbirdData();
}

//------------------------------------------------------------------------------
// XPCOM Startup Registration

/* static */ NS_METHOD
sbMediaExportService::RegisterSelf(nsIComponentManager *aCompMgr,
                                   nsIFile *aPath,
                                   const char *aLoaderStr,
                                   const char *aType,
                                   const nsModuleComponentInfo *aInfo)
{
  NS_ENSURE_ARG_POINTER(aCompMgr);
  NS_ENSURE_ARG_POINTER(aPath);
  NS_ENSURE_ARG_POINTER(aLoaderStr);
  NS_ENSURE_ARG_POINTER(aType);
  NS_ENSURE_ARG_POINTER(aInfo);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsICategoryManager> catMgr =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catMgr->AddCategoryEntry("app-startup",
                                SB_MEDIAEXPORTSERVICE_CLASSNAME,
                                "service,"
                                SB_MEDIAEXPORTSERVICE_CONTRACTID,
                                PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

