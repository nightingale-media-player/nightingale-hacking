/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

/* base class */
#include "sbDeviceLibrary.h"

/* mozilla interfaces */
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsIFileURL.h>
#include <nsIPrefService.h>
#include <nsIVariant.h>
#include <nsIWritablePropertyBag2.h>

/* nspr headers */
#include <prlog.h>
#include <prprf.h>
#include <prtime.h>

/* mozilla headers */
#include <nsAppDirectoryServiceDefs.h>
#include <nsArrayUtils.h>
#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsCOMArray.h>
#include <nsComponentManagerUtils.h>
#include <nsDirectoryServiceUtils.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsXPCOM.h>
#include <nsXPCOMCIDInternal.h>

/* songbird interfaces */
#include <sbIDevice.h>
#include <sbIDeviceContent.h>
#include <sbIDeviceEvent.h>
#include <sbIDeviceEventTarget.h>
#include <sbIDeviceManager.h>
#include <sbILibraryFactory.h>
#include <sbILibraryManager.h>
#include <sbIPrompter.h>
#include <sbIPropertyArray.h>

/* songbird headers */
#include <sbDeviceLibraryHelpers.h>
#include <sbDeviceUtils.h>
#include <sbLibraryUtils.h>
#include <sbLocalDatabaseCID.h>
#include <sbMemoryUtils.h>
#include <sbPropertiesCID.h>
#include <sbProxyUtils.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>
#include <sbVariantUtils.h>


/* List of properties for which to sync updates. */
const static char* sbDeviceLibrarySyncUpdatePropertyTable[] =
{
  SB_PROPERTY_CONTENTTYPE,
  SB_PROPERTY_CONTENTLENGTH,
  SB_PROPERTY_TRACKNAME,
  SB_PROPERTY_ALBUMNAME,
  SB_PROPERTY_ARTISTNAME,
  SB_PROPERTY_DURATION,
  SB_PROPERTY_HIDDEN,
  SB_PROPERTY_GENRE,
  SB_PROPERTY_TRACKNUMBER,
  SB_PROPERTY_YEAR,
  SB_PROPERTY_DISCNUMBER,
  SB_PROPERTY_TOTALDISCS,
  SB_PROPERTY_TOTALTRACKS,
  SB_PROPERTY_ISPARTOFCOMPILATION,
  SB_PROPERTY_PRODUCERNAME,
  SB_PROPERTY_COMPOSERNAME,
  SB_PROPERTY_CONDUCTORNAME,
  SB_PROPERTY_LYRICISTNAME,
  SB_PROPERTY_LYRICS,
  SB_PROPERTY_RECORDLABELNAME,
  SB_PROPERTY_LASTPLAYTIME,
  SB_PROPERTY_PLAYCOUNT,
  SB_PROPERTY_LASTSKIPTIME,
  SB_PROPERTY_SKIPCOUNT,
  SB_PROPERTY_RATING,
  SB_PROPERTY_BITRATE,
  SB_PROPERTY_SAMPLERATE,
  SB_PROPERTY_BPM,
  SB_PROPERTY_KEY,
  SB_PROPERTY_LANGUAGE,
  SB_PROPERTY_COMMENT,
  SB_PROPERTY_COPYRIGHT,
  SB_PROPERTY_COPYRIGHTURL,
  SB_PROPERTY_SUBTITLE,
  SB_PROPERTY_SOFTWAREVENDOR,
  SB_PROPERTY_MEDIALISTNAME,
  SB_PROPERTY_ALBUMARTISTNAME
};

NS_IMPL_THREADSAFE_ADDREF(sbDeviceLibrary)
NS_IMPL_THREADSAFE_RELEASE(sbDeviceLibrary)

NS_INTERFACE_MAP_BEGIN(sbDeviceLibrary)
  NS_IMPL_QUERY_CLASSINFO(sbDeviceLibrary)
  NS_INTERFACE_MAP_ENTRY(sbIDeviceLibrary)
  NS_INTERFACE_MAP_ENTRY(sbILibrary)
  NS_INTERFACE_MAP_ENTRY(sbIMediaList)
  NS_INTERFACE_MAP_ENTRY(sbIMediaItem)
  NS_INTERFACE_MAP_ENTRY(sbILibraryResource)
  NS_INTERFACE_MAP_ENTRY(sbIMediaListListener)
  NS_INTERFACE_MAP_ENTRY(sbILocalDatabaseMediaListCopyListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, sbIDeviceLibrary)
NS_INTERFACE_MAP_END

NS_IMPL_CI_INTERFACE_GETTER6(sbDeviceLibrary,
                             nsIClassInfo,
                             sbIDeviceLibrary,
                             sbILibrary,
                             sbIMediaList,
                             sbIMediaItem,
                             sbILibraryResource)

NS_DECL_CLASSINFO(sbDeviceLibrary)
NS_IMPL_THREADSAFE_CI(sbDeviceLibrary)

#define PREF_SYNC_PREFIX    "library."
#define PREF_SYNC_BRANCH    ".sync."
#define PREF_SYNC_MGMTTYPE  "mgmtType"
#define PREF_SYNC_LISTS     "playlists"

const static char* gMediaType[] = {
  ".audio",
  ".video" 
};

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbDeviceLibrary:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gDeviceLibraryLog = nsnull;
#define TRACE(args) PR_LOG(gDeviceLibraryLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gDeviceLibraryLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

sbDeviceLibrary::sbDeviceLibrary(sbIDevice* aDevice)
  : mDevice(aDevice),
    mLock(nsnull)
{
#ifdef PR_LOGGING
  if (!gDeviceLibraryLog) {
    gDeviceLibraryLog = PR_NewLogModule("sbDeviceLibrary");
  }
#endif
  TRACE(("DeviceLibrary[0x%.8x] - Constructed", this));
}

sbDeviceLibrary::~sbDeviceLibrary()
{
  Finalize();

  if(mLock) {
    nsAutoLock::DestroyLock(mLock);
  }

  TRACE(("DeviceLibrary[0x%.8x] - Destructed", this));
}

NS_IMETHODIMP
sbDeviceLibrary::Initialize(const nsAString& aLibraryId)
{
  NS_ENSURE_FALSE(mLock, NS_ERROR_ALREADY_INITIALIZED);
  mLock = nsAutoLock::NewLock(__FILE__ "sbDeviceLibrary::mLock");
  NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);
  PRBool succeeded = mListeners.Init();
  NS_ENSURE_TRUE(succeeded, NS_ERROR_OUT_OF_MEMORY);
  return CreateDeviceLibrary(aLibraryId, nsnull);
}

NS_IMETHODIMP
sbDeviceLibrary::Finalize()
{
  nsresult rv;

  // unhook the main library listener
  if (mMainLibraryListener) {
    nsCOMPtr<sbILibrary> mainLib;
    rv = GetMainLibrary(getter_AddRefs(mainLib));
    NS_ASSERTION(NS_SUCCEEDED(rv), "WTF this should never fail");
    // if we fail, meh, too bad, we still need to get on with the releasing
    if (NS_SUCCEEDED(rv)) {
      rv = mainLib->RemoveListener(mMainLibraryListener);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "failed to remove main lib listener");
    }
    mMainLibraryListener = nsnull;
  }

  // remove the device event listener
  nsCOMPtr<sbIDeviceEventTarget>
    deviceEventTarget = do_QueryInterface(mDevice, &rv);
  if (NS_SUCCEEDED(rv))
    deviceEventTarget->RemoveEventListener(this);

  if (mDeviceLibrary)
    UnregisterDeviceLibrary();

  // Get and clear the device library.
  nsCOMPtr<sbILibrary> deviceLibrary;
  {
    nsAutoLock lock(mLock);
    deviceLibrary = mDeviceLibrary;
    mDeviceLibrary = nsnull;
    // remove the listeners
    mListeners.Clear();
  }

  // let go of the owner device
  mDevice = nsnull;

  return NS_OK;
}

/**
 * Enumerator class used to attach listeners to playlists
 */
class sbPlaylistAttachListenerEnumerator : public sbIMediaListEnumerationListener
{
public:
  sbPlaylistAttachListenerEnumerator(sbLibraryUpdateListener* aListener)
   : mListener(aListener)
   {}
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER
private:
  sbLibraryUpdateListener* mListener;
};

NS_IMPL_ISUPPORTS1(sbPlaylistAttachListenerEnumerator,
                   sbIMediaListEnumerationListener)

NS_IMETHODIMP sbPlaylistAttachListenerEnumerator::OnEnumerationBegin(sbIMediaList*,
                                                                     PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

NS_IMETHODIMP sbPlaylistAttachListenerEnumerator::OnEnumeratedItem(sbIMediaList*,
                                                                   sbIMediaItem* aItem,
                                                                   PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mListener, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;

  // check if the list has a custom type
  nsString customType;
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CUSTOMTYPE),
                          customType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (customType.IsEmpty() ||
      customType.EqualsLiteral("simple")) {
    nsCOMPtr<sbIMediaList> list(do_QueryInterface(aItem, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mListener->ListenToPlaylist(list);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}

NS_IMETHODIMP sbPlaylistAttachListenerEnumerator::OnEnumerationEnd(sbIMediaList*,
                                                                   nsresult)
{
  return NS_OK;
}

nsresult
sbDeviceLibrary::SetMgmtTypePref(PRUint32 aContentType, PRUint32* aMgmtTypes)
{
  NS_ENSURE_ARG_POINTER(aMgmtTypes);
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);

  nsresult rv;

  // Get the preference key.
  nsString prefKey;
  rv = GetMgmtTypePrefKey(aContentType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the preference.
  rv = mDevice->SetPreference(prefKey, sbNewVariant(*aMgmtTypes));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceLibrary::SetSyncPlaylistListPref(PRUint32 aContentType,
                                         nsIArray *aPlaylistList)
{
  NS_ENSURE_ARG_POINTER(aPlaylistList);
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);

  nsresult rv;

  // Get the number of sync playlists
  PRUint32 length;
  rv = aPlaylistList->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  // Produce the sync playlist guid list CSV
  nsAutoString listGuidsCSV;
  for (PRUint32 i = 0; i < length; i++) {
    // Get the next sync playlist media list
    nsCOMPtr<sbIMediaList> mediaList = do_QueryElementAt(aPlaylistList, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the guid of the list
    nsAutoString guid;
    rv = mediaList->GetGuid(guid);
    NS_ENSURE_SUCCESS(rv, rv);

    // Skip the list if it's already present
    if (listGuidsCSV.Find(guid) >= 0)
      continue;

    // Add the guid to the list of sync playlist guids
    if (i > 0)
      listGuidsCSV.AppendLiteral(",");
    listGuidsCSV.Append(guid);
  }

  // Get the preference key
  nsString prefKey;
  rv = GetSyncListsPrefKey(aContentType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  // Write the preference
  rv = mDevice->SetPreference(prefKey, sbNewVariant(listGuidsCSV));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceLibrary::CreateDeviceLibrary(const nsAString &aDeviceIdentifier,
                                     nsIURI *aDeviceDatabaseURI)
{
  nsresult rv;
  nsCOMPtr<sbILibraryFactory> libraryFactory =
    do_GetService(SB_LOCALDATABASE_LIBRARYFACTORY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWritablePropertyBag2> libraryProps =
    do_CreateInstance(NS_HASH_PROPERTY_BAG_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> libraryFile;
  if(aDeviceDatabaseURI) {
    //Use preferred database location.
    nsCOMPtr<nsIFileURL> furl = do_QueryInterface(aDeviceDatabaseURI, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = furl->GetFile(getter_AddRefs(libraryFile));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    //No preferred database location, fetch default location.
    rv = GetDefaultDeviceLibraryDatabaseFile(aDeviceIdentifier,
                                             getter_AddRefs(libraryFile));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = libraryProps->SetPropertyAsInterface(NS_LITERAL_STRING("databaseFile"),
                                            libraryFile);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef PR_LOGGING
  {
    nsCAutoString str;
    if(NS_SUCCEEDED(libraryFile->GetNativePath(str))) {
      LOG(("Attempting to create device library with file path %s", str.get()));
    }
  }
#endif

  rv = libraryFactory->CreateLibrary(libraryProps,
                                     getter_AddRefs(mDeviceLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  // Store our guid in the inner device library
  nsString deviceLibraryGuid;
  rv = this->GetGuid(deviceLibraryGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDeviceLibrary->SetProperty
                              (NS_LITERAL_STRING(SB_PROPERTY_DEVICE_LIBRARY_GUID),
                              deviceLibraryGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Don't allow writing metadata back to device media files.  Let the device
  // component handle property updates.
  rv = this->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_DONT_WRITE_METADATA),
                         NS_LITERAL_STRING("1"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> list;
  list = do_QueryInterface(mDeviceLibrary, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // hook up listeners
  rv = list->AddListener(this,
                         PR_FALSE,
                         sbIMediaList::LISTENER_FLAGS_ITEMADDED |
                         sbIMediaList::LISTENER_FLAGS_AFTERITEMREMOVED |
                         sbIMediaList::LISTENER_FLAGS_ITEMUPDATED |
                         sbIMediaList::LISTENER_FLAGS_BEFORELISTCLEARED |
                         sbIMediaList::LISTENER_FLAGS_BATCHBEGIN |
                         sbIMediaList::LISTENER_FLAGS_BATCHEND,
                         nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> mainLib;
  rv = GetMainLibrary(getter_AddRefs(mainLib));
  NS_ENSURE_SUCCESS(rv, rv);

  // get the current is synced locally state
  rv = GetIsSyncedLocally(&mLastIsSyncedLocally);
  NS_ENSURE_SUCCESS(rv, rv);

  // add a device event listener to listen for changes to the is synced locally
  // state
  nsCOMPtr<sbIDeviceEventTarget>
    deviceEventTarget = do_QueryInterface(mDevice, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deviceEventTarget->AddEventListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isManual = PR_FALSE;
  rv = GetIsMgmtTypeManual(&isManual);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isSyncPlaylist = PR_FALSE;
  rv = GetIsMgmtTypeSyncList(&isSyncPlaylist);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> syncPlaylists;
  if (isSyncPlaylist) {
    rv = GetSyncPlaylistList(getter_AddRefs(syncPlaylists));
    NS_ENSURE_SUCCESS(rv,rv);
  }
  // Check to see if playlists are supported, and if not ignore
  bool const ignorePlaylists = !sbDeviceUtils::ArePlaylistsSupported(mDevice);
  // hook up the listener now if we need to
  mMainLibraryListener =
    new sbLibraryUpdateListener(mDeviceLibrary,
                                isManual,
                                isSyncPlaylist,
                                syncPlaylists,
                                ignorePlaylists);
  NS_ENSURE_TRUE(mMainLibraryListener, NS_ERROR_OUT_OF_MEMORY);

  mMainLibraryListenerFilter = do_CreateInstance
                                 (SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsAutoString voidString;
  voidString.SetIsVoid(PR_TRUE);
  nsAutoString propertyID;
  PRUint32 propertyCount =
             NS_ARRAY_LENGTH(sbDeviceLibrarySyncUpdatePropertyTable);
  for (PRUint32 i = 0; i < propertyCount; i++) {
    propertyID.AssignLiteral(sbDeviceLibrarySyncUpdatePropertyTable[i]);
    rv = mMainLibraryListenerFilter->AppendProperty(propertyID, voidString);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // update the main library listeners
  rv = UpdateMainLibraryListeners();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabaseSimpleMediaList> simpleList;
  simpleList = do_QueryInterface(list, &rv);

  if(NS_SUCCEEDED(rv)) {
    rv = simpleList->SetCopyListener(this);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    NS_WARN_IF_FALSE(rv,
      "Failed to get sbILocalDatabaseSimpleMediaList interface. Copy Listener disabled.");
  }

  // update the library is read-only property
  rv = UpdateIsReadOnly();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RegisterDeviceLibrary();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceLibrary::RegisterDeviceLibrary()
{

  nsresult rv;
  nsCOMPtr<sbILibraryManager> libraryManager;
  libraryManager =
    do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return libraryManager->RegisterLibrary(this, PR_FALSE);
}

nsresult
sbDeviceLibrary::UnregisterDeviceLibrary()
{
  nsresult rv;
  nsCOMPtr<sbILibraryManager> libraryManager;
  libraryManager =
    do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return libraryManager->UnregisterLibrary(this);
}

/* static */ nsresult
sbDeviceLibrary::GetDefaultDeviceLibraryDatabaseFile
                   (const nsAString& aDeviceIdentifier,
                    nsIFile**        aDBFile)
{
  NS_ENSURE_ARG_POINTER(aDBFile);

  nsresult rv;

  // Fetch default device library database file location.
  nsCOMPtr<nsIFile> libraryFile;
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                              getter_AddRefs(libraryFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = libraryFile->Append(NS_LITERAL_STRING("db"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists = PR_FALSE;
  rv = libraryFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if(!exists) {
    rv = libraryFile->Create(nsIFile::DIRECTORY_TYPE, 0700);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsAutoString filename(aDeviceIdentifier);
  filename.AppendLiteral(".db");

  rv = libraryFile->Append(filename);
  NS_ENSURE_SUCCESS(rv, rv);

  libraryFile.forget(aDBFile);

  return NS_OK;
}

nsresult
sbDeviceLibrary::GetIsSyncedLocally(PRBool* aIsSyncedLocally)
{
  NS_ASSERTION(aIsSyncedLocally, "aIsSyncedLocally is null");

  PRBool   isSyncedLocally = PR_FALSE;
  nsresult rv;

  nsAutoString localSyncPartnerID;
  rv = GetMainLibraryId(localSyncPartnerID);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> deviceSyncPartnerIDVariant;
  nsAutoString         deviceSyncPartnerID;
  rv = mDevice->GetPreference(NS_LITERAL_STRING("SyncPartner"),
                              getter_AddRefs(deviceSyncPartnerIDVariant));
  if (NS_SUCCEEDED(rv)) {
    rv = deviceSyncPartnerIDVariant->GetAsAString(deviceSyncPartnerID);
    if (NS_SUCCEEDED(rv))
      isSyncedLocally = deviceSyncPartnerID.Equals(localSyncPartnerID);
  }

  *aIsSyncedLocally = isSyncedLocally;

  return NS_OK;
}

nsresult
sbDeviceLibrary::GetIsMgmtTypeManual(PRBool* aIsMgmtTypeManual)
{
  NS_ASSERTION(aIsMgmtTypeManual, "aIsMgmtTypeManual is null");

  *aIsMgmtTypeManual = PR_FALSE;

  PRUint32 mgmtType;
  nsresult rv;
  for (PRUint32 i = 0; i < sbIDeviceLibrary::MEDIATYPE_COUNT; ++i) {
    rv = GetMgmtType(i, &mgmtType);
    NS_ENSURE_SUCCESS(rv, rv);

    // Manual mode.
    if (mgmtType & sbIDeviceLibrary::MGMT_TYPE_MANUAL) {
      *aIsMgmtTypeManual = PR_TRUE;
      return NS_OK;
    }
  }

  return NS_OK;
}

nsresult
sbDeviceLibrary::GetIsMgmtTypeSyncList(PRBool* aIsMgmtTypeSyncList)
{
  NS_ASSERTION(aIsMgmtTypeSyncList, "aIsMgmtTypeSyncList is null");

  *aIsMgmtTypeSyncList = PR_FALSE;

  PRUint32 mgmtType;
  nsresult rv;
  for (PRUint32 i = 0; i < sbIDeviceLibrary::MEDIATYPE_COUNT; ++i) {
    rv = GetMgmtType(i, &mgmtType);
    NS_ENSURE_SUCCESS(rv, rv);

    // Manual mode.
    if (mgmtType & sbIDeviceLibrary::MGMT_TYPE_MANUAL)
      return NS_OK;

    // Sync playlist mode.
    if (mgmtType == sbIDeviceLibrary::MGMT_TYPE_SYNC_PLAYLISTS) {
      *aIsMgmtTypeSyncList = PR_TRUE;
      return NS_OK;
    }
  }

  return NS_OK;
}

nsresult
sbDeviceLibrary::GetIsMgmtTypeSyncAll(PRBool* aIsMgmtTypeSyncAll)
{
  NS_ASSERTION(aIsMgmtTypeSyncAll, "aIsMgmtTypeSyncAll is null");

  *aIsMgmtTypeSyncAll = PR_TRUE;

  PRUint32 mgmtType;
  nsresult rv;
  for (PRUint32 i = 0; i < sbIDeviceLibrary::MEDIATYPE_COUNT; ++i) {
    rv = GetMgmtType(i, &mgmtType);
    NS_ENSURE_SUCCESS(rv, rv);

    // Manual mode.
    if (mgmtType & sbIDeviceLibrary::MGMT_TYPE_MANUAL) {
      *aIsMgmtTypeSyncAll = PR_FALSE;
      return NS_OK;
    }

    // Sync playlist mode.
    if (mgmtType == sbIDeviceLibrary::MGMT_TYPE_SYNC_PLAYLISTS) {
      *aIsMgmtTypeSyncAll = PR_FALSE;
      return NS_OK;
    }
  }

  return NS_OK;
}

nsresult
sbDeviceLibrary::UpdateMainLibraryListeners()
{
  NS_ENSURE_STATE(mDevice);

  nsresult rv;

  nsCOMPtr<sbILibrary> mainLib;
  rv = GetMainLibrary(getter_AddRefs(mainLib));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isManual = PR_FALSE;
  rv = GetIsMgmtTypeManual(&isManual);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isSyncedLocally = PR_FALSE;
  rv = GetIsSyncedLocally(&isSyncedLocally);
  NS_ENSURE_SUCCESS(rv, rv);

  // Not in manual mode.
  if (!isManual && isSyncedLocally) {
    // hook up the metadata updating listener
    rv = mainLib->AddListener(mMainLibraryListener,
                              PR_FALSE,
                              sbIMediaList::LISTENER_FLAGS_ITEMADDED |
                              sbIMediaList::LISTENER_FLAGS_BEFOREITEMREMOVED |
                              sbIMediaList::LISTENER_FLAGS_ITEMUPDATED,
                              mMainLibraryListenerFilter);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool isSyncAll = PR_FALSE;
    rv = GetIsMgmtTypeSyncAll(&isSyncAll);
    NS_ENSURE_SUCCESS(rv, rv);
    if (isSyncAll) {
      mMainLibraryListener->SetSyncMode(isManual, nsnull);

      // hook up the media list listeners to the existing lists
      nsRefPtr<sbPlaylistAttachListenerEnumerator> enumerator =
        new sbPlaylistAttachListenerEnumerator(mMainLibraryListener);
      NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

      rv = mainLib->EnumerateItemsByProperty(NS_LITERAL_STRING(SB_PROPERTY_ISLIST),
                                             NS_LITERAL_STRING("1"),
                                             enumerator,
                                             sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      nsCOMPtr<nsIArray> playlists;
      rv = GetSyncPlaylistList(getter_AddRefs(playlists));
      NS_ENSURE_SUCCESS(rv, rv);

      mMainLibraryListener->SetSyncMode(isManual, playlists);

      // Listen to all the playlists specified for synchronization
      PRUint32 length;
      rv = playlists->GetLength(&length);
      NS_ENSURE_SUCCESS(rv, rv);

      // Need to stop listening to all the playlists so we can listen to just
      // the selected ones
      mMainLibraryListener->StopListeningToPlaylists();

      for (PRUint32 index = 0; index < length; ++index) {
        nsCOMPtr<sbIMediaItem> item = do_QueryElementAt(playlists, index, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<sbIMediaList> list = do_QueryInterface(item, &rv);
        // Not media list. Skip.
        if (NS_FAILED(rv))
          continue;

        rv = mMainLibraryListener->ListenToPlaylist(list);
      }

      // remove the metadata updating listener
      rv = mainLib->RemoveListener(mMainLibraryListener);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } else {
    mMainLibraryListener->SetSyncMode(isManual, nsnull);

    // remove the metadata updating listener
    rv = mainLib->RemoveListener(mMainLibraryListener);
    NS_ENSURE_SUCCESS(rv, rv);

    mMainLibraryListener->StopListeningToPlaylists();
  }

  return NS_OK;
}

nsresult
sbDeviceLibrary::UpdateIsReadOnly()
{
  nsresult rv;

  // Get the management type
  PRBool isManual;
  rv = GetIsMgmtTypeManual(&isManual);
  NS_ENSURE_SUCCESS(rv, rv);

  // Update the library is read-only property
  if (isManual) {
    // Mark library as read-write
    nsString str;
    str.SetIsVoid(PR_TRUE);
    rv = this->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_ISREADONLY), str);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    // Mark library as read-only
    rv = this->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_ISREADONLY),
                           NS_LITERAL_STRING("1"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbDeviceLibrary::RemoveFromSyncPlaylistList(PRUint32 aContentType,
                                            nsAString& aGUID)
{
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);
  nsresult rv;

  // Get the preference key
  nsString prefKey;
  rv = GetSyncListsPrefKey(aContentType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the preference variant
  nsCOMPtr<nsIVariant> var;
  rv = mDevice->GetPreference(prefKey, getter_AddRefs(var));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the preference string
  nsAutoString listGuidsCSV;
  rv = var->GetAsAString(listGuidsCSV);
  NS_ENSURE_SUCCESS(rv, rv);

  // Find the start and end of the GUID to remove
  PRInt32 start = listGuidsCSV.Find(aGUID);
  if (start < 0)
    return NS_OK;
  PRInt32 end = start + aGUID.Length();

  // Include a comma before or after GUID
  if (end < ((PRInt32) listGuidsCSV.Length()))
    end++;
  else if (start > 0)
    start--;

  // Cut out GUID with comma
  listGuidsCSV.Cut(start, end - start);

  // Write the preference
  rv = mDevice->SetPreference(prefKey, sbNewVariant(listGuidsCSV));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceLibrary::GetMgmtTypePrefKey(PRUint32 aContentType, nsAString& aPrefKey)
{
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);

  // Get the device library GUID
  nsString guid;
  nsresult rv = mDeviceLibrary->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the preference key
  aPrefKey.Assign(NS_LITERAL_STRING(PREF_SYNC_PREFIX));
  aPrefKey.Append(guid);
  aPrefKey.AppendLiteral(PREF_SYNC_BRANCH PREF_SYNC_MGMTTYPE);

  aPrefKey.AppendLiteral(gMediaType[aContentType]);

  return NS_OK;
}

nsresult
sbDeviceLibrary::GetSyncListsPrefKey(PRUint32 aContentType,
                                     nsAString& aPrefKey)
{
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);

  // Get the device library GUID
  nsString guid;
  nsresult rv = mDeviceLibrary->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the preference key
  aPrefKey.Assign(NS_LITERAL_STRING(PREF_SYNC_PREFIX));
  aPrefKey.Append(guid);
  aPrefKey.AppendLiteral(PREF_SYNC_BRANCH);
  aPrefKey.AppendLiteral(PREF_SYNC_LISTS);
  aPrefKey.AppendLiteral(gMediaType[aContentType]);

  return NS_OK;
}

nsresult
sbDeviceLibrary::ConfirmSwitchFromManualToSync(PRBool* aCancelSwitch)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aCancelSwitch);

  // Function variables.
  nsresult rv;

  // Get the device name.
  nsString deviceName;
  rv = mDevice->GetName(deviceName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get a prompter.
  nsCOMPtr<sbIPrompter> prompter =
                          do_CreateInstance(SONGBIRD_PROMPTER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the prompt title.
  nsAString const& title =
    SBLocalizedString("device.dialog.sync_confirmation.change_mode.title");

  // Get the prompt message.
  nsTArray<nsString> formatParams;
  formatParams.AppendElement(deviceName);
  nsAString const& message =
    SBLocalizedString("device.dialog.sync_confirmation.change_mode.msg",
                      formatParams);

  // Configure the buttons.
  PRUint32 buttonFlags = 0;

  // Configure the no button as button 1.
  nsAString const& noButton =
    SBLocalizedString("device.dialog.sync_confirmation.change_mode.no_button");
  buttonFlags += (nsIPromptService::BUTTON_POS_1 *
                  nsIPromptService::BUTTON_TITLE_IS_STRING);

  // Configure the sync button as button 0.
  nsAString const& syncButton = SBLocalizedString
    ("device.dialog.sync_confirmation.change_mode.sync_button");
  buttonFlags += (nsIPromptService::BUTTON_POS_0 *
                  nsIPromptService::BUTTON_TITLE_IS_STRING) +
                 nsIPromptService::BUTTON_POS_0_DEFAULT;
  PRInt32 grantModeChangeIndex = 0;

  // XXX lone> see mozbug 345067, there is no way to tell the prompt service
  // what code to return when the titlebar's X close button is clicked, it is
  // always 1, so we have to make the No button the second button.

  // Query the user to determine whether the device management mode should be
  // changed from manual to sync.
  PRInt32 buttonPressed;
  rv = prompter->ConfirmEx(nsnull,
                           title.BeginReading(),
                           message.BeginReading(),
                           buttonFlags,
                           syncButton.BeginReading(),
                           noButton.BeginReading(),
                           nsnull,                      // No button 2.
                           nsnull,                      // No check message.
                           nsnull,                      // No check result.
                           &buttonPressed);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check if the management mode switch was cancelled.
  if (buttonPressed == grantModeChangeIndex)
    *aCancelSwitch = PR_FALSE;
  else
    *aCancelSwitch = PR_TRUE;

  return NS_OK;
}

/**
 * sbIDeviceLibrary
 */

NS_IMETHODIMP
sbDeviceLibrary::GetMgmtType(PRUint32 aContentType, PRUint32* aMgmtType)
{
  NS_ENSURE_STATE(mDevice);
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);
  nsresult rv;

  nsString prefKey;
  rv = GetMgmtTypePrefKey(aContentType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> var;
  rv = mDevice->GetPreference(prefKey, getter_AddRefs(var));
  NS_ENSURE_SUCCESS(rv, rv);

  // check if a value exists
  PRUint16 dataType;
  rv = var->GetDataType(&dataType);
  if (dataType == nsIDataType::VTYPE_VOID ||
      dataType == nsIDataType::VTYPE_EMPTY)
  {
    // no value, default to manual
    *aMgmtType = sbIDeviceLibrary::MGMT_TYPE_MANUAL;
  } else {
    // has a value
    PRInt32 mgmtType;
    rv = var->GetAsInt32(&mgmtType);
    NS_ENSURE_SUCCESS(rv, rv);

    // Double check that it is a valid number
    NS_ENSURE_ARG_RANGE(mgmtType,
                        sbIDeviceLibrary::MGMT_TYPE_MANUAL,
                        sbIDeviceLibrary::MGMT_TYPE_MAX_VALUE);

    *aMgmtType = mgmtType;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::SetMgmtTypes(PRUint32 aMgmtType, PRBool aManualFlag)
{
  NS_ENSURE_STATE(mDevice);
  nsresult rv;

  // Check we are setting to a valid number
  NS_ENSURE_ARG_RANGE(aMgmtType,
                      sbIDeviceLibrary::MGMT_TYPE_MANUAL,
                      sbIDeviceLibrary::MGMT_TYPE_MAX_VALUE);

  PRBool isManual;
  rv = GetIsMgmtTypeManual(&isManual);
  NS_ENSURE_SUCCESS(rv, rv);

  // Flag for auto sync.
  PRBool autoSync = !aManualFlag ||
                    (aMgmtType != sbIDeviceLibrary::MGMT_TYPE_MANUAL);

  // If switching from manual management mode to sync mode, confirm with user
  // before proceeding.  Do nothing more if switch is cancelled.
  if (isManual && autoSync) {
    // Check if device is linked to a local sync partner.
    PRBool isLinkedLocally;
    rv = sbDeviceUtils::SyncCheckLinkedPartner(mDevice,
                                               PR_FALSE,
                                               &isLinkedLocally);
    NS_ENSURE_SUCCESS(rv, rv);

    // Confirm either link partner switch or manual to sync mode switch.
    PRBool cancelSwitch = PR_TRUE;
    if (!isLinkedLocally) {
      // Request that the link partner be changed.
      rv = sbDeviceUtils::SyncCheckLinkedPartner(mDevice,
                                                 PR_TRUE,
                                                 &isLinkedLocally);
      NS_ENSURE_SUCCESS(rv, rv);

      // Cancel switch if link partner was not changed.
      if (!isLinkedLocally)
        cancelSwitch = PR_TRUE;
      else
        cancelSwitch = PR_FALSE;
    } else {
      rv = ConfirmSwitchFromManualToSync(&cancelSwitch);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Do nothing more is switch canceled.
    if (cancelSwitch)
      return NS_OK;
  }

  // Set the management type to the preference.
  PRBool sameMgmtType = PR_TRUE;
  PRUint32 origMgmtType;

  // Toggle manual for the management type preference.
  if (aMgmtType == sbIDeviceLibrary::MGMT_TYPE_MANUAL) {
    for (PRUint32 i = 0; i < sbIDeviceLibrary::MEDIATYPE_COUNT; ++i) {
      // figure out the old pref first
      rv = GetMgmtType(i, &origMgmtType);
      NS_ENSURE_SUCCESS(rv, rv);

      // Set to manual mode.
      if (aManualFlag) {
        rv = SetMgmtType(i, origMgmtType | sbIDeviceLibrary::MGMT_TYPE_MANUAL);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      // Unset manual mode.
      else {
        origMgmtType &= ~(sbIDeviceLibrary::MGMT_TYPE_MANUAL);
        // No initial value for management type. Set to sync all.
        if (!origMgmtType)
          origMgmtType = sbIDeviceLibrary::MGMT_TYPE_SYNC_ALL;
        rv = SetMgmtType(i, origMgmtType);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    // Toggle manual mode. Not same.
    sameMgmtType = PR_FALSE;
  }
  // Set one of the sync modes.
  else {
    for (PRUint32 i = 0; i < sbIDeviceLibrary::MEDIATYPE_COUNT; ++i) {
      rv = GetMgmtType(i, &origMgmtType);
      NS_ENSURE_SUCCESS(rv, rv);

      if (origMgmtType != aMgmtType) {
        rv = SetMgmtType(i, aMgmtType);
        NS_ENSURE_SUCCESS(rv, rv);
        sameMgmtType = PR_FALSE;
      }
    }
  }

  if (!sameMgmtType) {
    if (autoSync) {
      // sync
      rv = mDevice->SyncLibraries();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // update the library is read-only property
    rv = UpdateIsReadOnly();
    NS_ENSURE_SUCCESS(rv, rv);

    // update the main library listeners
    rv = UpdateMainLibraryListeners();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::SetMgmtType(PRUint32 aContentType, PRUint32 aMgmtType)
{
  NS_ENSURE_STATE(mDevice);
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);

  // Check we are setting to a valid number
  NS_ENSURE_ARG_RANGE(aMgmtType,
                      sbIDeviceLibrary::MGMT_TYPE_MANUAL,
                      sbIDeviceLibrary::MGMT_TYPE_MAX_VALUE);

  nsresult rv = SetMgmtTypePref(aContentType, &aMgmtType);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::GetSyncPlaylistList(nsIArray **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv;

  PRBool isSyncAll;
  rv = GetIsMgmtTypeSyncAll(&isSyncAll);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibraryManager> libManager =
      do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> mainLibrary;
  rv = libManager->GetMainLibrary(getter_AddRefs(mainLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  // Create a sync array.
  nsCOMPtr<nsIMutableArray>
    syncAllList =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1",
                        &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isSyncAll) {
    nsCOMPtr<sbIMediaList> srcLibML = do_QueryInterface(mainLibrary, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = syncAllList->AppendElement(srcLibML, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*_retval = syncAllList);
    return NS_OK;
  }

  PRUint32 mgmtType;
  // the variable i is used for both index and the media type
  for (PRUint32 i = 0; i < sbIDeviceLibrary::MEDIATYPE_COUNT; ++i) {
    rv = GetMgmtType(i, &mgmtType);
    NS_ENSURE_SUCCESS(rv, rv);

    // Add all items/lists with media type "i" to the sync list.
    if (mgmtType == sbIDeviceLibrary::MGMT_TYPE_SYNC_ALL) {
      nsCOMPtr<sbIMediaList> srcLibML = do_QueryInterface(mainLibrary, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 length;
      rv = srcLibML->GetLength(&length);
      NS_ENSURE_SUCCESS(rv, rv);

      // Loop through the items/lists in the main library.
      for (PRUint32 j = 0; j < length; ++j) {
        nsCOMPtr<sbIMediaItem> mediaItem;
        rv = srcLibML->GetItemByIndex(j, getter_AddRefs(mediaItem));
        NS_ENSURE_SUCCESS(rv, rv);

        // Duplicate item. Skip
        PRUint32 index;
        rv = syncAllList->IndexOf(0, mediaItem, &index);
        if (index != -1)
          continue;

        nsCOMPtr<sbIMediaList> mediaList =
            do_QueryInterface(mediaItem, &rv);
        // Process media item.
        if (NS_FAILED(rv)) {
          nsString contentType;
          rv = mediaItem->GetProperty(
              NS_LITERAL_STRING(SB_PROPERTY_CONTENTTYPE),
              contentType);
          NS_ENSURE_SUCCESS(rv, rv);

          PRInt32 mediaType = -1;
          if (contentType.Equals(NS_LITERAL_STRING("audio")))
            mediaType = sbIDeviceLibrary::MEDIATYPE_AUDIO;
          else if (contentType.Equals(NS_LITERAL_STRING("video")))
            mediaType = sbIDeviceLibrary::MEDIATYPE_VIDEO;

          // Add audio or video item to the array for syncing
          if (mediaType == i) {
            rv = syncAllList->AppendElement(mediaItem, PR_FALSE);
            NS_ENSURE_SUCCESS(rv, rv);
          }
        }
        // Process media list.
        else {
          PRUint16 listType;
          rv = mediaList->GetListContentType(&listType);
          NS_ENSURE_SUCCESS(rv, rv);

          // Matches CONTENTTYPE_* and MEDIETYPE_* constants
          if ((listType == sbIMediaList::CONTENTTYPE_MIX) ||
              (listType-1) == i) {
            rv = syncAllList->AppendElement(mediaItem, PR_FALSE);
            NS_ENSURE_SUCCESS(rv, rv);
          }
        }
      }
    }
    // Add playlists to the sync list.
    else if (mgmtType == sbIDeviceLibrary::MGMT_TYPE_SYNC_PLAYLISTS) {
      nsCOMPtr<nsIArray> array;

      rv = GetSyncPlaylistListByType(i, getter_AddRefs(array));
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 length;
      rv = array->GetLength(&length);
      NS_ENSURE_SUCCESS(rv, rv);

      for (PRUint32 j = 0; j < length; ++j) {
        nsCOMPtr<sbIMediaList> syncPlaylist = do_QueryElementAt(array,
                                                                j,
                                                                &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = syncAllList->AppendElement(syncPlaylist, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  NS_ADDREF(*_retval = syncAllList);
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::GetSyncPlaylistListByType(PRUint32 aContentType,
                                           nsIArray **_retval)
{
  NS_ENSURE_ARG_POINTER( _retval );
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);
  nsresult rv;

  nsCOMPtr<nsIMutableArray> array =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibraryManager> libManager =
      do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> mainLibrary;
  rv = libManager->GetMainLibrary(getter_AddRefs(mainLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the preference key
  nsString prefKey;
  rv = GetSyncListsPrefKey(aContentType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> var;
  rv = mDevice->GetPreference(prefKey, getter_AddRefs(var));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString listGuidsCSV;
  rv = var->GetAsAString(listGuidsCSV);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 start = 0;
  PRInt32 end = listGuidsCSV.FindChar(',', start);
  if (end < 0)
    end = listGuidsCSV.Length();
  while (end > start) {
    // Extract the list GUID
    nsDependentSubstring listGUID = Substring(listGuidsCSV, start, end - start);

    // Find the playlist item in the main Songbird library
    nsCOMPtr<sbIMediaItem> syncPlaylistItem;
    rv = mainLibrary->GetItemByGuid(listGUID, getter_AddRefs(syncPlaylistItem));
    if (NS_FAILED(rv)) {
      if (rv == NS_ERROR_NOT_AVAILABLE) {
        RemoveFromSyncPlaylistList(aContentType, listGUID);
      }
      syncPlaylistItem = nsnull;
    }

    // Add the playlist media list to the sync playlist list array
    if (syncPlaylistItem) {
      nsCOMPtr<sbIMediaList> syncPlaylist = do_QueryInterface(syncPlaylistItem,
                                                              &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = array->AppendElement(syncPlaylist, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Scan for the next playlist GUID
    start = end + 1;
    end = listGuidsCSV.FindChar(',', start);
    if (end < 0)
      end = listGuidsCSV.Length();
  }

  NS_ADDREF(*_retval = array);
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::SetSyncPlaylistList(nsIArray *aPlaylistList)
{
  NS_ENSURE_ARG_POINTER(aPlaylistList);
  nsresult rv;

  for (PRUint32 i = 0; i < sbIDeviceLibrary::MEDIATYPE_COUNT; ++i) {
    rv = SetSyncPlaylistListByType(i, aPlaylistList);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::SetSyncPlaylistListByType(PRUint32 aContentType,
                                           nsIArray *aPlaylistList)
{
  NS_ENSURE_ARG_POINTER(aPlaylistList);
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);

  nsresult rv = SetSyncPlaylistListPref(aContentType, aPlaylistList);
  NS_ENSURE_SUCCESS(rv, rv);

  /*XXXeps resync on change?*/
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::AddToSyncPlaylistList(PRUint32 aContentType,
                                       sbIMediaList *aPlaylist)
{
  NS_ENSURE_ARG_POINTER(aPlaylist);
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);
  nsresult rv;

  // Get the preference key
  nsString prefKey;
  rv = GetSyncListsPrefKey(aContentType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the preference variant
  nsCOMPtr<nsIVariant> var;
  rv = mDevice->GetPreference(prefKey, getter_AddRefs(var));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the preference string
  nsAutoString listGuidsCSV;
  rv = var->GetAsAString(listGuidsCSV);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the guid of the list to add
  nsString guid;
  rv = aPlaylist->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the sync playlist if it's not already present
  if (listGuidsCSV.Find(guid) < 0) {
    if (listGuidsCSV.Length() > 0)
      listGuidsCSV.AppendLiteral(",");
    listGuidsCSV.Append(guid);
    rv = mDevice->SetPreference(prefKey, sbNewVariant(listGuidsCSV));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::RemoveFromSyncPlaylistList(PRUint32 aContentType,
                                            sbIMediaList *aPlaylist)
{
  NS_ENSURE_ARG_POINTER(aPlaylist);
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);
  nsresult rv;

  // Get the preference key
  nsString prefKey;
  rv = GetSyncListsPrefKey(aContentType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the preference variant
  nsCOMPtr<nsIVariant> var;
  rv = mDevice->GetPreference(prefKey, getter_AddRefs(var));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the preference string
  nsAutoString listGuidsCSV;
  rv = var->GetAsAString(listGuidsCSV);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the guid of the list to remove
  nsString guid;
  rv = aPlaylist->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Remove from the playlist if it exists
  PRUint32 guidPos = listGuidsCSV.Find(guid);
  if (guidPos >= 0) {
    listGuidsCSV.Cut(guidPos, guid.Length() + 1); // Add 1 for any ","
    listGuidsCSV.Trim(",", PR_TRUE, PR_TRUE); // Remove extra ","s
    rv = mDevice->SetPreference(prefKey, sbNewVariant(listGuidsCSV));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::Sync()
{
  nsresult rv;

  nsCOMPtr<sbIDevice> device;
  rv = GetDevice(getter_AddRefs(device));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(device,
               "sbDeviceLibrary::GetDevice returned success with no device");

  nsCOMPtr<sbILibraryManager> libraryManager =
    do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> mainLib;
  rv = libraryManager->GetMainLibrary(getter_AddRefs(mainLib));
  NS_ENSURE_SUCCESS(rv, rv);

  #if DEBUG
    // sanity check that we're using the right device
    { /* scope */
      nsCOMPtr<sbIDeviceContent> content;
      rv = device->GetContent(getter_AddRefs(content));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIArray> libraries;
      rv = content->GetLibraries(getter_AddRefs(libraries));
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 libraryCount;
      rv = libraries->GetLength(&libraryCount);
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool found = PR_FALSE;
      for (PRUint32 index = 0; index < libraryCount; ++index) {
        nsCOMPtr<sbIDeviceLibrary> library =
          do_QueryElementAt(libraries, index, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        if (SameCOMIdentity(NS_ISUPPORTS_CAST(sbILibrary*, this), library)) {
          found = PR_TRUE;
          break;
        }
      }
      NS_ASSERTION(found,
                   "sbDeviceLibrary has device that doesn't hold this library");
    }
  #endif /* DEBUG */

  nsCOMPtr<nsIWritablePropertyBag2> requestParams =
    do_CreateInstance(NS_HASH_PROPERTY_BAG_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = requestParams->SetPropertyAsInterface(NS_LITERAL_STRING("item"),
                                             mainLib);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = requestParams->SetPropertyAsInterface(NS_LITERAL_STRING("list"),
                                             NS_ISUPPORTS_CAST(sbIMediaList*, this));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = device->SubmitRequest(sbIDevice::REQUEST_SYNC, requestParams);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::AddDeviceLibraryListener(sbIDeviceLibraryListener* aListener)
{
  TRACE(("sbDeviceLibrary[0x%x] - AddListener", this));
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
  nsCOMPtr<sbIDeviceLibraryListener> proxy;
  nsresult rv = SB_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                                     NS_GET_IID(sbIDeviceLibraryListener),
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

NS_IMETHODIMP
sbDeviceLibrary::RemoveDeviceLibraryListener(sbIDeviceLibraryListener* aListener)
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
 * \brief This callback adds the enumerated startup libraries to an nsCOMArray.
 *
 * \param aKey      - An nsAString representing the GUID of the library.
 * \param aEntry    - An sbIDeviceLibraryListener entry.
 * \param aUserData - An nsCOMArray to hold the enumerated pointers.
 *
 * \return PL_DHASH_NEXT on success
 * \return PL_DHASH_STOP on failure
 */
/* static */ PLDHashOperator PR_CALLBACK
sbDeviceLibrary::AddListenersToCOMArrayCallback(nsISupportsHashKey::KeyType aKey,
                                                sbIDeviceLibraryListener* aEntry,
                                                void* aUserData)
{
  NS_ASSERTION(aKey, "Null key in hashtable!");
  NS_ASSERTION(aEntry, "Null entry in hashtable!");

  nsCOMArray<sbIDeviceLibraryListener>* array =
    static_cast<nsCOMArray<sbIDeviceLibraryListener>*>(aUserData);

  PRBool success = array->AppendObject(aEntry);
  NS_ENSURE_TRUE(success, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

#define SB_NOTIFY_LISTENERS(call)                                             \
  nsCOMArray<sbIDeviceLibraryListener> listeners;                             \
  {                                                                           \
    nsAutoLock lock(mLock);                                                   \
    mListeners.EnumerateRead(AddListenersToCOMArrayCallback, &listeners);     \
  }                                                                           \
                                                                              \
  PRInt32 count = listeners.Count();                                          \
  for (PRInt32 index = 0; index < count; index++) {                           \
    nsCOMPtr<sbIDeviceLibraryListener> listener = listeners.ObjectAt(index);  \
    NS_ASSERTION(listener, "Null listener!");                                 \
    listener->call;                                                           \
  }                                                                           \

#define SB_NOTIFY_LISTENERS_ASK_PERMISSION(call)                              \
  PRBool mShouldProcceed = PR_TRUE;                                           \
  PRBool mPerformAction = PR_TRUE;                                            \
                                                                              \
  nsCOMArray<sbIDeviceLibraryListener> listeners;                             \
  {                                                                           \
    nsAutoLock lock(mLock);                                                   \
    mListeners.EnumerateRead(AddListenersToCOMArrayCallback, &listeners);     \
  }                                                                           \
                                                                              \
  PRInt32 count = listeners.Count();                                          \
  for (PRInt32 index = 0; index < count; index++) {                           \
    nsCOMPtr<sbIDeviceLibraryListener> listener = listeners.ObjectAt(index);  \
    NS_ASSERTION(listener, "Null listener!");                                 \
    listener->call/*(param, ..., &mShouldProcceed)*/;                         \
    if (!mShouldProcceed) {                                                   \
      /* Abort as soon as one returns false */                                \
      mPerformAction = PR_FALSE;                                              \
      break;                                                                  \
    }                                                                         \
  }                                                                           \


NS_IMETHODIMP
sbDeviceLibrary::OnBatchBegin(sbIMediaList* aMediaList)
{
  TRACE(("sbDeviceLibrary[0x%x] - OnBatchBegin", this));

  SB_NOTIFY_LISTENERS(OnBatchBegin(aMediaList));
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::OnBatchEnd(sbIMediaList* aMediaList)
{
  TRACE(("sbDeviceLibrary[0x%x] - OnBatchEnd", this));

  SB_NOTIFY_LISTENERS(OnBatchEnd(aMediaList));
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::OnItemAdded(sbIMediaList* aMediaList,
                             sbIMediaItem* aMediaItem,
                             PRUint32 aIndex,
                             PRBool* aNoMoreForBatch)
{
  TRACE(("sbDeviceLibrary[0x%x] - OnItemAdded", this));

  SB_NOTIFY_LISTENERS(OnItemAdded(aMediaList,
                                  aMediaItem,
                                  aIndex,
                                  aNoMoreForBatch));

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::OnBeforeItemRemoved(sbIMediaList* aMediaList,
                                     sbIMediaItem* aMediaItem,
                                     PRUint32 aIndex,
                                     PRBool* aNoMoreForBatch)
{
  TRACE(("sbDeviceLibrary[0x%x] - OnBeforeItemRemoved", this));

  SB_NOTIFY_LISTENERS(OnBeforeItemRemoved(aMediaList,
                                          aMediaItem,
                                          aIndex,
                                          aNoMoreForBatch));
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::OnAfterItemRemoved(sbIMediaList* aMediaList,
                                    sbIMediaItem* aMediaItem,
                                    PRUint32 aIndex,
                                    PRBool* aNoMoreForBatch)
{
  TRACE(("sbDeviceLibrary[0x%x] - OnAfterItemRemoved", this));

  SB_NOTIFY_LISTENERS(OnAfterItemRemoved(aMediaList,
                                         aMediaItem,
                                         aIndex,
                                         aNoMoreForBatch));
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::OnItemUpdated(sbIMediaList *aMediaList,
                               sbIMediaItem *aMediaItem,
                               sbIPropertyArray* aProperties,
                               PRBool* aNoMoreForBatch)
{
  TRACE(("sbDeviceLibrary[0x%x] - OnItemUpdated", this));

  SB_NOTIFY_LISTENERS(OnItemUpdated(aMediaList,
                                    aMediaItem,
                                    aProperties,
                                    aNoMoreForBatch));
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::OnItemMoved(sbIMediaList *aMediaList,
                             PRUint32 aFromIndex,
                             PRUint32 aToIndex,
                             PRBool* aNoMoreForBatch)
{
  TRACE(("sbDeviceLibrary[0x%x] - OnItemMoved", this));

  SB_NOTIFY_LISTENERS(OnItemMoved(aMediaList,
                                  aFromIndex,
                                  aToIndex,
                                  aNoMoreForBatch));
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::OnBeforeListCleared(sbIMediaList *aMediaList,
                                     PRBool aExcludeLists,
                                     PRBool* aNoMoreForBatch)
{
  TRACE(("sbDeviceLibrary[0x%x] - OnListCleared", this));

  SB_NOTIFY_LISTENERS(OnBeforeListCleared(aMediaList,
                                          aExcludeLists,
                                          aNoMoreForBatch));
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::OnListCleared(sbIMediaList *aMediaList,
                               PRBool aExcludeLists,
                               PRBool* aNoMoreForBatch)
{
  TRACE(("sbDeviceLibrary[0x%x] - OnListCleared", this));

  SB_NOTIFY_LISTENERS(OnListCleared(aMediaList,
                                    aExcludeLists,
                                    aNoMoreForBatch));
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::OnItemCopied(sbIMediaItem *aSourceItem,
                              sbIMediaItem *aDestItem)
{
  TRACE(("sbDeviceLibrary[0x%x] - OnItemCopied", this));

  SB_NOTIFY_LISTENERS(OnItemCopied(aSourceItem, aDestItem));

  return NS_OK;
}

NS_IMETHODIMP sbDeviceLibrary::OnDeviceEvent(sbIDeviceEvent* aEvent)
{
  nsresult rv;

  // get the event type.
  PRUint32 type;
  rv = aEvent->GetType(&type);
  NS_ENSURE_SUCCESS(rv, rv);

  // handle changes to the sync parnter preference
  if (type == sbIDeviceEvent::EVENT_DEVICE_PREFS_CHANGED) {
    // get the synced locally state
    PRBool isSyncedLocally = PR_FALSE;
    rv = GetIsSyncedLocally(&isSyncedLocally);
    NS_ENSURE_SUCCESS(rv, rv);

    // update the main library listeners if the synced locally state changed
    if (isSyncedLocally != mLastIsSyncedLocally) {
      rv = UpdateMainLibraryListeners();
      NS_ENSURE_SUCCESS(rv, rv);
      mLastIsSyncedLocally = isSyncedLocally;
    }
  }

  return NS_OK;
}

/*
 * See sbILibrary
 */
NS_IMETHODIMP
sbDeviceLibrary::CreateMediaItem(nsIURI *aContentUri,
                                 sbIPropertyArray *aProperties,
                                 PRBool aAllowDuplicates,
                                 sbIMediaItem **_retval)
{
  NS_ASSERTION(mDeviceLibrary, "mDeviceLibrary is null, call init first.");
  SB_NOTIFY_LISTENERS_ASK_PERMISSION(OnBeforeCreateMediaItem(aContentUri,
                                                             aProperties,
                                                             aAllowDuplicates,
                                                             &mShouldProcceed));
  if (mPerformAction) {
    nsresult rv;
    nsCOMPtr<sbILibrary> lib;
    rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                              NS_GET_IID(sbILibrary),
                              mDeviceLibrary,
                              NS_PROXY_SYNC,
                              getter_AddRefs(lib));
    NS_ENSURE_SUCCESS(rv, rv);

    return lib->CreateMediaItem(aContentUri,
                                aProperties,
                                aAllowDuplicates,
                                _retval);
  } else {
    return NS_OK;
  }
}

/*
 * See sbILibrary
 */
NS_IMETHODIMP
sbDeviceLibrary::CreateMediaItemIfNotExist(nsIURI *aContentUri,
                                           sbIPropertyArray *aProperties,
                                           sbIMediaItem **aResultItem,
                                           PRBool *_retval)
{
  NS_ASSERTION(mDeviceLibrary, "mDeviceLibrary is null, call init first.");
  SB_NOTIFY_LISTENERS_ASK_PERMISSION(OnBeforeCreateMediaItem(aContentUri,
                                                             aProperties,
                                                             PR_FALSE,
                                                             &mShouldProcceed));
  if (mPerformAction) {
    nsresult rv;
    nsCOMPtr<sbILibrary> lib;
    rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                              NS_GET_IID(sbILibrary),
                              mDeviceLibrary,
                              NS_PROXY_SYNC,
                              getter_AddRefs(lib));
    NS_ENSURE_SUCCESS(rv, rv);

    return lib->CreateMediaItemIfNotExist(aContentUri,
                                aProperties,
                                aResultItem,
                                _retval);
  } else {
    return NS_OK;
  }
}

/*
 * See sbILibrary
 */
NS_IMETHODIMP
sbDeviceLibrary::CreateMediaList(const nsAString & aType,
                                 sbIPropertyArray *aProperties,
                                 sbIMediaList **_retval)
{
  NS_ASSERTION(mDeviceLibrary, "mDeviceLibrary is null, call init first.");
  SB_NOTIFY_LISTENERS_ASK_PERMISSION(OnBeforeCreateMediaList(aType,
                                                             aProperties,
                                                             &mShouldProcceed));
  if (mPerformAction) {
    return mDeviceLibrary->CreateMediaList(aType, aProperties, _retval);
  } else {
    return NS_OK;
  }
}

/*
 * See sbILibrary
 */
NS_IMETHODIMP
sbDeviceLibrary::GetDevice(sbIDevice **aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_IF_ADDREF(*aDevice = mDevice);
  return *aDevice ? NS_OK : NS_ERROR_UNEXPECTED;
}

/*
 * See sbILibrary
 */
NS_IMETHODIMP
sbDeviceLibrary::ClearItems()
{
  NS_ASSERTION(mDeviceLibrary, "mDeviceLibrary is null, call init first.");
  return mDeviceLibrary->ClearItems();
}

/*
 * See sbIMediaList
 */
NS_IMETHODIMP
sbDeviceLibrary::Add(sbIMediaItem *aMediaItem)
{
  NS_ASSERTION(mDeviceLibrary, "mDeviceLibrary is null, call init first.");
  SB_NOTIFY_LISTENERS_ASK_PERMISSION(OnBeforeAdd(aMediaItem, &mShouldProcceed));

  if (mPerformAction) {
    return mDeviceLibrary->Add(aMediaItem);
  } else {
    return NS_OK;
  }
}

/*
 * See sbIMediaList
 */
NS_IMETHODIMP
sbDeviceLibrary::AddAll(sbIMediaList *aMediaList)
{
  NS_ASSERTION(mDeviceLibrary, "mDeviceLibrary is null, call init first.");
  SB_NOTIFY_LISTENERS_ASK_PERMISSION(OnBeforeAddAll(aMediaList,
                                                    &mShouldProcceed));
  if (mPerformAction) {
    // XXX: not implemented!
    return mDeviceLibrary->AddAll(aMediaList);
  } else {
    return NS_OK;
  }
}

/*
 * See sbIMediaList
 */
NS_IMETHODIMP
sbDeviceLibrary::AddSome(nsISimpleEnumerator *aMediaItems)
{
  NS_ASSERTION(mDeviceLibrary, "mDeviceLibrary is null, call init first.");
  SB_NOTIFY_LISTENERS_ASK_PERMISSION(OnBeforeAddSome(aMediaItems,
                                                    &mShouldProcceed));

  if (mPerformAction) {
    return mDeviceLibrary->AddSome(aMediaItems);
  } else {
    return NS_OK;
  }
}

/*
 * See sbIMediaList
 */
NS_IMETHODIMP
sbDeviceLibrary::Clear(void)
{
  NS_ASSERTION(mDeviceLibrary, "mDeviceLibrary is null, call init first.");
  SB_NOTIFY_LISTENERS_ASK_PERMISSION(OnBeforeClear(&mShouldProcceed));
  if (mPerformAction) {
    return mDeviceLibrary->Clear();
  } else {
    return NS_OK;
  }
}
