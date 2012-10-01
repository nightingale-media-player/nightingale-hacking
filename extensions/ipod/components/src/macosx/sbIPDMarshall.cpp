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

// Self import.
#include "sbIPDMarshall.h"

// Local imports.
#include "sbIPDLog.h"
#include "sbIPDUtils.h"

// Songbird imports.
#include <sbIDevice.h>
#include <sbIDeviceController.h>
#include <sbIDeviceControllerRegistrar.h>
#include <sbIDeviceEvent.h>

// Mozilla imports.
#include <nsComponentManagerUtils.h>
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsIWritablePropertyBag.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>

#include <sbDebugUtils.h>


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
  nsresult rv;

  // Serialize.
  nsAutoMonitor mon(mMonitor);

  // Initialize the monitor services.
  rv = InitializeVolumeMonitor();
  NS_ENSURE_SUCCESS(rv, rv);

  // Scan for connected devices.
  rv = ScanForConnectedDevices();
  NS_ENSURE_SUCCESS(rv, rv);

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

  // Finalize the monitor services.
  FinalizeVolumeMonitor();

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
  mVolumeEventHandler(NULL),
  mVolumeEventHandlerRef(NULL)
{
  nsresult rv;

  SB_PRLOG_SETUP(sbIPDMarshall);

  // Create the device marshall services monitor.
  mMonitor = nsAutoMonitor::NewMonitor("sbIPDMarshall.mMonitor");
  NS_ENSURE_TRUE(mMonitor, /* void */);

  // Get the device manager and registrar.
  mDeviceManager = do_GetService("@songbirdnest.com/Songbird/DeviceManager;2",
                                 &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);
  mDeviceRegistrar = do_QueryInterface(mDeviceManager, &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);

  // Initialize the device list.
  mDeviceList.Init();
}


/**
 * Destroy an iPod device marshall object.
 */

sbIPDMarshall::~sbIPDMarshall()
{
  // Finalize the monitor services.
  FinalizeVolumeMonitor();

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


//------------------------------------------------------------------------------
//
// iPod device marshall monitor services.
//
//------------------------------------------------------------------------------

/**
 * Initialize the device marshall monitor services.
 */

nsresult
sbIPDMarshall::InitializeVolumeMonitor()
{
  EventTypeSpec volumeEventTypes[] = 
      { { kEventClassVolume, kEventVolumeMounted },
        { kEventClassVolume, kEventVolumeUnmounted } };
  OSStatus error = noErr;

  // Set up to finalize monitor services on error.
  sbAutoFinalizeVolumeMonitor autoFinalizeVolumeMonitor(this);

  // Install a volume event handler.
  mVolumeEventHandler = NewEventHandlerUPP(VolumeEventHandler);
  NS_ENSURE_TRUE(mVolumeEventHandler, NS_ERROR_OUT_OF_MEMORY);
  error = InstallApplicationEventHandler(mVolumeEventHandler, 
                                         GetEventTypeCount(volumeEventTypes),
                                         volumeEventTypes,
                                         this, 
                                         &mVolumeEventHandlerRef);
  NS_ENSURE_TRUE(error == noErr, NS_ERROR_FAILURE);

  // Don't finalize monitor services.
  autoFinalizeVolumeMonitor.forget();

  return NS_OK;
}


/**
 * Finalize the device marshall monitor services.
 */

void
sbIPDMarshall::FinalizeVolumeMonitor()
{
  // Remove the volume event handler.
  if (mVolumeEventHandlerRef) {
    RemoveEventHandler(mVolumeEventHandlerRef);
    mVolumeEventHandlerRef = NULL;
  }
  if (mVolumeEventHandler) {
    DisposeEventHandlerUPP(mVolumeEventHandler);
    mVolumeEventHandler = NULL;
  }
}


/**
 * Handle the volume event specified by aEventRef.  The iPod device marshall
 * object is retrieved from the handler context specified by aHandlerCtx.
 * aNextHandler is not used.
 * Don't consume iPod device events to allow other handlers to receive those
 * events.
 *
 * \param aNextHandler          Next handler in the event handler chain.
 * \param aEventRef             Event reference.
 * \param aHandlerCtx           iPod device marshall object.
 *
 * \return eventNotHandledErr   This function never consumes the event.
 */

/* static */
pascal OSStatus 
sbIPDMarshall::VolumeEventHandler(EventHandlerCallRef aNextHandler, 
                                  EventRef            aEventRef,
                                  void*               aHandlerCtx)
{
  // Validate arguments
  NS_ENSURE_TRUE(aHandlerCtx, eventNotHandledErr);

  // Trace execution.
  FIELD_LOG(("Enter: sbIPDMarshall::VolumeEventHandler\n"));

  // Handle the event.
  sbIPDMarshall* psbIPDMarshall = (sbIPDMarshall *) aHandlerCtx;
  psbIPDMarshall->VolumeEventHandler(aNextHandler, aEventRef);

  // Trace execution.
  FIELD_LOG(("Exit: sbIPDMarshall::VolumeEventHandler\n"));

  // Indicate that the event was not consumed.
  return eventNotHandledErr;
}

void 
sbIPDMarshall::VolumeEventHandler(EventHandlerCallRef aNextHandler, 
                                  EventRef            aEventRef)
{
  OSStatus error = noErr;

  // Get the volume reference number.
  FSVolumeRefNum volumeRefNum = kFSInvalidVolumeRefNum;
  error = GetEventParameter(aEventRef,
                            kEventParamDirectObject, 
                            typeFSVolumeRefNum,
                            NULL,
                            sizeof(volumeRefNum),
                            NULL,
                            &volumeRefNum);
  NS_ENSURE_TRUE(error == noErr, /* void */);

  // Get the event type.
  UInt32 eventType = GetEventKind(aEventRef);

  // Dispatch event processing.
  switch (eventType)
  {
      case kEventVolumeMounted :
        // Send iPod added event if volume is an iPod volume.
        if (IsIPod(volumeRefNum))
          HandleAddedEvent(volumeRefNum);
        break;

      case kEventVolumeUnmounted :
        HandleRemovedEvent(volumeRefNum);
        break;

      default :
        break;
  }
}


/**
 * Handle processing of the device added event for the iPod device with the
 * volume reference number specified by aVolumeRefNum.
 *
 * \param aVolumeRefNum         Volume reference number of added iPod device.
 */

void 
sbIPDMarshall::HandleAddedEvent(FSVolumeRefNum aVolumeRefNum)
{
  nsCOMPtr<sbIDevice> device;
  nsresult            rv;

  // Serialize.
  nsAutoMonitor mon(mMonitor);

  // Log progress.
  FIELD_LOG(("Enter: HandleAddedEvent %d\n", aVolumeRefNum));

  // Do nothing if device has already been added.
  if (mDeviceList.Get(aVolumeRefNum, getter_AddRefs(device)))
    return;

  // Set up the device properties.
  nsCOMPtr<nsIWritablePropertyBag>
    propBag = do_CreateInstance("@mozilla.org/hash-property-bag;1", &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);
  rv = propBag->SetProperty(NS_LITERAL_STRING("DeviceType"),
                            sbIPDVariant("iPod").get());
  NS_ENSURE_SUCCESS(rv, /* void */);

  rv = propBag->SetProperty(NS_LITERAL_STRING("VolumeRefNum"),
                            sbIPDVariant(aVolumeRefNum).get());
  NS_ENSURE_SUCCESS(rv, /* void */);
  nsAutoString mountPoint;
  rv = GetVolumePath(aVolumeRefNum, mountPoint);
  NS_ENSURE_SUCCESS(rv, /* void */);
  rv = propBag->SetProperty(NS_LITERAL_STRING("MountPath"),
                            sbIPDVariant(mountPoint).get());
  NS_ENSURE_SUCCESS(rv, /* void */);

  nsAutoString firewireGUID;
  rv = GetFirewireGUID(aVolumeRefNum, firewireGUID);
  NS_ENSURE_SUCCESS(rv, /* void */);
  rv = propBag->SetProperty(NS_LITERAL_STRING("FirewireGUID"),
                            sbIPDVariant(firewireGUID).get());
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
  mDeviceList.Put(aVolumeRefNum, device);

  // Send device added notification.
  CreateAndDispatchDeviceManagerEvent(sbIDeviceEvent::EVENT_DEVICE_ADDED,
                                      sbIPDVariant(device).get(),
                                      (sbIDeviceMarshall *) this);
}


/**
 * Handle processing of the device removed event for the device with the volume
 * reference number specified by aVolumeRefNum.
 *
 * \param aVolumeRefNum         Volume reference number of removed device.
 */

void 
sbIPDMarshall::HandleRemovedEvent(FSVolumeRefNum aVolumeRefNum)
{
  nsresult rv;

  // Log progress.
  FIELD_LOG(("Enter: HandleRemovedEvent %d\n", aVolumeRefNum));

  // Get removed device.  Do nothing if device has not been added.
  nsCOMPtr<sbIDevice> device;
  if (!mDeviceList.Get(aVolumeRefNum, getter_AddRefs(device)))
    return;

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
  mDeviceList.Remove(aVolumeRefNum);

  // Send device removed notification.
  CreateAndDispatchDeviceManagerEvent(sbIDeviceEvent::EVENT_DEVICE_REMOVED,
                                      sbIPDVariant(device).get(),
                                      (sbIDeviceMarshall *) this);

  // Log progress.
  FIELD_LOG(("Enter: HandleRemovedEvent\n"));
}


//------------------------------------------------------------------------------
//
// Internal iPod device marshall services.
//
//------------------------------------------------------------------------------

/**
 * Scan for all connected iPod devices and add each one found.
 */

nsresult
sbIPDMarshall::ScanForConnectedDevices()
{
  OSStatus error;

  // Trace execution.
  FIELD_LOG(("Enter: sbIPDMarshall::ScanForConnectedDevices\n"));

  // Search for iPod volumes and send iPod added events for all connected iPods.
  for (ItemCount volumeIndex = 1; ; volumeIndex++) {
    // Get the volume info.
    FSVolumeInfo volumeInfo;
    FSVolumeRefNum volumeRefNum;
    HFSUniStr255 volumeName;
    bzero(&volumeInfo, sizeof(volumeInfo));
    error = FSGetVolumeInfo(kFSInvalidVolumeRefNum,
                            volumeIndex, 
                            &volumeRefNum,
                            kFSVolInfoFSInfo,
                            &volumeInfo,
                            &volumeName,
                            NULL);
    if (error == nsvErr)
      break;
    if (error != noErr)
      continue;

    // Trace execution.
    FIELD_LOG(("1: sbIPDMarshall::ScanForConnectedDevices %d\n", volumeRefNum));

    // Send iPod added event if volume is an iPod volume.
    if (IsIPod(volumeRefNum)) {
      HandleAddedEvent(volumeRefNum);
    }
  }

  // Trace execution.
  FIELD_LOG(("Exit: sbIPDMarshall::ScanForConnectedDevices\n"));

  return NS_OK;
}


/*
 * IsIPod
 *
 *   --> volumeRefNum           Device volume reference number.
 *
 *   <--                        True if device is an iPod.
 *
 *   This function tests whether the device mounted on the volume specified by
 * volumeRefNum is an iPod.  If it is, this function returns true; otherwise, it
 * returns false.
 */

bool 
sbIPDMarshall::IsIPod(FSVolumeRefNum volumeRefNum)
{
  GetVolParmsInfoBuffer volumeParms;
  HParamBlockRec pb;
  char *bsdDevName = NULL;
  CFMutableDictionaryRef dictionaryRef;
  io_service_t volNode = (io_service_t) NULL;
  io_registry_entry_t scsiNode = (io_registry_entry_t) NULL;
  bool isIPod = PR_FALSE;
  OSStatus error = noErr;

  /* Trace execution. */
  LOG("1: IPodDevIfIsIPod\n");

  /* Get the BSD device name. */
  pb.ioParam.ioNamePtr = NULL;
  pb.ioParam.ioVRefNum = volumeRefNum;
  pb.ioParam.ioBuffer = (Ptr) &volumeParms;
  pb.ioParam.ioReqCount = sizeof(volumeParms);
  error = PBHGetVolParmsSync(&pb);
  if (error == noErr) {
    bsdDevName = (char *) volumeParms.vMDeviceID;
  }

  /* Trace execution. */
  LOG("2: IPodDevIfIsIPod %s\n", bsdDevName);

  /* Get the volume I/O registry node. */
  if (error == noErr) {
    dictionaryRef = IOBSDNameMatching(kIOMasterPortDefault, 0, bsdDevName);
    if (dictionaryRef == NULL) {
      error = -1;
    }
  }
  if (error == noErr) {
    volNode = IOServiceGetMatchingService(kIOMasterPortDefault,
                        dictionaryRef);
    if (volNode == ((io_service_t) NULL)) {
      error = -1;
    }
  }

  /* Trace execution. */
  LOG("2: IPodDevIfIsIPod %d\n", (int) error);

  /* Search for a parent SCSI I/O node. */
  if (error == noErr) {
    io_registry_entry_t     node = (io_registry_entry_t) NULL;
    io_registry_entry_t     parentNode;
    kern_return_t         result = KERN_SUCCESS;
    Boolean           done = FALSE;

    /* Search from the volume node. */
    node = volNode;
    while (!done) {
      /* Get the node parent. */
      result = IORegistryEntryGetParentEntry(node, kIOServicePlane, 
          &parentNode);
      IOObjectRelease(node);
      node = (io_registry_entry_t) NULL;
      if (result == KERN_SUCCESS) {
        node = parentNode;
      } else {
        done = TRUE;
      }

      /* Check if parent is a SCSI peripheral device nub node. */
      if (result == KERN_SUCCESS) {
        if (IOObjectConformsTo(node, "IOSCSIPeripheralDeviceNub")) {
          scsiNode = node;
          node = (io_registry_entry_t) NULL;
          done = TRUE;
        }
      }
    }

    /* Clean up. */
    if (node != ((io_registry_entry_t) NULL)) {
      IOObjectRelease(node);
    }
  }

  /* Trace execution. */
  LOG("3: IPodDevIfIsIPod 0x%08x\n", scsiNode);

  /* If a SCSI I/O node was found, check if device is an iPod. */
  if ((error == noErr) && (scsiNode != ((io_registry_entry_t) NULL))) {
    __CFString          *vendorID = NULL;
    __CFString          *productID = NULL;

    /* Get the SCSI vendor and product IDs. */
    vendorID = (__CFString *) IORegistryEntryCreateCFProperty(scsiNode, 
        CFSTR("Vendor Identification"), kCFAllocatorDefault, 0);
    productID = (__CFString *) IORegistryEntryCreateCFProperty(scsiNode, 
        CFSTR("Product Identification"), kCFAllocatorDefault, 0);

    /* Trace execution. */
    LOG("4: IPodDevIfIsIPod \"%s\" \"%s\"\n", (char *) vendorID, 
          (char *) productID);

    /* Check if SCSI device is an iPod. */
    isIPod = PR_TRUE;
    if (CFStringCompare(CFSTR("Apple"), vendorID, 0)) {
      isIPod = PR_FALSE;
    } else if (CFStringCompare(CFSTR("iPod"), productID, 0)) {
      isIPod = PR_FALSE;
    }

    /* Clean up. */
    if (vendorID != NULL) {
      CFRelease(vendorID);
    }
    if (productID != NULL) {
      CFRelease(productID);
    }
  }

  /* Trace execution. */
  LOG("5: IPodDevIfIsIPod %d\n", (int) error);

  /* Clean up. */
  if (scsiNode != ((io_registry_entry_t) NULL)) {
    IOObjectRelease(scsiNode);
  }
  if (volNode != ((io_service_t) NULL)) {
    IOObjectRelease(volNode);
  }

  return (isIPod);
}


/*
 * GetVolumePath
 *
 *   --> aVolumeRefNum          Volume reference number.
 *   <-- aVolumePath            Volume file path.
 *
 *   This function returns in aVolumePath the file path to the volume specified
 * by aVolumeRefNum.
 */

nsresult
sbIPDMarshall::GetVolumePath(FSVolumeRefNum aVolumeRefNum,
                             nsAString      &aVolumePath)
{
  OSStatus error;

  /* Get the volume root. */
  FSRef volumeFSRef;
  error = FSGetVolumeInfo(aVolumeRefNum,
                          0,
                          NULL,
                          kFSVolInfoNone,
                          NULL,
                          NULL,
                          &volumeFSRef);
  NS_ENSURE_TRUE(error == noErr, NS_ERROR_FAILURE);

  /* Get the volume root URL. */
  cfref<CFURLRef> volumeURLRef = CFURLCreateFromFSRef(kCFAllocatorDefault,
                                                      &volumeFSRef);
  NS_ENSURE_TRUE(volumeURLRef, NS_ERROR_FAILURE);

  /* Get the volume root path CFStringRef. */
  cfref<CFStringRef> volumePathStringRef =
                       CFURLCopyFileSystemPath(volumeURLRef,
                                               kCFURLPOSIXPathStyle);
  NS_ENSURE_TRUE(volumePathStringRef, NS_ERROR_FAILURE);

  /* Get the volume root path nsString. */
  CFIndex volumePathLength = CFStringGetLength(volumePathStringRef);
  PRUnichar* volumePath = aVolumePath.BeginWriting(volumePathLength);
  NS_ENSURE_TRUE(volumePath, NS_ERROR_OUT_OF_MEMORY);
  CFStringGetCharacters(volumePathStringRef,
                        CFRangeMake(0, volumePathLength),
                        volumePath);

  return NS_OK;
}


/*
 * GetFirewireGUID
 *
 *   --> aVolumeRefNum          Volume reference number.
 *   <-- aFirewireGUID          Volume Firewire GUID
 *
 *   This function returns in aVolumePath the file path to the volume specified
 * by aVolumeRefNum.
 */

nsresult 
sbIPDMarshall::GetFirewireGUID(FSVolumeRefNum aVolumeRefNum, 
    nsAString &aFirewireGUID)
{
  GetVolParmsInfoBuffer volumeParms;
  HParamBlockRec pb;
  char *bsdDevName = NULL;
  CFMutableDictionaryRef dictionaryRef;
  io_service_t volNode = (io_service_t) NULL;
  io_registry_entry_t usbDevNode = (io_registry_entry_t) NULL;
  OSStatus error = noErr;

  /* Get the BSD device name. */
  pb.ioParam.ioNamePtr = NULL;
  pb.ioParam.ioVRefNum = aVolumeRefNum;
  pb.ioParam.ioBuffer = (Ptr) &volumeParms;
  pb.ioParam.ioReqCount = sizeof(volumeParms);
  error = PBHGetVolParmsSync(&pb);
  if (error == noErr) {
    bsdDevName = (char *) volumeParms.vMDeviceID;
  }

  /* Get the volume I/O registry node. */
  if (error == noErr) {
    dictionaryRef = IOBSDNameMatching(kIOMasterPortDefault, 0, bsdDevName);
    if (dictionaryRef == NULL) {
      error = -1;
    }
  } 
  if (error == noErr) {
    volNode = IOServiceGetMatchingService(kIOMasterPortDefault, dictionaryRef);
    if (volNode == ((io_service_t) NULL)) {
      error = -1;
    }
  }

  /* Search for a parent USB device node. */
  if (error == noErr) {
    io_registry_entry_t node = (io_registry_entry_t) NULL;
    io_registry_entry_t parentNode;
    kern_return_t result = KERN_SUCCESS;
    Boolean done = FALSE;

    /* Search from the volume node. */
    node = volNode;
    IOObjectRetain(volNode);
    while (!done) {
      /* Get the node parent. */
      result = IORegistryEntryGetParentEntry(node, kIOServicePlane, 
          &parentNode);
      IOObjectRelease(node);
      node = (io_registry_entry_t) NULL;
      if (result == KERN_SUCCESS) {
        node = parentNode;
      } else {
        done = TRUE;
      }

      /* Check if parent is a USB device node. */
      if (result == KERN_SUCCESS) {
        if (IOObjectConformsTo(node, "IOUSBDevice")) {
          usbDevNode = node;
          node = (io_registry_entry_t) NULL;
          done = TRUE;
        }
      }
    }

    /* Clean up. */
    if (node != ((io_registry_entry_t) NULL)) {
      IOObjectRelease(node);
    }
  }

  /* If a USB device node was found, and the   */
  /* device is an iPod, get the serial number. */
  if ((error == noErr) && (usbDevNode != ((io_registry_entry_t) NULL))) {
    __CFString *serialNumber = NULL;
    char cSerialNumber[256];
    Boolean success;

    /* Get the serial number. */
    serialNumber = (__CFString *) IORegistryEntryCreateCFProperty(usbDevNode, 
        CFSTR("USB Serial Number"), kCFAllocatorDefault, 0);

    /* Assign the Firewire GUID. */
    if (serialNumber) {
      success = CFStringGetCString(serialNumber, cSerialNumber, 
          sizeof(cSerialNumber), kCFStringEncodingUTF8);
      if (success) {
        aFirewireGUID.AssignLiteral(cSerialNumber);
      }
    }

    /* Clean up. */
    if (serialNumber != NULL) {
      CFRelease(serialNumber);
    }
  }

  /* Clean up. */
  if (usbDevNode != ((io_registry_entry_t) NULL)) {
    IOObjectRelease(usbDevNode);
  }
  if (volNode != ((io_service_t) NULL)) {
    IOObjectRelease(volNode);
  }

  return (error == noErr) ? NS_OK : NS_ERROR_UNEXPECTED;
}

nsresult
sbIPDMarshall::RemoveDevice(sbIDevice* aDevice)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
