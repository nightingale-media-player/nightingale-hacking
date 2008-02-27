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
#include <nsINetUtil.h>
#include <nsStringAPI.h>
#include <nsIThread.h>
#include <nsNetUtil.h>
#include <nsThreadUtils.h>
#include <nsIVariant.h>
#include <nsCRT.h>
#include <nsCOMPtr.h>
#include <nsAutoPtr.h>
#include <nsIFileURL.h>
#include <nsIURL.h>
#include <nsID.h>
#include <sbIDevice.h>
#include <sbIDeviceEvent.h>
#include "sbIDeviceLibrary.h"
#include <sbILibrary.h>
#include <sbDeviceStatus.h>
#include <sbLibraryListenerHelpers.h>
#include "sbWPDCommon.h"
#include "sbPropertyVariant.h"
#include <sbDeviceContent.h>
#include <nsIInputStream.h>
#include <nsIOutputStream.h>
#include <nsIWritablePropertyBag.h>
#include "sbStringUtils.h"
#include "sbWPDDeviceThread.h"
#include "sbWPDPropertyAdapter.h"
#include <nsIPrefService.h>
#include <nsIPrefBranch.h>
#include <nsTArray.h>
#include <sbProxiedComponentManager.h>
#include <sbIOrderableMediaList.h>
/* damn you microsoft */
#undef CreateEvent

#include <sbIDeviceManager.h>
#include "sbWPDCapabilitiesBuilder.h"
#include "sbPortableDevicePropertiesBulkImportCallback.h"

/* Implementation file */

/**
 * Macros
 */
static inline 
nsresult CreateAndDispatchGenericDeviceErrorEvent(sbWPDDevice *aDevice) {
  nsresult rv;
  
  nsCOMPtr<nsIWritableVariant> var = do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  var->SetAsISupports(NS_ISUPPORTS_CAST(sbIDevice *, aDevice));

  rv = aDevice->CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_ERROR_UNEXPECTED, var);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

#define SB_ENSURE_NO_DEVICE_ERROR_GENERIC(_result, _device)    \
  PR_BEGIN_MACRO                                               \
  if(NS_FAILED(_result)) {                                     \
    CreateAndDispatchGenericDeviceErrorEvent(_device);         \
  }                                                            \
  PR_END_MACRO

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
NS_IMPL_QUERY_INTERFACE3_CI(sbWPDDevice,
    sbIDevice,
    sbIDeviceEventTarget,
    nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER2(sbWPDDevice,
    sbIDeviceEventTarget,
    sbIDevice)

NS_DECL_CLASSINFO(sbWPDDevice)
NS_IMPL_THREADSAFE_CI(sbWPDDevice)

nsString const sbWPDDevice::DEVICE_ID_PROP(NS_LITERAL_STRING("DeviceID"));
nsString const sbWPDDevice::DEVICE_FRIENDLY_NAME_PROP(NS_LITERAL_STRING("DeviceFriendlyName"));
nsString const sbWPDDevice::DEVICE_DESCRIPTION_PROP(NS_LITERAL_STRING("DeviceDescription"));
nsString const sbWPDDevice::DEVICE_MANUFACTURER_PROP(NS_LITERAL_STRING("DeviceManufacturer"));

nsString const sbWPDDevice::PUID_SBIMEDIAITEM_PROPERTY(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#WPDPUID"));

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
                           mState(STATE_IDLE),
                           mAccessCompatibility(0)
{
  NS_ASSERTION(deviceProperties, "deviceProperties cannot be null");
  
  // look up the device
  deviceProperties->GetPropertyAsInterface(NS_LITERAL_STRING("DevicePointer"),
                                           *reinterpret_cast<const nsIID*>(&IID_IPortableDevice),
                                           getter_AddRefs(mPortableDevice));
  
  // Startup our worker thread
  mRequestsPendingEvent = CreateEventW(0, TRUE, FALSE, 0);
  mDeviceThread = new sbWPDDeviceThread(this,
                                  mRequestsPendingEvent);
  NS_NewThread(getter_AddRefs(mThreadObject), mDeviceThread);
  NS_IF_ADDREF(mDeviceThread);

  sbBaseDevice::Init();
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

inline
nsresult AddHash(unsigned char * aBuffer, int & aOffset,
                 nsIPropertyBag2* aPropBag, const nsAString& aProp)
{
  nsCString value;
  nsresult rv = aPropBag->GetPropertyAsACString(aProp, value);
  NS_ENSURE_SUCCESS(rv, rv);
  
  const nsCString::char_type *p, *end;
  
  for (value.BeginReading(&p, &end); p < end; ++p) {
    unsigned int data = (*p) & 0x7F; // only look at 7 bits
    data <<= (aOffset / 7 + 1) % 8;
    int index = (aOffset + 6) / 8;
    if (index > 0)
      aBuffer[(index - 1) % sizeof(nsID)] ^= (data & 0xFF00) >> 8;
    aBuffer[index % sizeof(nsID)] ^= (data & 0xFF);
    aOffset += 7;
  }

  return S_OK;
}

/* readonly attribute nsIDPtr id; */
NS_IMETHODIMP sbWPDDevice::GetId(nsID * *aId)
{
  NS_ENSURE_ARG(aId);
  
  if (!mDeviceId) {
    // no cached version, make the device id from the property bag
    unsigned char buffer[sizeof(nsID)];
    memset(buffer, 0, sizeof(nsID));
    
    nsresult rv;

    int offset = 0;
    rv = AddHash(buffer, offset, mDeviceProperties,
                 NS_LITERAL_STRING("DeviceManufacturer"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = AddHash(buffer, offset, mDeviceProperties,
                 NS_LITERAL_STRING("ModelNo"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = AddHash(buffer, offset, mDeviceProperties,
                 NS_LITERAL_STRING("SerialNo"));
    NS_ENSURE_SUCCESS(rv, rv);
    
    mDeviceId = new nsID; // yay nsAutoPtr
    *mDeviceId = *(reinterpret_cast<nsID*>(&buffer)); // copy by value
  }

  *aId = static_cast<nsID*>(NS_Alloc(sizeof(nsID)));
  **aId = *mDeviceId; // NB: copy by value
  return NS_OK;
}

/* void connect (); */
NS_IMETHODIMP sbWPDDevice::Connect()
{
  HRESULT hr;
  nsresult rv;
  
  // If the pointer is set then it's already opened
  if (!mPortableDevice) {
    nsRefPtr<IPortableDevice> portableDevice;
    hr = CoCreateInstance(CLSID_PortableDevice,
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
    
    mAccessCompatibility = 1;
    hr = portableDevice->Open(GetDeviceID(portableDevice).get(), clientInfo);

    if(hr == E_ACCESSDENIED) {
      mAccessCompatibility = 0;
      clientInfo->SetUnsignedIntegerValue(WPD_CLIENT_DESIRED_ACCESS,
                                          GENERIC_READ);
      hr = portableDevice->Open(GetDeviceID(portableDevice).get(), clientInfo);
    }
    NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

    mPortableDevice = portableDevice;
  }

  if (!mDeviceThread) {
    mDeviceThread = new sbWPDDeviceThread(this, mRequestsPendingEvent);
    NS_NewThread(getter_AddRefs(mThreadObject), mDeviceThread);
    NS_IF_ADDREF(mDeviceThread);
  }
  
  // get the libraries on the device
  nsRefPtr<IPortableDeviceCapabilities> capabilities;
  hr = mPortableDevice->Capabilities(getter_AddRefs(capabilities));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDevicePropVariantCollection> storageObjs;
  hr = capabilities->GetFunctionalObjects(WPD_FUNCTIONAL_CATEGORY_STORAGE,
                                          getter_AddRefs(storageObjs));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  DWORD storageObjCount;
  hr = storageObjs->GetCount(&storageObjCount);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDeviceContent> content;
  hr = mPortableDevice->Content(getter_AddRefs(content));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDeviceProperties> properties;
  hr = content->Properties(getter_AddRefs(properties));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDeviceKeyCollection> propertiesToRead;
  rv = sbWPDCreatePropertyKeyCollection(WPD_OBJECT_PERSISTENT_UNIQUE_ID,
                                        getter_AddRefs(propertiesToRead));
  NS_ENSURE_SUCCESS(rv, rv);

  char idBuffer[NSID_LENGTH];
  nsID *id;
  rv = GetId(&id);
  NS_ENSURE_SUCCESS(rv, rv);
  id->ToProvidedString(idBuffer);
  NS_Free(id);
  
  for (DWORD objIdx = 0; objIdx < storageObjCount; ++objIdx) {
    nsRefPtr<sbPropertyVariant> variant = sbPropertyVariant::New();
    hr = storageObjs->GetAt(objIdx, variant->GetPropVariant());
    NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
    
    nsString objId;
    rv = variant->GetAsAString(objId);
    NS_ENSURE_SUCCESS(rv, rv);
    
    // get the PUID of the object...
    nsRefPtr<IPortableDeviceValues> values;
    hr = properties->GetValues(objId.BeginReading(),
                               propertiesToRead,
                               getter_AddRefs(values));
    NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
    
    LPWSTR ppuid;
    hr = values->GetStringValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, &ppuid);
    NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
    
    // hack for Sansa View (firmware 01.00.03H):
    // the storage GUID for a View would occasionally have two extra characters
    // in it: U+00C5 (Latin capital A with ring above), and an underscore.
    // we work around this by stripping all non-7bit characters, as well as
    // a few selected delimiters.
    
    nsString puid = nsDependentString(ppuid);
    ::CoTaskMemFree(ppuid);
    
    nsString libId = NS_ConvertASCIItoUTF16(idBuffer + 1, NSID_LENGTH - 3);
    libId.AppendLiteral(".");
    libId.Append(puid);
    libId.AppendLiteral("@devices.library.songbirdnest.com");
    
    PRUnichar *begin, *end;
    
    for (libId.BeginWriting(&begin, &end); begin < end; ++begin) {
      if (*begin & (~0x7F)) {
        *begin = PRUnichar('_');
      }
    }
    libId.StripChars(FILE_ILLEGAL_CHARACTERS
                     FILE_PATH_SEPARATOR
                     " _-");

    nsCOMPtr<sbIDeviceLibrary> devLib;
    rv = CreateDeviceLibrary(libId, nsnull, getter_AddRefs(devLib));
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<sbIDeviceContent> sbContent;
    rv = GetContent(getter_AddRefs(sbContent));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = sbContent->AddLibrary(devLib);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = devLib->SetProperty(sbWPDDevice::PUID_SBIMEDIAITEM_PROPERTY, puid);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = PushRequest(sbBaseDevice::TransferRequest::REQUEST_MOUNT,
                     nsnull,
                     devLib);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK; 
}

/* void disconnect (); */
NS_IMETHODIMP sbWPDDevice::Disconnect()
{
  nsresult rv;
  
  // The thread requires mPortableDevice! This means we must shutdown the 
  // thread before we release the portable device instance.
  if (mDeviceThread) {
    mDeviceThread->Die();
    mThreadObject->Shutdown();
  }

  NS_IF_RELEASE(mDeviceThread);

  if (mPortableDevice) {
    if (FAILED(mPortableDevice->Close()))
      NS_WARNING("Failed to close PortableDevice instance!");
    mPortableDevice = nsnull;
  }
  
  // clear the device passed in the creation parameters, since
  // PortableDeviceApi.dll wants to go away too fast
  nsCOMPtr<nsIWritablePropertyBag> bag =
    do_QueryInterface(mDeviceProperties, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // silently eat failures
  bag->DeleteProperty(NS_LITERAL_STRING("DevicePointer"));

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
  
  return GetProperty(nsString(WPD_DEVICE_OBJECT_ID), properties, aPrefName, retval);
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
  nsRefPtr<IPortableDeviceCapabilities> deviceCaps;
  if (FAILED(mPortableDevice->Capabilities(getter_AddRefs(deviceCaps))))
    return NS_ERROR_FAILURE;
  sbWPDCapabilitiesBuilder builder(deviceCaps);
  return builder.Create(theCapabilities);
}

/* readonly attribute sbIDeviceContent content; */
NS_IMETHODIMP sbWPDDevice::GetContent(sbIDeviceContent * *aContent)
{
  nsresult rv;
  if (!mDeviceContent) {
    nsRefPtr<sbDeviceContent> deviceContent = sbDeviceContent::New();
    NS_ENSURE_TRUE(deviceContent, NS_ERROR_OUT_OF_MEMORY);
    rv = deviceContent->Init();
    NS_ENSURE_SUCCESS(rv, rv);
    mDeviceContent = deviceContent;
  }
  NS_ADDREF(*aContent = mDeviceContent.get());
  return NS_OK;
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

NS_IMETHODIMP sbWPDDevice::SubmitRequest(PRUint32 aRequest,
                                         nsIPropertyBag2 *aRequestParameters)
{
  nsRefPtr<TransferRequest> transferRequest;
  nsresult rv = CreateTransferRequest(aRequest, 
                                      aRequestParameters, 
                                      getter_AddRefs(transferRequest));
  NS_ENSURE_SUCCESS(rv, rv);

  return PushRequest(transferRequest);
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
    
    PRBool isOk = PR_TRUE;
    
    // PeekRequest returns NS_OK+nsnull if there is no next request
    rv = PeekRequest(getter_AddRefs(nextRequest));
    // Something bad happened terminate the thread
    NS_ENSURE_SUCCESS(rv, PR_FALSE);
  
    nsCOMPtr<sbILibrary> lib;
    if (request->list.get()) {
      lib = do_QueryInterface(request->list);
    }
    
    nsCOMPtr<sbIMediaList> itemList; // request->item as a list
    if (request->item) {
      itemList = do_QueryInterface(request->item);
    }
    nsCOMPtr<sbILibrary> itemLib; // request->item as a library
    if (itemList) {
      itemLib = do_QueryInterface(itemList);
    }
  
    switch (request->type) {
      case TransferRequest::REQUEST_MOUNT:
        rv = MountRequest(request);
        isOk = NS_SUCCEEDED(rv);
        break;
      case TransferRequest::REQUEST_EJECT:
        // MTP devices can't be ejected
        return PR_FALSE;
      case TransferRequest::REQUEST_READ:
        rv = ReadRequest(request);
        isOk = NS_SUCCEEDED(rv);
        break;
      case TransferRequest::REQUEST_SUSPEND:
        break;
      case TransferRequest::REQUEST_WRITE: {
        if (lib.get()) {
          // add item to library
          rv = WriteRequest(request);
          isOk = NS_SUCCEEDED(rv);
        } else {
          // add item to playlist
          rv = AddItemToPlaylist(request->item, request->list, request->index);
          isOk = NS_SUCCEEDED(rv);
        }
        break;
      }
      case TransferRequest::REQUEST_DELETE: {
        if (lib.get()) {
          // remove item from library
          rv = RemoveItem(request->item);
          isOk = NS_SUCCEEDED(rv);
        } else {
          // remove item from list
          rv = RemoveItemFromPlaylist(request->list, request->index);
          isOk = NS_SUCCEEDED(rv);
        }
        break;
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
        isOk = NS_SUCCEEDED(rv);
        break;
      }
      case TransferRequest::REQUEST_UPDATE:
        if (itemLib) {
          /* nothing to update for libraries */
        } else if (itemList) {
          rv = UpdatePlaylist(itemList);
          isOk = NS_SUCCEEDED(rv);
        }
        break;
      case TransferRequest::REQUEST_NEW_PLAYLIST: {
        if (itemLib) {
          /* umm, what? creating a new... library? this makes no sense */
          NS_NOTREACHED("Can't create a new library as a playlist");
          isOk = PR_FALSE;
        } else if (itemList) {
          rv = CreatePlaylist(itemList);
          isOk = NS_SUCCEEDED(rv);
        } else {
          isOk = PR_FALSE;
        }
      }
    }
    
    NS_WARN_IF_FALSE(isOk, "WPD Device Processor Thread is going to die.");

    if (!isOk) {
      return PR_FALSE;
    }
    // keep going
  }
  return PR_TRUE; // let the worker thread know we can keep going
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

nsresult sbWPDDevice::GetProperty(const nsAString &objectID,
                                  IPortableDeviceProperties * properties,
                                  PROPERTYKEY const & propKey,
                                  nsIVariant ** retval)
{
  nsRefPtr<IPortableDeviceKeyCollection> propertyKeys;
  nsresult rv = sbWPDCreatePropertyKeyCollection(propKey, getter_AddRefs(propertyKeys));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsRefPtr<IPortableDeviceValues> propValues;
  rv = properties->GetValues(nsString(objectID).get(), propertyKeys, getter_AddRefs(propValues));
  NS_ENSURE_SUCCESS(rv, rv);
  
  PROPVARIANT propVal = {0};
  PropVariantInit(&propVal);
  
  NS_ENSURE_TRUE(SUCCEEDED(propValues->GetValue(propKey, &propVal)), 
                 NS_ERROR_FAILURE);
  *retval = sbPropertyVariant::New(propVal);
  if (!*retval)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*retval);
  return NS_OK;
}

nsresult sbWPDDevice::GetProperty(const nsAString &objectID,
                                  IPortableDeviceProperties * properties,
                                  nsAString const & key,
                                  nsIVariant ** value)
{
  PROPERTYKEY propKey;
  nsresult rv = PropertyKeyFromString(key, &propKey);
  NS_ENSURE_SUCCESS(rv, rv);
  return GetProperty(objectID, properties, propKey, value);
}

nsresult sbWPDDevice::CreatePlaylist(nsAString const &aName,
                                     nsAString const &aParent,
                                     /*out*/nsAString &aObjId)
{
  HRESULT hr;
  nsresult rv;
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
  
  nsRefPtr<IPortableDeviceCapabilities> caps;
  hr = mPortableDevice->Capabilities(getter_AddRefs(caps));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDevicePropVariantCollection> formats;
  hr = caps->GetSupportedFormats(WPD_CONTENT_TYPE_PLAYLIST,
                                 getter_AddRefs(formats));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<sbPropertyVariant> format = new sbPropertyVariant();
  rv = formats->GetAt(0, format->GetPropVariant());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  hr = values->SetGuidValue(WPD_OBJECT_FORMAT,
                            *format->GetPropVariant()->puuid);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsString fileName(aName);
  fileName.AppendLiteral(".");
  nsCString extension;
  rv = sbWPDGUIDtoFileExtension(*format->GetPropVariant()->puuid, extension);
  if (NS_SUCCEEDED(rv)) {
    fileName.Append(NS_ConvertASCIItoUTF16(extension));
  } else {
    // fallback
    fileName.AppendLiteral("pla");
  }
  
  hr = values->SetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME,
                              fileName.BeginReading());
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
  
  // we can get a create message that really should be an update
  nsString objId = GetWPDDeviceIDFromMediaItem(aPlaylist);
  if (!objId.IsEmpty()) {
    return UpdatePlaylist(aPlaylist);
  }
  
  nsCOMPtr<sbILibrary> library;
  rv = aPlaylist->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString name;
  rv = aPlaylist->GetName(name);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString libraryObjId = GetWPDDeviceIDFromMediaItem(library);
  NS_ENSURE_TRUE(!libraryObjId.IsEmpty(), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDeviceContent> content;
  hr = mPortableDevice->Content(getter_AddRefs(content));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  // the parent folder is a child of the library (storage) guid
  nsString parentObjId = sbWPDGetFolderForContentType(WPD_CONTENT_TYPE_PLAYLIST,
                                                      libraryObjId,
                                                      content);

  // do what we can before actually creating the playlist in case something
  // fails (so we don't get a playlist on the device we don't know about)
  nsRefPtr<IPortableDeviceProperties> properties;
  hr = content->Properties(getter_AddRefs(properties));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  nsRefPtr<IPortableDeviceKeyCollection> keys;
  rv = sbWPDCreatePropertyKeyCollection(WPD_OBJECT_PERSISTENT_UNIQUE_ID,
                                        getter_AddRefs(keys));
  NS_ENSURE_SUCCESS(rv, rv);

  // okay, we've done what we can, actually create the playlist now
  nsString objid;
  rv = CreatePlaylist(name, parentObjId, objid);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsRefPtr<IPortableDeviceValues> values;
  hr = properties->GetValues(objid.BeginReading(), keys, getter_AddRefs(values));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  LPWSTR ppuid = nsnull;
  hr = values->GetStringValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, &ppuid);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsString puid = nsDependentString(ppuid);
  ::CoTaskMemFree(ppuid);
  
  rv = aPlaylist->SetProperty(PUID_SBIMEDIAITEM_PROPERTY, puid);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult sbWPDDevice::UpdatePlaylist(sbIMediaList* aPlaylist)
{
  NS_ENSURE_ARG_POINTER(aPlaylist);
  NS_ENSURE_STATE(mPortableDevice);

  nsresult rv;
  HRESULT hr;
  
  // check if the playlist is already gone from the library (i.e. this is a
  // post-delete update notification)
  nsString guid;
  rv = aPlaylist->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbILibrary> ownerLibrary;
  rv = aPlaylist->GetLibrary(getter_AddRefs(ownerLibrary));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbIMediaItem> item;
  rv = ownerLibrary->GetMediaItem(guid, getter_AddRefs(item));
  if (!item) {
    // this playlist doesn't actually exist
    return NS_OK;
  }
  
  nsString objId = GetWPDDeviceIDFromMediaItem(aPlaylist);
  if (objId.IsEmpty())
    return CreatePlaylist(aPlaylist);
  
  nsString name;
  rv = aPlaylist->GetName(name);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsRefPtr<IPortableDeviceContent> content;
  hr = mPortableDevice->Content(getter_AddRefs(content));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDeviceProperties> properties;
  hr = content->Properties(getter_AddRefs(properties));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDeviceValues> values;
  hr = CoCreateInstance(CLSID_PortableDeviceValues,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IPortableDeviceValues,
                        getter_AddRefs(values));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  hr = values->SetStringValue(WPD_OBJECT_NAME, name.BeginReading());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  // in order to figure out the extension, we must first have the object format
  nsRefPtr<IPortableDeviceKeyCollection> formatKey;
  rv = sbWPDCreatePropertyKeyCollection(WPD_OBJECT_FORMAT,
                                        getter_AddRefs(formatKey));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsRefPtr<IPortableDeviceValues> formatValues;
  hr = properties->GetValues(objId.BeginReading(),
                             formatKey,
                             getter_AddRefs(formatValues));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  GUID formatGuid;
  hr = formatValues->GetGuidValue(WPD_OBJECT_FORMAT, &formatGuid);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsString fileName(name);
  fileName.AppendLiteral(".");
  nsCString extension;
  rv = sbWPDGUIDtoFileExtension(formatGuid, extension);
  if (NS_SUCCEEDED(rv)) {
    fileName.Append(NS_ConvertASCIItoUTF16(extension));
  } else {
    fileName.AppendLiteral("pla"); // fallback :|
  }
  
  hr = values->SetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME,
                              fileName.BeginReading());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  nsRefPtr<IPortableDeviceValues> results;
  
  hr = properties->SetValues(objId.BeginReading(),
                             values,
                             getter_AddRefs(results));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
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

/**
 * Returns the content and format type for the Songbird item. This uses
 * the MIME type if present or the extension if not. If neither the MIME
 * type or extension are known then NS_ERROR_NOT_AVAILABLE is returned
 */
static nsresult GetWPDContentType(sbIMediaItem * item,
                                  GUID & contentType,
                                  GUID & formatType)
{
  struct MimeTypeMediaMapItem
  {
    WCHAR * MimeType;
    GUID WPDContentType;
    GUID WPDFormatType;
  };
  static MimeTypeMediaMapItem const MimeTypeMediaMap[] = 
  {
    {L"audio/x-ms-wma;", WPD_CONTENT_TYPE_AUDIO, WPD_OBJECT_FORMAT_WMA},
    {L"audio/mp3;", WPD_CONTENT_TYPE_AUDIO, WPD_OBJECT_FORMAT_MP3},
    {L"audio/mpeg;", WPD_CONTENT_TYPE_AUDIO, WPD_OBJECT_FORMAT_MP3},
    {L"audio/x-wav;", WPD_CONTENT_TYPE_AUDIO, WPD_OBJECT_FORMAT_WAVE},
    {L"audio/x-aiff;", WPD_CONTENT_TYPE_AUDIO, WPD_OBJECT_FORMAT_AIFF}
  };
  static PRUint32 const MimeTypeMediaMapSize = NS_ARRAY_LENGTH(MimeTypeMediaMap);

  // Attempt to find the MIME type
  nsString PUID;
  nsresult rv = item->GetProperty(sbWPDDevice::PUID_SBIMEDIAITEM_PROPERTY,
                                  PUID);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString contentMimeType;
  rv = item->GetContentType(contentMimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!contentMimeType.IsEmpty()) {
    for (int index = 0; index < MimeTypeMediaMapSize; ++index) {
      MimeTypeMediaMapItem const & item = MimeTypeMediaMap[index];
      if (contentMimeType.Equals(item.MimeType)) {
        contentType = item.WPDContentType;
        formatType = item.WPDFormatType;
        return NS_OK;
      }
    }
  }  
  // If we fail to get it from the mime type, attempt to get it from the file extension.
  nsCOMPtr<nsIURI> contentURI;
  rv = item->GetContentSrc(getter_AddRefs(contentURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIThread> target;
  rv = NS_GetMainThread(getter_AddRefs(target));

  nsCOMPtr<nsIURL> proxiedURL;
  rv = do_GetProxyForObject(target,
                            NS_GET_IID(nsIURL),
                            contentURI,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(proxiedURL));

  nsCString fileExtension;
  rv = proxiedURL->GetFileExtension(fileExtension);
  NS_ENSURE_SUCCESS(rv, rv);

  return sbWPDFileExtensionToGUIDs(fileExtension, contentType, formatType);
}

nsresult sbWPDDevice::RemoveItem(sbIMediaItem *aItem)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(aItem);

  // if we can't find the object id, it's perfectly fine - it means the item
  // has already been removed.
  nsString objid = GetWPDDeviceIDFromMediaItem(aItem);
  if (!objid.IsEmpty()) {
    rv = RemoveItem(objid);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  PRInt64 contentLength = 0;
  GUID contentType;
  GUID contentFormat;
  if (NS_SUCCEEDED(aItem->GetContentLength(&contentLength)) &&
      NS_SUCCEEDED(GetWPDContentType(aItem, contentType, contentFormat))) {
    if (contentType == WPD_CONTENT_TYPE_AUDIO) {
      DeviceStatistics().SubAudioUsed(contentLength);
    }
    else if (contentType == WPD_CONTENT_TYPE_VIDEO) {
      DeviceStatistics().SubVideoUsed(contentLength);
    }
    else {
      DeviceStatistics().SubOtherUsed(contentLength);
    }
  }

  nsString nullString;
  nullString.SetIsVoid(PR_TRUE);
  rv = aItem->SetProperty(PUID_SBIMEDIAITEM_PROPERTY, nullString);
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

// This function performs and incremental update of the playlist.  If the
// playlist has been modified on another system, this function imports the
// differences.
nsresult sbWPDDevice::ImportPlaylist(sbILibrary* aLibrary,
                                     nsAString const &aObjId)
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_STATE(mPortableDevice);

  HRESULT hr;
  nsresult rv = NS_OK;

  // Get the device content.
  nsRefPtr<IPortableDeviceContent> content;
  hr = mPortableDevice->Content(getter_AddRefs(content));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // Get the content properties.
  nsRefPtr<IPortableDeviceProperties> properties;
  hr = content->Properties(getter_AddRefs(properties));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // Get the playlist properties.
  nsRefPtr<IPortableDeviceKeyCollection> keys;
  rv = sbWPDCreatePropertyKeyCollection(WPD_OBJECT_PERSISTENT_UNIQUE_ID,
                                        getter_AddRefs(keys));
  NS_ENSURE_SUCCESS(rv, rv);
  hr = keys->Add(WPD_OBJECT_NAME);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  nsRefPtr<IPortableDeviceValues> values;
  hr = properties->GetValues(aObjId.BeginReading(),
                             keys,
                             getter_AddRefs(values));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // Get the playlist PUID.
  LPWSTR pPlaylistPUID = nsnull;
  hr = values->GetStringValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, &pPlaylistPUID);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  nsString playlistPUID = nsDependentString(pPlaylistPUID);
  ::CoTaskMemFree(pPlaylistPUID);

  // Get the playlist name.
  LPWSTR pPlaylistName = nsnull;
  hr = values->GetStringValue(WPD_OBJECT_NAME, &pPlaylistName);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  nsString playlistName = nsDependentString(pPlaylistName);
  ::CoTaskMemFree(pPlaylistName);

  // Get the list of playlist items.
  nsRefPtr<IPortableDevicePropVariantCollection> items;
  rv = GetPlaylistReferences(aObjId, getter_AddRefs(items));
  NS_ENSURE_SUCCESS(rv, rv);

  // Look for the playlist media list.  Create the media list if not found.
  nsCOMPtr<sbIMediaItem> item;
  nsCOMPtr<sbIMediaList> playlistML;
  rv = sbWPDGetMediaItemByPUID(aLibrary, playlistPUID, getter_AddRefs(item));
  if (NS_SUCCEEDED(rv)) {
    playlistML = do_QueryInterface(item, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    rv = aLibrary->CreateMediaList(NS_LITERAL_STRING("simple"),
                                   nsnull,
                                   getter_AddRefs(playlistML));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = playlistML->SetProperty(PUID_SBIMEDIAITEM_PROPERTY, playlistPUID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Set device playlist listener to ignore changes during import.
  nsRefPtr<sbBaseDeviceMediaListListener> playlistListener;
  if (mMediaListListeners.Get(playlistML, &playlistListener)) {
    rv = playlistListener->SetIgnoreListener(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Set the playlist name, ignoring errors.
  playlistML->SetName(playlistName);

  // Import each playlist item, ignoring errors.
  DWORD itemCount;
  hr = items->GetCount(&itemCount);
  if (SUCCEEDED(hr)) {
    for (DWORD i = 0; i < itemCount; i++) {
      ImportPlaylistItem(aLibrary, playlistML, items, i);
    }
  }
  else {
    rv = NS_ERROR_FAILURE;
  }

  // Remove any playlist media list items left over.  This could happen if any
  // items were moved or deleted.
  if (NS_SUCCEEDED(rv)) {
    PRUint32 length;
    rv = playlistML->GetLength(&length);
    if (NS_SUCCEEDED(rv) && (length > 0)) {
      for (PRInt32 i = length - 1; i >= itemCount; i--) {
        playlistML->RemoveByIndex(i);
      }
    }
  }

  // Set device playlist listener to no longer ignore changes.
  if (playlistListener) {
    rv = playlistListener->SetIgnoreListener(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return rv;
}

nsresult sbWPDDevice::ImportPlaylistItem
                        (sbILibrary* aLibrary,
                         sbIMediaList* aPlaylistML,
                         IPortableDevicePropVariantCollection* aItems,
                         DWORD aItemIndex)
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(aPlaylistML);
  NS_ENSURE_ARG_POINTER(aItems);
  NS_ENSURE_STATE(mPortableDevice);

  HRESULT hr;
  nsresult rv;

  // Get the device content.
  nsRefPtr<IPortableDeviceContent> content;
  hr = mPortableDevice->Content(getter_AddRefs(content));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // Get the content properties.
  nsRefPtr<IPortableDeviceProperties> properties;
  hr = content->Properties(getter_AddRefs(properties));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // Get the item object ID.
  nsRefPtr<sbPropertyVariant> variant = sbPropertyVariant::New();
  hr = aItems->GetAt(aItemIndex, variant->GetPropVariant());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  nsString objId;
  rv = variant->GetAsAString(objId);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the item properties.
  nsRefPtr<IPortableDeviceKeyCollection> keys;
  rv = sbWPDCreatePropertyKeyCollection(WPD_OBJECT_PERSISTENT_UNIQUE_ID,
                                        getter_AddRefs(keys));
  NS_ENSURE_SUCCESS(rv, rv);
  nsRefPtr<IPortableDeviceValues> values;
  hr = properties->GetValues(objId.BeginReading(),
                             keys,
                             getter_AddRefs(values));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // Get the item PUID.
  LPWSTR pItemPUID = nsnull;
  hr = values->GetStringValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, &pItemPUID);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  nsString itemPUID = nsDependentString(pItemPUID);
  ::CoTaskMemFree(pItemPUID);

  // Get the media item.
  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = sbWPDGetMediaItemByPUID(aLibrary, itemPUID, getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  // Append item to end of media list if it comes after the end.  Otherwise,
  // insert it inside the media list.
  PRUint32 length;
  rv = aPlaylistML->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aItemIndex >= length) {
    rv = aPlaylistML->Add(mediaItem);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    // Get the media list item at the insert point.
    nsCOMPtr<sbIMediaItem> mediaItemAtIndex;
    rv = aPlaylistML->GetItemByIndex(aItemIndex,
                                     getter_AddRefs(mediaItemAtIndex));
    NS_ENSURE_SUCCESS(rv, rv);

    // Insert item in media list if it's not already present there.
    PRBool equals;
    rv = mediaItem->Equals(mediaItemAtIndex, &equals);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!equals) {
      nsCOMPtr<sbIOrderableMediaList> playlistOML =
                                        do_QueryInterface(aPlaylistML, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = playlistOML->InsertBefore(aItemIndex, mediaItem);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult sbWPDDevice::GetPlaylistReferences(sbIMediaList* aList,
                                            /* out */ IPortableDevicePropVariantCollection** aItems)
{
  NS_ENSURE_ARG_POINTER(aList);
  NS_ENSURE_ARG_POINTER(aItems);

  nsString objId = GetWPDDeviceIDFromMediaItem(aList);
  NS_ENSURE_TRUE(!objId.IsEmpty(), NS_ERROR_FAILURE);

  return GetPlaylistReferences(objId, aItems);
}

nsresult sbWPDDevice::GetPlaylistReferences(nsAString const &aObjId,
                                            /* out */ IPortableDevicePropVariantCollection** aItems)
{
  HRESULT hr;
  NS_ENSURE_STATE(mPortableDevice);
  NS_ENSURE_ARG_POINTER(aItems);

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
  hr = properties->GetValues(aObjId.BeginReading(),
                             keys,
                             getter_AddRefs(values));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  hr = values->GetIPortableDevicePropVariantCollectionValue(WPD_OBJECT_REFERENCES,
                                                            aItems);
  if (hr == HRESULT_FROM_WIN32(ERROR_EMPTY)) {
    // the drive is too dumb to create a new one for us
    hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IPortableDevicePropVariantCollection,
                          (LPVOID*)aItems);
  }
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

nsresult sbWPDDevice::CreateAndDispatchEventFromHRESULT(HRESULT hr, 
                                                        nsIVariant *aData)
{
  NS_ENSURE_ARG_POINTER(aData);

  nsresult rv;
  PRUint32 eventType = 0;

  nsCOMPtr<sbIDeviceManager2> manager =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  switch(hr) {
    case STG_E_ACCESSDENIED:
      eventType = sbIDeviceEvent::EVENT_DEVICE_ACCESS_DENIED;
    break;

    case STG_E_CANTSAVE:
    case STG_E_MEDIUMFULL:
      eventType = sbIDeviceEvent::EVENT_DEVICE_NOT_ENOUGH_FREESPACE;
    break;

    case E_PENDING:
    case STG_E_INVALIDPOINTER:
    case STG_E_REVERTED:
      eventType = sbIDeviceEvent::EVENT_DEVICE_NOT_AVAILABLE;
    break;

    case STG_E_WRITEFAULT:
      eventType = sbIDeviceEvent::EVENT_DEVICE_MEDIA_WRITE_FAILED;
    break;
    
    // No event to dispatch for unknown HRESULT values.
    default:
      return NS_OK;
  }

  nsCOMPtr<sbIDeviceEvent> deviceEvent;
  rv = manager->CreateEvent(eventType, aData, static_cast<sbIDevice*>(this),
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
static nsresult CopynsIStream2IStream(sbDeviceStatus * status,
                                      sbWPDDevice * device,
                                      sbBaseDevice::TransferRequest * request,
                                      PRInt64 contentLength,
                                      nsIInputStream * input,
                                      IStream * output,
                                      HRESULT * hr,
                                      PRUint32 bufferSize)
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
    HRESULT _hr = output->Write(buffer, bytesRead, &bytesWritten);

    if (FAILED(_hr) || bytesWritten < bytesRead) {
      rv = NS_ERROR_FAILURE;
      output->Revert();
      *hr = _hr;
      break;
    }

    current += bytesWritten;
    double const progress = current / length * 100.0;
    nsCOMPtr<nsIWritableVariant> var =
      do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    var->SetAsISupports(request->item);
    device->CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_TRANSFER_PROGRESS,
                                   var);
    status->Progress(progress);
    
  }
  delete [] buffer;
  // Something bad happened
  if (bytesRead)
    return rv;
  return NS_OK;
}

static nsresult CopyIStream2nsIStream(sbDeviceStatus * status,
                                      sbWPDDevice * device,
                                      IStream * input,
                                      nsIOutputStream * output,
                                      PRUint32 bufferSize = 64 * 1024)
{
  char * buffer = new char[bufferSize];
  double current = 0;
  ULONG bytesRead;
  nsresult rv = NS_OK;

  PRBool enableProgress = PR_TRUE;
  double length = -1;
  STATSTG streamStats = {0};
  
  HRESULT hr = input->Stat(&streamStats, STATFLAG_NONAME);
  if(FAILED(hr)) {
    enableProgress = PR_FALSE;
  }
  else {
    length = streamStats.cbSize.QuadPart;
  }

  for (hr = input->Read(buffer, bufferSize, &bytesRead);
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

    current += bytesWritten;
    if(enableProgress) {
      double const progress = current / length * 100.0;
      nsCOMPtr<nsIWritableVariant> var =
        do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      var->SetAsDouble(progress);
      device->CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_TRANSFER_PROGRESS,
        var);
      status->Progress(progress);
    }
  }

  delete [] buffer;

  // Something bad happened
  if (bytesRead)
    return rv;
  return NS_OK;
}

static nsresult SetParentProperty(IPortableDeviceContent * aContent,
                                  sbIMediaItem * aItem,
                                  const GUID & aContentType,
                                  IPortableDeviceValues * aProperties)
{
  nsresult rv;
  HRESULT hr;
  
  nsCOMPtr<sbILibrary> deviceLibrary;
  rv = aItem->GetLibrary(getter_AddRefs(deviceLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString libraryPuid, libraryObjId;
  deviceLibrary->GetProperty(sbWPDDevice::PUID_SBIMEDIAITEM_PROPERTY,
                             libraryPuid);
  if (libraryPuid.IsEmpty())
    return NS_ERROR_FAILURE;

  rv = sbWPDObjectIDFromPUID(aContent, libraryPuid, libraryObjId);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_FALSE(libraryObjId.IsEmpty(), NS_ERROR_FAILURE);
  
  nsString parentObjId = sbWPDGetFolderForContentType(aContentType,
                                                      libraryObjId,
                                                      aContent);
  NS_ENSURE_SUCCESS(rv, rv);
  
  hr = aProperties->SetStringValue(WPD_OBJECT_PARENT_ID, parentObjId.get());
  if (FAILED(hr))
    return NS_ERROR_FAILURE;

  return NS_OK;
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

  nsCOMPtr<nsIThread> target;
  rv = NS_GetMainThread(getter_AddRefs(target));

  nsCOMPtr<nsIURI> proxiedURI;
  rv = do_GetProxyForObject(target,
                            NS_GET_IID(nsIURI),
                            theURI,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(proxiedURI));

  nsCOMPtr<nsIURL> theURL = do_QueryInterface(proxiedURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURL> proxiedURL;
  rv = do_GetProxyForObject(target,
                            NS_GET_IID(nsIURL),
                            theURL,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(proxiedURL));

  nsCString fileName;
  rv = proxiedURL->GetFileName(fileName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString extension;
  rv = proxiedURL->GetFileExtension(extension);
  NS_ENSURE_SUCCESS(rv, rv);
  
  //If there's an extension, cut it off for the name of the object.
  nsCString objectName(fileName);
  if(extension.Length())
    objectName.Cut(objectName.Length() - extension.Length() - 1, extension.Length() + 1);

  nsCOMPtr<nsINetUtil> netUtil;
  netUtil = do_GetService("@mozilla.org/network/util;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCString decodedObjectName;
  rv = netUtil->UnescapeString(objectName,
    nsINetUtil::ESCAPE_URL_SKIP_CONTROL,
    decodedObjectName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString decodedFileName;
  rv = netUtil->UnescapeString(fileName,
    nsINetUtil::ESCAPE_URL_SKIP_CONTROL,
    decodedFileName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (FAILED(properties->SetStringValue(WPD_OBJECT_NAME, NS_ConvertUTF8toUTF16(decodedObjectName).get())))
    return NS_ERROR_FAILURE;

  if (FAILED(properties->SetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, NS_ConvertUTF8toUTF16(decodedFileName).get())))
    return NS_ERROR_FAILURE;

  return rv;
}

static nsresult SetStandardProperties(sbIMediaItem * item,
                                      IPortableDeviceValues * properties)
{
  nsresult rv = NS_ERROR_UNEXPECTED;
  HRESULT hr = S_FALSE;

  nsString propValue;
  PROPERTYKEY propKey;

  // content url
  /*rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL), propValue);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = sbWPDStandardItemPropertyToPropertyKey(SB_PROPERTY_CONTENTURL, propKey);
  NS_ENSURE_SUCCESS(rv, rv);

  hr = properties->SetStringValue(propKey, propValue.get());
  NS_ENSURE_SUCCESS(SUCCEEDED(hr), NS_ERROR_FAILURE);*/

  // track name
  rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME), propValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbWPDStandardItemPropertyToPropertyKey(SB_PROPERTY_TRACKNAME, propKey);
  NS_ENSURE_SUCCESS(rv, rv);
  
  hr = properties->SetStringValue(propKey, propValue.get());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // album name
  rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME), propValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbWPDStandardItemPropertyToPropertyKey(SB_PROPERTY_ALBUMNAME, propKey);
  NS_ENSURE_SUCCESS(rv, rv);

  hr = properties->SetStringValue(propKey, propValue.get());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  // artist name
  rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME), propValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbWPDStandardItemPropertyToPropertyKey(SB_PROPERTY_ARTISTNAME, propKey);
  NS_ENSURE_SUCCESS(rv, rv);

  hr = properties->SetStringValue(propKey, propValue.get());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // duration
  rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_DURATION), propValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbWPDStandardItemPropertyToPropertyKey(SB_PROPERTY_DURATION, propKey);
  NS_ENSURE_SUCCESS(rv, rv);

  hr = properties->SetStringValue(propKey, propValue.get());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  // genre
  rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_GENRE), propValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbWPDStandardItemPropertyToPropertyKey(SB_PROPERTY_GENRE, propKey);
  NS_ENSURE_SUCCESS(rv, rv);

  hr = properties->SetStringValue(propKey, propValue.get());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // track no 
  rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKNUMBER), propValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbWPDStandardItemPropertyToPropertyKey(SB_PROPERTY_TRACKNUMBER, propKey);
  NS_ENSURE_SUCCESS(rv, rv);

  hr = properties->SetStringValue(propKey, propValue.get());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // composer
  rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_COMPOSERNAME), propValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbWPDStandardItemPropertyToPropertyKey(SB_PROPERTY_COMPOSERNAME, propKey);
  NS_ENSURE_SUCCESS(rv, rv);

  hr = properties->SetStringValue(propKey, propValue.get());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // lyrics
  rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_LYRICS), propValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbWPDStandardItemPropertyToPropertyKey(SB_PROPERTY_LYRICS, propKey);
  NS_ENSURE_SUCCESS(rv, rv);

  hr = properties->SetStringValue(propKey, propValue.get());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // last played time
  rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_LASTPLAYTIME), propValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbWPDStandardItemPropertyToPropertyKey(SB_PROPERTY_LASTPLAYTIME, propKey);
  NS_ENSURE_SUCCESS(rv, rv);

  hr = properties->SetStringValue(propKey, propValue.get());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  
  // play count
  rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_PLAYCOUNT), propValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbWPDStandardItemPropertyToPropertyKey(SB_PROPERTY_PLAYCOUNT, propKey);
  NS_ENSURE_SUCCESS(rv, rv);

  hr = properties->SetStringValue(propKey, propValue.get());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // skip count
  rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_SKIPCOUNT), propValue);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = sbWPDStandardItemPropertyToPropertyKey(SB_PROPERTY_SKIPCOUNT, propKey);
  NS_ENSURE_SUCCESS(rv, rv);
    
  hr = properties->SetStringValue(propKey, propValue.get());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // rating
  rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_RATING), propValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbWPDStandardItemPropertyToPropertyKey(SB_PROPERTY_RATING, propKey);
  NS_ENSURE_SUCCESS(rv, rv);

  hr = properties->SetStringValue(propKey, propValue.get());
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  return NS_OK;
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
    
  GUID WPDContentType;
  GUID WPDFormatType;
  nsresult rv = GetWPDContentType(item,
                                  WPDContentType,
                                  WPDFormatType);

  // We can't determine the type of the content, fire event to indicate that the
  // file will not be transferred.
  if (NS_FAILED(rv)) {
    nsCOMPtr<nsIWritableVariant> var =
      do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    var->SetAsAString(GetItemID(item));
    CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_MEDIA_WRITE_UNSUPPORTED_TYPE,
                           var);
    return SB_ERROR_MEDIA_TYPE_NOT_SUPPORTED;
  }

  if (FAILED(properties->SetGuidValue(WPD_OBJECT_CONTENT_TYPE,
                                      WPDContentType)) ||
      FAILED(properties->SetGuidValue(WPD_OBJECT_FORMAT,
                                      WPDFormatType))) {
    return NS_ERROR_FAILURE;
  }

  rv = SetParentProperty(content, item, WPDContentType, properties);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = SetContentLength(item, properties);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = SetContentName(item, properties);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = SetStandardProperties(item, properties);
  NS_ENSURE_SUCCESS(rv, rv);

  properties.forget(itemProperties);
  return NS_OK;
}

nsresult sbWPDDevice::CreateDeviceObjectFromMediaItem(sbDeviceStatus * status,
                                                      TransferRequest * request)
{
  PRInt64 contentLength;
  nsresult rv = request->item->GetContentLength(&contentLength);
  NS_ENSURE_SUCCESS(rv, rv);
  status->CurrentOperation(NS_LITERAL_STRING("Copying"));
  status->SetItem(request->item);
  status->SetList(request->list);
  status->StateMessage(NS_LITERAL_STRING("Starting"));
  status->WorkItemProgress(request->batchIndex);
  status->WorkItemProgressEndCount(request->batchCount);
  nsRefPtr<IPortableDeviceContent> content;
  rv = mPortableDevice->Content(getter_AddRefs(content));
  SB_ENSURE_NO_DEVICE_ERROR_GENERIC(rv, this);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<IPortableDeviceValues> properties;
  rv = GetPropertiesFromItem(content,
                             request->item,
                             request->list,
                             getter_AddRefs(properties));
  SB_ENSURE_NO_DEVICE_ERROR_GENERIC(rv, this);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<IStream> streamWriter;
  DWORD optimalBufferSize;
  if (FAILED(content->CreateObjectWithPropertiesAndData(properties,
                                                        getter_AddRefs(streamWriter),
                                                        &optimalBufferSize,
                                                        NULL))) {
    SB_ENSURE_NO_DEVICE_ERROR_GENERIC(NS_ERROR_FAILURE, this);
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIThread> target;
  rv = NS_GetMainThread(getter_AddRefs(target));

  nsCOMPtr<sbIMediaItem> proxiedItem;
  rv = do_GetProxyForObject(target,
                            NS_GET_IID(sbIMediaItem),
                            request->item,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(proxiedItem));

  nsCOMPtr<nsIInputStream> inputStream;
  rv = proxiedItem->OpenInputStream(getter_AddRefs(inputStream));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<IPortableDeviceDataStream> portableDataStream;
  if (FAILED(streamWriter->QueryInterface(__uuidof(IPortableDeviceDataStream), getter_AddRefs(portableDataStream)))) {
    SB_ENSURE_NO_DEVICE_ERROR_GENERIC(NS_ERROR_FAILURE, this);
    return NS_ERROR_FAILURE;
  }
  
  status->StateMessage(NS_LITERAL_STRING("InProgress"));

  // This function handles firing it's own error events.
  HRESULT hr = S_FALSE;
  rv = CopynsIStream2IStream(status,
                             this,
                             request,
                             contentLength,
                             inputStream,
                             streamWriter,
                             &hr,
                             optimalBufferSize);

  streamWriter->Commit(STGC_DEFAULT);

  if(NS_FAILED(rv)) {
    nsresult rv2;
    nsCOMPtr<nsIWritableVariant> var = do_CreateInstance(NS_VARIANT_CONTRACTID, &rv2);
    NS_ENSURE_SUCCESS(rv2, rv2);

    rv2 = var->SetAsISupports(request->item);
    NS_ENSURE_SUCCESS(rv2, rv2);
    
    CreateAndDispatchEventFromHRESULT(hr, var);
  }

  NS_ENSURE_SUCCESS(rv, rv);

  LPWSTR newObjectID = NULL;
  hr = portableDataStream->GetObjectID(&newObjectID);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_UNEXPECTED);

  nsString objectID(newObjectID);
  ::CoTaskMemFree(newObjectID);

  nsRefPtr<IPortableDeviceProperties> objectProps;
  hr = content->Properties(getter_AddRefs(objectProps));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIVariant> objectPersistentID;
  rv = GetProperty(objectID, 
                   objectProps, 
                   WPD_OBJECT_PERSISTENT_UNIQUE_ID, 
                   getter_AddRefs(objectPersistentID));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString strObjectPersistentID;
  rv = objectPersistentID->GetAsAString(strObjectPersistentID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = request->item->SetProperty(sbWPDDevice::PUID_SBIMEDIAITEM_PROPERTY, 
                         strObjectPersistentID);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * Returns the device's ID as a string
 */
nsresult GetDeviceIDAsString(sbWPDDevice * device,
                             nsString & deviceID)
{
  NS_ENSURE_ARG_POINTER(device);
  nsID * id;
  if (NS_FAILED(device->GetId(&id)))
    return NS_ERROR_FAILURE;
  
  deviceID = NS_ConvertASCIItoUTF16(id->ToString());
  return NS_OK;
}

nsresult sbWPDDevice::WriteRequest(TransferRequest * request)
{
  nsresult rv;
  nsCOMPtr<nsIWritableVariant> var =
    do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  var->SetAsISupports(request->item);
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_MEDIA_WRITE_START,
                         var);  
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_TRANSFER_START,
                         var);
                    

  nsString deviceIDStr;
  rv = GetDeviceIDAsString(this, deviceIDStr);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbDeviceStatus> status =
    sbDeviceStatus::New(deviceIDStr);

  rv = CreateDeviceObjectFromMediaItem(status,
                                       request);
  
  PRInt64 contentLength = 0;
  GUID contentType;
  GUID contentFormat;
  if (NS_SUCCEEDED(rv) && 
      NS_SUCCEEDED(request->item->GetContentLength(&contentLength)) &&
      NS_SUCCEEDED(GetWPDContentType(request->item, contentType, contentFormat))) {
    if (contentType == WPD_CONTENT_TYPE_AUDIO) {
      DeviceStatistics().AddAudioUsed(contentLength);
    }
    else if (contentType == WPD_CONTENT_TYPE_VIDEO) {
      DeviceStatistics().AddVideoUsed(contentLength);
    }
    else {
      DeviceStatistics().AddOtherUsed(contentLength);
    }
  }
  // Operation is complete regardless of any errors
  status->StateMessage(NS_LITERAL_STRING("Completed"));
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_TRANSFER_END,
                         var);    
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_MEDIA_WRITE_END,
                         var);      

  // XXXAus: Failures are handled within 
  // CreateDeviceObjectFromMediaItem() and methods it calls.

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
  var->SetAsISupports(request->item);
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
  
  nsCOMPtr<nsIOutputStream> outputStream;

  nsString deviceIDStr;
  rv = GetDeviceIDAsString(this, deviceIDStr);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbDeviceStatus> status =
    sbDeviceStatus::New(deviceIDStr);

  if(request->item &&
     !request->data) {
    rv = request->item->OpenOutputStream(getter_AddRefs(outputStream));
  }
  else if(request->data) {
    nsCOMPtr<nsIFile> file;
    nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(request->data, &rv);
    PRBool exists = PR_FALSE;
    if(NS_SUCCEEDED(rv) && fileURL &&
       NS_SUCCEEDED(fileURL->GetFile(getter_AddRefs(file))) &&
       NS_SUCCEEDED(file->Exists(&exists)) &&
       !exists) {
      rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream), file);
    }
    else if(exists) {
      return NS_OK;
    }
  }
  else {
    // We simply don't have enough data to complete the operation.
    rv = NS_ERROR_UNEXPECTED;
  }

  if(FAILED(rv)) {
    status->StateMessage(NS_LITERAL_STRING("Failed"));
    CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_MEDIA_READ_FAILED,
                           var);
    // Operation is complete regardless of errors.
    status->StateMessage(NS_LITERAL_STRING("Completed"));
    CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_TRANSFER_END,
                           var);    
    CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_MEDIA_READ_END,
                           var);
    return rv;
  }

  rv = CopyIStream2nsIStream(status,
                             this,
                             inputStream,
                             outputStream,
                             optimalBufferSize); 

  if(FAILED(rv)) {
    status->StateMessage(NS_LITERAL_STRING("Failed"));
    CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_MEDIA_READ_FAILED,
                           var);
  }

  // Operation is complete regardless of errors.
  status->StateMessage(NS_LITERAL_STRING("Completed"));
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_TRANSFER_END,
                         var);    
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_MEDIA_READ_END,
                         var);

  return NS_OK;
}

inline
void AddObjectSize(IPortableDeviceValues * values, 
                   sbPropertyVariant * propVar,
                   PRUint64 & total)
{
  if (SUCCEEDED(values->GetValue(WPD_OBJECT_SIZE, propVar->GetPropVariant()))) {
    PRUint64 val;
    if (NS_SUCCEEDED(propVar->GetAsUint64(&val)))
      total += val;
    PropVariantClear(propVar->GetPropVariant());
  }
}

nsresult sbWPDDevice::MountRequest(TransferRequest * request) {
  NS_ENSURE_ARG_POINTER(request);
  NS_ENSURE_STATE(mPortableDevice);

  nsresult rv;
  nsCOMPtr<sbILibrary> library = do_QueryInterface(request->list, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the library wpd puid (this tells us which storage is associated
  // with the library).
  nsString libraryPUID;
  rv = library->GetProperty(sbWPDDevice::PUID_SBIMEDIAITEM_PROPERTY,
                            libraryPUID);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<IPortableDeviceContent> content;
  HRESULT hr = mPortableDevice->Content(getter_AddRefs(content));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  nsRefPtr<IPortableDeviceProperties> properties;
  hr = content->Properties(getter_AddRefs(properties));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // Get the WPD ID of the storage based on it's PUID.
  nsString libraryObjectID;
  rv = sbWPDObjectIDFromPUID(content, libraryPUID, libraryObjectID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Ignore listener events during mount.
   rv = mLibraryListener->SetIgnoreListener(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Add all the PROPERTYKEYs we want to read from the objects.
  nsRefPtr<IPortableDeviceKeyCollection> keys;

  rv = sbWPDCreatePropertyKeyCollection(WPD_OBJECT_CONTENT_TYPE, getter_AddRefs(keys));
  NS_ENSURE_SUCCESS(rv, rv);

  nsTArray<PROPERTYKEY> keyArray;
  keyArray.AppendElement(WPD_OBJECT_CONTENT_TYPE);

  // Standard set of PROPERTYKEYs we will attempt to fetch from items.
  static PROPERTYKEY standardKeys[] = 
    { WPD_OBJECT_FORMAT,
      WPD_OBJECT_PERSISTENT_UNIQUE_ID,
      WPD_OBJECT_ORIGINAL_FILE_NAME,
      WPD_MEDIA_TITLE,
      WPD_MUSIC_ALBUM,
      WPD_MEDIA_ARTIST,
      WPD_MEDIA_DURATION,
      WPD_MEDIA_GENRE,
      WPD_MUSIC_TRACK,
      WPD_MEDIA_COMPOSER,
      WPD_MUSIC_LYRICS,
      WPD_MEDIA_LAST_ACCESSED_TIME,
      WPD_MEDIA_USE_COUNT,
      WPD_MEDIA_SKIP_COUNT,
      WPD_MEDIA_STAR_RATING,
      WPD_OBJECT_SIZE };

  const PRUint32 standardKeysCount = NS_ARRAY_LENGTH(standardKeys);
  for(PRUint32 current = 0; current < standardKeysCount; ++current) {
    hr = keys->Add(standardKeys[current]);
    NS_ENSURE_SUCCESS(SUCCEEDED(hr), NS_ERROR_FAILURE);
    
    keyArray.AppendElement(standardKeys[current]);
  }

  // Attempt to do this with the bulk interface.
  nsRefPtr<IPortableDevicePropertiesBulk> bulkProps;
  hr = properties->QueryInterface(__uuidof(IPortableDevicePropertiesBulk), getter_AddRefs(bulkProps));


  nsID *deviceID = nsnull;
  rv = GetId(&deviceID);
  NS_ENSURE_SUCCESS(rv, rv);

#if 0
  if(SUCCEEDED(hr)) {
  // We get to use the bulk interface.
    
  }
  else {
#endif
    // Stats to track
    PRUint64 videoUsed = 0;
    PRUint64 audioUsed = 0;
    PRUint64 otherUsed = 0;

  // Couldn't get the bulk interface, fall back to normal properties interface.
    nsTArray<nsString> objectsToScan;
    nsTArray<nsString> playlistsToScan;
    objectsToScan.AppendElement(libraryObjectID);
  
    // This is created here to prevent one from being created for each loop
    nsRefPtr<sbPropertyVariant> sizeVar = sbPropertyVariant::New();

    while (!objectsToScan.IsEmpty()) {
      nsRefPtr<IEnumPortableDeviceObjectIDs> enumObjectIds;
      PRUint32 nextObjectIdx = objectsToScan.Length() - 1;
      hr = content->EnumObjects(0,
                                objectsToScan[nextObjectIdx].get(),
                                NULL,
                                getter_AddRefs(enumObjectIds));
      NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
      objectsToScan.RemoveElementAt(nextObjectIdx);
  
      ULONG count = 0;
      do {
        // Do this in batches of 100
        LPWSTR objectIds[100] = {0};
  
        hr = enumObjectIds->Next(100, objectIds, &count);
        
        if (FAILED(hr))
          break;

        // This is created here to prevent one from being created for each loop
        nsRefPtr<sbPropertyVariant> sizeVar = sbPropertyVariant::New();
  
        for(ULONG current = 0; current < count; ++current) {
          nsString objId(objectIds[current]);
          ::CoTaskMemFree(objectIds[current]);
          objectIds[current] = NULL;
          nsRefPtr<IPortableDeviceValues> values;

          hr = properties->GetValues(objId.get(), keys, getter_AddRefs(values));
          if (FAILED(hr))
            continue;

          LPWSTR value = NULL;
          hr = values->GetStringValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, 
                                      &value);

          // Skip if no PUID.
          if (FAILED(hr))
            continue;

          nsString objectPUID(value);
          ::CoTaskMemFree(value);

          GUID contentTypeGuid;
          hr = values->GetGuidValue(WPD_OBJECT_CONTENT_TYPE, &contentTypeGuid);

          // Skip if no content type.
          if (FAILED(hr))
            continue;

          // Skip if not audio or video.
          // XXXAus: THIS WILL CHANGE AS WE SUPPORT OTHER CONTENT TYPES
          if (IsEqualGUID(contentTypeGuid, WPD_CONTENT_TYPE_AUDIO)) {
            AddObjectSize(values, sizeVar, audioUsed);
          }
          else if (IsEqualGUID(contentTypeGuid, WPD_CONTENT_TYPE_VIDEO)) {
            AddObjectSize(values, sizeVar, videoUsed);
          }
          else {
            if (IsEqualGUID(contentTypeGuid, WPD_CONTENT_TYPE_FOLDER)) {
              // scan this folder too
              objectsToScan.AppendElement(objId);
            }
            if (IsEqualGUID(contentTypeGuid, WPD_CONTENT_TYPE_PLAYLIST)) {
              // Scan playlists after all media items have been scanned.
              playlistsToScan.AppendElement(objId);
            }
            AddObjectSize(values, sizeVar, otherUsed);
            continue;
          }
          
          GUID objectFormatGuid;
          hr = values->GetGuidValue(WPD_OBJECT_FORMAT, &objectFormatGuid);
          
          // Skip if no object format
          if (FAILED(hr)) 
            continue;

          // Skip if content type does not map to something we support.
          // XXXAus: THIS IS WRONG, it should use the PlaylistPlayback extensions list!!!
          nsCString contentTypeExt;
          if (NS_FAILED(sbWPDGUIDtoFileExtension(objectFormatGuid, contentTypeExt)))
            continue;

          nsCOMPtr<sbIMediaItem> item;
          rv = sbWPDGetMediaItemByPUID(library, objectPUID, getter_AddRefs(item));
          
          // Looks like we will have to create the item.
          if(rv == NS_ERROR_NOT_AVAILABLE) {
            rv = sbWPDCreateMediaItemFromDeviceValues(library,
                                                      keyArray,
                                                      values,
                                                      deviceID,
                                                      getter_AddRefs(item));
            /* ignore errors */
          }
          else {
            rv = sbWPDSetMediaItemPropertiesFromDeviceValues(item,
                                                             keyArray,
                                                             values);
            /* ignore errors */
          }
        } /* for current = 0 ... count - 1 */
      } while(count);
    } /* !objectsToScan.IsEmpty() */

    // Scan playlists.
    for (PRUint32 i = 0; i < playlistsToScan.Length(); i++) {
      ImportPlaylist(library, playlistsToScan[i]);
    }

    mDeviceStatistics.SetAudioUsed(audioUsed);
    mDeviceStatistics.SetVideoUsed(videoUsed);
    mDeviceStatistics.SetOtherUsed(otherUsed);

    if(deviceID) {
      NS_Free(deviceID);
    }

    rv = mLibraryListener->SetIgnoreListener(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

#if 0 
  }
#endif

  return NS_OK;
}

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
