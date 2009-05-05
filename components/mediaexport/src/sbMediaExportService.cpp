/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbMediaExportService.h"

#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <prlog.h>
#include <sbILibraryManager.h>
#include <nsIObserverService.h>
#include <sbILibrary.h>
#include <sbLibraryUtils.h>
#include <sbPropertiesCID.h>
#include <sbStandardProperties.h>
#include <nsIClassInfoImpl.h>
#include <nsMemory.h>
#include <nsIProgrammingLanguage.h>


#ifdef PR_LOGGING
PRLogModuleInfo *gMediaExportLog = nsnull;
#endif

NS_IMPL_ISUPPORTS6(sbMediaExportService,
                   nsIClassInfo,
                   nsIObserver,
                   sbIMediaListListener,
                   sbIMediaListEnumerationListener,
                   sbIJobProgress,
                   sbIShutdownJob)

NS_IMPL_CI_INTERFACE_GETTER6(sbMediaExportService,
                             nsIClassInfo,
                             nsIObserver,
                             sbIMediaListListener,
                             sbIMediaListEnumerationListener,
                             sbIJobProgress,
                             sbIShutdownJob)

NS_DECL_CLASSINFO(sbMediaExportService)
NS_IMPL_THREADSAFE_CI(sbMediaExportService)

//------------------------------------------------------------------------------

sbMediaExportService::sbMediaExportService()
  : mIsRunning(PR_FALSE)
  , mEnumState(eNone)
  , mFinishedExportState(PR_FALSE)
{
#ifdef PR_LOGGING
   if (!gMediaExportLog) {
     gMediaExportLog = PR_NewLogModule("sbMediaExportService");
   }
#endif
}

sbMediaExportService::~sbMediaExportService()
{
}

nsresult
sbMediaExportService::Init()
{
  LOG(("%s: Starting the export service", __FUNCTION__));

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
  LOG(("%s: Internal initializing the export service", __FUNCTION__));

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

  // In order to listen to all of the current playlists, a non-blocking
  // enumerate lookup is needed.
  if (mPrefController->GetShouldExportPlaylists() ||
      mPrefController->GetShouldExportSmartPlaylists())
  {
    rv = mainLibrary->EnumerateItemsByProperty(
        NS_LITERAL_STRING(SB_PROPERTY_ISLIST),
        NS_LITERAL_STRING("1"),
        this,
        sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbMediaExportService::Shutdown()
{
  LOG(("%s: Shutting down export service", __FUNCTION__));

  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->RemoveObserver(this, SB_LIBRARY_MANAGER_SHUTDOWN_TOPIC);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPrefController->Shutdown();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = StopListeningMediaLists();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediaExportService::StopListeningMediaLists()
{
  LOG(("%s: Removing listeners from all media lists", __FUNCTION__));
  
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
      LOG(("%s: Removing listener for media list '%s",
            __FUNCTION__, NS_ConvertUTF16toUTF8(listGuid).get()));
#endif 
    }

    mObservedMediaLists.Clear();

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
  LOG(("%s: '%s' pref changed : %s",
        __FUNCTION__,
        NS_ConvertUTF16toUTF8(aPrefName).get(),
        (aNewPrefValue ? "true" : "false")));

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
  LOG(("%s: Adding listener for media list '%s",
        __FUNCTION__, NS_ConvertUTF16toUTF8(listGuid).get()));
#endif

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
  LOG(("%s: Deciding if medialist '%s' should be watched.",
        __FUNCTION__, NS_ConvertUTF16toUTF8(mediaListName).get()));
#endif

  nsString propValue;
  
  // Don't watch the Downloads folder
  rv = aMediaList->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CUSTOMTYPE),
                               propValue);
  if (NS_FAILED(rv) || propValue.EqualsLiteral("download")) {
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

nsresult
sbMediaExportService::BeginExportData()
{
  LOG(("%s: Starting to export data", __FUNCTION__));
  nsresult rv;

  mTaskWriter = new sbMediaExportTaskWriter();
  NS_ENSURE_TRUE(mTaskWriter, NS_ERROR_OUT_OF_MEMORY);

  rv = mTaskWriter->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mFinishedExportState = PR_FALSE;
  mExportState = eNone;

  // Reset the mediaitems map iter
  mCurExportListIter = mAddedItemsMap.end();

  // Start the export:
  rv = DetermineNextExportState();
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult
sbMediaExportService::FinishExportData()
{
  LOG(("%s: Done exporting data", __FUNCTION__));
  nsresult rv;
  rv = mTaskWriter->Finish();
  NS_ENSURE_SUCCESS(rv, rv);

  // Clean up the recording structures.
  mAddedItemsMap.clear();
  mAddedMediaList.clear();
  mRemovedMediaLists.clear();

  mStatus = sbIJobProgress::STATUS_SUCCEEDED;
  rv = NotifyListeners();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediaExportService::DetermineNextExportState()
{
  LOG(("%s: Determining next state (cur state: %i)", 
        __FUNCTION__, mExportState));
  
  nsresult rv;

  // The order of exported data looks like this
  // 1.) Added media lists
  // 2.) Removed media lists
  // 3.) Medialist added items

  PRBool shouldFinish = PR_FALSE;
  
  // If the current export task has finished, increment to the next state.
  if (mFinishedExportState) {
    switch (mExportState) {
      case eAddedMediaLists:
        mExportState = eRemovedMediaLists;
        break;

      case eRemovedMediaLists:
        mExportState = eAddedMediaItems;
        break;
      
      case eAddedMediaItems:
        shouldFinish = PR_TRUE;
        break;

      default:
        // Make compiler happy
        NS_WARNING("ERROR: export state not handled in switch statement!");
        break;
    }
  }
  
  // Loop until we find the next task that needs to be processed.
  PRBool foundTask = PR_FALSE;
  while (!foundTask && !shouldFinish) {
    switch (mExportState) {
      case eNone:
        mExportState = eAddedMediaLists;
        break;

      case eAddedMediaLists:
        // First check to see if any playlists are being exported
        if (mPrefController->GetShouldExportAnyPlaylists()) {
          // Ensure that there are added medialists to process
          if (mAddedMediaList.size() > 0) {
            foundTask = PR_TRUE;
          }
          else {
            mExportState = eRemovedMediaLists;
          }
        }
        // Playlists are currently not being exported, jump down to
        // look for added mediaitems.
        else {
          mExportState = eAddedMediaItems;
        }
        break;

      case eRemovedMediaLists:
        // First double check to ensure that playlists are being exported
        if (mPrefController->GetShouldExportAnyPlaylists() &&
            mRemovedMediaLists.size() > 0) 
        {
          foundTask = PR_TRUE;
        }
        // Playlists are currently not being exported - or there has not
        // been any removed medialists. Move on to added mediaitems.
        else {
          mExportState = eAddedMediaItems;
        }
        break;

      case eAddedMediaItems:
        if (mPrefController->GetShouldExportTracks() && 
            mAddedItemsMap.size() > 0) 
        {
          foundTask = PR_TRUE;
        }
        else {
          shouldFinish = PR_TRUE;
        }
        break;

      default:
        // Safety-net, just break and finish the export state.
        shouldFinish = PR_TRUE;
        NS_WARNING("ERROR: export state not handled in switch statement!");
        break;
    }
  }

  if (shouldFinish) {
    rv = FinishExportData();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = StartExportState();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  return NS_OK;
}

nsresult
sbMediaExportService::StartExportState()
{
  LOG(("%s: Starting export state: %i", __FUNCTION__, mExportState));
  nsresult rv;

  mFinishedExportState = PR_FALSE;

  switch (mExportState) {
    case eAddedMediaLists:
    {
      rv = mTaskWriter->WriteAddedMediaListsHeader();
      NS_ENSURE_SUCCESS(rv, rv);

      sbStringListIter begin = mAddedMediaList.begin();
      sbStringListIter end = mAddedMediaList.end();
      sbStringListIter next;
      for (next = begin; next != end; ++next) {
        // Get the current media list by guid.
        nsCOMPtr<sbIMediaList> curMediaList;
        rv = GetMediaListByGuid(*next, getter_AddRefs(curMediaList));
        if (NS_FAILED(rv) || !curMediaList) {
          NS_WARNING("ERROR: Could not get a media list by GUID!");
          continue;
        }

        rv = mTaskWriter->WriteMediaListName(curMediaList);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      // Keep things clean by calling the finish state method.
      rv = FinishExportState();
      NS_ENSURE_SUCCESS(rv, rv);

      break;
    }

    case eRemovedMediaLists:
    {
      // Removed playlists are fun because their name doesn't need to be
      // looked up.
      rv = mTaskWriter->WriteRemovedMediaListsHeader();
      NS_ENSURE_SUCCESS(rv, rv);

      sbStringListIter end = mRemovedMediaLists.end();
      sbStringListIter next;
      for (next = mRemovedMediaLists.begin(); next != end; ++next) {
        rv = mTaskWriter->WriteString(*next);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      // Keep things clean by calling the finish state method
      rv = FinishExportState();
      NS_ENSURE_SUCCESS(rv, rv);

      break;
    }

    case eAddedMediaItems:
    {
      if (mCurExportListIter == mAddedItemsMap.end()) {
        // If the |mCurExportListIter| iterator is currently null, then
        // this is the first time
        mCurExportListIter = mAddedItemsMap.begin();
        LOG(("%s: There are %i media lists with added media items.",
              __FUNCTION__, mAddedItemsMap.size()));
      }
      
      // Lookup the name of the current media item.
      nsCOMPtr<sbILibrary> mainLibrary;
      rv = GetMainLibrary(getter_AddRefs(mainLibrary));
      NS_ENSURE_SUCCESS(rv, rv);

      // If this is the main library, we don't have to lookup the mediaitem
      nsString mainLibraryGuid;
      rv = mainLibrary->GetGuid(mainLibraryGuid);
      NS_ENSURE_SUCCESS(rv, rv);

      nsString curExportListGuid(mCurExportListIter->first);

      LOG(("%s: Getting ready to lookup medialist guid '%s'",
            __FUNCTION__, NS_ConvertUTF16toUTF8(curExportListGuid).get()));

      if (mainLibraryGuid.Equals(curExportListGuid)) {
        LOG(("%s: This GUID is the main library!", __FUNCTION__));
        mCurExportMediaList = mainLibrary;
        mExportState = eMediaListAddedItems;
        // Fall through to process below -
      }
      else {
        LOG(("%s: This GUID is not the main library!", __FUNCTION__));
        // This is not the main library, lookup the list name.
        NS_NAMED_LITERAL_STRING(guidProperty, SB_PROPERTY_GUID);
        rv = mainLibrary->EnumerateItemsByProperty(
            guidProperty,
            curExportListGuid,
            this,
            sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      }
    }

    case eMediaListAddedItems:
    {
      // In the previous step (eAddedMediaItems), the process looked up the
      // medialist to process next and stores it in |mCurExportMediaList|.
      // Use this list to lookup all the recorded media items by GUID.

      // First, write out the name of the medialist with the task write.
      rv = mTaskWriter->WriteAddedMediaItemsListHeader(mCurExportMediaList);
      NS_ENSURE_SUCCESS(rv, rv);

      // Now enumerate mediaitems by the recorded guid.
      rv = EnumerateItemsByGuids(mCurExportListIter->second, 
                                 mCurExportMediaList);
      NS_ENSURE_SUCCESS(rv, rv);

      break;
    }

    default:
      // Make compiler happy
      NS_WARNING("ERROR: export state not handled in switch statement!");
      break;
  }

  return NS_OK;
}

nsresult
sbMediaExportService::FinishExportState()
{
  LOG(("%s: Finishing export state: %i", __FUNCTION__, mExportState));
  
  nsresult rv;

  mFinishedExportState = PR_TRUE;

  switch (mExportState) {
    case eAddedMediaLists:
    case eRemovedMediaLists:
      // These two states handle all the export data in the start task method.
      // Simply setup the move to the next export state.
      rv = DetermineNextExportState();
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    case eAddedMediaItems:
    {
      // Now that the medialist has been looked up, begin processing all
      // of the recorded media items for it.
      mExportState = eMediaListAddedItems;
      mFinishedExportState = PR_FALSE;
      
      rv = StartExportState();
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
  
    case eMediaListAddedItems:
    {
#ifdef PR_LOGGING
      nsString mediaListGuid;
      rv = mCurExportMediaList->GetGuid(mediaListGuid);
      NS_ENSURE_SUCCESS(rv, rv);
      LOG(("%s: Done exporting items for medialist '%s'",
            __FUNCTION__, NS_ConvertUTF16toUTF8(mediaListGuid).get()));
#endif
      
      ++mCurExportListIter;
      mCurExportMediaList = nsnull;

      if (mCurExportListIter == mAddedItemsMap.end()) {
        LOG(("%s: Done exporting all the added mediaitems!", __FUNCTION__));
        // All of the items have been processed - and this is the last state
        // of the export, finish up the service.
        rv = DetermineNextExportState();
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      }

      // Switch back to the parent mediaitems enum
      mExportState = eAddedMediaItems;
      mFinishedExportState = PR_FALSE;

      rv = StartExportState();
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }

    default:
      // Make compiler happy
      NS_WARNING("ERROR: export state not handled in switch statement!");
      break;
  }

  return NS_OK;
}

nsresult
sbMediaExportService::GetMediaListByGuid(const nsAString & aItemGuid,
                                         sbIMediaList **aMediaList)
{
  LOG(("%s: Getting item by GUID '%s'",
        __FUNCTION__, NS_ConvertUTF16toUTF8(aItemGuid).get()));

  nsresult rv;
  nsCOMPtr<sbILibrary> mainLibrary;
  rv = GetMainLibrary(getter_AddRefs(mainLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = mainLibrary->GetItemByGuid(aItemGuid, getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> itemAsList = do_QueryInterface(mediaItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  itemAsList.swap(*aMediaList);
  return NS_OK;
}

nsresult
sbMediaExportService::EnumerateItemsByGuids(sbStringList & aGuidStringList,
                                            sbIMediaList *aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsresult rv;
  
#ifdef PR_LOGGING
  nsString mediaListName;
  rv = aMediaList->GetName(mediaListName);
  NS_ENSURE_SUCCESS(rv, rv);
  LOG(("%s: Enumerate items by guids on media list '%s'",
        __FUNCTION__, NS_ConvertUTF16toUTF8(mediaListName).get()));
#endif
  
  nsCOMPtr<sbIMutablePropertyArray> properties =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING(guidProperty, SB_PROPERTY_GUID);

  sbStringListIter begin = aGuidStringList.begin();
  sbStringListIter end = aGuidStringList.end();
  sbStringListIter next;
  
  for (next = begin; next != end; ++next) {
    rv = properties->AppendProperty(guidProperty, *next);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRUint16 enumType = sbIMediaList::ENUMERATIONTYPE_SNAPSHOT;
  rv = aMediaList->EnumerateItemsByProperties(properties, this, enumType);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediaExportService::NotifyListeners()
{
  LOG(("%s: updating listeners of job progress", __FUNCTION__));

  nsresult rv;
  for (PRInt32 i = 0; i < mJobListeners.Count(); i++) {
    rv = mJobListeners[i]->OnJobProgress(this);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), 
        "Could not notify job progress listener!");
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
// nsIObserver

NS_IMETHODIMP
sbMediaExportService::Observe(nsISupports *aSubject,
                              const char *aTopic,
                              const PRUnichar *aData)
{
  NS_ENSURE_ARG_POINTER(aTopic);
  
  LOG(("%s: Topic '%s' observed", __FUNCTION__, aTopic));
  
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
  LOG(("%s: Media Item Added!", __FUNCTION__));
  
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
  return NS_OK;
}

NS_IMETHODIMP 
sbMediaExportService::OnAfterItemRemoved(sbIMediaList *aMediaList, 
                                         sbIMediaItem *aMediaItem, 
                                         PRUint32 aIndex, PRBool *_retval)
{
  LOG(("%s: After Media Item Removed!!", __FUNCTION__));

  if (mPrefController->GetShouldExportPlaylists() ||
      mPrefController->GetShouldExportSmartPlaylists())
  {
    nsresult rv;
    nsCOMPtr<sbIMediaList> itemAsList = do_QueryInterface(aMediaItem, &rv);
    if (NS_SUCCEEDED(rv) && itemAsList) {
      // If this is a list that is currently being observed by this service,
      // then remove the listener hook and add the list name to the removed
      // media lists string list.
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

  return NS_OK;
}

NS_IMETHODIMP 
sbMediaExportService::OnItemUpdated(sbIMediaList *aMediaList, 
                                    sbIMediaItem *aMediaItem, 
                                    sbIPropertyArray *aProperties, 
                                    PRBool *aRetVal)
{
  LOG(("%s: Media Item Updated!!", __FUNCTION__));
  return NS_OK;
}

NS_IMETHODIMP 
sbMediaExportService::OnItemMoved(sbIMediaList *aMediaList, 
                                  PRUint32 aFromIndex, 
                                  PRUint32 aToIndex, 
                                  PRBool *aRetVal)
{
  LOG(("%s: Media Item Moved!", __FUNCTION__));
  return NS_OK;
}

NS_IMETHODIMP 
sbMediaExportService::OnListCleared(sbIMediaList *aMediaList, 
                                    PRBool *aRetVal)
{
  LOG(("%s: Media List Cleared!", __FUNCTION__));
  return NS_OK;
}

NS_IMETHODIMP 
sbMediaExportService::OnBatchBegin(sbIMediaList *aMediaList)
{
  LOG(("%s: Media List Batch Begin!", __FUNCTION__));
  return NS_OK;
}

NS_IMETHODIMP 
sbMediaExportService::OnBatchEnd(sbIMediaList *aMediaList)
{
  LOG(("%s: Media List Batch End!", __FUNCTION__));
  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIMediaListEnumerationListener

NS_IMETHODIMP
sbMediaExportService::OnEnumerationBegin(sbIMediaList *aMediaList,
                                         PRUint16 *aRetVal)
{
  LOG(("%s: mEnumState == %i", __FUNCTION__, mEnumState));
  
  NS_ENSURE_ARG_POINTER(aRetVal);
  *aRetVal = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::OnEnumeratedItem(sbIMediaList *aMediaList,
                                       sbIMediaItem *aMediaItem,
                                       PRUint16 *aRetVal)
{
  LOG(("%s: mEnumState == %i", __FUNCTION__, mEnumState));
  
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aRetVal);
  
  *aRetVal = sbIMediaListEnumerationListener::CONTINUE;
  nsresult rv;
  
  // If |mExportState| isn't set, than the service is looking up all of the
  // medialists to figure out if they need to be watched.
  if (mExportState == eNone && mEnumState == eAllMediaLists) {
    nsCOMPtr<sbIMediaList> itemAsList = do_QueryInterface(aMediaItem, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool shouldWatch = PR_FALSE;
    rv = GetShouldWatchMediaList(itemAsList, &shouldWatch);

    if (shouldWatch) {
      rv = ListenToMediaList(itemAsList);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else {
    // The service is currently looking up export data, push data to the task
    // writer as appropriate.
    switch (mExportState) {
      case eAddedMediaItems:
      {
        mCurExportMediaList = do_QueryInterface(aMediaItem, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      }

      case eMediaListAddedItems:
      {
        // The task writer is alreay set in exporting media items mode,
        // simply pass the found media item to the writer.
        rv = mTaskWriter->WriteAddedTrack(aMediaItem);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      }

      default:
        // Make compiler happy
        NS_WARNING("ERROR: export state not handled in switch statement!");
        break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::OnEnumerationEnd(sbIMediaList *aMediaList,
                                       nsresult aStatusCode)
{
  LOG(("%s: mEnumState == %i", __FUNCTION__, mEnumState));
  nsresult rv;
  
  // If the export state is currently running, it was most likely waiting for
  // a media resource lookup. Finish the current export state.
  if (mExportState != eNone) {
    rv = FinishExportState();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mEnumState = eNone;
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
sbMediaExportService::GetStatusText(nsAString & aStatusText)
{
  //
  // TODO: Use me!
  //
  return NS_OK;
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
  // Not used by the shutdown service
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::GetTotal(PRUint32 *aTotal)
{
  // Not used by the shutdown service
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
  
  // Only export data if there was any recorded activity.
  if (mAddedItemsMap.size() > 0 || 
      mAddedMediaList.size() > 0 ||
      mRemovedMediaLists.size() > 0)
  {
    *aNeedsToRunTask = PR_TRUE;
  }
  else {
    *aNeedsToRunTask = PR_FALSE;
  }

  LOG(("%s: Export service needs to run at shutdown == %s",
        __FUNCTION__, (*aNeedsToRunTask ? "true" : "false")));

  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::StartTask()
{
  LOG(("%s: Starting export service shutdown task...", __FUNCTION__));
  
  // This method gets called by the shutdown service when it is our turn to
  // begin processing. Simply start the export data process here.
  mStatus = sbIJobProgress::STATUS_RUNNING;
  nsresult rv = NotifyListeners();
  NS_ENSURE_SUCCESS(rv, rv);

  return BeginExportData();
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
                                SONGBIRD_MEDIAEXPORTSERVICE_CLASSNAME,
                                "service,"
                                SONGBIRD_MEDIAEXPORTSERVICE_CONTRACTID,
                                PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

