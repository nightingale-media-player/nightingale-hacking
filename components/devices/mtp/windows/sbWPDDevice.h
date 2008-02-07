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
#ifndef SBWPDDEVICE_H_
#define SBWPDDEVICE_H_

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <PortableDeviceApi.h>
#include <PortableDevice.h>
#include <nsStringAPI.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbBaseDevice.h>
#include <nsIPropertyBag2.h>
#include <nsIClassInfo.h>
#include <nsStringAPI.h>
#include <nsCOMPtr.h>
#include <nsIThread.h>

struct IPortableDevice;
struct IPortableDeviceValues;
class sbIDeviceEvent;
class sbWPDDeviceThread;
class sbDeviceStatus;

/**
 * This class represents a WPD device and is used to communicate with the WPD
 * device
 */
class sbWPDDevice : public sbBaseDevice,
                     public nsIClassInfo
{
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICE
  NS_DECL_NSICLASSINFO

public:
  /**
   * TODO: These may be deprecated or otherwise replaced with constants elsewhere
   */
  static nsString const DEVICE_ID_PROP;
  static nsString const DEVICE_FRIENDLY_NAME_PROP;
  static nsString const DEVICE_DESCRIPTION_PROP;
  static nsString const DEVICE_MANUFACTURER_PROP;
  /**
   * The key to the property on the Songbird media item that holds
   * the persistent unique ID to the corresponding object on the 
   * device
   */
  static nsString const PUID_SBIMEDIAITEM_PROPERTY;
  
  /**
   * Initializse the device with the creating controller's ID
   * device properties and optionally the Portable Device object
   * The optional parameter is if there's an existing instance this
   * will eliminate the need to recreate the device and open it. If
   * the caller passes a device it MUST be open.
   */
  sbWPDDevice(nsID const & controllerID,
              nsIPropertyBag2 * deviceProperties,
              IPortableDevice * device = 0);
  virtual ~sbWPDDevice();
  virtual nsresult ProcessRequest();
  IPortableDevice* PortableDevice()
  {
    return mPortableDevice;
  }
  nsIPropertyBag2* DeviceProperties()
  {
    return mDeviceProperties;
  }
  /**
   * Creates the WPD client info needed to open the device
   */
  static nsresult GetClientInfo(IPortableDeviceValues ** clientInfo);
  /**
   * Retrieves the properties object from the device
   */
  static nsresult GetDeviceProperties(IPortableDevice * device,
                                      IPortableDeviceProperties ** propValues);
  /**
   * Sets a property on the device
   */
  static nsresult SetProperty(IPortableDeviceProperties * properties,
                              nsAString const & key,
                              nsIVariant * value);
  /**
   * Returns the property on the device
   */
  static nsresult GetProperty(IPortableDeviceProperties * properties,
                              PROPERTYKEY const & key,
                              nsIVariant ** value);
  /**
   * Returns the property on the device, uses a string as the key, this is a standard
   * uuid format wrapped by braces followed by a space 
   * example: {F29F85E0-4FF9-1068-AB91-08002B27B3D9} 2
   */
  static nsresult GetProperty(IPortableDeviceProperties * properties,
                              nsAString const & key,
                              nsIVariant ** value);
  /**
   * Creates an event for the device
   */
  nsresult CreateAndDispatchEvent(PRUint32 aType,
                                  nsIVariant *aData);
  /**
   * This returns the MTP persistent unique ID for the corresponding object on
   * the device
   */
  nsString GetWPDDeviceIDFromMediaItem(sbIMediaItem * mediaItem);

  /**
   * Returns the PnP device ID for the device
   */
  static nsString GetDeviceID(IPortableDevice * device);
  /**
   * This is called by the worker thread (sbWPDDeviceThread) to do work. It 
   * returns PR_FALSE if there is no more work
   */
  PRBool ProcessThreadsRequest();
private:
  nsRefPtr<IPortableDevice> mPortableDevice;
  nsCOMPtr<nsIPropertyBag2> mDeviceProperties;
  nsString mPnPDeviceID;
  nsID mControllerID;
  PRUint32 mState;
  sbWPDDeviceThread * mDeviceThread;
  nsCOMPtr<nsIThread> mThreadObject;
  
  HANDLE mRequestsPendingEvent;
 
  nsresult GetPropertiesFromItem(IPortableDeviceContent * content,
                                 sbIMediaItem * item,
                                 sbIMediaList * list,
                                 IPortableDeviceValues ** itemProperties);
  nsresult CreateDeviceObjectFromMediaItem(sbDeviceStatus & status,
                                           sbIMediaItem * item,
                                           sbIMediaList * list);
  /**
   * Process the write request
   */
  nsresult WriteRequest(TransferRequest * request);
  /**
   * Process the read request
   */
  nsresult ReadRequest(TransferRequest * request);
  // Prevent copying and assignment
  sbWPDDevice(sbWPDDevice const &) {}
  sbWPDDevice & operator= (sbWPDDevice const &) { return *this; }
};

#define SB_WPDDEVICE_CID \
{0x22ab529d, 0xd2b2, 0x442d, {0xad, 0x30, 0x42, 0x81, 0x0a, 0xde, 0x49, 0x04}}

#define SB_WPDDEVICE_CONTRACTID \
  "@songbirdnest.com/Songbird/WPDDevice;1"

#define SB_WPDDEVICE_CLASSNAME \
  "WindowsPortableDeviceDevice"

#define SB_WPDDEVICE_DESCRIPTION \
  "Windows Portable Device Device"

#endif /*SBWPDDEVICE_H_*/
