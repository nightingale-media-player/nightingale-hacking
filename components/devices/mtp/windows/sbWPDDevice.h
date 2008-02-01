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

// The following is placed below the other includes to avoid win32 macro 
// madness
#ifndef WINVER              // Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0600       // Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT        // Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0600     // Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINDOWS      // Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0600 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE           // Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0400    // Change this to the appropriate value to target IE 5.0 or later.
#endif

#include <stdio.h>
#include <tchar.h>
#include <PortableDeviceApi.h>
#include <PortableDevice.h>
#include <atlbase.h>
#include <atlstr.h>
#include <nsStringAPI.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbBaseDevice.h>
#include <nsIPropertyBag2.h>
#include <nsIClassInfo.h>
#include <nsStringAPI.h>
#include <nsCOMPtr.h>

struct IPortableDevice;
struct IPortableDeviceValues;

class sbWPDDevice : public sbBaseDevice,
                     public nsIClassInfo
{
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICE
  NS_DECL_NSICLASSINFO

public:
  static nsString const DEVICE_ID_PROP;
  static nsString const DEVICE_FRIENDLY_NAME_PROP;
  static nsString const DEVICE_DESCRIPTION_PROP;
  static nsString const DEVICE_MANUFACTURER_PROP;

  /**
   * Initializse the device with the creating controller's ID
   * device properties and optionally the Portable Device object
   */
  sbWPDDevice(nsID const & controllerID,
              nsIPropertyBag2 * deviceProperties,
              IPortableDevice * device = 0);
  virtual ~sbWPDDevice();
  virtual nsresult ProcessRequest();
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
private:
  CComPtr<IPortableDevice> mPortableDevice;
  nsCOMPtr<nsIPropertyBag2> mDeviceProperties;
  nsString mPnPDeviceID;
  nsID mControllerID;
  PRUint32 mState;
  nsString GetDeviceID() const;
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
