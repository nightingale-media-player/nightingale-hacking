/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#ifndef __SB_USB_DEVICE_UTILS_H__
#define __SB_USB_DEVICE_UTILS_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird USB device utilities defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbUSBDeviceUtils.h
 * \brief Songbird USB Device Utilities Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird USB device utilities imported services.
//
//------------------------------------------------------------------------------

// Mozilla imports.
#include <nsAutoPtr.h>
#include <nsTArray.h>


//------------------------------------------------------------------------------
//
// Songbird USB device utilities typedefs.
//
//------------------------------------------------------------------------------

// List of USB descriptors.
class sbUSBDescriptor;
typedef nsTArray<nsRefPtr<sbUSBDescriptor> > sbUSBDescriptorList;


//------------------------------------------------------------------------------
//
// Songbird USB standard defs and structures.
//
//------------------------------------------------------------------------------

//
// USB descriptor type defs.
//

#define SB_USB_DESCRIPTOR_TYPE_DEVICE        1
#define SB_USB_DESCRIPTOR_TYPE_CONFIGURATION 2
#define SB_USB_DESCRIPTOR_TYPE_STRING        3
#define SB_USB_DESCRIPTOR_TYPE_INTERFACE     4
#define SB_USB_DESCRIPTOR_TYPE_ENDPOINT      5

//
// Apple's USB vendor ID is 05ac according to: http://www.linux-usb.org/usb.ids
//
#define SB_USB_APPLE_VENDOR_ID 0x05ac

/**
 * This structure contains the common header of all USB descriptors.
 *
 *   bLength                    Size of this descriptor in bytes.
 *   bDescriptorType            DEVICE Descriptor Type.
 */

typedef struct
{
  PRUint8                       bLength;
  PRUint8                       bDescriptorType;
} sbUSBDescriptorHeader;


/**
 * This structure contains the fields for a USB device descriptor.
 *
 *   bLength                    Size of this descriptor in bytes.
 *   bDescriptorType            DEVICE Descriptor Type.
 *   bcdUSB                     USB Specification Release Number in
 *                              Binary-Coded Decimal.
 *   bDeviceClass               Class code (assigned by the USB-IF).
 *   bDeviceSubClass            Subclass code (assigned by the USB-IF).
 *   bDeviceProtocol            Protocol code (assigned by the USB-IF).
 *   bMaxPacketSize0            Maximum packet size for endpoint zero.
 *   idVendor                   Vendor ID (assigned by the USB-IF).
 *   idProduct                  Product ID (assigned by the manufacturer).
 *   bcdDevice                  Device release number in binary-coded decimal.
 *   iManufacturer              Index of string descriptor describing
 *                              manufacturer.
 *   iProduct                   Index of string descriptor describing product.
 *   iSerialNumber              Index of string descriptor describing the
 *                              device's serial number.
 *   bNumConfigurations         Number of possible configurations.
 */

typedef struct
{
  PRUint8                       bLength;
  PRUint8                       bDescriptorType;
  PRUint16                      bcdUSB;
  PRUint8                       bDeviceClass;
  PRUint8                       bDeviceSubClass;
  PRUint8                       bDeviceProtocol;
  PRUint8                       bMaxPacketSize0;
  PRUint16                      idVendor;
  PRUint16                      idProduct;
  PRUint16                      bcdDevice;
  PRUint8                       iManufacturer;
  PRUint8                       iProduct;
  PRUint8                       iSerialNumber;
  PRUint8                       bNumConfigurations;
} sbUSBDeviceDescriptor;


/**
 * This structure contains the fields for a USB configuration descriptor.
 *
 *   bLength                    Size of this descriptor in bytes.
 *   bDescriptorType            DEVICE Descriptor Type.
 *   wTotalLength               Total length of data returned for this
 *                              configuration.
 *   bNumInterfaces             Number of interfaces supported by this
 *                              configuration.
 *   bConfigurationValue        Value to use as an argument to the
 *                              SetConfiguration() request to select this
 *                              configuration.
 *   iConfiguration             Index of string descriptor describing this
 *                              configuration.
 *   bmAttributes               Configuration characteristics.
 *   bMaxPower                  Maximum power consumption of the device from the
 *                              bus in this specific configuration when the
 *                              device is fully operational.
 */

typedef struct
{
  PRUint8                       bLength;
  PRUint8                       bDescriptorType;
  PRUint16                      wTotalLength;
  PRUint8                       bNumInterfaces;
  PRUint8                       bConfigurationValue;
  PRUint8                       iConfiguration;
  PRUint8                       bmAttributes;
  PRUint8                       bMaxPower;
} sbUSBConfigurationDescriptor;


/**
 * This structure contains the fields for a USB interface descriptor.
 *
 *   bLength                    Size of this descriptor in bytes.
 *   bDescriptorType            INTERFACE Descriptor Type.
 *   bInterfaceNumber           Value used to select this alternate setting for
 *                              the interface identified in the prior field.
 *   bAlternateSetting          Value used to select this alternate setting for
 *                              the interface identified in the prior field.
 *   bNumEndpoints              Number of endpoints used by this interface
 *                              (excluding the Default Control Pipe).
 *   bInterfaceClass            Class code (assigned by the USB-IF).
 *   bInterfaceSubClass         Subclass code (assigned by the USB-IF).
 *   bInterfaceProtocol         Protocol code (assigned by the USB).
 *   iInterface                 Index of string descriptor describing this
 *                              interface.
 */

typedef struct
{
  PRUint8                       bLength;
  PRUint8                       bDescriptorType;
  PRUint8                       bInterfaceNumber;
  PRUint8                       bAlternateSetting;
  PRUint8                       bNumEndpoints;
  PRUint8                       bInterfaceClass;
  PRUint8                       bInterfaceSubClass;
  PRUint8                       bInterfaceProtocol;
  PRUint8                       iInterface;
} sbUSBInterfaceDescriptor;


/**
 * This structure contains the fields for a USB string descriptor.
 *
 *   bLength                    Size of this descriptor in bytes.
 *   bDescriptorType            STRING Descriptor Type.
 *   bString                    UNICODE encoded string.
 */

typedef struct
{
  PRUint8                       bLength;
  PRUint8                       bDescriptorType;
  PRUnichar                     bString[1];
} sbUSBStringDescriptor;


//------------------------------------------------------------------------------
//
// Songbird USB device utilities classes and structures.
//
//------------------------------------------------------------------------------

//
// sbUSBDeviceRef               Reference to a USB device.  This class should be
//                              sub-classed by the USB device system services
//                              to contain whatever data is required to
//                              reference a USB device.
//

class sbUSBDeviceRef {};


/**
 * This class wraps a USB descriptor record in a class that may be used with
 * nsRefPtr and inserted into an nsTArray.
 */

class sbUSBDescriptor : public nsISupports
{
public:
  //
  // Inherited interfaces.
  //

  NS_DECL_ISUPPORTS


  //
  // Constructors/destructors.
  //

  sbUSBDescriptor(void*    aDescriptor,
                  PRUint16 aDescriptorLength) {
    mDescriptor = NS_Alloc(aDescriptorLength);
    if (mDescriptor) {
      mTotalLength = aDescriptorLength;
      memcpy(mDescriptor, aDescriptor, mTotalLength);
    } else {
      mTotalLength = 0;
    }
  }

  sbUSBDescriptor(sbUSBDescriptorHeader* aDescriptor)
  {
    if (aDescriptor && (aDescriptor->bLength > 0))
      mDescriptor = NS_Alloc(aDescriptor->bLength);
    else
      mDescriptor = nsnull;
    if (mDescriptor) {
      mTotalLength = aDescriptor->bLength;
      memcpy(mDescriptor, aDescriptor, mTotalLength);
    } else {
      mTotalLength = 0;
    }
  }

  virtual ~sbUSBDescriptor()
  {
    if (mDescriptor)
      NS_Free(mDescriptor);
  }


  /**
   * Getters.
   */

  void* Get()
  {
    return mDescriptor;
  }

  sbUSBDescriptorHeader* GetHeader()
  {
    return static_cast<sbUSBDescriptorHeader*>(mDescriptor);
  }

  sbUSBConfigurationDescriptor* GetConfiguration()
  {
    return static_cast<sbUSBConfigurationDescriptor*>(mDescriptor);
  }

  sbUSBDeviceDescriptor* GetDevice()
  {
    return static_cast<sbUSBDeviceDescriptor*>(mDescriptor);
  }

  sbUSBStringDescriptor* GetString()
  {
    return static_cast<sbUSBStringDescriptor*>(mDescriptor);
  }

  PRUint16 GetTotalLength()
  {
    return mTotalLength;
  }


private:
  //
  // mDescriptor                USB descriptor.
  // mTotalLength               Total length of descriptor data.
  //

  void*                         mDescriptor;
  PRUint16                      mTotalLength;
};


//------------------------------------------------------------------------------
//
// Songbird USB device services.
//
//------------------------------------------------------------------------------

//
// Songbird USB device services.
//

nsresult sbUSBDeviceImplementsClass(sbUSBDescriptorList& aDescriptorList,
                                    PRUint8              aUSBClass,
                                    PRBool*              aImplementsClass);

nsresult sbUSBDeviceGetDescriptor(sbUSBDeviceRef*   aDeviceRef,
                                  PRUint8           aDescriptorType,
                                  PRUint8           aDescriptorIndex,
                                  PRUint16          aDescriptorIndex2,
                                  sbUSBDescriptor** aDescriptor);

nsresult sbUSBDeviceGetStringDescriptor(sbUSBDeviceRef* aDeviceRef,
                                        PRUint8         aDescriptorIndex,
                                        PRUint16        aDescriptorIndex2,
                                        nsAString&      aString);

nsresult sbUSBDeviceGetLanguageList(sbUSBDeviceRef*     aDeviceRef,
                                    nsTArray<PRUint16>& aLanguageList);

nsresult sbUSBDeviceAddConfigurationDescriptors
           (sbUSBDeviceRef*      aDeviceRef,
            PRUint8              aConfigurationIndex,
            sbUSBDescriptorList& aDescriptorList);


//
// Songbird USB device system services.
//
//   These services must be provided by a system dependent sources.
//

nsresult sbUSBDeviceGetDescriptor(sbUSBDeviceRef*   aDeviceRef,
                                  PRUint8           aDescriptorType,
                                  PRUint8           aDescriptorIndex,
                                  PRUint16          aDescriptorIndex2,
                                  PRUint16          aDescriptorLength,
                                  sbUSBDescriptor** aDescriptor);


#endif // __SB_USB_DEVICE_UTILS_H__

