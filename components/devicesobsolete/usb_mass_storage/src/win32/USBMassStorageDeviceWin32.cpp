/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

/** 
* \file  USBMassStorageDeviceWin32.cpp
* \brief Songbird USBMassStorageDevice Win32 Implementation.
*/

#include "USBMassStorageDeviceWin32.h"

#include <windows.h>
#include <winioctl.h>

//-----------------------------------------------------------------------------
// From Windows DDK , mountmgr.h
//-----------------------------------------------------------------------------
// retrieve the storage device descriptor data for a device. 
typedef struct _STORAGE_DEVICE_DESCRIPTOR {
  ULONG  Version;
  ULONG  Size;
  UCHAR  DeviceType;
  UCHAR  DeviceTypeModifier;
  BOOLEAN  RemovableMedia;
  BOOLEAN  CommandQueueing;
  ULONG  VendorIdOffset;
  ULONG  ProductIdOffset;
  ULONG  ProductRevisionOffset;
  ULONG  SerialNumberOffset;
  STORAGE_BUS_TYPE  BusType;
  ULONG  RawPropertiesLength;
  UCHAR  RawDeviceProperties[1];

} STORAGE_DEVICE_DESCRIPTOR, *PSTORAGE_DEVICE_DESCRIPTOR;

// retrieve the properties of a storage device or adapter. 
typedef enum _STORAGE_QUERY_TYPE {
  PropertyStandardQuery = 0,
  PropertyExistsQuery,
  PropertyMaskQuery,
  PropertyQueryMaxDefined

} STORAGE_QUERY_TYPE, *PSTORAGE_QUERY_TYPE;

// retrieve the properties of a storage device or adapter. 
typedef enum _STORAGE_PROPERTY_ID {
  StorageDeviceProperty = 0,
  StorageAdapterProperty,
  StorageDeviceIdProperty

} STORAGE_PROPERTY_ID, *PSTORAGE_PROPERTY_ID;

// retrieve the properties of a storage device or adapter. 
typedef struct _STORAGE_PROPERTY_QUERY {
  STORAGE_PROPERTY_ID  PropertyId;
  STORAGE_QUERY_TYPE  QueryType;
  UCHAR  AdditionalParameters[1];

} STORAGE_PROPERTY_QUERY, *PSTORAGE_PROPERTY_QUERY;

typedef struct _MOUNTMGR_DRIVE_LETTER_TARGET {
  USHORT  DeviceNameLength;
  WCHAR  DeviceName[1];
} MOUNTMGR_DRIVE_LETTER_TARGET, *PMOUNTMGR_DRIVE_LETTER_TARGET;

typedef struct _MOUNTMGR_DRIVE_LETTER_INFORMATION {
  BOOLEAN  DriveLetterWasAssigned;
  UCHAR  CurrentDriveLetter;
} MOUNTMGR_DRIVE_LETTER_INFORMATION, *PMOUNTMGR_DRIVE_LETTER_INFORMATION;

#define MOUNTMGRCONTROLTYPE                                     ((ULONG)'m')
#define MOUNTDEVCONTROLTYPE                                     ((ULONG)'M')

#define MOUNTMGR_DEVICE_NAME              L"\\Device\\MountPointManager"
#define MOUNTMGR_DOS_DEVICE_NAME          L"\\\\.\\MountPointManager"

#define IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 4, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)  

#define IOCTL_STORAGE_QUERY_PROPERTY \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)


//-----------------------------------------------------------------------------
CUSBMassStorageDeviceHelperWin32::CUSBMassStorageDeviceHelperWin32()
: m_USBBusses(nsnull)
, m_USBDescriptor(nsnull)
, m_USBDevice(nsnull)
{
} //ctor

//-----------------------------------------------------------------------------
CUSBMassStorageDeviceHelperWin32::~CUSBMassStorageDeviceHelperWin32()
{
} //dtor

//-----------------------------------------------------------------------------
PRBool CUSBMassStorageDeviceHelperWin32::Initialize(const nsAString &deviceName, const nsAString &deviceIdentifier)
{
  PRBool bCanHandleDevice = PR_FALSE;

  PRInt32 index = deviceName.Find(NS_LITERAL_STRING("USBSTOR"));
  if(index > -1)
  {
    usb_init();
    usb_find_busses();
    usb_find_devices();

    m_USBBusses = usb_get_busses();
    NS_ASSERTION(m_USBBusses, "CUSBMassStorageDeviceHelperWin32::Initialize - Warning! USB Busses not found! Machine lacks USB support?");

    if(!m_USBBusses)
      return bCanHandleDevice;

    m_USBDescriptor = GetDeviceByName(deviceName);
    if(!m_USBDescriptor)
    {
      m_USBBusses = nsnull;
      return bCanHandleDevice;
    }

    m_USBDevice = usb_open(m_USBDescriptor);
    if(!m_USBDevice) 
    {
      m_USBBusses = nsnull;
      m_USBDescriptor = nsnull;
      return bCanHandleDevice;
    }

    m_Initialized = PR_TRUE;
    m_DeviceName = deviceName;
    m_DeviceIdentifier = deviceIdentifier;

    GetDeviceInformation();

    bCanHandleDevice = PR_TRUE;
  }

  return bCanHandleDevice;
} //Initialize

//-----------------------------------------------------------------------------
PRBool CUSBMassStorageDeviceHelperWin32::Shutdown()
{ 
  if(m_Initialized)
  {
    m_Initialized = PR_FALSE;
    usb_close(m_USBDevice);

    m_USBBusses = nsnull;
    m_USBDescriptor = nsnull;
    m_USBDevice = nsnull;
  }

  return PR_TRUE;
} //Shutdown

//-----------------------------------------------------------------------------
PRBool CUSBMassStorageDeviceHelperWin32::IsInitialized() 
{ 
  return m_Initialized; 
} //IsInitialized

//-----------------------------------------------------------------------------
const nsAString & CUSBMassStorageDeviceHelperWin32::GetDeviceVendor()
{
  return m_DeviceVendorName;
} //GetDeviceVendor

//-----------------------------------------------------------------------------
const nsAString & CUSBMassStorageDeviceHelperWin32::GetDeviceModel()
{
  return m_DeviceModelName;
} //GetDeviceModel

//-----------------------------------------------------------------------------
const nsAString & CUSBMassStorageDeviceHelperWin32::GetDeviceSerialNumber()
{
  return m_DeviceSerialNumber;
} //GetDeviceSerialNumber

//-----------------------------------------------------------------------------
const nsAString & CUSBMassStorageDeviceHelperWin32::GetDeviceMountPoint()
{
  return m_DeviceMountPoint;
} //GetDeviceMountPoint

//-----------------------------------------------------------------------------
PRInt64 CUSBMassStorageDeviceHelperWin32::GetDeviceCapacity()
{
  return -1;
} //GetDeviceCapacity

//-----------------------------------------------------------------------------
void CUSBMassStorageDeviceHelperWin32::GetDeviceInformation()
{
  if(!m_Initialized) return;

  GetStringFromDescriptor(m_USBDescriptor->descriptor.iManufacturer, m_DeviceVendorName);
  GetStringFromDescriptor(m_USBDescriptor->descriptor.iProduct, m_DeviceModelName);
  GetStringFromDescriptor(m_USBDescriptor->descriptor.iSerialNumber, m_DeviceSerialNumber);

  return;
} //GetDeviceInformation

//-----------------------------------------------------------------------------
int CUSBMassStorageDeviceHelperWin32::GetStringFromDescriptor(int index, nsAString &deviceString, usb_dev_handle *handle /*= nsnull*/)
{
  char szBuf[1024] = {0};
  usb_dev_handle *dev = nsnull;

  if(handle) dev = handle;
  else dev = m_USBDevice;

  int len = usb_get_string_simple(dev, index, (char *) szBuf, sizeof(szBuf));

  nsAutoString strSerial;
  nsDependentCString cstrSerial(szBuf);
  
  CopyUTF8toUTF16(cstrSerial, strSerial);
  CompressWhitespace(strSerial);
  
  deviceString.Assign(strSerial);
  return len;
} //GetDeviceInformation

//-----------------------------------------------------------------------------
struct usb_device *CUSBMassStorageDeviceHelperWin32::GetDeviceByName(const nsAString &deviceName)
{
  struct usb_device *dev = nsnull;
  struct usb_bus *bus = nsnull;
  for(bus = m_USBBusses; bus; bus = bus->next)
  {
    struct usb_device *d = nsnull;
    for(d = bus->devices; d; d = d->next)
    {
      usb_dev_handle *dh = usb_open(d);
      if(!dh)
        continue;

      if(d->config->interface->altsetting->bInterfaceClass == USB_CLASS_MASS_STORAGE &&
         d->descriptor.iSerialNumber)
      {
        nsAutoString strSerial;
        GetStringFromDescriptor(d->descriptor.iSerialNumber, strSerial, dh);

        if(strSerial.Find(deviceName) > -1)
        {
          dev = d;
          usb_close(dh);
          break;
        }
       
        usb_close(dh);
      }
    }
  }

  return dev;
} //GetDeviceByIdentifier

//-----------------------------------------------------------------------------
PRBool CUSBMassStorageDeviceHelperWin32::UpdateMountPoint(const nsAString &deviceMountPoint)
{
  //Verify this mount point, make sure it is a USB drive!
  m_DeviceMountPoint = deviceMountPoint;
  return PR_TRUE;
}