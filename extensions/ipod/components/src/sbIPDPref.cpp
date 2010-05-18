/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://www.songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
//
// Software distributed under the License is distributed
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
//=END SONGBIRD GPL
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// iPod device preference services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbIPDPref.cpp
 * \brief Songbird iPod Device Preference Source.
 */

//------------------------------------------------------------------------------
//
// iPod device preference imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIPDDevice.h"
#include "sbIPDLog.h"

// Songbird imports.
#include <sbAutoRWLock.h>
#include <sbIDeviceEvent.h>
#include <sbIDeviceLibraryMediaSyncSettings.h>

// Mozilla imports.
#include <nsArrayUtils.h>
#include <prprf.h>


//------------------------------------------------------------------------------
//
// iPod device preference services.
//
//------------------------------------------------------------------------------

/**
 * Initialize the preference services.
 */

nsresult
sbIPDDevice::PrefInitialize()
{
  // Set up to auto-finalize on error.
  SB_IPD_DEVICE_AUTO_INVOKE(AutoFinalize, PrefFinalize()) autoFinalize(this);

  // Create the preference lock.
  mPrefLock = nsAutoLock::NewLock("sbIPDDevice::mPrefLock");
  NS_ENSURE_TRUE(mPrefLock, NS_ERROR_OUT_OF_MEMORY);

  // Forget auto-finalize.
  autoFinalize.forget();

  return NS_OK;
}


/**
 * Finalize the preference services.
 */

void
sbIPDDevice::PrefFinalize()
{
  // Dispose of pref lock.
  if (mPrefLock)
    nsAutoLock::DestroyLock(mPrefLock);
  mPrefLock = nsnull;
}


/**
 * Connect the preference services.
 */

nsresult
sbIPDDevice::PrefConnect()
{
  nsresult rv;

  // Set up to auto-disconnect on error.
  SB_IPD_DEVICE_AUTO_INVOKE(AutoDisconnect, PrefDisconnect())
    autoDisconnect(this);

  // Connect the iPod preferences file services.
  rv = PrefConnectIPodPrefs();
  NS_ENSURE_SUCCESS(rv, rv);

  // Mark the preference services as connected.  After this, "pref lock" fields
  // may be used.
  {
    nsAutoLock autoPrefLock(mPrefLock);
    mPrefConnected = PR_TRUE;
  }

  // Forget auto-disconnect.
  autoDisconnect.forget();

  return NS_OK;
}


/**
 * Disconnect the preference services.
 */

void
sbIPDDevice::PrefDisconnect()
{
  // Mark the preference services as not connected.  After this, "pref lock"
  // fields may not be used.
  {
    nsAutoLock autoPrefLock(mPrefLock);
    mPrefConnected = PR_FALSE;
  }

  // Disconnect the iPod preferences file services.
  PrefDisconnectIPodPrefs();
}


//------------------------------------------------------------------------------
//
// iPod device preference sbIDeviceLibrary services.
//
//------------------------------------------------------------------------------

/**
 * Return in aMgmtType the currently configured device management type.
 *
 * \param aMgmtType             Device management type.
 */

nsresult
sbIPDDevice::GetMgmtType(PRUint32* aMgmtType)
{
  // Validate arguments.
  NS_ASSERTION(aMgmtType, "aMgmtType is null");

  // Operate under the preference lock.
  nsAutoLock autoPrefLock(mPrefLock);
  NS_ENSURE_TRUE(mPrefConnected, NS_ERROR_NOT_AVAILABLE);

  // Read the management type.
  if (mIPodPrefs->music_mgmt_type == 0x00) {
    *aMgmtType = sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_NONE;
  } else {
    if (mIPodPrefs->music_update_type == 0x01)
      *aMgmtType = sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_ALL;
    else if (mIPodPrefs->music_update_type == 0x02)
      *aMgmtType = sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_PLAYLISTS;
    else {
      NS_WARNING("Unexpected management type preference.");
      return NS_ERROR_UNEXPECTED;
    }
  }

  return NS_OK;
}


/**
 * Set the device management type to the value specified by aMgmtType.
 *
 * \param aMgmtType             Device management type.
 */

nsresult
sbIPDDevice::SetMgmtType(PRUint32 aMgmtType)
{
  nsresult rv;

  // Operate under the preference lock.
  {
    nsAutoLock autoPrefLock(mPrefLock);
    NS_ENSURE_TRUE(mPrefConnected, NS_ERROR_NOT_AVAILABLE);

    // Set the management type.
    switch (aMgmtType) {
      case sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_NONE :
        mIPodPrefs->music_mgmt_type = 0x00;
        break;

      case sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_ALL :
        mIPodPrefs->music_mgmt_type = 0x01;
        mIPodPrefs->music_update_type = 0x01;
        break;

      case sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_PLAYLISTS :
        mIPodPrefs->music_mgmt_type = 0x01;
        mIPodPrefs->music_update_type = 0x02;
        break;

      default :
        NS_WARNING("Invalid management type.");
        return NS_ERROR_INVALID_ARG;
    }

    // Mark the preferences as dirty.
    mIPodPrefsDirty = PR_TRUE;
  }

  // Write the preferences.  Do this outside of the preference lock to avoid
  // potential deadlock warnings.
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_PREFS_CHANGED, nsnull);
  rv = PushRequest(REQUEST_WRITE_PREFS);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Return in aIsSetUp the state of the "iPod is set up" flag
 *
 * \param aIsSetUp             is the iPod set up
 */

nsresult
sbIPDDevice::GetIsSetUp(PRBool* aIsSetUp)
{
  // Validate arguments.
  NS_ASSERTION(aIsSetUp, "aIsSetUp is null");

  // Operate under the preference lock.
  nsAutoLock autoPrefLock(mPrefLock);
  NS_ENSURE_TRUE(mPrefConnected, NS_ERROR_NOT_AVAILABLE);

  // Read the ipod set up flag
  *aIsSetUp = (mIPodPrefs->ipod_set_up ? PR_TRUE : PR_FALSE);

  return NS_OK;
}


/**
 * Set the "iPod is set up" flag to the value specified by aIsSetUp.
 *
 * \param aIsSetUp             is the iPod set up?
 */

nsresult
sbIPDDevice::SetIsSetUp(PRBool aIsSetUp)
{
  nsresult rv;

  // Operate under the preference lock.
  {
    nsAutoLock autoPrefLock(mPrefLock);
    NS_ENSURE_TRUE(mPrefConnected, NS_ERROR_NOT_AVAILABLE);

    // Set the ipod set up flag
    mIPodPrefs->ipod_set_up = (aIsSetUp ? 0x01 : 0x00);

    // Mark the preferences as dirty.
    mIPodPrefsDirty = PR_TRUE;
  }

  // Write the preferences.  Do this outside of the preference lock to avoid
  // potential deadlock warnings.
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_PREFS_CHANGED, nsnull);
  rv = PushRequest(REQUEST_WRITE_PREFS);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Return in aPlaylistList the list of playlists from which to synchronize.
 *
 * \param aPlaylistList         List of playlists from which to synchronize.
 */

nsresult
sbIPDDevice::GetSyncPlaylistList(nsIArray** aPlaylistList)
{
  // Validate arguments.
  NS_ASSERTION(aPlaylistList, "aPlaylistList is null");

  // Function variables.
  nsresult rv;

  // Operate under the preference lock.
  nsAutoLock autoPrefLock(mPrefLock);
  NS_ENSURE_TRUE(mPrefConnected, NS_ERROR_NOT_AVAILABLE);

  // Get a mutable array.
  nsCOMPtr<nsIMutableArray> playlistList =
                              do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Build a Songbird sync playlist list from the iPod sync playlist list.
  PRUint32 playlistCount = mSyncPlaylistList.Length();
  for (PRUint32 i = 0; i < playlistCount; i++) {
    // Get the next media list from the next playlist ID.  Skip sync playlist if
    // it's not mapped to a Songbird media list.
    nsCOMPtr<sbIMediaItem> mediaItem;
    rv = IDMapGet(mSyncPlaylistList[i], getter_AddRefs(mediaItem));
    if (NS_FAILED(rv))
      continue;

    // Add it to the Songbird playlist list.
    rv = playlistList->AppendElement(mediaItem, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Return results.
  NS_ADDREF(*aPlaylistList = playlistList);

  return NS_OK;
}


/**
 * Set the list of playlists from which to synchronize to the list specified by
 * aPlaylistList.
 *
 * \param aPlaylistList         List of playlists from which to synchronize.
 */

nsresult
sbIPDDevice::SetSyncPlaylistList(nsIArray* aPlaylistList)
{
  // Validate arguments.
  NS_ASSERTION(aPlaylistList, "aPlaylistList is null");

  // Function variables.
  PRBool   success;
  nsresult rv;

  // Operate under the preference lock.
  {
    nsAutoLock autoPrefLock(mPrefLock);
    NS_ENSURE_TRUE(mPrefConnected, NS_ERROR_NOT_AVAILABLE);

    // Get the number of sync playlists.
    PRUint32 playlistCount;
    rv = aPlaylistList->GetLength(&playlistCount);
    NS_ENSURE_SUCCESS(rv, rv);

    // Reset iPod sync playlist list.
    mSyncPlaylistList.Clear();
    success = mSyncPlaylistList.SetCapacity(playlistCount);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    // Build an iPod sync playlist list from the Songbird sync playlist list.
    for (PRUint32 i = 0; i < playlistCount; i++) {
      // Get the next Songbird sync playlist media list item.
      nsCOMPtr<sbIMediaItem> mediaItem = do_QueryElementAt(aPlaylistList,
                                                           i,
                                                           &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      // Add the sync playlist media item to the iPod sync playlist list.
      guint64 playlistID;
      guint64* pPlaylistID;
      rv = GetSyncPlaylist(mediaItem, &playlistID);
      NS_ENSURE_SUCCESS(rv, rv);
      pPlaylistID = mSyncPlaylistList.AppendElement(playlistID);
      NS_ENSURE_TRUE(pPlaylistID, NS_ERROR_OUT_OF_MEMORY);
    }

    // Mark the preferences as dirty.
    mSyncPlaylistListDirty = PR_TRUE;
  }

  // Write the sync playlist list file.  Do this outside of the preference lock
  // to avoid potential deadlock warnings.
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_PREFS_CHANGED, nsnull);
  rv = PushRequest(REQUEST_WRITE_PREFS);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Add the playlist specified by aPlaylist to the list of playlists from which
 * to synchronize.
 *
 * \param aPlaylist             Playlist from which to synchronize.
 */

nsresult
sbIPDDevice::AddToSyncPlaylistList(sbIMediaList* aPlaylist)
{
  // Validate parameters.
  NS_ASSERTION(aPlaylist, "aPlaylist is null");

  // Function variables.
  nsresult rv;

  // Operate under the preference lock.
  {
    nsAutoLock autoPrefLock(mPrefLock);
    NS_ENSURE_TRUE(mPrefConnected, NS_ERROR_NOT_AVAILABLE);

    // Get the iPod playlist ID mapped to the sync playlist media list.
    guint64 playlistID;
    rv = GetSyncPlaylist(aPlaylist, &playlistID);
    NS_ENSURE_SUCCESS(rv, rv);

    // Do nothing more if playlist is already in list.
    if (mSyncPlaylistList.Contains(playlistID))
      return NS_OK;

    // Add to sync playlist list.
    guint64* appendSyncPlaylist;
    appendSyncPlaylist = mSyncPlaylistList.AppendElement(playlistID);
    NS_ENSURE_TRUE(appendSyncPlaylist, NS_ERROR_OUT_OF_MEMORY);

    // Mark the preferences as dirty.
    mSyncPlaylistListDirty = PR_TRUE;
  }

  // Write the sync playlist list file.  Do this outside of the preference lock
  // to avoid potential deadlock warnings.
  CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_PREFS_CHANGED, nsnull);
  rv = PushRequest(REQUEST_WRITE_PREFS);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Return in aIPodID the iPod ID mapped to the sync playlist specified by
 * aPlaylist.  If no mapping exists, create one.
 *
 * \param aPlaylist             Playlist from which to synchronize.
 * \param aIPodID               iPod ID mapped to sync playlist.
 */

nsresult
sbIPDDevice::GetSyncPlaylist(sbIMediaItem* aPlaylist,
                             guint64*      aIPodID)
{
  // Validate arguments.
  NS_ASSERTION(aPlaylist, "aPlaylist is null");

  // Function variables.
  nsresult rv;

  // If the playlist item is mapped to an iPod playlist ID, return that playlist
  // ID.
  nsAutoString      sbID;
  nsTArray<guint64> iPodIDList;
  rv = GetSBID(aPlaylist, sbID);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = IDMapGet(sbID, iPodIDList);
  if (NS_SUCCEEDED(rv) && !iPodIDList.IsEmpty()) {
    *aIPodID = iPodIDList[0];
    return NS_OK;
  }

  // If no iPod mapping exists for the media list item, create one and add
  // the mapped playlist ID to the iPod sync playlist list.
  guint64 iPodID = (((guint64) g_random_int()) << 32) |
                   ((guint64) g_random_int());
  rv = IDMapAdd(sbID, iPodID);
  NS_ENSURE_SUCCESS(rv, rv);
  *aIPodID = iPodID;

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// iPod device iPod preferences file services.
//
//   These services provide access to the iPod preferences files stored on the
// iPod.  These are the files that iTunes writes when saving iPod preference
// settings.
//
//------------------------------------------------------------------------------

/**
 * \brief Connect the iPod device iPod preferences file services.
 *
 * Since this function is called as a part of the preference connection process,
 * it does not need to operate under the preference lock.
 */

nsresult
sbIPDDevice::PrefConnectIPodPrefs()
{
  GError*  gError = NULL;
  PRBool   success;
  nsresult rv;

  // Set up to auto-disconnect on error.
  SB_IPD_DEVICE_AUTO_INVOKE(AutoDisconnect, PrefDisconnectIPodPrefs())
    autoDisconnect(this);

  // Parse the iPod preferences.
  mIPodPrefs = itdb_prefs_parse(mITDB->device, &gError);
  if (gError) {
    if (gError->message)
      FIELD_LOG((gError->message));
    g_error_free(gError);
    gError = NULL;
  }
  NS_ENSURE_TRUE(mIPodPrefs, NS_ERROR_FAILURE);

  // Read the sync playlist list and arrange for auto-cleanup.
  guint64* syncPlaylistList;
  int      syncPlaylistCount;
  success = itdb_update_playlists_read(mITDB->device,
                                       &syncPlaylistList,
                                       &syncPlaylistCount,
                                       &gError);
  if (gError) {
    if (gError->message)
      FIELD_LOG((gError->message));
    g_error_free(gError);
    gError = NULL;
  }
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  sbAutoGMemPtr autoSyncPlaylistList(syncPlaylistList);

  // Save the sync playlist list.
  mSyncPlaylistList.Clear();
  guint64* appendSyncPlaylistList =
             mSyncPlaylistList.AppendElements(syncPlaylistList,
                                              syncPlaylistCount);
  NS_ENSURE_TRUE(appendSyncPlaylistList, NS_ERROR_OUT_OF_MEMORY);

  // If a valid iPod preferences file was not available, set one up.
  if (!(mIPodPrefs->valid_file)) {
    // Write the preferences file.
    mIPodPrefsDirty = PR_TRUE;
    CreateAndDispatchEvent(sbIDeviceEvent::EVENT_DEVICE_PREFS_CHANGED, nsnull);
    rv = PushRequest(REQUEST_WRITE_PREFS);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Initialize the sync partner preference.
  rv = PrefInitializeSyncPartner();
  NS_ENSURE_SUCCESS(rv, rv);

  // Forget auto-disconnect.
  autoDisconnect.forget();

  return NS_OK;
}


/**
 * \brief Disconnect the iPod device iPod preferences file services.
 *
 * Since this function is called as a part of the preference connection process,
 * it does not need to operate under the pref lock.
 */

void
sbIPDDevice::PrefDisconnectIPodPrefs()
{
  // Dispose of the iPod preferences data record.
  if (mIPodPrefs)
    g_free(mIPodPrefs);
  mIPodPrefs = NULL;

  // Clear the sync playlist list.
  mSyncPlaylistList.Clear();
}


/**
 * Initialize the sync partner preference.
 */

nsresult
sbIPDDevice::PrefInitializeSyncPartner()
{
  nsresult rv;

  // Get the Songbird sync partner ID from the iPod sync partner ID.  Try
  // getting a Songbird sync partner ID mapped to the iPod sync partner ID.  If
  // none mapped, use the iPod sync partner ID as the Songbird sync partner ID
  // but don't create a mapping between them.
  nsAutoString sbSyncPartnerID;
  guint64      iPodSyncPartnerID = mIPodPrefs->music_lib_link_id;
  rv = IDMapGet(iPodSyncPartnerID, sbSyncPartnerID);
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    char sbSyncPartnerIDLiteral[64];
    PR_snprintf(sbSyncPartnerIDLiteral,
                sizeof(sbSyncPartnerIDLiteral),
                "%08x%08x",
                (PRUint32) ((iPodSyncPartnerID >> 32) & 0xFFFFFFFF),
                (PRUint32) (iPodSyncPartnerID & 0xFFFFFFFF));
    sbSyncPartnerID.AssignLiteral(sbSyncPartnerIDLiteral);
  } else {
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Set the internal sync partner ID preference.
  rv = sbBaseDevice::SetPreferenceInternalNoNotify(
                       NS_LITERAL_STRING("SyncPartner"),
                       sbIPDVariant(sbSyncPartnerID).get(),
                       nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

