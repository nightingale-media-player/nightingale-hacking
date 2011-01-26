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
 * \file  sbIPDProperties.cpp
 * \brief Songbird iPod Device Properties Source.
 */

//------------------------------------------------------------------------------
//
// iPod device properties imported services.
//
//------------------------------------------------------------------------------

// Self import.
#include "sbIPDProperties.h"

// Local imports.
#include "sbIPDDevice.h"

// Songbird imports.
#include <sbIDeviceEvent.h>
#include <sbStandardDeviceProperties.h>


//------------------------------------------------------------------------------
//
// iPod device properties nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ADDREF(sbIPDProperties)
NS_IMPL_THREADSAFE_RELEASE(sbIPDProperties)
NS_INTERFACE_MAP_BEGIN(sbIPDProperties)
  NS_INTERFACE_MAP_ENTRY(sbIDeviceProperties)
  NS_INTERFACE_MAP_ENTRY(nsIWritablePropertyBag)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIPropertyBag, nsIWritablePropertyBag)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWritablePropertyBag)
  NS_INTERFACE_MAP_ENTRY(nsIPropertyBag2)
  NS_INTERFACE_MAP_ENTRY(nsIWritablePropertyBag2)
NS_INTERFACE_MAP_END


//------------------------------------------------------------------------------
//
// iPod device properties sbIDeviceProperties implementation.
//
//------------------------------------------------------------------------------

/**
 * \brief Initialize Friendly Name property from a device
 */

NS_IMETHODIMP
sbIPDProperties::InitFriendlyName(const nsAString& aFriendlyName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 * \brief Initialize Default Name property for a device
 */

NS_IMETHODIMP
sbIPDProperties::InitDefaultName(const nsAString& aDefaultName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


/**
 * \brief Initialize Vendor Name propery for a device
 */

NS_IMETHODIMP
sbIPDProperties::InitVendorName(const nsAString& aVendorName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


/**
 * \brief Initialize Model Number propery for a device
 */

NS_IMETHODIMP
sbIPDProperties::InitModelNumber(nsIVariant* aModelNumber)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


/**
 * \brief Initialize Serial Number propery for a device
 */

NS_IMETHODIMP
sbIPDProperties::InitSerialNumber(nsIVariant* aSerialNumber)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 * \brief Initialize Firmware Version propery for a device
 */

NS_IMETHODIMP
sbIPDProperties::InitFirmwareVersion(const nsAString &aFirmwareVersion)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 * \brief Initialize Device Location for a device
 */

NS_IMETHODIMP
sbIPDProperties::InitDeviceLocation(nsIURI* aDeviceLocationUri)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


/**
 * \brief Initialize propery for a device
 */

NS_IMETHODIMP
sbIPDProperties::InitDeviceIcon(nsIURI* aDeviceIconUri)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


/**
 * \brief Initialize Device Properties for a device
 */

NS_IMETHODIMP
sbIPDProperties::InitDeviceProperties(nsIPropertyBag2* aProperties)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


/**
 * \brief Indicates that the initialization is complete, the initilize
 *        functions above will no longer affect the data.
 */

NS_IMETHODIMP
sbIPDProperties::InitDone()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


//
// Getters/setters.
//

/**
 * A human readable name. Not included in the hash code and therefore
 * optional. This name can be written to by the user and should be reflected
 * back to the device.
 */

NS_IMETHODIMP
sbIPDProperties::GetFriendlyName(nsAString& aFriendlyName)
{
  return GetPropertyAsAString(NS_LITERAL_STRING("FriendlyName"), aFriendlyName);
}

NS_IMETHODIMP
sbIPDProperties::SetFriendlyName(const nsAString& aFriendlyName)
{
  return SetPropertyAsAString(NS_LITERAL_STRING("FriendlyName"), aFriendlyName);
}


/**
 * Default name for the device. Not included in the hash code and therefore
 * optional.
 */

NS_IMETHODIMP
sbIPDProperties::GetDefaultName(nsAString& aDefaultName)
{
  return GetPropertyAsAString(
             NS_LITERAL_STRING(SB_DEVICE_PROPERTY_DEFAULT_NAME),
             aDefaultName);
}


/**
 * A string identifying the vendor of a device. Included in the hash code.
 */

NS_IMETHODIMP
sbIPDProperties::GetVendorName(nsAString& aVendorName)
{
  return GetPropertyAsAString(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MANUFACTURER),
                              aVendorName);
}


/**
 * Model number for the device. Can be set in any appropriate format, but
 * will be converted to a string and included in the hash code.
 */

NS_IMETHODIMP
sbIPDProperties::GetModelNumber(nsIVariant** aModelNumber)
{
  return GetProperty(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MODEL), aModelNumber);
}


/**
 * Serial number for the device. Can be set in any appropriate format, but
 * will be converted to a string and included in the hash code.
 */

NS_IMETHODIMP
sbIPDProperties::GetSerialNumber(nsIVariant** aSerialNumber)
{
  return GetProperty(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_SERIAL_NUMBER),
                     aSerialNumber);
}

NS_IMETHODIMP
sbIPDProperties::GetFirmwareVersion(nsAString &aFirmwareVersion)
{
  aFirmwareVersion.Truncate();
  return NS_OK;
}

/**
 * A URI representing the location of the device. Not included in the hash
 * code and therefore optional.
 */

NS_IMETHODIMP
sbIPDProperties::GetUri(nsIURI** aUri)
{
  return NS_ERROR_FAILURE;
}


/**
 * The preferred icon to be displayed to the user.
 * The user may be able to specify a custom icon for the device in the future.
 */

NS_IMETHODIMP
sbIPDProperties::GetIconUri(nsIURI** aIconUri)
{
  return GetPropertyAsInterface(NS_LITERAL_STRING("DeviceIcon"),
                                NS_GET_IID(nsIURI),
                                (void**) aIconUri);
}


NS_IMETHODIMP
sbIPDProperties::SetHidden(PRBool aHidden)
{
  nsresult rv =
    mProperties2->SetPropertyAsBool(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_HIDDEN),
                                    aHidden);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDevice->CreateAndDispatchEvent(
      sbIDeviceEvent::EVENT_DEVICE_INFO_CHANGED, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}


NS_IMETHODIMP
sbIPDProperties::GetHidden(PRBool *aHidden)
{
  NS_ENSURE_ARG_POINTER(aHidden);

  nsresult rv = mProperties2->GetPropertyAsBool(
      NS_LITERAL_STRING(SB_DEVICE_PROPERTY_HIDDEN),
      aHidden);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * All of the properties available for a device.
 * Should only contain objects that implement nsIProperty!
 */

NS_IMETHODIMP
sbIPDProperties::GetProperties(nsIPropertyBag2** aProperties)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aProperties);

  // Return results.
  NS_ADDREF(*aProperties = this);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// iPod device properties nsIWritablePropertyBag implementation.
//
//------------------------------------------------------------------------------

/**
 * Set a property with the given name to the given value.  If
 * a property already exists with the given name, it is
 * overwritten.
 */

NS_IMETHODIMP
sbIPDProperties::SetProperty(const nsAString& name,
                             nsIVariant*      value)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(value);

  // Function variables.
  nsresult rv;

  // If a property needs to be set under the request lock, enqueue a set
  // property request.  The internal property in the properties object is still
  // set here to provide immediate feedback.
  if (name.EqualsLiteral("FriendlyName")) {
    rv = mDevice->ReqPushSetNamedValue(sbIPDDevice::REQUEST_SET_PROPERTY,
                                       name,
                                       value);
    NS_ENSURE_SUCCESS(rv, rv);

    // once the name is set, mark the device as set up
    rv = mDevice->SetIsSetUp(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Set the property internally.
  rv = mProperties->SetProperty(name, value);
  NS_ENSURE_SUCCESS(rv, rv);

  // Dispatch a device info changed event.
  rv = mDevice->CreateAndDispatchEvent
                  (sbIDeviceEvent::EVENT_DEVICE_INFO_CHANGED, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Delete a property with the given name.
 * @throws NS_ERROR_FAILURE if a property with that name doesn't
 * exist.
 */

NS_IMETHODIMP
sbIPDProperties::DeleteProperty(const nsAString& name)
{
  return mProperties->DeleteProperty(name);
}


//------------------------------------------------------------------------------
//
// iPod device properties services.
//
//------------------------------------------------------------------------------

/**
 * Construct an iPod device properties object.
 */

sbIPDProperties::sbIPDProperties(sbIPDDevice* aDevice) :
  mDevice(aDevice)
{
  // Validate object fields.
  NS_ASSERTION(mDevice, "mDevice is null");
}


/**
 * Destroy an iPod device properties object.
 */

sbIPDProperties::~sbIPDProperties()
{
}


/**
 * Initialize the iPod device properties.
 */

nsresult
sbIPDProperties::Initialize()
{
  nsresult rv;

  // Create the internal property storage object.
  mProperties = do_CreateInstance("@songbirdnest.com/moz/xpcom/sbpropertybag;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mProperties2 = do_QueryInterface(mProperties, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Copy the device creation parameters to the device properties.
  nsCOMPtr<nsIPropertyBag2> parameters;
  nsAutoString              property;
  rv = mDevice->GetParameters(getter_AddRefs(parameters));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = parameters->GetPropertyAsAString
                      (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MANUFACTURER),
                       property);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mProperties2->SetPropertyAsAString
                       (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MANUFACTURER),
                        property);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = parameters->GetPropertyAsAString
                     (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MODEL),
                      property);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mProperties2->SetPropertyAsAString
                       (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MODEL),
                        property);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = parameters->GetPropertyAsAString
                      (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_SERIAL_NUMBER),
                       property);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mProperties2->SetPropertyAsAString
                       (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_SERIAL_NUMBER),
                        property);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mProperties2->SetPropertyAsBool
                       (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_HIDDEN),
                        PR_FALSE);

  // Set the device access compatibility.
  rv = mProperties2->SetPropertyAsAString
         (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_ACCESS_COMPATIBILITY),
          NS_LITERAL_STRING("rw"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the device icon property.
  rv = mProperties2->SetPropertyAsInterface
                       (NS_LITERAL_STRING("DeviceIcon"),
                        sbIPDURI("chrome://ipod/skin/icon-ipod.png").get());
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Finalize the iPod device properties.
 */

void
sbIPDProperties::Finalize()
{
  // Remove object references.
  mDevice = nsnull;
  mProperties = nsnull;
  mProperties2 = nsnull;
}


/**
 * Set the internal property storage for the property specified by aName to the
 * value specified by aValue.  Bypass any special processing of properties.
 *
 * \param aName                 Name of property to set.
 * \param aValue                Value to which to set property.
 */

nsresult
sbIPDProperties::SetPropertyInternal(const nsAString& aName,
                                     nsIVariant*      aValue)
{
  // Validate properties.
  NS_ENSURE_ARG_POINTER(aValue);

  // Set the internal property storage.
  return mProperties->SetProperty(aName, aValue);
}

