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

#include "sbCDDeviceController.h"

#include "sbCDDevice.h"
#include "sbCDDeviceDefines.h"

#include <sbDeviceCompatibility.h>

#include <nsIClassInfoImpl.h>
#include <nsIGenericFactory.h>
#include <nsIProgrammingLanguage.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsIPropertyBag2.h>


SB_DEVICE_CONTROLLER_REGISTERSELF_IMPL(sbCDDeviceController,
                                       SB_CDDEVICE_CONTROLLER_CONTRACTID)

NS_IMPL_THREADSAFE_ADDREF(sbCDDeviceController)
NS_IMPL_THREADSAFE_RELEASE(sbCDDeviceController)
NS_IMPL_QUERY_INTERFACE1_CI(sbCDDeviceController, sbIDeviceController)
NS_IMPL_CI_INTERFACE_GETTER1(sbCDDeviceController, sbIDeviceController)

// nsIClassInfo implementation.
NS_DECL_CLASSINFO(sbCDDeviceController)
NS_IMPL_THREADSAFE_CI(sbCDDeviceController)

sbCDDeviceController::sbCDDeviceController()
{
  static nsID const id = SB_CDDEVICE_CONTROLLER_CID;
  nsresult rv = SetControllerIdInternal(id);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to set controller id");

  static nsID const marshallId = SB_CDDEVICE_MARSHALL_CID;
  rv = SetMarshallIdInternal(marshallId);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to set controller id");

  rv = SetControllerNameInternal(NS_LITERAL_STRING("sbCDDeviceController"));
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to set controller name");
}

sbCDDeviceController::~sbCDDeviceController()
{
}

//------------------------------------------------------------------------------
// sbIDeviceController

NS_IMETHODIMP
sbCDDeviceController::GetCompatibility(nsIPropertyBag *aParams,
                                       sbIDeviceCompatibility **aRetVal)
{
  NS_ENSURE_ARG_POINTER(aParams);
  NS_ENSURE_ARG_POINTER(aRetVal);

  nsresult rv;

  // Create the device compatibility object.
  nsRefPtr<sbDeviceCompatibility> deviceCompatibility;
  NS_NEWXPCOM(deviceCompatibility, sbDeviceCompatibility);
  NS_ENSURE_TRUE(deviceCompatibility, NS_ERROR_OUT_OF_MEMORY);

  // Get the device type.
  nsCOMPtr<nsIVariant> property;
  rv = aParams->GetProperty(NS_LITERAL_STRING("DeviceType"),
                            getter_AddRefs(property));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString deviceType;
  rv = property->GetAsAString(deviceType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (deviceType.EqualsLiteral("CD")) {
    rv = deviceCompatibility->Init(
        sbIDeviceCompatibility::COMPATIBLE_ENHANCED_SUPPORT,
        sbIDeviceCompatibility::PREFERENCE_SELECTED);
  }
  else {
    rv = deviceCompatibility->Init(sbIDeviceCompatibility::INCOMPATIBLE,
                                   sbIDeviceCompatibility::PREFERENCE_UNKNOWN);
  }

  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  NS_ADDREF(*aRetVal = deviceCompatibility);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDeviceController::CreateDevice(nsIPropertyBag *aParams,
                                   sbIDevice **aRetVal)
{
  NS_ENSURE_ARG_POINTER(aParams);
  NS_ENSURE_ARG_POINTER(aRetVal);

  nsID id;
  nsresult rv = GetControllerIdInternal(id);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbCDDevice> cdDevice;
  rv = sbCDDevice::New(id, aParams, getter_AddRefs(cdDevice));
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Add the device to the internal list
  rv = AddDeviceInternal(cdDevice);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return the new CD device.
  NS_ADDREF(*aRetVal = cdDevice);

  return NS_OK;
}

NS_IMETHODIMP
sbCDDeviceController::ControlsDevice(sbIDevice *aDevice, PRBool *aRetVal)
{
  return ControlsDeviceInternal(aDevice, aRetVal);
}

NS_IMETHODIMP
sbCDDeviceController::ConnectDevices()
{
  return ConnectDevicesInternal();
}

NS_IMETHODIMP
sbCDDeviceController::DisconnectDevices()
{
  return DisconnectDevicesInternal();
}

NS_IMETHODIMP
sbCDDeviceController::ReleaseDevice(sbIDevice *aDevice)
{
  return ReleaseDeviceInternal(aDevice);
}

NS_IMETHODIMP
sbCDDeviceController::ReleaseDevices()
{
  return ReleaseDevicesInternal();
}

NS_IMETHODIMP
sbCDDeviceController::GetId(nsID **aId)
{
  NS_ENSURE_ARG_POINTER(aId);

  *aId = nsnull;
  *aId = static_cast<nsID *>(NS_Alloc(sizeof(nsID)));
  nsresult rv = GetControllerIdInternal(**aId);

  if (NS_FAILED(rv)) {
    NS_Free(*aId);
    *aId = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbCDDeviceController::GetName(nsAString & aName)
{
  return GetControllerNameInternal(aName);
}

NS_IMETHODIMP
sbCDDeviceController::GetMarshallId(nsID **aId)
{
  NS_ENSURE_ARG_POINTER(aId);

  *aId = nsnull;
  *aId = static_cast<nsID *>(NS_Alloc(sizeof(nsID)));
  nsresult rv = GetMarshallIdInternal(**aId);

  if (NS_FAILED(rv)) {
    NS_Free(*aId);
    *aId = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbCDDeviceController::GetDevices(nsIArray **aDevices)
{
  return GetDevicesInternal(aDevices);
}

