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
#include "sbIPDMarshall.h"
#include "sbIPDWindowsUtils.h"

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
#include <prprf.h>

// Win32 imports.
#include <dbt.h>
#include <devioctl.h>
#include <tchar.h>

#include <ntddstor.h>

// Fix up some Win32 conflicts.
#undef CreateEvent


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

  // Initialize the event services.
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
  mEventWindow((HWND) INVALID_HANDLE_VALUE),
  mEventWindowClass(NULL)
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
  // Create an event window class set up with the device event handler.
  WNDCLASS wndClass;
  ZeroMemory(&wndClass, sizeof(wndClass));
  wndClass.hInstance = GetModuleHandle(NULL);
  wndClass.lpfnWndProc = WindowEventHandler;
  wndClass.lpszClassName = _T(SB_IPDMARSHALL_CLASSNAME);
  mEventWindowClass = RegisterClass(&wndClass);
  NS_ENSURE_TRUE(mEventWindowClass, NS_ERROR_UNEXPECTED);

  // Create a window for receiving device events.
  mEventWindow = CreateWindow(MAKEINTATOM(mEventWindowClass),
                              NULL,
                              WS_POPUP,
                              0,
                              0,
                              1,
                              1,
                              NULL,
                              NULL,
                              GetModuleHandle(NULL),
                              NULL);
  NS_ENSURE_TRUE(mEventWindow != INVALID_HANDLE_VALUE, NS_ERROR_UNEXPECTED);
  SetLastError(0);
  SetWindowLong(mEventWindow, GWL_USERDATA, (LONG) this);
  NS_ENSURE_FALSE(GetLastError(), NS_ERROR_UNEXPECTED);

  return NS_OK;
}


/**
 * Finalize the device marshall event services.
 */

void
sbIPDMarshall::EventFinalize()
{
  // Destroy the device event window.
  if (mEventWindow != INVALID_HANDLE_VALUE) {
    DestroyWindow(mEventWindow);
    mEventWindow = (HWND) INVALID_HANDLE_VALUE;
  }

  // Unregister the device event window class.
  if (mEventWindowClass) {
    UnregisterClass(MAKEINTATOM(mEventWindowClass), GetModuleHandle(NULL));
    mEventWindowClass = NULL;
  }
}


/**
 * Handle device window events for the window specified by hwnd.  The event is
 * specified by msg, param1, and param2.
 *
 * \param hwnd                  Window handle.
 * \param msg                   Event message.
 * \param param1                Event parameter 1.
 * \param param2                Event parameter 2.
 *
 * \return A zero return value causes the window to close.
 */


/* static */
LRESULT CALLBACK
sbIPDMarshall::WindowEventHandler(HWND   hwnd,
                                  UINT   msg,
                                  WPARAM param1,
                                  LPARAM param2)
{
  // Trace execution.
  FIELD_LOG(("Enter: sbIPDMarshall::WindowEventHandler\n"));

  // Handle the event within an object context.
  sbIPDMarshall* marshall = (sbIPDMarshall *) GetWindowLong(hwnd, GWL_USERDATA);
  NS_ENSURE_TRUE(marshall, 1);
  marshall->_WindowEventHandler(hwnd, msg, param1, param2);

  // Trace execution.
  FIELD_LOG(("Exit: sbIPDMarshall::WindowEventHandler\n"));

  return 1;
}

void
sbIPDMarshall::_WindowEventHandler(HWND   hwnd,
                                   UINT   msg,
                                   WPARAM param1,
                                   LPARAM param2)
{
  // Only handle device events.
  if (msg != WM_DEVICECHANGE)
    return;

  // Only handle volume device events.
  PDEV_BROADCAST_HDR pDevBroadcastHdr = (PDEV_BROADCAST_HDR) param2;
  if (!pDevBroadcastHdr)
    return;
  if (pDevBroadcastHdr->dbch_devicetype != DBT_DEVTYP_VOLUME)
    return;

  // Get the broadcast volume event data record.
  PDEV_BROADCAST_VOLUME pDevBroadcastVolume = (PDEV_BROADCAST_VOLUME) param2;

  // Get the volume drive letter.
  char driveLetter;
  int unitmask = pDevBroadcastVolume->dbcv_unitmask;
  for (driveLetter = 'A'; driveLetter <= 'Z'; driveLetter++) {
    if (unitmask & 0x01)
      break;
    unitmask >>= 1;
  }

  // Dispatch event handling.
  switch (param1)
  {
    case DBT_DEVICEARRIVAL :
      HandleAddedEvent(driveLetter);
      break;

    case DBT_DEVICEREMOVECOMPLETE :
      HandleRemovedEvent(driveLetter);
      break;

    default :
      break;
  }
}


/**
 * Handle a device added event for the device with the drive letter specified by
 * aDriveLetter.
 */

void
sbIPDMarshall::HandleAddedEvent(char aDriveLetter)
{
  nsCOMPtr<sbIDevice> device;
  nsresult            rv;

  // Serialize.
  nsAutoMonitor mon(mMonitor);

  // Do nothing if device is not an iPod.
  if (!IsIPod(aDriveLetter))
    return;

  // Log progress.
  FIELD_LOG(("Enter: HandleAddedEvent %c\n", aDriveLetter));

  // Do nothing if device has already been added.
  if (mDeviceList.Get(aDriveLetter, getter_AddRefs(device)))
    return;

  // Set up the device properties.
  nsCOMPtr<nsIWritablePropertyBag>
    propBag = do_CreateInstance("@mozilla.org/hash-property-bag;1", &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);
  rv = propBag->SetProperty(NS_LITERAL_STRING("DeviceType"),
                            sbIPDVariant("iPod").get());
  NS_ENSURE_SUCCESS(rv, /* void */);
  rv = propBag->SetProperty(NS_LITERAL_STRING("DriveLetter"),
                            sbIPDVariant(aDriveLetter).get());
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

  // Add device to device list.
  mDeviceList.Put(aDriveLetter, device);

  // Send device added notification.
  CreateAndDispatchDeviceManagerEvent(sbIDeviceEvent::EVENT_DEVICE_ADDED,
                                      sbIPDVariant(device).get(),
                                      (sbIDeviceMarshall *) this);
}


/**
 * Handle a device removed event for the device with the drive letter specified
 * by aDriveLetter.
 */

void
sbIPDMarshall::HandleRemovedEvent(char aDriveLetter)
{
  nsresult rv;

  // Serialize.
  nsAutoMonitor mon(mMonitor);

  // Get removed device.  Do nothing if device has not been added.
  nsCOMPtr<sbIDevice> device;
  if (!mDeviceList.Get(aDriveLetter, getter_AddRefs(device)))
    return;

  // Log progress.
  FIELD_LOG(("Enter: HandleRemovedEvent %c\n", aDriveLetter));

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

  // Remove device from device list.
  mDeviceList.Remove(aDriveLetter);

  // Send device removed notification.
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
  PRBool   success;
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
  // Scan all drive letters for an iPod device and simulate a device added
  // event.
  for (char driveLetter = 'A'; driveLetter <= 'Z'; driveLetter++) {
    HandleAddedEvent(driveLetter);
  }

  return NS_OK;
}


/**
 * Test whether the device with the drive letter specified by aDriveLetter is
 * an iPod.  If it is, return true; otherwise, return false.
 *
 * \param aDriveLetter          Device drive letter.
 * \return True if device is an iPod.
 */

PRBool
sbIPDMarshall::IsIPod(char aDriveLetter)
{
  PRBool isIPod = PR_FALSE;
  BOOL   success;

  // Produce the device path.
  char devicePath[8];
  PR_snprintf(devicePath, sizeof(devicePath), "\\\\.\\%c:", aDriveLetter);

  // Trace execution.
  FIELD_LOG(("1: IsIPod %s\n", devicePath));

  // Create a file interface for the device and set up auto-closing for the
  // handle.
  HANDLE hDev = CreateFileA(devicePath,
                            0,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);
  if (hDev == INVALID_HANDLE_VALUE)
    return PR_FALSE;
  sbAutoHANDLE autoHDev(hDev);

  // Trace execution.
  FIELD_LOG(("2: IsIPod\n"));

  // Query the device for information.
  STORAGE_PROPERTY_QUERY query;
  UCHAR                  queryData[512];
  DWORD                  queryDataSize;
  query.PropertyId = StorageDeviceProperty;
  query.QueryType = PropertyStandardQuery;
  success = DeviceIoControl(hDev,
                            IOCTL_STORAGE_QUERY_PROPERTY,
                            &query,
                            sizeof (STORAGE_PROPERTY_QUERY),
                            &queryData,
                            sizeof(queryData),
                            &queryDataSize,
                            NULL);
  if (!success)
    return PR_FALSE;

  // Get the device vendor and product IDs.
  PSTORAGE_DEVICE_DESCRIPTOR pDevDesc = (PSTORAGE_DEVICE_DESCRIPTOR) queryData;
  const char* vendorID = (const char *) &(queryData[pDevDesc->VendorIdOffset]);
  const char* productID =
                (const char *) &(queryData[pDevDesc->ProductIdOffset]);

  // Trace execution.
  FIELD_LOG(("3: IsIPod \"%s\" \"%s\"\n", vendorID, productID));

  // Check for an iPod device.
  isIPod = PR_TRUE;
  if (strncmp("Apple", vendorID, strlen("Apple")))
    isIPod = PR_FALSE;
  else if (strncmp("iPod", productID, strlen("iPod")))
    isIPod = PR_FALSE;

  /* Trace execution. */
  FIELD_LOG(("4: IsIPod %d\n", isIPod));

  return isIPod;
}


nsresult
sbIPDMarshall::RemoveDevice(sbIDevice* aDevice)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
