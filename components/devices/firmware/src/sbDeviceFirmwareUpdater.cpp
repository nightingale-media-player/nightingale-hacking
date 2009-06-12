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

#include "sbDeviceFirmwareUpdater.h"

#include <nsISupportsPrimitives.h>

#include <sbIDevice.h>
#include <sbIDeviceEvent.h>
#include <sbIDeviceEventTarget.h>
#include <sbIDeviceFirmwareHandler.h>

#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <prlog.h>

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

NS_IMPL_THREADSAFE_ISUPPORTS2(sbDeviceFirmwareUpdater,
                              sbIDeviceFirmwareUpdater,
                              sbIDeviceEventListener)

sbDeviceFirmwareUpdater::sbDeviceFirmwareUpdater()
: mMonitor(nsnull)
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

  mRunningHandlers.Clear();
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

  success = mHandlerStatus.Init(MIN_RUNNING_HANDLERS);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIEventTarget> threadPool = 
    do_GetService("@songbirdnest.com/Songbird/ThreadPoolService;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  threadPool.swap(mThreadPool);

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
                                           sbIDeviceEventListener *aListener, 
                                           PRBool aCreate)
{
  NS_ENSURE_TRUE(aDevice, nsnull);

  sbIDeviceFirmwareHandler *_retval = nsnull;

  nsCOMPtr<sbIDeviceFirmwareHandler> handler;
  if(!mRunningHandlers.Get(aDevice, getter_AddRefs(handler)) && aCreate) {
    nsresult rv = GetHandler(aDevice, getter_AddRefs(handler));
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

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::CheckForUpdate(sbIDevice *aDevice, 
                                        sbIDeviceEventListener *aListener)
{
  LOG(("[sbDeviceFirmwareUpdater] - CheckForUpdate"));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<sbIDeviceFirmwareHandler> handler = 
    GetRunningHandler(aDevice, aListener, PR_TRUE);

  // XXXAus: Check to make sure it's not busy right now?

  nsCOMPtr<sbIDeviceEventTarget> eventTarget = do_QueryInterface(aDevice, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = eventTarget->AddEventListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = PutRunningHandler(aDevice, handler);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoMonitor mon(mMonitor);
  
  sbDeviceFirmwareHandlerStatus *handlerStatus = GetHandlerStatus(handler);
  NS_ENSURE_TRUE(handlerStatus, NS_ERROR_OUT_OF_MEMORY);

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
  NS_ENSURE_ARG_POINTER(aDevice);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::VerifyUpdate(sbIDevice *aDevice, 
                                      sbIDeviceFirmwareUpdate *aFirmwareUpdate, 
                                      sbIDeviceEventListener *aListener)
{
  LOG(("[sbDeviceFirmwareUpdater] - VerifyUpdate"));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
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
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aFirmwareUpdate);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbDeviceFirmwareUpdater::AutoUpdate(sbIDevice *aDevice,
                                    sbIDeviceEventListener *aListener)
{
  LOG(("[sbDeviceFirmwareUpdater] - AutoUpdate"));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);
  
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<sbIDeviceFirmwareHandler> handler = 
    GetRunningHandler(aDevice, aListener, PR_TRUE);

  // XXXAus: Check to make sure it's not busy right now?

  nsAutoMonitor mon(mMonitor);

  rv = CheckForUpdate(aDevice, aListener);
  NS_ENSURE_SUCCESS(rv, rv);

  sbDeviceFirmwareHandlerStatus *handlerStatus = GetHandlerStatus(handler);
  NS_ENSURE_TRUE(handlerStatus, NS_ERROR_OUT_OF_MEMORY);

  handlerStatus->IsAutoUpdate(PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::VerifyDevice(sbIDevice *aDevice, 
                                      sbIDeviceEventListener *aListener)
{
  LOG(("[sbDeviceFirmwareUpdater] - VerifyDevice"));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::RegisterHandler(sbIDeviceFirmwareHandler *aFirmwareHandler)
{
  LOG(("[sbDeviceFirmwareUpdater] - RegisterHandler"));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
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
                                    PRBool *_retval)
{
  LOG(("[sbDeviceFirmwareUpdater] - HasHandler"));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIDeviceFirmwareHandler> handler;
  nsresult rv = GetHandler(aDevice, getter_AddRefs(handler));
  
  *_retval = PR_FALSE;
  if(NS_SUCCEEDED(rv)) {
    *_retval = PR_TRUE;
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::GetHandler(sbIDevice *aDevice, 
                                    sbIDeviceFirmwareHandler **_retval)
{
  LOG(("[sbDeviceFirmwareUpdater] - GetHandler"));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
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
    rv = handler->CanUpdate(aDevice, &canHandleDevice);
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
  NS_ENSURE_ARG_POINTER(aDevice);

  nsAutoMonitor mon(mMonitor);
  
  nsCOMPtr<sbIDeviceFirmwareHandler> handler = GetRunningHandler(aDevice);

  if(handler) {
    nsresult rv = handler->Cancel();
    NS_ENSURE_SUCCESS(rv, rv);

    mRunningHandlers.Remove(aDevice);
  }

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

  switch(operation) {
    case sbDeviceFirmwareHandlerStatus::OP_REFRESH: {
      LOG(("[sbDeviceFirmwareUpdater] - OP_REFRESH"));

      if(eventType == sbIDeviceEvent::EVENT_FIRMWARE_CFU_START &&
         status == sbDeviceFirmwareHandlerStatus::STATUS_WAITING_FOR_START) {
        rv = handlerStatus->SetStatus(sbDeviceFirmwareHandlerStatus::STATUS_RUNNING);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if(eventType == sbIDeviceEvent::EVENT_FIRMWARE_CFU_END &&
              status == sbDeviceFirmwareHandlerStatus::STATUS_RUNNING) {
        rv = handlerStatus->SetStatus(sbDeviceFirmwareHandlerStatus::STATUS_FINISHED);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        // XXXAus: Abort!
      }
    }
    break;

    case sbDeviceFirmwareHandlerStatus::OP_DOWNLOAD: {
      LOG(("[sbDeviceFirmwareUpdater] - OP_DOWNLOAD"));
    }
    break;

    case sbDeviceFirmwareHandlerStatus::OP_UPDATE: {
      LOG(("[sbDeviceFirmwareUpdater] - OP_UPDATE"));
    }
    break;

    default:
      NS_WARNING("Unsupported operation");
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
, mIsAutoUpdate(PR_FALSE)
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

PRBool 
sbDeviceFirmwareHandlerStatus::IsAutoUpdate()
{
  NS_ENSURE_TRUE(mMonitor, PR_FALSE);
  nsAutoMonitor mon(mMonitor);
  
  return mIsAutoUpdate;
}

void 
sbDeviceFirmwareHandlerStatus::IsAutoUpdate(PRBool aAutoUpdate)
{
  NS_ENSURE_TRUE(mMonitor, /*void*/);
  
  nsAutoMonitor mon(mMonitor);
  mIsAutoUpdate = aAutoUpdate;

  return;
}
