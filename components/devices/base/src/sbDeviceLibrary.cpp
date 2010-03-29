/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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
#include <nsIPrefBranch.h>
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
#include <sbProxiedComponentManager.h>
#include <nsComponentManagerUtils.h>
#include <nsDirectoryServiceUtils.h>
#include <nsIFile.h>
#include <nsILocalFile.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsXPCOM.h>
#include <nsXPCOMCIDInternal.h>

/* songbird interfaces */
#include <sbIDevice.h>
#include <sbIDeviceCapabilities.h>
#include <sbIDeviceContent.h>
#include <sbIDeviceEvent.h>
#include <sbIDeviceEventTarget.h>
#include <sbIDeviceManager.h>
#include <sbIDeviceProperties.h>
#include <sbILibraryFactory.h>
#include <sbILibraryManager.h>
#include <sbILocalDatabaseSmartMediaList.h>
#include <sbIPrompter.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>

/* songbird headers */
#include <sbDeviceLibraryHelpers.h>
#include <sbDeviceUtils.h>
#include <sbLibraryUtils.h>
#include <sbLocalDatabaseCID.h>
#include <sbMemoryUtils.h>
#include <sbPropertiesCID.h>
#include <sbProxiedComponentManager.h>
#include <sbStandardDeviceProperties.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>
#include <sbVariantUtils.h>

#define SB_AUDIO_SMART_PLAYLIST "<sbaudio>"

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
#define PREF_SYNC_ROOT      "root"

const static char* gMediaType[] = {
  ".audio",
  ".video",
  ".image"
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
    mSyncSettingsChanged(PR_FALSE),
    mSyncModeChanged(PR_FALSE),
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
sbDeviceLibrary::SetMgmtTypePref(PRUint32 aContentType, PRUint32 aMgmtTypes)
{
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);

  nsresult rv;

  // Get the preference key.
  nsString prefKey;
  rv = GetMgmtTypePrefKey(aContentType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the preference.
  rv = mDevice->SetPreference(prefKey, sbNewVariant(aMgmtTypes));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceLibrary::GetMgmtTypePref(PRUint32 aContentType,
                                 PRUint32 & aMgmtTypes,
                                 PRUint32 aDefault)
{
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
  // If there is no value, set to aDefault
  if (dataType == nsIDataType::VTYPE_VOID ||
      dataType == nsIDataType::VTYPE_EMPTY)
  {
    aMgmtTypes = aDefault;
  } else {
    // has a value
    PRInt32 mgmtType;
    rv = var->GetAsInt32(&mgmtType);
    NS_ENSURE_SUCCESS(rv, rv);

    // Double check that it is a valid number
    NS_ENSURE_ARG_RANGE(mgmtType,
                        sbIDeviceLibrary::MGMT_TYPE_NONE,
                        sbIDeviceLibrary::MGMT_TYPE_MAX_VALUE);

    // Max the manual mode
    aMgmtTypes = mgmtType;
  }

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
  NS_ENSURE_FALSE(aContentType == sbIDeviceLibrary::MEDIATYPE_IMAGE,
                  NS_ERROR_NOT_IMPLEMENTED);

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

  // Setup the defaults for sync management
  PRUint32 mgmtSetting;
  rv = GetMgmtTypePref(sbIDeviceLibrary::MEDIATYPE_AUDIO,
                       mgmtSetting,
                       MGMT_PREF_UNINITIALIZED);
  NS_ENSURE_SUCCESS(rv, rv);
  if (mgmtSetting == 0) {
    mgmtSetting = sbIDeviceLibrary::MGMT_TYPE_MANUAL |
                    sbIDeviceLibrary::MGMT_TYPE_SYNC_ALL;
    rv = SetMgmtTypePref(sbIDeviceLibrary::MEDIATYPE_AUDIO, mgmtSetting);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = GetMgmtTypePref(sbIDeviceLibrary::MEDIATYPE_VIDEO,
                       mgmtSetting,
                       MGMT_PREF_UNINITIALIZED);
  NS_ENSURE_SUCCESS(rv, rv);
  if (mgmtSetting == 0) {
    mgmtSetting = sbIDeviceLibrary::MGMT_TYPE_MANUAL |
                    sbIDeviceLibrary::MGMT_TYPE_SYNC_PLAYLISTS;
    rv = SetMgmtTypePref(sbIDeviceLibrary::MEDIATYPE_VIDEO, mgmtSetting);
    NS_ENSURE_SUCCESS(rv, rv);
  }

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

  // get the current sync all setting.
  rv = GetIsMgmtTypeSyncAll(&mLastIsSyncAll);
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

  nsCOMPtr<nsIArray> syncPlaylistList;
  rv = GetSyncPlaylistList(getter_AddRefs(syncPlaylistList));
  NS_ENSURE_SUCCESS(rv,rv);

  PRUint32 playlistListCount;
  rv = syncPlaylistList->GetLength(&playlistListCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check to see if playlists are supported, and if not ignore
  bool const ignorePlaylists = !sbDeviceUtils::ArePlaylistsSupported(mDevice);
  // hook up the listener now if we need to
  mMainLibraryListener =
    new sbLibraryUpdateListener(mDeviceLibrary,
                                isManual != PR_FALSE,
                                playlistListCount ? syncPlaylistList : nsnull,
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

  nsresult rv;

  PRUint32 syncMode;
  rv = GetSyncMode(&syncMode);
  NS_ENSURE_SUCCESS(rv, rv);

  if (syncMode == SYNC_MANUAL) {
    *aIsMgmtTypeManual = PR_TRUE;
  }
  else {
    *aIsMgmtTypeManual = PR_FALSE;
  }

  return NS_OK;
}

nsresult
sbDeviceLibrary::GetIsMgmtTypeSyncList(PRBool* aIsMgmtTypeSyncList)
{
  NS_ASSERTION(aIsMgmtTypeSyncList, "aIsMgmtTypeSyncList is null");

  PRBool isSyncList = PR_FALSE;
  PRUint32 mgmtType;
  nsresult rv;
  for (PRUint32 i = 0; i < sbIDeviceLibrary::MEDIATYPE_COUNT; ++i) {
    // Ignore management type for images, it is always semi-manual
    if (i == sbIDeviceLibrary::MEDIATYPE_IMAGE)
      continue;

    rv = GetMgmtType(i, &mgmtType);
    NS_ENSURE_SUCCESS(rv, rv);

    // Manual mode.
    if (mgmtType & sbIDeviceLibrary::MGMT_TYPE_MANUAL) {
      break;
    }

    // Sync playlist mode.
    if (mgmtType == sbIDeviceLibrary::MGMT_TYPE_SYNC_PLAYLISTS) {
      isSyncList = PR_TRUE;
      break;
    }
  }

  *aIsMgmtTypeSyncList = isSyncList;
  return NS_OK;
}

nsresult
sbDeviceLibrary::GetIsMgmtTypeSyncAll(PRBool* aIsMgmtTypeSyncAll)
{
  NS_ASSERTION(aIsMgmtTypeSyncAll, "aIsMgmtTypeSyncAll is null");
  nsresult rv;

  // This is a hack for now, since we don't support video all yet.
  PRUint32 mgmtType;
  rv = GetMgmtTypePref(sbIDeviceLibrary::MEDIATYPE_AUDIO, mgmtType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mgmtType == sbIDeviceLibrary::MGMT_TYPE_SYNC_ALL) {
    *aIsMgmtTypeSyncAll = PR_TRUE;
  }
  else {
    *aIsMgmtTypeSyncAll = PR_FALSE;
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

  PRUint32 syncMode;
  rv = GetSyncMode(&syncMode);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isSyncedLocally = PR_FALSE;
  rv = GetIsSyncedLocally(&isSyncedLocally);
  NS_ENSURE_SUCCESS(rv, rv);

  // Not in manual mode.
  if (syncMode == SYNC_AUTO && isSyncedLocally) {
    PRBool isSyncAll = PR_FALSE;
    rv = GetIsMgmtTypeSyncAll(&isSyncAll);
    NS_ENSURE_SUCCESS(rv, rv);
    if (isSyncAll) {
      // hook up the metadata updating listener
      rv = mainLib->AddListener(mMainLibraryListener,
                                PR_FALSE,
                                sbIMediaList::LISTENER_FLAGS_ITEMADDED |
                                sbIMediaList::LISTENER_FLAGS_BEFOREITEMREMOVED |
                                sbIMediaList::LISTENER_FLAGS_ITEMUPDATED,
                                mMainLibraryListenerFilter);
      NS_ENSURE_SUCCESS(rv, rv);

      mMainLibraryListener->SetSyncMode(false, nsnull);

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
      nsCOMPtr<nsIArray> playlistList;
      rv = GetSyncPlaylistList(getter_AddRefs(playlistList));
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 length;
      rv = playlistList->GetLength(&length);
      NS_ENSURE_SUCCESS(rv, rv);

      // Need to stop listening to all the playlists so we can listen to just
      // the selected ones
      mMainLibraryListener->StopListeningToPlaylists();

      mMainLibraryListener->SetSyncMode(false, length ? playlistList : nsnull);

      for (PRUint32 index = 0; index < length; ++index) {
        nsCOMPtr<sbIMediaItem> item = do_QueryElementAt(playlistList, index, &rv);
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
    mMainLibraryListener->StopListeningToPlaylists();

    mMainLibraryListener->SetSyncMode(syncMode == SYNC_MANUAL, nsnull);

    // remove the metadata updating listener
    rv = mainLib->RemoveListener(mMainLibraryListener);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbDeviceLibrary::UpdateIsReadOnly()
{
  nsresult rv;

  // Get the device properties.
  nsCOMPtr<sbIDeviceProperties> baseDeviceProperties;
  nsCOMPtr<nsIPropertyBag2>     deviceProperties;
  rv = mDevice->GetProperties(getter_AddRefs(baseDeviceProperties));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = baseDeviceProperties->GetProperties(getter_AddRefs(deviceProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the access compatibility.
  nsAutoString accessCompatibility;
  rv = deviceProperties->GetPropertyAsAString
         (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_ACCESS_COMPATIBILITY),
          accessCompatibility);
  if (NS_FAILED(rv))
    accessCompatibility.Truncate();

  // If device is read-only, mark library as such and return.
  if (accessCompatibility.Equals(NS_LITERAL_STRING("ro"))) {
    rv = this->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_ISREADONLY),
                           NS_LITERAL_STRING("1"));
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

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
  NS_ENSURE_FALSE(aContentType == sbIDeviceLibrary::MEDIATYPE_IMAGE,
                  NS_ERROR_NOT_IMPLEMENTED);
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
  NS_ENSURE_STATE(mDeviceLibrary);

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
  NS_ENSURE_STATE(mDeviceLibrary);

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
sbDeviceLibrary::GetSyncRootPrefKey(PRUint32 aContentType, nsAString& aPrefKey)
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
  aPrefKey.AppendLiteral(PREF_SYNC_ROOT);
  aPrefKey.AppendLiteral(gMediaType[aContentType]);

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

  PRUint32 mgmtType;
  rv = GetMgmtTypePref(aContentType, mgmtType);
  NS_ENSURE_SUCCESS(rv, rv);

  // Max the manual mode
  *aMgmtType = mgmtType & ~sbIDeviceLibrary::MGMT_TYPE_MANUAL;

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::GetSyncMode(PRUint32 * aSyncMode)
{
  NS_ENSURE_ARG_POINTER(aSyncMode);

  nsresult rv;
  PRUint32 syncMode = SYNC_AUTO;
  for (PRUint32 i = 0; i < MEDIATYPE_COUNT; ++i) {
    // Ignore management type for images, it is always semi-manual
    if (i == sbIDeviceLibrary::MEDIATYPE_IMAGE)
      continue;

    PRUint32 mgmtType;
    rv = GetMgmtTypePref(i, mgmtType);
    NS_ENSURE_SUCCESS(rv, rv);

    // Manual mode.
    if (mgmtType & MGMT_TYPE_MANUAL) {
      syncMode = SYNC_MANUAL;
      break;
    }
  }
  *aSyncMode = syncMode;

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::SetSyncMode(PRUint32 aSyncMode)
{
  NS_ENSURE_STATE(mDevice);
  nsresult rv;

  PRUint32 syncMode;
  rv = GetSyncMode(&syncMode);
  NS_ENSURE_SUCCESS(rv, rv);

  // If we're not changing anything then do nothing
  if (aSyncMode == syncMode) {
    return NS_OK;
  }

  nsCOMPtr<nsIPrefBranch> rootPrefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevice> device;
  rv = GetDevice(getter_AddRefs(device));
  NS_ENSURE_SUCCESS(rv, rv);

  nsID* id;
  rv = device->GetId(&id);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString key(NS_LITERAL_CSTRING("songbird.device."));
  key.AppendLiteral(id->ToString());
  key.AppendLiteral(".syncmode.changed");

  PRBool hasValue = PR_FALSE;
  rv = rootPrefBranch->PrefHasUserValue(key.get(), &hasValue);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isChanged = PR_FALSE;
  if (hasValue) {
    rv = rootPrefBranch->GetBoolPref(key.get(), &isChanged);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // If switching from manual management mode to sync mode, confirm with user
  // before proceeding.  Do nothing more if switch is canceled.
  if (aSyncMode == sbIDeviceLibrary::SYNC_AUTO && !isChanged) {
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
      rv = sbDeviceUtils::SyncModeOrPartnerChange(mDevice, &cancelSwitch);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Do nothing more is switch canceled.
    if (cancelSwitch)
      return NS_OK;
  }

  for (PRUint32 i = 0; i < sbIDeviceLibrary::MEDIATYPE_COUNT; ++i) {
    // Skip images - not managed by playlists
    if (i == sbIDeviceLibrary::MEDIATYPE_IMAGE)
      continue;

    // figure out the old pref first
    PRUint32 origMgmtType;
    rv = GetMgmtTypePref(i, origMgmtType);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 newMgmtType = origMgmtType & ~sbIDeviceLibrary::MGMT_TYPE_MANUAL;
    if (aSyncMode == sbIDeviceLibrary::SYNC_MANUAL) {
      newMgmtType |= sbIDeviceLibrary::MGMT_TYPE_MANUAL;
    }
    rv = SetMgmtTypePref(i, newMgmtType);
  }

  if (aSyncMode == sbIDeviceLibrary::SYNC_MANUAL && !mSyncModeChanged) {
    // update the library is read-only property
    rv = UpdateIsReadOnly();
    NS_ENSURE_SUCCESS(rv, rv);

    // update the main library listeners
    rv = UpdateMainLibraryListeners();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  // Delay the updates for toggling from manual to auto
  else if (aSyncMode == sbIDeviceLibrary::SYNC_AUTO) {
    mSyncModeChanged = PR_TRUE;
  }
  // Skip the updates for canceling the sync mode change and toggle back
  // to manual.
  else if (mSyncModeChanged) {
    mSyncModeChanged = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::GetSyncModeChanged(PRBool *aSyncModeChanged)
{
  NS_ENSURE_ARG_POINTER(aSyncModeChanged);
  *aSyncModeChanged = mSyncModeChanged;

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
  NS_ENSURE_TRUE(aMgmtType == MGMT_TYPE_NONE ||
                 aMgmtType == MGMT_TYPE_SYNC_ALL ||
                 aMgmtType == MGMT_TYPE_SYNC_PLAYLISTS,
                 NS_ERROR_INVALID_ARG);

  nsresult rv;

  PRUint32 oldMgmtType;
  rv = GetMgmtTypePref(aContentType, oldMgmtType);
  NS_ENSURE_SUCCESS(rv, rv);

  // Preserve the existing manual flag, we're only setting the type of sync
  // not whether to sync or not
  aMgmtType = (aMgmtType & ~sbIDeviceLibrary::MGMT_TYPE_MANUAL) |
              (oldMgmtType & sbIDeviceLibrary::MGMT_TYPE_MANUAL);

  rv = SetMgmtTypePref(aContentType, aMgmtType);
  NS_ENSURE_SUCCESS(rv, rv);

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
  NS_ENSURE_FALSE(aContentType == sbIDeviceLibrary::MEDIATYPE_IMAGE,
                  NS_ERROR_NOT_IMPLEMENTED);

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
  if (end < 0) {
    end = listGuidsCSV.Length();
  }
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

  return CallQueryInterface(array, _retval);
}

NS_IMETHODIMP
sbDeviceLibrary::SetSyncPlaylistListByType(PRUint32 aContentType,
                                           nsIArray *aPlaylistList)
{
  NS_ENSURE_ARG_POINTER(aPlaylistList);
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);
  NS_ENSURE_FALSE(aContentType == sbIDeviceLibrary::MEDIATYPE_IMAGE,
                  NS_ERROR_NOT_IMPLEMENTED);

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
  NS_ENSURE_FALSE(aContentType == sbIDeviceLibrary::MEDIATYPE_IMAGE,
                  NS_ERROR_NOT_IMPLEMENTED);
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
  NS_ENSURE_FALSE(aContentType == sbIDeviceLibrary::MEDIATYPE_IMAGE,
                  NS_ERROR_NOT_IMPLEMENTED);
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
sbDeviceLibrary::GetSyncRootFolderByType(PRUint32 aContentType,
                                         nsIFile **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);
  NS_ENSURE_TRUE(aContentType == sbIDeviceLibrary::MEDIATYPE_IMAGE,
                 NS_ERROR_NOT_IMPLEMENTED);
  nsresult rv;

  // Get the preference
  nsString prefKey;
  rv = GetSyncRootPrefKey(aContentType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> var;
  rv = mDevice->GetPreference(prefKey, getter_AddRefs(var));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString folderPath;
  rv = var->GetAsAString(folderPath);
  NS_ENSURE_SUCCESS(rv, rv);

  if (folderPath.IsEmpty()) {
    *_retval = nsnull;
    return NS_OK;
  }

  // Convert folder path into a file object
  nsCOMPtr<nsILocalFile> folder;
  rv = NS_NewLocalFile(folderPath, PR_TRUE, getter_AddRefs(folder));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = folder);

  return NS_OK;  
}

NS_IMETHODIMP
sbDeviceLibrary::SetSyncRootFolderByType(PRUint32 aContentType,
                                         nsIFile *aFolder)
{
  NS_ENSURE_ARG_POINTER(aFolder);
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);
  NS_ENSURE_TRUE(aContentType == sbIDeviceLibrary::MEDIATYPE_IMAGE,
                 NS_ERROR_NOT_IMPLEMENTED);
  nsresult rv;

  // Get folder path
  nsAutoString folderPath;
  rv = aFolder->GetPath(folderPath);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the preference key
  nsString prefKey;
  rv = GetSyncRootPrefKey(aContentType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  // Write the preference
  rv = mDevice->SetPreference(prefKey, sbNewVariant(folderPath));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;  
}

NS_IMETHODIMP
sbDeviceLibrary::GetSyncFolderListByType(PRUint32 aContentType,
                                         nsIArray **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);
  NS_ENSURE_TRUE(aContentType == sbIDeviceLibrary::MEDIATYPE_IMAGE,
                 NS_ERROR_NOT_IMPLEMENTED);
  nsresult rv;

  // Create an array for the result
  nsCOMPtr<nsIMutableArray> array =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mgmtType;
  rv = GetMgmtType(aContentType, &mgmtType);
  NS_ENSURE_SUCCESS(rv, rv);

  // If all items, simply add the root folder to the list
  if (mgmtType == sbIDeviceLibrary::MGMT_TYPE_SYNC_ALL) {
    nsCOMPtr<nsIFile> rootFolder;
    rv = GetSyncRootFolderByType(aContentType, getter_AddRefs(rootFolder));
    NS_ENSURE_SUCCESS(rv, rv);

    if (rootFolder) {
      rv = array->AppendElement(rootFolder, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else if (mgmtType == sbIDeviceLibrary::MGMT_TYPE_SYNC_PLAYLISTS) {
    // Get the preference
    nsString prefKey;
    rv = GetSyncListsPrefKey(aContentType, prefKey);
    NS_ENSURE_SUCCESS(rv, rv);
  
    nsCOMPtr<nsIVariant> var;
    rv = mDevice->GetPreference(prefKey, getter_AddRefs(var));
    NS_ENSURE_SUCCESS(rv, rv);
  
    nsAutoString foldersDSV;
    rv = var->GetAsAString(foldersDSV);
    NS_ENSURE_SUCCESS(rv, rv);
  
    // Scan folder list DSV for folder paths and add them to the array
    PRInt32 start = 0;
    PRInt32 end = foldersDSV.FindChar('\1', start);
    if (end < 0) {
      end = foldersDSV.Length();
    }
    while (end > start) {
      // Extract the folder path
      nsDependentSubstring folderPath = Substring(foldersDSV, start, end - start);
  
      nsCOMPtr<nsILocalFile> folder;
      rv = NS_NewLocalFile(folderPath, PR_TRUE, getter_AddRefs(folder));
      if (NS_FAILED(rv))  // Invalid path, skip
        continue;
  
      rv = array->AppendElement(folder, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
  
      // Scan for the next folder path
      start = end + 1;
      end = foldersDSV.FindChar('\1', start);
      if (end < 0)
        end = foldersDSV.Length();
    }
  }

  NS_ADDREF(*_retval = array);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::SetSyncFolderListByType(PRUint32 aContentType,
                                         nsIArray *aFolderList)
{
  NS_ENSURE_ARG_POINTER(aFolderList);
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);
  NS_ENSURE_TRUE(aContentType == sbIDeviceLibrary::MEDIATYPE_IMAGE,
                 NS_ERROR_NOT_IMPLEMENTED);
  nsresult rv;

  // Get the number of sync folders
  PRUint32 length;
  rv = aFolderList->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  // Produce the sync folder list DSV
  nsAutoString foldersDSV;
  for (PRUint32 i = 0; i < length; i++) {
    // Get the next sync folder
    nsCOMPtr<nsIFile> folder = do_QueryElementAt(aFolderList, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the folder path
    nsAutoString folderPath;
    rv = folder->GetPath(folderPath);
    NS_ENSURE_SUCCESS(rv, rv);

    // Add the folder to the list of sync folders
    if (i > 0)
      foldersDSV.AppendLiteral("\1");
    foldersDSV.Append(folderPath);
  }

  // Get the preference key
  nsString prefKey;
  rv = GetSyncListsPrefKey(aContentType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  // Write the preference
  rv = mDevice->SetPreference(prefKey, sbNewVariant(foldersDSV));
  NS_ENSURE_SUCCESS(rv, rv);

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

  PRBool isManual = PR_FALSE;
  rv = GetIsMgmtTypeManual(&isManual);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (!isManual) {
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

    // Update if sync mode is changed to SYNC_AUTO.
    if (mSyncModeChanged) {
      // update the library is read-only property
      rv = UpdateIsReadOnly();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (mSyncSettingsChanged || mSyncModeChanged) {
      // update the main library listeners
      rv = UpdateMainLibraryListeners();
      NS_ENSURE_SUCCESS(rv, rv);

      // No harm to reset both to false
      mSyncSettingsChanged = PR_FALSE;
      mSyncModeChanged = PR_FALSE;
    }
  }
  
  // If the user has enabled image sync, trigger it after the audio/video sync
  PRUint32 mgmtType;
  rv = GetMgmtType(sbIDeviceLibrary::MEDIATYPE_IMAGE, &mgmtType);
  NS_ENSURE_SUCCESS(rv, rv);
  if (mgmtType != sbIDeviceLibrary::MGMT_TYPE_NONE) {
    nsCOMPtr<nsIWritablePropertyBag2> requestParams =
      do_CreateInstance(NS_HASH_PROPERTY_BAG_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    // make a low priority request so it's guaranteed to be handled after
    // higher priority requests, such as audio and video
    rv = requestParams->SetPropertyAsInt32(
                            NS_LITERAL_STRING("priority"),
                            sbBaseDevice::TransferRequest::PRIORITY_LOW);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = requestParams->SetPropertyAsInterface
                          (NS_LITERAL_STRING("list"),
                           NS_ISUPPORTS_CAST(sbIMediaList*, this));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = device->SubmitRequest(sbIDevice::REQUEST_IMAGESYNC, requestParams);
    NS_ENSURE_SUCCESS(rv, rv);
  }

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
  nsresult rv = do_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
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
 * Returns the equality operator for the content property
 */
static nsresult
GetEqualOperator(sbIPropertyOperator ** aOperator)
{

  nsresult rv;

  nsCOMPtr<sbIPropertyManager> manager =
    do_GetService("@songbirdnest.com/Songbird/Properties/PropertyManager;1",
                  &rv);
  nsCOMPtr<sbIPropertyInfo> info;
  rv = manager->GetPropertyInfo(NS_LITERAL_STRING(SB_PROPERTY_CONTENTTYPE),
                                getter_AddRefs(info));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString opName;
  rv = info->GetOPERATOR_EQUALS(opName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the operator.
  rv = info->GetOperator(opName, aOperator);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * This returns the existing audio smart playlist or creates a new one if
 * not found
 */
static nsresult
GetOrCreateAudioSmartMediaList(sbIMediaList ** aAudioMediaList)
{
  NS_ENSURE_ARG_POINTER(aAudioMediaList);

  nsresult rv;

  nsCOMPtr<sbILibraryManager> libManager =
    do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create a smart playlist with just audio files
  nsCOMPtr<sbILibrary> mainLibrary;
  rv = libManager->GetMainLibrary(getter_AddRefs(mainLibrary));

  nsCOMPtr<sbIMutablePropertyArray> properties =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  rv = properties->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                                  NS_LITERAL_STRING("1"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = properties->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_MEDIALISTNAME),
                                  NS_LITERAL_STRING(SB_AUDIO_SMART_PLAYLIST));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 count = 0;
  nsCOMPtr<nsIArray> smartMediaLists;
  rv = mainLibrary->GetItemsByProperties(properties,
                                         getter_AddRefs(smartMediaLists));
  if (rv != NS_ERROR_NOT_AVAILABLE) {
    NS_ENSURE_SUCCESS(rv, rv);

    rv = smartMediaLists->GetLength(&count);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_WARN_IF_FALSE(count < 2, "Multiple audio sync'ing playlists");
  if (count > 0) {
    nsCOMPtr<sbIMediaList> list = do_QueryElementAt(smartMediaLists,
                                                    0,
                                                    &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbILocalDatabaseSmartMediaList> audioList =
      do_QueryInterface(list, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = audioList->Rebuild();
    NS_ENSURE_SUCCESS(rv, rv);

    return CallQueryInterface(list, aAudioMediaList);
  }
  nsCOMPtr<nsIThread> target;
  rv = NS_GetMainThread(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> proxiedLibrary;
  rv = do_GetProxyForObject(target,
                            mainLibrary.get(),
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(proxiedLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> mediaList;
  rv = proxiedLibrary->CreateMediaList(NS_LITERAL_STRING("smart"),
                                       properties,
                                       getter_AddRefs(mediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabaseSmartMediaList> audioList =
    do_QueryInterface(mediaList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyOperator> equal;
  rv = GetEqualOperator(getter_AddRefs(equal));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabaseSmartMediaListCondition> condition;
  rv = audioList->AppendCondition(NS_LITERAL_STRING(SB_PROPERTY_CONTENTTYPE),
                                  equal,
                                  NS_LITERAL_STRING("audio"),
                                  nsString(),
                                  nsString(),
                                  getter_AddRefs(condition));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = audioList->Rebuild();
  NS_ENSURE_SUCCESS(rv, rv);

  mediaList.forget(aAudioMediaList);

  return NS_OK;
}

/**
 * Checks to see if a |nsIArray| instance contains a specific |sbIMediaList|
 * by comparing the list guid with the guids in the array.
 */
static PRBool
DoesArrayContainMediaList(nsIArray *aArray, sbIMediaList *aMediaList)
{
  NS_ENSURE_TRUE(aArray, PR_FALSE);
  NS_ENSURE_TRUE(aMediaList, PR_FALSE);

  PRBool found = PR_FALSE;
  PRUint32 length = 0;
  nsresult rv = aArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  nsString searchListGuid;
  rv = aMediaList->GetGuid(searchListGuid);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  for (PRUint32 i = 0; i < length && !found; i++) {
    nsCOMPtr<sbIMediaList> curList =
      do_QueryElementAt(aArray, i, &rv);
    if (NS_FAILED(rv) || !curList) {
      continue;
    }

    nsString curListGuid;
    rv = curList->GetGuid(curListGuid);
    if (NS_FAILED(rv)) {
      continue;
    }

    if (curListGuid.Equals(searchListGuid)) {
      found = PR_TRUE;
    }
  }

  return found;
}

NS_IMETHODIMP
sbDeviceLibrary::GetSyncPlaylistList(nsIArray ** aPlaylistList)
{
  nsresult rv;

  nsCOMPtr<nsIMutableArray> allPlaylistList =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Determine if the sync list is for all "music" playlists.
  // NOTE: This also counts for normal, mixed playlists as well.
  PRBool isSyncAll;
  rv = GetIsMgmtTypeSyncAll(&isSyncAll);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isSyncAll) {
    // If the "sync all" option is turned on, add every audio medialist and
    // every normal mixed playlist.
    nsCOMPtr<sbIMediaList> mediaList;
    rv = GetOrCreateAudioSmartMediaList(getter_AddRefs(mediaList));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = allPlaylistList->AppendElement(mediaList, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbILibraryManager> libManager =
      do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbILibrary> mainLibrary;
    rv = libManager->GetMainLibrary(getter_AddRefs(mainLibrary));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMutablePropertyArray> propArray =
      do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = propArray->AppendProperty(
      NS_LITERAL_STRING(SB_PROPERTY_ISLIST),
      NS_LITERAL_STRING("1"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = propArray->AppendProperty(
      NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
      NS_LITERAL_STRING("0"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIArray> mainLibraryPlaylists;
    rv = mainLibrary->GetItemsByProperties(
      propArray,
      getter_AddRefs(mainLibraryPlaylists));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 length = 0;
    rv = mainLibraryPlaylists->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool deviceSupportsVideo = sbDeviceUtils::GetDoesDeviceSupportContent(
      mDevice,
      sbIDeviceCapabilities::CONTENT_VIDEO,
      sbIDeviceCapabilities::FUNCTION_VIDEO_PLAYBACK);

    for (PRUint32 i = 0; i < length; i++) {
      nsCOMPtr<sbIMediaList> curList =
        do_QueryElementAt(mainLibraryPlaylists, i, &rv);
      if (NS_FAILED(rv) || !curList) {
        continue;
      }

      nsString listName;
      curList->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_MEDIALISTNAME), listName);

      // First, ensure that the content type is audio.
      PRUint16 listContentType;
      rv = curList->GetListContentType(&listContentType);
      if (NS_FAILED(rv) || listContentType == sbIMediaList::CONTENTTYPE_VIDEO) {
        continue;
      }

      if (listContentType == sbIMediaList::CONTENTTYPE_AUDIO) {
        rv = allPlaylistList->AppendElement(curList, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (deviceSupportsVideo &&
               listContentType == sbIMediaList::CONTENTTYPE_MIX)
      {
        rv = allPlaylistList->AppendElement(curList, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  // The user has opted to manually select which playlists are going to get
  // sycn'd to the device.
  nsCOMPtr<nsIArray> playlistList;
  for (PRUint32 i = 0; i < sbIDeviceLibrary::MEDIATYPE_COUNT; ++i) {
    PRUint32 mgmtType;
    rv = GetMgmtType(i, &mgmtType);
    NS_ENSURE_SUCCESS(rv, rv);

    // Skip images - not managed by playlists
    if (i == sbIDeviceLibrary::MEDIATYPE_IMAGE ||
        mgmtType != sbIDeviceLibrary::MGMT_TYPE_SYNC_PLAYLISTS)
    {
      continue;
    }

    // Get an array of playlists that the user has selected to sync based on
    // the device preferences.
    // NOTE: These preferences are set in the device sync page.
    rv = GetSyncPlaylistListByType(i, getter_AddRefs(playlistList));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 length;
    rv = playlistList->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);
    for (PRUint32 index = 0; index < length; ++index) {
      nsCOMPtr<sbIMediaList> playlist =
        do_QueryElementAt(playlistList, index, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      // Ensure that the playlist doesn't already exist in the array.
      if (!DoesArrayContainMediaList(allPlaylistList, playlist)) {
        rv = allPlaylistList->AppendElement(playlist, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  return CallQueryInterface(allPlaylistList, aPlaylistList);
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
    // get the sync all setting
    PRBool isSyncAll = PR_FALSE;
    rv = GetIsMgmtTypeSyncAll(&isSyncAll);
    NS_ENSURE_SUCCESS(rv, rv);

    // Save the flag if the sync setting changes to update the main library
    // listeners later.
    if (isSyncAll != mLastIsSyncAll) {
      mSyncSettingsChanged = PR_TRUE;
      mLastIsSyncAll = isSyncAll;
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
    rv = do_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
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
    rv = do_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
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
