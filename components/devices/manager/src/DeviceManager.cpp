/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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
* \file  DeviceManager.cpp
* \Windows Media device Component Implementation.
*/
#include "DeviceManager.h"
#include "nspr.h"
#include "objbase.h"

#include <xpcom/nscore.h>
#include <xpcom/nsXPCOM.h>
#include <xpcom/nsCOMPtr.h>
#include <xpcom/nsComponentManagerUtils.h>
#include <necko/nsIURI.h>
#include <webbrowserpersist/nsIWebBrowserPersist.h>
#include <xpcom/nsILocalFile.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsIComponentRegistrar.h>
#include <xpcom/nsSupportsPrimitives.h>
#include <xpconnect/xpcComponents.h>
#include <string/nsStringAPI.h>
#include "nsIWebProgressListener.h"
// #include "../sbIDownloadDevice/sbIDownloadDevice.h" // no.

#include "IDatabaseResult.h"
#include "IDatabaseQuery.h"
#include "DeviceManager.h"

#define NAME_WINDOWS_MEDIA_DEVICE NS_LITERAL_STRING("Songbird Device Manager").get()

const NUM_DEVICES_SUPPORTED = 1;

sbDeviceManager* sbDeviceManager::mSingleton = NULL;

NS_IMPL_ISUPPORTS1(sbDeviceManager, sbIDeviceManager)

sbDeviceManager* sbDeviceManager::GetSingleton()
{
  NS_IF_ADDREF( mSingleton? mSingleton : mSingleton = new sbDeviceManager() );
  return mSingleton;  
}

sbDeviceManager::sbDeviceManager():
  mIntialized(false),
  mFinalized(false)
{
  InializeInternal();
}

sbDeviceManager::~sbDeviceManager()
{
  FinalizeInternal();
}

void sbDeviceManager::InializeInternal()
{
  if (mIntialized)
    return; // Already initialized

  // Instantiate all supported devices
  // This is done by iterating through all registered
  // XPCOM components and finding the components with
  // @songbird.org/Songbird/Device/ prefix for the 
  // contract ID for the interface.

  nsresult rv;
  nsCOMPtr<nsIComponentRegistrar> registrar;
  rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
  if (rv != NS_OK)
    return;

  nsCOMPtr<nsISimpleEnumerator> simpleEnumerator;
  rv = registrar->EnumerateContractIDs(getter_AddRefs(simpleEnumerator));
  if (rv != NS_OK)
    return;
  
  PRBool moreAvailable;

  while(simpleEnumerator->HasMoreElements(&moreAvailable) == NS_OK &&
    moreAvailable)
  {
    nsCOMPtr<nsISupportsCString> contractString;
    if (simpleEnumerator->GetNext(getter_AddRefs(contractString)) == NS_OK)
    {
      char* contractID = NULL;
      contractString->ToString(&contractID);
      if (strstr(contractID, "@songbird.org/Songbird/Device/"))
      {
        nsCOMPtr<sbIDeviceBase> device(do_CreateInstance(contractID));
        if (device.get())
        {
          ((sbIDeviceBase *) device.get())->AddRef();
          PRBool retVal = false;
          device->Initialize(&retVal);
          mSupportedDevices.push_front(device);
        }
      }
      PR_Free(contractID);
    }
  }

  mIntialized = true;
  mFinalized = false;

}
void sbDeviceManager::FinalizeInternal()
{
  if (mFinalized)
    return; // Already finalized

  std::list<_Device>::iterator deviceIterator = mSupportedDevices.begin();
  while (deviceIterator != mSupportedDevices.end())
  {
    PRBool retVal = false;
    (*deviceIterator)->Finalize(&retVal);

    deviceIterator ++;
  }

  mIntialized = false;
  mFinalized = true;
}

/* PRUint32 GetNumDeviceCategories (); */
NS_IMETHODIMP sbDeviceManager::GetNumDeviceCategories(PRUint32 *_retval)
{
	*_retval = (PRUint32) mSupportedDevices.size();
    return NS_OK;
}

/* void Initialize (); */
NS_IMETHODIMP sbDeviceManager::Initialize()
{
  InializeInternal();
  return NS_OK;
}

/* void Finalize (); */
NS_IMETHODIMP sbDeviceManager::Finalize()
{
  FinalizeInternal();
  return NS_OK;
}

/* wstring EnumDeviceCategoryString (in PRUint32 Index); */
NS_IMETHODIMP sbDeviceManager::EnumDeviceCategoryString(PRUint32 Index, PRUnichar **_retval)
{
	std::list<_Device>::iterator deviceIterator = mSupportedDevices.begin();
	while (deviceIterator != mSupportedDevices.end())
	{
		if (Index -- == 0)
		{
			(*deviceIterator)->GetDeviceCategory(_retval);
			break;
		}
		deviceIterator ++;
	}
	return NS_OK;
}

/* sbIDeviceBase GetDevice (in wstring DeviceCategory); */
NS_IMETHODIMP sbDeviceManager::GetDevice(const PRUnichar *DeviceCategory, sbIDeviceBase **_retval)
{
	*_retval = GetDeviceMatchingCategory(DeviceCategory);
  
  if (*_retval)
    (*_retval)->AddRef();

    return NS_OK;
}

sbIDeviceBase* sbDeviceManager::GetDeviceMatchingCategory(const PRUnichar *DeviceCategory)
{
	// Find the device with matching DeviceCategory
	std::list<_Device>::iterator deviceIterator = mSupportedDevices.begin();
	while (deviceIterator != mSupportedDevices.end())
	{
		PRUnichar *category = NULL;
		(*deviceIterator)->GetDeviceCategory(&category);
		if (category)
		{
			if (nsString(category) == nsString(DeviceCategory))
			{
				return (*deviceIterator).get();
			}
		}

		deviceIterator ++;
	}

	return NULL;
}

