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
#ifndef SBWPDMARSHALL_H_
#define SBWPDMARSHALL_H_

#include <sbBaseDeviceMarshall.h>
#include <nsIClassInfo.h>
#include <nsInterfaceHashtable.h>
#include <nsStringAPI.h>
#include <sbIDevice.h>
#include <nsAutoLock.h>

class nsIPropertyBag;
struct IWMDeviceManager;

/**
 * This class provides access to the Windows Media Device Manager notification
 * of when a device is arrives or is removed. This class communicates this to
 * sbIDevicManager service
 */
class sbWPDMarshall : public sbBaseDeviceMarshall,
                       public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEMARSHALL
  NS_DECL_NSICLASSINFO

  /**
   * Initializes the marshall object and begins monitoring for device events
   */
  sbWPDMarshall();
  /**
   * Cleans up our lock resource
   */
  ~sbWPDMarshall();
  /**
   * Adds a device to our list of known devices
   */
  void AddDevice(nsAString const & name, 
                 sbIDevice * device)
  {
    nsAutoLock lock(mKnownDevicesLock);
    mKnownDevices.Put(name, device);
  }
  /**
   * Returns a device given the device's ID
   */
  PRBool GetDevice(nsAString const & name,
                 sbIDevice ** device)
  {
    nsCOMPtr<nsISupports> ptr;
    nsAutoLock lock(mKnownDevicesLock);
    if (NS_SUCCEEDED(mKnownDevices.Get(name, getter_AddRefs(ptr))) && ptr) {
      nsCOMPtr<sbIDevice> devicePtr = do_QueryInterface(ptr);
      devicePtr.forget(device);
      return !device;
    }
    return PR_FALSE;
  }
  /**
   * Removes the device from the known device list
   */
  void RemoveDevice(nsAString const & name)
  {
    nsAutoLock lock(mKnownDevicesLock);
    mKnownDevices.Remove(name);
  }
  nsresult DiscoverDevices();
private:
  nsInterfaceHashtableMT<nsStringHashKey, nsISupports> mKnownDevices;
  PRLock* mKnownDevicesLock;
  nsresult BeginMonitoring();

  // Prevent copying and assignment
  sbWPDMarshall(sbWPDMarshall const &) : sbBaseDeviceMarshall(nsCString()) {}
  sbWPDMarshall & operator= (sbWPDMarshall const &) { return *this; }
};

#define SB_WPDMARSHALL_CID \
{0x40505c3c, 0x4ce9, 0x46b8, {0xb4, 0x81, 0x03, 0x33, 0xe0, 0x98, 0x4b, 0xc3}}

#define SB_WPDMARSHALL_CONTRACTID \
  "@songbirdnest.com/Songbird/WPDMarshall;1"

#define SB_WPDMARSHALL_CLASSNAME \
  "WindowsPortableDeviceMarshall"

#define SB_WPDMARSHALL_DESCRIPTION \
  "Windows Portable Device Marshall"

#endif