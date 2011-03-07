/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2011 POTI, Inc.
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
// iPod device sync services.
//
//   These services may only be used within both the connect and request locks.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbIPDSync.cpp
 * \brief Songbird iPod Device Sync Source.
 */

//------------------------------------------------------------------------------
//
// iPod device sync imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIPDDevice.h"

// Songbird imports.
#include <sbDeviceUtils.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>

#include <sbIDeviceLibraryMediaSyncSettings.h>
#include <sbIDeviceLibrarySyncSettings.h>

//------------------------------------------------------------------------------
//
// iPod device sync track services.
//
//------------------------------------------------------------------------------

/**
 * Synchronize from the iPod tracks to the Songbird library.
 */

nsresult
sbIPDDevice::SyncFromIPodTracks()
{
  // Sync from all of the iPod tracks.
  GList* trackList = mITDB->tracks;
  while (trackList) {
    // Check for abort.
    NS_ENSURE_FALSE(IsRequestAborted(), NS_ERROR_ABORT);

    // Get the track.
    Itdb_Track* track = (Itdb_Track*) trackList->data;
    trackList = trackList->next;

    // Synchronize from the iPod track.
    SyncFromIPodTrack(track);
  }

  return NS_OK;
}


/**
 * Synchronize from the device track specified by aTrack to the Songbird
 * library.
 *
 * \param aTrack                device track from which to sync.
 */

nsresult
sbIPDDevice::SyncFromIPodTrack(Itdb_Track* aTrack)
{
  // Validate arguments.
  NS_ASSERTION(aTrack, "aTrack is null");

  // Function variables.
  nsresult rv;

  // Search for a matching Songbird media item.
  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = sbDeviceUtils::GetOriginMediaItemByDevicePersistentId
                        (mDeviceLibrary,
                         sbAutoString(aTrack->dbid),
                         getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  // Synchronize the playcount metadata.
  if (aTrack->playcount2 > 0) {
    // Get the Songbird track playcount.
    guint32 playcount = 0;
    rv = GetTrackProp(mediaItem, SB_PROPERTY_PLAYCOUNT, &playcount);
    NS_ENSURE_SUCCESS(rv, rv);

    // Add and clear the device playcount.
    playcount += aTrack->playcount2;
    aTrack->playcount2 = 0;

    // Write the Songbird track playcount.
    rv = SetTrackProp(mediaItem, SB_PROPERTY_PLAYCOUNT, playcount);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// iPod device sync services.
//
//------------------------------------------------------------------------------

/**
 * Synchronize media from the iPod device to the Songbird library.
 */

nsresult
sbIPDDevice::SyncFromDevice()
{
  nsresult rv;

  // Set the SyncPartner preference.
  rv = sbDeviceUtils::SetLinkedSyncPartner(this);
  NS_ENSURE_SUCCESS(rv, rv);

  // Synchronize from device tracks.
  rv = SyncFromIPodTracks();
  NS_ENSURE_SUCCESS(rv, rv);

  // Synchronize from on-the-go playlists.
  rv = SyncFromOTGPlaylists();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Synchronize from the iPod on-the-go playlists to the Songbird library.
 */

nsresult
sbIPDDevice::SyncFromOTGPlaylists()
{
  nsresult                    rv;

  // Synchronize from each on-the-go playlist.
  GList* playlistList = mITDB->playlists;
  while (playlistList) {
    // Check for abort.
    NS_ENSURE_FALSE(IsRequestAborted(), NS_ERROR_ABORT);

    // Get the playlist.
    Itdb_Playlist* playlist = (Itdb_Playlist*) playlistList->data;
    playlistList = playlistList->next;

    // If the playlist is an on-the-go playlist, sync it to the Songbird
    // main library.
    if (playlist->is_otg) {
      // Import on-the-go playlists into the main Songbird library.
      nsCOMPtr<sbIMediaList> mediaList;
      rv = ImportPlaylist(mSBMainLib, playlist, getter_AddRefs(mediaList));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbIDeviceLibrarySyncSettings> syncSettings;
      rv = mDeviceLibrary->GetSyncSettings(getter_AddRefs(syncSettings));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbIDeviceLibraryMediaSyncSettings> mediaSyncSettings;
      rv = syncSettings->GetMediaSettings(sbIDeviceLibrary::MEDIATYPE_AUDIO,
                                          getter_AddRefs(mediaSyncSettings));
      NS_ENSURE_SUCCESS(rv, rv);

      // If the device is set to sync to a list of playlists, add the
      // on-the-go playlist to the sync list.
      PRUint32 mgmtType;
      rv = mediaSyncSettings->GetMgmtType(&mgmtType);
      if (NS_SUCCEEDED(rv) &&
          (mgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_PLAYLISTS)) {
        // Add the on-the-go playlist to the sync list.
        mediaSyncSettings->SetPlaylistSelected(mediaList, PR_TRUE);
      }
    }
  }

  return NS_OK;
}

