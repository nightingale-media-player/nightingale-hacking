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

#include "sbDeviceLibrarySyncSettings.h"

// Mozilla includes
#include <nsArrayUtils.h>
#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsILocalFile.h>
#include <nsThreadUtils.h>

// Songbird local includes
#include "sbDeviceLibrary.h"
#include "sbDeviceLibraryMediaSyncSettings.h"

// Songibrd includes

#include <sbArrayUtils.h>
#include <sbDeviceUtils.h>
#include <sbIDevice.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <sbPropertiesCID.h>
#include <sbProxiedComponentManager.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>
#include <sbVariantUtils.h>

#define SB_AUDIO_SMART_PLAYLIST "<sbaudio>"

extern PLDHashOperator ArrayBuilder(nsISupports * aKey,
                                    PRBool aData,
                                    void* userArg);

NS_IMPL_ISUPPORTS1(sbDeviceLibrarySyncSettings, sbIDeviceLibrarySyncSettings);

sbDeviceLibrarySyncSettings * sbDeviceLibrarySyncSettings::New(
                                          nsID const & aDeviceID,
                                          nsAString const & aDeviceLibraryGuid)
{
  return new sbDeviceLibrarySyncSettings(aDeviceID,
                                         aDeviceLibraryGuid);
}

sbDeviceLibrarySyncSettings::sbDeviceLibrarySyncSettings(
                                         nsID const & aDeviceID,
                                         nsAString const & aDeviceLibraryGuid) :
  mDeviceID(aDeviceID),
  mDeviceLibraryGuid(aDeviceLibraryGuid),
  mSyncMode(sbIDeviceLibrarySyncSettings::SYNC_MODE_MANUAL),
  mChanged(false),
  mNotifyDeviceLibrary(false),
  mLock(nsAutoLock::NewLock("sbDeviceLibrarySyncSettings"))
{
  mMediaSettings.SetLength(sbIDeviceLibrary::MEDIATYPE_COUNT);
}

sbDeviceLibrarySyncSettings::~sbDeviceLibrarySyncSettings()
{
  nsAutoLock::DestroyLock(mLock);
}

nsresult sbDeviceLibrarySyncSettings::Assign(
                                        sbDeviceLibrarySyncSettings * aSource)
{
  NS_ENSURE_ARG_POINTER(aSource);

  nsresult rv;

  // Check for assignment to self
  // We can do a simple check since this will always be a
  // simple object
  if (this == aSource) {
    return NS_OK;
  }

  mDeviceID = aSource->mDeviceID;
  mDeviceLibraryGuid = aSource->mDeviceLibraryGuid;

  mSyncMode = aSource->mSyncMode;
  // Copy the media setting entries
  nsRefPtr<sbDeviceLibraryMediaSyncSettings> mediaSettings;
  nsRefPtr<sbDeviceLibraryMediaSyncSettings> newMediaSettings;
  for (PRUint32 mediaType = sbIDeviceLibrary::MEDIATYPE_AUDIO;
       mediaType < sbIDeviceLibrary::MEDIATYPE_COUNT;
       ++mediaType) {
    mediaSettings = aSource->mMediaSettings[mediaType];
    if (mediaSettings) {
      rv = mediaSettings->CreateCopy(getter_AddRefs(newMediaSettings));
      NS_ENSURE_SUCCESS(rv, rv);

      mMediaSettings[mediaType] = newMediaSettings;
    }
  }
  return NS_OK;
}

void sbDeviceLibrarySyncSettings::Changed()
{
  // If we're already have changed nothing to do.
  if (mChanged) {
    return;
  }
  mChanged = true;
  if (mNotifyDeviceLibrary) {
    nsCOMPtr<sbIDeviceLibrary> deviceLibrary;
    nsresult rv = sbDeviceUtils::GetDeviceLibrary(mDeviceLibraryGuid,
                                                  &mDeviceID,
                                                  getter_AddRefs(deviceLibrary));
    NS_ENSURE_SUCCESS(rv, /* void */);

    rv = deviceLibrary->FireTempSyncSettingsEvent(
                               sbIDeviceLibraryListener::SYNC_SETTINGS_CHANGED);
    NS_ENSURE_SUCCESS(rv, /* void */);
  }
}

nsresult sbDeviceLibrarySyncSettings::CreateCopy(
                                        sbDeviceLibrarySyncSettings ** aSettings)
{
  NS_ENSURE_ARG_POINTER(aSettings);
  nsresult rv;

  nsRefPtr<sbDeviceLibrarySyncSettings> settings =
    sbDeviceLibrarySyncSettings::New(mDeviceID, mDeviceLibraryGuid);
  NS_ENSURE_TRUE(settings, NS_ERROR_OUT_OF_MEMORY);

  rv = settings->Assign(this);
  NS_ENSURE_SUCCESS(rv, rv);

  settings.forget(aSettings);

  return NS_OK;
}

bool sbDeviceLibrarySyncSettings::HasChanged() const
{
  nsAutoLock lock(mLock);
  return HasChangedNoLock();
}

void sbDeviceLibrarySyncSettings::ResetChangedNoLock()
{
  mChanged = false;
  nsRefPtr<sbDeviceLibraryMediaSyncSettings> mediaSettings;
  for (PRUint32 mediaType = sbIDeviceLibrary::MEDIATYPE_AUDIO;
       mediaType < sbIDeviceLibrary::MEDIATYPE_COUNT;
       ++mediaType) {
    mediaSettings = mMediaSettings[mediaType];
    if (mediaSettings) {
      mediaSettings->ResetChanged();
    }
  }
}

void sbDeviceLibrarySyncSettings::ResetChanged()
{
  nsAutoLock lock(mLock);
  ResetChangedNoLock();
}

/* attribute unsigned long syncMode; */
NS_IMETHODIMP
sbDeviceLibrarySyncSettings::GetSyncMode(PRUint32 *aSyncMode)
{
  NS_ASSERTION(mLock, "sbDeviceLibrarySyncSettings not initialized");
  NS_ENSURE_ARG_POINTER(aSyncMode);
  nsAutoLock lock(mLock);

  *aSyncMode = mSyncMode;

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrarySyncSettings::SetSyncMode(PRUint32 aSyncMode)
{
  NS_ASSERTION(mLock, "sbDeviceLibrarySyncSettings not initialized");

  mSyncMode = aSyncMode;

  Changed();

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceLibrarySyncSettings::GetMediaSettings(
                            PRUint32 aMediaType,
                            sbIDeviceLibraryMediaSyncSettings ** aMediaSettings)
{
  NS_ASSERTION(mLock, "sbDeviceLibrarySyncSettings not initialized");

  nsAutoLock lock(mLock);
  return GetMediaSettingsNoLock(aMediaType, aMediaSettings);
}

nsresult
sbDeviceLibrarySyncSettings::GetMediaSettingsNoLock(
                            PRUint32 aMediaType,
                            sbIDeviceLibraryMediaSyncSettings ** aMediaSettings)
{
  NS_ASSERTION(aMediaSettings,
               "aMediaSettings is null in "
               "DeviceLibrarySyncSettings::GetMediaSettingsNoLock");
  nsRefPtr<sbDeviceLibraryMediaSyncSettings> newSettings =
    mMediaSettings[aMediaType];
  // If we don't have one, create a default one
  if (!newSettings) {
    newSettings = sbDeviceLibraryMediaSyncSettings::New(this,
                                                        aMediaType,
                                                        mLock);
    NS_ENSURE_TRUE(newSettings, NS_ERROR_OUT_OF_MEMORY);

    mMediaSettings[aMediaType] = newSettings;
  }
  else {
    newSettings->SetSyncSettings(this);
  }

  *aMediaSettings =
    static_cast<sbIDeviceLibraryMediaSyncSettings*>(newSettings.get());
  newSettings.forget();

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
  rv = sbLibraryUtils::GetEqualOperator(getter_AddRefs(equal));
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

NS_IMETHODIMP
sbDeviceLibrarySyncSettings::GetSyncPlaylists(nsIArray ** aMediaLists)
{
  NS_ASSERTION(mLock, "sbDeviceLibrarySyncSettings not initialized");
  nsresult rv;

  nsCOMPtr<nsIMutableArray> allPlaylists =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoLock lock(mLock);

  nsCOMPtr<sbDeviceLibraryMediaSyncSettings> mediaSettings;
  for (PRUint32 mediaType = sbIDeviceLibrary::MEDIATYPE_AUDIO;
       mediaType < sbIDeviceLibrary::MEDIATYPE_COUNT;
       ++mediaType) {

    mediaSettings = mMediaSettings[mediaType];
    if (!mediaSettings) {
      continue;
    }

    PRUint32 mgmtType;
    rv = mediaSettings->GetMgmtTypeNoLock(&mgmtType);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIArray> playlists;
    if (mgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_ALL) {
      if (mediaType == sbIDeviceLibrary::MEDIATYPE_AUDIO) {
        // If the "sync all" option is turned on for audio, add every audio
        // medialist and every normal mixed playlist.
        nsCOMPtr<sbIMediaList> mediaList;
        rv = GetOrCreateAudioSmartMediaList(getter_AddRefs(mediaList));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = allPlaylists->AppendElement(mediaList, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      rv = mediaSettings->GetSyncPlaylistsNoLock(getter_AddRefs(playlists));
      if (rv == NS_ERROR_NOT_AVAILABLE) {
        continue;
      }
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else if (mgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_PLAYLISTS) {
      rv = mediaSettings->GetSelectedPlaylistsNoLock(getter_AddRefs(playlists));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (playlists) {
      rv = sbAppendnsIArray(playlists, allPlaylists);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = CallQueryInterface(allPlaylists, aMediaLists);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceLibrarySyncSettings::GetMgmtTypePref(sbIDevice * aDevice,
                                             PRUint32 aContentType,
                                             PRUint32 & aMgmtTypes)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);

  nsresult rv;

  nsString prefKey;
  rv = GetMgmtTypePrefKey(aContentType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> var;
  rv = aDevice->GetPreference(prefKey, getter_AddRefs(var));
  NS_ENSURE_SUCCESS(rv, rv);

  // check if a value exists
  PRUint16 dataType;
  rv = var->GetDataType(&dataType);
  // If there is no value, set to manual
  if (dataType == nsIDataType::VTYPE_VOID ||
      dataType == nsIDataType::VTYPE_EMPTY)
  {
    aMgmtTypes = sbIDeviceLibrarySyncSettings::SYNC_MODE_MANUAL;
  } else {
    // has a value
    PRInt32 mgmtType;
    rv = var->GetAsInt32(&mgmtType);
    NS_ENSURE_SUCCESS(rv, rv);

    // Double check that it is a valid number
    NS_ENSURE_ARG_RANGE(
                   mgmtType,
                   sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_NONE,
                   sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_TYPE_MAX_VALUE);

    // Max the manual mode
    aMgmtTypes = mgmtType;
  }

  return NS_OK;
}

nsresult
sbDeviceLibrarySyncSettings::ReadLegacySyncMode(sbIDevice * aDevice,
                                                PRUint32 & aSyncMode)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;
  PRUint32 syncMode = sbIDeviceLibrarySyncSettings::SYNC_MODE_AUTO;
  for (PRUint32 i = 0; i < sbIDeviceLibrary::MEDIATYPE_COUNT; ++i) {
    // Ignore management type for images, it is always semi-manual
    if (i == sbIDeviceLibrary::MEDIATYPE_IMAGE)
      continue;

    PRUint32 mgmtType;
    rv = GetMgmtTypePref(aDevice, i, mgmtType);
    NS_ENSURE_SUCCESS(rv, rv);

    // Manual mode.
    if (mgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_NONE) {
      syncMode = sbIDeviceLibrarySyncSettings::SYNC_MODE_MANUAL;
      break;
    }
  }
  aSyncMode = syncMode;

  return NS_OK;
}

nsresult
sbDeviceLibrarySyncSettings::ReadPRUint32(sbIDevice * aDevice,
                                          nsAString const & aPrefKey,
                                          PRUint32 & aInt,
                                          PRUint32 aDefault)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;

  nsCOMPtr<nsIVariant> var;
  rv = aDevice->GetPreference(aPrefKey, getter_AddRefs(var));
  NS_ENSURE_SUCCESS(rv, rv);

  // check if a value exists
  PRUint16 dataType;
  rv = var->GetDataType(&dataType);
  // If there is no value, set to aDefault
  if (dataType == nsIDataType::VTYPE_VOID ||
      dataType == nsIDataType::VTYPE_EMPTY)
  {
    aInt = aDefault;
  } else {
    // has a value
    rv = var->GetAsUint32(&aInt);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

template <class T>
nsresult
sbDeviceLibrarySyncSettings::WritePref(sbIDevice * aDevice,
                                       nsAString const & aPrefKey,
                                       T aValue)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;

  rv = aDevice->SetPreference(aPrefKey, sbNewVariant(aValue));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceLibrarySyncSettings::ReadAString(sbIDevice * aDevice,
                                         nsAString const & aPrefKey,
                                         nsAString & aString,
                                         nsAString const & aDefault)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;

  nsCOMPtr<nsIVariant> var;
  rv = aDevice->GetPreference(aPrefKey, getter_AddRefs(var));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint16 dataType;
  rv = var->GetDataType(&dataType);
  if (dataType == nsIDataType::VTYPE_VOID) {
    aString = aDefault;
  }
  rv = var->GetAsAString(aString);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceLibrarySyncSettings::ReadSyncMode(sbIDevice * aDevice,
                                          PRUint32 & aSyncMode)
{
  nsresult rv;

  nsString prefKey;
  rv = GetSyncModePrefKey(prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 const NOT_FOUND = 0xFFFFFFFF;
  PRUint32 mode;
  rv = ReadPRUint32(aDevice, prefKey, mode, NOT_FOUND);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mode == NOT_FOUND)
  {
    rv = ReadLegacySyncMode(aDevice, aSyncMode);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    // Double check that it is a valid number
    NS_ENSURE_ARG_RANGE(mode,
                        sbIDeviceLibrarySyncSettings::SYNC_MODE_MANUAL,
                        sbIDeviceLibrarySyncSettings::SYNC_MODE_AUTO);

    aSyncMode = mode;
  }

  return NS_OK;

}

nsresult
sbDeviceLibrarySyncSettings::ReadMediaSyncSettings(
                         sbIDevice * aDevice,
                         sbIDeviceLibrary * aDeviceLibrary,
                         PRUint32 aMediaType,
                         sbDeviceLibraryMediaSyncSettings ** aMediaSyncSettings)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aMediaSyncSettings);

  nsresult rv;

  nsRefPtr<sbDeviceLibraryMediaSyncSettings> settings =
    sbDeviceLibraryMediaSyncSettings::New(this,
                                          aMediaType,
                                          mLock);
  NS_ENSURE_TRUE(settings, NS_ERROR_OUT_OF_MEMORY);

  nsString prefKey;
  rv = GetMgmtTypePrefKey(aMediaType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ReadPRUint32(aDevice,
                    prefKey,
                    settings->mSyncMgmtType,
                    sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_NONE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> mediaLists;
  rv = settings->GetSyncPlaylistsNoLock(getter_AddRefs(mediaLists));
  // Some media types won't have playlists
  if (rv != NS_ERROR_NOT_AVAILABLE) {
    NS_ENSURE_SUCCESS(rv, rv);

    settings->mPlaylistsSelection.Clear();

    PRUint32 count;
    rv = mediaLists->GetLength(&count);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 index = 0; index < count; ++index) {
      nsCOMPtr<sbIMediaList> mediaList = do_QueryElementAt(mediaLists,
                                                           index,
                                                           &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsISupports> supports = do_QueryInterface(mediaList, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool added = settings->mPlaylistsSelection.Put(supports, PR_FALSE);
      NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
    }

    rv = GetSyncListsPrefKey(aMediaType, prefKey);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString mediaListGuids;
    rv = ReadAString(aDevice, prefKey, mediaListGuids, nsString());

    nsCOMPtr<sbILibraryManager> libManager =
        do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbILibrary> mainLibrary;
    rv = libManager->GetMainLibrary(getter_AddRefs(mainLibrary));
    NS_ENSURE_SUCCESS(rv, rv);

    nsTArray<nsString> mediaListGuidArray;
    nsString_Split(mediaListGuids, NS_LITERAL_STRING(","), mediaListGuidArray);
    count = mediaListGuidArray.Length();
    for (PRUint32 index = 0; index < count; ++index)  {
      nsCOMPtr<sbIMediaItem> mediaItem;
      rv = mainLibrary->GetItemByGuid(mediaListGuidArray[index],
                                      getter_AddRefs(mediaItem));
      if (rv != NS_ERROR_NOT_AVAILABLE) {
        NS_ENSURE_SUCCESS(rv, rv);
        nsCOMPtr<nsISupports> supports = do_QueryInterface(mediaItem, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        // We don't want to add to this list just set them to true if they match
        PRBool selected;
        PRBool exists = settings->mPlaylistsSelection.Get(supports, &selected);
        if (exists) {
          PRBool added = settings->mPlaylistsSelection.Put(supports, PR_TRUE);
          NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
        }
      }
    }
  }

  rv = GetSyncFolderPrefKey(aMediaType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = ReadAString(aDevice, prefKey, settings->mSyncFolder, nsString());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetSyncFromFolderPrefKey(aMediaType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString syncFromFolderString;
  rv = ReadAString(aDevice, prefKey, syncFromFolderString, nsString());
  NS_ENSURE_SUCCESS(rv, rv);

  if (!syncFromFolderString.IsEmpty()) {
    nsCOMPtr<nsILocalFile> folder;
    rv = NS_NewLocalFile(syncFromFolderString,
                         PR_TRUE,
                         getter_AddRefs(folder));
    NS_ENSURE_SUCCESS(rv, rv);

    settings->mSyncFromFolder = folder;
  }
  settings.forget(aMediaSyncSettings);

  return NS_OK;
}

nsresult
sbDeviceLibrarySyncSettings::WriteMediaSyncSettings(
                          sbIDevice * aDevice,
                          PRUint32 aMediaType,
                          sbDeviceLibraryMediaSyncSettings * aMediaSyncSettings)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aMediaSyncSettings);

  nsresult rv;

  nsString prefKey;
  rv = GetMgmtTypePrefKey(aMediaType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = WritePref(aDevice,
                 prefKey,
                 aMediaSyncSettings->mSyncMgmtType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetSyncFromFolderPrefKey(aMediaType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString syncFromFolderString;
  if (aMediaSyncSettings->mSyncFromFolder) {
    rv = aMediaSyncSettings->mSyncFromFolder->GetPath(syncFromFolderString);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = WritePref(aDevice, prefKey, syncFromFolderString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetSyncFolderPrefKey(aMediaType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = WritePref(aDevice, prefKey, aMediaSyncSettings->mSyncFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  // Ignore sync playlists writing for image.
  if (aMediaType == sbIDeviceLibrary::MEDIATYPE_IMAGE)
    return NS_OK;

  rv = GetSyncListsPrefKey(aMediaType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMutableArray> selected =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  aMediaSyncSettings->mPlaylistsSelection.EnumerateRead(ArrayBuilder,
                                                        selected.get());

  PRUint32 count;
  rv = selected->GetLength(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString mediaListGuids;
  nsCOMPtr<sbIMediaList> mediaList;
  for (PRUint32 index = 0; index < count; ++index) {
    if (count) {
      mediaListGuids.Append(NS_LITERAL_STRING(","));
    }
    mediaList = do_QueryElementAt(selected, index, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString guid;
    rv = mediaList->GetGuid(guid);
    NS_ENSURE_SUCCESS(rv, rv);

    mediaListGuids.Append(guid);
  }
  rv = WritePref(aDevice, prefKey, mediaListGuids);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceLibrarySyncSettings::Read(sbIDevice * aDevice,
                                  sbIDeviceLibrary * aDeviceLibrary)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;

  rv = ReadSyncMode(aDevice, mSyncMode);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbDeviceLibraryMediaSyncSettings> mediaSettings;
  for (PRUint32 mediaType = sbIDeviceLibrary::MEDIATYPE_AUDIO;
       mediaType < sbIDeviceLibrary::MEDIATYPE_COUNT;
       ++mediaType) {
    if (mMediaSettings[mediaType]) {
      rv = mediaSettings->Assign(mMediaSettings[mediaType]);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = ReadMediaSyncSettings(aDevice,
                                 aDeviceLibrary,
                                 mediaType,
                                 getter_AddRefs(mediaSettings));
      NS_ENSURE_SUCCESS(rv, rv);

      mMediaSettings[mediaType] = mediaSettings;
    }
  }
  return NS_OK;
}

nsresult
sbDeviceLibrarySyncSettings::Write(sbIDevice * aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;

  nsString prefKey;
  rv = GetSyncModePrefKey(prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = WritePref(aDevice, prefKey, mSyncMode);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbDeviceLibraryMediaSyncSettings> mediaSettings;
  for (PRUint32 mediaType = sbIDeviceLibrary::MEDIATYPE_AUDIO;
       mediaType < sbIDeviceLibrary::MEDIATYPE_COUNT;
       ++mediaType) {
    mediaSettings = mMediaSettings[mediaType];
    if (mediaSettings) {
      rv = WriteMediaSyncSettings(aDevice,
                                  mediaType,
                                  mediaSettings);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}

nsresult
sbDeviceLibrarySyncSettings::GetSyncModePrefKey(nsAString& aPrefKey)
{
  NS_ENSURE_STATE(!mDeviceLibraryGuid.IsEmpty());


  // Get the preference key
  aPrefKey.Assign(NS_LITERAL_STRING(PREF_SYNC_PREFIX));
  aPrefKey.Append(mDeviceLibraryGuid);
  aPrefKey.AppendLiteral(PREF_SYNC_BRANCH PREF_SYNC_MODE);

  return NS_OK;
}

nsresult
sbDeviceLibrarySyncSettings::GetMgmtTypePrefKey(PRUint32 aContentType,
                                                nsAString& aPrefKey)
{
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);
  NS_ENSURE_STATE(!mDeviceLibraryGuid.IsEmpty());


  // Get the preference key
  aPrefKey.Assign(NS_LITERAL_STRING(PREF_SYNC_PREFIX));
  aPrefKey.Append(mDeviceLibraryGuid);
  aPrefKey.AppendLiteral(PREF_SYNC_BRANCH PREF_SYNC_MGMTTYPE);

  aPrefKey.AppendLiteral(gMediaType[aContentType]);

  return NS_OK;
}

nsresult
sbDeviceLibrarySyncSettings::GetSyncListsPrefKey(PRUint32 aContentType,
                                     nsAString& aPrefKey)
{
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);
  NS_ENSURE_STATE(!mDeviceLibraryGuid.IsEmpty());

  // Get the preference key
  aPrefKey.Assign(NS_LITERAL_STRING(PREF_SYNC_PREFIX));
  aPrefKey.Append(mDeviceLibraryGuid);
  aPrefKey.AppendLiteral(PREF_SYNC_BRANCH);
  aPrefKey.AppendLiteral(PREF_SYNC_LISTS);
  aPrefKey.AppendLiteral(gMediaType[aContentType]);

  return NS_OK;
}

nsresult
sbDeviceLibrarySyncSettings::GetSyncFromFolderPrefKey(PRUint32 aContentType,
                                                      nsAString& aPrefKey)
{
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);
  NS_ENSURE_STATE(!mDeviceLibraryGuid.IsEmpty());

  // Get the preference key
  aPrefKey.Assign(NS_LITERAL_STRING(PREF_SYNC_PREFIX));
  aPrefKey.Append(mDeviceLibraryGuid);
  aPrefKey.AppendLiteral(PREF_SYNC_BRANCH);
  aPrefKey.AppendLiteral(PREF_SYNC_ROOT);
  aPrefKey.AppendLiteral(gMediaType[aContentType]);

  return NS_OK;
}

nsresult
sbDeviceLibrarySyncSettings::GetSyncFolderPrefKey(PRUint32 aContentType,
                                                  nsAString& aPrefKey)
{
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);
  NS_ENSURE_STATE(!mDeviceLibraryGuid.IsEmpty());

  // Get the preference key
  aPrefKey.Assign(NS_LITERAL_STRING(PREF_SYNC_PREFIX));
  aPrefKey.Append(mDeviceLibraryGuid);
  aPrefKey.AppendLiteral(PREF_SYNC_BRANCH);
  aPrefKey.AppendLiteral(PREF_SYNC_FOLDER);
  aPrefKey.AppendLiteral(gMediaType[aContentType]);

  return NS_OK;
}
