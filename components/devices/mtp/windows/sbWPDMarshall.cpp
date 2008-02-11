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

#include "sbWPDMarshall.h"
#include <vector>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsMemory.h>
#include <nsStringAPI.h>
#include <nsIWritablePropertyBag.h>
#include <nsIArray.h>
#include <sbIDevice.h>
#include <sbIDeviceController.h>
#include <sbIDeviceEventTarget.h>
#include <sbIDeviceEvent.h>
#include <sbIDeviceManager.h>
#include <sbIDeviceRegistrar.h>
#include <nsIVariant.h>
#include "sbPortableDeviceEventsCallback.h"
#include "sbWPDCommon.h"

#include <mswmdm.h>
#include <sac.h>
#include <SCClient.h>
#include <PortableDeviceTypes.h>
#include <mswmdm_i.c>
#include <nsCOMArray.h>
#include "sbWPDDevice.h"
#include "sbWPDDeviceController.h"
#include "sbWPDDeviceController.h"

typedef std::vector<nsString> StringArray;

static char const * const HASH_PROPERTY_BAG_CONTRACTID = "@mozilla.org/hash-property-bag;1";
static char const * const SONGBIRD_DEVICEMANAGER_CONTRACTID = "@songbirdnest.com/Songbird/DeviceManager;2";
/**
 * sbWMDMarshall framework installation
 */
NS_IMPL_THREADSAFE_ADDREF(sbWPDMarshall)
NS_IMPL_THREADSAFE_RELEASE(sbWPDMarshall)
NS_IMPL_QUERY_INTERFACE2_CI(sbWPDMarshall,
                            sbIDeviceMarshall,
                            nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER1(sbWPDMarshall,
                             sbIDeviceMarshall)

NS_DECL_CLASSINFO(sbWPDMarshall)
NS_IMPL_THREADSAFE_CI(sbWPDMarshall)

/**
 * Retreives the WPD device manager
 */
inline
HRESULT GetDeviceManager(IPortableDeviceManager ** deviceManager)
{
  nsRefPtr<IPortableDeviceManager> devMgr;
  // CoCreate the IPortableDeviceManager interface to enumerate
  // portable devices and to get information about them.
  if (SUCCEEDED(CoCreateInstance(CLSID_PortableDeviceManager,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IPortableDeviceManager,
                        getter_AddRefs(devMgr)))) {
    devMgr.forget(deviceManager);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

/**
 * Returns an array of the IDs of the currently connected devices
 */
static nsresult GetDeviceIDs(IPortableDeviceManager * deviceManager,
                             StringArray & deviceIDs)
{
  // Get the number of devices
  DWORD devices = 0;
  if (FAILED(deviceManager->GetDevices(NULL, &devices)))
    return NS_ERROR_FAILURE;
  if (devices > 0) {
    // Retrieve the device ID's into our array
    LPWSTR* deviceIDsBuffer = new LPWSTR[devices];
    if (!deviceIDsBuffer)
      return NS_ERROR_OUT_OF_MEMORY;
    if (SUCCEEDED(deviceManager->GetDevices(deviceIDsBuffer, &devices))) {
      //Copy the device IDs from the local array to array passed to us
      for (DWORD index = 0; index < devices; ++index) {
        deviceIDs.push_back(nsString(deviceIDsBuffer[index]));
        CoTaskMemFree(deviceIDsBuffer[index]);
      }
      delete [] deviceIDsBuffer;
    }
  }
  return NS_OK;
}


/**
 * Creates a device listner that is attached to an IPortableDevice instance
 */
static nsresult CreateDeviceListener(sbWPDMarshall * marshall,
                                     IPortableDevice * device,
                                     sbIDevice * sbDevice)
{
  LPWSTR cookie;
  nsAString const & deviceID = sbWPDDevice::GetDeviceID(device);
  if (!deviceID.IsEmpty() &&
      SUCCEEDED(device->Advise(0,
                               sbPortableDeviceEventsCallback::New(marshall,
                                                                  sbDevice,
                                                                  deviceID),
                               nsnull,
                               &cookie)))
    return NS_OK;
  return NS_ERROR_FAILURE;
}

/**
 * Create a variant that holds the string in value
 */
static nsresult CreateVariant(nsString const & value, nsIVariant ** var)
{
  nsresult rv;
  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    p->SetAsAString(value);
    p.forget(reinterpret_cast<nsIWritableVariant**>(var));
  }
  return rv;
}

/******************************************************************************
 * This a helper class that aids in writing properties to the property bag from
 * the IPortableDeviceManager. It centeralizes the allocation and reuses the
 * buffer across assignments, expanding the size if needed.
 */
class PropertyBagWriter
{
public:
  /**
   * Initializes the writer with the device manager and property bag that is being
   * written to
   */
  PropertyBagWriter(IPortableDeviceManager * deviceManager, 
                    IPortableDevice * device, 
                    nsIWritablePropertyBag * bag) :
    mDeviceManager(deviceManager),
    mDevice(device),
    mBag(bag),
    mBuffer(nsnull),
    mCurrentBufferSize(0)
  {
    NS_ASSERTION(device, "Device is null");
    sbWPDDevice::GetDeviceProperties(device, getter_AddRefs(mDeviceProperties));
  }
  /**
   * Release the buffer we were using
   */
  ~PropertyBagWriter()
  {
    delete [] mBuffer;
  }
  /**
   * Function to set the property value
   * @param method This is a pointer to member of the IPortableDeviceManager function to call
   * @param propKey This is the property key used to set the property in the bag
   * @param deviceID this is the ID of the device to query the property
   */
  typedef HRESULT (STDMETHODCALLTYPE IPortableDeviceManager::*getterMethod)(LPCWSTR, WCHAR*, DWORD*);
  nsresult SetProperty(getterMethod method,
                       nsAString const & propKey,
                       LPCWSTR deviceID);
  /**
   * This is a simpler overloaded function that sets properties on the property bag
   */
  nsresult SetProperty(nsAString const & propKey,
                       nsAString const & value)
  {
    nsCOMPtr<nsIVariant> var;
    nsresult rv = CreateVariant(nsString(value), getter_AddRefs(var));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mBag->SetProperty(nsString(propKey), var);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }
  nsresult SetPropertyFromDevice(PROPERTYKEY const & deviceProperty, nsAString const & propKey)
  {
    nsCOMPtr<nsIVariant> var;
    nsresult rv = sbWPDDevice::GetProperty(mDeviceProperties, deviceProperty, getter_AddRefs(var));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mBag->SetProperty(nsString(propKey), var);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }
private:
  IPortableDeviceManager* mDeviceManager;
  IPortableDevice* mDevice;
  nsRefPtr<IPortableDeviceProperties> mDeviceProperties;
  nsIWritablePropertyBag* mBag;
  WCHAR * mBuffer;
  DWORD mCurrentBufferSize;
};

nsresult PropertyBagWriter::SetProperty(getterMethod method,
                                        nsAString const & propKey,
                                        LPCWSTR deviceID)
{
  DWORD bufferSize = 0;
  // Query the size of the value
  if (SUCCEEDED((mDeviceManager->*method)(deviceID, 0, &bufferSize))) {
    // Expand the buffer if needed
    if (bufferSize > mCurrentBufferSize) {
      delete [] mBuffer;
      mBuffer = new WCHAR[bufferSize];
      if (!mBuffer)
        return NS_ERROR_OUT_OF_MEMORY;
      mCurrentBufferSize = bufferSize;
    }
    // Actually query the value into our buffer
    if (SUCCEEDED((mDeviceManager->*method)(deviceID, mBuffer, &bufferSize))) {
      nsCOMPtr<nsIVariant> var;
      // Create the variant for the value and stuff in the bag
      if (NS_SUCCEEDED(CreateVariant(nsString(mBuffer, bufferSize), getter_AddRefs(var))) &&
          var &&
          NS_SUCCEEDED(mBag->SetProperty(nsString(propKey), var)))
          return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

/**
 * Adds the device's properties to the property bag
 * @param deviceManager The device manager it has some of the methods needed to
 * query the device
 * @param device The device we're querying the properties for
 * @param propertyBag the bag that will receive the property values
 */
static
nsresult AddDevicePropertiesToPropertyBag(IPortableDeviceManager * deviceManager,
                                          IPortableDevice * device,
                                          nsIWritablePropertyBag * propertyBag)
{
  nsString const & deviceID = sbWPDDevice::GetDeviceID(device);
  if (deviceID.IsEmpty())
    return NS_ERROR_FAILURE;
  PropertyBagWriter writer(deviceManager, device, propertyBag);
  // If any of these properties fail, they just don't get assigned
  // TODO: there is are constants coming in from erikstaats that
  // this should use instead.
  writer.SetProperty(&IPortableDeviceManager::GetDeviceFriendlyName, 
                     NS_LITERAL_STRING("DeviceFriendlyName"),
                     deviceID.get());
  writer.SetProperty(&IPortableDeviceManager::GetDeviceDescription, 
                     NS_LITERAL_STRING("DeviceDescription"), 
                     deviceID.get());
  writer.SetProperty(&IPortableDeviceManager::GetDeviceManufacturer,
                     NS_LITERAL_STRING("DeviceManufacturer"),
                     deviceID.get());
  // Set the device ID as a property
  writer.SetProperty(NS_LITERAL_STRING("DeviceID"),
                     deviceID);
  writer.SetProperty(NS_LITERAL_STRING("DeviceType"),
                     NS_LITERAL_STRING("WPD"));
  writer.SetPropertyFromDevice(WPD_DEVICE_SERIAL_NUMBER,
                               NS_LITERAL_STRING("SerialNo"));
  writer.SetPropertyFromDevice(WPD_DEVICE_MODEL,
                               NS_LITERAL_STRING("ModelNo"));
  return NS_OK;
}

/**
 * Creates an IPortableDevice based object for the device with the ID of 
 * deviceID
 * @param deviceID the ID of the device we want
 * @param device out parameter to receive the device object
 */
static nsresult CreatePortableDevice(nsString const & deviceID,
                                     IPortableDevice** device)
{
  nsRefPtr<IPortableDeviceValues> clientInfo;
  nsRefPtr<IPortableDevice> dev;
  if (SUCCEEDED(CoCreateInstance(CLSID_PortableDevice,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    IID_IPortableDevice,
                                    getter_AddRefs(dev))) && dev &&                                    
      SUCCEEDED(sbWPDDevice::GetClientInfo(getter_AddRefs(clientInfo))) && clientInfo &&
      SUCCEEDED(dev->Open(deviceID.get(), clientInfo))) {
    dev.forget(device);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}
                              
/******************************************************************************
 * COM Listener class that is called when a device arrives or is removed
 */
class sbDeviceMarshallListener : public IWMDMNotification
{
public:
  /**
   * Just initializes the ref count
   */
  sbDeviceMarshallListener(sbWPDMarshall * marshall) : mRefCnt(0),
                                                        mMarshall(marshall)
  {
  }
  /**
   * IUnknown::QueryInterface implementation
   */
  STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
  /**
   * IUnknown::AddRef implementation
   */
  ULONG STDMETHODCALLTYPE AddRef();
  /**
   * IUnknown::Release implementation
   */
  ULONG STDMETHODCALLTYPE Release();
  /**
   * Receives the notifications
   */
  STDMETHOD (WMDMMessage) (/*[in]*/DWORD dwMessageType, /*[in]*/LPCWSTR pwszCanonicalName);

private:
  ULONG mRefCnt;
  nsRefPtr<sbWPDMarshall> mMarshall;
};

HRESULT sbDeviceMarshallListener::WMDMMessage(DWORD dwMessageType,
                                              LPCWSTR pwszCanonicalName)
{
  HRESULT hr = S_OK;
  switch (dwMessageType)
  {
    case WMDM_MSG_DEVICE_ARRIVAL:
    {
      mMarshall->DiscoverDevices();
    }
    break;
    // All other events should be handled by the IPortableDevice::Advise
    case WMDM_MSG_DEVICE_REMOVAL:
    case WMDM_MSG_MEDIA_ARRIVAL:
    case WMDM_MSG_MEDIA_REMOVAL:
    break;
  }
  return hr;
}

//IUnknown impl

STDMETHODIMP sbDeviceMarshallListener::QueryInterface(REFIID riid,
                                                      void ** ppvObject)
{
  HRESULT hr = E_INVALIDARG;
  if (NULL != ppvObject) {
    *ppvObject = NULL;
    
    if (IsEqualIID(riid, IID_IWMDMNotification) || IsEqualIID(riid,
                                                              IID_IUnknown)) {
      *ppvObject = this;
      AddRef();
      hr = S_OK;
    }
    else {
      hr = E_NOINTERFACE;
    }
  }
  return hr;
}

ULONG STDMETHODCALLTYPE sbDeviceMarshallListener::AddRef()
{
  InterlockedIncrement((long*) &mRefCnt);
  return mRefCnt;
}

ULONG STDMETHODCALLTYPE sbDeviceMarshallListener::Release()
{
  ULONG ulRefCount = mRefCnt - 1;

  if (InterlockedDecrement((long*) &mRefCnt) == 0)
  {
      delete this;
      return 0;
  }
  return ulRefCount;
}

/******************************************************************************
 * sbWPDMarshall Implementation
 */
sbWPDMarshall::sbWPDMarshall() :
  sbBaseDeviceMarshall(NS_LITERAL_CSTRING(SB_DEVICE_CONTROLLER_CATEGORY)),
  mKnownDevicesLock(nsAutoMonitor::NewMonitor("sbWPDMarshall.mKnownDevicesLock"))
{
  mKnownDevices.Init(8);
}

sbWPDMarshall::~sbWPDMarshall()
{
  nsAutoMonitor mon(mKnownDevicesLock);
  mon.Exit();

  nsAutoMonitor::DestroyMonitor(mKnownDevicesLock);
}


NS_IMETHODIMP sbWPDMarshall::BeginMonitoring()
{
  nsAutoMonitor mon(mKnownDevicesLock);

  // Create the secure client object
  BYTE abPVK[] =
  { 0x00 };
  BYTE abCert[] =
  { 0x00 };
  nsRefPtr<IComponentAuthenticate> pAuth;
  HRESULT hr =:: CoCreateInstance(__uuidof(::MediaDevMgr),
      NULL,
      CLSCTX_INPROC_SERVER,
      __uuidof(IComponentAuthenticate),
      getter_AddRefs(pAuth));
  if (FAILED(hr))
    return NS_ERROR_FAILURE;

  CSecureChannelClient * sac = new ::CSecureChannelClient;
  if (!sac)
    return NS_ERROR_FAILURE;
  hr = sac->SetCertificate(
      SAC_CERT_V1,
      abCert, sizeof(abCert),
      abPVK, sizeof(abPVK));
  if (FAILED(hr))
    return NS_ERROR_FAILURE;
  sac->SetInterface(pAuth);
  hr = sac->Authenticate(SAC_PROTOCOL_V1);
  if (FAILED(hr))
    return NS_ERROR_FAILURE;
  
  // Create the Windows Media Device manager
  nsRefPtr<IWMDeviceManager> devMgr;

  hr = pAuth->QueryInterface(__uuidof(IWMDeviceManager), getter_AddRefs(devMgr));
  if (FAILED(hr))
    return NS_ERROR_FAILURE;
  
  // See what devices are currently active
  DiscoverDevices();
  // Hook up the listener for device events
  sbDeviceMarshallListener * listener = new sbDeviceMarshallListener(this);
  nsRefPtr<::IConnectionPointContainer> pConnectionPointContainer;
  nsRefPtr<::IConnectionPoint> pConnectionPoint;
  DWORD cookie = 0;
  if (SUCCEEDED(hr = devMgr->QueryInterface(__uuidof(IConnectionPointContainer), getter_AddRefs(pConnectionPointContainer))) &&
      SUCCEEDED(hr = pConnectionPointContainer->FindConnectionPoint(::IID_IWMDMNotification, getter_AddRefs(pConnectionPoint))) &&
      SUCCEEDED(hr = pConnectionPoint->Advise(listener, &cookie))) {
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

/* readonly attribute nsIDPtr id; */
NS_IMETHODIMP sbWPDMarshall::GetId(nsID * *aId)
{
  NS_ENSURE_ARG(aId);
  static nsID const id = SB_WPDMARSHALL_CID;
  *aId = static_cast<nsID*>(NS_Alloc(sizeof(nsID)));
  **aId = id;
  return NS_OK;
}

/* readonly attribute AString name; */
NS_IMETHODIMP sbWPDMarshall::GetName(nsAString & aName)
{
  aName = L"sbWMDMarshall";
  return NS_OK;
}

/* void loadControllers (in sbIDeviceControllerRegistrar aRegistrar); */
NS_IMETHODIMP sbWPDMarshall::LoadControllers(sbIDeviceControllerRegistrar *aRegistrar)
{
  RegisterControllers(aRegistrar);
  return NS_OK;
}

/* void stopMonitoring (); */
NS_IMETHODIMP sbWPDMarshall::StopMonitoring()
{
  ClearMonitoringFlag();
  return NS_OK;
}

nsresult sbWPDMarshall::DiscoverDevices()
{
  nsresult rv = NS_OK;
  
  // Create the property bad to pass to the control
  nsCOMPtr<nsIWritablePropertyBag> propBag = do_CreateInstance(HASH_PROPERTY_BAG_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Grab the device manager service instance
  nsCOMPtr<sbIDeviceManager2> deviceManager = do_GetService(SONGBIRD_DEVICEMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Grab the device registrar from the device manager instance
  nsCOMPtr<sbIDeviceRegistrar> deviceRegistrar = do_QueryInterface(deviceManager, &rv);
  if (NS_SUCCEEDED(rv) && propBag)
  {
    // Create the WPD device manager
    nsRefPtr<IPortableDeviceManager> deviceManager;
    if (FAILED(GetDeviceManager(getter_AddRefs(deviceManager))) || !deviceManager)
      return NS_ERROR_FAILURE;
    // Get the list of devices
    StringArray deviceIDs;
    if (NS_FAILED(GetDeviceIDs(deviceManager, deviceIDs)))
      return NS_ERROR_FAILURE;
    size_t const deviceIDCount = deviceIDs.size();
    for (size_t index = 0; index < deviceIDCount; ++index)
    {
      nsString const & deviceID = deviceIDs[index];
      // If we don't know this device, set it up
      nsAutoMonitor mon(mKnownDevicesLock);
      if (!mKnownDevices.Get(deviceID, nsnull)) {
        // Create the WPD device and get it's properties
        nsRefPtr<IPortableDevice> device;
        if (NS_SUCCEEDED(CreatePortableDevice(deviceID, getter_AddRefs(device))) &&  
            NS_SUCCEEDED(AddDevicePropertiesToPropertyBag(deviceManager,
                                                          device,
                                                          propBag))) {
          // Find a controller that likes this device
          nsCOMPtr<sbIDeviceController> controller = FindCompatibleControllers(propBag);
          if (controller) {
            // Ask the controller to create a songbird device for us
            nsCOMPtr<sbIDevice> sbDevice;
            if (NS_SUCCEEDED(controller->CreateDevice(propBag,
                                                      getter_AddRefs(sbDevice))) && sbDevice) {

              // XXXAus: This is bad to have to do this. 
              // But I have to reuse the IPortableDeviceInstance.
              sbWPDDevice *wpdDevice = static_cast<sbWPDDevice *>(sbDevice.get());
              wpdDevice->mPortableDevice = device;

              // Create the listener for this device
              CreateDeviceListener(this, device, sbDevice);

              // Record this device so we know we've created it
              AddDevice(deviceID, sbDevice);
              
              // Register the device with the device registrar.
              rv = deviceRegistrar->RegisterDevice(sbDevice);
              NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to register device");

              rv = sbWPDCreateAndDispatchEvent(this,
                                               sbDevice,
                                               sbIDeviceEvent::EVENT_DEVICE_ADDED);
              NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create device add event");
            }
          }
        }
      }
    }
  }
  return rv;
}