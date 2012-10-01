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

//------------------------------------------------------------------------------
//
// iPod device marshall services.
//
//   The iPod device marshall services scan for iPod devices at startup and
// listen for added and removed devices.  The marshall communicates these events
// to the other device services.
//
// Thread strategy:
//
//   Most marshall interfaces are not typically called from more than one
// thread.  The marshall device event handlers are always called from the main
// thread.  A single lock is thus used to serialize thread access since multiple
// threads are not expected to be blocked.
//   A few exceptions to this strategy will use alternate methods or no locking
// (e.g., GetId).
//
//------------------------------------------------------------------------------


/**
 * \file  sbIPDMarshall.cpp
 * \brief Songbird iPod Device Marshall Source.
 */

//------------------------------------------------------------------------------
//
// iPod device marshall imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIPDLog.h"
#include "sbIPDUtils.h"
#include "sbIPDMarshall.h"

// Songbird imports.
#include <sbIDevice.h>
#include <sbIDeviceController.h>
#include <sbIDeviceControllerRegistrar.h>
#include <sbIDeviceEvent.h>

// Mozilla imports.
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsIWritablePropertyBag.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsComponentManagerUtils.h>


//------------------------------------------------------------------------------
//
// iPod device marshall nsISupports and nsIClassInfo implementation.
//
//------------------------------------------------------------------------------

// nsISupports implementation.
// NS_IMPL_THREADSAFE_ISUPPORTS1_CI(sbIPDMarshall, sbIDeviceMarshall)
NS_IMPL_THREADSAFE_ADDREF(sbIPDMarshall)
NS_IMPL_THREADSAFE_RELEASE(sbIPDMarshall)
NS_IMPL_QUERY_INTERFACE1_CI(sbIPDMarshall, sbIDeviceMarshall)
NS_IMPL_CI_INTERFACE_GETTER1(sbIPDMarshall, sbIDeviceMarshall)

// nsIClassInfo implementation.
NS_IMPL_THREADSAFE_CI(sbIPDMarshall)


//------------------------------------------------------------------------------
//
// iPod device marshall sbIDeviceMarshall implementation.
//
//------------------------------------------------------------------------------

/**
 * Loads all controllers registered with this marshall's category. Registers
 * each one with the registrar given.
 */

NS_IMETHODIMP
sbIPDMarshall::LoadControllers(sbIDeviceControllerRegistrar* aRegistrar)
{
  // Validate parameters.
  NS_ENSURE_ARG_POINTER(aRegistrar);

  // Register the controllers.
  RegisterControllers(aRegistrar);

  return NS_OK;
}


/**
 * After calling this method, the marshall will begin monitoring for any new
 * devices.
 */

NS_IMETHODIMP
sbIPDMarshall::BeginMonitoring()
{
  // Serialize.
  nsAutoMonitor mon(mMonitor);

  // Initialize event services.
  EventInitialize();

  // Scan for connected devices.
  ScanForConnectedDevices();

  return NS_OK;
}


/**
 * After calling this method, the marshall will stop monitoring for any new
 * devices.
 */

NS_IMETHODIMP
sbIPDMarshall::StopMonitoring()
{
  // Serialize.
  nsAutoMonitor mon(mMonitor);

  // Finalize the event services.
  EventFinalize();

  return NS_OK;
}


//
// Getters/setters.
//

/**
 * Gets the unique id associated with this marshall.
 */

NS_IMETHODIMP
sbIPDMarshall::GetId(nsID** aId)
{
  // Validate parameters.
  NS_ENSURE_ARG_POINTER(aId);

  // Allocate an nsID.
  nsID* pId = static_cast<nsID*>(NS_Alloc(sizeof(nsID)));
  NS_ENSURE_TRUE(pId, NS_ERROR_OUT_OF_MEMORY);

  // Return the ID.
  static nsID const id = SB_IPDMARSHALL_CID;
  *pId = id;
  *aId = pId;

  return NS_OK;
}


/**
 * A human-readable name for the marshall. Optional.
 */

NS_IMETHODIMP
sbIPDMarshall::GetName(nsAString& aName)
{
  aName.AssignLiteral(SB_IPDMARSHALL_CLASSNAME);
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// iPod device marshall public services.
//
//------------------------------------------------------------------------------

/**
 * Construct an iPod device marshall object.
 */

sbIPDMarshall::sbIPDMarshall() :
  sbBaseDeviceMarshall(NS_LITERAL_CSTRING(SB_DEVICE_CONTROLLER_CATEGORY)),
  mSBLibHalCtx(NULL)
{
  // Initialize the logging services.
  sbIPDLog::Initialize();

  // Initialize the device marshall services.
  Initialize();
}


/**
 * Destroy an iPod device marshall object.
 */

sbIPDMarshall::~sbIPDMarshall()
{
  // Finalize the device marshall services.
  Finalize();
}


//------------------------------------------------------------------------------
//
// iPod device marshall event services.
//
//------------------------------------------------------------------------------

/**
 * Initialize the device marshall event services.
 */

nsresult
sbIPDMarshall::EventInitialize()
{
  nsresult                    rv;

  // Trace execution.
  FIELD_LOG(("1: sbIPDMarshall::InitializeHal\n"));

  // Initialize the HAL library context.
  mSBLibHalCtx = new sbLibHalCtx();
  NS_ENSURE_TRUE(mSBLibHalCtx, NS_ERROR_OUT_OF_MEMORY);
  rv = mSBLibHalCtx->Initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  // Set up HAL library callbacks.  Listen for property modified events to
  // detect added iPods.  The neccessary properties are not yet available in the
  // device added callback.
  rv = mSBLibHalCtx->SetUserData(this);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mSBLibHalCtx->DevicePropertyWatchAll();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mSBLibHalCtx->SetDevicePropertyModified
                       (HandleLibHalDevicePropertyModified);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mSBLibHalCtx->SetDeviceRemoved(HandleLibHalDeviceRemoved);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Finalize the device marshall event services.
 */

void
sbIPDMarshall::EventFinalize()
{
  // Dispose of the HAL library context.
  if (mSBLibHalCtx)
    delete mSBLibHalCtx;
  mSBLibHalCtx = nsnull;
}


/**
 *   Handle the device property modified event for the device specified by
 * aDeviceUDI within the HAL library API context specified by aLibHalCtx.
 *   The modified property key is specified by aKey.  If the property was
 * removed, aIsRemoved is true.  If the property was added, aIsAdded is true.
 *
 * \param aLibHalCtx            HAL library API context.
 * \param aDeviceUDI            Modified device UDI.
 * \param aKey                  Modified property key.
 * \param aIsRemoved            True if property removed.
 * \param aIsAdded              True if property added.
 */

/* static */
void
sbIPDMarshall::HandleLibHalDevicePropertyModified(LibHalContext* aLibHalCtx,
                                                  const char*    aDeviceUDI,
                                                  const char*    aKey,
                                                  dbus_bool_t    aIsRemoved,
                                                  dbus_bool_t    aIsAdded)
{
  // Validate arguments.
  NS_ENSURE_TRUE(aLibHalCtx, /* void */);
  NS_ENSURE_TRUE(aDeviceUDI, /* void */);
  NS_ENSURE_TRUE(aKey, /* void */);

  // Get the iPod device system object.
  sbIPDMarshall* marshall =
                   (sbIPDMarshall *) libhal_ctx_get_user_data(aLibHalCtx);

  // Dispatch processing to the object.
  nsCAutoString deviceUDI;
  deviceUDI = aDeviceUDI;
  marshall->HandleLibHalDevicePropertyModified(deviceUDI,
                                               aKey,
                                               aIsRemoved,
                                               aIsAdded);
}

void sbIPDMarshall::HandleLibHalDevicePropertyModified(nsACString& aDeviceUDI,
                                                       const char* aKey,
                                                       dbus_bool_t aIsRemoved,
                                                       dbus_bool_t aIsAdded)
{
  // Probe device.
  if (!strcmp(aKey, "volume.is_mounted")) {
    ProbeDev(aDeviceUDI);
  }
}


/**
 * Handle the device removed event for the device specified by aDeviceUDI within
 * the HAL library API context specified by aLibHalCtx.
 *
 * \param aLibHalCtx            HAL library API context.
 * \param aDeviceUDI            Removed device UDI.
 */

/* static */
void
sbIPDMarshall::HandleLibHalDeviceRemoved(LibHalContext* aLibHalCtx,
                                         const char*    aDeviceUDI)
{
  // Validate arguments.
  NS_ENSURE_TRUE(aLibHalCtx, /* void */);
  NS_ENSURE_TRUE(aDeviceUDI, /* void */);

  // Get the iPod device system object.
  sbIPDMarshall* marshall =
                   (sbIPDMarshall *) libhal_ctx_get_user_data(aLibHalCtx);

  // Dispatch processing to the object.
  nsCAutoString deviceUDI;
  deviceUDI = aDeviceUDI;
  marshall->HandleLibHalDeviceRemoved(deviceUDI);
}

void
sbIPDMarshall::HandleLibHalDeviceRemoved(nsACString& aDeviceUDI)
{
  // Trace execution.
  FIELD_LOG(("1: sbIPDMarshall::HandleLibHalDeviceRemoved %s\n",
             aDeviceUDI.BeginReading()));

  // If the removed device is the media partition of a device in the device
  // list, remove the device.

  // Do nothing if the removed device is not in the device media partition
  // table.
  nsCString* iPodUDI;
  if (!mDeviceMediaPartitionTable.Get(aDeviceUDI, &iPodUDI))
    return;

  // Trace execution.
  FIELD_LOG(("2: sbIPDMarshall::HandleLibHalDeviceRemoved %s\n",
             iPodUDI->get()));

  // Handle the device removed event.
  HandleRemovedEvent(*iPodUDI, aDeviceUDI);
}


/**
 * Handle a device added event for the device specified by aDeviceUDI with the
 * media partition specified by aMediaPartitionUDI.
 *
 * \param aDeviceUDI            UDI of device.
 * \param aMediaPartitionUDI    UDI of device media partition.
 */

void
sbIPDMarshall::HandleAddedEvent(const nsACString& aDeviceUDI,
                                const nsACString& aMediaPartitionUDI)
{
  nsresult            rv;

  // Serialize.
  nsAutoMonitor mon(mMonitor);

  // Log progress.
  FIELD_LOG(("Enter: HandleAddedEvent device=%s media partition=%s\n",
             aDeviceUDI.BeginReading(),
             aMediaPartitionUDI.BeginReading()));

  // Do nothing if device has already been added.
  nsCOMPtr<sbIDevice> device;
  if (mDeviceList.Get(aDeviceUDI, getter_AddRefs(device)))
    return;

  // Set up the device properties.
  nsCOMPtr<nsIWritablePropertyBag>
    propBag = do_CreateInstance("@mozilla.org/hash-property-bag;1", &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);
  rv = propBag->SetProperty(NS_LITERAL_STRING("DeviceType"),
                            sbIPDVariant("iPod").get());
  NS_ENSURE_SUCCESS(rv, /* void */);
  rv = propBag->SetProperty(NS_LITERAL_STRING("DeviceUDI"),
                            sbIPDVariant(aDeviceUDI).get());
  NS_ENSURE_SUCCESS(rv, /* void */);
  rv = propBag->SetProperty(NS_LITERAL_STRING("MediaPartitionUDI"),
                            sbIPDVariant(aMediaPartitionUDI).get());
  NS_ENSURE_SUCCESS(rv, /* void */);

  // Find a controller for the device.  Do nothing more if none found.
  nsCOMPtr<sbIDeviceController> controller = FindCompatibleControllers(propBag);
  if (!controller)
    return;

  // Create a device.
  rv = controller->CreateDevice(propBag, getter_AddRefs(device));
  NS_ENSURE_SUCCESS(rv, /* void */);

  // Register the device.
  rv = mDeviceRegistrar->RegisterDevice(device);
  NS_ENSURE_SUCCESS(rv, /* void */);

  // Add device to device list and media partition to media partition list.
  mDeviceList.Put(aDeviceUDI, device);
  mDeviceMediaPartitionTable.Put(aMediaPartitionUDI, new nsCString(aDeviceUDI));

  // Send device added notification.
  CreateAndDispatchDeviceManagerEvent(sbIDeviceEvent::EVENT_DEVICE_ADDED,
                                      sbIPDVariant(device).get(),
                                      (sbIDeviceMarshall *) this);
}


/**
 * Handle a device removed event for the device specified by aDeviceUDI with the
 * media partition specified by aMediaPartitionUDI.
 *
 * \param aDeviceUDI            UDI of device.
 * \param aMediaPartitionUDI    UDI of device media partition.
 */

void
sbIPDMarshall::HandleRemovedEvent(const nsACString& aDeviceUDI,
                                  const nsACString& aMediaPartitionUDI)
{
  nsresult rv;

  // Serialize.
  nsAutoMonitor mon(mMonitor);

  // Get removed device.  Do nothing if device has not been added.
  nsCOMPtr<sbIDevice> device;
  if (!mDeviceList.Get(aDeviceUDI, getter_AddRefs(device)))
    return;

  // Log progress.
  FIELD_LOG(("Enter: HandleRemovedEvent %s\n", nsCString(aDeviceUDI).get()));

  // Get the device and device controller registrars.
  nsCOMPtr<sbIDeviceRegistrar> deviceRegistrar =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);
  nsCOMPtr<sbIDeviceControllerRegistrar> deviceControllerRegistrar =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);

  // Get the device controller ID and set it up for auto-disposal.
  nsID *controllerID = nsnull;
  rv = device->GetControllerId(&controllerID);
  sbAutoNSMemPtr autoControllerID(controllerID);

  // Get the device controller.
  nsCOMPtr<sbIDeviceController> deviceController;
  if (NS_SUCCEEDED(rv)) {
    rv = deviceControllerRegistrar->GetController
                                      (controllerID,
                                       getter_AddRefs(deviceController));
  }
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to get device controller.");
    deviceController = nsnull;
  }

  // Release the device from the controller.
  if (deviceController) {
    rv = deviceController->ReleaseDevice(device);
    if (NS_FAILED(rv))
      NS_WARNING("Failed to release device.");
  }

  // Unregister the device.
  rv = deviceRegistrar->UnregisterDevice(device);
  if (NS_FAILED(rv))
    NS_WARNING("Failed to unregister device.");

  // Remove device from device list and media partition from media partition
  // list.
  mDeviceList.Remove(aDeviceUDI);
  mDeviceMediaPartitionTable.Remove(aMediaPartitionUDI);

  // Send device added notification.
  CreateAndDispatchDeviceManagerEvent(sbIDeviceEvent::EVENT_DEVICE_REMOVED,
                                      sbIPDVariant(device).get(),
                                      (sbIDeviceMarshall *) this);
}


//------------------------------------------------------------------------------
//
// Internal iPod device marshall services.
//
//------------------------------------------------------------------------------

/**
 * Initialize the device marshall services.
 */

nsresult
sbIPDMarshall::Initialize()
{
  bool   success;
  nsresult rv;

  // Create the device marshall services monitor.
  mMonitor = nsAutoMonitor::NewMonitor("sbIPDMarshall.mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  // Get the device manager and registrar.
  mDeviceManager = do_GetService("@songbirdnest.com/Songbird/DeviceManager;2",
                                 &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mDeviceRegistrar = do_QueryInterface(mDeviceManager, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize the device list.
  success = mDeviceList.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  // Initialize the device media partition table.
  success = mDeviceMediaPartitionTable.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  return NS_OK;
}


/**
 * Finalize the device marshall services.
 */

void
sbIPDMarshall::Finalize()
{
  // Dispose of the device marshall services monitor.
  if (mMonitor)
  {
    nsAutoMonitor::DestroyMonitor(mMonitor);
    mMonitor = nsnull;
  }

  // Release object references.
  mDeviceManager = nsnull;
  mDeviceRegistrar = nsnull;
}


/**
 * Scan for connected devices and handle any as a device added event.
 */

nsresult
sbIPDMarshall::ScanForConnectedDevices()
{
  nsCStringArray              deviceList;
  nsCString                   deviceUDI;
  nsCString                   iPodUDI;
  PRInt32                     i;
  nsresult                    rv;

  // Probe all devices.
  rv = mSBLibHalCtx->GetAllDevices(deviceList);
  NS_ENSURE_SUCCESS(rv, rv);
  for (i = 0; i < deviceList.Count(); i++)
  {
    // Get the device UDI.
    deviceUDI = *(deviceList[i]);

    // Probe the device.
    ProbeDev(deviceUDI);
  }

  return NS_OK;
}


/**
 *   Probe the device specified by aProbeUDI to determine if it is a newly added
 * iPod device.  If it is, send a device added event.
 *   This function looks for mounted device media partitions.
 *
 * \param aProbeUDI             UDI of device to probe.
 */

nsresult
sbIPDMarshall::ProbeDev(nsACString &aProbeUDI)
{
  nsresult rv;

  // Trace execution.
  FIELD_LOG(("ProbeDev aProbeUDI=%s\n", aProbeUDI.BeginReading()));

  // If probed device is not a media partition, do nothing more.
  nsCAutoString mediaPartitionUDI;
  if (!IsMediaPartition(aProbeUDI))
    return NS_OK;
  mediaPartitionUDI = aProbeUDI;

  // If the media partition is not mounted, do nothing more.
  nsCAutoString mountPoint;
  rv = mSBLibHalCtx->DeviceGetPropertyString(mediaPartitionUDI,
                                             "volume.mount_point",
                                             mountPoint);
  if (NS_FAILED(rv) || mountPoint.IsEmpty()) {
    return NS_OK;
  }

  // Get the media partition's parent.  This is the device UDI for iPods.
  nsCAutoString parentUDI;
  rv = mSBLibHalCtx->DeviceGetPropertyString(mediaPartitionUDI,
                                             "info.parent",
                                             parentUDI);
  if (NS_FAILED(rv))
    return NS_OK;
  FIELD_LOG(("ProbeDev parentUDI=%s\n", parentUDI.get()));

  // If probed device parent is not an iPod, do nothing more.
  nsCAutoString iPodUDI;
  if (!IsIPod(parentUDI))
    return NS_OK;
  iPodUDI = parentUDI;

  // Send a device added event.
  HandleAddedEvent(iPodUDI, mediaPartitionUDI);

  return NS_OK;
}


/**
 * Test whether the device specified by aDeviceUDI is an iPod.  If it is, return
 * true; otherwise, return false.
 *
 * \param aDeviceUDI            UDI of device to test.
 *
 * \return                      True if device is an iPod.
 */

bool
sbIPDMarshall::IsIPod(nsACString& aDeviceUDI)
{
  nsresult                    rv;

  // Trace execution.
  FIELD_LOG(("1: sbIPDMarshall::IsIPod %s\n", aDeviceUDI.BeginReading()));

  // Get the device info vendor and product properties.
  nsCAutoString vendor;
  nsCAutoString product;
  rv = mSBLibHalCtx->DeviceGetPropertyString(aDeviceUDI, "info.vendor", vendor);
  if (NS_FAILED(rv))
    return PR_FALSE;
  rv = mSBLibHalCtx->DeviceGetPropertyString(aDeviceUDI,
                                             "info.product",
                                             product);
  if (NS_FAILED(rv))
    return PR_FALSE;

  // Trace execution.
  FIELD_LOG(("2: sbIPDMarshall::IsIPod vendor=%s product=%s\n",
             vendor.get(),
             product.get()));

  // Check device vendor and product properties.
  if (!vendor.Equals("Apple") || !product.Equals("iPod"))
    return PR_FALSE;

  return PR_TRUE;
}


/**
 * Check if the device with the HAL library UDI specified by aDeviceUDI is a
 * media partition.  If it is, return true; otherwise, return false.
 *
 * \param aDeviceUDI            Device UDI.
 *
 * \return                      True if device is a media partition; false
 *                              otherwise.
 */

bool
sbIPDMarshall::IsMediaPartition(nsACString& aDeviceUDI)
{
  nsresult                    rv;

  // Check if device has a volume interface.
  bool hasInterface;
  rv = mSBLibHalCtx->DeviceHasInterface(aDeviceUDI,
                                        "org.freedesktop.Hal.Device.Volume",
                                        &hasInterface);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!hasInterface)
    return PR_FALSE;

  return PR_TRUE;
}

nsresult
sbIPDMarshall::RemoveDevice(sbIDevice* aDevice)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
