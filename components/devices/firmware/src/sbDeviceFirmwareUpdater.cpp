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

#include <sbIDeviceFirmwareHandler.h>

#include <nsAutoLock.h>
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

// XXX WARNING XXX 
// If you change this value here, you have to change it 
// in sbBaseDeviceFirmwareHandler.cpp as well!
#define SB_DEVICE_FIRMWARE_HANLDER_CONTRACTID_ROOT \
  "@songbirdnest.com/Songbird/Device/Firmware/Handler/"

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceFirmwareUpdater, 
                              sbIDeviceFirmwareUpdater)

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

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::CheckForUpdate(sbIDevice *aDevice, 
                                        sbIDeviceEventListener *aListener)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::DownloadUpdate(sbIDevice *aDevice, 
                                        PRBool aVerifyFirmwareUpdate, 
                                        sbIDeviceEventListener *aListener)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::VerifyUpdate(sbIDevice *aDevice, 
                                      sbIDeviceFirmwareUpdate *aFirmwareUpdate, 
                                      sbIDeviceEventListener *aListener)
{
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
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aFirmwareUpdate);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::VerifyDevice(sbIDevice *aDevice, 
                                      sbIDeviceEventListener *aListener)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::RegisterHandler(sbIDeviceFirmwareHandler *aFirmwareHandler)
{
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
