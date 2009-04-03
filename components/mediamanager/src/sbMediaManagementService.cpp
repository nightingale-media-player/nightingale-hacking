/* vim: set sw=2 :*/
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

#include "sbMediaManagementService.h"

/**
 * mozilla interfaces
 */
#include <nsIObserverService.h>
#include <nsIPrefBranch2.h>
#include <nsIPrefService.h>
#include <nsITimer.h>

/**
 * songbird interfaces
 */
#include <sbILibrary.h>
#include <sbILibraryManager.h>
#include <sbIMediaFileManager.h>

/**
 * other mozilla headers
 */
#include <nsISupportsUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>

/**
 * other songbird headers
 */
#include <sbLibraryUtils.h>

/**
 * constants
 */

#define MMS_STARTUP_DELAY (1 * 10 * 1000) /* milliseconds */
#define MMS_SCAN_DELAY    (5 * 1000) /* milliseconds */

/**
 * logging
 * 
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediaManagementService:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gMediaManagementServiceLog = nsnull;
#define TRACE(args) PR_LOG(gMediaManagementServiceLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMediaManagementServiceLog, PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

/**
 * local type declarations
 */
struct ProcessItemData {
  /**
   * a structure used to pass data to the ProcessItem enumeration callback.
   * the items here are shared across the items being enumerated.
   *
   * these pointers are not owning; they are temporary only
   */
  sbMediaManagementService* mediaMgmtService;
  sbIMediaFileManager* fileMan;
};

NS_IMPL_ISUPPORTS4(sbMediaManagementService,
                   sbIMediaManagementService,
                   sbIMediaListListener,
                   nsITimerCallback,
                   nsIObserver)

sbMediaManagementService::sbMediaManagementService()
  : mEnabled(PR_FALSE)
{
#ifdef PR_LOGGING
  if (!gMediaManagementServiceLog) {
    gMediaManagementServiceLog = PR_NewLogModule("sbMediaManagementService");
  }
#endif
}

sbMediaManagementService::~sbMediaManagementService()
{
  /* destructor code */
}

/*****
 * sbIMediaManagementService
 *****/

/* readonly attribute sbILibrary managedLibrary; */
NS_IMETHODIMP
sbMediaManagementService::GetManagedLibrary(sbILibrary * *aManagedLibrary)
{
  NS_ENSURE_ARG_POINTER(aManagedLibrary);
  NS_ENSURE_TRUE(mLibrary, NS_ERROR_ALREADY_INITIALIZED);

  return CallQueryInterface(mLibrary, aManagedLibrary);
}

/* attribute boolean isEnabled; */
NS_IMETHODIMP
sbMediaManagementService::GetIsEnabled(PRBool *aIsEnabled)
{
  NS_ENSURE_ARG_POINTER(aIsEnabled);
  *aIsEnabled = mEnabled;
  return NS_OK;
}
NS_IMETHODIMP
sbMediaManagementService::SetIsEnabled(PRBool aIsEnabled)
{
  // XXX Mook: needs to start doing things
  nsresult rv;
  if (aIsEnabled == mEnabled) {
    // no transition, don't do anything
    return NS_OK;
  }

  if (aIsEnabled) {
    // disabled -> enabled transition
    if (!mDelayedStartupTimer) {
      mDelayedStartupTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    
    TRACE(("%s: arming delayed startup timer due to manual enable", __FUNCTION__));
    rv = mDelayedStartupTimer->InitWithCallback(this,
                                                MMS_SCAN_DELAY,
                                                nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);

    mEnabled = aIsEnabled;

    return NS_OK;

  } else {
    // enabled -> disabled transition
    rv = StopListening();
    NS_ENSURE_SUCCESS(rv, rv);

    mEnabled = aIsEnabled;
    return NS_OK;
  }
  NS_NOTREACHED("neither enabled nor disabled!");
  return NS_ERROR_UNEXPECTED;
}

/* readonly attribute boolean isScanning; */
NS_IMETHODIMP
sbMediaManagementService::GetIsScanning(PRBool *aIsScanning)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/*****
 * sbIMediaManagementService
 *****/

/* void observe (in nsISupports aSubject, in string aTopic, in wstring aData); */
NS_IMETHODIMP
sbMediaManagementService::Observe(nsISupports *aSubject,
                                  const char *aTopic,
                                  const PRUnichar *aData)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(aTopic);
  NS_ASSERTION(NS_IsMainThread(), "observe not on main thread");
  TRACE(("%s: observing %s", __FUNCTION__, aTopic));

  if (!strcmp("profile-after-change", aTopic)) {
    nsCOMPtr<nsIPrefBranch> prefBranch =
      do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = prefBranch->GetBoolPref("songbird.media_management.library.enabled",
                                 &mEnabled);
    if (NS_FAILED(rv)) {
      mEnabled = PR_FALSE;
      NS_ENSURE_SUCCESS(rv, rv);
    }
    
    if (!mEnabled) {
      TRACE(("not enabled, don't bother doing anything else"));
      return NS_OK;
    }
    
    // library management is enabled, wait for library ready and manage things
  
    nsCOMPtr<nsIObserverService> obs =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = obs->AddObserver(this,
                          SB_LIBRARY_MANAGER_READY_TOPIC,
                          PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = obs->AddObserver(this,
                          SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC,
                          PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    
    return NS_OK;
  }

  if (!strcmp(SB_LIBRARY_MANAGER_READY_TOPIC, aTopic)) {
    // XXX Mook: manage main lib only for now? :(
    rv = GetMainLibrary(getter_AddRefs(mLibrary));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIObserverService> obs =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obs->RemoveObserver(this, aTopic);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (!mDelayedStartupTimer) {
      mDelayedStartupTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    
    TRACE(("%s: arming delayed startup timer", __FUNCTION__));
    rv = mDelayedStartupTimer->InitWithCallback(this,
                                                MMS_STARTUP_DELAY,
                                                nsITimer::TYPE_ONE_SHOT);
    if (NS_FAILED(rv)) {
      mDelayedStartupTimer = nsnull;
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return NS_OK;
  }
  
  if (!strcmp(SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC, aTopic)) {
    TRACE(("%s: shutting down", __FUNCTION__));
    rv = StopListening();
    NS_ENSURE_SUCCESS(rv, rv);
    
    mLibrary = nsnull;
    return NS_OK;
  }
  
  if (!strcmp("app-startup", aTopic)) {
    return NS_OK;
  }

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*****
 * sbIMediaListListener
 *****/

/* boolean onItemAdded (in sbIMediaList aMediaList, in sbIMediaItem aMediaItem,
                        in unsigned long aIndex); */
NS_IMETHODIMP
sbMediaManagementService::OnItemAdded(sbIMediaList *aMediaList,
                                      sbIMediaItem *aMediaItem,
                                      PRUint32 aIndex,
                                      PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE; /* keep listening to messages */
  return this->QueueItem(aMediaItem, sbIMediaFileManager::MANAGE_COPY);
}

/* boolean onBeforeItemRemoved (in sbIMediaList aMediaList, in sbIMediaItem aMediaItem,
                                in unsigned long aIndex); */
NS_IMETHODIMP
sbMediaManagementService::OnBeforeItemRemoved(sbIMediaList *aMediaList,
                                              sbIMediaItem *aMediaItem,
                                              PRUint32 aIndex,
                                              PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_TRUE; /* skip all further OnBeforeItemRemoved */
  return NS_OK;
}

/* boolean onAfterItemRemoved (in sbIMediaList aMediaList, in sbIMediaItem aMediaItem,
                               in unsigned long aIndex); */
NS_IMETHODIMP
sbMediaManagementService::OnAfterItemRemoved(sbIMediaList *aMediaList,
                                             sbIMediaItem *aMediaItem,
                                             PRUint32 aIndex,
                                             PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE; /* keep listening to messages */
  return this->QueueItem(aMediaItem, sbIMediaFileManager::MANAGE_DELETE);
}

/* boolean onItemUpdated (in sbIMediaList aMediaList, in sbIMediaItem aMediaItem,
                          in sbIPropertyArray aProperties); */
NS_IMETHODIMP
sbMediaManagementService::OnItemUpdated(sbIMediaList *aMediaList,
                                        sbIMediaItem *aMediaItem,
                                        sbIPropertyArray *aProperties,
                                        PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE; /* keep listening to messages */
  return this->QueueItem(aMediaItem, sbIMediaFileManager::MANAGE_MOVE);
}

/* boolean onItemMoved (in sbIMediaList aMediaList, in unsigned long aFromIndex,
                        in unsigned long aToIndex); */
NS_IMETHODIMP
sbMediaManagementService::OnItemMoved(sbIMediaList *aMediaList,
                                      PRUint32 aFromIndex,
                                      PRUint32 aToIndex,
                                      PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_TRUE; /* skip all further OnItemMoved */
  return NS_OK;
}

/* boolean onListCleared (in sbIMediaList aMediaList); */
NS_IMETHODIMP
sbMediaManagementService::OnListCleared(sbIMediaList *aMediaList,
                                        PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_TRUE; /* skip all further OnListCleared */
  return NS_OK;
}

/* void onBatchBegin (in sbIMediaList aMediaList); */
NS_IMETHODIMP
sbMediaManagementService::OnBatchBegin(sbIMediaList *aMediaList)
{
  /* we don't care about batching, we use a timeout anyway */
  return NS_OK;
}

/* void onBatchEnd (in sbIMediaList aMediaList); */
NS_IMETHODIMP
sbMediaManagementService::OnBatchEnd(sbIMediaList *aMediaList)
{
  /* we don't care about batching, we use a timeout anyway */
  return NS_OK;
}

/*****
 * nsITimerCallback
 */
/* void notify (in nsITimer timer); */
NS_IMETHODIMP
sbMediaManagementService::Notify(nsITimer *aTimer)
{
  NS_ENSURE_ARG_POINTER(aTimer);
  nsresult rv;

  if (aTimer == mDelayedStartupTimer) {
    TRACE(("%s: delayed startup timer fired", __FUNCTION__));
    mDelayedStartupTimer = nsnull;

    rv = StartListening();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = ScanLibrary();
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  } else if (aTimer == mPerformActionTimer) {
    TRACE(("%s: perform action timer fired", __FUNCTION__));
    
    NS_ENSURE_TRUE(mDirtyItems.IsInitialized(), NS_ERROR_NOT_INITIALIZED);
    
    // get the management type
    mManageMode = 0;
    PRBool isSet;
    nsCOMPtr<nsIPrefBranch> prefBranch =
      do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = prefBranch->GetBoolPref("songbird.media_management.library.copy",
                                 &isSet);
    if (NS_SUCCEEDED(rv) && isSet) {
      mManageMode |= sbIMediaFileManager::MANAGE_COPY;
    }
    
    rv = prefBranch->GetBoolPref("songbird.media_management.library.move",
                                 &isSet);
    if (NS_SUCCEEDED(rv) && isSet) {
      mManageMode |= sbIMediaFileManager::MANAGE_MOVE;
    }

    rv = prefBranch->GetBoolPref("songbird.media_management.library.rename",
                                 &isSet);
    if (NS_SUCCEEDED(rv) && isSet) {
      mManageMode |= sbIMediaFileManager::MANAGE_RENAME;
    }

    rv = prefBranch->GetBoolPref("songbird.media_management.library.delete",
                                 &isSet);
    if (NS_SUCCEEDED(rv) && isSet) {
      mManageMode |= sbIMediaFileManager::MANAGE_DELETE;
    }

    if (mManageMode) {
      nsCOMPtr<sbIMediaFileManager> fileMan =
        do_CreateInstance(SB_MEDIAFILEMANAGER_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      
      ProcessItemData data = { this, fileMan };
      
      PRUint32 count = mDirtyItems.EnumerateRead(ProcessItem, &data);
    }
    return NS_OK;
  }
  
  NS_NOTREACHED("sbMediaManagementService::Notify on an unknown timer");
  return NS_ERROR_UNEXPECTED;
}

/*****
 * sbMediaManagementService
 *****/

NS_METHOD
sbMediaManagementService::Init()
{
  TRACE(("%s: initing", __FUNCTION__));
  NS_ENSURE_FALSE(mLibrary, NS_ERROR_ALREADY_INITIALIZED);
  nsresult rv;
  
  NS_ENSURE_TRUE(mDirtyItems.Init(), NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIObserverService> obs =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = obs->AddObserver(this,
                        "profile-after-change",
                        PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_METHOD
sbMediaManagementService::ScanLibrary()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD
sbMediaManagementService::QueueItem(sbIMediaItem* aItem, PRUint32 aOperation)
{
  PRBool success;
  
  #if DEBUG
  PRUint32 oldOp;
  success = mDirtyItems.Get(aItem, &oldOp);
  if (success) {
    NS_ASSERTION(oldOp != sbIMediaFileManager::MANAGE_DELETE,
                 "operations on a deleted media item!");
  }
  #endif
  
  success = mDirtyItems.Put(aItem, aOperation);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  
  nsresult rv;
  
  if (!mPerformActionTimer) {
    mPerformActionTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  TRACE(("%s: item changed, setting timer", __FUNCTION__));
  rv = mPerformActionTimer->InitWithCallback(this,
                                             MMS_SCAN_DELAY,
                                             nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_METHOD
sbMediaManagementService::StartListening()
{
  TRACE(("%s: starting", __FUNCTION__));
  NS_ENSURE_TRUE(mLibrary, NS_ERROR_NOT_INITIALIZED);
  nsresult rv;
  
  rv = mLibrary->AddListener(this,
                             PR_FALSE,
                             sbIMediaList::LISTENER_FLAGS_ITEMADDED |
                               sbIMediaList::LISTENER_FLAGS_AFTERITEMREMOVED |
                               sbIMediaList::LISTENER_FLAGS_ITEMUPDATED |
                               sbIMediaList::LISTENER_FLAGS_LISTCLEARED,
                             nsnull);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

NS_METHOD
sbMediaManagementService::StopListening()
{
  TRACE(("%s: stopping", __FUNCTION__));
  NS_ENSURE_TRUE(mLibrary, NS_ERROR_NOT_INITIALIZED);
  nsresult rv;
  
  rv = mLibrary->RemoveListener(this);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // flush
  if (mPerformActionTimer) {
    rv = mPerformActionTimer->InitWithCallback(this, 0, nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  return NS_OK;
}

/* static */
PLDHashOperator
sbMediaManagementService::ProcessItem(nsISupports* aKey,
                                      PRUint32 aOperation,
                                      void* aClosure)
{
  nsresult rv;
  
  ProcessItemData* data = 
    reinterpret_cast<ProcessItemData*>(aClosure);
  PRUint32 opMask = data->mediaMgmtService->mManageMode;

  if (!(aOperation & opMask)) {
    // we don't want to manage this
    return PL_DHASH_NEXT;
  }
  
  switch (aOperation) {
    /* on copy and move, also rename if enabled */
    case sbIMediaFileManager::MANAGE_COPY:
    case sbIMediaFileManager::MANAGE_MOVE:
      aOperation |= (opMask & sbIMediaFileManager::MANAGE_RENAME);
  }

  nsCOMPtr<sbIMediaItem> item = do_QueryInterface(aKey);
  NS_ENSURE_TRUE(item, PL_DHASH_STOP);
  PRBool success;
  rv = data->fileMan->OrganizeItem(item, aOperation, &success);
  if (NS_FAILED(rv)) {
    nsresult __rv = rv;
    NS_ENSURE_SUCCESS_BODY(rv, rv);
  } else if (!success) {
    NS_WARNING("%s: OrganizeItem failed with no error code");
  }
  return PL_DHASH_NEXT;
}
