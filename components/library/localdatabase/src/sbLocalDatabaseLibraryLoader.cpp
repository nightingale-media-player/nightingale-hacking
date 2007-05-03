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

#include "sbLocalDatabaseLibraryLoader.h"

#include <nsICategoryManager.h>
#include <nsIFile.h>
#include <nsIGenericFactory.h>
#include <nsILocalFile.h>
#include <nsIPrefService.h>
#include <nsIPropertyBag2.h>
#include <nsISupportsPrimitives.h>
#include <sbILibrary.h>

#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsTHashtable.h>
#include <nsXPCOMCID.h>
#include <prlog.h>
#include <sbLibraryManager.h>
#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseLibraryFactory.h"

#define PROPERTY_KEY_DATABASEFILE "databaseFile"

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbLocalDatabaseLibraryLoader:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* sLibraryLoaderLog = nsnull;
#define TRACE(args) PR_LOG(sLibraryLoaderLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(sLibraryLoaderLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

#define NS_APPSTARTUP_CATEGORY         "app-startup"

#define PREFBRANCH_LOADER SB_PREFBRANCH_LIBRARY "loader."

// These are on PREFBRANCH_LOADER.[index]
#define PREF_DATABASE_GUID     "databaseGUID"
#define PREF_DATABASE_LOCATION "databaseLocation"
#define PREF_LOAD_AT_STARTUP   "loadAtStartup"

#define MINIMUM_LIBRARY_COUNT 3

template <class T>
class sbAutoFreeXPCOMArray
{
public:
  sbAutoFreeXPCOMArray(PRUint32 aCount, T aArray)
  : mCount(aCount),
    mArray(aArray)
  {
    if (aCount) {
      NS_ASSERTION(aArray, "Null pointer!");
    }
  }

  ~sbAutoFreeXPCOMArray()
  {
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(mCount, mArray);
  }

private:
  PRUint32 mCount;
  T mArray;
};

NS_IMPL_ISUPPORTS1(sbLocalDatabaseLibraryLoader, sbILibraryLoader)

sbLocalDatabaseLibraryLoader::sbLocalDatabaseLibraryLoader()
{
#ifdef PR_LOGGING
  if (!sLibraryLoaderLog)
    sLibraryLoaderLog = PR_NewLogModule("sbLocalDatabaseLibraryLoader");
#endif
  TRACE(("sbLocalDatabaseLibraryLoader[0x%x] - Created", this));
}

sbLocalDatabaseLibraryLoader::~sbLocalDatabaseLibraryLoader()
{
  TRACE(("sbLocalDatabaseLibraryLoader[0x%x] - Destroyed", this));
}

/**
 * \brief
 */
nsresult
sbLocalDatabaseLibraryLoader::Init()
{
  TRACE(("sbLocalDatabaseLibraryLoader[0x%x] - Init", this));

  nsresult rv;
  nsCOMPtr<nsIPrefService> prefService =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mRootBranch = do_QueryInterface(prefService, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 libraryKeysCount;
  char** libraryKeys;

  rv = mRootBranch->GetChildList(PREFBRANCH_LOADER, &libraryKeysCount,
                                 &libraryKeys);
  NS_ENSURE_SUCCESS(rv, rv);

  sbAutoFreeXPCOMArray<char**> autoFree(libraryKeysCount, libraryKeys);

  PRBool success = mLibraryInfoTable.Init(MINIMUM_LIBRARY_COUNT);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  for (PRUint32 index = 0; index < libraryKeysCount; index++) {
    // Should be something like "songbird.library.loader.2.loadAtStartup".
    nsCAutoString pref(libraryKeys[index]);
    NS_ASSERTION(StringBeginsWith(pref, NS_LITERAL_CSTRING(PREFBRANCH_LOADER)),
                 "Bad pref string!");

    PRUint32 branchLength = NS_LITERAL_CSTRING(PREFBRANCH_LOADER).Length();

    PRInt32 firstDotIndex = pref.FindChar('.', branchLength);
    NS_ASSERTION(firstDotIndex != -1, "Bad pref string!");

    PRUint32 keyLength = firstDotIndex - branchLength;
    NS_ASSERTION(keyLength > 0, "Bad pref string!");

    // Should be something like "1".
    nsCAutoString keyString(Substring(pref, branchLength, keyLength));
    PRUint32 libraryKey = keyString.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Should be something like "songbird.library.loader.13.".
    nsCAutoString branchString(Substring(pref, 0, branchLength + keyLength + 1));
    NS_ASSERTION(StringEndsWith(branchString, NS_LITERAL_CSTRING(".")),
                 "Bad pref string!");

    if (!mLibraryInfoTable.Get(libraryKey, nsnull)) {
      nsAutoPtr<sbLibraryLoaderInfo> newLibraryInfo(new sbLibraryLoaderInfo());
      NS_ENSURE_TRUE(newLibraryInfo, NS_ERROR_OUT_OF_MEMORY);

      rv = newLibraryInfo->Init(branchString);
      NS_ENSURE_SUCCESS(rv, rv);

      success = mLibraryInfoTable.Put(libraryKey, newLibraryInfo);
      NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

      newLibraryInfo.forget();
    }
  }

  mLibraryInfoTable.Enumerate(VerifyEntriesCallback, nsnull);

  return NS_OK;
}

nsresult
sbLocalDatabaseLibraryLoader::EnsureDefaultLibraries()
{
  nsresult rv = EnsureDefaultLibrary(NS_LITERAL_CSTRING(SB_PREF_MAIN_LIBRARY),
                                     NS_LITERAL_STRING(SB_GUID_MAIN_LIBRARY));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = EnsureDefaultLibrary(NS_LITERAL_CSTRING(SB_PREF_WEB_LIBRARY),
                            NS_LITERAL_STRING(SB_GUID_WEB_LIBRARY));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = EnsureDefaultLibrary(NS_LITERAL_CSTRING(SB_PREF_DOWNLOAD_LIBRARY),
                            NS_LITERAL_STRING(SB_GUID_DOWNLOAD_LIBRARY));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseLibraryLoader::EnsureDefaultLibrary(const nsACString& aLibraryGUIDPref,
                                                   const nsAString& aDefaultGUID)
{
  nsCAutoString prefKey(aLibraryGUIDPref);

  // Figure out the GUID for this library.
  nsAutoString databaseGUID;
  nsCOMPtr<nsISupportsString> supportsString;
  nsresult rv = mRootBranch->GetComplexValue(prefKey.get(),
                                             NS_GET_IID(nsISupportsString),
                                             getter_AddRefs(supportsString));
  if (NS_SUCCEEDED(rv)) {
    // Use the value stored in the prefs.
    rv = supportsString->GetData(databaseGUID);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    // Use the value passed in.
    supportsString = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = supportsString->SetData(aDefaultGUID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mRootBranch->SetComplexValue(prefKey.get(),
                                      NS_GET_IID(nsISupportsString),
                                      supportsString);
    NS_ENSURE_SUCCESS(rv, rv);

    databaseGUID.Assign(aDefaultGUID);
  }

  // See if this library already exists in the hashtable.
  sbLibraryExistsInfo existsInfo(databaseGUID);
  mLibraryInfoTable.EnumerateRead(LibraryExistsCallback, &existsInfo);

  sbLibraryLoaderInfo* libraryInfo;
  if ((existsInfo.index == -1) ||
      (!mLibraryInfoTable.Get(existsInfo.index, &libraryInfo))) {
    // The library wasn't in our hashtable, so make a new sbLibraryLoaderInfo
    // object. That will take care of setting the prefs up correctly.
    PRUint32 index = GetNextLibraryIndex();

    prefKey.AssignLiteral(PREFBRANCH_LOADER);
    prefKey.AppendInt(index);
    prefKey.AppendLiteral(".");

    nsAutoPtr<sbLibraryLoaderInfo>
      newLibraryInfo(CreateDefaultLibraryInfo(prefKey, databaseGUID));
    if (!newLibraryInfo || !mLibraryInfoTable.Put(index, newLibraryInfo)) {
      return NS_ERROR_FAILURE;
    }

    libraryInfo = newLibraryInfo.forget();
  }

#ifdef DEBUG
  nsAutoString guid;
  libraryInfo->GetDatabaseGUID(guid);
  NS_ASSERTION(!guid.IsEmpty(), "Empty GUID!");
#endif

  // Make sure this library loads at startup.
  if (!libraryInfo->GetLoadAtStartup()) {
    rv = libraryInfo->SetLoadAtStartup(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // And make sure that the database file actually exists and is accessible.
  nsCOMPtr<nsILocalFile> location = libraryInfo->GetDatabaseLocation();
  NS_ENSURE_TRUE(location, NS_ERROR_UNEXPECTED);

  sbLocalDatabaseLibraryFactory libraryFactory;

  nsCOMPtr<sbILibrary> library;
  rv = libraryFactory.CreateLibraryFromDatabase(location,
                                                getter_AddRefs(library));
  if (NS_FAILED(rv)) {
    // We can't access this required database file. For now we're going to
    // simply make a new blank database in the default location and switch 
    // the preferences to use it.
    location = libraryFactory.GetFileForGUID(databaseGUID);
    NS_ENSURE_TRUE(location, NS_ERROR_FAILURE);

    // Make sure we can access this one.
    rv = libraryFactory.CreateLibraryFromDatabase(location,
                                                  getter_AddRefs(library));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = libraryInfo->SetDatabaseLocation(location);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

/**
 * \brief
 */
sbLibraryLoaderInfo*
sbLocalDatabaseLibraryLoader::CreateDefaultLibraryInfo(const nsACString& aPrefKey,
                                                       const nsAString& aGUID)
{
  nsAutoPtr<sbLibraryLoaderInfo> newLibraryInfo(new sbLibraryLoaderInfo());
  NS_ENSURE_TRUE(newLibraryInfo, nsnull);

  nsresult rv = newLibraryInfo->Init(aPrefKey);
  NS_ENSURE_SUCCESS(rv, nsnull);

  rv = newLibraryInfo->SetDatabaseGUID(aGUID);
  NS_ENSURE_SUCCESS(rv, nsnull);

  sbLocalDatabaseLibraryFactory libraryFactory;
  nsCOMPtr<nsILocalFile> location = libraryFactory.GetFileForGUID(aGUID);
  NS_ENSURE_TRUE(location, nsnull);

  rv = newLibraryInfo->SetDatabaseLocation(location);
  NS_ENSURE_SUCCESS(rv, nsnull);

  rv = newLibraryInfo->SetLoadAtStartup(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsCOMPtr<nsIPrefService> prefService = do_QueryInterface(mRootBranch, &rv);
  NS_ENSURE_SUCCESS(rv, nsnull);

  rv = prefService->SavePrefFile(nsnull);
  NS_ENSURE_SUCCESS(rv, nsnull);

  return newLibraryInfo.forget();
}

/* static */ void
sbLocalDatabaseLibraryLoader::RemovePrefBranch(const nsACString& aPrefBranch)
{
  nsresult rv;
  nsCOMPtr<nsIPrefService> prefService =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,);

  nsCAutoString prefBranch(aPrefBranch);

  nsCOMPtr<nsIPrefBranch> doomedBranch;
  rv = prefService->GetBranch(prefBranch.get(), getter_AddRefs(doomedBranch));
  NS_ENSURE_SUCCESS(rv,);

  rv = doomedBranch->DeleteBranch("");
  NS_ENSURE_SUCCESS(rv,);

  rv = prefService->SavePrefFile(nsnull);
  NS_ENSURE_SUCCESS(rv,);
}

PRUint32
sbLocalDatabaseLibraryLoader::GetNextLibraryIndex()
{
  for (PRUint32 index = 0; ; index++) {
    if (!mLibraryInfoTable.Get(index, nsnull)) {
      return index;
    }
  }
  NS_NOTREACHED("Shouldn't be here!");
  return 0;
}

/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseLibraryLoader::LoadLibrariesCallback(nsUint32HashKey::KeyType aKey,
                                                    sbLibraryLoaderInfo* aEntry,
                                                    void* aUserData)
{
  sbLoaderInfo* loaderInfo = NS_STATIC_CAST(sbLoaderInfo*, aUserData);
  NS_ASSERTION(loaderInfo, "Doh, how did this happen?!");

  if (!aEntry->GetLoadAtStartup()) {
    return PL_DHASH_NEXT;
  }

  nsCOMPtr<nsILocalFile> databaseFile = aEntry->GetDatabaseLocation();
  NS_ASSERTION(databaseFile, "Must have a file here!");

  nsCOMPtr<sbILibrary> library;
  nsresult rv =
    loaderInfo->libraryFactory->CreateLibraryFromDatabase(databaseFile,
                                                          getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, PL_DHASH_NEXT);
  
  rv = loaderInfo->libraryManager->RegisterLibrary(library, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_NEXT);

  return PL_DHASH_NEXT;
}

/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseLibraryLoader::LibraryExistsCallback(nsUint32HashKey::KeyType aKey,
                                                    sbLibraryLoaderInfo* aEntry,
                                                    void* aUserData)
{
  sbLibraryExistsInfo* existsInfo =
    NS_STATIC_CAST(sbLibraryExistsInfo*, aUserData);
  NS_ASSERTION(existsInfo, "Doh, how did this happen?!");

  nsAutoString databaseGUID;
  aEntry->GetDatabaseGUID(databaseGUID);
  NS_ASSERTION(!databaseGUID.IsEmpty(), "GUID can't be empty here!");

  if (databaseGUID.Equals(existsInfo->databaseGUID)) {
    existsInfo->index = (PRInt32)aKey;
    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseLibraryLoader::VerifyEntriesCallback(nsUint32HashKey::KeyType aKey,
                                                    nsAutoPtr<sbLibraryLoaderInfo>& aEntry,
                                                    void* aUserData)
{
  nsCAutoString prefBranch;
  aEntry->GetPrefBranch(prefBranch);
  NS_ASSERTION(!prefBranch.IsEmpty(), "This can't be empty!");

  nsAutoString databaseGUID;
  aEntry->GetDatabaseGUID(databaseGUID);
  if (databaseGUID.IsEmpty()) {
    NS_WARNING("One of the libraries was missing a GUID and will be removed.");
    RemovePrefBranch(prefBranch);
    return PL_DHASH_REMOVE;
  }

  nsCOMPtr<nsILocalFile> location = aEntry->GetDatabaseLocation();
  if (!location) {
    NS_WARNING("One of the libraries had no location and will be removed.");
    RemovePrefBranch(prefBranch);
    return PL_DHASH_REMOVE;
  }

  return PL_DHASH_NEXT;
}

/**
 * \brief Load all of the libraries we need for startup.
 */
NS_IMETHODIMP
sbLocalDatabaseLibraryLoader::OnRegisterStartupLibraries(sbILibraryManager* aLibraryManager)
{
  TRACE(("sbLocalDatabaseLibraryLoader[0x%x] - LoadLibraries", this));

  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = EnsureDefaultLibraries();
  NS_ENSURE_SUCCESS(rv, rv);

  sbLocalDatabaseLibraryFactory libraryFactory;
  sbLoaderInfo info(aLibraryManager, &libraryFactory);

  PRUint32 enumeratedLibraries = 
    mLibraryInfoTable.EnumerateRead(LoadLibrariesCallback, &info);
  NS_ASSERTION(enumeratedLibraries >= 3, "Too few libraries enumerated!");

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseLibraryLoader::OnLibraryStartupModified(sbILibrary* aLibrary,
                                                       PRBool aLoadAtStartup)
{
  NS_ENSURE_ARG_POINTER(aLibrary);

  // See if we support this library type.
  nsCOMPtr<sbILibraryFactory> factory;
  nsresult rv = aLibrary->GetFactory(getter_AddRefs(factory));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString factoryType;
  rv = factory->GetType(factoryType);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(factoryType.EqualsLiteral(SB_LOCALDATABASE_LIBRARYFACTORY_TYPE),
                 NS_ERROR_NOT_AVAILABLE);

  // See if this library already exists in the hashtable.
  nsAutoString databaseGUID;
  rv = aLibrary->GetGuid(databaseGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  sbLibraryExistsInfo existsInfo(databaseGUID);
  mLibraryInfoTable.EnumerateRead(LibraryExistsCallback, &existsInfo);

  sbLibraryLoaderInfo* libraryInfo;
  if ((existsInfo.index == -1) ||
      (!mLibraryInfoTable.Get(existsInfo.index, &libraryInfo))) {

    // The library wasn't in the hashtable so make sure that we can recreate
    // it.
    nsCOMPtr<nsIPropertyBag2> creationParameters;
    rv = aLibrary->GetCreationParameters(getter_AddRefs(creationParameters));
    NS_ENSURE_SUCCESS(rv, rv);

    NS_NAMED_LITERAL_STRING(fileKey, PROPERTY_KEY_DATABASEFILE);
    nsCOMPtr<nsILocalFile> databaseFile;
    rv = creationParameters->GetPropertyAsInterface(fileKey,
                                                    NS_GET_IID(nsILocalFile),
                                                    getter_AddRefs(databaseFile));
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ASSERTION(databaseFile, "This can't be null!");

    // Make a new sbLibraryLoaderInfo object.
    PRUint32 index = GetNextLibraryIndex();

    nsCAutoString prefKey(PREFBRANCH_LOADER);
    prefKey.AppendInt(index);
    prefKey.AppendLiteral(".");

    nsAutoPtr<sbLibraryLoaderInfo>
      newLibraryInfo(CreateDefaultLibraryInfo(prefKey, databaseGUID));
    if (!newLibraryInfo || !mLibraryInfoTable.Put(index, newLibraryInfo)) {
      return NS_ERROR_FAILURE;
    }

    rv = newLibraryInfo->SetDatabaseLocation(databaseFile);
    NS_ENSURE_SUCCESS(rv, rv);

    libraryInfo = newLibraryInfo.forget();
  }

  rv = libraryInfo->SetLoadAtStartup(aLoadAtStartup);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * sbLibraryLoaderInfo implementation
 */
nsresult
sbLibraryLoaderInfo::Init(const nsACString& aPrefKey)
{
  nsresult rv;
  nsCOMPtr<nsIPrefService> prefService =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString prefBranchString(aPrefKey);
  rv = prefService->GetBranch(prefBranchString.get(),
                              getter_AddRefs(mPrefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  mGUIDKey.Assign(PREF_DATABASE_GUID);
  mLocationKey.Assign(PREF_DATABASE_LOCATION);
  mStartupKey.Assign(PREF_LOAD_AT_STARTUP);

  // Now ensure that the key exists.
  PRBool exists;
  rv = mPrefBranch->PrefHasUserValue(mStartupKey.get(), &exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!exists) {
    rv = SetLoadAtStartup(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbLibraryLoaderInfo::SetDatabaseGUID(const nsAString& aGUID)
{
  NS_ENSURE_FALSE(aGUID.IsEmpty(), NS_ERROR_INVALID_ARG);

  nsresult rv;
  nsCOMPtr<nsISupportsString> supportsString =
    do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = supportsString->SetData(aGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPrefBranch->SetComplexValue(mGUIDKey.get(),
                                    NS_GET_IID(nsISupportsString),
                                    supportsString);

  return NS_OK;
}

void
sbLibraryLoaderInfo::GetDatabaseGUID(nsAString& _retval)
{
  _retval.Truncate();

  nsCOMPtr<nsISupportsString> supportsString;
  nsresult rv = mPrefBranch->GetComplexValue(mGUIDKey.get(),
                                             NS_GET_IID(nsISupportsString),
                                             getter_AddRefs(supportsString));
  NS_ENSURE_SUCCESS(rv,);

  rv = supportsString->GetData(_retval);
  NS_ENSURE_SUCCESS(rv,);
}

nsresult
sbLibraryLoaderInfo::SetDatabaseLocation(nsILocalFile* aLocation)
{
  NS_ENSURE_ARG_POINTER(aLocation);

  nsresult rv;
  nsCOMPtr<nsIFile> file = do_QueryInterface(aLocation, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString filePath;
  rv = file->GetNativePath(filePath);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPrefBranch->SetCharPref(mLocationKey.get(), filePath.get());
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

already_AddRefed<nsILocalFile>
sbLibraryLoaderInfo::GetDatabaseLocation()
{
  nsresult rv;
  nsCOMPtr<nsILocalFile> location = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsCAutoString filePath;
  rv = mPrefBranch->GetCharPref(mLocationKey.get(), getter_Copies(filePath));
  NS_ENSURE_SUCCESS(rv, nsnull);

  rv = location->InitWithNativePath(filePath);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsILocalFile* _retval;
  NS_ADDREF(_retval = location);
  return _retval;
}

nsresult
sbLibraryLoaderInfo::SetLoadAtStartup(PRBool aLoadAtStartup)
{
  nsresult rv = mPrefBranch->SetBoolPref(mStartupKey.get(), aLoadAtStartup);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

PRBool
sbLibraryLoaderInfo::GetLoadAtStartup()
{
  PRBool loadAtStartup;
  nsresult rv = mPrefBranch->GetBoolPref(mStartupKey.get(), &loadAtStartup);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  return loadAtStartup;
}

void
sbLibraryLoaderInfo::GetPrefBranch(nsACString& _retval)
{
  _retval.Truncate();

  nsCAutoString prefBranch;
  nsresult rv = mPrefBranch->GetRoot(getter_Copies(prefBranch));
  NS_ENSURE_SUCCESS(rv,);

  _retval.Assign(prefBranch);
}
