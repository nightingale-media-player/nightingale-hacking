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

#include "sbDeviceLibrarySyncSettings.h"

// Mozilla includes
#include <nsArrayUtils.h>
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
#include <sbStandardProperties.h>
#include <sbStringUtils.h>
#include <sbVariantUtils.h>

extern PLDHashOperator ArrayBuilder(nsISupports * aKey,
                                    bool aData,
                                    void* userArg);

PRUint32 const LEGACY_MGMT_TYPE_MANUAL = 0x1;

/**
 * Used to convert old legacy management values to the newer values
 */
static void
MigrateLegacyMgmtValues(PRUint32 & aValue)
{
  // Legacy values from the old sync settings system
  PRUint32 const LEGACY_SYNC_NONE = 0x0;
  PRUint32 const LEGACY_SYNC_ALL = 0x2;
  PRUint32 const LEGACY_MANUAL_SYNC_ALL = 0x3;
  PRUint32 const LEGACY_SYNC_PLAYLISTS = 0x4;
  PRUint32 const LEGACY_MANUAL_SYNC_PLAYLISTS = 0x5;

  switch (aValue) {
    case sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_NONE:
    case sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_ALL:
    case sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_PLAYLISTS:
      // aValue is already an acceptable value, no need to migrate
      break;
    case LEGACY_SYNC_NONE:
      aValue = sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_NONE;
      break;
    case LEGACY_SYNC_ALL:
    case LEGACY_MANUAL_SYNC_ALL:
      aValue = sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_ALL;
      break;
    case LEGACY_SYNC_PLAYLISTS:
    case LEGACY_MANUAL_SYNC_PLAYLISTS:
      aValue = sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_PLAYLISTS;
      break;
    default:
      // unexpected aValue, default to SYNC_MGMT_NONE
      aValue = sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_NONE;
      break;
  }
}

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
      // TODO: removed buggy/deadlocky code. This is used by the code for
      // determining whether things will fit on the device, but that code needs
      // a total rewrite anyway.
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
  PRUint32 mgmtType;
  // If there is no value, set to manual
  if (dataType == nsIDataType::VTYPE_VOID ||
      dataType == nsIDataType::VTYPE_EMPTY)
  {
    mgmtType = sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_NONE;
  } else {
    // has a value
    rv = var->GetAsUint32(&mgmtType);
    NS_ENSURE_SUCCESS(rv, rv);

    MigrateLegacyMgmtValues(mgmtType);

    // Double check that it is a valid number
    NS_ENSURE_ARG(
            mgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_NONE ||
            mgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_ALL ||
            mgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_PLAYLISTS);

  }

  // Return the management type
  aMgmtTypes = mgmtType;

  return NS_OK;
}

nsresult
sbDeviceLibrarySyncSettings::GetImportPref(sbIDevice * aDevice,
                                           PRUint32 aContentType,
                                           bool & aImport)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);

  nsresult rv;

  nsString prefKey;
  rv = GetImportPrefKey(aContentType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> var;
  rv = aDevice->GetPreference(prefKey, getter_AddRefs(var));
  NS_ENSURE_SUCCESS(rv, rv);

  // check if a value exists
  PRUint16 dataType;
  rv = var->GetDataType(&dataType);
  // If there is no value default to false
  if (dataType == nsIDataType::VTYPE_VOID ||
      dataType == nsIDataType::VTYPE_EMPTY)
  {
    aImport = PR_FALSE;
  } else {
    // has a value
    rv = var->GetAsBool(&aImport);
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

  // Set the import flag
  bool import;
  rv = GetImportPref(aDevice, aMediaType, import);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = settings->SetImport(import);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mgmtType;
  rv = GetMgmtTypePref(aDevice, aMediaType, mgmtType);
  NS_ENSURE_SUCCESS(rv, rv);
  settings->SetMgmtType(mgmtType);

  nsCOMPtr<nsIArray> mediaLists;
  nsString prefKey;
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

      bool added = settings->mPlaylistsSelection.Put(supports, PR_FALSE);
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
        bool selected;
        bool exists = settings->mPlaylistsSelection.Get(supports, &selected);
        if (exists) {
          bool added = settings->mPlaylistsSelection.Put(supports, PR_TRUE);
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

  rv = GetImportPrefKey(aMediaType, prefKey);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WritePref(aDevice,
                 prefKey,
                 aMediaSyncSettings->mImport);
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

NS_IMETHODIMP
sbDeviceLibrarySyncSettings::Write(sbIDevice * aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;

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
sbDeviceLibrarySyncSettings::GetImportPrefKey(PRUint32 aContentType,
                                              nsAString& aPrefKey)
{
  NS_ENSURE_ARG_RANGE(aContentType,
                      sbIDeviceLibrary::MEDIATYPE_AUDIO,
                      sbIDeviceLibrary::MEDIATYPE_COUNT - 1);
  NS_ENSURE_STATE(!mDeviceLibraryGuid.IsEmpty());


  // Get the preference key
  aPrefKey.Assign(NS_LITERAL_STRING(PREF_SYNC_PREFIX));
  aPrefKey.Append(mDeviceLibraryGuid);
  aPrefKey.AppendLiteral(PREF_SYNC_BRANCH PREF_SYNC_IMPORT);

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
