/* vim: set sw=2 : */
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

#include <nsXPCOM.h>
#include "sbStaticModule.h"
#include "nsStaticComponents.h"
#include <nsAutoPtr.h>
#include <nsIGenericFactory.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbStaticModule, nsIModule)

sbStaticModule::sbStaticModule()
  : mModules(kStaticModuleCount)
{
  NS_ASSERTION(kStaticModuleCount, "No static modules found!");
}

sbStaticModule::~sbStaticModule()
{
  /* destructor code */
}

nsresult sbStaticModule::Initialize(nsIComponentManager *aCompMgr,
                                    nsIFile* aLocation)
{
  if (mModules.Count()) {
    // already initialized
    return NS_ERROR_ALREADY_INITIALIZED;
  }
  nsresult rv;
  for (int i = 0; i < kStaticModuleCount; ++i) {
    nsCOMPtr<nsIModule> module;
    rv = kPStaticModules[i].getModule(aCompMgr, aLocation, getter_AddRefs(module));
    NS_ENSURE_SUCCESS(rv, rv);
    mModules.AppendObject(module);
  }
  return NS_OK;
}

/* void getClassObject (in nsIComponentManager aCompMgr,
                        in nsCIDRef aClass,
                        in nsIIDRef aIID,
                        [iid_is (aIID), retval] out nsQIResult aResult); */
NS_IMETHODIMP sbStaticModule::GetClassObject(nsIComponentManager *aCompMgr,
                                             const nsCID & aClass,
                                             const nsIID & aIID,
                                             void * *aResult)
{
  NS_ENSURE_ARG_POINTER(aCompMgr);
  NS_ENSURE_ARG_POINTER(aResult);
  NS_ENSURE_STATE(mModules.Count());
  *aResult = nsnull;
  
  for (PRInt32 i = 0; i < mModules.Count(); ++i) {
    nsresult rv = mModules[i]->GetClassObject(aCompMgr, aClass, aIID, aResult);
    if (NS_SUCCEEDED(rv)) {
      return rv;
    }
  }
  
  return NS_ERROR_FAILURE;
}

/* void registerSelf (in nsIComponentManager aCompMgr,
                      in nsIFile aLocation,
                      in string aLoaderStr,
                      in string aType); */
NS_IMETHODIMP sbStaticModule::RegisterSelf(nsIComponentManager *aCompMgr,
                                           nsIFile *aLocation,
                                           const char *aLoaderStr,
                                           const char *aType)
{
  NS_ENSURE_STATE(mModules.Count());
  
  for (PRInt32 i = 0; i < mModules.Count(); ++i) {
    nsresult rv = mModules[i]->RegisterSelf(aCompMgr, aLocation, aLoaderStr, aType);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  return NS_OK;
}

/* void unregisterSelf (in nsIComponentManager aCompMgr,
                        in nsIFile aLocation,
                        in string aLoaderStr); */
NS_IMETHODIMP sbStaticModule::UnregisterSelf(nsIComponentManager *aCompMgr,
                                             nsIFile *aLocation,
                                             const char *aLoaderStr)
{
  NS_ENSURE_STATE(mModules.Count());
  
  for (PRInt32 i = 0; i < mModules.Count(); ++i) {
    nsresult rv = mModules[i]->UnregisterSelf(aCompMgr, aLocation, aLoaderStr);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  return NS_OK;
}

/* boolean canUnload (in nsIComponentManager aCompMgr); */
NS_IMETHODIMP sbStaticModule::CanUnload(nsIComponentManager *aCompMgr,
                                        PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(mModules.Count());
  *_retval = PR_TRUE;
  nsresult rv;
  for (PRInt32 i = mModules.Count() - 1; i >= 0; --i) {
    rv = mModules[i]->CanUnload(aCompMgr, _retval);
    if (NS_FAILED(rv) || !_retval) {
      return rv;
    }
  }

  // this is what nsGenericModule::CanUnload returns; go figure.
  return NS_ERROR_FAILURE;
}

NSGETMODULE_ENTRY_POINT(sbStaticComponents)
(nsIComponentManager *servMgr,
            nsIFile* location,
            nsIModule** result)
{
  nsRefPtr<sbStaticModule> staticModule = new sbStaticModule();
  NS_ENSURE_TRUE(staticModule, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv = staticModule->Initialize(servMgr, location);
  NS_ENSURE_SUCCESS(rv, rv);
  return CallQueryInterface(staticModule.get(), result);
}
