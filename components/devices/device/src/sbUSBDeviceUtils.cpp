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

//------------------------------------------------------------------------------
//
// Songbird USB device utilities services.
//
//------------------------------------------------------------------------------

/**
 * \file  sbUSBDeviceUtils.cpp
 * \brief Songbird USBDevice Utilities Source.
 */

//------------------------------------------------------------------------------
//
// Songbird USB device utilities imported services.
//
//------------------------------------------------------------------------------

// Self import.
#include "sbUSBDeviceUtils.h"

// Mozilla imports.
#include <nsStringGlue.h>


//------------------------------------------------------------------------------
//
// Songbird USB device services.
//
//------------------------------------------------------------------------------

/**
 * Return true in aImplementsClass if the USB device with the descriptors
 * specified by aDescriptorList implements the USB class specified by aUSBClass.
 *
 * \param aDescriptorList       List of device USB descriptors.
 * \param aUSBClass             USB class to check.
 * \param aImplementsClass      Returned true if device implements USB class.
 */

nsresult
sbUSBDeviceImplementsClass(sbUSBDescriptorList& aDescriptorList,
                           PRUint8              aUSBClass,
                           bool*              aImplementsClass)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aImplementsClass);

  // Function variables.
  bool implementsClass = PR_FALSE;

  // Check all device and interface descriptors for the specified class.
  for (PRUint32 i = 0; i < aDescriptorList.Length(); i++) {
    // Get the next descriptor.
    sbUSBDescriptor*       descriptor = aDescriptorList[i];
    sbUSBDescriptorHeader* descriptorHeader = descriptor->GetHeader();

    // Get the class from device and interface descriptors.  Skip other
    // descriptors.
    PRUint8 usbClass;
    bool  hasClass = PR_TRUE;
    switch (descriptorHeader->bDescriptorType) {
      case SB_USB_DESCRIPTOR_TYPE_DEVICE :
        {
          sbUSBDeviceDescriptor* deviceDescriptor =
            reinterpret_cast<sbUSBDeviceDescriptor*>(descriptorHeader);
          usbClass = deviceDescriptor->bDeviceClass;
        }
        break;

      case SB_USB_DESCRIPTOR_TYPE_INTERFACE :
        {
          sbUSBInterfaceDescriptor* interfaceDescriptor =
            reinterpret_cast<sbUSBInterfaceDescriptor*>(descriptorHeader);
          usbClass = interfaceDescriptor->bInterfaceClass;
        }
        break;

      default :
        hasClass = PR_FALSE;
        break;
    }
    if (!hasClass)
      continue;

    // Check for a match.
    if (usbClass == aUSBClass)
    {
      implementsClass = PR_TRUE;
      break;
    }
  }

  // Return results.
  *aImplementsClass = implementsClass;

  return NS_OK;
}


/**
 * Return in aDescriptor the USB descriptor for the device specified by
 * aDeviceRef of descriptor type specified by aDescriptorType and descriptor
 * index specified by aDescriptorIndex and aDescriptorIndex2.
 *
 * \param aDeviceRef            Device reference for which to get descriptor.
 * \param aDescriptorType       Type of descriptor to get.
 * \param aDescriptorIndex      Index of descriptor to get.
 * \param aDescriptorIndex2     Second index of descriptor to get (only used for
 *                              string descriptors).
 * \param aDescriptor           Returned descriptor.
 */

nsresult
sbUSBDeviceGetDescriptor(sbUSBDeviceRef*   aDeviceRef,
                         PRUint8           aDescriptorType,
                         PRUint8           aDescriptorIndex,
                         PRUint16          aDescriptorIndex2,
                         sbUSBDescriptor** aDescriptor)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceRef);
  NS_ENSURE_ARG_POINTER(aDescriptor);

  // Function variables.
  PRUint16                descriptorLength = 0;
  nsresult                rv;

  // Get the descriptor length for fixed size descriptors.
  switch (aDescriptorType) {
    case SB_USB_DESCRIPTOR_TYPE_DEVICE :
      descriptorLength = sizeof(sbUSBDeviceDescriptor);
      break;

    case SB_USB_DESCRIPTOR_TYPE_CONFIGURATION :
      descriptorLength = sizeof(sbUSBConfigurationDescriptor);
      break;

    default :
      break;
  }

  // If length unknown, get the descriptor header to get the length.
  if (!descriptorLength) {
    // Get the descriptor header.
    nsRefPtr<sbUSBDescriptor> sbDescriptorHeader;
    rv = sbUSBDeviceGetDescriptor(aDeviceRef,
                                  aDescriptorType,
                                  aDescriptorIndex,
                                  aDescriptorIndex2,
                                  sizeof(sbUSBDescriptorHeader),
                                  getter_AddRefs(sbDescriptorHeader));
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the descriptor length.
    sbUSBDescriptorHeader* descriptorHeader = sbDescriptorHeader->GetHeader();
    descriptorLength = descriptorHeader->bLength;
  }

  // Read the descriptor.
  nsRefPtr<sbUSBDescriptor> descriptor;
  rv = sbUSBDeviceGetDescriptor(aDeviceRef,
                                aDescriptorType,
                                aDescriptorIndex,
                                aDescriptorIndex2,
                                descriptorLength,
                                getter_AddRefs(descriptor));
  NS_ENSURE_SUCCESS(rv, rv);

  // If getting a configuration descriptor, read all descriptors.
  if (aDescriptorType == SB_USB_DESCRIPTOR_TYPE_CONFIGURATION) {
    // Get the total length of configuration data.
    sbUSBConfigurationDescriptor* configurationDescriptor =
                                    descriptor->GetConfiguration();
    PRUint16 totalLength = configurationDescriptor->wTotalLength;

    // Read all configuration descriptors.
    rv = sbUSBDeviceGetDescriptor(aDeviceRef,
                                  aDescriptorType,
                                  aDescriptorIndex,
                                  aDescriptorIndex2,
                                  totalLength,
                                  getter_AddRefs(descriptor));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Return results.
  descriptor.forget(aDescriptor);

  return NS_OK;
}


/**
 * Return in aString the USB string descriptor string for the device specified
 * by aDeviceRef with the descriptor index specified by aDescriptorIndex and
 * aDescriptorIndex2.
 *
 * \param aDeviceRef            Device reference for which to get descriptor.
 * \param aDescriptorIndex      Index of descriptor to get.
 * \param aDescriptorIndex2     Second index of descriptor to get (only used for
 *                              string descriptors).
 * \param aString               Returned descriptor string.
 */

nsresult
sbUSBDeviceGetStringDescriptor(sbUSBDeviceRef* aDeviceRef,
                               PRUint8         aDescriptorIndex,
                               PRUint16        aDescriptorIndex2,
                               nsAString&      aString)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceRef);

  // Function variables.
  nsresult rv;

  // Get the USB string descriptor.
  nsRefPtr<sbUSBDescriptor> sbStringDescriptor;
  rv = sbUSBDeviceGetDescriptor(aDeviceRef,
                                SB_USB_DESCRIPTOR_TYPE_STRING,
                                aDescriptorIndex,
                                aDescriptorIndex2,
                                getter_AddRefs(sbStringDescriptor));
  NS_ENSURE_SUCCESS(rv, rv);
  sbUSBStringDescriptor* stringDescriptor = sbStringDescriptor->GetString();

  // Return results.
  PRUint32 length = (stringDescriptor->bLength -
                     offsetof(sbUSBStringDescriptor, bString)) /
                    sizeof(PRUnichar);
  aString.Assign(stringDescriptor->bString, length);

  return NS_OK;
}


/**
 * Return in aLanguageList the list of language ID's supported by the device
 * specified by aDeviceRef.
 *
 * \param aDeviceRef            Device reference for which to get language ID's.
 * \param aLanguageList         List of language ID's.
 */

nsresult
sbUSBDeviceGetLanguageList(sbUSBDeviceRef*     aDeviceRef,
                           nsTArray<PRUint16>& aLanguageList)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceRef);

  // Function variables.
  nsresult rv;

  // Get the string language descriptor.
  nsRefPtr<sbUSBDescriptor> sbLanguageDescriptor;
  rv = sbUSBDeviceGetDescriptor(aDeviceRef,
                                SB_USB_DESCRIPTOR_TYPE_STRING,
                                0,
                                0,
                                getter_AddRefs(sbLanguageDescriptor));
  NS_ENSURE_SUCCESS(rv, rv);
  sbUSBStringDescriptor* languageDescriptor = sbLanguageDescriptor->GetString();

  // Get the string language list.
  PRUint32 languageCount = (languageDescriptor->bLength -
                            offsetof(sbUSBStringDescriptor, bString)) /
                           sizeof(PRUnichar);
  aLanguageList.Clear();
  for (PRUint32 i = 0; i < languageCount; i++) {
    aLanguageList.AppendElement
                    (static_cast<PRUint16>(languageDescriptor->bString[i]));
  }

  return NS_OK;
}


/**
 * Add to aDescriptorList the list of configuration descriptors for the device
 * specified by aDeviceRef and configuration index specified by
 * aConfigurationIndex.
 *
 * \param aDeviceRef            Device reference for which to get descriptors.
 * \param aConfigurationIndex   Index of configuration to get.
 * \param aDescriptorList       List of descriptors.
 */

nsresult
sbUSBDeviceAddConfigurationDescriptors(sbUSBDeviceRef*      aDeviceRef,
                                       PRUint8              aConfigurationIndex,
                                       sbUSBDescriptorList& aDescriptorList)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceRef);

  // Function variables.
  nsresult rv;

  // Get the configuration descriptor.
  nsRefPtr<sbUSBDescriptor> sbConfigurationDescriptor;
  rv = sbUSBDeviceGetDescriptor(aDeviceRef,
                                SB_USB_DESCRIPTOR_TYPE_CONFIGURATION,
                                aConfigurationIndex,
                                0,
                                getter_AddRefs(sbConfigurationDescriptor));
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the configuration descriptors to the descriptor list.
  sbUSBDescriptorHeader* descriptorHeader =
                           sbConfigurationDescriptor->GetHeader();
  sbUSBConfigurationDescriptor* configurationDescriptor =
                                  sbConfigurationDescriptor->GetConfiguration();
  PRUint16 bytesRemaining = configurationDescriptor->wTotalLength;
  while ((bytesRemaining > sizeof(sbUSBDescriptorHeader)) &&
         (bytesRemaining > descriptorHeader->bLength)) {
    // Create a Songbird USB descriptor.
    nsRefPtr<sbUSBDescriptor> descriptor =
                                new sbUSBDescriptor(descriptorHeader);
    NS_ENSURE_TRUE(descriptor, NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_TRUE(descriptor->Get(), NS_ERROR_OUT_OF_MEMORY);

    // Add the descriptor to the descriptor list.
    NS_ENSURE_TRUE(aDescriptorList.AppendElement(descriptor),
                   NS_ERROR_OUT_OF_MEMORY);

    // Add the next configuration descriptor.
    bytesRemaining -= descriptorHeader->bLength;
    descriptorHeader = reinterpret_cast<sbUSBDescriptorHeader*>
                         (((reinterpret_cast<PRUint8*>(descriptorHeader)) +
                           descriptorHeader->bLength));
  }

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Songbird USB device system services.
//
//   These services must be provided by a system dependent sources.
//
//------------------------------------------------------------------------------

#if 0 // Must be implemented by system dependent sources.

/**
 * Return in aDescriptor the USB descriptor for the device specified by
 * aDeviceRef of descriptor type specified by aDescriptorType and descriptor
 * index specified by aDescriptorIndex and aDescriptorIndex2.  Read up to
 * aDescriptorLength bytes of descriptor data.
 *
 * \param aDeviceRef            Device reference for which to get descriptor.
 * \param aDescriptorType       Type of descriptor to get.
 * \param aDescriptorIndex      Index of descriptor to get.
 * \param aDescriptorIndex2     Second index of descriptor to get (only used for
 *                              string descriptors).
 * \param aDescriptorLength     Number of bytes of descriptor data to get.
 * \param aDescriptor           Returned descriptor.
 */

nsresult
sbUSBDeviceGetDescriptor(sbUSBDeviceRef*   aDeviceRef,
                         PRUint8           aDescriptorType,
                         PRUint8           aDescriptorIndex,
                         PRUint16          aDescriptorIndex2,
                         PRUint16          aDescriptorLength,
                         sbUSBDescriptor** aDescriptor)
{
}


#endif


//------------------------------------------------------------------------------
//
// Songbird USB descriptor class.
//
//------------------------------------------------------------------------------

NS_IMPL_ISUPPORTS0(sbUSBDescriptor)

