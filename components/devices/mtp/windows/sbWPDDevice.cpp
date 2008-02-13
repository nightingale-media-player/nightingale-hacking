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
#include <set>
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsMemory.h>
#include <nsStringAPI.h>
#include <nsIThread.h>
#include <nsThreadUtils.h>
#include <nsIVariant.h>
#include <nsCOMPtr.h>
#include <nsAutoPtr.h>
#include <nsIURL.h>
#include <nsID.h>
#include <sbIDevice.h>
#include <sbIDeviceEvent.h>
#include <sbILibrary.h>
#include <sbDeviceStatus.h>
#include "sbWPDCommon.h"
#include "sbPropertyVariant.h"
#include <sbDeviceContent.h>
#include <nsIInputStream.h>
#include <nsIOutputStream.h>
#include "sbWPDDeviceThread.h"
#include "sbWPDPropertyAdapter.h"
#include <sbDeviceCapabilities.h>
/* damn you microsoft */
#undef CreateEvent
#include <sbIDeviceManager.h>

/* Implementation file */

/**
 * Helper functions
 */
/**
 * This helper functions retrieves a list of values for a content item on the device
 * @param theWPDDevice is the device to ask
 * @param aWPDContentID is the ID of the content item
 * @param keys is an stl collection that provides begin and end methods.
 * @param values is the collection of values that is the result
 * @return is NS_OK or some other value if there is an error
 */
template<class T> 
static nsresult GetDeviceContentProperties(IPortableDevice * theWPDDevice,
                                           nsAString const & aWPDContentID,
                                           T const & keys,
                                           IPortableDeviceValues ** values)
{
  nsRefPtr<IPortableDeviceContent> content;
  
  hr = theWPDDevice->Content(getter_AddRefs(content));
  if (FAILED(hr)) {
    return NS_ERROR_FAILURE;
  }
  
  nsRefPtr<IPortableDeviceProperties> properties;
  hr = content->Properties(properties);
  if (FAILED(hr)) {
    return NS_ERROR_FAILURE;
  }
  nsRefPtr<IPortableDeviceKeyCollection> propertiesToRead;
  hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection, 
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IPortableDeviceKeyCollection,
                        getter_AddRefs(propertiesToRead));
  if (FAILED(hr)) {
    return NS_ERROR_FAILURE;
  }
  keys::const_iterator end = keys.end();
  for (keys::const_iterator iter = keys.begin(); iter != end; ++iter) {
    propertiesToRead->Add(*iter);
  }
  return properties->GetValues(aWPDContentID,
                               propertiesToRead,
                               values);
}

/**
 * sbWPDDevice Implementation
 */
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
nsString const sbWPDDevice::PUID_SBIMEDIAITEM_PROPERTY(NS_LITERAL_STRING("WPDPUID"));

sbWPDDevice * sbWPDDevice::New(nsID const & controllerID,
                               nsIPropertyBag2 * deviceProperties,
                               IPortableDevice * device)
{
  return new sbWPDDevice(controllerID,
                         deviceProperties,
                         device);
}

sbWPDDevice::sbWPDDevice(nsID const & controllerID,
                         nsIPropertyBag2 * deviceProperties,
                         IPortableDevice * device) :
                           mPortableDevice(device),
                           mDeviceProperties(deviceProperties),
                           mControllerID(controllerID),
                           mState(STATE_IDLE)
{
  NS_ASSERTION(deviceProperties, "deviceProperties cannot be null");
  
  // Startup our worker thread
  mRequestsPendingEvent = CreateEventW(0, FALSE, FALSE, 0);
  mDeviceThread = new sbWPDDeviceThread(this,
                                  mRequestsPendingEvent);
  NS_NewThread(getter_AddRefs(mThreadObject), mDeviceThread);
  NS_IF_ADDREF(mDeviceThread);
}

sbWPDDevice::~sbWPDDevice()
{
  if (mDeviceThread) {
    mDeviceThread->Die();
    mThreadObject->Shutdown();
    NS_RELEASE(mDeviceThread);
  }
  NS_IF_RELEASE(mDeviceThread);
  CloseHandle(mRequestsPendingEvent);
}

/* readonly attribute AString name; */
NS_IMETHODIMP sbWPDDevice::GetName(nsAString & aName)
{
  nsString const & deviceID = GetDeviceID(mPortableDevice);
  nsRefPtr<IPortableDeviceManager> deviceManager;
  nsresult rv = sbWPDGetPortableDeviceManager(getter_AddRefs(deviceManager));
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
    nsRefPtr<IPortableDevice> portableDevice;
    HRESULT hr = CoCreateInstance(CLSID_PortableDevice,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IPortableDevice,
                                  getter_AddRefs(portableDevice));
    if (FAILED(hr)) {
      NS_ERROR("Unable to create the CLSID_PortableDevice");
      return NS_ERROR_FAILURE;
    }
    nsRefPtr<IPortableDeviceValues> clientInfo;
    if (NS_FAILED(GetClientInfo(getter_AddRefs(clientInfo))) || !clientInfo) {
      NS_ERROR("Unable to get client info");
      return NS_ERROR_FAILURE;
    }
    if (NS_FAILED(portableDevice->Open(GetDeviceID(mPortableDevice).get(), clientInfo))) {
      NS_ERROR("Unable to open the MTP device");
      return NS_ERROR_FAILURE;
    }
    mPortableDevice = portableDevice;
  }

  if (!mDeviceThread) {
    mDeviceThread = new sbWPDDeviceThread(this, mRequestsPendingEvent);
    NS_NewThread(getter_AddRefs(mThreadObject), mDeviceThread);
    NS_IF_ADDREF(mDeviceThread);
  }

  return NS_OK; 
}

/* void disconnect (); */
NS_IMETHODIMP sbWPDDevice::Disconnect()
{
  if (mPortableDevice) {
    if (FAILED(mPortableDevice->Close()))
      NS_WARNING("Failed to close PortableDevice instance!");
    mPortableDevice = nsnull;
  }

  if (mDeviceThread) {
    mDeviceThread->Die();
    mThreadObject->Shutdown();
  }

  NS_IF_RELEASE(mDeviceThread);

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

/* nsIVariant getPreference (in AString aPrefName); */
NS_IMETHODIMP sbWPDDevice::GetPreference(const nsAString & aPrefName,
                                        nsIVariant **retval)
{
  NS_ASSERTION(mPortableDevice, "No portable device");
  nsRefPtr<IPortableDeviceProperties> properties;
  nsresult rv = GetDeviceProperties(mPortableDevice, getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);
  
  return GetProperty(properties, aPrefName, retval);
}

/* void setPreference (in AString aPrefName, in nsIVariant aPrefValue); */
NS_IMETHODIMP sbWPDDevice::SetPreference(const nsAString & aPrefName,
                                      nsIVariant *aPrefValue)
{
  NS_ASSERTION(mPortableDevice, "No portable device");
  nsRefPtr<IPortableDeviceProperties> properties;
  nsresult rv = GetDeviceProperties(mPortableDevice, getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);  
  return SetProperty(properties, aPrefName, aPrefValue);
}

/* readonly attribute sbIDeviceCapabilities capabilities; */
NS_IMETHODIMP sbWPDDevice::GetCapabilities(sbIDeviceCapabilities * *theCapabilities)
{
  nsresult rv;
  nsCOMPtr<sbIDeviceCapabilities> capabilities = do_CreateInstance(SONGBIRD_DEVICECAPABILITIES_CONTRACTID,
                                                                   &rv);
  nsRefPtr<IPortableDeviceCapabilities> deviceCaps;
  if (FAILED(mPortableDevice->Capabilities(getter_AddRefs(deviceCaps))))
    return NS_ERROR_FAILURE;
  //rv = SetFunctionalTypes(deviceCaps,
  //                        capabilities);
  NS_ENSURE_SUCCESS(rv, rv);
  capabilities.forget(theCapabilities);
  return NS_OK;
}

/* readonly attribute sbIDeviceContent content; */
NS_IMETHODIMP sbWPDDevice::GetContent(sbIDeviceContent * *aContent)
{
  sbDeviceContent* deviceContent = sbDeviceContent::New();
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

/* readonly attribute nsIDeviceProperties device properties; */
NS_IMETHODIMP sbWPDDevice::GetProperties(sbIDeviceProperties * *theProperties)
{
  NS_ENSURE_ARG(theProperties);
  *theProperties = sbWPDPropertyAdapter::New(this);
  NS_IF_ADDREF(*theProperties);
  return *theProperties ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP sbWPDDevice::GetState(PRUint32 *aState)
{
  return sbBaseDevice::GetState(aState);
}

nsresult sbWPDDevice::ProcessRequest()
{
  // Let the worker thread know there's work to be done.
  ::SetEvent(mRequestsPendingEvent);
  return NS_OK;
}

PRBool sbWPDDevice::ProcessThreadsRequest()
{
  nsresult rv;
  nsRefPtr<TransferRequest> request;
  nsRefPtr<TransferRequest> nextRequest;

  while (NS_SUCCEEDED(rv = PopRequest(getter_AddRefs(request))) && request) {
    
    // PeekRequest returns NS_OK+nsnull if there is no next request
    rv = PeekRequest(getter_AddRefs(nextRequest));
    // Something bad happened terminate the thread
    NS_ENSURE_SUCCESS(rv, PR_FALSE);
  
    nsCOMPtr<sbILibrary> lib;
    if (request->list) {
      lib = do_QueryInterface(request->list);
    }
  
    switch (request->type) {
      case TransferRequest::REQUEST_EJECT:
      case TransferRequest::REQUEST_MOUNT:
        // MTP devices can't be mounted or ejected
        return NS_ERROR_NOT_IMPLEMENTED;
      case TransferRequest::REQUEST_READ:
        return ReadRequest(request);
      case TransferRequest::REQUEST_SUSPEND:
        break;
      case TransferRequest::REQUEST_WRITE: {
        if (lib) {
          // add item to library
          return WriteRequest(request);
        } else {
          // add item to playlist
          rv = AddItemToPlaylist(request->item, request->list, request->index);
          return NS_SUCCEEDED(rv);
        }
      }
      case TransferRequest::REQUEST_DELETE: {
        if (lib) {
          // remove item from library
          return RemoveItem(request->item);
        } else {
          // remove item from list
          rv = RemoveItemFromPlaylist(request->list, request->index);
          return NS_SUCCEEDED(rv);
        }
      }
      case TransferRequest::REQUEST_SYNC:
      case TransferRequest::REQUEST_WIPE:                    /* delete all files */
        break;
      case TransferRequest::REQUEST_MOVE: {                  /* move an item in one playlist */
        /* note that we can't quite batch things, since at this point the
           media list we're given may be from the future (other non-move actions
           might have been applied to it).  So we can't just rebuild the whole
           thing, we need to be dumb. */
        rv = MoveItemInPlaylist(request->list, request->index, request->otherIndex);
        return NS_SUCCEEDED(rv);
      }
      case TransferRequest::REQUEST_UPDATE:
        break;
      case TransferRequest::REQUEST_NEW_PLAYLIST: {
        nsCOMPtr<sbIMediaList> list = do_QueryInterface(request->item, &rv);
        NS_ENSURE_SUCCESS(rv, PR_FALSE);
        rv = CreatePlaylist(list);
        return NS_SUCCEEDED(rv);
      }
    }
    // Let the worker thread know there's work to be done.
  }
  return PR_TRUE;
}

nsresult sbWPDDevice::GetClientInfo(IPortableDeviceValues ** clientInfo)
{
  nsRefPtr<IPortableDeviceValues> info;
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
  info.forget(clientInfo);
  return NS_OK;
}

nsString sbWPDDevice::GetDeviceID(IPortableDevice * device)
{
  nsString result;
  if (device) {
    LPWSTR deviceID;
    if (SUCCEEDED(device->GetPnPDeviceID(&deviceID))) {
      // Temporarily discard const, we're still logically const
      result = deviceID;
      CoTaskMemFree(deviceID);
    }
  }
  return result;
}

nsresult sbWPDDevice::GetDeviceProperties(IPortableDevice * device,
                                          IPortableDeviceProperties ** properties)
{
  NS_ENSURE_ARG(device);
  NS_ENSURE_ARG(properties);
  nsRefPtr<IPortableDeviceContent> content;
  NS_ENSURE_TRUE(SUCCEEDED(device->Content(getter_AddRefs(content))),
                 NS_ERROR_FAILURE);
  
  NS_ENSURE_TRUE(SUCCEEDED(content->Properties(properties)),
                 NS_ERROR_FAILURE);
  return NS_OK;
}

nsresult sbWPDDevice::SetProperty(IPortableDeviceProperties * properties,
                                  nsAString const & key,
                                  nsIVariant * value)
{
  PROPERTYKEY propKey;
  nsresult rv;
  rv = PropertyKeyFromString(key, &propKey);
  NS_ENSURE_SUCCESS(rv, rv);
  return SetProperty(properties,
                     propKey,
                     value);
}

nsresult sbWPDDevice::SetProperty(IPortableDeviceProperties * properties,
                                  PROPERTYKEY const & key,
                                  nsIVariant * value)
{
  nsRefPtr<IPortableDeviceValues> propValues;
  PROPVARIANT pv = {0};
  PropVariantInit(&pv);
  nsresult rv = sbWPDnsIVariantToPROPVARIANT(value, pv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetProperty(properties,
                   key,
                   pv);
  PropVariantClear(&pv);
  return rv;  
}
nsresult sbWPDDevice::SetProperty(IPortableDeviceProperties * properties,
                                  PROPERTYKEY const & key,
                                  PROPVARIANT const & value)
{
  
  nsRefPtr<IPortableDeviceValues> propValues;
  NS_ENSURE_TRUE(SUCCEEDED(CoCreateInstance(CLSID_PortableDeviceValues,
                                            NULL,
                                            CLSCTX_INPROC_SERVER,
                                            IID_IPortableDeviceValues,
                                            getter_AddRefs(propValues))),
                 NS_ERROR_FAILURE);
  propValues->SetValue(key, &value);
  nsRefPtr<IPortableDeviceValues> results;
  nsresult rv = properties->SetValues(WPD_DEVICE_OBJECT_ID, propValues , getter_AddRefs(results));
  NS_ENSURE_SUCCESS(rv, rv);
  
  HRESULT error;
  NS_ENSURE_TRUE(SUCCEEDED(results->GetErrorValue(key, &error)) && SUCCEEDED(error),
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
  nsRefPtr<IPortableDeviceKeyCollection> propertyKeys;
  nsresult rv = sbWPDCreatePropertyKeyCollection(propKey, getter_AddRefs(propertyKeys));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsRefPtr<IPortableDeviceValues> propValues;
  rv = properties->GetValues(WPD_DEVICE_OBJECT_ID, propertyKeys, getter_AddRefs(propValues));
  NS_ENSURE_SUCCESS(rv, rv);
  
  PROPVARIANT propVal = {0};
  PropVariantInit(&propVal);
  
  NS_ENSURE_TRUE(SUCCEEDED(propValues->GetAt(0, 0, &propVal)), 
                 NS_ERROR_FAILURE);
  *retval = sbPropertyVariant::New(propVal);
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
  nsresult rv = PropertyKeyFromString(key, &propKey);
  NS_ENSURE_SUCCESS(rv, rv);
  return GetProperty(properties, propKey, value);
}

nsresult sbWPDDevice::CreatePlaylist(nsAString const &aName,
                                     nsAString const &aParent,
                                     /*out*/nsAString &aObjId)
{
  HRESULT hr;
  NS_ENSURE_STATE(mPortableDevice);
  
  // create the set of values we need to set on the new playlist
  nsRefPtr<IPortableDeviceValues> values;
  hr = CoCreateInstance(CLSID_PortableDeviceValues,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IPortableDeviceValues,
                        getter_AddRefs(values));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_OUT_OF_MEMORY);
  
  hr = values->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, WPD_CONTENT_TYPE_PLAYLIST);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  hr = values->SetStringValue(WPD_OBJECT_PARENT_ID, aParent.BeginReading());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  hr = values->SetStringValue(WPD_OBJECT_NAME, aName.BeginReading());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  // XXX Mook: do we need to figure out what sort of playlist to use?
  hr = values->SetGuidValue(WPD_OBJECT_FORMAT, WPD_OBJECT_FORMAT_M3UPLAYLIST);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  hr = values->SetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, aName.BeginReading());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  // remember to add the (empty) list of references
  nsRefPtr<IPortableDevicePropVariantCollection> references;
  hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IPortableDevicePropVariantCollection,
                        getter_AddRefs(references));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = references->ChangeType(VT_LPWSTR);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = values->SetIUnknownValue(WPD_OBJECT_REFERENCES, references);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // create the new playlist
  nsRefPtr<IPortableDeviceContent> content;
  hr = mPortableDevice->Content(getter_AddRefs(content));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  LPWSTR newObjId = nsnull;
  hr = content->CreateObjectWithPropertiesOnly(values, &newObjId);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  // record the resulting name
  aObjId.Assign(nsDependentString(newObjId));
  ::CoTaskMemFree(newObjId);
  
  return NS_OK;
}

nsresult sbWPDDevice::CreatePlaylist(sbIMediaList* aPlaylist)
{
  HRESULT hr;
  nsresult rv;
  NS_ENSURE_ARG_POINTER(aPlaylist);
  NS_ENSURE_STATE(mPortableDevice);
  
  nsCOMPtr<sbILibrary> library;
  rv = aPlaylist->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString name;
  rv = aPlaylist->GetName(name);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString libraryObjId = GetWPDDeviceIDFromMediaItem(library);
  NS_ENSURE_TRUE(!libraryObjId.IsEmpty(), NS_ERROR_FAILURE);
  
  // do what we can before actually creating the playlist in case something
  // fails (so we don't get a playlist on the device we don't know about)
  nsRefPtr<IPortableDeviceContent> content;
  hr = mPortableDevice->Content(getter_AddRefs(content));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDeviceProperties> properties;
  hr = content->Properties(getter_AddRefs(properties));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDeviceKeyCollection> keys;
  hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IPortableDeviceKeyCollection,
                        getter_AddRefs(keys));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  hr = keys->Add(WPD_OBJECT_PERSISTENT_UNIQUE_ID);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // okay, we've done what we can, actually create the playlist now
  nsString puid, objid;
  rv = CreatePlaylist(name, libraryObjId, objid);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsRefPtr<IPortableDeviceValues> values;
  hr = properties->GetValues(objid.BeginReading(), keys, getter_AddRefs(values));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<sbPropertyVariant> variant = sbPropertyVariant::New();
  hr = values->GetAt(0, NULL, variant->GetPropVariant());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  rv = variant->GetAsAString(puid);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = aPlaylist->SetProperty(sbWPDDevice::DEVICE_ID_PROP, puid);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult sbWPDDevice::RemoveItem(nsAString const &aObjId)
{
  HRESULT hr;
  nsresult rv;
  NS_ENSURE_STATE(mPortableDevice);


  // remember to add the (empty) list of references
  nsRefPtr<IPortableDevicePropVariantCollection> objIdList;
  hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IPortableDevicePropVariantCollection,
                        getter_AddRefs(objIdList));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = objIdList->ChangeType(VT_LPWSTR);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  nsRefPtr<sbPropertyVariant> variant = sbPropertyVariant::New();
  rv = variant->SetAsWString(aObjId.BeginReading());
  NS_ENSURE_SUCCESS(rv, rv);
  
  hr = objIdList->Add(variant->GetPropVariant());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  nsRefPtr<IPortableDeviceContent> content;
  hr = mPortableDevice->Content(getter_AddRefs(content));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = content->Delete(PORTABLE_DEVICE_DELETE_NO_RECURSION, objIdList, NULL);
  if (S_FALSE == hr) {
    return NS_ERROR_FAILURE;
  }
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  return NS_OK;
}

nsresult sbWPDDevice::RemoveItem(sbIMediaItem *aItem)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(aItem);

  nsString objid = GetWPDDeviceIDFromMediaItem(aItem);
  NS_ENSURE_TRUE(!objid.IsEmpty(), NS_ERROR_FAILURE);
  
  rv = RemoveItem(objid);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult sbWPDDevice::AddItemToPlaylist(sbIMediaItem* aItem, /* the item to add */
                                        sbIMediaList* aList, /* the playlist to add to */
                                        PRUint32 aIndex)     /* the index to add to */
{
  HRESULT hr;
  nsresult rv;
  NS_ENSURE_STATE(mPortableDevice);
  
  nsString itemObjId = GetWPDDeviceIDFromMediaItem(aItem);
  NS_ENSURE_TRUE(!itemObjId.IsEmpty(), NS_ERROR_FAILURE);
  
  /* since WPD has no concept of inserting an item (just append), we need to
     completely rebuild the list. */

  nsRefPtr<IPortableDevicePropVariantCollection> items;
  rv = GetPlaylistReferences(aList, getter_AddRefs(items));
  NS_ENSURE_SUCCESS(rv, rv);

  DWORD itemCount;
  hr = items->GetCount(&itemCount);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(aIndex <= itemCount, NS_ERROR_UNEXPECTED);

  nsRefPtr<IPortableDevicePropVariantCollection> newItems;
  hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IPortableDevicePropVariantCollection,
                        getter_AddRefs(newItems));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  nsRefPtr<sbPropertyVariant> item = sbPropertyVariant::New();
  NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);

  for (PRUint32 i = 0; i < aIndex; ++i) {
    hr = items->GetAt(i, item->GetPropVariant());
    NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
    hr = newItems->Add(item->GetPropVariant());
    NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  }

  rv = item->SetAsAString(itemObjId);
  NS_ENSURE_SUCCESS(rv, rv);
  hr = newItems->Add(item->GetPropVariant());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  for (PRUint32 i = aIndex; i < itemCount; ++i) {
    hr = items->GetAt(i, item->GetPropVariant());
    NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
    hr = newItems->Add(item->GetPropVariant());
    NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  }

  rv = SetPlaylistReferences(aList, newItems);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult sbWPDDevice::MoveItemInPlaylist(sbIMediaList* aList,
                                         PRUint32 aFromIndex,
                                         PRUint32 aToIndex)
{
  HRESULT hr;
  nsresult rv;
  NS_ENSURE_STATE(mPortableDevice);

  /* so we need to first figure out the old media item objid, since both indices
     are useless in looking at the media list (as moves are batched so by the
     point we get here, the whole batch has already been completed). */
  
  nsRefPtr<IPortableDevicePropVariantCollection> items;
  rv = GetPlaylistReferences(aList, getter_AddRefs(items));
  NS_ENSURE_SUCCESS(rv, rv);
  
  DWORD itemCount;
  hr = items->GetCount(&itemCount);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(aFromIndex < itemCount, NS_ERROR_UNEXPECTED);

  nsRefPtr<sbPropertyVariant> item = sbPropertyVariant::New();
  NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);
  
  hr = items->GetAt(aFromIndex, item->GetPropVariant());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsString targetObjId;
  rv = item->GetAsAString(targetObjId);
  NS_ENSURE_SUCCESS(rv, rv);
  
  /* now we can move it around, yay */
  /* note that since there's no *move* or even *insert at*... we have to
     create a whole new list and replace it. yay. */
  nsRefPtr<IPortableDevicePropVariantCollection> newItems;
  hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IPortableDevicePropVariantCollection,
                        getter_AddRefs(newItems));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  if (aFromIndex < aToIndex) {
    // moving to a later slot
    // first, the items before the item to move
    for (PRUint32 i = 0; i < aFromIndex; ++i) {
      hr = items->GetAt(i, item->GetPropVariant());
      NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
      hr = newItems->Add(item->GetPropVariant());
      NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
    }
    // next, the items between the two
    for (PRUint32 i = aFromIndex + 1; i <= aToIndex; ++i) {
      hr = items->GetAt(i, item->GetPropVariant());
      NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
      hr = newItems->Add(item->GetPropVariant());
      NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
    }
    // third, the item to insert
    rv = item->SetAsAString(targetObjId);
    NS_ENSURE_SUCCESS(rv, rv);
    hr = newItems->Add(item->GetPropVariant());
    NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
    // lastly, the items after both
    for (PRUint32 i = aToIndex + 1; i < itemCount; ++i) {
      hr = items->GetAt(i, item->GetPropVariant());
      NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
      hr = newItems->Add(item->GetPropVariant());
      NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
    }
  } else {
    // move from a later slot to an earlier slot
    // first, the items before the destination
    for (PRUint32 i = 0; i < aToIndex; ++i) {
      hr = items->GetAt(i, item->GetPropVariant());
      NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
      hr = newItems->Add(item->GetPropVariant());
      NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
    }
    // next, the item to insert
    rv = item->SetAsAString(targetObjId);
    NS_ENSURE_SUCCESS(rv, rv);
    hr = newItems->Add(item->GetPropVariant());
    NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
    // third, the items between the two
    for (PRUint32 i = aToIndex; i < aFromIndex; ++i) {
      hr = items->GetAt(i, item->GetPropVariant());
      NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
      hr = newItems->Add(item->GetPropVariant());
      NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
    }
    // lastly, the items after both
    for (PRUint32 i = aFromIndex + 1; i < itemCount; ++i) {
      hr = items->GetAt(i, item->GetPropVariant());
      NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
      hr = newItems->Add(item->GetPropVariant());
      NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
    }
  }
  // now we can replace the list.
  rv = SetPlaylistReferences(aList, newItems);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult sbWPDDevice::RemoveItemFromPlaylist(sbIMediaList* aList, /* the list to remove from */
                                             PRUint32 aIndex)     /* the item to remove */
{
  HRESULT hr;
  nsresult rv;
  NS_ENSURE_STATE(mPortableDevice);

  nsRefPtr<IPortableDevicePropVariantCollection> items;
  rv = GetPlaylistReferences(aList, getter_AddRefs(items));
  NS_ENSURE_SUCCESS(rv, rv);
  
  hr = items->RemoveAt(aIndex);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  rv = SetPlaylistReferences(aList, items);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult sbWPDDevice::GetPlaylistReferences(sbIMediaList* aList,
                                            /* out */ IPortableDevicePropVariantCollection** aItems)
{
  HRESULT hr;
  NS_ENSURE_STATE(mPortableDevice);
  NS_ENSURE_ARG_POINTER(aList);
  NS_ENSURE_ARG_POINTER(aItems);

  nsString objId = GetWPDDeviceIDFromMediaItem(aList);
  NS_ENSURE_TRUE(!objId.IsEmpty(), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDeviceContent> content;
  hr = mPortableDevice->Content(getter_AddRefs(content));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDeviceProperties> properties;
  hr = content->Properties(getter_AddRefs(properties));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDeviceKeyCollection> keys;
  hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IPortableDeviceKeyCollection,
                        getter_AddRefs(keys));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  hr = keys->Add(WPD_OBJECT_REFERENCES);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDeviceValues> values;
  hr = properties->GetValues(objId.BeginReading(),
                             keys,
                             getter_AddRefs(values));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  hr = values->GetIPortableDevicePropVariantCollectionValue(WPD_OBJECT_REFERENCES,
                                                            aItems);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  return NS_OK;
}

nsresult sbWPDDevice::SetPlaylistReferences(sbIMediaList* aList,
                                            IPortableDevicePropVariantCollection* aItems)
{
  HRESULT hr;
  NS_ENSURE_STATE(mPortableDevice);
  NS_ENSURE_ARG_POINTER(aList);
  NS_ENSURE_ARG_POINTER(aItems);
  
  nsString objId = GetWPDDeviceIDFromMediaItem(aList);
  NS_ENSURE_TRUE(!objId.IsEmpty(), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDeviceContent> content;
  hr = mPortableDevice->Content(getter_AddRefs(content));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDeviceProperties> properties;
  hr = content->Properties(getter_AddRefs(properties));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  nsRefPtr<IPortableDeviceKeyCollection> keys;
  hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IPortableDeviceKeyCollection,
                        getter_AddRefs(keys));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = keys->Add(WPD_OBJECT_REFERENCES);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDeviceValues> values;
  hr = properties->GetValues(objId.BeginReading(),
                             keys,
                             getter_AddRefs(values));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = values->SetIPortableDevicePropVariantCollectionValue(WPD_OBJECT_REFERENCES,
                                                            aItems);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDeviceValues> setValueResult;
  hr = properties->SetValues(objId.BeginReading(),
                             values,
                             getter_AddRefs(setValueResult));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  return NS_OK;
}

nsresult sbWPDDevice::CreateAndDispatchEvent(PRUint32 aType,
                                             nsIVariant *aData)
{
  nsresult rv;
  
  nsCOMPtr<sbIDeviceManager2> manager =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<sbIDeviceEvent> deviceEvent;
  rv = manager->CreateEvent(aType, aData, static_cast<sbIDevice*>(this),
                            getter_AddRefs(deviceEvent));
  NS_ENSURE_SUCCESS(rv, rv);
  
  return DispatchEvent(deviceEvent, PR_TRUE, nsnull);
}

nsString sbWPDDevice::GetWPDDeviceIDFromMediaItem(sbIMediaItem * mediaItem)
{
  nsString result;
  nsString mediaID;
  // Ignore error, treat as not found
  mediaItem->GetProperty(PUID_SBIMEDIAITEM_PROPERTY,
                         mediaID);
  if (!mediaID.IsEmpty()) {
    nsRefPtr<IPortableDeviceContent> content;
    if (SUCCEEDED(mPortableDevice->Content(getter_AddRefs(content))) && content) {
      sbWPDObjectIDFromPUID(content,
                         mediaID,
                         result);
    }
  }
  return result;
}

/**
 * Copy from a moz stream to a COM stream
 * @param status The status option to record our progress
 * @param contentLength the total length of the content we're writing
 * @param input The Moz input stream we'll be reading from
 * @param output The COM output stream we'll be writing to
 * @param bufferSize the optional buffer size to use.
 */
static nsresult CopynsIStream2IStream(sbDeviceStatus & status,
                                      sbWPDDevice * device,
                                      PRInt64 contentLength,
                                      nsIInputStream * input,
                                      IStream * output,
                                      PRUint32 bufferSize = 64 * 1024)
{
  double const length = contentLength;
  double current = 0;
  char * buffer = new char[bufferSize];
  PRUint32 bytesRead;
  nsresult rv;
  for (rv = input->Read(buffer, bufferSize, &bytesRead);
       NS_SUCCEEDED(rv) && bytesRead != 0;
       rv = input->Read(buffer, bufferSize, &bytesRead)) {
    ULONG bytesWritten;
    HRESULT hr = output->Write(buffer, bytesRead, &bytesWritten);
    if (FAILED(hr) || bytesWritten < bytesRead) {
      rv = NS_ERROR_FAILURE;
      output->Revert();
      break;
    }
    current += bytesWritten;
    double const progress = current / length * 100.0;
    nsCOMPtr<nsIWritableVariant> var =
      do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    var->SetAsDouble(progress);
    device->CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_TRANSFER_START,
                                   var);
    status.Progress(progress);
    
  }
  delete [] buffer;
  // Something bad happened
  if (bytesRead)
    return rv;
  return NS_OK;
}

static nsresult CopyIStream2nsIStream(sbDeviceStatus & status,
                                      sbWPDDevice * device,
                                      IStream * input,
                                      nsIOutputStream * output,
                                      PRUint32 bufferSize = 64 * 1024)
{
  char * buffer = new char[bufferSize];
  ULONG bytesRead;
  nsresult rv;
  for (HRESULT hr = input->Read(buffer, bufferSize, &bytesRead);
       SUCCEEDED(hr) && bytesRead != 0;
       hr = input->Read(buffer, bufferSize, &bytesRead)) {
    PRUint32 bytesWritten;  
    rv = output->Write(buffer, bytesRead, &bytesWritten);
    if (NS_FAILED(rv) || bytesWritten < bytesRead) {
      if (NS_SUCCEEDED(rv))
        rv = NS_ERROR_FAILURE;
      // TODO: We need to clean up the file
      output->Close();
    }
  }
  delete [] buffer;
  // Something bad happened
  if (bytesRead)
    return rv;
  return NS_OK;
}

struct MimeTypeMediaMapItem
{
  WCHAR * MimeType;
  GUID WPDContentType;
  GUID WPDFormatType;
};

MimeTypeMediaMapItem MimeTypeMediaMap[] = 
{
  {L"audio/x-ms-wma;", WPD_CONTENT_TYPE_AUDIO, WPD_OBJECT_FORMAT_WMA},
  {L"audio/mp3;", WPD_CONTENT_TYPE_AUDIO, WPD_OBJECT_FORMAT_MP3},
  {L"audio/mpeg;", WPD_CONTENT_TYPE_AUDIO, WPD_OBJECT_FORMAT_MPEG},
  {L"audio/x-wav;", WPD_CONTENT_TYPE_AUDIO, WPD_OBJECT_FORMAT_WAVE},
  {L"audio/x-aiff;", WPD_CONTENT_TYPE_AUDIO, WPD_OBJECT_FORMAT_AIFF}
};
PRUint32 const MimeTypeMediaMapSize = sizeof(MimeTypeMediaMap)/sizeof(MimeTypeMediaMapItem);

static nsresult GetWPDContentType(nsAString const & mimeType,
                                  GUID & contentType,
                                  GUID & formatType)
{
  for (int index = 0; index < MimeTypeMediaMapSize; ++index) {
    MimeTypeMediaMapItem const & item = MimeTypeMediaMap[index];
    if (mimeType.Equals(item.MimeType)) {
      contentType = item.WPDContentType;
      formatType = item.WPDFormatType;
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

static nsresult SetParentProperty(IPortableDeviceContent * content,
                                  sbIMediaList * list,
                                  IPortableDeviceValues * properties)
{
  nsString puid;
  list->GetProperty(sbWPDDevice::PUID_SBIMEDIAITEM_PROPERTY,
                    puid);
  nsString objectID;
  if (puid.IsEmpty() || NS_FAILED(sbWPDObjectIDFromPUID(content,
                                                     puid,
                                                     objectID)))
    return NS_ERROR_FAILURE;
  if (FAILED(properties->SetStringValue(WPD_OBJECT_PARENT_ID, objectID.get())))
    return NS_ERROR_FAILURE;
}

static nsresult SetContentLength(sbIMediaItem * item,
                                 IPortableDeviceValues * properties)
{
  PRInt64 length;
  if (NS_FAILED(item->GetContentLength(&length)))
    return NS_ERROR_FAILURE;
  
  if (FAILED(properties->SetUnsignedLargeIntegerValue(WPD_OBJECT_SIZE, length)))
    return NS_ERROR_FAILURE;
  
  return NS_OK;
}

static nsresult SetContentName(sbIMediaItem * item,
                               IPortableDeviceValues * properties)
{
  nsCOMPtr<nsIURI> theURI;
  nsresult rv = item->GetContentSrc(getter_AddRefs(theURI));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIURL> theURL = do_QueryInterface(theURI, &rv);
  nsCString temp;
  rv = theURL->GetFileName(temp);
  NS_ENSURE_SUCCESS(rv, rv);
  nsString const & fileName = NS_ConvertUTF8toUTF16(temp);
  if (FAILED(properties->SetStringValue(WPD_OBJECT_NAME, fileName.get())))
    return NS_ERROR_FAILURE;
  
  return rv;
}

inline nsString GetItemID(sbIMediaItem * item)
{
  nsString guid;
  item->GetGuid(guid);
  return guid;
}

nsresult sbWPDDevice::GetPropertiesFromItem(IPortableDeviceContent * content,
                                            sbIMediaItem * item,
                                            sbIMediaList * list,
                                            IPortableDeviceValues ** itemProperties)
{
  nsRefPtr<IPortableDeviceValues> properties;
  HRESULT hr = CoCreateInstance(CLSID_PortableDeviceValues,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IPortableDeviceValues,
                                getter_AddRefs(properties));
  if (FAILED(hr))
    return NS_ERROR_FAILURE;
  
  nsString PUID;
  nsresult rv = item->GetProperty(sbWPDDevice::PUID_SBIMEDIAITEM_PROPERTY,
                                  PUID);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString contentMimeType;
  rv = item->GetContentType(contentMimeType);
  NS_ENSURE_SUCCESS(rv, rv);
  
  GUID WPDContentType;
  GUID WPDFormatType;
  rv = GetWPDContentType(contentMimeType,
                         WPDContentType,
                         WPDFormatType);
  if (NS_SUCCEEDED(rv)) {
    if (FAILED(properties->SetGuidValue(WPD_OBJECT_CONTENT_TYPE,
                                        WPDContentType)) ||
        FAILED(properties->SetGuidValue(WPD_OBJECT_FORMAT,
                                        WPDFormatType)))
      return NS_ERROR_FAILURE;
  }
  else {
    nsCOMPtr<nsIWritableVariant> var =
      do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    var->SetAsAString(GetItemID(item));
    CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_MEDIA_WRITE_UNSUPPORTED_TYPE,
                           var);    
    return SB_ERROR_MEDIA_TYPE_NOT_SUPPORTED;
  }
  rv = SetParentProperty(content, list, properties);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = SetContentLength(item, properties);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = SetContentName(item, properties);
  NS_ENSURE_SUCCESS(rv, rv);
  
  properties.forget(itemProperties);
  return NS_OK;
}

nsresult sbWPDDevice::CreateDeviceObjectFromMediaItem(sbDeviceStatus & status,
                                                      sbIMediaItem * item,
                                                      sbIMediaList * list)
{
  PRInt64 contentLength;
  nsresult rv = item->GetContentLength(&contentLength);
  NS_ENSURE_SUCCESS(rv, rv);
  status.CurrentOperation(NS_LITERAL_STRING("Copying"));
  status.SetItem(item);
  status.SetList(list);
  status.StateMessage(NS_LITERAL_STRING("Starting"));
  nsRefPtr<IPortableDeviceContent> content;
  rv = mPortableDevice->Content(getter_AddRefs(content));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<IPortableDeviceValues> properties;
  rv = GetPropertiesFromItem(content,
                             item,
                             list,
                             getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<IStream> streamWriter;
  DWORD optimalBufferSize;
  if (FAILED(content->CreateObjectWithPropertiesAndData(properties,
                                                        getter_AddRefs(streamWriter),
                                                        &optimalBufferSize,
                                                        NULL)))
    return NS_ERROR_FAILURE;
  nsCOMPtr<nsIInputStream> inputStream;
  rv = item->OpenInputStream(getter_AddRefs(inputStream));
  nsRefPtr<IPortableDeviceDataStream> portableDataStream;
  if (FAILED(streamWriter->QueryInterface(__uuidof(IPortableDeviceDataStream), getter_AddRefs(portableDataStream))))
      return NS_ERROR_FAILURE;
  NS_ENSURE_SUCCESS(rv, rv);
  status.StateMessage(NS_LITERAL_STRING("InProgress"));
  rv = CopynsIStream2IStream(status,
                             this,
                             contentLength,
                             inputStream,
                             streamWriter,
                             optimalBufferSize);
  streamWriter->Commit(STGC_DEFAULT);
  LPWSTR newObjectID = NULL;
  if (SUCCEEDED(portableDataStream->GetObjectID(&newObjectID)))
  {
    properties = nsnull;
    GUID puid;
    if (SUCCEEDED(properties->GetGuidValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, &puid))) {
      WCHAR buffer[48];
      StringFromGUID2(puid,
                      buffer,
                      sizeof(buffer));
      item->SetProperty(sbWPDDevice::PUID_SBIMEDIAITEM_PROPERTY, nsString(buffer));
    }
    ::CoTaskMemFree(newObjectID);
  }
  return NS_OK;
}

nsresult sbWPDDevice::WriteRequest(TransferRequest * request)
{
  nsresult rv;
  nsCOMPtr<nsIWritableVariant> var =
    do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  var->SetAsAString(GetItemID(request->item));
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_MEDIA_WRITE_START,
                         var);  
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_TRANSFER_START,
                         var);
                    
  sbDeviceStatus status(GetDeviceID(mPortableDevice));
  rv = CreateDeviceObjectFromMediaItem(status,
                                       request->item,
                                       request->list);
  if (NS_SUCCEEDED(rv)) {
    status.StateMessage(NS_LITERAL_STRING("Completed"));
    CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_TRANSFER_END,
                           var);    
    CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_MEDIA_WRITE_END,
                           var);      
  }
  else {
    status.StateMessage(NS_LITERAL_STRING("Failed"));
    CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_MEDIA_WRITE_FAILED,
                           var);      }
  NS_RELEASE(request);
  return rv;
}

static nsresult GetStreamFromDevice(IPortableDeviceContent * content,
                                    nsAString const & objectID,
                                    IStream ** stream,
                                    DWORD & optimalBufferSize)
{
  
  nsRefPtr<IPortableDeviceResources> resources;
  if (FAILED(content->Transfer(getter_AddRefs(resources))))
    return NS_ERROR_FAILURE;
  if (FAILED(resources->GetStream(nsString(objectID).get(),            // Identifier of the object we want to transfer
                                 WPD_RESOURCE_DEFAULT,    // We are transferring the default resource (which is the entire object's data)
                                 STGM_READ,               // Opening a stream in READ mode, because we are reading data from the device.
                                 &optimalBufferSize,  // Driver supplied optimal transfer size
                                 stream)))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

nsresult sbWPDDevice::ReadRequest(TransferRequest * request)
{
  nsresult rv;
  nsCOMPtr<nsIWritableVariant> var =
    do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  var->SetAsAString(GetItemID(request->item));
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_MEDIA_READ_START,
                         var);
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_TRANSFER_START,
                         var);
  nsRefPtr<IPortableDeviceContent> content;
  rv = mPortableDevice->Content(getter_AddRefs(content));
  NS_ENSURE_SUCCESS(rv, rv);
  nsString objectID = GetWPDDeviceIDFromMediaItem(request->item);
  DWORD optimalBufferSize = 0;
  nsRefPtr<IStream> inputStream;
  rv = GetStreamFromDevice(content,
                           objectID,
                           getter_AddRefs(inputStream),
                           optimalBufferSize);
  NS_ENSURE_SUCCESS(rv, rv);
  STATSTG stat;
  if (FAILED(inputStream->Stat(&stat, STATFLAG_DEFAULT)))
    return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIOutputStream> outputStream;
  rv = request->item->OpenOutputStream(getter_AddRefs(outputStream));
  NS_ENSURE_SUCCESS(rv, rv);
  sbDeviceStatus status(GetDeviceID(mPortableDevice));
  rv = CopyIStream2nsIStream(status,
                             this,
                             inputStream,
                             outputStream,
                             optimalBufferSize); 
  if (NS_SUCCEEDED(rv)) {
    status.StateMessage(NS_LITERAL_STRING("Completed"));
    CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_TRANSFER_END,
                           var);    
    CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_MEDIA_READ_END,
                           var);      
  }
  else {
    status.StateMessage(NS_LITERAL_STRING("Failed"));
    CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_MEDIA_READ_FAILED,
                           var);      }
  NS_RELEASE(request);
  return NS_OK;
}

#ifndef NSID_LENGTH
#define NSID_LENGTH 39
#pragma message(" XXX Mook Remove this junk when we get new xulrunners!")
#endif
nsresult sbWPDDevice::PropertyKeyFromString(const nsAString & aString,
                                            PROPERTYKEY* aPropKey)
{
  NS_ENSURE_ARG_POINTER(aPropKey);
  
  NS_ENSURE_TRUE(aString.Length() > NSID_LENGTH + 2, NS_ERROR_INVALID_ARG);
  nsID id;
  PRBool success;
  success = id.Parse(NS_LossyConvertUTF16toASCII(aString).BeginReading());
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  aPropKey->fmtid = *(GUID*)&id;
  
  nsresult rv;
  aPropKey->pid = nsDependentSubstring(aString, NSID_LENGTH + 2).ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}
