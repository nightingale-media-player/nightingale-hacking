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

#include "sbBaseDeviceFirmwareHandler.h"

#include <nsIIOService.h>

#include <nsAutoLock.h>
#include <nsServiceManagerUtils.h>

#include <sbProxiedComponentManager.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbBaseDeviceFirmwareHandler, 
                              sbIDeviceFirmwareHandler)

sbBaseDeviceFirmwareHandler::sbBaseDeviceFirmwareHandler()
: mMonitor(nsnull)
, mFirmwareVersion(0)
{
}

sbBaseDeviceFirmwareHandler::~sbBaseDeviceFirmwareHandler()
{
  if(mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
  }
}

nsresult
sbBaseDeviceFirmwareHandler::Init()
{
  mMonitor = nsAutoMonitor::NewMonitor("sbBaseDeviceFirmwareHandler::mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = OnInit();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDeviceFirmwareHandler::CreateProxiedURI(const nsACString &aURISpec,
                                              nsIURI **aURI)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aURI);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIIOService> ioService = 
    do_ProxiedGetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  rv = ioService->NewURI(aURISpec, 
                         nsnull, 
                         nsnull, 
                         getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIThread> mainThread;
  rv = NS_GetMainThread(getter_AddRefs(mainThread));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = do_GetProxyForObject(mainThread,
                            NS_GET_IID(nsIURI),
                            uri, 
                            NS_PROXY_ALWAYS | NS_PROXY_SYNC,
                            (void **) aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*virtual*/ nsresult 
sbBaseDeviceFirmwareHandler::OnInit()
{
  /**
   * Here is where you will want to initialize yourself for the first time.
   * This should include setting the following member variables to the
   * the values you need: mContractId.
   *
   * You should end up with a string that looks something like this:
   * "@yourdomain.com/Songbird/Device/Firmware/Handler/Acme Portable Player 900x"
   *
   * The other values only need to be set when OnRefreshInfo is called.
   * These values are: mFirmwareVersion, mReadableFirmwareVersion, mFirmwareLocation
   * mResetInstructionsLocation and mReleaseNotesLocation.
   *
   * Events must be sent to both the device and the listener (if it is specified 
   * during the call).
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult 
sbBaseDeviceFirmwareHandler::OnCanUpdate(sbIDevice *aDevice, 
                                         PRBool *_retval)
{
  /**
   * Here is where you will want to verify the incoming sbIDevice object
   * to determine if your handler can support updating the firmware on
   * the device. _retval should be set to either PR_TRUE (yes, can update) 
   * or PR_FALSE (no, cannot update).
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult
sbBaseDeviceFirmwareHandler::OnRefreshInfo(sbIDevice *aDevice, 
                                           sbIDeviceEventListener *aListener)
{
  /**
   * Here is where you will want to refresh the info for your handler. 
   * This includes the latest firmware version, firmware location, reset instructions
   * and release notes locations.
   *
   * Always use CreateProxiedURI when creating the nsIURIs for the firmware, 
   * reset instructions and release notes location. This will ensure
   * that the object is thread-safe and it is created in a thread-safe manner.
   *
   * This method must be asynchronous and should not block the main thread. 
   * Progress for this operation is also expected. The flow of expected events 
   * is as follows: firmware refresh info start, firmware refresh info progress,
   * firmware refresh info end. See sbIDeviceEvent for more information about
   * event payload.
   *
   * Events must be sent to both the device and the listener (if it is specified 
   * during the call).
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult
sbBaseDeviceFirmwareHandler::OnUpdate(sbIDevice *aDevice, 
                                      sbIDeviceFirmwareUpdate *aFirmwareUpdate, 
                                      sbIDeviceEventListener *aListener)
{
  /**
   * Here is where you will want to actually perform the firmware update
   * on the device. The firmware update object will contain the local 
   * location for the firmware image. It also contains the version of the 
   * firmware image. 
   *
   * The implementation of this method must be asynchronous and not block
   * the main thread. The flow of expected events is as follows:
   * firmware update start, firmware write start, firmware write progress, 
   * firmware write end, firmware verify start, firmware verify progress, 
   * firmware verify end, firmware update end.
   *
   * See sbIDeviceEvent for more infomation about event payload.
   *
   * Events must be sent to both the device and the listener (if it is specified 
   * during the call).
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult
sbBaseDeviceFirmwareHandler::OnVerifyDevice(sbIDevice *aDevice, 
                                            sbIDeviceEventListener *aListener)
{
  /**
   * Here is where you will want to verify the firmware on the device itself
   * to ensure that it is not corrupt. Whichever method you use will most likely
   * be device specific.
   * 
   * The implementation of this method must be asynchronous and not block
   * the main thread. The flow of expected events is as follows:
   * firmware verify start, firmware verify progress, firmware verify end.
   *
   * If any firmware verify error events are sent during the process
   * the firmware is considered corrupted.
   *
   * See sbIDeviceEvent for more infomation about event payload.
   *
   * Events must be sent to both the device and the listener (if it is specified 
   * during the call).
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult
sbBaseDeviceFirmwareHandler::OnVerifyUpdate(sbIDevice *aDevice, 
                                            sbIDeviceFirmwareUpdate *aFirmwareUpdate, 
                                            sbIDeviceEventListener *aListener)
{
  /**
   * Here is where you should provide a way to verify the firmware update
   * image itself to make sure that it is not corrupt in any way.
   *
   * The implementation of this method must be asynchronous and not block
   * the main thread. The flow of expected events is as follows:
   * firmware image verify start, firmware image verify progress, firmware 
   * image verify end.
   *
   * If any firmware image verify error events are sent during the process
   * the firmware image is considered corrupted.
   *
   * See sbIDeviceEvent for more infomation about event payload. 
   *
   * Events must be sent to both the device and the listener (if it is specified 
   * during the call).
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbBaseDeviceFirmwareHandler::GetContractId(nsAString & aContractId)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);
  aContractId = mContractId;

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseDeviceFirmwareHandler::GetLatestFirmwareLocation(nsIURI * *aLatestFirmwareLocation)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aLatestFirmwareLocation);

  nsAutoMonitor mon(mMonitor);
  
  if(!mFirmwareLocation) {
    *aLatestFirmwareLocation = nsnull;
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsresult rv = mFirmwareLocation->Clone(aLatestFirmwareLocation);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseDeviceFirmwareHandler::GetLatestFirmwareVersion(PRUint32 *aLatestFirmwareVersion)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aLatestFirmwareVersion);

  nsAutoMonitor mon(mMonitor);
  *aLatestFirmwareVersion = mFirmwareVersion;
  
  return NS_OK;
}

NS_IMETHODIMP 
sbBaseDeviceFirmwareHandler::GetLatestFirmwareReadableVersion(nsAString & aLatestFirmwareReadableVersion)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);
  aLatestFirmwareReadableVersion = mReadableFirmwareVersion;
  
  return NS_OK;
}

NS_IMETHODIMP 
sbBaseDeviceFirmwareHandler::GetReleaseNotesLocation(nsIURI * *aReleaseNotesLocation)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aReleaseNotesLocation);

  nsAutoMonitor mon(mMonitor);
  
  if(!mReleaseNotesLocation) {
    *aReleaseNotesLocation = nsnull;
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsresult rv = mReleaseNotesLocation->Clone(aReleaseNotesLocation);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseDeviceFirmwareHandler::GetResetInstructionsLocation(nsIURI * *aResetInstructionsLocation)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aResetInstructionsLocation);

  nsAutoMonitor mon(mMonitor);
  
  if(!mResetInstructionsLocation) {
    *aResetInstructionsLocation = nsnull;
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsresult rv = mResetInstructionsLocation->Clone(aResetInstructionsLocation);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseDeviceFirmwareHandler::CanUpdate(sbIDevice *aDevice, 
                                       PRBool *_retval)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(_retval);

  nsAutoMonitor mon(mMonitor);

  nsresult rv = OnCanUpdate(aDevice, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseDeviceFirmwareHandler::RefreshInfo(sbIDevice *aDevice, 
                                         sbIDeviceEventListener *aListener)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);

  nsAutoMonitor mon(mMonitor);

  nsresult rv = OnRefreshInfo(aDevice, aListener);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseDeviceFirmwareHandler::Update(sbIDevice *aDevice, 
                                    sbIDeviceFirmwareUpdate *aFirmwareUpdate, 
                                    sbIDeviceEventListener *aListener)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aFirmwareUpdate);

  nsAutoMonitor mon(mMonitor);

  nsresult rv = OnUpdate(aDevice, aFirmwareUpdate, aListener);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseDeviceFirmwareHandler::VerifyDevice(sbIDevice *aDevice, 
                                          sbIDeviceEventListener *aListener)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);

  nsAutoMonitor mon(mMonitor);

  nsresult rv = OnVerifyDevice(aDevice, aListener);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseDeviceFirmwareHandler::VerifyUpdate(sbIDevice *aDevice, 
                                          sbIDeviceFirmwareUpdate *aFirmwareUpdate, 
                                          sbIDeviceEventListener *aListener)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);

  nsAutoMonitor mon(mMonitor);

  nsresult rv = OnVerifyUpdate(aDevice, aFirmwareUpdate, aListener);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
