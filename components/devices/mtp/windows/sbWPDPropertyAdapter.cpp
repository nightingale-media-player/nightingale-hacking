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

#include "sbWPDPropertyAdapter.h"
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsComponentManagerUtils.h>
#include <nsMemory.h>
#include <nsStringAPI.h>
#include <nsNetUtil.h>
#include <nsIURI.h>
#include "sbWPDCommon.h"
#include "sbWPDDevice.h"
#include <nsIVariant.h>
#include <PortableDevice.h>
#include <nsNetCID.h>

NS_IMPL_THREADSAFE_ADDREF(sbWPDPropertyAdapter)
NS_IMPL_THREADSAFE_RELEASE(sbWPDPropertyAdapter)

NS_INTERFACE_MAP_BEGIN(sbWPDPropertyAdapter)
  NS_INTERFACE_MAP_ENTRY(sbIDeviceProperties)
  NS_INTERFACE_MAP_ENTRY(nsIWritablePropertyBag)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIPropertyBag, nsIWritablePropertyBag)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, sbIDeviceProperties)
  NS_INTERFACE_MAP_ENTRY(nsIPropertyBag2)
  NS_INTERFACE_MAP_ENTRY(nsIWritablePropertyBag2)
NS_INTERFACE_MAP_END

sbWPDPropertyAdapter::sbWPDPropertyAdapter(sbWPDDevice * device)
{
  nsresult rv;
  mWorkerVariant = do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
  nsString const & deviceID = device->GetDeviceID(device->PortableDevice());
  nsRefPtr<IPortableDeviceContent> content;
  if (SUCCEEDED(device->PortableDevice()->Content(getter_AddRefs(content))))
    content->Properties(getter_AddRefs(mDeviceProperties));
  mDeviceID = device->GetDeviceID(device->PortableDevice());
}

sbWPDPropertyAdapter::~sbWPDPropertyAdapter()
{
}

sbWPDPropertyAdapter * sbWPDPropertyAdapter::New(sbWPDDevice * device)
{
  return new sbWPDPropertyAdapter(device);
}

/* void initFriendlyName (in AString aFriendlyName); */
NS_IMETHODIMP sbWPDPropertyAdapter::InitFriendlyName(const nsAString & aFriendlyName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void initVendorName (in AString aVendorName); */
NS_IMETHODIMP sbWPDPropertyAdapter::InitVendorName(const nsAString & aVendorName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void initModelNumber (in nsIVariant aModelNumber); */
NS_IMETHODIMP sbWPDPropertyAdapter::InitModelNumber(nsIVariant *aModelNumber)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void initSerialNumber (in nsIVariant aSerialNumber); */
NS_IMETHODIMP sbWPDPropertyAdapter::InitSerialNumber(nsIVariant *aSerialNumber)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void initDeviceLocation (in nsIURI aDeviceLocationUri); */
NS_IMETHODIMP sbWPDPropertyAdapter::InitDeviceLocation(nsIURI *aDeviceLocationUri)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void initDeviceIcon (in nsIURI aDeviceIconUri); */
NS_IMETHODIMP sbWPDPropertyAdapter::InitDeviceIcon(nsIURI *aDeviceIconUri)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void initDeviceProperties (in nsIPropertyBag2 aProperties); */
NS_IMETHODIMP sbWPDPropertyAdapter::InitDeviceProperties(nsIPropertyBag2 *aProperties)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void initDone (); */
NS_IMETHODIMP sbWPDPropertyAdapter::InitDone()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult sbWPDPropertyAdapter::GetPropertyString(PROPERTYKEY const & key,
                                                 nsIVariant ** var)
{
  NS_ENSURE_TRUE(mDeviceProperties, NS_ERROR_NULL_POINTER);
  return sbWPDDevice::GetProperty(mDeviceProperties,
                                  key,
                                  var);
}

nsresult sbWPDPropertyAdapter::GetPropertyString(PROPERTYKEY const & key,
                                                 nsAString & value)
{
  NS_ENSURE_TRUE(mDeviceProperties, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIVariant> var;
  nsresult rv = GetPropertyString(key,
                                  getter_AddRefs(var));
  NS_ENSURE_SUCCESS(rv, rv);
  return var->GetAsAString(value);
}

nsresult sbWPDPropertyAdapter::SetPropertyString(PROPERTYKEY const & key,
                                                 nsAString const & value)
{
  NS_ENSURE_TRUE(mDeviceProperties, NS_ERROR_NULL_POINTER);
  PROPVARIANT var;
  nsresult rv = sbWPDStringToPropVariant(value, var);
  NS_ENSURE_SUCCESS(rv, rv);  
  rv = sbWPDDevice::SetProperty(mDeviceProperties,
                                key,
                                var);
  PropVariantClear(&var);
  return rv;
}

/* attribute AString friendlyName; */
NS_IMETHODIMP sbWPDPropertyAdapter::GetFriendlyName(nsAString & aFriendlyName)
{
  return GetPropertyString(WPD_DEVICE_FRIENDLY_NAME,
                           aFriendlyName);
}
NS_IMETHODIMP sbWPDPropertyAdapter::SetFriendlyName(const nsAString & aFriendlyName)
{
  return SetPropertyString(WPD_DEVICE_FRIENDLY_NAME,
                           aFriendlyName);
}

/* readonly attribute AString vendorName; */
NS_IMETHODIMP sbWPDPropertyAdapter::GetVendorName(nsAString & aVendorName)
{
  return GetPropertyString(WPD_DEVICE_MANUFACTURER,
                           aVendorName);
}

/* readonly attribute nsIVariant modelNumber; */
NS_IMETHODIMP sbWPDPropertyAdapter::GetModelNumber(nsIVariant * *aModelNumber)
{
  return GetPropertyString(WPD_DEVICE_MODEL,
                           aModelNumber);
}

/* readonly attribute nsIVariant serialNumber; */
NS_IMETHODIMP sbWPDPropertyAdapter::GetSerialNumber(nsIVariant * *aSerialNumber)
{
  return GetPropertyString(WPD_DEVICE_SERIAL_NUMBER,
                           aSerialNumber);
}

/* readonly attribute nsIURI uri; */
NS_IMETHODIMP sbWPDPropertyAdapter::GetUri(nsIURI * *aUri)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIURI iconUri; */
NS_IMETHODIMP sbWPDPropertyAdapter::GetIconUri(nsIURI * *aIconUri)
{
  nsRefPtr<IPortableDeviceManager> manager;
  nsresult rv = sbWPDGetPortableDeviceManager(getter_AddRefs(manager));
  NS_ENSURE_SUCCESS(rv, rv);
  // Retrieve the icon path from the device's properties, first query the length
  DWORD bytes = 0;
  DWORD type;
  if (FAILED(manager->GetDeviceProperty(mDeviceID.get(),
                                        PORTABLE_DEVICE_ICON,
                                        0,
                                        &bytes,
                                        &type)))
    return NS_ERROR_FAILURE;
  BYTE * buffer = new BYTE[bytes];
  // Retrieve the actual iconURI string
  HRESULT hr = manager->GetDeviceProperty(mDeviceID.get(),
                                          PORTABLE_DEVICE_ICON,
                                          buffer,
                                          &bytes,
                                          &type);
  // Save off the file name if successful
  nsString fileName;
  if (SUCCEEDED(hr)) {
    fileName = reinterpret_cast<WCHAR const *>(buffer);
  }
  // Cleanup the buffer
  delete [] buffer;
  // If we couldn't retrieve the icon path return failure
  if (FAILED(hr))
    return NS_ERROR_FAILURE;
  // Create a local file object and initialize it with the icon path
  nsCOMPtr<nsILocalFile> file = 
    do_CreateInstance("@mozilla.org/file/local;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = file->InitWithPath(fileName);
  NS_ENSURE_SUCCESS(rv, rv);
  // Create the URI object to return, initializing it with the file object
  nsCOMPtr<nsIURI> uri;
  rv = NS_NewFileURI(getter_AddRefs(uri), file);
  NS_ENSURE_SUCCESS(rv, rv);
  uri.forget(aIconUri);
  return NS_OK;
}

/* readonly attribute nsIPropertyBag2 properties; */
NS_IMETHODIMP sbWPDPropertyAdapter::GetProperties(nsIPropertyBag2 * *aProperties)
{
  nsCOMPtr<nsIPropertyBag2> properties = do_QueryInterface(static_cast<sbIDeviceProperties*>(this));
  properties.forget(aProperties);
  return NS_OK;
}

/* readonly attribute nsISimpleEnumerator enumerator; */
NS_IMETHODIMP sbWPDPropertyAdapter::GetEnumerator(nsISimpleEnumerator * *aEnumerator)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIVariant getProperty (in AString name); */
NS_IMETHODIMP sbWPDPropertyAdapter::GetProperty(const nsAString & name, nsIVariant **retval)
{
  PROPERTYKEY key;
  NS_ENSURE_TRUE(sbWPDStandardDevicePropertyToPropertyKey(NS_LossyConvertUTF16toASCII(name).get(), key), NS_ERROR_FAILURE);
  return GetPropertyString(key, retval);
}

#define GET_PROPERTY(prop, retval, type) \
  PR_BEGIN_MACRO \
    nsCOMPtr<nsIVariant> var; \
    nsresult rv = GetProperty(prop, getter_AddRefs(var)); \
    NS_ENSURE_SUCCESS(rv, rv); \
    return var->GetAs##type(retval); \
  PR_END_MACRO
  
/* PRInt32 getPropertyAsInt32 (in AString prop); */
NS_IMETHODIMP sbWPDPropertyAdapter::GetPropertyAsInt32(const nsAString & prop, PRInt32 *retval)
{
  GET_PROPERTY(prop, retval, Int32);
}

/* PRUint32 getPropertyAsUint32 (in AString prop); */
NS_IMETHODIMP sbWPDPropertyAdapter::GetPropertyAsUint32(const nsAString & prop, PRUint32 *retval)
{
  GET_PROPERTY(prop, retval, Uint32);
}

/* PRInt64 getPropertyAsInt64 (in AString prop); */
NS_IMETHODIMP sbWPDPropertyAdapter::GetPropertyAsInt64(const nsAString & prop, PRInt64 *retval)
{
  GET_PROPERTY(prop, retval, Int64);
}

/* PRUint64 getPropertyAsUint64 (in AString prop); */
NS_IMETHODIMP sbWPDPropertyAdapter::GetPropertyAsUint64(const nsAString & prop, PRUint64 *retval)
{
  GET_PROPERTY(prop, retval, Uint64);
}

/* double getPropertyAsDouble (in AString prop); */
NS_IMETHODIMP sbWPDPropertyAdapter::GetPropertyAsDouble(const nsAString & prop, double *retval)
{
  GET_PROPERTY(prop, retval, Double);
}

/* AString getPropertyAsAString (in AString prop); */
NS_IMETHODIMP sbWPDPropertyAdapter::GetPropertyAsAString(const nsAString & prop, nsAString & retval)
{
  GET_PROPERTY(prop, retval, AString);
}

/* ACString getPropertyAsACString (in AString prop); */
NS_IMETHODIMP sbWPDPropertyAdapter::GetPropertyAsACString(const nsAString & prop, nsACString & retval)
{
  GET_PROPERTY(prop, retval, ACString);
}

/* AUTF8String getPropertyAsAUTF8String (in AString prop); */
NS_IMETHODIMP sbWPDPropertyAdapter::GetPropertyAsAUTF8String(const nsAString & prop, nsACString & retval)
{
  GET_PROPERTY(prop, retval, AUTF8String);
}

/* boolean getPropertyAsBool (in AString prop); */
NS_IMETHODIMP sbWPDPropertyAdapter::GetPropertyAsBool(const nsAString & prop, PRBool *retval)
{
  GET_PROPERTY(prop, retval, Bool);
}

/* void getPropertyAsInterface (in AString prop, in nsIIDRef iid, [iid_is (iid), retval] out nsQIResult result); */
NS_IMETHODIMP sbWPDPropertyAdapter::GetPropertyAsInterface(const nsAString & prop, const nsIID & iid, void * * retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIVariant get (in AString prop); */
NS_IMETHODIMP sbWPDPropertyAdapter::Get(const nsAString & prop, nsIVariant **retval)
{
  return GetProperty(prop, retval);
}

/* PRBool hasKey (in AString prop); */
NS_IMETHODIMP sbWPDPropertyAdapter::HasKey(const nsAString & prop, PRBool *retval)
{
  NS_ENSURE_ARG(retval);
  *retval = PR_FALSE;
  nsRefPtr<IPortableDeviceKeyCollection> keys;
  DWORD count;
  if (FAILED(mDeviceProperties->GetSupportedProperties(WPD_DEVICE_OBJECT_ID,
                                                       getter_AddRefs(keys))) ||
      FAILED(keys->GetCount(&count)))
    return NS_ERROR_FAILURE;
  
  PROPERTYKEY keyToFind;
  if (sbWPDStandardDevicePropertyToPropertyKey(NS_LossyConvertUTF16toASCII(prop).get(), keyToFind)) {
    for (DWORD index = 0; index < count; ++index) {
      PROPERTYKEY key;
      if (SUCCEEDED(keys->GetAt(index, &key)) &&
          memcmp(&key, &keyToFind, sizeof(PROPERTYKEY)) == 0) {
        *retval = PR_TRUE;
        break;
      }
    }
  }
  return NS_OK;
}

/* void setProperty (in AString name, in nsIVariant value); */
NS_IMETHODIMP sbWPDPropertyAdapter::SetProperty(const nsAString & name, nsIVariant *value)
{
  PROPERTYKEY key;
  if (sbWPDStandardDevicePropertyToPropertyKey(NS_LossyConvertUTF16toASCII(name).get(), key)) {
    return sbWPDDevice::SetProperty(mDeviceProperties,
                                    key,
                                    value);
  }
  return NS_ERROR_FAILURE;
}

/* void deleteProperty (in AString name); */
NS_IMETHODIMP sbWPDPropertyAdapter::DeleteProperty(const nsAString & name)
{
  PROPERTYKEY key;
  if (sbWPDStandardDevicePropertyToPropertyKey(NS_LossyConvertUTF16toASCII(name).get(), key)) {
    nsRefPtr<IPortableDeviceKeyCollection> propertyKeys;
    nsresult rv = sbWPDCreatePropertyKeyCollection(key, getter_AddRefs(propertyKeys));
    NS_ENSURE_SUCCESS(rv, rv);
    if (FAILED(mDeviceProperties->Delete(WPD_DEVICE_OBJECT_ID,
                                         propertyKeys)))
      return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

#define SET_PROPERTY(name, val, type) \
  PR_BEGIN_MACRO \
    nsresult rv; \
    PROPERTYKEY key; \
    if (sbWPDStandardDevicePropertyToPropertyKey(NS_LossyConvertUTF16toASCII(name).get(), key)) { \
      rv = mWorkerVariant->SetAs##type(val); \
      NS_ENSURE_SUCCESS(rv, rv); \
      return sbWPDDevice::SetProperty(mDeviceProperties, \
                                      prop, \
                                      mWorkerVariant); \
    } \
    return NS_ERROR_FAILURE; \
  PR_END_MACRO
  
/* void setPropertyAsInt32 (in AString prop, in PRInt32 value); */
NS_IMETHODIMP sbWPDPropertyAdapter::SetPropertyAsInt32(const nsAString & prop, PRInt32 value)
{
  SET_PROPERTY(prop, value, Int32);
}

/* void setPropertyAsUint32 (in AString prop, in PRUint32 value); */
NS_IMETHODIMP sbWPDPropertyAdapter::SetPropertyAsUint32(const nsAString & prop, PRUint32 value)
{
  SET_PROPERTY(prop, value, Uint32);
}

/* void setPropertyAsInt64 (in AString prop, in PRInt64 value); */
NS_IMETHODIMP sbWPDPropertyAdapter::SetPropertyAsInt64(const nsAString & prop, PRInt64 value)
{
  SET_PROPERTY(prop, value, Int64);
}

/* void setPropertyAsUint64 (in AString prop, in PRUint64 value); */
NS_IMETHODIMP sbWPDPropertyAdapter::SetPropertyAsUint64(const nsAString & prop, PRUint64 value)
{
  SET_PROPERTY(prop, value, Uint64);
}

/* void setPropertyAsDouble (in AString prop, in double value); */
NS_IMETHODIMP sbWPDPropertyAdapter::SetPropertyAsDouble(const nsAString & prop, double value)
{
  SET_PROPERTY(prop, value, Double);
}

/* void setPropertyAsAString (in AString prop, in AString value); */
NS_IMETHODIMP sbWPDPropertyAdapter::SetPropertyAsAString(const nsAString & prop, const nsAString & value)
{
  SET_PROPERTY(prop, value, AString);
}

/* void setPropertyAsACString (in AString prop, in ACString value); */
NS_IMETHODIMP sbWPDPropertyAdapter::SetPropertyAsACString(const nsAString & prop, const nsACString & value)
{
  SET_PROPERTY(prop, value, ACString);
}

/* void setPropertyAsAUTF8String (in AString prop, in AUTF8String value); */
NS_IMETHODIMP sbWPDPropertyAdapter::SetPropertyAsAUTF8String(const nsAString & prop, const nsACString & value)
{
  SET_PROPERTY(prop, value, AUTF8String);
}

/* void setPropertyAsBool (in AString prop, in boolean value); */
NS_IMETHODIMP sbWPDPropertyAdapter::SetPropertyAsBool(const nsAString & prop, PRBool value)
{
  SET_PROPERTY(prop, value, Bool);
}

/* void setPropertyAsInterface (in AString prop, in nsISupports value); */
NS_IMETHODIMP sbWPDPropertyAdapter::SetPropertyAsInterface(const nsAString & prop, nsISupports *value)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
