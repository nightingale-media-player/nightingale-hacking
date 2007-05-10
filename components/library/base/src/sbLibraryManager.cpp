/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

/** 
* \file  sbLibraryManager.cpp
* \brief Songbird Library Manager Implementation.
*/
#include "sbLibraryManager.h"

#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>
#include <nsIObserver.h>
#include <nsIObserverService.h>
#include <nsIPrefBranch.h>
#include <nsIPrefService.h>
#include <nsIRDFDataSource.h>
#include <nsISimpleEnumerator.h>
#include <nsISupportsPrimitives.h>
#include <sbILibraryFactory.h>
#include <sbILibraryManagerListener.h>

#include <nsArrayEnumerator.h>
#include <nsAutoLock.h>
#include <nsCOMArray.h>
#include <nsComponentManagerUtils.h>
#include <nsEnumeratorUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <prlog.h>
#include <rdf.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbLibraryManager:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gLibraryManagerLog = nsnull;
#define TRACE(args) PR_LOG(gLibraryManagerLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gLibraryManagerLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

#define NS_PROFILE_STARTUP_OBSERVER_ID  "profile-after-change"
#define NS_PROFILE_SHUTDOWN_OBSERVER_ID "profile-before-change"

NS_IMPL_THREADSAFE_ISUPPORTS3(sbLibraryManager, nsIObserver,
                                                nsISupportsWeakReference,
                                                sbILibraryManager)

sbLibraryManager::sbLibraryManager()
: mLoaderCache(SB_LIBRARY_LOADER_CATEGORY),
  mLock(nsnull)
{
#ifdef PR_LOGGING
  if (!gLibraryManagerLog)
    gLibraryManagerLog = PR_NewLogModule("sbLibraryManager");
#endif

  TRACE(("sbLibraryManager[0x%x] - Created", this));

  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

sbLibraryManager::~sbLibraryManager()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if(mLock) {
    nsAutoLock::DestroyLock(mLock);
  }

  TRACE(("LibraryManager[0x%x] - Destroyed", this));
}

/* static */ NS_METHOD
sbLibraryManager::RegisterSelf(nsIComponentManager* aCompMgr,
                               nsIFile* aPath,
                               const char* aLoaderStr,
                               const char* aType,
                               const nsModuleComponentInfo *aInfo)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = categoryManager->AddCategoryEntry(APPSTARTUP_CATEGORY,
                                         SONGBIRD_LIBRARYMANAGER_DESCRIPTION,
                                         "service," SONGBIRD_LIBRARYMANAGER_CONTRACTID,
                                         PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

/**
 * \brief Register with the Observer Service.
 */
nsresult
sbLibraryManager::Init()
{
  TRACE(("sbLibraryManager[0x%x] - Init", this));
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  PRBool success = mLibraryTable.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  success = mListeners.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  mLock = nsAutoLock::NewLock("sbLibraryManager::mLock");
  NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService = 
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, NS_PROFILE_STARTUP_OBSERVER_ID,
                                    PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, NS_PROFILE_SHUTDOWN_OBSERVER_ID,
                                    PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * \brief This callback adds the enumerated libraries to an nsCOMArray.
 *
 * \param aKey      - An nsAString representing the GUID of the library.
 * \param aEntry    - An sbILibrary entry.
 * \param aUserData - An nsCOMArray to hold the enumerated pointers.
 *
 * \return PL_DHASH_NEXT on success
 * \return PL_DHASH_STOP on failure
 */
/* static */ PLDHashOperator PR_CALLBACK
sbLibraryManager::AddLibrariesToCOMArrayCallback(nsStringHashKey::KeyType aKey,
                                                 sbLibraryInfo* aEntry,
                                                 void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in hashtable!");
  NS_ASSERTION(aEntry->library, "Null library in hashtable!");

  nsCOMArray<sbILibrary>* array =
    NS_STATIC_CAST(nsCOMArray<sbILibrary>*, aUserData);

  PRBool success = array->AppendObject(aEntry->library);
  NS_ENSURE_TRUE(success, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

/**
 * \brief This callback adds the enumerated startup libraries to an nsCOMArray.
 *
 * \param aKey      - An nsAString representing the GUID of the library.
 * \param aEntry    - An sbILibrary entry.
 * \param aUserData - An nsCOMArray to hold the enumerated pointers.
 *
 * \return PL_DHASH_NEXT on success
 * \return PL_DHASH_STOP on failure
 */
/* static */ PLDHashOperator PR_CALLBACK
sbLibraryManager::AddStartupLibrariesToCOMArrayCallback(nsStringHashKey::KeyType aKey,
                                                        sbLibraryInfo* aEntry,
                                                        void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in hashtable!");
  NS_ASSERTION(aEntry->library, "Null library in hashtable!");

  nsCOMArray<sbILibrary>* array =
    NS_STATIC_CAST(nsCOMArray<sbILibrary>*, aUserData);

  if (aEntry->loader && aEntry->loadAtStartup) {
    PRBool success = array->AppendObject(aEntry->library);
    NS_ENSURE_TRUE(success, PL_DHASH_STOP);
  }

  return PL_DHASH_NEXT;
}

/**
 * \brief This callback asserts all the enumerated libraries into the given
 *        datasource.
 *
 * \param aKey      - An nsAString representing the GUID of the library.
 * \param aEntry    - An sbILibrary entry.
 * \param aUserData - An nsIRDFDataSource to assert into.
 *
 * \return PL_DHASH_NEXT on success
 * \return PL_DHASH_STOP on failure
 */
/* static */ PLDHashOperator PR_CALLBACK
sbLibraryManager::AssertAllLibrariesCallback(nsStringHashKey::KeyType aKey,
                                             sbLibraryInfo* aEntry,
                                             void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in hashtable!");
  NS_ASSERTION(aEntry->library, "Null library in hashtable!");

  nsCOMPtr<nsIRDFDataSource> ds = NS_STATIC_CAST(nsIRDFDataSource*, aUserData);
  NS_ENSURE_TRUE(ds, PL_DHASH_STOP);

  nsresult rv = AssertLibrary(ds, aEntry->library);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

/**
 * \brief This callback notifies all registered libraries that they should
 *        shutdown.
 *
 * \param aKey      - An nsAString representing the GUID of the library.
 * \param aEntry    - An sbILibrary entry.
 * \param aUserData - Should be null.
 *
 * \return PL_DHASH_NEXT always
 */
/* static */ PLDHashOperator PR_CALLBACK
sbLibraryManager::ShutdownAllLibrariesCallback(nsStringHashKey::KeyType aKey,
                                               sbLibraryInfo* aEntry,
                                               void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in hashtable!");
  NS_ASSERTION(aEntry->library, "Null library in hashtable!");

  nsresult rv = aEntry->library->Shutdown();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "A library's shutdown method failed!");

  return PL_DHASH_NEXT;
}

/**
 * \brief This method asserts a single library into the given datasource.
 *
 * \param aDataSource - The datasource to be modified.
 * \param aLibrary    - An sbILibrary to be asserted.
 *
 * \return NS_OK on success
 */
/* static */ nsresult
sbLibraryManager::AssertLibrary(nsIRDFDataSource* aDataSource,
                                sbILibrary* aLibrary)
{
  NS_NOTYETIMPLEMENTED("sbLibraryManager::AssertLibrary");
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 * \brief This method unasserts a single library in the given datasource.
 *
 * \param aDataSource - The datasource to be modified.
 * \param aLibrary    - An sbILibrary to be unasserted.
 *
 * \return NS_OK on success
 */
/* static */ nsresult
sbLibraryManager::UnassertLibrary(nsIRDFDataSource* aDataSource,
                                  sbILibrary* aLibrary)
{
  NS_NOTYETIMPLEMENTED("sbLibraryManager::UnassertLibrary");
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 * \brief This method notifies listeners that a library has been registered.
 */
/* static */ PLDHashOperator PR_CALLBACK
sbLibraryManager::NotifyListenersLibraryRegisteredCallback(nsISupportsHashKey* aKey,
                                                           void* aUserData)
{
  TRACE(("sbLibraryManager[static] - NotifyListenersLibraryRegisteredCallback"));

  NS_ASSERTION(aKey, "Nulls in the hashtable!");
  nsresult rv;
  nsCOMPtr<sbILibraryManagerListener> listener =
    do_QueryInterface(aKey->GetKey(), &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_NEXT);

  sbILibrary* library = NS_STATIC_CAST(sbILibrary*, aUserData);
  NS_ASSERTION(library, "Null pointer!");

  listener->OnLibraryRegistered(library);
  
  return PL_DHASH_NEXT;
}

/**
 * \brief This method notifies listeners that a library has been unregistered.
 */
/* static */ PLDHashOperator PR_CALLBACK
sbLibraryManager::NotifyListenersLibraryUnregisteredCallback(nsISupportsHashKey* aKey,
                                                             void* aUserData)
{
  TRACE(("sbLibraryManager[static] - NotifyListenersLibraryUnregisteredCallback"));

  NS_ASSERTION(aKey, "Nulls in the hashtable!");
  nsresult rv;
  nsCOMPtr<sbILibraryManagerListener> listener =
    do_QueryInterface(aKey->GetKey(), &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_NEXT);

  sbILibrary* library = NS_STATIC_CAST(sbILibrary*, aUserData);
  NS_ASSERTION(library, "Null pointer!");

  listener->OnLibraryUnregistered(library);
  
  return PL_DHASH_NEXT;
}

/**
 * \brief This method generates an in-memory datasource and sets it up with all
 *        the currently registered libraries.
 *
 * \return NS_OK on success
 */
nsresult
sbLibraryManager::GenerateDataSource()
{
  NS_ASSERTION(!mDataSource, "GenerateDataSource called twice!");

  nsresult rv;
  mDataSource = do_CreateInstance(NS_RDF_DATASOURCE_CONTRACTID_PREFIX
                                  "in-memory-datasource", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 libraryCount = mLibraryTable.Count();
  if (!libraryCount) {
    return NS_OK;
  }

  PRUint32 enumCount =
    mLibraryTable.EnumerateRead(AssertAllLibrariesCallback, mDataSource);
  NS_ENSURE_TRUE(enumCount == libraryCount, NS_ERROR_FAILURE);

  return NS_OK;
}

/**
 * \brief This method causes the listeners to be enumerated when a library has
 *        been registered with the Library Manager.
 *
 * \param aLibrary - The library that has been added.
 *
 * \return NS_OK on success
 */
void
sbLibraryManager::NotifyListenersLibraryRegistered(sbILibrary* aLibrary)
{
  TRACE(("sbLibraryManager[0x%x] - NotifyListenersLibraryRegistered", this));
  
  nsAutoLock lock(mLock);
  mListeners.EnumerateEntries(NotifyListenersLibraryRegisteredCallback,
                              aLibrary);
}

/**
 * \brief This method causes the listeners to be enumerated when a library has
 *        been unregistered from the Library Manager.
 *
 * \param aLibrary - The library that has been removed.
 *
 * \return NS_OK on success
 */
void
sbLibraryManager::NotifyListenersLibraryUnregistered(sbILibrary* aLibrary)
{
  TRACE(("sbLibraryManager[0x%x] - NotifyListenersLibraryUnregistered", this));
  
  nsAutoLock lock(mLock);
  mListeners.EnumerateEntries(NotifyListenersLibraryUnregisteredCallback,
                              aLibrary);
}

void
sbLibraryManager::InvokeLoaders()
{
  TRACE(("sbLibraryManager[0x%x] - InvokeLoaders", this));
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsCOMArray<sbILibraryLoader> loaders = mLoaderCache.GetEntries();
  for (PRUint32 index = 0; index < loaders.Count(); index++) {
    mCurrentLoader = loaders.ObjectAt(index);
    NS_ASSERTION(mCurrentLoader, "Null pointer!");

    nsresult rv = mCurrentLoader->OnRegisterStartupLibraries(this);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "OnRegisterStartupLibraries failed!");
  }
  mCurrentLoader = nsnull;
}

sbILibraryLoader*
sbLibraryManager::FindLoaderForLibrary(sbILibrary* aLibrary)
{
  // First see if this library is in our hash table.
  nsAutoString libraryGUID;
  nsresult rv = aLibrary->GetGuid(libraryGUID);
  NS_ENSURE_SUCCESS(rv, nsnull);

  sbLibraryInfo* libraryInfo;
  if (mLibraryTable.Get(libraryGUID, &libraryInfo) && libraryInfo->loader) {
    return libraryInfo->loader;
  }

  nsCOMPtr<sbILibraryFactory> factory;
  rv = aLibrary->GetFactory(getter_AddRefs(factory));
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsAutoString factoryType;
  rv = factory->GetType(factoryType);
  NS_ENSURE_SUCCESS(rv, nsnull);
  
  return nsnull;
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::GetMainLibrary(sbILibrary** _retval)
{
  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefService =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupportsString> supportsString;
  rv = prefService->GetComplexValue(SB_PREF_MAIN_LIBRARY,
                                    NS_GET_IID(nsISupportsString),
                                    getter_AddRefs(supportsString));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString mainLibraryGUID;
  rv = supportsString->GetData(mainLibraryGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetLibrary(mainLibraryGUID, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::GetDataSource(nsIRDFDataSource** aDataSource)
{
  TRACE(("sbLibraryManager[0x%x] - GetDataSource", this));
  NS_ENSURE_ARG_POINTER(aDataSource);

  nsAutoLock lock(mLock);

  if (!mDataSource) {
    nsresult rv = GenerateDataSource();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF(*aDataSource = mDataSource);
  return NS_OK;
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::GetLibrary(const nsAString& aGuid,
                             sbILibrary** _retval)
{
  TRACE(("sbLibraryManager[0x%x] - GetLibrary", this));
  NS_ENSURE_ARG_POINTER(_retval);

  sbLibraryInfo* libraryInfo;
  PRBool exists = mLibraryTable.Get(aGuid, &libraryInfo);
  NS_ENSURE_TRUE(exists, NS_ERROR_NOT_AVAILABLE);

  NS_ASSERTION(libraryInfo->library, "Null library in hash table!");

  NS_ADDREF(*_retval = libraryInfo->library);
  return NS_OK;
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::GetLibraries(nsISimpleEnumerator** _retval)
{
  TRACE(("sbLibraryManager[0x%x] - GetLibraries", this));
  NS_ENSURE_ARG_POINTER(_retval);

  PRUint32 libraryCount = mLibraryTable.Count();
  if (!libraryCount) {
    return NS_NewEmptyEnumerator(_retval);
  }

  nsCOMArray<sbILibrary> libraryArray(libraryCount);

  PRUint32 enumCount =
    mLibraryTable.EnumerateRead(AddLibrariesToCOMArrayCallback, &libraryArray);
  NS_ENSURE_TRUE(enumCount == libraryCount, NS_ERROR_FAILURE);

  return NS_NewArrayEnumerator(_retval, libraryArray);
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::GetStartupLibraries(nsISimpleEnumerator** _retval)
{
  TRACE(("sbLibraryManager[0x%x] - GetStartupLibraries", this));
  NS_ENSURE_ARG_POINTER(_retval);

  PRUint32 libraryCount = mLibraryTable.Count();
  if (!libraryCount) {
    return NS_NewEmptyEnumerator(_retval);
  }

  nsCOMArray<sbILibrary> libraryArray(libraryCount);

  PRUint32 enumCount =
    mLibraryTable.EnumerateRead(AddStartupLibrariesToCOMArrayCallback,
                                &libraryArray);
  NS_ENSURE_TRUE(enumCount == libraryCount, NS_ERROR_FAILURE);

  return NS_NewArrayEnumerator(_retval, libraryArray);
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::RegisterLibrary(sbILibrary* aLibrary,
                                  PRBool aLoadAtStartup)
{
  TRACE(("sbLibraryManager[0x%x] - RegisterLibrary", this));
  NS_ENSURE_ARG_POINTER(aLibrary);

  nsAutoString libraryGUID;
  nsresult rv = aLibrary->GetGuid(libraryGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool alreadyRegistered;

  sbLibraryInfo* libraryInfo = nsnull;
  if (mLibraryTable.Get(libraryGUID, &libraryInfo)) {
    NS_WARNING("Registering a library that has already been registered!");
    alreadyRegistered = PR_TRUE;
  }
  else {
    nsAutoPtr<sbLibraryInfo> newLibraryInfo(new sbLibraryInfo());
    NS_ENSURE_TRUE(newLibraryInfo, NS_ERROR_OUT_OF_MEMORY);

    PRBool success = mLibraryTable.Put(libraryGUID, newLibraryInfo);
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

    libraryInfo = newLibraryInfo.forget();
    alreadyRegistered = PR_FALSE;
  }

  libraryInfo->library = aLibrary;
  libraryInfo->loader = mCurrentLoader;

  if (aLoadAtStartup) {
    if (mCurrentLoader) {
      libraryInfo->loadAtStartup = PR_TRUE;
    }
    else {
      rv = SetLibraryLoadsAtStartup(aLibrary, aLoadAtStartup);
      if (NS_FAILED(rv)) {
        NS_WARNING("No loader can add this library to its startup group.");
        libraryInfo->loadAtStartup = PR_FALSE;
      }
    }
  }

  if (mDataSource) {
    rv = AssertLibrary(mDataSource, aLibrary);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to update dataSource!");
  }

  // Don't notify while within InvokeLoaders or if this library was previously
  // registered.
  if (!mCurrentLoader && !alreadyRegistered) {
    NotifyListenersLibraryRegistered(aLibrary);
  }

  return NS_OK;
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::UnregisterLibrary(sbILibrary* aLibrary)
{
  TRACE(("sbLibraryManager[0x%x] - UnregisterLibrary", this));
  NS_ENSURE_ARG_POINTER(aLibrary);

  NS_ASSERTION(!mCurrentLoader, "Shouldn't call this within InvokeLoaders!");

  nsAutoString libraryGUID;
  nsresult rv = aLibrary->GetGuid(libraryGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists = mLibraryTable.Get(libraryGUID, nsnull);
  if (!exists) {
    NS_WARNING("Unregistering a library that was never registered!");
    return NS_OK;
  }

  // Don't notify while within InvokeLoaders.
  if (!mCurrentLoader) {
    NotifyListenersLibraryUnregistered(aLibrary);
  }

  if (mDataSource) {
    rv = UnassertLibrary(mDataSource, aLibrary);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to update dataSource!");
  }

  mLibraryTable.Remove(libraryGUID);
  return NS_OK;
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::SetLibraryLoadsAtStartup(sbILibrary* aLibrary,
                                           PRBool aLoadAtStartup)
{
  NS_ENSURE_ARG_POINTER(aLibrary);

  // First see if this library is in our hash table.
  nsAutoString libraryGUID;
  nsresult rv = aLibrary->GetGuid(libraryGUID);
  NS_ENSURE_SUCCESS(rv, nsnull);

  sbLibraryInfo* libraryInfo;
  if (!mLibraryTable.Get(libraryGUID, &libraryInfo)) {
    NS_WARNING("Can't modify the startup of an unregistered library!");
    return NS_ERROR_INVALID_ARG;
  }

  if (libraryInfo->loader) {
    rv = libraryInfo->loader->OnLibraryStartupModified(aLibrary,
                                                       aLoadAtStartup);
    NS_ENSURE_SUCCESS(rv, rv);

    libraryInfo->loadAtStartup = aLoadAtStartup;
    return NS_OK;
  }

  // We'll have to query all the loaders to see if they can accomodate this
  // library.
  nsCOMArray<sbILibraryLoader> loaders = mLoaderCache.GetEntries();
  for (PRUint32 index = 0; index < loaders.Count(); index++) {
    nsCOMPtr<sbILibraryLoader> cachedLoader = loaders.ObjectAt(index);
    NS_ASSERTION(cachedLoader, "Null pointer!");

    rv = cachedLoader->OnLibraryStartupModified(aLibrary, aLoadAtStartup);
    if (NS_SUCCEEDED(rv)) {
      libraryInfo->loader = cachedLoader;
      libraryInfo->loadAtStartup = aLoadAtStartup;
      return NS_OK;
    }
  }

  // Fail if none of the registered loaders could accomodate the request.
  return NS_ERROR_NOT_AVAILABLE;
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::GetLibraryLoadsAtStartup(sbILibrary* aLibrary,
                                           PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(_retval);

  nsAutoString libraryGUID;
  nsresult rv = aLibrary->GetGuid(libraryGUID);
  NS_ENSURE_SUCCESS(rv, nsnull);

  sbLibraryInfo* libraryInfo;
  if (!mLibraryTable.Get(libraryGUID, &libraryInfo)) {
    NS_WARNING("Can't check the startup of an unregistered library!");
    return NS_ERROR_INVALID_ARG;
  }

  *_retval = libraryInfo->loader && libraryInfo->loadAtStartup;
  return NS_OK;
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::AddListener(sbILibraryManagerListener* aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  nsAutoLock lock(mLock);

#ifdef DEBUG
  nsISupportsHashKey* exists = mListeners.GetEntry(aListener);
  if (exists) {
    NS_WARNING("Trying to add a listener twice!");
  }
#endif

  nsISupportsHashKey* success = mListeners.PutEntry(aListener);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  return NS_OK;
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::RemoveListener(sbILibraryManagerListener* aListener)
{
  TRACE(("sbLibraryManager[0x%x] - RemoveListener", this));
  NS_ENSURE_ARG_POINTER(aListener);
  nsAutoLock lock(mLock);

#ifdef DEBUG
  nsISupportsHashKey* exists = mListeners.GetEntry(aListener);
  NS_WARN_IF_FALSE(exists, "Trying to remove a listener that was never added!");
#endif

  mListeners.RemoveEntry(aListener);

  return NS_OK;
}

/**
 * See nsIObserver.idl
 */
NS_IMETHODIMP
sbLibraryManager::Observe(nsISupports* aSubject,
                          const char* aTopic,
                          const PRUnichar* aData)
{
  TRACE(("sbLibraryManager[0x%x] - Observe: %s", this, aTopic));

  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService = 
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);

  if (strcmp(aTopic, APPSTARTUP_TOPIC) == 0) {
    return NS_OK;
  }
  else if (strcmp(aTopic, NS_PROFILE_STARTUP_OBSERVER_ID) == 0) {
    // Remove ourselves from the observer service.
    if (NS_SUCCEEDED(rv)) {
      observerService->RemoveObserver(this, NS_PROFILE_STARTUP_OBSERVER_ID);
    }

    // Ask all the library loader components to register their libraries.
    InvokeLoaders();

    // And notify any observers that we're ready to go.
    rv = observerService->NotifyObservers(NS_ISUPPORTS_CAST(sbILibraryManager*, this),
                                          SB_LIBRARY_MANAGER_READY_TOPIC,
                                          nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }
  else if (strcmp(aTopic, NS_PROFILE_SHUTDOWN_OBSERVER_ID) == 0) {

    // Remove ourselves from the observer service.
    if (NS_SUCCEEDED(rv)) {
      observerService->RemoveObserver(this, NS_PROFILE_SHUTDOWN_OBSERVER_ID);
    }

    // Tell all the registered libraries to shutdown.
    mLibraryTable.EnumerateRead(ShutdownAllLibrariesCallback, nsnull);

    return NS_OK;
  }

  NS_NOTREACHED("Observing a topic that wasn't handled!");
  return NS_OK;
}
