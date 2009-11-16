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

#include "sbCDDeviceMarshall.h"

#include "sbICDDevice.h"
#include "sbICDDeviceService.h"

#include <sbIDevice.h>
#include <sbIDeviceController.h>
#include <sbIDeviceRegistrar.h>
#include <sbIDeviceControllerRegistrar.h>
#include <sbIDeviceManager.h>
#include <sbIDeviceEvent.h>
#include <sbIDeviceEventTarget.h>
#include <sbVariantUtils.h>

#include <nsComponentManagerUtils.h>
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsISupportsPrimitives.h>
#include <nsServiceManagerUtils.h>
#include <nsMemory.h>
#include <prlog.h>

//
// To log this module, set the following environment variable:
//   NSPR_LOG_MODULES=sbCDDevice:5
//

#ifdef PR_LOGGING
static PRLogModuleInfo* gCDDeviceLog = nsnull;
#define TRACE(args) PR_LOG(gCDDeviceLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gCDDeviceLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */

NS_DEFINE_STATIC_IID_ACCESSOR(sbCDDeviceMarshall, SB_CDDEVICE_MARSHALL_IID)

NS_IMPL_THREADSAFE_ADDREF(sbCDDeviceMarshall)
NS_IMPL_THREADSAFE_RELEASE(sbCDDeviceMarshall)
NS_IMPL_QUERY_INTERFACE2_CI(sbCDDeviceMarshall,
                            sbIDeviceMarshall,
                            sbICDDeviceListener)
NS_IMPL_CI_INTERFACE_GETTER2(sbCDDeviceMarshall,
                             sbIDeviceMarshall,
                             sbICDDeviceListener)

// nsIClassInfo implementation.
NS_DECL_CLASSINFO(sbCDDeviceMarshall)
NS_IMPL_THREADSAFE_CI(sbCDDeviceMarshall)

sbCDDeviceMarshall::sbCDDeviceMarshall()
  : sbBaseDeviceMarshall(NS_LITERAL_CSTRING(SB_DEVICE_CONTROLLER_CATEGORY))
  , mKnownDevicesLock(nsAutoMonitor::NewMonitor("sbCDDeviceMarshall::mKnownDevicesLock"))
{
#ifdef PR_LOGGING
  if (!gCDDeviceLog) {
    gCDDeviceLog = PR_NewLogModule("sbCDDevice");
  }
#endif

  mKnownDevices.Init(8);
}

sbCDDeviceMarshall::~sbCDDeviceMarshall()
{
  nsAutoMonitor mon(mKnownDevicesLock);
  mon.Exit();

  nsAutoMonitor::DestroyMonitor(mKnownDevicesLock);
}

nsresult
sbCDDeviceMarshall::Init()
{
  nsresult rv;
  nsCOMPtr<sbIDeviceManager2> deviceMgr =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mCDDeviceService = nsnull;
  PRInt32 selectedWeight = -1;

  // Enumerate the category manager for cdrip services
  nsCOMPtr<nsICategoryManager> catman =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISimpleEnumerator> categoryEnum;
  rv = catman->EnumerateCategory("cdrip-engine", getter_AddRefs(categoryEnum));
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool hasMore = PR_FALSE;
  while (NS_SUCCEEDED(categoryEnum->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> ptr;
    if (NS_SUCCEEDED(categoryEnum->GetNext(getter_AddRefs(ptr))) && ptr) {
      nsCOMPtr<nsISupportsCString> stringValue(do_QueryInterface(ptr));
      nsCString factoryName;
      if (stringValue && NS_SUCCEEDED(stringValue->GetData(factoryName))) {
        nsCString contractId;
        rv = catman->GetCategoryEntry("cdrip-engine", factoryName.get(),
                                      getter_Copies(contractId));
        NS_ENSURE_SUCCESS(rv, rv);

        // Get this CD device service
        nsCOMPtr<sbICDDeviceService> cdDevSvc =
            do_GetService(contractId.get(), &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        PRInt32 currentWeight;
        rv = cdDevSvc->GetWeight(&currentWeight);
        NS_ENSURE_SUCCESS(rv, rv);

        // if we don't have a CD device service yet, or if this service has
        // a higher voting weight than the current service, then choose this
        // service as our selected CD device service
        if (selectedWeight == -1 || currentWeight >= selectedWeight) {
          mCDDeviceService = cdDevSvc;
          selectedWeight = currentWeight;
        }
      }
    }
  }

  return NS_OK;
}

nsresult
sbCDDeviceMarshall::AddDevice(sbICDDevice *aCDDevice)
{
  NS_ENSURE_ARG_POINTER(aCDDevice);

  nsresult rv;

  nsString deviceName;
  rv = aCDDevice->GetName(deviceName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Don't bother watching this device if this marshall is already watching
  // it in mKnownDevices.
  PRBool hasDevice = PR_FALSE;
  rv = GetHasDevice(deviceName, &hasDevice);
  if (NS_FAILED(rv) || hasDevice) {
    return NS_OK;
  }

  // Fill out some properties for this device.
  nsCOMPtr<nsIWritablePropertyBag> propBag =
    do_CreateInstance("@mozilla.org/hash-property-bag;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWritableVariant> deviceType =
    do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deviceType->SetAsAString(NS_LITERAL_STRING("CD"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propBag->SetProperty(NS_LITERAL_STRING("DeviceType"),
                             deviceType);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceController> controller = FindCompatibleControllers(propBag);
  NS_ENSURE_TRUE(controller, NS_ERROR_UNEXPECTED);

  // Stash the device with the property bag.
  nsCOMPtr<nsIWritableVariant> deviceVar =
    do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deviceVar->SetAsISupports(aCDDevice);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propBag->SetProperty(NS_LITERAL_STRING("sbICDDevice"), deviceVar);
  NS_ENSURE_SUCCESS(rv, rv);

  // Have the controller create the device for us.
  nsCOMPtr<sbIDevice> sbDevice;
  rv = controller->CreateDevice(propBag, getter_AddRefs(sbDevice));
  NS_ENSURE_SUCCESS(rv, rv);

  // Ensure that the device has media inserted into it.
  PRBool hasDisc = PR_FALSE;
  rv = aCDDevice->GetIsDiscInserted(&hasDisc);
  if (NS_FAILED(rv) || !hasDisc) {
    return NS_OK;
  }

  // Ensure that the inserted disc is a media disc
  PRUint32 discType;
  rv = aCDDevice->GetDiscType(&discType);
  if (NS_FAILED(rv) || discType != sbICDDevice::AUDIO_DISC_TYPE) {
    return NS_OK;
  }

  nsCOMPtr<sbIDeviceManager2> deviceManager =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceRegistrar> deviceRegistrar =
    do_QueryInterface(deviceManager, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Register this device with the device registrar.
  rv = deviceRegistrar->RegisterDevice(sbDevice);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to register device!");

  // Dispatch the added device event.
  CreateAndDispatchDeviceManagerEvent(sbIDeviceEvent::EVENT_DEVICE_ADDED,
                                      sbNewVariant(sbDevice),
                                      static_cast<sbIDeviceMarshall *>(this));

  // Stash this device in the hash of known CD devices.
  nsAutoMonitor mon(mKnownDevicesLock);
  mKnownDevices.Put(deviceName, sbDevice);

  return NS_OK;
}

nsresult
sbCDDeviceMarshall::RemoveDevice(sbIDevice* aDevice) {
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
sbCDDeviceMarshall::RemoveDevice(nsAString const & aName)
{
  nsresult rv;
  // Only remove the device if it's stashed in the device hash.
  nsCOMPtr<sbIDevice> device;
  rv = GetDevice(aName, getter_AddRefs(device));
  if (NS_FAILED(rv) || !device) {
    return NS_OK;
  }

  nsCOMPtr<sbIDeviceRegistrar> deviceRegistrar =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceControllerRegistrar> deviceControllerRegistrar =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the device controller for this device.
  nsCOMPtr<sbIDeviceController> deviceController;
  nsID *controllerId = nsnull;
  rv = device->GetControllerId(&controllerId);
  if (NS_SUCCEEDED(rv)) {
    rv = deviceControllerRegistrar->GetController(
        controllerId,
        getter_AddRefs(deviceController));
  }

  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to get device controller.");
    deviceController = nsnull;
  }

  if (controllerId) {
    NS_Free(controllerId);
  }

  // Release the device from the controller
  if (deviceController) {
    rv = deviceController->ReleaseDevice(device);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to release the device");
  }

  // Unregister the device
  rv = deviceRegistrar->UnregisterDevice(device);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to unregister device");

  nsAutoMonitor mon(mKnownDevicesLock);
  mKnownDevices.Remove(aName);

  return NS_OK;
}

nsresult
sbCDDeviceMarshall::GetDevice(nsAString const & aName, sbIDevice **aOutDevice)
{
  NS_ENSURE_ARG_POINTER(aOutDevice);

  nsresult rv;
  nsCOMPtr<nsISupports> supports;
  rv = mKnownDevices.Get(aName, getter_AddRefs(supports));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevice> device = do_QueryInterface(supports, &rv);
  if (NS_FAILED(rv) || !device) {
    return NS_ERROR_FAILURE;
  }

  device.forget(aOutDevice);
  return NS_OK;
}

nsresult
sbCDDeviceMarshall::GetHasDevice(nsAString const &aName, PRBool *aOutHasDevice)
{
  NS_ENSURE_ARG_POINTER(aOutHasDevice);
  *aOutHasDevice = PR_FALSE;  // assume false

  // Operate under the known devices lock
  nsAutoMonitor mon(mKnownDevicesLock);

  nsresult rv;
  nsCOMPtr<sbIDevice> deviceRef;
  rv = GetDevice(aName, getter_AddRefs(deviceRef));
  if (NS_SUCCEEDED(rv) && deviceRef) {
    *aOutHasDevice = PR_TRUE;
  }

  return NS_OK;
}

nsresult
sbCDDeviceMarshall::DiscoverDevices()
{
  // Iterate over the available CD devices and check to see if they have
  // media currently inserted.
  nsresult rv;

  NS_ENSURE_STATE(mCDDeviceService);

  // Since the GW stuff is a little jacked, use the index getter
  PRInt32 deviceCount = 0;
  rv = mCDDeviceService->GetNbDevices(&deviceCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Dispatch the scan start event.
  CreateAndDispatchDeviceManagerEvent(sbIDeviceEvent::EVENT_DEVICE_SCAN_START,
                                      nsnull,
                                      static_cast<sbIDeviceMarshall *>(this));

  for (PRInt32 i = 0; i < deviceCount; i++) {
    nsCOMPtr<sbICDDevice> curDevice;
    rv = mCDDeviceService->GetDevice(i, getter_AddRefs(curDevice));
    if (NS_FAILED(rv) || !curDevice) {
      NS_WARNING("Could not get the current device!");
      continue;
    }

    rv = AddDevice(curDevice);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not add a CD Device!");
  }

  // Dispatch the scan end event.
  CreateAndDispatchDeviceManagerEvent(sbIDeviceEvent::EVENT_DEVICE_SCAN_END,
                                      nsnull,
                                      static_cast<sbIDeviceMarshall *>(this));

  return NS_OK;
}

nsresult
sbCDDeviceMarshall::CreateAndDispatchDeviceManagerEvent(PRUint32 aType,
                                                        nsIVariant *aData,
                                                        nsISupports *aOrigin,
                                                        PRBool aAsync)
{
  nsresult rv;

  // Get the device manager.
  nsCOMPtr<sbIDeviceManager2> manager =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Use the device manager as the event target.
  nsCOMPtr<sbIDeviceEventTarget> eventTarget = do_QueryInterface(manager, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the event.
  nsCOMPtr<sbIDeviceEvent> event;
  rv = manager->CreateEvent(aType,
                            aData,
                            aOrigin,
                            sbIDevice::STATE_IDLE,
                            sbIDevice::STATE_IDLE,
                            getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  // Dispatch the event.
  PRBool dispatched;
  rv = eventTarget->DispatchEvent(event, aAsync, &dispatched);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIDeviceMarshall

NS_IMETHODIMP
sbCDDeviceMarshall::LoadControllers(sbIDeviceControllerRegistrar *aRegistrar)
{
  NS_ENSURE_ARG_POINTER(aRegistrar);

  RegisterControllers(aRegistrar);

  return NS_OK;
}

NS_IMETHODIMP
sbCDDeviceMarshall::BeginMonitoring()
{
  nsresult rv;

  NS_ENSURE_STATE(mCDDeviceService);
  NS_ASSERTION(IsMonitoring(), "BeginMonitoring() called after StopMonitoring()!");

  rv = mCDDeviceService->RegisterListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DiscoverDevices();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbCDDeviceMarshall::StopMonitoring()
{
  nsresult rv;

  if (mCDDeviceService) {
    rv = mCDDeviceService->RemoveListener(this);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  ClearMonitoringFlag();
  return NS_OK;
}

NS_IMETHODIMP
sbCDDeviceMarshall::GetId(nsID **aId)
{
  NS_ENSURE_ARG_POINTER(aId);

  static nsID const id = SB_CDDEVICE_MARSHALL_CID;
  *aId = static_cast<nsID *>(NS_Alloc(sizeof(nsID)));
  **aId = id;

  return NS_OK;
}

NS_IMETHODIMP
sbCDDeviceMarshall::GetName(nsAString & aName)
{
  aName.Assign(NS_LITERAL_STRING(SB_CDDEVICE_MARSHALL_NAME));
  return NS_OK;
}

//------------------------------------------------------------------------------
// sbICDDeviceListener

NS_IMETHODIMP
sbCDDeviceMarshall::OnDeviceRemoved(sbICDDevice *aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;
  nsString deviceName;
  rv = aDevice->GetName(deviceName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Only remove the device if this marshall is currently monitoring it.
  nsCOMPtr<sbIDevice> device;
  rv = GetDevice(deviceName, getter_AddRefs(device));
  if (NS_SUCCEEDED(rv) && device) {
    rv = RemoveDevice(deviceName);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbCDDeviceMarshall::OnMediaInserted(sbICDDevice *aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv = AddDevice(aDevice);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbCDDeviceMarshall::OnMediaEjected(sbICDDevice *aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;
  nsString deviceName;
  rv = aDevice->GetName(deviceName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RemoveDevice(deviceName);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

