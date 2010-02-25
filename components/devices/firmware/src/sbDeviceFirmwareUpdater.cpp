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

#include "sbDeviceFirmwareUpdater.h"

#include <nsILocalFile.h>
#include <nsIMutableArray.h>
#include <nsIObserverService.h>
#include <nsISupportsPrimitives.h>
#include <nsIVariant.h>

#include <sbIDevice.h>
#include <sbIDeviceEvent.h>
#include <sbIDeviceEventTarget.h>
#include <sbIDeviceFirmwareHandler.h>
#include <sbILibraryManager.h>

#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsArrayUtils.h>
#include <nsComponentManagerUtils.h>
#include <prlog.h>

#include "sbDeviceFirmwareDownloader.h"
#include "sbDeviceFirmwareUpdate.h"

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbDeviceFirmwareUpdater:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gDeviceFirmwareUpdater = nsnull;
#define TRACE(args) PR_LOG(gDeviceFirmwareUpdater, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gDeviceFirmwareUpdater, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

#define MIN_RUNNING_HANDLERS (2)

#define FIRMWARE_FILE_PREF      "firmware.cache.file"
#define FIRMWARE_VERSION_PREF   "firmware.cache.version"
#define FIRMWARE_READABLE_PREF  "firmware.cache.readableVersion"

NS_IMPL_THREADSAFE_ISUPPORTS2(sbDeviceFirmwareUpdater,
                              sbIDeviceFirmwareUpdater,
                              sbIDeviceEventListener)

sbDeviceFirmwareUpdater::sbDeviceFirmwareUpdater()
: mMonitor(nsnull)
, mIsShutdown(PR_FALSE)
{
#ifdef PR_LOGGING
  if(!gDeviceFirmwareUpdater) {
    gDeviceFirmwareUpdater = PR_NewLogModule("sbDeviceFirmwareUpdater");
  }
#endif
}

sbDeviceFirmwareUpdater::~sbDeviceFirmwareUpdater()
{
  if(mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
  }
}

nsresult
sbDeviceFirmwareUpdater::Init()
{
  LOG(("[sbDeviceFirmwareUpdater] - Init"));

  mMonitor = nsAutoMonitor::NewMonitor("sbDeviceFirmwareUpdater::mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsISimpleEnumerator> categoryEnum;

  nsCOMPtr<nsICategoryManager> cm =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = cm->EnumerateCategory(SB_DEVICE_FIRMWARE_HANDLER_CATEGORY,
                             getter_AddRefs(categoryEnum));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  while (NS_SUCCEEDED(categoryEnum->HasMoreElements(&hasMore)) &&
         hasMore) {

    nsCOMPtr<nsISupports> ptr;
    if (NS_SUCCEEDED(categoryEnum->GetNext(getter_AddRefs(ptr))) &&
        ptr) {

      nsCOMPtr<nsISupportsCString> stringValue(do_QueryInterface(ptr));

      nsCString factoryName;
      if (stringValue &&
          NS_SUCCEEDED(stringValue->GetData(factoryName))) {

        nsCString contractId;
        rv = cm->GetCategoryEntry(SB_DEVICE_FIRMWARE_HANDLER_CATEGORY,
                                  factoryName.get(), getter_Copies(contractId));
        NS_ENSURE_SUCCESS(rv, rv);

        {
          nsAutoMonitor mon(mMonitor);
          nsCString *element = 
            mFirmwareHandlers.AppendElement(contractId);
          NS_ENSURE_TRUE(element, NS_ERROR_OUT_OF_MEMORY);

          LOG(("[sbDeviceFirmwareUpdater] - Init - Adding %s as a firmware handler", 
               contractId.BeginReading()));
        }
      }
    }
  }

  PRBool success = mRunningHandlers.Init(MIN_RUNNING_HANDLERS);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mRecoveryModeHandlers.Init(MIN_RUNNING_HANDLERS);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mHandlerStatus.Init(MIN_RUNNING_HANDLERS);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mDownloaders.Init(MIN_RUNNING_HANDLERS);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIEventTarget> threadPool = 
    do_GetService("@songbirdnest.com/Songbird/ThreadPoolService;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  threadPool.swap(mThreadPool);

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, SB_LIBRARY_MANAGER_SHUTDOWN_TOPIC,
                                    PR_FALSE);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to add library manager observer");

  return NS_OK;
}

nsresult
sbDeviceFirmwareUpdater::Shutdown()
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mIsShutdown, NS_ERROR_ILLEGAL_DURING_SHUTDOWN);

  nsAutoMonitor mon(mMonitor);
  
  // Even if we fail to shutdown we will report ourselves as shutdown.
  // This is done on purpose to avoid new operations coming into play
  // during the shutdown sequence under error conditions.
  mIsShutdown = PR_TRUE;

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIMutableArray> mutableArray =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mRunningHandlers.EnumerateRead(sbDeviceFirmwareUpdater::EnumerateIntoArrayISupportsKey,
                                 mutableArray.get());

  PRUint32 length = 0;
  rv = mutableArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRUint32 current = 0; current < length; ++current) {
    nsCOMPtr<sbIDeviceFirmwareHandler> handler = 
      do_QueryElementAt(mutableArray, current, &rv);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Bad object in hashtable!");
    if(NS_FAILED(rv)) {
      continue;
    }

    sbDeviceFirmwareHandlerStatus *handlerStatus = GetHandlerStatus(handler);
    NS_ENSURE_TRUE(handlerStatus, NS_ERROR_OUT_OF_MEMORY);

    sbDeviceFirmwareHandlerStatus::handlerstatus_t status = 
      sbDeviceFirmwareHandlerStatus::STATUS_NONE;
    rv = handlerStatus->GetStatus(&status);
    NS_ENSURE_SUCCESS(rv, rv);

    if(status == sbDeviceFirmwareHandlerStatus::STATUS_WAITING_FOR_START ||
       status == sbDeviceFirmwareHandlerStatus::STATUS_RUNNING) {

      rv = handler->Cancel();
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), 
        "Failed to cancel, may not shutdown cleanly!");
    }
  }

  mRunningHandlers.Clear();
  mRecoveryModeHandlers.Clear();
  mHandlerStatus.Clear();
  mDownloaders.Clear();

  return NS_OK;
}

already_AddRefed<sbIDeviceFirmwareHandler> 
sbDeviceFirmwareUpdater::GetRunningHandler(sbIDevice *aDevice)
{
  NS_ENSURE_TRUE(aDevice, nsnull);

  sbIDeviceFirmwareHandler *_retval = nsnull;

  nsCOMPtr<sbIDeviceFirmwareHandler> handler;
  if(mRunningHandlers.Get(aDevice, getter_AddRefs(handler))) {
    handler.forget(&_retval);
  }

  return _retval;
}

already_AddRefed<sbIDeviceFirmwareHandler>
sbDeviceFirmwareUpdater::GetRunningHandler(sbIDevice *aDevice, 
                                           PRUint32 aVendorID,
                                           PRUint32 aProductID,
                                           sbIDeviceEventListener *aListener, 
                                           PRBool aCreate)
{
  NS_ENSURE_TRUE(aDevice, nsnull);

  sbIDeviceFirmwareHandler *_retval = nsnull;

  nsCOMPtr<sbIDeviceFirmwareHandler> handler;
  if(!mRunningHandlers.Get(aDevice, getter_AddRefs(handler)) && aCreate) {
    nsresult rv = GetHandler(aDevice, 
                             aVendorID, 
                             aProductID, 
                             getter_AddRefs(handler));
    NS_ENSURE_SUCCESS(rv, nsnull);

    rv = handler->Bind(aDevice, aListener);
    NS_ENSURE_SUCCESS(rv, nsnull);

    rv = PutRunningHandler(aDevice, handler);
    NS_ENSURE_SUCCESS(rv, nsnull);
  }

  if(handler) {
    handler.forget(&_retval);
  }

  return _retval;
}

nsresult
sbDeviceFirmwareUpdater::PutRunningHandler(sbIDevice *aDevice, 
                                           sbIDeviceFirmwareHandler *aHandler)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aHandler);

  nsCOMPtr<sbIDeviceFirmwareHandler> handler;
  if(!mRunningHandlers.Get(aDevice, getter_AddRefs(handler))) {
    PRBool success = mRunningHandlers.Put(aDevice, aHandler);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }
#if defined PR_LOGGING
  else {
    NS_WARN_IF_FALSE(handler == aHandler, 
      "Attempting to replace a running firmware handler!");
  }
#endif

  return NS_OK;
}

sbDeviceFirmwareHandlerStatus* 
sbDeviceFirmwareUpdater::GetHandlerStatus(sbIDeviceFirmwareHandler *aHandler)
{
  NS_ENSURE_TRUE(mMonitor, nsnull);
  NS_ENSURE_TRUE(aHandler, nsnull);

  nsAutoMonitor mon(mMonitor);
  sbDeviceFirmwareHandlerStatus *_retval = nsnull;

  if(!mHandlerStatus.Get(aHandler, &_retval)) {
    nsAutoPtr<sbDeviceFirmwareHandlerStatus> status(new sbDeviceFirmwareHandlerStatus);

    nsresult rv = status->Init();
    NS_ENSURE_SUCCESS(rv, nsnull);
    
    PRBool success = mHandlerStatus.Put(aHandler, status);
    NS_ENSURE_TRUE(success, nsnull);

    _retval = status.forget();
  }

  return _retval;
}

nsresult 
sbDeviceFirmwareUpdater::RequiresRecoveryMode(sbIDevice *aDevice,
                                              sbIDeviceFirmwareHandler *aHandler)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aHandler);

  PRBool needsRecoveryMode = PR_FALSE;
  
  nsresult rv = aHandler->GetNeedsRecoveryMode(&needsRecoveryMode);
  NS_ENSURE_SUCCESS(rv, rv);

  if(needsRecoveryMode && !mRecoveryModeHandlers.Get(aDevice, nsnull)) {
    PRBool success = mRecoveryModeHandlers.Put(aDevice, aHandler);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

nsresult
sbDeviceFirmwareUpdater::GetCachedFirmwareUpdate(sbIDevice *aDevice,
                                                 sbIDeviceFirmwareUpdate **aUpdate)
{
  nsCOMPtr<nsIVariant> firmwareVersion;
  nsresult rv = 
    aDevice->GetPreference(NS_LITERAL_STRING(FIRMWARE_VERSION_PREF),
                           getter_AddRefs(firmwareVersion));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 prefVersion = 0;
  rv = firmwareVersion->GetAsUint32(&prefVersion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDevice->GetPreference(NS_LITERAL_STRING(FIRMWARE_READABLE_PREF),
                              getter_AddRefs(firmwareVersion));

  nsString prefReadableVersion;
  rv = firmwareVersion->GetAsAString(prefReadableVersion);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> firmwareFilePath;
  rv = aDevice->GetPreference(NS_LITERAL_STRING(FIRMWARE_FILE_PREF),
    getter_AddRefs(firmwareFilePath));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString filePath;
  rv = firmwareFilePath->GetAsAString(filePath);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFile> localFile;
  rv = NS_NewLocalFile(filePath, PR_FALSE, getter_AddRefs(localFile));

  PRBool exists = PR_FALSE;
  rv = localFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if(!exists) {
    *aUpdate = nsnull;
    return NS_OK;
  }

  nsCOMPtr<sbIDeviceFirmwareUpdate> firmwareUpdate = 
    do_CreateInstance(SB_DEVICEFIRMWAREUPDATE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = firmwareUpdate->Init(localFile, 
                            prefReadableVersion, 
                            prefVersion);
  NS_ENSURE_SUCCESS(rv, rv);

  firmwareUpdate.forget(aUpdate);

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::CheckForUpdate(sbIDevice *aDevice, 
                                        sbIDeviceEventListener *aListener)
{
  LOG(("[sbDeviceFirmwareUpdater] - CheckForUpdate"));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mIsShutdown, NS_ERROR_ILLEGAL_DURING_SHUTDOWN);
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<sbIDeviceFirmwareHandler> handler = 
    GetRunningHandler(aDevice, 0, 0, aListener, PR_TRUE);
  
  NS_ENSURE_TRUE(handler, NS_ERROR_UNEXPECTED);
  
  PRBool canUpdate;
  rv = handler->CanUpdate(aDevice, 0, 0, &canUpdate);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(canUpdate, NS_ERROR_NOT_IMPLEMENTED);

  nsAutoMonitor mon(mMonitor);
  
  sbDeviceFirmwareHandlerStatus *handlerStatus = GetHandlerStatus(handler);
  NS_ENSURE_TRUE(handlerStatus, NS_ERROR_OUT_OF_MEMORY);

  sbDeviceFirmwareHandlerStatus::handlerstatus_t status = 
    sbDeviceFirmwareHandlerStatus::STATUS_NONE;
  rv = handlerStatus->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);

  if(status != sbDeviceFirmwareHandlerStatus::STATUS_NONE &&
     status != sbDeviceFirmwareHandlerStatus::STATUS_FINISHED) {
    return NS_ERROR_FAILURE;
  }


  nsCOMPtr<sbIDeviceEventTarget> eventTarget = do_QueryInterface(aDevice, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = eventTarget->AddEventListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = PutRunningHandler(aDevice, handler);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = handlerStatus->SetOperation(sbDeviceFirmwareHandlerStatus::OP_REFRESH);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = handlerStatus->SetStatus(sbDeviceFirmwareHandlerStatus::STATUS_WAITING_FOR_START);
  NS_ENSURE_SUCCESS(rv, rv);

  mon.Exit();

  rv = handler->RefreshInfo();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::DownloadUpdate(sbIDevice *aDevice, 
                                        PRBool aVerifyFirmwareUpdate, 
                                        sbIDeviceEventListener *aListener)
{
  LOG(("[sbDeviceFirmwareUpdater] - DownloadUpdate"));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mIsShutdown, NS_ERROR_ILLEGAL_DURING_SHUTDOWN);
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<sbIDeviceFirmwareHandler> handler = 
    GetRunningHandler(aDevice, 0, 0, aListener, PR_TRUE);

  nsAutoMonitor mon(mMonitor);
  
  sbDeviceFirmwareHandlerStatus *handlerStatus = GetHandlerStatus(handler);
  NS_ENSURE_TRUE(handlerStatus, NS_ERROR_OUT_OF_MEMORY);

  sbDeviceFirmwareHandlerStatus::handlerstatus_t status = 
    sbDeviceFirmwareHandlerStatus::STATUS_NONE;
  rv = handlerStatus->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);

  if(status != sbDeviceFirmwareHandlerStatus::STATUS_NONE &&
     status != sbDeviceFirmwareHandlerStatus::STATUS_FINISHED) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<sbIDeviceEventTarget> eventTarget = do_QueryInterface(aDevice, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = eventTarget->AddEventListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = PutRunningHandler(aDevice, handler);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = handlerStatus->SetOperation(sbDeviceFirmwareHandlerStatus::OP_DOWNLOAD);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = handlerStatus->SetStatus(sbDeviceFirmwareHandlerStatus::STATUS_WAITING_FOR_START);
  NS_ENSURE_SUCCESS(rv, rv);

  mon.Exit();

  nsRefPtr<sbDeviceFirmwareDownloader> downloader;
  NS_NEWXPCOM(downloader, sbDeviceFirmwareDownloader);
  NS_ENSURE_TRUE(downloader, NS_ERROR_OUT_OF_MEMORY);

  rv = downloader->Init(aDevice,
                        aListener,
                        handler);
  NS_ENSURE_SUCCESS(rv, rv);

  // Firmware Downloader is responsible for sending all necessary events.
  rv = downloader->Start();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIFileDownloaderListener> listener;
  if(mDownloaders.Get(aDevice, getter_AddRefs(listener))) {
    sbDeviceFirmwareDownloader *downloader = 
      reinterpret_cast<sbDeviceFirmwareDownloader *>(listener.get());
    
    rv = downloader->Cancel();
    NS_ENSURE_SUCCESS(rv, rv);

    mDownloaders.Remove(aDevice);
  }

  PRBool success = mDownloaders.Put(aDevice, downloader);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::VerifyUpdate(sbIDevice *aDevice, 
                                      sbIDeviceFirmwareUpdate *aFirmwareUpdate, 
                                      sbIDeviceEventListener *aListener)
{
  LOG(("[sbDeviceFirmwareUpdater] - VerifyUpdate"));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mIsShutdown, NS_ERROR_ILLEGAL_DURING_SHUTDOWN);
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aFirmwareUpdate);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::ApplyUpdate(sbIDevice *aDevice, 
                                     sbIDeviceFirmwareUpdate *aFirmwareUpdate, 
                                     sbIDeviceEventListener *aListener)
{
  LOG(("[sbDeviceFirmwareUpdater] - ApplyUpdate"));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mIsShutdown, NS_ERROR_ILLEGAL_DURING_SHUTDOWN);
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aFirmwareUpdate);

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<sbIDeviceFirmwareHandler> handler = 
    GetRunningHandler(aDevice, 0, 0, aListener, PR_TRUE);

  nsAutoMonitor mon(mMonitor);

  sbDeviceFirmwareHandlerStatus *handlerStatus = GetHandlerStatus(handler);
  NS_ENSURE_TRUE(handlerStatus, NS_ERROR_OUT_OF_MEMORY);

  sbDeviceFirmwareHandlerStatus::handlerstatus_t status = 
    sbDeviceFirmwareHandlerStatus::STATUS_NONE;
  rv = handlerStatus->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);

  if(status != sbDeviceFirmwareHandlerStatus::STATUS_NONE &&
     status != sbDeviceFirmwareHandlerStatus::STATUS_FINISHED) {
    return NS_ERROR_FAILURE;
  }


  nsCOMPtr<sbIDeviceEventTarget> eventTarget = do_QueryInterface(aDevice, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = eventTarget->AddEventListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = PutRunningHandler(aDevice, handler);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = handlerStatus->SetOperation(sbDeviceFirmwareHandlerStatus::OP_UPDATE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = handlerStatus->SetStatus(sbDeviceFirmwareHandlerStatus::STATUS_WAITING_FOR_START);
  NS_ENSURE_SUCCESS(rv, rv);

  mon.Exit();

  nsRefPtr<sbDeviceFirmwareUpdaterRunner> runner;
  NS_NEWXPCOM(runner, sbDeviceFirmwareUpdaterRunner);
  NS_ENSURE_TRUE(runner, NS_ERROR_OUT_OF_MEMORY);

  rv = runner->Init(aDevice, aFirmwareUpdate, handler);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mThreadPool->Dispatch(runner, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::RecoveryUpdate(sbIDevice *aDevice, 
                                        sbIDeviceFirmwareUpdate *aFirmwareUpdate,
                                        PRUint32 aDeviceVendorID,
                                        PRUint32 aDeviceProductID,
                                        sbIDeviceEventListener *aListener)
{
  LOG(("[sbDeviceFirmwareUpdater] - ApplyUpdate"));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mIsShutdown, NS_ERROR_ILLEGAL_DURING_SHUTDOWN);
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<sbIDeviceFirmwareHandler> handler = 
    GetRunningHandler(aDevice, 
                      aDeviceVendorID, 
                      aDeviceProductID, 
                      aListener, 
                      PR_TRUE);

  nsAutoMonitor mon(mMonitor);

  sbDeviceFirmwareHandlerStatus *handlerStatus = GetHandlerStatus(handler);
  NS_ENSURE_TRUE(handlerStatus, NS_ERROR_OUT_OF_MEMORY);

  sbDeviceFirmwareHandlerStatus::handlerstatus_t status = 
    sbDeviceFirmwareHandlerStatus::STATUS_NONE;
  rv = handlerStatus->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);

  if(status != sbDeviceFirmwareHandlerStatus::STATUS_NONE &&
     status != sbDeviceFirmwareHandlerStatus::STATUS_FINISHED) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<sbIDeviceEventTarget> eventTarget = do_QueryInterface(aDevice, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = eventTarget->AddEventListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = PutRunningHandler(aDevice, handler);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = handlerStatus->SetOperation(sbDeviceFirmwareHandlerStatus::OP_RECOVERY);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = handlerStatus->SetStatus(sbDeviceFirmwareHandlerStatus::STATUS_WAITING_FOR_START);
  NS_ENSURE_SUCCESS(rv, rv);

  mon.Exit();

  // This will determine if we need to cache the firmware update
  // or if it's already cached.
  PRBool needsCaching = PR_FALSE;

  // This will be the actual final firmware update we use.
  nsCOMPtr<sbIDeviceFirmwareUpdate> firmwareUpdate;

  // 'Default' firmware update, this may end up being the firmware update
  // passed in as an argument
  nsCOMPtr<sbIDeviceFirmwareUpdate> defaultFirmwareUpdate;

  // Cached firmware update
  nsCOMPtr<sbIDeviceFirmwareUpdate> cachedFirmwareUpdate;
  rv = GetCachedFirmwareUpdate(aDevice, getter_AddRefs(cachedFirmwareUpdate));
  LOG(("Using cached firmware update? %s", 
       NS_SUCCEEDED(rv) ? "true" : "false"));

  // No firmware update passed, use default one.
  if(!aFirmwareUpdate) {
    rv = handler->GetDefaultFirmwareUpdate(getter_AddRefs(defaultFirmwareUpdate));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    //
    // Use specific firmware updated passed. In this case we assume 
    // it's available and has already been cached into the firmware cache
    // via a download.
    //
    // This isn't totally awesome, it would be nice to instead check the path
    // of the firmware image and see if it's in the firmware cache path already
    // or not. But even then one would have to make sure it's in the exact
    // path for the firmware cache for the device.
    //
    defaultFirmwareUpdate = aFirmwareUpdate;
    needsCaching = PR_FALSE;
  }

  // Check to make sure the cached firmware image is the same name
  // as the default one. If not, we need to delete the cached one and use
  // the default one. We'll also check the filesizes, if they don't match
  // we're going to prefer the default firmware instead.
  if(cachedFirmwareUpdate && defaultFirmwareUpdate) {
    nsCOMPtr<nsIFile> cachedFile;
    rv = cachedFirmwareUpdate->GetFirmwareImageFile(getter_AddRefs(cachedFile));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> defaultFile;
    rv = defaultFirmwareUpdate->GetFirmwareImageFile(getter_AddRefs(defaultFile));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString cachedFileName, defaultFileName;
    rv = cachedFile->GetLeafName(cachedFileName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = defaultFile->GetLeafName(defaultFileName);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt64 cachedFileSize = 0;
    rv = cachedFile->GetFileSize(&cachedFileSize);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt64 defaultFileSize = 0;
    rv = defaultFile->GetFileSize(&defaultFileSize);
    NS_ENSURE_SUCCESS(rv, rv);

    if((cachedFileName != defaultFileName) || 
       (cachedFileName == defaultFileName && cachedFileSize != defaultFileSize)) {
      nsCOMPtr<nsIFile> cacheDir;
      rv = cachedFile->GetParent(getter_AddRefs(cacheDir));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = cachedFile->Remove(PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsISimpleEnumerator> cacheDirEntries;
      rv = cacheDir->GetDirectoryEntries(getter_AddRefs(cacheDirEntries));
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool hasMore = PR_FALSE;
      rv = cacheDirEntries->HasMoreElements(&hasMore);
      NS_ENSURE_SUCCESS(rv, rv);

      while(hasMore) {
        nsCOMPtr<nsIFile> file;
        rv = cacheDirEntries->GetNext(getter_AddRefs(file));
        NS_ENSURE_SUCCESS(rv, rv);

        PRBool isFile = PR_FALSE;
        rv = file->IsFile(&isFile);
        NS_ENSURE_SUCCESS(rv, rv);
        
        if(isFile) {
          rv = file->Remove(PR_FALSE);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        rv = cacheDirEntries->HasMoreElements(&hasMore);
        NS_ENSURE_SUCCESS(rv, rv);
      }
        
      firmwareUpdate = defaultFirmwareUpdate;
      needsCaching = PR_TRUE;
    }
    else {
      firmwareUpdate = cachedFirmwareUpdate;
      needsCaching = PR_FALSE;
    }
  }
  else if(cachedFirmwareUpdate) {
    firmwareUpdate = cachedFirmwareUpdate;
  }
  else if(defaultFirmwareUpdate) {
    firmwareUpdate = defaultFirmwareUpdate;
    needsCaching = PR_TRUE;
  }
  else {
    return NS_ERROR_UNEXPECTED;
  }

  //
  // This isn't great to have to do this on the Main Thread
  // but the components required to cache the firmware update
  // are inherently NOT thread-safe and proxying to the main 
  // thread causes A LOT of badness as it pumps the main thread
  // at a really bad time almost EVERYTIME.
  //
  // Long term we'd probably want to do this copy in an async 
  // manner and then call this method again after the file has been cached.
  //
  // Yes, this sucks if it's a 100MB firmware image. But that's really, really
  // really, really rare :)
  //
  if(needsCaching) {
    nsCOMPtr<sbIDeviceFirmwareUpdate> cachedFirmwareUpdate;
    rv = sbDeviceFirmwareDownloader::CacheFirmwareUpdate(aDevice,
                                                         firmwareUpdate, 
                                                         getter_AddRefs(cachedFirmwareUpdate));
    NS_ENSURE_SUCCESS(rv, rv);
    
    cachedFirmwareUpdate.swap(firmwareUpdate);
  }

  nsRefPtr<sbDeviceFirmwareUpdaterRunner> runner;
  NS_NEWXPCOM(runner, sbDeviceFirmwareUpdaterRunner);
  NS_ENSURE_TRUE(runner, NS_ERROR_OUT_OF_MEMORY);

  rv = runner->Init(aDevice, firmwareUpdate, handler, PR_TRUE, needsCaching);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mThreadPool->Dispatch(runner, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceFirmwareUpdater::ContinueUpdate(sbIDevice *aDevice,
                                        sbIDeviceEventListener *aListener,
                                        PRBool *_retval)
{
  LOG(("[sbDeviceFirmwareUpdater] - ContinueUpdate"));
  
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mIsShutdown, NS_ERROR_ILLEGAL_DURING_SHUTDOWN);
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv = NS_ERROR_UNEXPECTED;

  *_retval = PR_FALSE;

  nsCOMPtr<nsIMutableArray> mutableArray =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mRecoveryModeHandlers.EnumerateRead(
    sbDeviceFirmwareUpdater::EnumerateIntoArrayISupportsKey,
    mutableArray.get());

  PRUint32 length = 0;
  rv = mutableArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRUint32 current = 0; current < length; ++current) {
    nsCOMPtr<sbIDeviceFirmwareHandler> handler = 
      do_QueryElementAt(mutableArray, current, &rv);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Bad object in hashtable!");
    if(NS_FAILED(rv)) {
      continue;
    }

    nsCOMPtr<sbIDevice> oldDevice;
    rv = handler->GetBoundDevice(getter_AddRefs(oldDevice));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool success = PR_FALSE;
    rv = handler->Rebind(aDevice, aListener, &success);
    NS_ENSURE_SUCCESS(rv, rv);

    if(success) {
      if (oldDevice) {
        mRecoveryModeHandlers.Remove(oldDevice);
        mRunningHandlers.Remove(oldDevice);
      }

      rv = PutRunningHandler(aDevice, handler);
      NS_ENSURE_SUCCESS(rv, rv);

      *_retval = PR_TRUE;
#ifdef PR_LOGGING
      nsString contractId;
      rv = handler->GetContractId(contractId);
      NS_ENSURE_SUCCESS(rv, rv);

      LOG(("[%s] continueUpdate successful", __FUNCTION__));
      LOG(("Handler with contract id %s will continue the update", 
           NS_ConvertUTF16toUTF8(contractId).BeginReading()));
#endif
      return NS_OK;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceFirmwareUpdater::FinalizeUpdate(sbIDevice *aDevice)
{
  LOG(("[sbDeviceFirmwareUpdater] - FinalizeUpdate"));
  
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mIsShutdown, NS_ERROR_ILLEGAL_DURING_SHUTDOWN);
  NS_ENSURE_ARG_POINTER(aDevice);

  nsCOMPtr<sbIDeviceFirmwareHandler> handler = GetRunningHandler(aDevice);
  NS_ENSURE_TRUE(handler, NS_OK);

  nsAutoMonitor mon(mMonitor);

  mRunningHandlers.Remove(aDevice);
  mRecoveryModeHandlers.Remove(aDevice);
  mHandlerStatus.Remove(handler);

  nsCOMPtr<sbIFileDownloaderListener> listener;
  if(mDownloaders.Get(aDevice, getter_AddRefs(listener))) {
    sbDeviceFirmwareDownloader *downloader = 
      reinterpret_cast<sbDeviceFirmwareDownloader *>(listener.get());
    
    nsresult rv = downloader->Cancel();
    NS_ENSURE_SUCCESS(rv, rv);

    mDownloaders.Remove(aDevice);
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::VerifyDevice(sbIDevice *aDevice, 
                                      sbIDeviceEventListener *aListener)
{
  LOG(("[sbDeviceFirmwareUpdater] - VerifyDevice"));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mIsShutdown, NS_ERROR_ILLEGAL_DURING_SHUTDOWN);
  NS_ENSURE_ARG_POINTER(aDevice);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::RegisterHandler(sbIDeviceFirmwareHandler *aFirmwareHandler)
{
  LOG(("[sbDeviceFirmwareUpdater] - RegisterHandler"));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mIsShutdown, NS_ERROR_ILLEGAL_DURING_SHUTDOWN);
  NS_ENSURE_ARG_POINTER(aFirmwareHandler);

  nsString contractId;
  nsresult rv = aFirmwareHandler->GetContractId(contractId);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ConvertUTF16toUTF8 contractId8(contractId);
  
  nsAutoMonitor mon(mMonitor);
  if(!mFirmwareHandlers.Contains(contractId8)) {
    nsCString *element = mFirmwareHandlers.AppendElement(contractId8);
    NS_ENSURE_TRUE(element, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::UnregisterHandler(sbIDeviceFirmwareHandler *aFirmwareHandler)
{
  LOG(("[sbDeviceFirmwareUpdater] - UnregisterHandler"));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mIsShutdown, NS_ERROR_ILLEGAL_DURING_SHUTDOWN);
  NS_ENSURE_ARG_POINTER(aFirmwareHandler);

  nsString contractId;
  nsresult rv = aFirmwareHandler->GetContractId(contractId);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ConvertUTF16toUTF8 contractId8(contractId);
  
  nsAutoMonitor mon(mMonitor);
  firmwarehandlers_t::index_type index = 
    mFirmwareHandlers.IndexOf(contractId8);

  if(index != firmwarehandlers_t::NoIndex) {
    mFirmwareHandlers.RemoveElementAt(index);
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::HasHandler(sbIDevice *aDevice, 
                                    PRUint32 aDeviceVendorID,
                                    PRUint32 aDeviceProductID,
                                    PRBool *_retval)
{
  LOG(("[sbDeviceFirmwareUpdater] - HasHandler"));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mIsShutdown, NS_ERROR_ILLEGAL_DURING_SHUTDOWN);
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIDeviceFirmwareHandler> handler;
  nsresult rv = GetHandler(aDevice, 
                           aDeviceVendorID, 
                           aDeviceProductID, 
                           getter_AddRefs(handler));
  
  *_retval = PR_FALSE;
  if(NS_SUCCEEDED(rv)) {
    *_retval = PR_TRUE;
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::GetHandler(sbIDevice *aDevice, 
                                    PRUint32 aDeviceVendorID,
                                    PRUint32 aDeviceProductID,
                                    sbIDeviceFirmwareHandler **_retval)
{
  LOG(("[sbDeviceFirmwareUpdater] - GetHandler"));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mIsShutdown, NS_ERROR_ILLEGAL_DURING_SHUTDOWN);
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(_retval);

  firmwarehandlers_t firmwareHandlers;
  
  // Because our data set is pretty small, it's much nicer to copy it
  // rather than operating on it for a long period of time with the
  // monitor held.
  {
    nsAutoMonitor mon(mMonitor);
    nsCString *element = firmwareHandlers.AppendElements(mFirmwareHandlers);
    NS_ENSURE_TRUE(element, NS_ERROR_OUT_OF_MEMORY);
  }

  nsresult rv = NS_ERROR_UNEXPECTED;
  firmwarehandlers_t::size_type length = firmwareHandlers.Length();
  for(firmwarehandlers_t::size_type i = 0; i < length; ++i) {
    nsCOMPtr<sbIDeviceFirmwareHandler> handler = 
      do_CreateInstance(firmwareHandlers[i].BeginReading(), &rv);
    if(NS_FAILED(rv)) {
      continue;
    }

    PRBool canHandleDevice = PR_FALSE;
    rv = handler->CanUpdate(aDevice, 
                            aDeviceVendorID, 
                            aDeviceProductID, 
                            &canHandleDevice);
    if(NS_FAILED(rv)) {
      continue;
    }

    // We assume there is only one device firmware handler
    // that can handle a device. In the future this may be
    // extended to put the handlers to a vote so that we may
    // pick the one that's most certain about it's ability to
    // handle the device.
    if(canHandleDevice) {
      handler.forget(_retval);
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
sbDeviceFirmwareUpdater::GetActiveHandler(sbIDevice *aDevice,
                                          sbIDeviceFirmwareHandler **_retval)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mIsShutdown, NS_ERROR_ILLEGAL_DURING_SHUTDOWN);
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIDeviceFirmwareHandler> handler = GetRunningHandler(aDevice);
  if(handler) {
    handler.forget(_retval);
    return NS_OK;
  }

  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
sbDeviceFirmwareUpdater::Cancel(sbIDevice *aDevice) 
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mIsShutdown, NS_ERROR_ILLEGAL_DURING_SHUTDOWN);
  NS_ENSURE_ARG_POINTER(aDevice);

  nsAutoMonitor mon(mMonitor);
  
  nsCOMPtr<sbIDeviceFirmwareHandler> handler = GetRunningHandler(aDevice);

  if(handler) {
    nsresult rv = handler->Cancel();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = handler->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);

    mRunningHandlers.Remove(aDevice);
    mHandlerStatus.Remove(handler);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceFirmwareUpdater::RequireRecovery(sbIDevice *aDevice)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);

  nsCOMPtr<sbIDeviceFirmwareHandler> handler = GetRunningHandler(aDevice);

  PRBool success = mRecoveryModeHandlers.Put(aDevice, handler);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceFirmwareUpdater::OnDeviceEvent(sbIDeviceEvent *aEvent) 
{
  LOG(("[sbDeviceFirmwareUpdater] - OnDeviceEvent"));

  NS_ENSURE_ARG_POINTER(aEvent);

  nsCOMPtr<sbIDevice> device;
  nsresult rv = aEvent->GetOrigin(getter_AddRefs(device));
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("[sbDeviceFirmwareUpdater] - Origin Device 0x%X", device.get()));

  nsCOMPtr<sbIDeviceFirmwareHandler> handler = GetRunningHandler(device);
  NS_ENSURE_TRUE(handler, NS_OK);

  PRUint32 eventType = 0;
  rv = aEvent->GetType(&eventType);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoMonitor mon(mMonitor);
  sbDeviceFirmwareHandlerStatus *handlerStatus = GetHandlerStatus(handler);
  NS_ENSURE_TRUE(handlerStatus, NS_ERROR_UNEXPECTED);
  
  sbDeviceFirmwareHandlerStatus::handleroperation_t operation = 
    sbDeviceFirmwareHandlerStatus::OP_NONE;
  rv = handlerStatus->GetOperation(&operation);
  NS_ENSURE_SUCCESS(rv, rv);

  sbDeviceFirmwareHandlerStatus::handlerstatus_t status = 
    sbDeviceFirmwareHandlerStatus::STATUS_NONE;
  rv = handlerStatus->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool removeListener = PR_FALSE;

  switch(operation) {
    case sbDeviceFirmwareHandlerStatus::OP_REFRESH: {
      LOG(("[sbDeviceFirmwareUpdater] - OP_REFRESH"));

      if(eventType == sbIDeviceEvent::EVENT_FIRMWARE_CFU_START &&
         status == sbDeviceFirmwareHandlerStatus::STATUS_WAITING_FOR_START) {
        rv = handlerStatus->SetStatus(sbDeviceFirmwareHandlerStatus::STATUS_RUNNING);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if((eventType == sbIDeviceEvent::EVENT_FIRMWARE_CFU_END ||
               eventType == sbIDeviceEvent::EVENT_FIRMWARE_CFU_ERROR) &&
              status == sbDeviceFirmwareHandlerStatus::STATUS_RUNNING) {
        rv = handlerStatus->SetStatus(sbDeviceFirmwareHandlerStatus::STATUS_FINISHED);
        NS_ENSURE_SUCCESS(rv, rv);

        removeListener = PR_TRUE;

        // Check to see if the device requires recovery mode
        rv = RequiresRecoveryMode(device, handler);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    break;

    case sbDeviceFirmwareHandlerStatus::OP_DOWNLOAD: {
      LOG(("[sbDeviceFirmwareUpdater] - OP_DOWNLOAD"));

      if(eventType == sbIDeviceEvent::EVENT_FIRMWARE_DOWNLOAD_START &&
         status == sbDeviceFirmwareHandlerStatus::STATUS_WAITING_FOR_START) {
        rv = handlerStatus->SetStatus(sbDeviceFirmwareHandlerStatus::STATUS_RUNNING);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if(eventType == sbIDeviceEvent::EVENT_FIRMWARE_DOWNLOAD_END &&
              status == sbDeviceFirmwareHandlerStatus::STATUS_RUNNING) {
        rv = handlerStatus->SetStatus(sbDeviceFirmwareHandlerStatus::STATUS_FINISHED);
        NS_ENSURE_SUCCESS(rv, rv);

        removeListener = PR_TRUE;
      }
    }
    break;

    case sbDeviceFirmwareHandlerStatus::OP_UPDATE:
    case sbDeviceFirmwareHandlerStatus::OP_RECOVERY: {
      LOG(("[sbDeviceFirmwareUpdater] - OP_UPDATE or OP_RECOVERY"));
      
      if(eventType == sbIDeviceEvent::EVENT_FIRMWARE_UPDATE_START &&
         status == sbDeviceFirmwareHandlerStatus::STATUS_WAITING_FOR_START) {
          rv = handlerStatus->SetStatus(sbDeviceFirmwareHandlerStatus::STATUS_RUNNING);
          NS_ENSURE_SUCCESS(rv, rv);
      }
      else if(eventType == sbIDeviceEvent::EVENT_FIRMWARE_UPDATE_END &&
              status == sbDeviceFirmwareHandlerStatus::STATUS_RUNNING) {
          rv = handlerStatus->SetStatus(sbDeviceFirmwareHandlerStatus::STATUS_FINISHED);
          NS_ENSURE_SUCCESS(rv, rv);

          removeListener = PR_TRUE;
      }
      else if(eventType == sbIDeviceEvent::EVENT_FIRMWARE_NEEDREC_ERROR) {
        // Reset status when we get a NEED RECOVERY error to enable normal
        // operation after device is reconnected in recovery mode.
        rv = handlerStatus->SetStatus(sbDeviceFirmwareHandlerStatus::STATUS_NONE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    break;

    default:
      NS_WARNING("Unsupported operation");
  }

  mon.Exit();

  if(removeListener) {
    nsCOMPtr<sbIDeviceEventTarget> eventTarget = 
      do_QueryInterface(device, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = eventTarget->RemoveEventListener(this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

template<class T>
PLDHashOperator appendElementToArray(T* aData, void* aArray)
{
  nsIMutableArray *array = (nsIMutableArray*)aArray;
  nsresult rv;
  nsCOMPtr<nsISupports> supports = do_QueryInterface(aData, &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  rv = array->AppendElement(aData, false);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

template<class T>
PLDHashOperator sbDeviceFirmwareUpdater::EnumerateIntoArrayISupportsKey(
  nsISupports* aKey,
  T* aData,
  void* aArray)
{
  TRACE(("sbDeviceFirmwareUpdater[0x%x] - EnumerateIntoArray (nsISupports)"));
  return appendElementToArray(aData, aArray);
}

// ----------------------------------------------------------------------------
// nsIObserver
// ----------------------------------------------------------------------------
NS_IMETHODIMP
sbDeviceFirmwareUpdater::Observe(nsISupports* aSubject,
                                 const char* aTopic,
                                 const PRUnichar* aData)
{
  LOG(("[sbDeviceFirmwareUpdater] - Observe: %s", this, aTopic));

  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (strcmp(aTopic, SB_LIBRARY_MANAGER_SHUTDOWN_TOPIC) == 0) {
    rv = observerService->RemoveObserver(this, SB_LIBRARY_MANAGER_SHUTDOWN_TOPIC);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to remove shutdown observer");

    rv = Shutdown();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

// ----------------------------------------------------------------------------
// sbDeviceFirmwareHandlerStatus
// ----------------------------------------------------------------------------
  
sbDeviceFirmwareHandlerStatus::sbDeviceFirmwareHandlerStatus()
: mMonitor(nsnull)
, mOperation(OP_NONE)
, mStatus(STATUS_NONE)
{
}

sbDeviceFirmwareHandlerStatus::~sbDeviceFirmwareHandlerStatus()
{
  if(mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
  }
}

nsresult 
sbDeviceFirmwareHandlerStatus::Init()
{
  mMonitor = nsAutoMonitor::NewMonitor("sbDeviceFirmwareHandlerStatus::mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult 
sbDeviceFirmwareHandlerStatus::GetOperation(handleroperation_t *aOperation)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aOperation);

  nsAutoMonitor mon(mMonitor);
  *aOperation = mOperation;

  return NS_OK;
}

nsresult 
sbDeviceFirmwareHandlerStatus::SetOperation(handleroperation_t aOperation)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);
  mOperation = aOperation;

  return NS_OK;
}

nsresult 
sbDeviceFirmwareHandlerStatus::GetStatus(handlerstatus_t *aStatus)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aStatus);
  
  nsAutoMonitor mon(mMonitor);
  *aStatus = mStatus;

  return NS_OK;
}

nsresult 
sbDeviceFirmwareHandlerStatus::SetStatus(handlerstatus_t aStatus)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  
  nsAutoMonitor mon(mMonitor);
  mStatus = aStatus;

  return NS_OK;
}

// -----------------------------------------------------------------------------
// sbDeviceFirmwareUpdaterRunner
// -----------------------------------------------------------------------------
NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceFirmwareUpdaterRunner,
                              nsIRunnable);

sbDeviceFirmwareUpdaterRunner::sbDeviceFirmwareUpdaterRunner()
: mRecovery(PR_FALSE)
, mFirmwareUpdateNeedsCaching(PR_FALSE)
{
}

/*virtual*/
sbDeviceFirmwareUpdaterRunner::~sbDeviceFirmwareUpdaterRunner()
{
}

nsresult
sbDeviceFirmwareUpdaterRunner::Init(sbIDevice *aDevice,
                                    sbIDeviceFirmwareUpdate *aUpdate, 
                                    sbIDeviceFirmwareHandler *aHandler,                                    
                                    PRBool aRecovery /*= PR_FALSE*/,
                                    PRBool aFirmwareUpdateNeedsCaching /*= PR_FALSE*/)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aUpdate);
  NS_ENSURE_ARG_POINTER(aHandler);

  mDevice = aDevice;
  mFirmwareUpdate = aUpdate;
  mHandler = aHandler;

  mRecovery = aRecovery;
  mFirmwareUpdateNeedsCaching = aFirmwareUpdateNeedsCaching;

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceFirmwareUpdaterRunner::Run()
{
  NS_ENSURE_STATE(mHandler);
  NS_ENSURE_STATE(mFirmwareUpdate);

  nsresult rv = NS_ERROR_UNEXPECTED;

  if(mRecovery) {
    rv = mHandler->Recover(mFirmwareUpdate);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = mHandler->Update(mFirmwareUpdate);
    NS_ENSURE_SUCCESS(rv, rv);
  }


  return NS_OK;
}
