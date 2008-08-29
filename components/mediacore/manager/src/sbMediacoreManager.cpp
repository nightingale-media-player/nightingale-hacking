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

#include "sbMediacoreManager.h"

#include <nsIAppStartupNotifier.h>
#include <nsIClassInfoImpl.h>
#include <nsIMutableArray.h>
#include <nsIObserverService.h>
#include <nsIProgrammingLanguage.h>
#include <nsISupportsPrimitives.h>

#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>

#include <sbIMediacore.h>
#include <sbIMediacoreFactory.h>

/* observer topics */
#define NS_PROFILE_STARTUP_OBSERVER_ID          "profile-after-change"
#define NS_QUIT_APPLICATION_GRANTED_OBSERVER_ID "quit-application-granted"
#define NS_PROFILE_SHUTDOWN_OBSERVER_ID         "profile-before-change"

/* default size of hashtable for active core instances */
#define SB_CORE_HASHTABLE_SIZE    (4)
#define SB_FACTORY_HASHTABLE_SIZE (4)

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediacoreManager:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gMediacoreManager = nsnull;
#define TRACE(args) PR_LOG(gMediacoreManager, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMediacoreManager, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

NS_IMPL_THREADSAFE_ADDREF(sbMediacoreManager)
NS_IMPL_THREADSAFE_RELEASE(sbMediacoreManager)
NS_IMPL_QUERY_INTERFACE5_CI(sbMediacoreManager,
                            sbIMediacoreManager,
                            sbIMediacoreFactoryRegistrar,
                            nsISupportsWeakReference,
                            nsIClassInfo,
                            nsIObserver)
NS_IMPL_CI_INTERFACE_GETTER3(sbMediacoreManager,
                             sbIMediacoreManager,
                             sbIMediacoreFactoryRegistrar,
                             nsISupportsWeakReference)

NS_DECL_CLASSINFO(sbMediacoreManager)
NS_IMPL_THREADSAFE_CI(sbMediacoreManager)

sbMediacoreManager::sbMediacoreManager()
: mMonitor(nsnull)
{
#ifdef PR_LOGGING
  if (!gMediacoreManager)
    gMediacoreManager = PR_NewLogModule("sbMediacoreManager");
#endif

  TRACE(("sbMediacoreManager[0x%x] - Created", this));
}

sbMediacoreManager::~sbMediacoreManager()
{
  TRACE(("sbMediacoreManager[0x%x] - Destroyed", this));

  if(mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
  }
}

template<class T>
PLDHashOperator sbMediacoreManager::EnumerateIntoArrayStringKey(
                                      const nsAString& aKey,
                                      T* aData,
                                      void* aArray)
{
  TRACE(("sbMediacoreManager[0x%x] - EnumerateIntoArray (String)"));

  nsIMutableArray *array = (nsIMutableArray*)aArray;
  nsresult rv;
  nsCOMPtr<nsISupports> supports = do_QueryInterface(aData, &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  rv = array->AppendElement(aData, false);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

template<class T>
PLDHashOperator sbMediacoreManager::EnumerateIntoArrayISupportsKey(
                                      nsISupports* aKey,
                                      T* aData,
                                      void* aArray)
{
  TRACE(("sbMediacoreManager[0x%x] - EnumerateIntoArray (nsISupports)"));

  nsIMutableArray *array = (nsIMutableArray*)aArray;
  nsresult rv;
  nsCOMPtr<nsISupports> supports = do_QueryInterface(aData, &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  rv = array->AppendElement(aData, false);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

nsresult
sbMediacoreManager::Init() 
{
  TRACE(("sbMediacoreManager[0x%x] - Init", this));

  mMonitor = nsAutoMonitor::NewMonitor("sbMediacoreManager::mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  PRBool success = mCores.Init(SB_CORE_HASHTABLE_SIZE);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mFactories.Init(SB_FACTORY_HASHTABLE_SIZE);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

// ----------------------------------------------------------------------------
// sbIMediacoreManager Interface 
// ----------------------------------------------------------------------------

NS_IMETHODIMP 
sbMediacoreManager::GetPrimaryCore(sbIMediacore * *aPrimaryCore)
{
  TRACE(("sbMediacoreManager[0x%x] - GetPrimaryCore", this));
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbMediacoreManager::SetPrimaryCore(sbIMediacore * aPrimaryCore)
{
  TRACE(("sbMediacoreManager[0x%x] - SetPrimaryCore", this));
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbMediacoreManager::GetBalanceControl(
                      sbIMediacoreBalanceControl * *aBalanceControl)
{
  TRACE(("sbMediacoreManager[0x%x] - GetBalanceControl", this));
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbMediacoreManager::GetVolumeControl(
                      sbIMediacoreVolumeControl * *aVolumeControl)
{
  TRACE(("sbMediacoreManager[0x%x] - GetVolumeControl", this));
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbMediacoreManager::GetEqualizer(
                      sbIMediacoreSimpleEqualizer * *aEqualizer)
{
  TRACE(("sbMediacoreManager[0x%x] - GetEqualizer", this));
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbMediacoreManager::GetPlaybackControl(
                      sbIMediacorePlaybackControl * *aPlaybackControl)
{
  TRACE(("sbMediacoreManager[0x%x] - GetPlaybackControl", this));
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbMediacoreManager::GetCapabilities(
                      sbIMediacoreCapabilities * *aCapabilities)
{
  TRACE(("sbMediacoreManager[0x%x] - GetCapabilities", this));
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbMediacoreManager::GetSequencer(
                      sbIMediacoreSequencer * *aSequencer)
{
  TRACE(("sbMediacoreManager[0x%x] - GetSequencer", this));
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbMediacoreManager::SetSequencer(
                      sbIMediacoreSequencer * aSequencer)
{
  TRACE(("sbMediacoreManager[0x%x] - SetSequencer", this));
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  return NS_ERROR_NOT_IMPLEMENTED;
}

// ----------------------------------------------------------------------------
// sbIMediacoreFactoryRegistrar Interface 
// ----------------------------------------------------------------------------

NS_IMETHODIMP 
sbMediacoreManager::GetFactories(nsIArray * *aFactories)
{
  TRACE(("sbMediacoreManager[0x%x] - GetFactories", this));
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aFactories);

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsIMutableArray> mutableArray = 
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoMonitor mon(mMonitor);
  mFactories.EnumerateRead(sbMediacoreManager::EnumerateIntoArrayISupportsKey, 
                           mutableArray.get());

  PRUint32 length = 0;
  rv = mutableArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  if(length < mCores.Count()) {
    return NS_ERROR_FAILURE;
  }

  rv = CallQueryInterface(mutableArray, aFactories);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreManager::GetInstances(nsIArray * *aInstances)
{
  TRACE(("sbMediacoreManager[0x%x] - GetInstances", this));
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aInstances);

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsIMutableArray> mutableArray = 
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoMonitor mon(mMonitor);
  
  mCores.EnumerateRead(sbMediacoreManager::EnumerateIntoArrayStringKey, 
                       mutableArray.get());

  PRUint32 length = 0;
  rv = mutableArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  if(length < mCores.Count()) {
    return NS_ERROR_FAILURE;
  }

  rv = CallQueryInterface(mutableArray, aInstances);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreManager::CreateMediacore(const nsAString & aContractID, 
                                    const nsAString & aInstanceName, 
                                    sbIMediacore **_retval)
{
  TRACE(("sbMediacoreManager[0x%x] - CreateMediacore", this));
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv = NS_ERROR_UNEXPECTED;
  NS_ConvertUTF16toUTF8 contractId(aContractID);

  nsCOMPtr<sbIMediacoreFactory> coreFactory = 
    do_CreateInstance(contractId.BeginReading(), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediacore> core;
  rv = GetMediacore(aInstanceName, getter_AddRefs(core));
  
  if(NS_SUCCEEDED(rv)) {
    core.forget(_retval);
    return NS_OK;
  }

  nsAutoMonitor mon(mMonitor);

  rv = coreFactory->Create(aInstanceName, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mCores.Put(aInstanceName, *_retval);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreManager::CreateMediacoreWithFactory(sbIMediacoreFactory *aFactory, 
                                               const nsAString & aInstanceName, 
                                               sbIMediacore **_retval)
{
  TRACE(("sbMediacoreManager[0x%x] - CreateMediacoreWithFactory", this));
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aFactory);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediacore> core;
  nsresult rv = GetMediacore(aInstanceName, getter_AddRefs(core));

  if(NS_SUCCEEDED(rv)) {
    core.forget(_retval);
    return NS_OK;
  }

  rv = aFactory->Create(aInstanceName, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mCores.Put(aInstanceName, *_retval);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreManager::GetMediacore(const nsAString & aInstanceName, 
                                 sbIMediacore **_retval)
{
  TRACE(("sbMediacoreManager[0x%x] - GetMediacore", this));
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(_retval);
  
  nsCOMPtr<sbIMediacore> core;
  
  nsAutoMonitor mon(mMonitor);

  PRBool success = mCores.Get(aInstanceName, getter_AddRefs(core));
  NS_ENSURE_TRUE(success, NS_ERROR_NOT_AVAILABLE);

  core.forget(_retval);

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreManager::DestroyMediacore(const nsAString & aInstanceName)
{
  TRACE(("sbMediacoreManager[0x%x] - DestroyMediacore", this));
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<sbIMediacore> core;

  nsAutoMonitor mon(mMonitor);

  PRBool success = mCores.Get(aInstanceName, getter_AddRefs(core));
  NS_ENSURE_TRUE(success, NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_TRUE(core, NS_ERROR_UNEXPECTED);
  
  nsresult rv = core->Shutdown();
  NS_ENSURE_SUCCESS(rv, rv);

  mCores.Remove(aInstanceName);
  
  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreManager::RegisterFactory(sbIMediacoreFactory *aFactory)
{
  TRACE(("sbMediacoreManager[0x%x] - RegisterFactory", this));
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aFactory);

  nsAutoMonitor mon(mMonitor);

  PRBool success = mFactories.Put(aFactory, aFactory);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreManager::UnregisterFactory(sbIMediacoreFactory *aFactory)
{
  TRACE(("sbMediacoreManager[0x%x] - UnregisterFactory", this));
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aFactory);

  nsAutoMonitor mon(mMonitor);

  mFactories.Remove(aFactory);

  return NS_OK;
}

// ----------------------------------------------------------------------------
// nsIObserver Interface 
// ----------------------------------------------------------------------------

NS_IMETHODIMP sbMediacoreManager::Observe(nsISupports *aSubject,
                                          const char *aTopic,
                                          const PRUnichar *aData)
{
  TRACE(("sbMediacoreManager[0x%x] - Observe", this));

  nsresult rv = NS_ERROR_UNEXPECTED;
  
  if (!strcmp(aTopic, APPSTARTUP_CATEGORY)) {
    // listen for profile startup and profile shutdown messages
    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIObserver> observer =
      do_QueryInterface(NS_ISUPPORTS_CAST(nsIObserver*, this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->AddObserver(observer, NS_PROFILE_STARTUP_OBSERVER_ID, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->AddObserver(observer, NS_QUIT_APPLICATION_GRANTED_OBSERVER_ID, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->AddObserver(observer, NS_PROFILE_SHUTDOWN_OBSERVER_ID, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

  } 
  else if (!strcmp(NS_PROFILE_STARTUP_OBSERVER_ID, aTopic)) {

    // Called after the profile has been loaded, so prefs and such are available
    rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);

  } 
  else if (!strcmp(NS_QUIT_APPLICATION_GRANTED_OBSERVER_ID, aTopic)) {
    
    // Called when the request to shutdown has been granted
    // We can't watch the -requested notification since it may not always be
    // fired :(

    // Pre-shutdown.

    // Remove the observer so we don't get called a second time, since we are
    // shutting down anyways this should not cause problems.
    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIObserver> observer =
      do_QueryInterface(NS_ISUPPORTS_CAST(nsIObserver*, this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->RemoveObserver(observer, NS_QUIT_APPLICATION_GRANTED_OBSERVER_ID);
    NS_ENSURE_SUCCESS(rv, rv);

  } 
  else if (!strcmp(NS_PROFILE_SHUTDOWN_OBSERVER_ID, aTopic)) {

    // Final shutdown.

    // remove all the observers
    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIObserver> observer =
      do_QueryInterface(NS_ISUPPORTS_CAST(nsIObserver*, this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->RemoveObserver(observer, NS_PROFILE_STARTUP_OBSERVER_ID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->RemoveObserver(observer, NS_PROFILE_SHUTDOWN_OBSERVER_ID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
