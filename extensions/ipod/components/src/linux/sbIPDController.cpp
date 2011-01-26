/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://www.songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
// 
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
//=END SONGBIRD GPL
*/

/** 
 * \file  sbIPDController.cpp
 * \brief Songbird iPod Device Controller Source.
 */

//------------------------------------------------------------------------------
//
// iPod device controller imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbIPDController.h"

// Local imports.
#include "sbIPDLog.h"
#include "sbIPDMarshall.h"
#include "sbIPDSysDevice.h"
#include "sbIPDUtils.h"

// Songbird imports.
#include <sbDeviceCompatibility.h>

// Mozilla imports.
#include <nsAutoPtr.h>
#include <nsIClassInfoImpl.h>
#include <nsIGenericFactory.h>
#include <nsIProgrammingLanguage.h>
#include <nsIPropertyBag.h>
#include <nsIVariant.h>
#include <nsIWritablePropertyBag.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>

#include <sbDebugUtils.h>

//------------------------------------------------------------------------------
//
// iPod device controller component module implementation.
//
//------------------------------------------------------------------------------

SB_DEVICE_CONTROLLER_REGISTERSELF_IMPL(sbIPDController, 
                                       SB_IPDCONTROLLER_CONTRACTID)


//------------------------------------------------------------------------------
//
// iPod device controller nsISupports and nsIClassInfo implementation.
//
//------------------------------------------------------------------------------

// nsISupports implementation.
// NS_IMPL_THREADSAFE_ISUPPORTS1_CI(sbIPDController, sbIDeviceController)
NS_IMPL_THREADSAFE_ADDREF(sbIPDController)
NS_IMPL_THREADSAFE_RELEASE(sbIPDController)
NS_IMPL_QUERY_INTERFACE1_CI(sbIPDController, sbIDeviceController)
NS_IMPL_CI_INTERFACE_GETTER1(sbIPDController, sbIDeviceController)

// nsIClassInfo implementation.
NS_IMPL_THREADSAFE_CI(sbIPDController)


//------------------------------------------------------------------------------
//
// iPod device controller sbIDeviceController implementation.
//
//------------------------------------------------------------------------------

/**
 * Given a set of device parameters, attempt to determine if the device
 * is supported, and with what level of functionality.
 */

NS_IMETHODIMP
sbIPDController::GetCompatibility(nsIPropertyBag*          aParams,
                                  sbIDeviceCompatibility** _retval)
{
  // Validate parameters.
  NS_ENSURE_ARG_POINTER(aParams);
  NS_ENSURE_ARG_POINTER(_retval);

  // Function variables.
  nsresult rv;

  // Create the device compatibility object.
  nsRefPtr<sbDeviceCompatibility> deviceCompatibility;
  NS_NEWXPCOM(deviceCompatibility, sbDeviceCompatibility);
  NS_ENSURE_TRUE(deviceCompatibility, NS_ERROR_OUT_OF_MEMORY);

  // Get the device type.
  nsCOMPtr<nsIVariant> property;
  nsAutoString deviceType;
  rv = aParams->GetProperty(NS_LITERAL_STRING("DeviceType"),
                            getter_AddRefs(property));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = property->GetAsAString(deviceType);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set up the device compatibility.
  if (deviceType.EqualsLiteral("iPod")) {
    rv = deviceCompatibility->Init
           (sbIDeviceCompatibility::COMPATIBLE_ENHANCED_SUPPORT,
            sbIDeviceCompatibility::PREFERENCE_SELECTED);
  } else {
    rv = deviceCompatibility->Init
           (sbIDeviceCompatibility::INCOMPATIBLE,
            sbIDeviceCompatibility::PREFERENCE_UNKNOWN);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  NS_ADDREF(*_retval = deviceCompatibility);

  return NS_OK;
}


/**
 * Constructs a device based on the given parameters.
 */

NS_IMETHODIMP
sbIPDController::CreateDevice(nsIPropertyBag* aParams,
                              sbIDevice**     _retval)
{
  // Validate parameters.
  NS_ENSURE_ARG_POINTER(aParams);
  NS_ENSURE_ARG_POINTER(_retval);

  // Function variables.
  nsresult rv;

  LOG("sbIPDController::CreateDevice");

  // Get the controller ID.
  nsID controllerID;
  rv = GetControllerIdInternal(controllerID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create and initialize the device object.
  nsRefPtr<sbIPDSysDevice> device = new sbIPDSysDevice(controllerID, aParams);
  NS_ENSURE_TRUE(device, NS_ERROR_OUT_OF_MEMORY);
  rv = device->Initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the device to the list of controlled devices.
  rv = AddDeviceInternal(device);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  NS_ADDREF(*_retval = device);

  return NS_OK;
}


/**
 * Determines if a device is currently controlled by the controller
 */

NS_IMETHODIMP
sbIPDController::ControlsDevice(sbIDevice* aDevice,
                                PRBool*    _retval)
{
  return ControlsDeviceInternal(aDevice, _retval);
}


/**
 * Connects all devices.
 */

NS_IMETHODIMP
sbIPDController::ConnectDevices()
{
  return ConnectDevicesInternal();
}


/**
 * Disconnects all devices.
 */

NS_IMETHODIMP
sbIPDController::DisconnectDevices()
{
  return DisconnectDevicesInternal();
}


/**
 * Called when the controller should release a single device.
 */

NS_IMETHODIMP
sbIPDController::ReleaseDevice(sbIDevice* aDevice)
{
  return ReleaseDeviceInternal(aDevice);
}


/**
 * Called when the controller should release all registered devices.
 */

NS_IMETHODIMP
sbIPDController::ReleaseDevices()
{
  return ReleaseDevicesInternal();
}


//
// Getters/setters.
//

/**
 * Unique identifier for the controller.
 */

NS_IMETHODIMP
sbIPDController::GetId(nsID** aId)
{
  // Validate parameters.
  NS_ENSURE_ARG_POINTER(aId);

  // Function variables.
  nsresult rv;

  // Allocate an nsID and set it up for auto-disposal.
  nsID* pId = static_cast<nsID*>(NS_Alloc(sizeof(nsID)));
  NS_ENSURE_TRUE(pId, NS_ERROR_OUT_OF_MEMORY);
  sbAutoNSMemPtr autoPId(pId);

  // Get the controller ID.
  rv = GetControllerIdInternal(*pId);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  *aId = static_cast<nsID*>(autoPId.forget());

  return NS_OK;
}


/**
 * A human readable name.
 */

NS_IMETHODIMP
sbIPDController::GetName(nsAString& aName)
{
  return GetControllerNameInternal(aName);
}


/**
 * The nsID of an sbIDeviceMarshall implementor that the controller wishes to
 * use. This must be valid or the controller will never be instantiated!
 */

NS_IMETHODIMP
sbIPDController::GetMarshallId(nsID** aId)
{
  // Validate parameters.
  NS_ENSURE_ARG_POINTER(aId);

  // Function variables.
  nsresult rv;

  // Allocate an nsID and set it up for auto-disposal.
  nsID* pId = static_cast<nsID*>(NS_Alloc(sizeof(nsID)));
  NS_ENSURE_TRUE(pId, NS_ERROR_OUT_OF_MEMORY);
  sbAutoNSMemPtr autoPId(pId);

  // Get the controller ID.
  rv = GetMarshallIdInternal(*pId);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  *aId = static_cast<nsID*>(autoPId.forget());

  return NS_OK;
}


/**
 * List of devices that are currently in-use.
 * Ordering is not guaranteed to be consistent between
 * reads!
 */

NS_IMETHODIMP
sbIPDController::GetDevices(nsIArray** aDevices)
{
  return GetDevicesInternal(aDevices);
}


//------------------------------------------------------------------------------
//
// iPod device controller public services.
//
//------------------------------------------------------------------------------

/**
 * Construct an iPod device controller object.
 */

sbIPDController::sbIPDController()
{
  nsresult rv;

  SB_PRLOG_SETUP(sbIPDController);

  // Initialize the logging services.
  sbIPDLog::Initialize();

  // Set the controller ID.
  static nsID const controllerID = SB_IPDCONTROLLER_CID;
  rv = SetControllerIdInternal(controllerID);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to set controller ID.");

  // Set the marshall ID.
  static nsID const marshallID = SB_IPDMARSHALL_CID;
  rv = SetMarshallIdInternal(marshallID);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to set marshall ID.");

  // Set the controller name.
  rv = SetControllerNameInternal(NS_LITERAL_STRING(SB_IPDCONTROLLER_CLASSNAME));
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to set controller name.");
}


/**
 * Destroy an iPod device controller object.
 */

sbIPDController::~sbIPDController()
{
}


