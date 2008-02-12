/* vim: set sw=2 :miv */
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


#ifndef SBWPDPROPERTYADAPTER_H_
#define SBWPDPROPERTYADAPTER_H_

#include <sbIDeviceProperties.h>
#include <nsIWritablePropertyBag.h>
#include <nsiWritablePropertyBag2.h>
#include <nsIClassInfo.h>
#include <nsAutoPtr.h>
#include <PortableDeviceApi.h>
#include <nsStringAPI.h>

struct IPortableDevice;
class sbWPDDevice;
class nsAString;
class nsIWritableVariant;

/**
 * This class provides the WPD properties through the sbIDeviceProperties interface
 * Setting the properties through this interface or the writable property bag
 * interfaces immediately updates the physical device's properties.
 * NOTE: The GetProperties implementation just returns "this" QI'd to a property
 * bag.
 */
class sbWPDPropertyAdapter : public sbIDeviceProperties,
                             public nsIWritablePropertyBag,
                             public nsIWritablePropertyBag2
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEPROPERTIES
  NS_DECL_NSIPROPERTYBAG
  NS_DECL_NSIPROPERTYBAG2
  NS_DECL_NSIWRITABLEPROPERTYBAG
  NS_DECL_NSIWRITABLEPROPERTYBAG2
  
	/**
	 * ALWAY use this method to create an instance of the sbWPDPropertyAdapter. Else you'll
	 * encure the wrath of MSVCRT allocation gods.
	 */
	static sbWPDPropertyAdapter * New(sbWPDDevice * device);
private:
  nsRefPtr<IPortableDeviceProperties> mDeviceProperties;
  nsCOMPtr<nsIWritableVariant> mWorkerVariant;
  nsString mDeviceID;

  /**
   * Creates a worker variant to eliminate excess object creation. Initializes
   * the devlice properties pointer as well as the ID of the device.
   */
  sbWPDPropertyAdapter(sbWPDDevice * device);
  /**
   * Only defined to centralize the code to clean up the members
   */
  virtual ~sbWPDPropertyAdapter();
  /**
   * Returns a variant for the property key
   */
  nsresult GetPropertyString(PROPERTYKEY const & key,
                             nsIVariant ** var);
  /**
   * Returns a string given the property key
   */
  nsresult GetPropertyString(PROPERTYKEY const & key,
                             nsAString & value);
  /**
   * Set the property value as a string given a property key
   */
  nsresult SetPropertyString(PROPERTYKEY const & key,
                             nsAString const & value);
};

#endif /*SBWPDPROPERTYADAPTER_H_*/
