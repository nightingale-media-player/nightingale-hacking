/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
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
// END SONGBIRD GPL
//
 */

/** 
* \file  DeviceManager.cpp
* \Windows Media device Component Implementation.
*/
#include "DeviceManager.h"
#include "DeviceBase.h"

#include <nsAutoLock.h>
#include <nsCRT.h>
#include <nsComponentManagerUtils.h>
#include <nsIComponentRegistrar.h>
#include <nsIObserverService.h>
#include <nsISimpleEnumerator.h>
#include <nsServiceManagerUtils.h>
#include <nsSupportsPrimitives.h>

#define NS_OBSERVERSERVICE_CONTRACTID "@mozilla.org/observer-service;1"
static const char kQuitApplicationTopic[] = "quit-application";
static const char kXPCOMShutdownTopic[] = "xpcom-shutdown";

#define SB_DEVICE_PREFIX "@songbird.org/Songbird/Device/"

// This allows us to be initialized once and only once.
PRBool sbDeviceManager::sServiceInitialized = PR_FALSE;

// This is a sanity check to make sure that we're finalizing properly
PRBool sbDeviceManager::sServiceFinalized = PR_FALSE;

NS_IMPL_THREADSAFE_ISUPPORTS2(sbDeviceManager, sbIDeviceManager, nsIObserver)

sbDeviceManager::sbDeviceManager()
: mLock(nsnull),
  mLastRequestedIndex(nsnull)
{
  // All initialization handled in Initialize()
}

sbDeviceManager::~sbDeviceManager()
{
  NS_ASSERTION(sbDeviceManager::sServiceFinalized,
               "DeviceManager never finalized!");
  if (mLock)
    PR_DestroyLock(mLock);
}

// Instantiate all supported devices.
// This is done by iterating through all registered XPCOM components and
// finding the components with @songbird.org/Songbird/Device/ prefix for the 
// contract ID for the interface.
NS_IMETHODIMP
sbDeviceManager::Initialize()
{
  // Test to make sure that we haven't been initialized yet. If consumers are
  // doing the right thing (using getService) then we should never get here
  // more than once. If they do the wrong thing (createInstance) then we'll
  // fail on them so that they fix their code.
  
  // XXXBen If someone calls createInstance *before* getService then this
  //        component will be dead once the first instance is destroyed. We
  //        need to make this a startup observer.

  NS_ENSURE_FALSE(sbDeviceManager::sServiceInitialized,
                  NS_ERROR_ALREADY_INITIALIZED);

  // Set the static variable so that we won't initialize again.
  sbDeviceManager::sServiceInitialized = PR_TRUE;

  mLock = PR_NewLock();
  NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);

  nsAutoLock autoLock(mLock);

  // Get the component registrar
  nsresult rv;
  nsCOMPtr<nsIComponentRegistrar> registrar;
  rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISimpleEnumerator> simpleEnumerator;
  rv = registrar->EnumerateContractIDs(getter_AddRefs(simpleEnumerator));
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Enumerate through the contractIDs and look for our prefix
  nsCOMPtr<nsISupports> element;
  PRBool more = PR_FALSE;
  while(NS_SUCCEEDED(simpleEnumerator->HasMoreElements(&more)) && more) {

    rv = simpleEnumerator->GetNext(getter_AddRefs(element));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupportsCString> contractString =
      do_QueryInterface(element, &rv);
    if NS_FAILED(rv) {
      NS_WARNING("QueryInterface failed");
      continue;
    }

    nsCAutoString contractID;
    rv = contractString->GetData(contractID);
    if NS_FAILED(rv) {
      NS_WARNING("GetData failed");
      continue;
    }

    NS_NAMED_LITERAL_CSTRING(prefix, SB_DEVICE_PREFIX);

    if (!StringBeginsWith(contractID, prefix))
      continue;

    // Create an instance of the device
    nsCOMPtr<sbIDeviceBase> device =
      do_CreateInstance(contractID.get(), &rv);
    if (!device) {
      NS_WARNING("Failed to create device!");
      continue;
    }

    // And initialize the device
    PRBool deviceInitialized;
    rv = device->Initialize(&deviceInitialized);
    if (!deviceInitialized) {
      NS_WARNING("Device failed to initialize!");
      continue;
    }

    // If everything has succeeded then we can add it to our array
    PRBool ok = mSupportedDevices.AppendObject(device);
    
    // Make sure that our array is behaving properly. If not we're in trouble.
    NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);
  }

  mLastRequestedCategory = EmptyString();

  // Now that everything has succeeded we'll register ourselves to observe
  // XPCOM shutdown so that we can finalize our devices properly.

  nsCOMPtr<nsIObserverService> observerService = 
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, kXPCOMShutdownTopic, PR_FALSE);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to add XPCOMShutdown observer");

  rv = observerService->AddObserver(this, kQuitApplicationTopic, PR_FALSE);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to add QuitApplication observer");

  // XXX Don't add any calls here that could possibly fail! We've already added
  //     ourselves to the observer service so if we fail now the observer
  //     service will hold an invalid pointer and later cause a crash at
  //     shutdown. Add any dangerous calls above *before* the call to
  //     AddObserver.

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceManager::Finalize()
{
  // Make sure we aren't called more than once
  NS_ENSURE_FALSE(sbDeviceManager::sServiceFinalized,
                  NS_ERROR_UNEXPECTED);

  sbDeviceManager::sServiceFinalized = PR_TRUE;

  // Loop through the array and call Finalize() on all the devices.
  nsresult rv;

  nsAutoLock autoLock(mLock);

  PRInt32 count = mSupportedDevices.Count();
  for (PRInt32 index = 0; index < count; index++) {
    nsCOMPtr<sbIDeviceBase> device = mSupportedDevices.ObjectAt(index);
    NS_ASSERTION(device, "Null pointer in mSupportedDevices");

    PRBool retval;
    rv = device->Finalize(&retval);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "A device failed to finalize");
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceManager::GetDeviceCount(PRUint32* aDeviceCount)
{
  NS_ENSURE_ARG_POINTER(aDeviceCount);

  nsAutoLock autoLock(mLock);

	*aDeviceCount = (PRUint32)mSupportedDevices.Count();
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceManager::GetCategoryByIndex(PRUint32 aIndex,
                                    nsAString& _retval)
{
  nsAutoLock autoLock(mLock);

  NS_ENSURE_ARG_MAX(aIndex, (PRUint32)mSupportedDevices.Count());

  nsCOMPtr<sbIDeviceBase> device = mSupportedDevices.ObjectAt(aIndex);
  NS_ENSURE_TRUE(device, NS_ERROR_NULL_POINTER);

  PRUnichar* stringBuffer;
  nsresult rv = device->GetDeviceCategory(&stringBuffer);
  NS_ENSURE_SUCCESS(rv, rv);

  _retval.Assign(stringBuffer);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceManager::GetDeviceByIndex(PRUint32 aIndex,
                                  sbIDeviceBase** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsAutoLock autoLock(mLock);

  NS_ENSURE_ARG_MAX(aIndex, (PRUint32)mSupportedDevices.Count());

  nsCOMPtr<sbIDeviceBase> device = mSupportedDevices.ObjectAt(aIndex);
  NS_ENSURE_TRUE(device, NS_ERROR_UNEXPECTED);

  NS_ADDREF(*_retval = device);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceManager::HasDeviceForCategory(const nsAString& aCategory,
                                      PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsAutoLock autoLock(mLock);

  PRUint32 dummy;
  nsresult rv = GetIndexForCategory(aCategory, &dummy);
  
  *_retval = NS_SUCCEEDED(rv);
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceManager::GetDeviceByCategory(const nsAString& aCategory,
                                     sbIDeviceBase** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsAutoLock autoLock(mLock);

  PRUint32 index;
  nsresult rv = GetIndexForCategory(aCategory, &index);

  // If a supporting device wasn't found then return an error
  NS_ENSURE_SUCCESS(rv, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<sbIDeviceBase> device =
    do_QueryInterface(mSupportedDevices.ObjectAt(index));
  NS_ENSURE_TRUE(device, NS_ERROR_UNEXPECTED);

  NS_ADDREF(*_retval = device);
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceManager::GetIndexForCategory(const nsAString& aCategory,
                                     PRUint32* _retval)
{
  // We don't bother checking arguments or locking because we assume that we
  // have already done so in our public methods.

  // First check to see if we've already looked up the result.
  if (!mLastRequestedCategory.IsEmpty() &&
      aCategory.Equals(mLastRequestedCategory)) {
    *_retval = mLastRequestedIndex;
    return NS_OK;
  }

  // Otherwise loop through the array and try to find the category.
  PRInt32 count = mSupportedDevices.Count();
  for (PRInt32 index = 0; index < count; index++) {
    nsCOMPtr<sbIDeviceBase> device = mSupportedDevices.ObjectAt(index);
    if (!device) {
      NS_WARNING("Null pointer in mSupportedDevices");
      continue;
    }

    PRUnichar* categoryBuffer;
    nsresult rv = device->GetDeviceCategory(&categoryBuffer);
    if (NS_FAILED(rv)) {
      NS_WARNING("GetDeviceCategory Failed");
      continue;
    }

    nsDependentString category(categoryBuffer);

    if (category.Equals(aCategory)) {
      mLastRequestedCategory = category;
      *_retval = mLastRequestedIndex = index;
      return NS_OK;
    }
  }

  // Not found
  mLastRequestedCategory = EmptyString();
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
sbDeviceManager::Observe(nsISupports* aSubject,
                         const char* aTopic,
                         const PRUnichar* aData)
{
  nsresult rv;
  if (nsCRT::strcmp(aTopic, kQuitApplicationTopic) == 0) {
    // The app is shutting down so it's time for our devices to finalize
    rv = Finalize();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (nsCRT::strcmp(aTopic, kXPCOMShutdownTopic) == 0) {
    // Remove ourselves from the observer service
    nsCOMPtr<nsIObserverService> observerService = 
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->RemoveObserver(this, kXPCOMShutdownTopic);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "Failed to remove XPCOMShutdown observer");

    rv = observerService->RemoveObserver(this, kQuitApplicationTopic);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "Failed to remove QuitApplication observer");
  }

  return NS_OK;
}
