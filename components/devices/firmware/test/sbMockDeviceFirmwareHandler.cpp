/* vim: set sw=2 :miv */
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

#include "sbMockDeviceFirmwareHandler.h"

#include <nsIPrefBranch.h>
#include <nsIPrefService.h>
#include <nsIVariant.h>
#include <nsIWritablePropertyBag2.h>

#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsISupportsUtils.h>
#include <nsIVariant.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOMCIDInternal.h>

#include <sbIDevice.h>
#include <sbIDeviceProperties.h>

NS_IMPL_ISUPPORTS_INHERITED0(sbMockDeviceFirmwareHandler, 
                             sbBaseDeviceFirmwareHandler)

SB_DEVICE_FIRMWARE_HANLDER_REGISTERSELF_IMPL(sbMockDeviceFirmwareHandler,
                                             "Songbird Device Firmware Tester - Mock Device Firmware Handler")

sbMockDeviceFirmwareHandler::sbMockDeviceFirmwareHandler()
{
}

sbMockDeviceFirmwareHandler::~sbMockDeviceFirmwareHandler()
{
}

/*virtual*/ nsresult 
sbMockDeviceFirmwareHandler::OnInit()
{
  mContractId = 
    NS_LITERAL_STRING("@songbirdnest.com/Songbird/Device/Firmware/Handler/MockDevice;1");

  return NS_OK;
}

/*virtual*/ nsresult 
sbMockDeviceFirmwareHandler::OnCanUpdate(sbIDevice *aDevice, 
                                         PRBool *_retval)
{
  *_retval = PR_FALSE;

  nsCOMPtr<sbIDeviceProperties> properties;
  nsresult rv = aDevice->GetProperties(getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString vendorName;
  rv = properties->GetVendorName(vendorName);
  NS_ENSURE_SUCCESS(rv, rv);

  if(!vendorName.EqualsLiteral("ACME Inc.")) {
    return NS_OK;
  }

  nsCOMPtr<nsIVariant> modelNumber;
  rv = properties->GetModelNumber(getter_AddRefs(modelNumber));
  NS_ENSURE_SUCCESS(rv, rv);

  // Yep, supported!
  *_retval = PR_TRUE;

  return NS_OK;
}

/*virtual*/ nsresult
sbMockDeviceFirmwareHandler::OnRefreshInfo(sbIDevice *aDevice, 
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
sbMockDeviceFirmwareHandler::OnUpdate(sbIDevice *aDevice, 
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
sbMockDeviceFirmwareHandler::OnVerifyDevice(sbIDevice *aDevice, 
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
sbMockDeviceFirmwareHandler::OnVerifyUpdate(sbIDevice *aDevice, 
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
