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

#include <nsArrayEnumerator.h>
#include <nsCOMArray.h>
#include <nsComponentManagerUtils.h>
#include <nsEnumeratorUtils.h>
#include <nsIRDFDataSource.h>
#include <nsISimpleEnumerator.h>
#include <nsServiceManagerUtils.h>
#include <prlog.h>
#include <rdf.h>
#include <sbILibrary.h>
#include <sbILibraryFactory.h>

/*
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

NS_IMPL_ISUPPORTS1(sbLibraryManager, sbILibraryManager)

sbLibraryManager::sbLibraryManager()
{
#ifdef PR_LOGGING
  if (!gLibraryManagerLog)
    gLibraryManagerLog = PR_NewLogModule("sbLibraryManager");
#endif

  TRACE(("LibraryManager[0x%x] - Created", this));
}

sbLibraryManager::~sbLibraryManager()
{
  TRACE(("LibraryManager[0x%x] - Destroyed", this));
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
                                                 sbILibrary* aEntry,
                                                 void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in hashtable!");

  nsCOMArray<sbILibrary>* array =
    NS_STATIC_CAST(nsCOMArray<sbILibrary>*, aUserData);
  NS_ENSURE_TRUE(array, PL_DHASH_STOP);

  PRBool success = array->AppendObject(aEntry);
  NS_ENSURE_TRUE(success, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

/**
 * \brief This callback adds the enumerated factories to an nsCOMArray.
 *
 * \param aKey      - An nsAString representing the type of the factory.
 * \param aEntry    - An sbILibraryFactory entry.
 * \param aUserData - An nsCOMArray to hold the enumerated pointers.
 *
 * \return PL_DHASH_NEXT on success
 * \return PL_DHASH_STOP on failure
 */
/* static */ PLDHashOperator PR_CALLBACK
sbLibraryManager::AddFactoriesToCOMArrayCallback(nsStringHashKey::KeyType aKey,
                                                 sbILibraryFactory* aEntry,
                                                 void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in hashtable!");

  nsCOMArray<sbILibraryFactory>* array =
    NS_STATIC_CAST(nsCOMArray<sbILibraryFactory>*, aUserData);
  NS_ENSURE_TRUE(array, PL_DHASH_STOP);

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
                                             sbILibrary* aEntry,
                                             void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in hashtable!");

  nsCOMPtr<nsIRDFDataSource> ds = NS_STATIC_CAST(nsIRDFDataSource*, aUserData);
  NS_ENSURE_TRUE(ds, PL_DHASH_STOP);

  nsresult rv = AssertLibrary(ds, aEntry);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

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
/* static */ NS_METHOD
sbLibraryManager::AssertLibrary(nsIRDFDataSource* aDataSource,
                                sbILibrary* aLibrary)
{
  NS_NOTREACHED("Not yet implemented");
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
/* static */ NS_METHOD
sbLibraryManager::UnassertLibrary(nsIRDFDataSource* aDataSource,
                                  sbILibrary* aLibrary)
{
  NS_NOTREACHED("Not yet implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 * \brief This method generates an in-memory datasource and sets it up with all
 *        the currently registered libraries.
 *
 * \return NS_OK on success
 */
NS_METHOD
sbLibraryManager::GenerateDataSource()
{
  NS_ASSERTION(!mDataSource, "GenerateDataSource called twice!");

  nsresult rv;
  mDataSource =
    do_CreateInstance(NS_RDF_DATASOURCE_CONTRACTID_PREFIX "in-memory-datasource", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mLibraryTable.IsInitialized()) {
    return NS_OK;
  }

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
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::GetLibrary(const nsAString& aGuid,
                             sbILibrary** _retval)
{
  TRACE(("LibraryManager[0x%x] - GetLibrary", this));
  NS_ENSURE_ARG_POINTER(_retval);

  if (!mLibraryTable.IsInitialized()) {
    NS_WARNING("Called GetLibrary before any libraries were registered!");
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<sbILibrary> library;
  PRBool exists = mLibraryTable.Get(aGuid, getter_AddRefs(library));
  NS_ENSURE_TRUE(exists, NS_ERROR_NOT_AVAILABLE);

  NS_ADDREF(*_retval = library);
  return NS_OK;
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::GetLibraryFactory(const nsAString& aType,
                                    sbILibraryFactory** _retval)
{
  TRACE(("LibraryManager[0x%x] - GetLibraryFactory", this));
  NS_ENSURE_ARG_POINTER(_retval);

  if (!mFactoryTable.IsInitialized()) {
    NS_WARNING("Called GetLibraryFactory before any factories were registered!");
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<sbILibraryFactory> factory;
  PRBool exists = mFactoryTable.Get(aType, getter_AddRefs(factory));
  NS_ENSURE_TRUE(exists, NS_ERROR_NOT_AVAILABLE);

  NS_ADDREF(*_retval = factory);
  return NS_OK;
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::RegisterLibrary(sbILibrary* aLibrary)
{
  TRACE(("LibraryManager[0x%x] - RegisterLibrary", this));
  NS_ENSURE_ARG_POINTER(aLibrary);

  if (!mLibraryTable.IsInitialized()) {
    PRBool success = mLibraryTable.Init();
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  }

  nsAutoString libraryGUID;
  nsresult rv = aLibrary->GetGuid(libraryGUID);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG
  PRBool exists = mLibraryTable.Get(libraryGUID, nsnull);
  if (exists) {
    NS_WARNING("Registering a library that has already been registered!");
  }
#endif
  PRBool success = mLibraryTable.Put(libraryGUID, aLibrary);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  if (mDataSource) {
    rv = AssertLibrary(mDataSource, aLibrary);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to update dataSource!");
  }

  return NS_OK;
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::UnregisterLibrary(sbILibrary* aLibrary)
{
  TRACE(("LibraryManager[0x%x] - UnregisterLibrary", this));
  NS_ENSURE_ARG_POINTER(aLibrary);

  NS_ENSURE_TRUE(mLibraryTable.IsInitialized(), NS_ERROR_UNEXPECTED);

  nsAutoString libraryGUID;
  nsresult rv = aLibrary->GetGuid(libraryGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists = mLibraryTable.Get(libraryGUID, nsnull);
  if (!exists) {
    NS_WARNING("Unregistering a library that was never registered!");
    return NS_OK;
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
sbLibraryManager::RegisterLibraryFactory(sbILibraryFactory* aLibraryFactory,
                                         const nsAString& aType)
{
  TRACE(("LibraryManager[0x%x] - RegisterLibraryFactory", this));
  NS_ENSURE_ARG_POINTER(aLibraryFactory);
  NS_ENSURE_TRUE(!aType.IsEmpty(), NS_ERROR_INVALID_ARG);

  if (!mFactoryTable.IsInitialized()) {
    PRBool success = mFactoryTable.Init();
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  }

#ifdef DEBUG
  PRBool exists = mFactoryTable.Get(aType, nsnull);
  if (exists) {
    NS_WARNING("Registering a factory that has already been registered!");
  }
#endif
  PRBool success = mFactoryTable.Put(aType, aLibraryFactory);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  return NS_OK;
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::GetLibraryEnumerator(nsISimpleEnumerator** _retval)
{
  TRACE(("LibraryManager[0x%x] - GetLibraryEnumerator", this));
  NS_ENSURE_ARG_POINTER(_retval);

  if (!mLibraryTable.IsInitialized()) {
    return NS_NewEmptyEnumerator(_retval);
  }

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
sbLibraryManager::GetLibraryFactoryEnumerator(nsISimpleEnumerator** _retval)
{
  TRACE(("LibraryManager[0x%x] - GetLibraryFactoryEnumerator", this));
  NS_ENSURE_ARG_POINTER(_retval);

  if (!mFactoryTable.IsInitialized()) {
    return NS_NewEmptyEnumerator(_retval);
  }

  PRUint32 factoryCount = mFactoryTable.Count();

  if (!factoryCount) {
    return NS_NewEmptyEnumerator(_retval);
  }

  nsCOMArray<sbILibraryFactory> factoryArray(factoryCount);

  PRUint32 enumCount =
    mFactoryTable.EnumerateRead(AddFactoriesToCOMArrayCallback, &factoryArray);
  NS_ENSURE_TRUE(enumCount == factoryCount, NS_ERROR_FAILURE);

  return NS_NewArrayEnumerator(_retval, factoryArray);
}

/**
 * See sbILibraryManager.idl
 */
NS_IMETHODIMP
sbLibraryManager::GetDataSource(nsIRDFDataSource** aDataSource)
{
  TRACE(("LibraryManager[0x%x] - GetDataSource", this));
  NS_ENSURE_ARG_POINTER(aDataSource);

  nsresult rv;
  if (!mDataSource) {
    rv = GenerateDataSource();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF(*aDataSource = mDataSource);
  return NS_OK;
}
