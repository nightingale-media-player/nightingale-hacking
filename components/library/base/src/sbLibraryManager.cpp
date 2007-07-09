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
#include <sbProxyUtils.h>

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

static nsString sString;

sbLibraryManager::sbLibraryManager()
: mLoaderCache(SB_LIBRARY_LOADER_CATEGORY),
  mLock(nsnull)
{
  MOZ_COUNT_CTOR(sbLibraryManager);
#ifdef PR_LOGGING
  if (!gLibraryManagerLog)
    gLibraryManagerLog = PR_NewLogModule("sbLibraryManager");
#endif

  TRACE(("sbLibraryManager[0x%x] - Created", this));

  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

sbLibraryManager::~sbLibraryManager()
{
  if(mLock) {
    nsAutoLock::DestroyLock(mLock);
  }

  TRACE(("LibraryManager[0x%x] - Destroyed", this));
  MOZ_COUNT_DTOR(sbLibraryManager);
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
                                         "service,"
                                         SONGBIRD_LIBRARYMANAGER_CONTRACTID,
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
    static_cast<nsCOMArray<sbILibrary>*>(aUserData);

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
    static_cast<nsCOMArray<sbILibrary>*>(aUserData);

  if (aEntry->loader && aEntry->loadAtStartup) {
    PRBool success = array->AppendObject(aEntry->library);
    NS_ENSURE_TRUE(success, PL_DHASH_STOP);
  }

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
sbLibraryManager::AddListenersToCOMArrayCallback(nsISupportsHashKey::KeyType aKey,
                                                 sbILibraryManagerListener* aEntry,
                                                 void* aUserData)
{
  NS_ASSERTION(aKey, "Null key in hashtable!");
  NS_ASSERTION(aEntry, "Null entry in hashtable!");

  nsCOMArray<sbILibraryManagerListener>* array =
    static_cast<nsCOMArray<sbILibraryManagerListener>*>(aUserData);

  PRBool success = array->AppendObject(aEntry);
  NS_ENSURE_TRUE(success, PL_DHASH_STOP);

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

  nsCOMPtr<nsIRDFDataSource> ds = static_cast<nsIRDFDataSource*>(aUserData);
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
/* static */ PRBool PR_CALLBACK
sbLibraryManager::ShutdownAllLibrariesCallback(sbILibrary* aEntry,
                                               void* /*aUserData*/)
{
  NS_ASSERTION(aEntry, "Null entry in hashtable!");

  nsresult rv = aEntry->Shutdown();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "A library's shutdown method failed!");

  return PR_TRUE;
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
  
  nsCOMArray<sbILibraryManagerListener> listeners;
  {
    nsAutoLock lock(mLock);
    mListeners.EnumerateRead(AddListenersToCOMArrayCallback, &listeners);
  }

  PRInt32 count = listeners.Count();
  for (PRInt32 index = 0; index < count; index++) {
    nsCOMPtr<sbILibraryManagerListener> listener =
      listeners.ObjectAt(index);
    NS_ASSERTION(listener, "Null listener!");

    listener->OnLibraryRegistered(aLibrary);
  }
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
  
  nsCOMArray<sbILibraryManagerListener> listeners;
  {
    nsAutoLock lock(mLock);
    mListeners.EnumerateRead(AddListenersToCOMArrayCallback, &listeners);
  }

  PRInt32 count = listeners.Count();
  for (PRInt32 index = 0; index < count; index++) {
    nsCOMPtr<sbILibraryManagerListener> listener =
      listeners.ObjectAt(index);
    NS_ASSERTION(listener, "Null listener!");

    listener->OnLibraryUnregistered(aLibrary);
  }
}

void
sbLibraryManager::InvokeLoaders()
{
  TRACE(("sbLibraryManager[0x%x] - InvokeLoaders", this));
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsCOMArray<sbILibraryLoader> loaders = mLoaderCache.GetEntries();
  PRInt32 count = loaders.Count();
  for (PRInt32 index = 0; index < count; index++) {
    mCurrentLoader = loaders.ObjectAt(index);
    NS_ASSERTION(mCurrentLoader, "Null pointer!");

    nsresult rv = mCurrentLoader->OnRegisterStartupLibraries(this);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "OnRegisterStartupLibraries failed!");
  }
  mCurrentLoader = nsnull;
}

/**
 * This method exists because we have a fundamental problem with threadsafety.
 * We have to lock across access to the contents of mLibraryTable or else copy
 * its contents. RegisterLibrary calls SetLibraryLoadsAtStartup to handle some
 * startup cases, but SetLibraryLoadsAtStartup is an external IDL method and
 * requires that the library has already been registered. Hence this method.
 */
nsresult
sbLibraryManager::SetLibraryLoadsAtStartupInternal(sbILibrary* aLibrary,
                                                   PRBool aLoadAtStartup,
                                                   sbLibraryInfo** aInfo)
{
  TRACE(("sbLibraryManager[0x%x] - SetLibraryLoadsAtStartupInternal", this));

  nsresult rv = NS_ERROR_NOT_AVAILABLE;

  nsAutoPtr<sbLibraryInfo> libraryInfo(*aInfo ? *aInfo : new sbLibraryInfo());
  NS_ENSURE_TRUE(libraryInfo, NS_ERROR_OUT_OF_MEMORY);

  if (!*aInfo) {
    // First see if this library is in our hash table.
    nsAutoString libraryGUID;
    rv = aLibrary->GetGuid(libraryGUID);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoLock lock(mLock);

    sbLibraryInfo* tableLibraryInfo;
    if (!mLibraryTable.Get(libraryGUID, &tableLibraryInfo)) {
      NS_WARNING("Can't modify the startup of an unregistered library!");
      return NS_ERROR_INVALID_ARG;
    }

    // Copy these values to our new sbLibraryInfo object.
    libraryInfo->loader = tableLibraryInfo->loader;
    libraryInfo->library = tableLibraryInfo->library;
    libraryInfo->loadAtStartup = tableLibraryInfo->loadAtStartup;
  }

  if (libraryInfo->loader) {
    rv = libraryInfo->loader->OnLibraryStartupModified(aLibrary,
                                                       aLoadAtStartup);
    NS_ENSURE_SUCCESS(rv, rv);

    libraryInfo->loadAtStartup = aLoadAtStartup;
  }
  else {
    // See if we can find a loader for this library.
    PRInt32 loaderCount;
    nsCOMArray<sbILibraryLoader> loaders;
    {
      nsAutoLock lock(mLock);
      nsCOMArray<sbILibraryLoader> cachedLoaders = mLoaderCache.GetEntries();

      loaderCount = cachedLoaders.Count();
      loaders.SetCapacity(loaderCount);
      loaders.AppendObjects(cachedLoaders);
    }

    NS_ENSURE_TRUE(loaderCount > 0, NS_ERROR_NOT_AVAILABLE);

    for (PRInt32 index = 0; index < loaderCount; index++) {
      nsCOMPtr<sbILibraryLoader> cachedLoader = loaders.ObjectAt(index);
      NS_ASSERTION(cachedLoader, "Null pointer!");

      rv = cachedLoader->OnLibraryStartupModified(aLibrary, aLoadAtStartup);
      if (NS_SUCCEEDED(rv)) {
        libraryInfo->loader = cachedLoader;
        libraryInfo->loadAtStartup = aLoadAtStartup;
      }
    }
  }

  if (NS_FAILED(rv)) {
    // Fail if none of the registered loaders could accomodate the request.
    return NS_ERROR_NOT_AVAILABLE;
  }

  *aInfo = libraryInfo.forget();
  return NS_OK;
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

  nsCOMPtr<sbILibrary> library;

  {
    nsAutoLock lock(mLock);

    sbLibraryInfo* libraryInfo;
    PRBool exists = mLibraryTable.Get(aGuid, &libraryInfo);
    NS_ENSURE_TRUE(exists, NS_ERROR_NOT_AVAILABLE);

    NS_ASSERTION(libraryInfo->library, "Null library in hash table!");
    library = libraryInfo->library;
  }

  NS_ADDREF(*_retval = library);
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

  nsCOMArray<sbILibrary> libraryArray;

  {
    nsAutoLock lock(mLock);

    PRUint32 libraryCount = mLibraryTable.Count();
    if (!libraryCount) {
      return NS_NewEmptyEnumerator(_retval);
    }

    libraryArray.SetCapacity(libraryCount);

    PRUint32 enumCount =
      mLibraryTable.EnumerateRead(AddLibrariesToCOMArrayCallback,
                                  &libraryArray);
    NS_ENSURE_TRUE(enumCount == libraryCount, NS_ERROR_FAILURE);
  }

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

  nsCOMArray<sbILibrary> libraryArray;

  {
    nsAutoLock lock(mLock);

    PRUint32 libraryCount = mLibraryTable.Count();
    if (!libraryCount) {
      return NS_NewEmptyEnumerator(_retval);
    }

    libraryArray.SetCapacity(libraryCount);

    PRUint32 enumCount =
      mLibraryTable.EnumerateRead(AddStartupLibrariesToCOMArrayCallback,
                                  &libraryArray);
    NS_ENSURE_TRUE(enumCount == libraryCount, NS_ERROR_FAILURE);
  }

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

  {
    nsAutoLock lock(mLock);

    if (mLibraryTable.Get(libraryGUID, nsnull)) {
      NS_WARNING("Registering a library that has already been registered!");
      return NS_OK;
    }
  }

  nsAutoPtr<sbLibraryInfo> newLibraryInfo(new sbLibraryInfo());
  NS_ENSURE_TRUE(newLibraryInfo, NS_ERROR_OUT_OF_MEMORY);

  newLibraryInfo->library = aLibrary;
  newLibraryInfo->loader = mCurrentLoader;

  if (aLoadAtStartup) {
    if (mCurrentLoader) {
      newLibraryInfo->loadAtStartup = PR_TRUE;
    }
    else {
      sbLibraryInfo* libraryInfo = newLibraryInfo;
      rv = SetLibraryLoadsAtStartupInternal(aLibrary, aLoadAtStartup,
                                            &libraryInfo);
      if (NS_FAILED(rv)) {
        NS_WARNING("No loader can add this library to its startup group.");
        newLibraryInfo->loadAtStartup = PR_FALSE;
      }
    }
  }

  {
    nsAutoLock lock(mLock);

    PRBool success = mLibraryTable.Put(libraryGUID, newLibraryInfo);
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

    newLibraryInfo.forget();
  }


  if (mDataSource) {
    rv = AssertLibrary(mDataSource, aLibrary);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to update dataSource!");
  }

  // Don't notify while within InvokeLoaders.
  if (!mCurrentLoader) {
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

  {
    nsAutoLock lock (mLock);

    sbLibraryInfo* libInfo;
    if (!mLibraryTable.Get(libraryGUID, &libInfo)) {
      NS_WARNING("Unregistering a library that was never registered!");
      return NS_OK;
    }

    mLibraryTable.Remove(libraryGUID);
  }

  // Don't notify while within InvokeLoaders.
  if (!mCurrentLoader) {
    NotifyListenersLibraryUnregistered(aLibrary);
  }

  if (mDataSource) {
    rv = UnassertLibrary(mDataSource, aLibrary);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to update dataSource!");
  }

  return NS_OK;
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::SetLibraryLoadsAtStartup(sbILibrary* aLibrary,
                                           PRBool aLoadAtStartup)
{
  TRACE(("sbLibraryManager[0x%x] - SetLibraryLoadsAtStartup", this));
  NS_ENSURE_ARG_POINTER(aLibrary);

  sbLibraryInfo* outLibInfo = nsnull;
  nsresult rv = SetLibraryLoadsAtStartupInternal(aLibrary, aLoadAtStartup,
                                                 &outLibInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  // Protect from leaks.
  nsAutoPtr<sbLibraryInfo> libraryInfo(outLibInfo);

#ifdef DEBUG
  // Prevent anyone from using this pointer as they should use libraryInfo.
  // It will dangle in release mode, but who cares.
  outLibInfo = nsnull;
#endif

  // There's a possibility that another thread came through and unregistered
  // the library while we were messing around above... That would suck, but we
  // can't hang on to our lock while calling OnLibraryStartupModified. If that
  // ever happens then we're just going to return failure and let the caller
  // deal with it.

  nsAutoString libraryGUID;
  rv = aLibrary->GetGuid(libraryGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoLock lock(mLock);

  if (!mLibraryTable.Get(libraryGUID, nsnull)) {
    NS_WARNING("Another thread unregistered the library you're trying to modify!");
    return NS_ERROR_UNEXPECTED;
  }

  PRBool success = mLibraryTable.Put(libraryGUID, libraryInfo);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  
  libraryInfo.forget();
  return NS_OK;
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::GetLibraryLoadsAtStartup(sbILibrary* aLibrary,
                                           PRBool* _retval)
{
  TRACE(("sbLibraryManager[0x%x] - GetLibraryLoadsAtStartup", this));
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(_retval);

  nsAutoString libraryGUID;
  nsresult rv = aLibrary->GetGuid(libraryGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoLock lock(mLock);

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
  TRACE(("sbLibraryManager[0x%x] - AddListener", this));
  NS_ENSURE_ARG_POINTER(aListener);

  {
    nsAutoLock lock(mLock);

    if (mListeners.Get(aListener, nsnull)) {
      NS_WARNING("Trying to add a listener twice!");
      return NS_OK;
    }
  }

  // Make a proxy for the listener that will always send callbacks to the
  // current thread.
  nsCOMPtr<sbILibraryManagerListener> proxy;
  nsresult rv = SB_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                                     NS_GET_IID(sbILibraryManagerListener),
                                     aListener,
                                     NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                     getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoLock lock(mLock);

  // Add the proxy to the hash table, using the listener as the key.
  PRBool success = mListeners.Put(aListener, proxy);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

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
  if (!mListeners.Get(aListener, nsnull)) {
    NS_WARNING("Trying to remove a listener that was never added!");
  }
#endif
  mListeners.Remove(aListener);

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

    // Notify observers that we're about to notify our libraries of shutdown
    rv = observerService->NotifyObservers(NS_ISUPPORTS_CAST(sbILibraryManager*, this),
                                          SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC,
                                          nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    // Tell all the registered libraries to shutdown.
    nsCOMArray<sbILibrary> libraries;
    {
      nsAutoLock lock(mLock);
      mLibraryTable.EnumerateRead(AddLibrariesToCOMArrayCallback, &libraries);
    }
    libraries.EnumerateForwards(ShutdownAllLibrariesCallback, nsnull);

    libraries.Clear();

    // Notify observers that we're totally shutdown
    rv = observerService->NotifyObservers(NS_ISUPPORTS_CAST(sbILibraryManager*, this),
                                          SB_LIBRARY_MANAGER_AFTER_SHUTDOWN_TOPIC,
                                          nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_NOTREACHED("Observing a topic that wasn't handled!");
  return NS_OK;
}
