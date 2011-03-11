/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
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
#include "sbDeviceLibrarySyncSettings.h"
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

char const * const gMediaType[] = {
  ".audio",
  ".video",
  ".image"
};


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
    mMonitor(nsnull)
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

  if(mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
  }

  TRACE(("DeviceLibrary[0x%.8x] - Destructed", this));
}

NS_IMETHODIMP
sbDeviceLibrary::Initialize(const nsAString& aLibraryId)
{
  NS_ENSURE_FALSE(mMonitor, NS_ERROR_ALREADY_INITIALIZED);
  mMonitor = nsAutoMonitor::NewMonitor(__FILE__ "sbDeviceLibrary::mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);
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

  // remove device library listener
  if (mDeviceLibrary) {
    nsCOMPtr<sbIMediaList> list = do_QueryInterface(mDeviceLibrary);
    if (list)
      list->RemoveListener(this);
  }

  if (mDeviceLibrary)
    UnregisterDeviceLibrary();

  // let go of the owner device
  mDevice = nsnull;

  // Don't null out mDeviceLibrary since there may be listeners on it that still
  // need to be removed.

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

  // do not listen to hidden list
  nsString hidden;
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                          hidden);
  NS_ENSURE_SUCCESS(rv, rv);

  if ((customType.IsEmpty() ||
       customType.EqualsLiteral("simple") ||
       customType.EqualsLiteral("smart")) &&
      !hidden.EqualsLiteral("1")) {
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

  nsCOMPtr<sbIDeviceLibrarySyncSettings> syncSettings;
  rv = GetSyncSettings(getter_AddRefs(syncSettings));
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

  // add a device event listener to listen for changes to the is synced locally
  // state
  nsCOMPtr<sbIDeviceEventTarget>
    deviceEventTarget = do_QueryInterface(mDevice, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deviceEventTarget->AddEventListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> syncPlaylistList;
  rv = syncSettings->GetSyncPlaylists(getter_AddRefs(syncPlaylistList));
  NS_ENSURE_SUCCESS(rv,rv);

  PRUint32 playlistListCount;
  rv = syncPlaylistList->GetLength(&playlistListCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check to see if playlists are supported, and if not ignore
  bool const ignorePlaylists = !sbDeviceUtils::ArePlaylistsSupported(mDevice);
  // hook up the listener now if we need to
  mMainLibraryListener =
    new sbLibraryUpdateListener(mDeviceLibrary,
                                PR_TRUE,
                                nsnull,
                                ignorePlaylists,
                                mDevice);
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
  rv = UpdateMainLibraryListeners(syncSettings);
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
sbDeviceLibrary::GetIsMgmtTypeSyncList(PRBool* aIsMgmtTypeSyncList)
{
  NS_ASSERTION(aIsMgmtTypeSyncList, "aIsMgmtTypeSyncList is null");
  nsresult rv;

  nsCOMPtr<sbIDeviceLibrarySyncSettings> syncSettings;
  rv = GetSyncSettings(getter_AddRefs(syncSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isSyncList = PR_FALSE;
  PRUint32 mgmtType;
  for (PRUint32 mediaType = 0;
       mediaType <= sbIDeviceLibrary::MEDIATYPE_COUNT;
       ++mediaType) {
    // Ignore management type for images, it is always semi-manual
    if (mediaType == sbIDeviceLibrary::MEDIATYPE_IMAGE)
      continue;

    nsCOMPtr<sbIDeviceLibraryMediaSyncSettings> mediaSyncSettings;
    rv = syncSettings->GetMediaSettings(mediaType,
                                        getter_AddRefs(mediaSyncSettings));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mediaSyncSettings->GetMgmtType(&mgmtType);
    NS_ENSURE_SUCCESS(rv, rv);

    // Manual mode.
    if (mgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_NONE) {
      break;
    }

    // Sync playlist mode.
    if (mgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_PLAYLISTS) {
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

  nsCOMPtr<sbIDeviceLibrarySyncSettings> syncSettings;
  rv = GetSyncSettings(getter_AddRefs(syncSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceLibraryMediaSyncSettings> mediaSyncSettings;
  rv = syncSettings->GetMediaSettings(sbIDeviceLibrary::MEDIATYPE_AUDIO,
                                      getter_AddRefs(mediaSyncSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  // This is a hack for now, since we don't support video all yet.
  PRUint32 mgmtType;
  rv = mediaSyncSettings->GetMgmtType(&mgmtType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_ALL) {
    *aIsMgmtTypeSyncAll = PR_TRUE;
  }
  else {
    *aIsMgmtTypeSyncAll = PR_FALSE;
  }

  return NS_OK;
}

nsresult
sbDeviceLibrary::UpdateMainLibraryListeners(
                                   sbIDeviceLibrarySyncSettings * aSyncSettings)
{
  NS_ENSURE_STATE(mDevice);

  nsresult rv;

  nsCOMPtr<sbILibrary> mainLib;
  rv = GetMainLibrary(getter_AddRefs(mainLib));
  NS_ENSURE_SUCCESS(rv, rv);

  // TODO: XXX This code will probably need to be updated with the device
  // listeners bug 23188
  // hook up the metadata updating listener
  rv = mainLib->AddListener(mMainLibraryListener,
                            PR_FALSE,
                            sbIMediaList::LISTENER_FLAGS_ITEMADDED |
                            sbIMediaList::LISTENER_FLAGS_BEFOREITEMREMOVED |
                            sbIMediaList::LISTENER_FLAGS_ITEMUPDATED,
                            mMainLibraryListenerFilter);
  NS_ENSURE_SUCCESS(rv, rv);

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

  // Mark library as read-write (TODO: XXX Not sure if we still need to do this
  // but leaving it in as it doesn't hurt anything)
  nsString str;
  str.SetIsVoid(PR_TRUE);
  rv = this->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_ISREADONLY), str);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

sbDeviceLibrarySyncSettings *
sbDeviceLibrary::CreateSyncSettings()
{
  nsresult rv;

  nsString guid;
  rv = GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsID* deviceID;
  rv = mDevice->GetId(&deviceID);
  NS_ENSURE_SUCCESS(rv, nsnull);
  sbAutoNSMemPtr autoDeviceID(deviceID);

  return sbDeviceLibrarySyncSettings::New(*deviceID, guid);
}
/**
 * sbIDeviceLibrary
 */

NS_IMETHODIMP
sbDeviceLibrary::GetSyncSettings(sbIDeviceLibrarySyncSettings ** aSyncSettings)
{
  NS_ENSURE_ARG_POINTER(aSyncSettings);

  nsresult rv;

  nsAutoMonitor lock(mMonitor);
  if (!mCurrentSyncSettings) {
    mCurrentSyncSettings = CreateSyncSettings();
    NS_ENSURE_TRUE(mCurrentSyncSettings, NS_ERROR_OUT_OF_MEMORY);

    rv = mCurrentSyncSettings->Read(mDevice, this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsRefPtr<sbDeviceLibrarySyncSettings> settings;
  rv = mCurrentSyncSettings->CreateCopy(getter_AddRefs(settings));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CallQueryInterface(settings.get(), aSyncSettings);
  NS_ENSURE_SUCCESS(rv, rv);

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

#define SB_NOTIFY_LISTENERS(call)                                             \
  nsCOMArray<sbIDeviceLibraryListener> listeners;                             \
  {                                                                           \
    nsAutoMonitor monitor(mMonitor);                                          \
    mListeners.EnumerateRead(AddListenersToCOMArrayCallback, &listeners);     \
  }                                                                           \
                                                                              \
  PRInt32 count = listeners.Count();                                          \
  for (PRInt32 index = 0; index < count; index++) {                           \
    nsCOMPtr<sbIDeviceLibraryListener> listener = listeners.ObjectAt(index);  \
    NS_ASSERTION(listener, "Null listener!");                                 \
    listener->call;                                                           \
  }                                                                           \

// SB_NOTIFY_LISTENERS sets a return value according to the return value of the
// last listener call. This allows a single listener to suppress batch
// notifications to all other listeners. Use SB_NOTIFY_LISTENERS_RETURN_FALSE
// to prevent erroneous suppression of notifications.
#define SB_NOTIFY_LISTENERS_RETURN_FALSE(call)                                \
  SB_NOTIFY_LISTENERS(call)                                                   \
  *aNoMoreForBatch = PR_FALSE;                                                \

#define SB_NOTIFY_LISTENERS_ASK_PERMISSION(call)                              \
  PRBool mShouldProcceed = PR_TRUE;                                           \
  PRBool mPerformAction = PR_TRUE;                                            \
                                                                              \
  nsCOMArray<sbIDeviceLibraryListener> listeners;                             \
  {                                                                           \
    nsAutoMonitor monitor(mMonitor);                                          \
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
sbDeviceLibrary::SetSyncSettings(sbIDeviceLibrarySyncSettings * aSyncSettings)
{
  NS_ENSURE_ARG_POINTER(aSyncSettings);

  nsresult rv = SetSyncSettingsNoLock(aSyncSettings);
  NS_ENSURE_SUCCESS(rv, rv);

  // update the library is read-only property
  rv = UpdateIsReadOnly();
  NS_ENSURE_SUCCESS(rv, rv);

  // update the main library listeners
  rv = UpdateMainLibraryListeners(mCurrentSyncSettings);
  NS_ENSURE_SUCCESS(rv, rv);

  SB_NOTIFY_LISTENERS(OnSyncSettings(
                                sbIDeviceLibraryListener::SYNC_SETTINGS_APPLIED,
                                mCurrentSyncSettings));

  return NS_OK;
}

nsresult
sbDeviceLibrary::SetSyncSettingsNoLock(
                                   sbIDeviceLibrarySyncSettings * aSyncSettings)
{
  NS_ENSURE_ARG_POINTER(aSyncSettings);

  nsresult rv;

  // A bit of a hack, we can get away with this since there will
  // never be a tearoff of this simple object.
  sbDeviceLibrarySyncSettings * syncSettings =
    static_cast<sbDeviceLibrarySyncSettings*>(aSyncSettings);

  // Lock for both assignment and the reading the aSyncSettings object
  nsAutoMonitor monitor(mMonitor);

  nsAutoLock lock(syncSettings->GetLock());

  if (syncSettings->HasChangedNoLock()) {
    if (mCurrentSyncSettings) {
      rv = mCurrentSyncSettings->Assign(syncSettings);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = syncSettings->CreateCopy(getter_AddRefs(mCurrentSyncSettings));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // if we're not being assigned the temp settings we need
    // to update it. We'll cheat and just test the raw pointers
    if (mTempSyncSettings && mTempSyncSettings != syncSettings) {
      nsAutoLock lock(mTempSyncSettings->GetLock());
      rv = mTempSyncSettings->Assign(syncSettings);
      NS_ENSURE_SUCCESS(rv, rv);

      mTempSyncSettings->NotifyDeviceLibrary();
      mTempSyncSettings->ResetChangedNoLock();
    }
    NS_ENSURE_SUCCESS(rv, rv);

    // Create a copy of sync settings for Write. We don't want to hold the lock
    // while writing as it can dispatch device EVENT_DEVICE_PREFS_CHANGED event.
    nsRefPtr<sbDeviceLibrarySyncSettings> copiedSyncSettings;
    rv = mCurrentSyncSettings->CreateCopy(getter_AddRefs(copiedSyncSettings));
    NS_ENSURE_SUCCESS(rv, rv);

    // Release the lock before dispatching sync settings change event
    lock.unlock();
    monitor.Exit();

    rv = copiedSyncSettings->Write(mDevice);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::GetTempSyncSettings(
                               sbIDeviceLibrarySyncSettings **aTempSyncSettings)
{
  NS_ENSURE_ARG_POINTER(aTempSyncSettings);

  nsresult rv;

  nsAutoMonitor monitor(mMonitor);
  if (!mTempSyncSettings) {
    if (!mCurrentSyncSettings) {
      mCurrentSyncSettings = CreateSyncSettings();
      NS_ENSURE_TRUE(mCurrentSyncSettings, NS_ERROR_OUT_OF_MEMORY);
    }
    mCurrentSyncSettings->CreateCopy(getter_AddRefs(mTempSyncSettings));
    mTempSyncSettings->NotifyDeviceLibrary();
  }

  rv = CallQueryInterface(mTempSyncSettings.get(), aTempSyncSettings);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* void resetSyncSettings (); */
NS_IMETHODIMP
sbDeviceLibrary::ResetSyncSettings()
{
  {
    nsAutoMonitor monitor(mMonitor);

    // If temp settings were never retrieved, nothing to do.
    if (!mTempSyncSettings) {
      return NS_OK;
    }

    nsAutoLock lockCurrentSyncSettings(mCurrentSyncSettings->GetLock());
    nsAutoLock lockTempSyncSettings(mTempSyncSettings->GetLock());

    nsresult rv;

    rv = mTempSyncSettings->Assign(mCurrentSyncSettings);
    NS_ENSURE_SUCCESS(rv, rv);

    mTempSyncSettings->NotifyDeviceLibrary();
    mTempSyncSettings->ResetChangedNoLock();
  }

  SB_NOTIFY_LISTENERS(OnSyncSettings(
                                  sbIDeviceLibraryListener::SYNC_SETTINGS_RESET,
                                  mTempSyncSettings));

  return NS_OK;
}

/* void applySyncSettings (); */
NS_IMETHODIMP
sbDeviceLibrary::ApplySyncSettings()
{
  // If temp settings were never retrieved, nothing to do.
  if (!mTempSyncSettings) {
    return NS_OK;
  }
  nsresult rv;

  rv = SetSyncSettingsNoLock(mTempSyncSettings);
  NS_ENSURE_SUCCESS(rv, rv);

  mTempSyncSettings->NotifyDeviceLibrary();
  mTempSyncSettings->ResetChanged();

  // update the library is read-only property
  rv = UpdateIsReadOnly();
  NS_ENSURE_SUCCESS(rv, rv);

  // update the main library listeners
  rv = UpdateMainLibraryListeners(mCurrentSyncSettings);
  NS_ENSURE_SUCCESS(rv, rv);

  SB_NOTIFY_LISTENERS(OnSyncSettings(
                                sbIDeviceLibraryListener::SYNC_SETTINGS_APPLIED,
                                mCurrentSyncSettings));

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::GetTempSyncSettingsChanged(PRBool * aChanged)
{
  NS_ENSURE_ARG_POINTER(aChanged);

  nsAutoMonitor monitor(mMonitor);

  if (!mTempSyncSettings) {
    *aChanged = PR_FALSE;
  }
  else {
    *aChanged = mTempSyncSettings->HasChanged();
  }
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

  nsCOMPtr<sbIDeviceLibraryMediaSyncSettings> mediaSyncSettings;
  rv = sbDeviceUtils::GetMediaSettings(this,
                                       sbIDeviceLibrary::MEDIATYPE_IMAGE,
                                       getter_AddRefs(mediaSyncSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mgmtType;
  rv = mediaSyncSettings->GetMgmtType(&mgmtType);
  NS_ENSURE_SUCCESS(rv, rv);

  // If all items, simply add the root folder to the list
  if (mgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_ALL) {
    nsCOMPtr<nsIFile> rootFolder;
    rv = mediaSyncSettings->GetSyncFromFolder(getter_AddRefs(rootFolder));
    NS_ENSURE_SUCCESS(rv, rv);

    if (rootFolder) {
      rv = array->AppendElement(rootFolder, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else if (mgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_PLAYLISTS) {
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

  // update the library is read-only property
  rv = UpdateIsReadOnly();
  NS_ENSURE_SUCCESS(rv, rv);

  // TODO: XXX For bug 23188 would we need to update listeners. We used to,
  // but I don't think it's necessary.
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrary::AddDeviceLibraryListener(sbIDeviceLibraryListener* aListener)
{
  TRACE(("sbDeviceLibrary[0x%x] - AddListener", this));
  NS_ENSURE_ARG_POINTER(aListener);

  {
    nsAutoMonitor monitor(mMonitor);

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

  nsAutoMonitor monitor(mMonitor);

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

  nsAutoMonitor monitor(mMonitor);

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

  SB_NOTIFY_LISTENERS_RETURN_FALSE(OnItemAdded(aMediaList,
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

  SB_NOTIFY_LISTENERS_RETURN_FALSE(OnBeforeItemRemoved(aMediaList,
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

  SB_NOTIFY_LISTENERS_RETURN_FALSE(OnAfterItemRemoved(aMediaList,
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

  SB_NOTIFY_LISTENERS_RETURN_FALSE(OnItemUpdated(aMediaList,
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

  SB_NOTIFY_LISTENERS_RETURN_FALSE(OnItemMoved(aMediaList,
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

  SB_NOTIFY_LISTENERS_RETURN_FALSE(OnBeforeListCleared(aMediaList,
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

  SB_NOTIFY_LISTENERS_RETURN_FALSE(OnListCleared(aMediaList,
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
    rv = mDeviceLibrary->CreateMediaItem(aContentUri,
                                         aProperties,
                                         aAllowDuplicates,
                                         _retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
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
    rv = mDeviceLibrary->CreateMediaItemIfNotExist(aContentUri,
                                                   aProperties,
                                                   aResultItem,
                                                   _retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
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

/**
 * See sbILibrary
 */
NS_IMETHODIMP
sbDeviceLibrary::ClearItemsByType(const nsAString &aContentType)
{
  NS_ASSERTION(mDeviceLibrary, "mDevice library is null, call init first.");
  return mDeviceLibrary->ClearItemsByType(aContentType);
}

/*
 * See sbIMediaList
 */
NS_IMETHODIMP
sbDeviceLibrary::Add(sbIMediaItem *aMediaItem)
{
  return AddItem(aMediaItem, nsnull);
}

/*
 * See sbIMediaList
 */
NS_IMETHODIMP
sbDeviceLibrary::AddItem(sbIMediaItem *aMediaItem,
                         sbIMediaItem ** aNewMediaItem)
{
  NS_ASSERTION(mDeviceLibrary, "mDeviceLibrary is null, call init first.");
  SB_NOTIFY_LISTENERS_ASK_PERMISSION(OnBeforeAdd(aMediaItem,
                                                 &mShouldProcceed));

  if (mPerformAction) {
    return mDeviceLibrary->AddItem(aMediaItem, aNewMediaItem);
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

  nsresult rv = NS_ERROR_UNEXPECTED;

  if (mPerformAction) {
    rv = mDeviceLibrary->AddSome(aMediaItems);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

/*
 * See sbIMediaList
 */
NS_IMETHODIMP
sbDeviceLibrary::AddSomeAsync(nsISimpleEnumerator *aMediaItems,
                              sbIMediaListAsyncListener *aListener)
{
  NS_ASSERTION(mDeviceLibrary, "mDeviceLibrary is null, call init first.");
  SB_NOTIFY_LISTENERS_ASK_PERMISSION(OnBeforeAddSome(aMediaItems,
                                                     &mShouldProcceed));

  nsresult rv = NS_ERROR_UNEXPECTED;

  if (mPerformAction) {
    rv = mDeviceLibrary->AddSomeAsync(aMediaItems, aListener);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
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

NS_IMETHODIMP
sbDeviceLibrary::FireTempSyncSettingsEvent(PRUint32 aEvent)
{
  SB_NOTIFY_LISTENERS(OnSyncSettings(aEvent, mCurrentSyncSettings));

  return NS_OK;
}
