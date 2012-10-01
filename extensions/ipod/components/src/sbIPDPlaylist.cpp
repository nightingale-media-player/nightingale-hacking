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
//
// iPod device playlist services.
//
//   These services may only be used within both the connect and request locks.
//
//------------------------------------------------------------------------------

/**
 * \file  sbIPDPlaylist.cpp
 * \brief Songbird iPod Device Playlist Source.
 */

//------------------------------------------------------------------------------
//
// iPod device playlist imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIPDDevice.h"
#include "sbIPDLog.h"
#include "sbIPDUtils.h"

// Songbird imports.
#include <sbDeviceUtils.h>
#include <sbLibraryListenerHelpers.h>
#include <sbStringUtils.h>

/* Mozilla imports. */
#include <nsComponentManagerUtils.h>
#include <nsISimpleEnumerator.h>


//------------------------------------------------------------------------------
//
// iPod device playlist import services.
//
//------------------------------------------------------------------------------

/**
 * Import the iPod database playlists into the Songbird device library.
 */

nsresult
sbIPDDevice::ImportPlaylists()
{
  // Import all of the playlists, skipping the master plalist.
  GList* playlistList = mITDB->playlists;
  while (playlistList) {
    // Check for abort.
    NS_ENSURE_FALSE(IsRequestAborted(), NS_ERROR_ABORT);

    // Get the playlist.
    Itdb_Playlist* playlist = (Itdb_Playlist *) playlistList->data;
    playlistList = playlistList->next;

    // Do nothing with the master playlist.
    if (itdb_playlist_is_mpl(playlist))
        continue;

    // Import the playlist.
    ImportPlaylist(playlist);
  }

  return NS_OK;
}


/**
 * Import the iPod database playlist specified by aPlaylist into the Songbird
 * device library.
 *
 * \param aPlaylist             iPod playlist to import.
 */

nsresult
sbIPDDevice::ImportPlaylist(Itdb_Playlist* aPlaylist)
{
  // Validate arguments.
  NS_ASSERTION(aPlaylist, "aPlaylist is null");

  // Import the playlist into the device Songbird library.
  return ImportPlaylist(mDeviceLibrary, aPlaylist);
}


/**
 * Import the iPod database playlist specified by aPlaylist into the Songbird
 * library specified by aLibrary.  If aMediaList is not null, the imported
 * media is returned in aMediaList.
 *
 * \param aLibrary              Library in which to import playlist.
 * \param aPlaylist             iPod playlist to import.
 * \param aMediaList            Imported media list.
 */

nsresult
sbIPDDevice::ImportPlaylist(sbILibrary*    aLibrary,
                            Itdb_Playlist* aPlaylist,
                            sbIMediaList** aMediaList)
{
  // Validate arguments.
  NS_ASSERTION(aLibrary, "aLibrary is null");
  NS_ASSERTION(aPlaylist, "aPlaylist is null");

  // Function variables.
  nsCOMPtr<sbIMediaList> mediaList;
  nsresult               rv;

  // Log progress.
  FIELD_LOG(("Importing playlist %s\n", aPlaylist->name));

  // Check if importing into the device library.
  PRBool inDeviceLibrary;
  rv = aLibrary->Equals(mDeviceLibrary, &inDeviceLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  // Ignore library and media list changes for device libraries and set up to
  // automatically stop ignoring.
  sbIPDAutoStopIgnoreLibrary    autoStopIgnoreLibrary;
  sbIPDAutoStopIgnoreMediaLists autoStopIgnoreMediaLists;
  if (inDeviceLibrary) {
    mLibraryListener->SetIgnoreListener(PR_TRUE);
    autoStopIgnoreLibrary.Set(mLibraryListener);
    SetIgnoreMediaListListeners(PR_TRUE);
    autoStopIgnoreMediaLists.Set(this);
  }

  // If importing into the device library, check if playlist media list already
  // exists.  Clear it out and import into it if so.
  if (inDeviceLibrary) {
    nsCOMPtr<sbIMediaItem> mediaItem;
    rv = sbDeviceUtils::GetMediaItemByDevicePersistentId
                          (aLibrary,
                           sbAutoString(aPlaylist->id),
                           getter_AddRefs(mediaItem));
    if (NS_SUCCEEDED(rv)) {
      mediaList = do_QueryInterface(mediaItem, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mediaList->Clear();
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      NS_ENSURE_TRUE(rv == NS_ERROR_NOT_AVAILABLE, rv);
    }
  }

  // Create a new media list if needed.
  if (!mediaList) {
    rv = aLibrary->CreateMediaList(NS_LITERAL_STRING("simple"),
                                   nsnull,
                                   getter_AddRefs(mediaList));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Set the media list name.
  nsAutoString playlistName;
  playlistName.Assign(NS_ConvertUTF8toUTF16(aPlaylist->name));
  rv = mediaList->SetName(playlistName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the device persistent ID and availability if importing into the device
  // library.
  if (inDeviceLibrary) {
    rv = mediaList->SetProperty
                      (NS_LITERAL_STRING(SB_PROPERTY_DEVICE_PERSISTENT_ID),
                       sbAutoString(aPlaylist->id));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mediaList->SetProperty
                      (NS_LITERAL_STRING(SB_PROPERTY_AVAILABILITY),
                       NS_LITERAL_STRING("1"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Stop ignoring library and media list changes for device libraries.
  if (inDeviceLibrary) {
    mLibraryListener->SetIgnoreListener(PR_FALSE);
    autoStopIgnoreLibrary.forget();
    SetIgnoreMediaListListeners(PR_FALSE);
    autoStopIgnoreMediaLists.forget();
  }

  // Add the playlist tracks to the media list.
  rv = ImportPlaylistTracks(aPlaylist, mediaList);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return imported media list.
  if (aMediaList)
    NS_ADDREF(*aMediaList = mediaList);

  return NS_OK;
}


/**
 * Import the tracks from the iPod playlist specified by aPlaylist into the
 * media list specified by aMediaList.
 *
 * \param aPlaylist             iPod playlist from which to import tracks.
 * \param aMediaList            Media list into which to import tracks.
 */

nsresult
sbIPDDevice::ImportPlaylistTracks(Itdb_Playlist* aPlaylist,
                                  sbIMediaList*  aMediaList)
{
  // Validate arguments.
  NS_ASSERTION(aPlaylist, "aPlaylist is null");
  NS_ASSERTION(aMediaList, "aMediaList is null");

  // Allocate the track batch and set it up for auto-disposal.
  Itdb_Track** trackBatch =
                 static_cast<Itdb_Track**>
                   (NS_Alloc(TrackBatchSize * sizeof (Itdb_Track *)));
  NS_ENSURE_TRUE(trackBatch, NS_ERROR_OUT_OF_MEMORY);
  sbAutoNSMemPtr autoTrackBatch(trackBatch);

  // Import all of the playlist tracks.
  int trackCount = itdb_playlist_tracks_number(aPlaylist);
  GList* trackList = aPlaylist->members;
  PRUint32 trackNum = 0;
  PRUint32 batchCount = 0;
  while (trackList) {
    // Check for abort.
    NS_ENSURE_FALSE(IsRequestAborted(), NS_ERROR_ABORT);

    // Get the track.
    Itdb_Track* track = (Itdb_Track *) trackList->data;
    trackList = trackList->next;

    // Update status.
    mIPDStatus->ItemStart(aPlaylist, track, trackNum, trackCount);

    // Add the track to the batch.
    trackBatch[batchCount] = track;
    batchCount++;

    // Import the track batch.
    if ((batchCount >= TrackBatchSize) || (!trackList)) {
      ImportPlaylistTrackBatch(aMediaList, trackBatch, batchCount);
      batchCount = 0;
    }

    // Import next track.
    trackNum++;
  }

  return NS_OK;
}


/**
 * Import the batch of iPod database tracks specified by aTrackBatch and
 * aBatchCount into the media list specified by aMediaList.
 *
 * \param aMediaList            Media list into which to import.
 * \param aTrackBatch           TrackBatch to import.
 * \param aBatchCount           Number of tracks in batch.
 */

nsresult
sbIPDDevice::ImportPlaylistTrackBatch(sbIMediaList* aMediaList,
                                      Itdb_Track**  aTrackBatch,
                                      PRUint32      aBatchCount)
{
  // Validate arguments.
  NS_ASSERTION(aMediaList, "aMediaList is null");
  NS_ASSERTION(aTrackBatch, "aTrackBatch is null");

  // Function variables.
  nsresult rv;

  // Get the target library and check if it's the device library.
  nsCOMPtr<sbILibrary> tgtLibrary;
  PRBool               inDeviceLibrary;
  rv = aMediaList->GetLibrary(getter_AddRefs(tgtLibrary));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = tgtLibrary->Equals(mDeviceLibrary, &inDeviceLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  // Ignore library and media list changes for device libraries and set up to
  // automatically stop ignoring.
  sbIPDAutoStopIgnoreLibrary    autoStopIgnoreLibrary;
  sbIPDAutoStopIgnoreMediaLists autoStopIgnoreMediaLists;
  if (inDeviceLibrary) {
    mLibraryListener->SetIgnoreListener(PR_TRUE);
    autoStopIgnoreLibrary.Set(mLibraryListener);
    SetIgnoreMediaListListeners(PR_TRUE);
    autoStopIgnoreMediaLists.Set(this);
  }

  // Create the track media item array.
  nsCOMPtr<nsIMutableArray> trackMediaItemArray =
                              do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the track media items.
  for (PRUint32 i = 0; i < aBatchCount; i++) {
    // Get the track media item.  Skip track on failure.
    Itdb_Track* track = aTrackBatch[i];
    nsCOMPtr<sbIMediaItem> mediaItem;
    if (inDeviceLibrary) {
      rv = sbDeviceUtils::GetMediaItemByDevicePersistentId
                            (mDeviceLibrary,
                             sbAutoString(track->dbid),
                             getter_AddRefs(mediaItem));
    } else {
      rv = sbDeviceUtils::GetOriginMediaItemByDevicePersistentId
                            (mDeviceLibrary,
                             sbAutoString(track->dbid),
                             getter_AddRefs(mediaItem));
    }
    if (NS_SUCCEEDED(rv))
      trackMediaItemArray->AppendElement(mediaItem, PR_FALSE);
  }
  nsCOMPtr<nsISimpleEnumerator> trackMediaItemEnum;
  rv = trackMediaItemArray->Enumerate(getter_AddRefs(trackMediaItemEnum));
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the track media items.
  rv = aMediaList->AddSome(trackMediaItemEnum);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// iPod device playlist manipulation services.
//
//------------------------------------------------------------------------------

/**
 * Add the playlist specified by aMediaList to the iPod device and return the
 * new playlist in aPlaylist.
 *
 * \param aMediaList            Playlist media list.
 * \param aPlaylist             Added playlist.
 */

nsresult
sbIPDDevice::PlaylistAdd(sbIMediaList*   aMediaList,
                         Itdb_Playlist** aPlaylist)
{
  //XXXeps add support for adding from non-dev playlist.
  // Validate arguments.
  NS_ASSERTION(aMediaList, "aMediaList is null");
  NS_ASSERTION(aPlaylist, "aPlaylist is null");

  // Function variables.
  nsresult rv;

  // Get the playlist name.
  nsAutoString playlistName;
  rv = aMediaList->GetName(playlistName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the iPod playlist and add it to the iPod database.
  Itdb_Playlist* playlist = itdb_playlist_new
                              (NS_ConvertUTF16toUTF8(playlistName).get(),
                               FALSE);
  NS_ENSURE_TRUE(playlist, NS_ERROR_OUT_OF_MEMORY);
  itdb_playlist_add(mITDB, playlist, -1);

  // Set the media list device persistent ID to the new playlist ID.
  mLibraryListener->IgnoreMediaItem(aMediaList);
  rv = aMediaList->SetProperty
                     (NS_LITERAL_STRING(SB_PROPERTY_DEVICE_PERSISTENT_ID),
                      sbAutoString(playlist->id));
  mLibraryListener->UnignoreMediaItem(aMediaList);
  NS_ENSURE_SUCCESS(rv, rv);

  // Mark the iPod database as dirty.
  mITDBDirty = PR_TRUE;

  // Return results.
  *aPlaylist = playlist;

  return NS_OK;
}


/**
 * Delete the iPod playlist corresponding to the Songbird device library media
 * list specified by aMediaList.
 */

nsresult
sbIPDDevice::PlaylistDelete(sbIMediaList* aMediaList)
{
  // Validate arguments.
  NS_ASSERTION(aMediaList, "aMediaList is null");

  // Function variables.
  nsresult rv;

  // Get the iPod playlist to delete.
  Itdb_Playlist* playlist;
  rv = GetPlaylist(aMediaList, &playlist);
  NS_ENSURE_SUCCESS(rv, rv);

  // Remove playlist from the iPod.
  itdb_playlist_remove(playlist);

  // Mark the iPod database as dirty.
  mITDBDirty = PR_TRUE;

  return NS_OK;
}


/**
 * Update the iPod playlist specified by aPlaylist with the properties from the
 * media list specified by aMediaList.  If aPlaylist is not specified, update
 * the playlist mapped to the specified media list.
 *
 * \param aMediaList            Media list from which to update.
 * \param aPlaylist             Playlist to update.
 */

nsresult
sbIPDDevice::PlaylistUpdateProperties(sbIMediaList*  aMediaList,
                                      Itdb_Playlist* aPlaylist)
{
  // Validate arguments.
  NS_ASSERTION(aMediaList, "aMediaList is null");

  // Function variables.
  nsresult rv;

  // Get the iPod playlist.
  Itdb_Playlist* playlist = aPlaylist;
  if (!playlist) {
    rv = GetPlaylist(aMediaList, &playlist);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Get the playlist name and arrange for auto-cleanup.
  nsAutoString nsPlaylistName;
  gchar*       cPlaylistName;
  rv = aMediaList->GetName(nsPlaylistName);
  NS_ENSURE_SUCCESS(rv, rv);
  cPlaylistName = g_strdup(NS_ConvertUTF16toUTF8(nsPlaylistName).get());
  NS_ENSURE_TRUE(cPlaylistName, NS_ERROR_OUT_OF_MEMORY);
  sbAutoGMemPtr autoCPlaylistName(cPlaylistName);

  // Update the playlist name if it's changed.
  if (strcmp(cPlaylistName, playlist->name)) {
    if (playlist->name)
      g_free(playlist->name);
    playlist->name = cPlaylistName;
    autoCPlaylistName.forget();
  }

  // Mark the iPod database as dirty.
  mITDBDirty = PR_TRUE;

  return NS_OK;
}


/**
 * Add the iPod track corresponding to the Songbird device library media item
 * specified by aMediaItem to the iPod playlist corresponding to the Songbird
 * device library media list specified by aMediaList at the playlist index
 * specified by aIndex.  When this function completes, the added track will
 * reside at the specified index.
 *
 * \param aMediaList            Media list of playlist to which to add track.
 * \param aMediaItem            Media item of track to add.
 * \param aIndex                Index in playlist at which to add track.
 */

nsresult
sbIPDDevice::PlaylistAddTrack(sbIMediaList* aMediaList,
                              sbIMediaItem* aMediaItem,
                              PRUint32      aIndex)
{
  // Validate arguments.
  NS_ASSERTION(aMediaList, "aMediaList is null");
  NS_ASSERTION(aMediaItem, "aMediaItem is null");

  // Function variables.
  nsresult rv;

  // Get the target iPod playlist.
  Itdb_Playlist* playlist;
  rv = GetPlaylist(aMediaList, &playlist);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the target iPod track.
  Itdb_Track* track;
  rv = GetTrack(aMediaItem, &track);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the track to the playlist.
  itdb_playlist_add_track(playlist, track, aIndex);

  // Mark the iPod database as dirty.
  mITDBDirty = PR_TRUE;

  return NS_OK;
}


/**
 * Remove the iPod track corresponding to the Songbird device library media item
 * specified by aMediaItem from the iPod playlist corresponding to the Songbird
 * device library media list specified by aMediaList at the playlist index
 * specified by aIndex.
 *
 * \param aMediaList            Media list of playlist from which to remove
 *                              track.
 * \param aMediaItem            Media item of track to remove.
 * \param aIndex                Index in playlist at which to remove track.
 */

nsresult
sbIPDDevice::PlaylistRemoveTrack(sbIMediaList* aMediaList,
                                 sbIMediaItem* aMediaItem,
                                 PRUint32      aIndex)
{
  // Validate arguments.
  NS_ASSERTION(aMediaList, "aMediaList is null");
  NS_ASSERTION(aMediaItem, "aMediaItem is null");

  // Function variables.
  nsresult rv;

  // Get the target iPod playlist.
  Itdb_Playlist* playlist;
  rv = GetPlaylist(aMediaList, &playlist);
  NS_ENSURE_SUCCESS(rv, rv);

  // Remove the track from the playlist.
  //XXXeps libgpod does not provide a direct interface for doing this.
  GList* members = playlist->members;
  GList* trackMember = g_list_nth(members, aIndex);
  NS_ENSURE_TRUE(trackMember, NS_ERROR_INVALID_ARG);
  playlist->members = g_list_delete_link(members, trackMember);

  // Mark the iPod database as dirty.
  mITDBDirty = PR_TRUE;

  return NS_OK;
}

/**
 * Wipes the playlist, clearing out all the tracks
 * \param aMediaList the playlist being wiped
 */
nsresult
sbIPDDevice::PlaylistWipe(sbIMediaList * aMediaList) {
  // Get the target iPod playlist.
  Itdb_Playlist* playlist;
  nsresult rv = GetPlaylist(aMediaList, &playlist);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the members to an empty list and free the old one
  GList* members = playlist->members;
  playlist->members = g_list_alloc();
  g_list_free(members);

  // Mark the iPod database as dirty.
  mITDBDirty = PR_TRUE;

  return NS_OK;
}

/**
 * Move the iPod track within the iPod playlist corresponding to the Songbird
 * device library media list specified by aMediaList from the playlist index
 * specified by aIndexFrom to the playlist index specified by aIndexTo.  When
 * this function completes, the moved track will reside at the index specified
 * by aIndexTo.
 *
 * \param aMediaList            Media list of playlist in which to move track.
 * \param aIndexFrom            Index in playlist from which to move track.
 * \param aIndexTo              Index in playlist to which to move track.
 */

nsresult
sbIPDDevice::PlaylistMoveTrack(sbIMediaList* aMediaList,
                               PRUint32      aIndexFrom,
                               PRUint32      aIndexTo)
{
  // Validate arguments.
  NS_ASSERTION(aMediaList, "aMediaList is null");

  // Function variables.
  nsresult rv;

  // Get the target iPod playlist.
  Itdb_Playlist* playlist;
  rv = GetPlaylist(aMediaList, &playlist);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the target iPod track.
  GList*      members = playlist->members;
  GList*      trackMember = g_list_nth(members, aIndexFrom);
  NS_ENSURE_TRUE(trackMember, NS_ERROR_INVALID_ARG);
  Itdb_Track* track = (Itdb_Track *) trackMember->data;

  // Remove the track from the current index in the playlist.
  //XXXeps libgpod does not provide a direct interface for doing this.
  playlist->members = g_list_delete_link(members, trackMember);

  // Add the track to the new index in the playlist.
  itdb_playlist_add_track(playlist, track, aIndexTo);

  // Mark the iPod database as dirty.
  mITDBDirty = PR_TRUE;

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// iPod device on-the-go playlist services.
//
//------------------------------------------------------------------------------

/**
 * Process the on-the-go playlists in the iPod database.  Set unique names for
 * each one and make them it a normal playlist.
 */

nsresult
sbIPDDevice::ProcessOTGPlaylists()
{
  nsresult                    rv;

  // Check for on-the-go playlists and process them.
  PRBool otgPlaylistsPresent = PR_FALSE;
  PRUint32 otgPlaylistIndex = 1;
  GList* playlistList = mITDB->playlists;
  while (playlistList) {
    // Check for abort.
    NS_ENSURE_FALSE(IsRequestAborted(), NS_ERROR_ABORT);

    // Get the playlist.
    Itdb_Playlist* playlist = (Itdb_Playlist *) playlistList->data;
    playlistList = playlistList->next;

    // If the playlist is an on-the-go playlist, set its name.  Remove
    // playlist on failure.  Increment the playlist index regardless to
    // match index shown on iPod.
    if (playlist->is_otg) {
      // Set the playlist name.  Remove it on failure.
      otgPlaylistsPresent = PR_TRUE;
      rv = SetOTGPlaylistName(playlist, otgPlaylistIndex);
      if (NS_FAILED(rv))
        itdb_playlist_remove(playlist);
      otgPlaylistIndex++;

      // Mark the iPod database as dirty.
      mITDBDirty = PR_TRUE;
    }
  }

  return NS_OK;
}


/**
 * Set the name of the on-the-go playlist specified by aPlaylist with the index
 * specified by aPlaylistIndex.
 *
 * \param aPlaylist             On-the-go playlist for which to set name.
 * \param aPlaylistIndex        Index of the on-the-go playlist.
 */

nsresult
sbIPDDevice::SetOTGPlaylistName(Itdb_Playlist* aPlaylist,
                                PRUint32       aPlaylistIndex)
{
  // Validate arguments.
  NS_ASSERTION(aPlaylist, "aPlaylist is null");

  // Function variables.
  nsresult rv;

  // Produce the playlist name.
  nsAutoString playlistName;
  sbAutoString playlistIndex(aPlaylistIndex);
  const PRUnichar *stringList[1] = { playlistIndex.get() };
  rv = mLocale->FormatStringFromName
                  (NS_LITERAL_STRING("on_the_go.playlist_name").get(),
                   stringList,
                   1,
                   getter_Copies(playlistName));
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the playlist name.
  gchar *cPlaylistName = g_strdup(NS_ConvertUTF16toUTF8(playlistName).get());
  NS_ENSURE_TRUE(cPlaylistName, NS_ERROR_OUT_OF_MEMORY);
  if (aPlaylist->name)
    g_free(aPlaylist->name);
  aPlaylist->name = cPlaylistName;

  // Mark the iPod database as dirty.
  mITDBDirty = PR_TRUE;

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// iPod device playlist services.
//
//------------------------------------------------------------------------------

/**
 * Return in aPlaylist the playlist corresponding to the media item specified by
 * aMediaItem.
 *
 * \param aMediaItem            Media item for which to get playlist.
 * \param aPlaylist             Playlist corresponding to media item.
 */

nsresult
sbIPDDevice::GetPlaylist(sbIMediaItem*    aMediaItem,
                         Itdb_Playlist**  aPlaylist)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aPlaylist, "aPlaylist is null");

  // Function variables.
  Itdb_Playlist* playlist;
  nsresult       rv;

  // Get the iPod ID.
  guint64 iPodID;
  rv = GetIPodID(aMediaItem, &iPodID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the playlist.
  playlist = itdb_playlist_by_id(mITDB, iPodID);
  NS_ENSURE_TRUE(playlist, NS_ERROR_NOT_AVAILABLE);

  // Return results.
  *aPlaylist = playlist;

  return NS_OK;
}

