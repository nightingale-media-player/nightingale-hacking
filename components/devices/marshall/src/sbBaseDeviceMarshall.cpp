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

#include "sbBaseDeviceMarshall.h"
#include <nsISupportsPrimitives.h>
#include <nsIMutableArray.h>
#include <nsICategoryManager.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <sbIDeviceControllerRegistrar.h>
#include <sbIDeviceController.h>
#include <sbIDeviceCompatibility.h>
#include <nsIPropertyBag.h>
#include <nsIUUIDGenerator.h>

sbBaseDeviceMarshall::sbBaseDeviceMarshall(nsACString const & categoryName) :
  mCategoryName(categoryName), mIsMonitoring(PR_TRUE)
{
}

sbBaseDeviceMarshall::~sbBaseDeviceMarshall()
{
  NS_ASSERTION(!mIsMonitoring,
               "A device marshaller is being released with monitoring enabled");
}

void sbBaseDeviceMarshall::RegisterControllers(sbIDeviceControllerRegistrar *registrar)
{
  nsIArray * controllers = GetControllers();
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  if (controllers
      && NS_SUCCEEDED(controllers->Enumerate(getter_AddRefs(enumerator)))
      && enumerator) {
    bool more;
    while (NS_SUCCEEDED(enumerator->HasMoreElements(&more)) && more) {
      nsCOMPtr<nsISupports> comPtr;
      if (NS_SUCCEEDED(enumerator->GetNext(getter_AddRefs(comPtr))) && comPtr) {
        nsCOMPtr<sbIDeviceController> controller(do_QueryInterface(comPtr));
        registrar->RegisterController(controller);
      }
    }
  }
}

inline
void AppendDeviceController(nsCOMPtr<nsISupports> & ptr,
                            nsCOMPtr<nsIMutableArray> & controllers)
{
  // Get the string value from the category entry
  nsCOMPtr<nsISupportsCString> stringValue(do_QueryInterface(ptr));
  nsCString controllerName;
  nsresult rv;
  if (stringValue && NS_SUCCEEDED(stringValue->GetData(controllerName))) {
    nsCOMPtr<sbIDeviceController> deviceController =
      do_CreateInstance(controllerName.get() , &rv);
    NS_ENSURE_SUCCESS(rv, /* void */);
    
    // check if the device manager already has a matching controller
    nsCOMPtr<sbIDeviceControllerRegistrar> controllerRegistrar =
      do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
    NS_ENSURE_SUCCESS(rv, /* void */);
    
    nsID* controllerId = nsnull;
    rv = deviceController->GetId(&controllerId);
    NS_ENSURE_SUCCESS(rv, /* void */);
    
    nsCOMPtr<sbIDeviceController> existingController;
    rv = controllerRegistrar->GetController(controllerId,
                                            getter_AddRefs(existingController));
    NS_Free(controllerId);
    
    if (NS_SUCCEEDED(rv) && existingController) {
      deviceController = existingController;
    }
    
    controllers->AppendElement(deviceController, PR_FALSE);
  }
}
static nsresult CopyCategoriesToArray(nsCOMPtr<nsISimpleEnumerator> & enumerator,
                                  nsCOMPtr<nsIMutableArray> & controllers)
{
  // Start with a fresh array
  nsresult rv = controllers->Clear();
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Enumerate the category entries
  bool more;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&more)) && more) {
    nsCOMPtr<nsISupports> ptr;
    if (NS_SUCCEEDED(enumerator->GetNext(getter_AddRefs(ptr))) && ptr) {
      AppendDeviceController(ptr,
                             controllers);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return rv;
}

nsIArray * sbBaseDeviceMarshall::RefreshControllers()
{
  if (!mControllers) {
    nsresult rv;
    mControllers = do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
    if (NS_FAILED(rv)) {
      NS_ERROR("unable to create an nsArray");
      return nsnull;
    }
  }
  nsCOMPtr<nsIMutableArray> controllers(do_QueryInterface(mControllers));
  nsCOMPtr<nsISimpleEnumerator> categoryEnumerator;
  if (NS_SUCCEEDED(GetCategoryManagerEnumerator(categoryEnumerator))) {
    if (NS_FAILED(CopyCategoriesToArray(categoryEnumerator, controllers)))
      return nsnull;
  }
  return mControllers;
}

nsresult sbBaseDeviceMarshall::GetCategoryManagerEnumerator(nsCOMPtr<nsISimpleEnumerator> & enumerator)
{
  nsCOMPtr<nsICategoryManager> cm(do_GetService(NS_CATEGORYMANAGER_CONTRACTID));
  return cm->EnumerateCategory(PromiseFlatCString(mCategoryName).get(), getter_AddRefs(enumerator));
}

/**
 * Returns true if comp2 is not incompatible and is "more" compatible than comp1
 */
inline
bool CompareCompatibility(sbIDeviceCompatibility * comp1,
                            sbIDeviceCompatibility * comp2)
{
  PRUint32 compVal1 = sbIDeviceCompatibility::INCOMPATIBLE;
  PRUint32 compVal2 = sbIDeviceCompatibility::INCOMPATIBLE;
  
  return (comp1 && NS_SUCCEEDED(comp1->GetCompatibility(&compVal1))) || 
         ((comp2 && NS_SUCCEEDED(comp2->GetCompatibility(&compVal2))) && 
          compVal2 > compVal1 && 
          compVal2 != sbIDeviceCompatibility::INCOMPATIBLE);
}

bool sbBaseDeviceMarshall::CompatibilityComparer::Compare(sbIDeviceController * controller,
                                                            nsIPropertyBag * deviceParameters)
{
  nsCOMPtr<sbIDeviceCompatibility> compatibility;
  if (NS_SUCCEEDED(controller->GetCompatibility(deviceParameters,
                                                getter_AddRefs(compatibility)))
      && compatibility) {
    if (!mCompatibility && CompareCompatibility(mCompatibility, compatibility)) {
      SetBestMatch(controller, compatibility);
    }
  }
  return PR_TRUE;
}

sbIDeviceController* sbBaseDeviceMarshall::FindCompatibleControllers(nsIPropertyBag * deviceParams,
                                                                     CompatibilityComparer & deviceComparer)
{
  // Get the list of controllers
  nsIArray * controllers = GetControllers();
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  if (controllers
      && NS_SUCCEEDED(controllers->Enumerate(getter_AddRefs(enumerator)))
      && enumerator) {
    // Enumerate the list of controllers
    bool more;
    while (NS_SUCCEEDED(enumerator->HasMoreElements(&more)) && more) {
      nsCOMPtr<sbIDeviceController> controller;
      if (NS_SUCCEEDED(enumerator->GetNext(getter_AddRefs(controller)))
          && controller) {
        nsCOMPtr<sbIDeviceCompatibility> compatibilityObj;
        if (!deviceComparer.Compare(controller, deviceParams))
          break;
      }
    }
  }
  // This may be null if none were found
  return deviceComparer.GetBestMatch();
}
