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

#include "sbMediaExportDefines.h"
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <prlog.h>
#include <sbILibraryManager.h>
#include <nsIObserverService.h>
#include <sbILibrary.h>
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

  rv = mPrefController->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediaExportService::InitInternal()
{
  nsresult rv;
  mObservedMediaLists = do_CreateInstance("@mozilla.org/array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // No need to listen for the library manager ready notice
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->RemoveObserver(this, SB_LIBRARY_MANAGER_READY_TOPIC);
  NS_ENSURE_SUCCESS(rv, rv);

  // Don't bother starting any listeners when the service should not run.
  if (!mPrefController->GetShouldExportTracks() &&
      !mPrefController->GetShouldExportPlaylists() && 
      !mPrefController->GetShouldExportSmartPlaylists())
  {
    return NS_OK;
  }

  mIsRunning = PR_TRUE;
  
  // At least one item needs to be exported, setup the library listener.
  nsCOMPtr<sbILibraryManager> libraryMgr = 
    do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> mainLibrary;
  rv = libraryMgr->GetMainLibrary(getter_AddRefs(mainLibrary));
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

  if (mIsRunning) {
    // Removing listener references from all the observed media lists.
    PRUint32 observedListsLength = 0;
    rv = mObservedMediaLists->GetLength(&observedListsLength);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 i = 0; i < observedListsLength; i++) {
      nsCOMPtr<sbIMediaList> curMediaList;
      rv = mObservedMediaLists->QueryElementAt(i, 
                                               NS_GET_IID(sbIMediaList),
                                               getter_AddRefs(curMediaList));
      if (NS_FAILED(rv) || !curMediaList) {
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

    rv = mObservedMediaLists->Clear();
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

  rv = mObservedMediaLists->AppendElement(aMediaList, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

nsresult
sbMediaExportService::GetShouldWatchMediaList(sbIMediaList *aMediaList, 
                                              PRBool *aShouldWatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aShouldWatch);

  *aShouldWatch = PR_FALSE;

  nsresult rv;
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

//------------------------------------------------------------------------------
// nsIObserver

NS_IMETHODIMP
sbMediaExportService::Observe(nsISupports *aSubject,
                              const char *aTopic,
                              const PRUnichar *aData)
{
  nsresult rv;
  if (strcmp(aTopic, SB_LIBRARY_MANAGER_READY_TOPIC) == 0) {
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
  if ((mPrefController->GetShouldExportPlaylists() ||
       mPrefController->GetShouldExportSmartPlaylists()) &&
      NS_SUCCEEDED(rv) && itemAsList) 
  {
    mAddedMediaList.push_back(itemGuid);

    // Don't block this call, wait until the batch add ends to see if this
    // media list needs to be listened to.
    NS_WARN_IF_FALSE(mPendingObservedMediaLists.AppendObject(itemAsList),
        "Could not add media list to pending observe media lists array!");
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
    nsCOMPtr<sbIMediaList> removedList = do_QueryInterface(aMediaItem, &rv);
    if (NS_SUCCEEDED(rv) && removedList) {
      nsString listName;

      rv = removedList->GetName(listName);
      NS_ENSURE_SUCCESS(rv, rv);
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

  // If there are any pending batched media lists that were added, start 
  // listening to them if we should.
  if (mPendingObservedMediaLists.Count() > 0) {
    for (PRInt32 i = 0; i < mPendingObservedMediaLists.Count(); i++) {
      nsCOMPtr<sbIMediaList> curPendingMediaList = 
        mPendingObservedMediaLists[i];

      PRBool shouldWatchList = PR_FALSE;
      nsresult rv = GetShouldWatchMediaList(curPendingMediaList, 
                                            &shouldWatchList);
      if (NS_SUCCEEDED(rv) && shouldWatchList) {
        rv = ListenToMediaList(curPendingMediaList);
        NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Coult not listen to media list!");
      }
    }

    mPendingObservedMediaLists.Clear();
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIMediaListEnumerationListener

NS_IMETHODIMP
sbMediaExportService::OnEnumerationBegin(sbIMediaList *aMediaList,
                                         PRUint16 *aRetVal)
{
  NS_ENSURE_ARG_POINTER(aRetVal);
  *aRetVal = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::OnEnumeratedItem(sbIMediaList *aMediaList,
                                       sbIMediaItem *aMediaItem,
                                       PRUint16 *aRetVal)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aRetVal);
  
  *aRetVal = sbIMediaListEnumerationListener::CONTINUE;
  
  nsresult rv;
  nsCOMPtr<sbIMediaList> itemAsList = do_QueryInterface(aMediaItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool shouldWatch = PR_FALSE;
  rv = GetShouldWatchMediaList(itemAsList, &shouldWatch);
  NS_ENSURE_SUCCESS(rv, rv);

  if (shouldWatch) {
    rv = ListenToMediaList(itemAsList);
    NS_ENSURE_SUCCESS(rv, rv); 
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::OnEnumerationEnd(sbIMediaList *aMediaList,
                                       nsresult aStatusCode)
{
  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIJobProgress

NS_IMETHODIMP
sbMediaExportService::GetStatus(PRUint16 *aStatus)
{
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::GetStatusText(nsAString & aStatusText)
{
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::GetTitleText(nsAString & aTitleText)
{
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::GetProgress(PRUint32 *aProgress)
{
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::GetTotal(PRUint32 *aTotal)
{
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::GetErrorCount(PRUint32 *aErrorCount)
{
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::GetErrorMessages(nsIStringEnumerator **aRetVal)
{
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::AddJobProgressListener(sbIJobProgressListener *aListener)
{
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::RemoveJobProgressListener(sbIJobProgressListener *aListener)
{
  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIShutdownTask

NS_IMETHODIMP
sbMediaExportService::GetNeedsToRunTask(PRBool *aNeedsToRunTask)
{
  NS_ENSURE_ARG_POINTER(aNeedsToRunTask);
  *aNeedsToRunTask = PR_FALSE;

  // Dump out all the added media items (for testing)
  sbMediaListItemMapIter begin = mAddedItemsMap.begin();
  sbMediaListItemMapIter end = mAddedItemsMap.end();
  sbMediaListItemMapIter next;
  
  return NS_OK;
}

NS_IMETHODIMP
sbMediaExportService::StartTask()
{
  //
  // write me 
  //

  return NS_OK;
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

