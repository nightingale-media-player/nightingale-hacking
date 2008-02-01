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

#include "sbWPDDevice.h"
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsMemory.h>
#include <nsStringAPI.h>
#include <propvarutil.h>

#include <nsCOMPtr.h>
#include <sbIDevice.h>
#include "sbWPDCommon.h"
#include "sbPropertyVariant.h"
#include <propvarutil.h>
#include <sbDeviceContent.h>

/* Implementation file */
NS_IMPL_THREADSAFE_ADDREF(sbWPDDevice)
NS_IMPL_THREADSAFE_RELEASE(sbWPDDevice)
NS_IMPL_QUERY_INTERFACE2_CI(sbWPDDevice,
    sbIDevice,
    nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER1(sbWPDDevice,
    sbIDevice)

NS_DECL_CLASSINFO(sbWPDDevice)
NS_IMPL_THREADSAFE_CI(sbWPDDevice)

nsString const sbWPDDevice::DEVICE_ID_PROP(NS_LITERAL_STRING("DeviceID"));
nsString const sbWPDDevice::DEVICE_FRIENDLY_NAME_PROP(NS_LITERAL_STRING("DeviceFriendlyName"));
nsString const sbWPDDevice::DEVICE_DESCRIPTION_PROP(NS_LITERAL_STRING("DeviceDescription"));
nsString const sbWPDDevice::DEVICE_MANUFACTURER_PROP(NS_LITERAL_STRING("DeviceManufacturer"));

sbWPDDevice::sbWPDDevice(nsID const & controllerID,
                         nsIPropertyBag2 * deviceProperties,
                         IPortableDevice * device) :
                           mPortableDevice(device),
                           mDeviceProperties(deviceProperties),
                           mControllerID(controllerID),
                           mState(STATE_IDLE)
{
  NS_ASSERTION(deviceProperties, "deviceProperties cannot be null");
}

sbWPDDevice::~sbWPDDevice()
{
  /* destructor code */
}

/* readonly attribute AString name; */
NS_IMETHODIMP sbWPDDevice::GetName(nsAString & aName)
{
  nsString const & deviceID = GetDeviceID();
  CComPtr<IPortableDeviceManager> deviceManager;
  nsresult rv = sbGetPortableDeviceManager(&deviceManager);
  NS_ENSURE_SUCCESS(rv, rv);
  
  DWORD bufferSize = 0;
  WCHAR * buffer;
  // Query the size of the value
  if (FAILED(deviceManager->GetDeviceFriendlyName(deviceID.get(), 0, &bufferSize)))
    return NS_ERROR_FAILURE;
  // Expand the buffer if needed
  buffer = new WCHAR[bufferSize];
  if (!buffer)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = deviceManager->GetDeviceFriendlyName(deviceID.get(), buffer, &bufferSize);
  delete [] buffer;
  return rv;
}

/* readonly attribute nsIDPtr controllerId; */
NS_IMETHODIMP sbWPDDevice::GetControllerId(nsID * *aControllerId)
{
  NS_ENSURE_ARG(aControllerId);
  *aControllerId = static_cast<nsID*>(NS_Alloc(sizeof(nsID)));
  if (!*aControllerId)
    return NS_ERROR_OUT_OF_MEMORY;

  **aControllerId = mControllerID;
  return NS_OK;
}

/* readonly attribute nsIDPtr id; */
NS_IMETHODIMP sbWPDDevice::GetId(nsID * *aId)
{
  NS_ENSURE_ARG(aId);
  static nsID const id = SB_WPDDEVICE_CID;
  *aId = static_cast<nsID*>(NS_Alloc(sizeof(nsID)));
  **aId = id;
  return NS_OK;
}

/* void connect (); */
NS_IMETHODIMP sbWPDDevice::Connect()
{
  // If the pointer is set then it's already opened
  if (!mPortableDevice) {
    CComPtr<IPortableDevice> portableDevice;
    HRESULT hr = CoCreateInstance(CLSID_PortableDevice,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IPortableDevice,
                                  reinterpret_cast<void**>(&portableDevice));
    if (FAILED(hr)) {
      NS_ERROR("Unable to create the CLSID_PortableDevice");
      return NS_ERROR_FAILURE;
    }
    CComPtr<IPortableDeviceValues> clientInfo;
    if (NS_FAILED(GetClientInfo(&clientInfo)) || !clientInfo) {
      NS_ERROR("Unable to get client info");
      return NS_ERROR_FAILURE;
    }
    if (NS_FAILED(portableDevice->Open(GetDeviceID().get(), clientInfo))) {
      NS_ERROR("Unable to open the MTP device");
      return NS_ERROR_FAILURE;
    }
    mPortableDevice = portableDevice;
  }
  return NS_OK; 
}

/* void disconnect (); */
NS_IMETHODIMP sbWPDDevice::Disconnect()
{
  if (mPortableDevice) {
    if (SUCCEEDED(mPortableDevice->Close()))
      return NS_ERROR_FAILURE;
    mPortableDevice = nsnull;
  }
  return NS_OK;
}

/* readonly attribute boolean connected; */
NS_IMETHODIMP sbWPDDevice::GetConnected(PRBool *aConnected)
{
  // If we have a portable device object then we're conntected
  *aConnected = mPortableDevice != nsnull;
  return NS_OK;
}

/* readonly attribute boolean threaded; */
NS_IMETHODIMP sbWPDDevice::GetThreaded(PRBool *aThreaded)
{
  *aThreaded = PR_TRUE;
  return NS_OK;
}

nsresult CreatePropertyKeyCollection(PROPERTYKEY const & key,
                                     IPortableDeviceKeyCollection ** propertyKeys)
{
  NS_ENSURE_ARG(propertyKeys);
  CComPtr<IPortableDeviceKeyCollection> propKeys;
  
  NS_ENSURE_TRUE(SUCCEEDED(CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                                            NULL,
                                            CLSCTX_INPROC_SERVER,
                                            IID_IPortableDeviceKeyCollection,
                                            (VOID**) &propKeys)),
                 NS_ERROR_FAILURE);
  *propertyKeys = propKeys.Detach();
  return (*propertyKeys)->Add(key);

}
/* nsIVariant getPreference (in AString aPrefName); */
NS_IMETHODIMP sbWPDDevice::GetPreference(const nsAString & aPrefName,
                                        nsIVariant **retval)
{
  NS_ASSERTION(mPortableDevice, "No portable device");
  CComPtr<IPortableDeviceProperties> properties;
  nsresult rv = GetDeviceProperties(mPortableDevice, &properties);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return GetProperty(properties, aPrefName, retval);
}

/* void setPreference (in AString aPrefName, in nsIVariant aPrefValue); */
NS_IMETHODIMP sbWPDDevice::SetPreference(const nsAString & aPrefName,
                                      nsIVariant *aPrefValue)
{
  NS_ASSERTION(mPortableDevice, "No portable device");
  CComPtr<IPortableDeviceProperties> properties;
  nsresult rv = GetDeviceProperties(mPortableDevice, &properties);
  NS_ENSURE_SUCCESS(rv, rv);  
  return SetProperty(properties, aPrefName, aPrefValue);
}

/* readonly attribute sbIDeviceCapabilities capabilities; */
NS_IMETHODIMP sbWPDDevice::GetCapabilities(sbIDeviceCapabilities * *aCapabilities)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute sbIDeviceContent content; */
NS_IMETHODIMP sbWPDDevice::GetContent(sbIDeviceContent * *aContent)
{
  sbDeviceContent* deviceContent = new sbDeviceContent;
  NS_ENSURE_TRUE(deviceContent, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(deviceContent);
  nsresult const rv = deviceContent->Init();
  if (NS_SUCCEEDED(rv))
    *aContent = deviceContent;
  else
    NS_RELEASE(deviceContent);
  return rv;
}

/* readonly attribute nsIPropertyBag2 parameters; */
NS_IMETHODIMP sbWPDDevice::GetParameters(nsIPropertyBag2 * *aParameters)
{
  NS_ENSURE_ARG(aParameters);
  NS_ENSURE_TRUE(mDeviceProperties, NS_ERROR_NULL_POINTER);
  *aParameters = mDeviceProperties.get();
  NS_ADDREF(*aParameters);
  return NS_OK;
}

NS_IMETHODIMP sbWPDDevice::GetState(PRUint32 *aState)
{
  return sbBaseDevice::GetState(aState);
}

nsresult sbWPDDevice::ProcessRequest()
{
  TransferRequest * request = nsnull;
  while (NS_SUCCEEDED(PopRequest(&request)) && request) {
    switch (request->type) {
    case TransferRequest::REQUEST_MOUNT:
    case TransferRequest::REQUEST_READ:
    case TransferRequest::REQUEST_EJECT:
    case TransferRequest::REQUEST_SUSPEND:
    case TransferRequest::REQUEST_WRITE:
    case TransferRequest::REQUEST_DELETE:
    case TransferRequest::REQUEST_SYNC:
    case TransferRequest::REQUEST_WIPE:                    /* delete all files */
    case TransferRequest::REQUEST_MOVE:                    /* move an item in one playlist */
    case TransferRequest::REQUEST_UPDATE:
    case TransferRequest::REQUEST_NEW_PLAYLIST:
      break;
    }
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult sbWPDDevice::GetClientInfo(IPortableDeviceValues ** clientInfo)
{
  CComPtr<IPortableDeviceValues> info;
  // CoCreate an IPortableDeviceValues interface to hold the client information.
  NS_ENSURE_TRUE(SUCCEEDED(CoCreateInstance(CLSID_PortableDeviceValues,
                                            NULL,
                                            CLSCTX_INPROC_SERVER,
                                            IID_IPortableDeviceValues,
                                            (VOID**) &info)),
                 NS_ERROR_FAILURE);

  LPCWSTR const CLIENT_NAME = L"Songbird";
  DWORD const MAJOR_VERSION = 0;
  DWORD const MINOR_VERSION = 5;
  DWORD const REVISION = 42;
  if (FAILED(info->SetStringValue(WPD_CLIENT_NAME, CLIENT_NAME)) ||
      FAILED(info->SetUnsignedIntegerValue(WPD_CLIENT_MAJOR_VERSION, MAJOR_VERSION)) ||
      FAILED(info->SetUnsignedIntegerValue(WPD_CLIENT_MINOR_VERSION, MINOR_VERSION)) ||
      FAILED(info->SetUnsignedIntegerValue(WPD_CLIENT_REVISION, REVISION)) ||
      FAILED(info->SetUnsignedIntegerValue(WPD_CLIENT_SECURITY_QUALITY_OF_SERVICE, SECURITY_IMPERSONATION)))
    return NS_ERROR_FAILURE;
  *clientInfo = info.Detach();
  return NS_OK;
}

nsString sbWPDDevice::GetDeviceID() const
{
  if (mPnPDeviceID.IsEmpty() && mPortableDevice) {
    LPWSTR deviceID;
    if (SUCCEEDED(mPortableDevice->GetPnPDeviceID(&deviceID))) {
      // Temporarily discard const, we're still logically const
      const_cast<sbWPDDevice*>(this)->mPnPDeviceID = deviceID;
      CoTaskMemFree(deviceID);
    }
  }
  if (mPnPDeviceID.IsEmpty() && mDeviceProperties) {
    nsresult rv = mDeviceProperties->GetPropertyAsAString(DEVICE_ID_PROP, const_cast<sbWPDDevice*>(this)->mPnPDeviceID);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Unable to retreive the device ID property");
  }
  return mPnPDeviceID;
}

nsresult sbWPDDevice::GetDeviceProperties(IPortableDevice * device,
                                          IPortableDeviceProperties ** properties)
{
  NS_ENSURE_ARG(device);
  NS_ENSURE_ARG(properties);
  CComPtr<IPortableDeviceContent> content;
  NS_ENSURE_TRUE(SUCCEEDED(device->Content(&content)),
                 NS_ERROR_FAILURE);
  
  NS_ENSURE_TRUE(SUCCEEDED(content->Properties(properties)),
                 NS_ERROR_FAILURE);
  return NS_OK;
}

nsresult nsIVariantToPROPVARIANT(nsIVariant * aValue,
                                 PROPVARIANT & prop)
{
  PRUint16 dataType;
  nsresult rv = aValue->GetDataType(&dataType);
  NS_ENSURE_SUCCESS(rv, rv);
  switch (dataType) {
    case nsIDataType::VTYPE_INT8:
    case nsIDataType::VTYPE_UINT8:
    case nsIDataType::VTYPE_INT16:
    case nsIDataType::VTYPE_UINT16:
    case nsIDataType::VTYPE_INT32:
    case nsIDataType::VTYPE_UINT32:
    case nsIDataType::VTYPE_BOOL: {
      PRInt32 valueInt;
      rv = aValue->GetAsInt32(&valueInt);
      if (NS_SUCCEEDED(rv)) {  // fall through PRInt64 case otherwise
        NS_ENSURE_TRUE(SUCCEEDED(InitPropVariantFromInt32(valueInt, &prop)),
                       NS_ERROR_FAILURE);
        return NS_OK;
      }
    }
    case nsIDataType::VTYPE_INT64:
    case nsIDataType::VTYPE_UINT64: {
      PRInt64 valueLong;
      rv = aValue->GetAsInt64(&valueLong);
      if (NS_SUCCEEDED(rv)) {  // fall through double case otherwise
        NS_ENSURE_TRUE(SUCCEEDED(InitPropVariantFromInt64(valueLong, &prop)),
                       NS_ERROR_FAILURE);
        return NS_OK;
      }
    }
    case nsIDataType::VTYPE_FLOAT:
    case nsIDataType::VTYPE_DOUBLE: {
      double valueDouble;
      rv = aValue->GetAsDouble(&valueDouble);
      NS_ENSURE_SUCCESS(rv, rv);
      
      NS_ENSURE_TRUE(SUCCEEDED(InitPropVariantFromDouble(valueDouble, &prop)),
                     NS_ERROR_FAILURE);
      return NS_OK;
    }
    case nsIDataType::VTYPE_CHAR:
    case nsIDataType::VTYPE_WCHAR:
    case nsIDataType::VTYPE_DOMSTRING:
    case nsIDataType::VTYPE_CHAR_STR:
    case nsIDataType::VTYPE_WCHAR_STR:
    case nsIDataType::VTYPE_STRING_SIZE_IS:
    case nsIDataType::VTYPE_WSTRING_SIZE_IS:
    case nsIDataType::VTYPE_UTF8STRING:
    case nsIDataType::VTYPE_CSTRING:
    case nsIDataType::VTYPE_ASTRING: {
      nsAutoString stringValue;
      rv = aValue->GetAsAString(stringValue);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_TRUE(SUCCEEDED(InitPropVariantFromString(stringValue.get(), &prop)),
                     NS_ERROR_FAILURE);
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}
                                 
nsresult sbWPDDevice::SetProperty(IPortableDeviceProperties * properties,
                                  nsAString const & key,
                                  nsIVariant * value)
{
  PROPERTYKEY propKey;
  NS_ENSURE_TRUE(SUCCEEDED(PSPropertyKeyFromString(nsString(key).get(), 
                                                   &propKey)),
                 NS_ERROR_FAILURE);
  
  CComPtr<IPortableDeviceValues> propValues;
  NS_ENSURE_TRUE(CoCreateInstance(CLSID_PortableDeviceValues,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IPortableDeviceValues,
                                  (VOID**) &propValues),
                 NS_ERROR_FAILURE);
  PROPVARIANT pv = {0};
  PropVariantInit(&pv);
  nsresult rv = nsIVariantToPROPVARIANT(value, pv);
  NS_ENSURE_SUCCESS(rv, rv);
  propValues->SetValue(propKey, &pv);
  CComPtr<IPortableDeviceValues> results;
  rv = properties->SetValues(WPD_DEVICE_OBJECT_ID, propValues , &results);
  NS_ENSURE_SUCCESS(rv, rv);
  
  HRESULT error;
  NS_ENSURE_TRUE(SUCCEEDED(results->GetErrorValue(propKey, &error)) && SUCCEEDED(error),
                 NS_ERROR_FAILURE);
  PROPVARIANT propVal = {0};
  PropVariantInit(&propVal);
  
  NS_ENSURE_TRUE(SUCCEEDED(propValues->GetAt(0, 0, &propVal)), 
                 NS_ERROR_FAILURE);
  return NS_OK;
}

nsresult sbWPDDevice::GetProperty(IPortableDeviceProperties * properties,
                                  PROPERTYKEY const & propKey,
                                  nsIVariant ** retval)
{
  CComPtr<IPortableDeviceKeyCollection> propertyKeys;
  nsresult rv = CreatePropertyKeyCollection(propKey, &propertyKeys);
  NS_ENSURE_SUCCESS(rv, rv);
  
  CComPtr<IPortableDeviceValues> propValues;
  rv = properties->GetValues(WPD_DEVICE_OBJECT_ID, propertyKeys, &propValues);
  NS_ENSURE_SUCCESS(rv, rv);
  
  PROPVARIANT propVal = {0};
  PropVariantInit(&propVal);
  
  NS_ENSURE_TRUE(SUCCEEDED(propValues->GetAt(0, 0, &propVal)), 
                 NS_ERROR_FAILURE);
  *retval = new sbPropertyVariant(propVal);
  if (!*retval)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*retval);
  return NS_OK;
}

nsresult sbWPDDevice::GetProperty(IPortableDeviceProperties * properties,
                                  nsAString const & key,
                                  nsIVariant ** value)
{
  PROPERTYKEY propKey;
  NS_ENSURE_TRUE(SUCCEEDED(PSPropertyKeyFromString(nsString(key).get(), 
                                                   &propKey)),
                 NS_ERROR_FAILURE);
  GetProperty(properties, propKey, value);
}