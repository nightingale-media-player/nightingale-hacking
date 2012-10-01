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
// iPod device track services.
//
//   These services may only be used within both the connect and request locks.
//
//------------------------------------------------------------------------------

/**
 * \file  sbIPDTrack.cpp
 * \brief Songbird iPod Device Track Source.
 */

//------------------------------------------------------------------------------
//
// iPod device track imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIPDDevice.h"
#include "sbIPDLog.h"
#include "sbIPDUtils.h"

// Songbird imports.
#include <sbDeviceUtils.h>
#include <sbIDeviceEvent.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyInfo.h>
#include <sbLibraryListenerHelpers.h>
#include <sbMediaListBatchCallback.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>

// Mozila imports.
#include <nsArrayUtils.h>
#include <nsILocalFile.h>
#include <nsIMutableArray.h>
#include <nsIURI.h>
#include <nsIURL.h>
#include <nsTArray.h>
#include <prprf.h>

// glib imports.
#include <glib/gstdio.h>


//------------------------------------------------------------------------------
//
// iPod device track configuration.
//
//------------------------------------------------------------------------------

//
// IPOD_DEVICE_BATCH_SIZE       Number of tracks to process in a batch.
//

#define IPOD_DEVICE_BATCH_SIZE 100


//------------------------------------------------------------------------------
//
// iPod device track services.
//
//------------------------------------------------------------------------------

/**
 * Import the iPod database tracks into the Songbird device library.
 */

nsresult
sbIPDDevice::ImportTracks()
{
  // Allocate the track batch and set it up for auto-disposal.
  Itdb_Track** trackBatch =
                 static_cast<Itdb_Track**>
                   (NS_Alloc(IPOD_DEVICE_BATCH_SIZE * sizeof (Itdb_Track *)));
  NS_ENSURE_TRUE(trackBatch, NS_ERROR_OUT_OF_MEMORY);
  sbAutoNSMemPtr autoTrackBatch(trackBatch);

  // Import all of the media tracks.
  int trackCount = itdb_tracks_number(mITDB);
  GList* trackList = mITDB->tracks;
  PRUint32 trackNum = 0;
  PRUint32 batchCount = 0;
  while (trackList) {
    // Check for abort.
    NS_ENSURE_FALSE(IsRequestAborted(), NS_ERROR_ABORT);

    // Get the track.
    Itdb_Track* track = (Itdb_Track*) trackList->data;
    trackList = trackList->next;

    // Update status.
    mIPDStatus->ItemStart(track, trackNum, trackCount);

    // Add the track to the batch.
    trackBatch[batchCount] = track;
    batchCount++;

    // Import the track batch.
    if ((batchCount >= IPOD_DEVICE_BATCH_SIZE) || (!trackList)) {
      ImportTrackBatch(trackBatch, batchCount);
      batchCount = 0;
    }

    // Import the next track.
    trackNum++;
  }

  return NS_OK;
}


/**
 * Upload the media item specified by aMediaItem to the iPod device.
 *
 * \param aMediaItem            Media item to upload.
 */

nsresult
sbIPDDevice::UploadTrack(sbIMediaItem* aMediaItem)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");

  // Function variables.
  nsresult rv;

  // Add the track to the iPod.
  Itdb_Track* track;
  rv = AddTrack(aMediaItem, &track);
  NS_ENSURE_SUCCESS(rv, rv);

  // Update the media item content source property to point to the device media
  // file.
  nsCOMPtr<nsIURI> trackURI;
  rv = GetTrackURI(track, getter_AddRefs(trackURI));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aMediaItem->SetContentSrc(trackURI);
  NS_ENSURE_SUCCESS(rv, rv);

  // Show the media item.
  rv = aMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                               NS_LITERAL_STRING("0"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Add the track specified by aMediaItem to the iPod device and return the new
 * track in aTrack.  This function creates the track, adds it to the iPod
 * database, and copies the track file to the iPod.
 *
 * \param aMediaItem            Track media item.
 * \param aTrack                Added track.
 */

nsresult
sbIPDDevice::AddTrack(sbIMediaItem* aMediaItem,
                      Itdb_Track**  aTrack)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aTrack, "aTrack is null");

  // Function variables.
  GError   *gError = NULL;
  nsresult rv;

  /* Return immediately if media item is not supported. */
  if (!IsMediaSupported(aMediaItem)) {
    AddUnsupportedMediaItem(aMediaItem);
    return NS_ERROR_ILLEGAL_VALUE;
  }

  // Create a new iPod track data record and set it up for auto-disposal.
  Itdb_Track* track = itdb_track_new();
  NS_ENSURE_TRUE(track, NS_ERROR_OUT_OF_MEMORY);
  sbIPDAutoTrack autoTrack(track, this);

  // Set the track properties.
  SetTrackProperties(track, aMediaItem);

  // Get the track file.
  nsCOMPtr<nsIURI>  trackURI;
  nsCAutoString     trackSpec;
  nsCOMPtr<nsIFile> trackFile;
  rv = aMediaItem->GetContentSrc(getter_AddRefs(trackURI));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = trackURI->GetSpec(trackSpec);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mFileProtocolHandler->GetFileFromURLSpec(trackSpec,
                                                getter_AddRefs(trackFile));
  NS_ENSURE_SUCCESS(rv, rv);

  // Set up any FairPlay info.
  rv = FPSetupTrackInfo(trackFile, track);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the track path.
  nsCAutoString trackPath;
  rv = trackFile->GetNativePath(trackPath);
  NS_ENSURE_SUCCESS(rv, rv);

  /* Trace execution. */
  FIELD_LOG(("1: sbIPDDevice::CreateTrack %s\n", trackPath.get()));

  // Get the track size.
  PRInt64 fileSize;
  rv = trackFile->GetFileSize(&fileSize);
  NS_ENSURE_SUCCESS(rv, rv);
  if (fileSize > (1 << 30)) {
    AddUnsupportedMediaItem(aMediaItem);
    return NS_ERROR_ILLEGAL_VALUE;
  }
  track->size = (guint32) (fileSize & 0xFFFFFFFF);

  // Add the track to the iPod database.
  itdb_track_add(mITDB, track, -1);

  // Add the track to the master playlist.
  itdb_playlist_add_track(mMasterPlaylist, track, -1);

  // Copy the track media file to the iPod.
  if (!itdb_cp_track_to_ipod(track, (gchar *) trackPath.get(), &gError)) {
    if (gError) {
      if (gError->message)
        FIELD_LOG(("%s", gError->message));
      g_error_free(gError);
      gError = NULL;
    }
    NS_ENSURE_TRUE(PR_FALSE, NS_ERROR_FAILURE);
  }
  mIPDStatus->ItemProgress(1.0);

  // Set the track media item device persistent ID.
  mLibraryListener->IgnoreMediaItem(aMediaItem);
  rv = aMediaItem->SetProperty
                     (NS_LITERAL_STRING(SB_PROPERTY_DEVICE_PERSISTENT_ID),
                      sbAutoString(track->dbid));
  mLibraryListener->UnignoreMediaItem(aMediaItem);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check for track FairPlay authorization.  Do this last so the FairPlay
  // information can be read from the track and a media item can be obtained
  // from the track.
  FPCheckTrackAuth(track);

  // Update statistics.
  StatsUpdate(PR_FALSE);

  // Mark the iPod database as dirty.
  mITDBDirty = PR_TRUE;

  // Return results.
  *aTrack = track;
  autoTrack.forget();

  return NS_OK;
}


/**
 * Delete the iPod track corresponding to the media item specified by
 * aMediaItem.
 */

nsresult
sbIPDDevice::DeleteTrack(sbIMediaItem* aMediaItem)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");

  // Function variables.
  nsresult rv;

  // Get the iPod track to delete.
  Itdb_Track* track;
  rv = GetTrack(aMediaItem, &track);
  NS_ENSURE_SUCCESS(rv, rv);

  // Delete the track.
  return DeleteTrack(track);
}


/**
 * Delete the iPod track specified by aTrack from the iPod.
 *
 * \param aTrack                iPod track to delete.
 */

nsresult
sbIPDDevice::DeleteTrack(Itdb_Track* aTrack)
{
  // Validate arguments.
  NS_ASSERTION(aTrack, "aTrack is null");

  // Get the track database.
  Itdb_iTunesDB* itdb = aTrack->itdb;

  // Remove track from all playlists.
  RemoveTrackFromAllPlaylists(aTrack);

  // Remove track from the iPod.
  if (itdb) {
    // Remove track file.
    gchar* fileName = itdb_filename_on_ipod(aTrack);
    if (fileName) {
      g_unlink(fileName);
      g_free(fileName);
    }

    // Remove track from database.
    itdb_track_remove(aTrack);
  }

  // Update statistics.
  StatsUpdate(PR_FALSE);

  // Mark the iPod database as dirty.
  mITDBDirty = PR_TRUE;

  // Update item progress.
  mIPDStatus->ItemProgress(1.0);

  return NS_OK;
}

/**
 * Removes all tracks from the devices
 */
nsresult
sbIPDDevice::WipeDevice() {

  // Import all of the media tracks.
  int trackCount = itdb_tracks_number(mITDB);
  GList* trackList = mITDB->tracks;
  PRUint32 trackNum = 0;
  while (trackList) {
    // Check for abort.
    NS_ENSURE_FALSE(IsRequestAborted(), NS_ERROR_ABORT);

    // Get the track.
    Itdb_Track* track = (Itdb_Track*) trackList->data;
    trackList = trackList->next;

    // Update status.
    mIPDStatus->ItemStart(track, trackNum, trackCount);

    // Ignore error and hope for the best
    DeleteTrack(track);
    // Import the next track.
    ++trackNum;
  }

  return NS_OK;
}

/**
 * Return in aTrack the track corresponding to the media item specified by
 * aMediaItem.
 *
 * \param aMediaItem            Media item for which to get track.
 * \param aTrack                Track corresponding to media item.
 */

nsresult
sbIPDDevice::GetTrack(sbIMediaItem* aMediaItem,
                      Itdb_Track**  aTrack)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aTrack, "aTrack is null");

  // Function variables.
  Itdb_Track* track;
  nsresult    rv;

  // Get the iPod ID.
  guint64 iPodID;
  rv = GetIPodID(aMediaItem, &iPodID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the track.
  track = itdb_track_by_dbid(mITDB, iPodID);
  NS_ENSURE_TRUE(track, NS_ERROR_NOT_AVAILABLE);

  // Return results.
  *aTrack = track;

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// iPod device track property services.
//
//------------------------------------------------------------------------------

/**
 * Set the track properties in the iPod track specified by aTrack to the values
 * read from the media item specified by aMediaItem.
 *
 * \param aTrack                iPod track for which to set properties.
 * \param aMediaItem            Media item from which to get properties.
 */

void
sbIPDDevice::SetTrackProperties(Itdb_Track*   aTrack,
                                sbIMediaItem* aMediaItem)
{
  // Validate arguments.
  NS_ASSERTION(aTrack, "aTrack is null");
  NS_ASSERTION(aMediaItem, "aMediaItem is null");

  // Get the track properties from the media item and set them in the iPod
  // track.
  GetTrackProp   (aMediaItem, SB_PROPERTY_TRACKNAME,    &(aTrack->title));
  GetTrackProp   (aMediaItem, SB_PROPERTY_ALBUMNAME,    &(aTrack->album));
  GetTrackProp   (aMediaItem, SB_PROPERTY_ARTISTNAME,   &(aTrack->artist));
  GetTrackProp   (aMediaItem, SB_PROPERTY_GENRE,        &(aTrack->genre));
  GetTrackProp   (aMediaItem, SB_PROPERTY_COMPOSERNAME, &(aTrack->composer));
  GetTrackPropDur(aMediaItem, SB_PROPERTY_DURATION,     &(aTrack->tracklen));
  GetTrackProp   (aMediaItem, SB_PROPERTY_DISCNUMBER,   &(aTrack->cd_nr));
  GetTrackProp   (aMediaItem, SB_PROPERTY_TOTALDISCS,   &(aTrack->cds));
  GetTrackProp   (aMediaItem, SB_PROPERTY_TRACKNUMBER,  &(aTrack->track_nr));
  GetTrackProp   (aMediaItem, SB_PROPERTY_TOTALTRACKS,  &(aTrack->tracks));
  GetTrackProp   (aMediaItem, SB_PROPERTY_YEAR,         &(aTrack->year));
  GetTrackPropRating(aMediaItem, SB_PROPERTY_RATING,    &(aTrack->rating));
  GetTrackProp   (aMediaItem, SB_PROPERTY_PLAYCOUNT,    &(aTrack->playcount));
  GetTrackPropFileType(aMediaItem, &(aTrack->filetype));
}


/**
 * Set the track properties in the media item specified by aMediaItem to the
 * values read from the iPod track specified by aTrack.
 *
 * \param aMediaItem            Media item for which to set properties.
 * \param aTrack                iPod track from which to get properties.
 */

void
sbIPDDevice::SetTrackProperties(sbIMediaItem* aMediaItem,
                                Itdb_Track*   aTrack)
{
  // Validate arguments.
  NS_ASSERTION(aTrack, "aTrack is null");
  NS_ASSERTION(aMediaItem, "aMediaItem is null");

  // Function variables.
  nsresult rv;

  // Get the track properties.
  nsCOMPtr<sbIPropertyArray> props;
  rv = GetTrackProperties(aTrack, getter_AddRefs(props));
  NS_ENSURE_SUCCESS(rv, /* void */);

  // Set the media item properties.
  mLibraryListener->IgnoreMediaItem(aMediaItem);
  rv = aMediaItem->SetProperties(props);
  mLibraryListener->UnignoreMediaItem(aMediaItem);
  NS_ENSURE_SUCCESS(rv, /* void */);
}


/**
 * Get the properties from the iPod track specified by aTrack and return them in
 * the property array specified by aPropertyArray.
 *
 * \param aTrack                iPod track from which to get properties.
 * \param aPropertyArray        Property array in which to return properties.
 */

nsresult
sbIPDDevice::GetTrackProperties(Itdb_Track*        aTrack,
                                sbIPropertyArray** aPropertyArray)
{
  // Validate arguments.
  NS_ASSERTION(aTrack, "aTrack is null");
  NS_ASSERTION(aPropertyArray, "aPropertyArray is null");

  // Function variables.
  nsresult rv;

  // Get a mutable property array.
  nsCOMPtr<sbIMutablePropertyArray>
    props = do_CreateInstance
              ("@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1",
               &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the track properties from the iPod track and set them in the property
  // array.
  SetTrackProp   (props, SB_PROPERTY_TRACKNAME,            aTrack->title);
  SetTrackProp   (props, SB_PROPERTY_ALBUMNAME,            aTrack->album);
  SetTrackProp   (props, SB_PROPERTY_ARTISTNAME,           aTrack->artist);
  SetTrackProp   (props, SB_PROPERTY_GENRE,                aTrack->genre);
  SetTrackProp   (props, SB_PROPERTY_COMPOSERNAME,         aTrack->composer);
  SetTrackPropDur(props, SB_PROPERTY_DURATION,             aTrack->tracklen);
  SetTrackProp   (props, SB_PROPERTY_DISCNUMBER,           aTrack->cd_nr);
  SetTrackProp   (props, SB_PROPERTY_TOTALDISCS,           aTrack->cds);
  SetTrackProp   (props, SB_PROPERTY_TRACKNUMBER,          aTrack->track_nr);
  SetTrackProp   (props, SB_PROPERTY_TOTALTRACKS,          aTrack->tracks);
  SetTrackProp   (props, SB_PROPERTY_YEAR,                 aTrack->year);
  SetTrackPropRating(props, SB_PROPERTY_RATING,            aTrack->rating);
  SetTrackProp   (props, SB_PROPERTY_PLAYCOUNT,            aTrack->playcount);
  SetTrackProp   (props, SB_PROPERTY_DEVICE_PERSISTENT_ID, aTrack->dbid);

  // The iPod track is available.
  SetTrackProp(props, SB_PROPERTY_AVAILABILITY, NS_LITERAL_STRING("1"));

  // Return results.
  NS_ADDREF(*aPropertyArray = props);

  return NS_OK;
}


/**
 * Get the properties from the iPod track specified by aTrack and return them in
 * the properties array specified by aPropertiesArrayArray.
 *
 * \param aTrack                iPod track from which to get properties.
 * \param aPropertiesArrayArray Properties array in which to return properties.
 */

nsresult
sbIPDDevice::GetTrackProperties(Itdb_Track*      aTrack,
                                nsIMutableArray* aPropertiesArrayArray)
{
  // Validate arguments.
  NS_ASSERTION(aTrack, "aTrack is null");
  NS_ASSERTION(aPropertiesArrayArray, "aPropertiesArrayArray is null");

  // Function variables.
  nsresult rv;

  // Get the track properties.
  nsCOMPtr<sbIPropertyArray> props;
  rv = GetTrackProperties(aTrack, getter_AddRefs(props));
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the track properties to the properties array.
  rv = aPropertiesArrayArray->AppendElement(props, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Update the iPod track specified by aTrack with the properties from the media
 * item specified by aMediaItem.  If aTrack is null, find the iPod track
 * corresponding to the specified media item and update its properties.
 *
 * \param aMediaItem            Media item from which to update properties.
 * \param aTrack                Track for which to update properties.
 */

nsresult
sbIPDDevice::TrackUpdateProperties(sbIMediaItem* aMediaItem,
                                   Itdb_Track*   aTrack)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");

  // Function variables.
  nsresult rv;

  // Get the iPod track.
  Itdb_Track* track = aTrack;
  if (!track) {
    rv = GetTrack(aMediaItem, &track);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Update the properties.
  SetTrackProperties(track, aMediaItem);

  // Mark the iPod database as dirty.
  mITDBDirty = PR_TRUE;

  return NS_OK;
}


/**
 * Get the property with the name specified by aPropName from the media item
 * specified by aMediaItem and return the string value in aProp.  The returned
 * string must be deallocated with g_free.
 *
 * \param aMediaItem            Media item from which to get property.
 * \param aPropName             Property name.
 * \param aProp                 Property string.
 */

nsresult
sbIPDDevice::GetTrackProp(sbIMediaItem* aMediaItem,
                          const char*   aPropName,
                          gchar**       aProp)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aPropName, "aPropName is null");
  NS_ASSERTION(aProp, "aProp is null");

  // Function variables.
  nsresult rv;

  // Get the property.
  nsAutoString propName;
  nsAutoString nsProp;
  propName.AssignLiteral(aPropName);
  rv = aMediaItem->GetProperty(propName, nsProp);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return property.
  gchar* prop = NULL;
  if (!nsProp.IsEmpty()) {
    prop = g_strdup(NS_ConvertUTF16toUTF8(nsProp).get());
    NS_ENSURE_TRUE(prop, NS_ERROR_OUT_OF_MEMORY);
  }
  if (*aProp)
    g_free(*aProp);
  *aProp = prop;

  return NS_OK;
}


/**
 * Set the property with the name specified by aPropName for the media item
 * specified by aMediaItem with the string value specified by aProp.
 *
 * \param aMediaItem            Media item for which to set property.
 * \param aPropName             Property name.
 * \param aProp                 Property string value.
 */

nsresult
sbIPDDevice::SetTrackProp(sbIMediaItem* aMediaItem,
                          const char*   aPropName,
                          gchar*        aProp)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aPropName, "aPropName is null");

  // Function variables.
  nsresult rv;

  // Set the property.
  nsAutoString propName;
  nsAutoString prop;
  propName.AssignLiteral(aPropName);
  if (aProp)
    prop.Assign(NS_ConvertUTF8toUTF16(aProp));
  else
    prop.SetIsVoid(PR_TRUE);
  rv = aMediaItem->SetProperty(propName, prop);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Set the property with the name specified by aPropName within the property
 * array specified by aPropertyArray with the string value specified by aProp.
 *
 * \param aPropertyArray        Property array for which to set property.
 * \param aPropName             Property name.
 * \param aProp                 Property string value.
 */

nsresult
sbIPDDevice::SetTrackProp(sbIMutablePropertyArray* aPropertyArray,
                          const char*              aPropName,
                          const nsAString&         aProp)
{
  // Validate arguments.
  NS_ASSERTION(aPropertyArray, "aPropertyArray is null");
  NS_ASSERTION(aPropName, "aPropName is null");

  // Function variables.
  nsresult rv;

  // Get the property info.
  nsAutoString              propName;
  nsCOMPtr<sbIPropertyInfo> propertyInfo;
  propName.AssignLiteral(aPropName);
  rv = mPropertyManager->GetPropertyInfo(propName,
                                         getter_AddRefs(propertyInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  // Validate the property value.
  PRBool valid;
  rv = propertyInfo->Validate(aProp, &valid);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(valid, NS_ERROR_INVALID_ARG);

  // Set the property.
  rv = aPropertyArray->AppendProperty(propName, aProp);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Set the property with the name specified by aPropName within the property
 * array specified by aPropertyArray with the string value specified by aProp.
 *
 * \param aPropertyArray        Property array for which to set property.
 * \param aPropName             Property name.
 * \param aProp                 Property string value.
 */

nsresult
sbIPDDevice::SetTrackProp(sbIMutablePropertyArray* aPropertyArray,
                          const char*              aPropName,
                          gchar*                   aProp)
{
  // Validate arguments.
  NS_ASSERTION(aPropertyArray, "aPropertyArray is null");
  NS_ASSERTION(aPropName, "aPropName is null");

  // Function variables.
  nsresult rv;

  // Do nothing if property is null.
  if (!aProp)
    return NS_OK;

  // Get the property info.
  nsAutoString              propName;
  nsCOMPtr<sbIPropertyInfo> propertyInfo;
  propName.AssignLiteral(aPropName);
  rv = mPropertyManager->GetPropertyInfo(propName,
                                         getter_AddRefs(propertyInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  // Convert and validate the property value.
  nsAutoString prop;
  if (aProp) {
    PRBool valid;
    prop.Assign(NS_ConvertUTF8toUTF16(aProp));
    rv = propertyInfo->Validate(prop, &valid);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(valid, NS_ERROR_INVALID_ARG);
  } else {
    prop.SetIsVoid(PR_TRUE);
  }

  // Set the property.
  rv = aPropertyArray->AppendProperty(propName, prop);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Get the property with the name specified by aPropName from the media item
 * specified by aMediaItem and return the integer value in aProp.  If the
 * property value has not been set, leave the contents of aProp unmodified.
 *
 * \param aMediaItem            Media item from which to get property.
 * \param aPropName             Property name.
 * \param aProp                 Property integer value.
 */

nsresult
sbIPDDevice::GetTrackProp(sbIMediaItem* aMediaItem,
                          const char*   aPropName,
                          gint*         aProp)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aPropName, "aPropName is null");
  NS_ASSERTION(aProp, "aProp is null");

  // Function variables.
  nsresult rv;

  // Get the property as a string.
  nsAutoString propName;
  nsAutoString propStr;
  propName.AssignLiteral(aPropName);
  rv = aMediaItem->GetProperty(propName, propStr);
  NS_ENSURE_SUCCESS(rv, rv);

  // If present, return the property as an integer.
  if (!propStr.IsEmpty()) {
    gint prop;
    int  numScanned;
    numScanned = PR_sscanf(NS_ConvertUTF16toUTF8(propStr).get(), "%d", &prop);
    if (numScanned >= 1)
      *aProp = prop;
  }

  return NS_OK;
}


/**
 * Set the property with the name specified by aPropName for the media item
 * specified by aMediaItem with the integer value specified by aProp.
 *
 * \param aMediaItem            Media item for which to set property.
 * \param aPropName             Property name.
 * \param aProp                 Property integer value.
 */

nsresult
sbIPDDevice::SetTrackProp(sbIMediaItem* aMediaItem,
                          const char*   aPropName,
                          gint          aProp)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aPropName, "aPropName is null");

  // Function variables.
  nsresult rv;

  // Set the property.
  nsAutoString propName;
  propName.AssignLiteral(aPropName);
  rv = aMediaItem->SetProperty(propName, sbAutoString(aProp));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Set the property with the name specified by aPropName within the property
 * array specified by aPropertyArray with the integer value specified by aProp.
 *
 * \param aPropertyArray        Property array for which to set property.
 * \param aPropName             Property name.
 * \param aProp                 Property integer value.
 */

nsresult
sbIPDDevice::SetTrackProp(sbIMutablePropertyArray* aPropertyArray,
                          const char*              aPropName,
                          gint                     aProp)
{
  // Validate arguments.
  NS_ASSERTION(aPropertyArray, "aPropertyArray is null");
  NS_ASSERTION(aPropName, "aPropName is null");

  // Function variables.
  nsresult rv;

  // Get the property info.
  nsAutoString              propName;
  nsCOMPtr<sbIPropertyInfo> propertyInfo;
  propName.AssignLiteral(aPropName);
  rv = mPropertyManager->GetPropertyInfo(propName,
                                         getter_AddRefs(propertyInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  // Convert and validate the property value.  Do not set the property if not
  // valid.
  sbAutoString prop(aProp);
  PRBool       valid;
  rv = propertyInfo->Validate(prop, &valid);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!valid)
    return NS_OK;

  // Set the property.
  rv = aPropertyArray->AppendProperty(propName, prop);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Get the property with the name specified by aPropName from the media item
 * specified by aMediaItem and return the unsigned 32-bit integer value in
 * aProp.  If the property value has not been set, leave the contents of aProp
 * unmodified.
 *
 * \param aMediaItem            Media item from which to get property.
 * \param aPropName             Property name.
 * \param aProp                 Property 32-bit integer value.
 */

nsresult
sbIPDDevice::GetTrackProp(sbIMediaItem* aMediaItem,
                          const char*   aPropName,
                          guint32*      aProp)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aPropName, "aPropName is null");
  NS_ASSERTION(aProp, "aProp is null");

  // Function variables.
  nsresult rv;

  // Get the property as a string.
  nsAutoString propName;
  nsAutoString propStr;
  propName.AssignLiteral(aPropName);
  rv = aMediaItem->GetProperty(propName, propStr);
  NS_ENSURE_SUCCESS(rv, rv);

  // If present, return the property as an integer.
  if (!propStr.IsEmpty()) {
    guint32 prop;
    int     numScanned;
    numScanned = PR_sscanf(NS_ConvertUTF16toUTF8(propStr).get(), "%u", &prop);
    if (numScanned >= 1)
      *aProp = prop;
  }

  return NS_OK;
}


/**
 * Set the property with the name specified by aPropName for the media item
 * specified by aMediaItem with the 32-bit unsigned integer value specified by
 * aProp.
 *
 * \param aMediaItem            Media item for which to set property.
 * \param aPropName             Property name.
 * \param aProp                 Property 32-bit unsigned integer value.
 */

nsresult
sbIPDDevice::SetTrackProp(sbIMediaItem* aMediaItem,
                          const char*   aPropName,
                          guint32       aProp)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aPropName, "aPropName is null");

  // Function variables.
  nsresult rv;

  // Set the property.
  nsAutoString propName;
  propName.AssignLiteral(aPropName);
  rv = aMediaItem->SetProperty(propName, sbAutoString(aProp));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Set the property with the name specified by aPropName within the property
 * array specified by aPropertyArray with the 32-bit unsigned integer value
 * specified by aProp.
 *
 * \param aPropertyArray        Property array for which to set property.
 * \param aPropName             Property name.
 * \param aProp                 Property 32-bit unsigned integer value.
 */

nsresult
sbIPDDevice::SetTrackProp(sbIMutablePropertyArray* aPropertyArray,
                          const char*              aPropName,
                          guint32                  aProp)
{
  // Validate arguments.
  NS_ASSERTION(aPropertyArray, "aPropertyArray is null");
  NS_ASSERTION(aPropName, "aPropName is null");

  // Function variables.
  nsresult rv;

  // Get the property info.
  nsAutoString              propName;
  nsCOMPtr<sbIPropertyInfo> propertyInfo;
  propName.AssignLiteral(aPropName);
  rv = mPropertyManager->GetPropertyInfo(propName,
                                         getter_AddRefs(propertyInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  // Convert and validate the property value.
  sbAutoString prop(aProp);
  PRBool       valid;
  rv = propertyInfo->Validate(prop, &valid);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(valid, NS_ERROR_INVALID_ARG);

  // Set the property.
  rv = aPropertyArray->AppendProperty(propName, prop);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Get the property with the name specified by aPropName from the media item
 * specified by aMediaItem and return the unsigned 64-bit integer value in
 * aProp.  If the property value has not been set, leave the contents of aProp
 * unmodified.
 *
 * \param aMediaItem            Media item from which to get property.
 * \param aPropName             Property name.
 * \param aProp                 Property 64-bit integer value.
 */

nsresult
sbIPDDevice::GetTrackProp(sbIMediaItem* aMediaItem,
                          const char*   aPropName,
                          guint64*      aProp)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aPropName, "aPropName is null");
  NS_ASSERTION(aProp, "aProp is null");

  // Function variables.
  nsresult rv;

  // Get the property as a string.
  nsAutoString propName;
  nsAutoString propStr;
  propName.AssignLiteral(aPropName);
  rv = aMediaItem->GetProperty(propName, propStr);
  NS_ENSURE_SUCCESS(rv, rv);

  // If present, return the property as an integer.
  if (!propStr.IsEmpty()) {
    guint64 prop;
    int     numScanned;
    numScanned = PR_sscanf(NS_ConvertUTF16toUTF8(propStr).get(), "%llu", &prop);
    if (numScanned >= 1)
      *aProp = prop;
  }

  return NS_OK;
}


/**
 * Set the property with the name specified by aPropName for the media item
 * specified by aMediaItem with the 64-bit unsigned integer value specified by
 * aProp.
 *
 * \param aMediaItem            Media item for which to set property.
 * \param aPropName             Property name.
 * \param aProp                 Property 64-bit unsigned integer value.
 */

nsresult
sbIPDDevice::SetTrackProp(sbIMediaItem* aMediaItem,
                          const char*   aPropName,
                          guint64       aProp)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aPropName, "aPropName is null");

  // Function variables.
  nsresult rv;

  // Set the property.
  nsAutoString propName;
  propName.AssignLiteral(aPropName);
  rv = aMediaItem->SetProperty(propName, sbAutoString(aProp));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Set the property with the name specified by aPropName within the property
 * array specified by aPropertyArray with the 64-bit unsigned integer value
 * specified by aProp.
 *
 * \param aPropertyArray        Property array for which to set property.
 * \param aPropName             Property name.
 * \param aProp                 Property 64-bit unsigned integer value.
 */

nsresult
sbIPDDevice::SetTrackProp(sbIMutablePropertyArray* aPropertyArray,
                          const char*              aPropName,
                          guint64                  aProp)
{
  // Validate arguments.
  NS_ASSERTION(aPropertyArray, "aPropertyArray is null");
  NS_ASSERTION(aPropName, "aPropName is null");

  // Function variables.
  nsresult rv;

  // Get the property info.
  nsAutoString              propName;
  nsCOMPtr<sbIPropertyInfo> propertyInfo;
  propName.AssignLiteral(aPropName);
  rv = mPropertyManager->GetPropertyInfo(propName,
                                         getter_AddRefs(propertyInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  // Convert and validate the property value.
  sbAutoString prop(aProp);
  PRBool       valid;
  rv = propertyInfo->Validate(prop, &valid);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(valid, NS_ERROR_INVALID_ARG);

  // Set the property.
  rv = aPropertyArray->AppendProperty(propName, prop);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Get the property with the name specified by aPropName from the media item
 * specified by aMediaItem and return the duration value in milliseconds in
 * aProp.  If the property value has not been set, leave the contents of aProp
 * unmodified.
 *
 * \param aMediaItem            Media item from which to get property.
 * \param aPropName             Property name.
 * \param aProp                 Property duration value.
 */

nsresult
sbIPDDevice::GetTrackPropDur(sbIMediaItem* aMediaItem,
                             const char*   aPropName,
                             gint*         aProp)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aPropName, "aPropName is null");
  NS_ASSERTION(aProp, "aProp is null");

  // Function variables.
  nsresult rv;

  // Get the property as a string.
  nsAutoString propName;
  nsAutoString propStr;
  propName.AssignLiteral(aPropName);
  rv = aMediaItem->GetProperty(propName, propStr);
  NS_ENSURE_SUCCESS(rv, rv);

  // If present, return the property as an integer.
  if (!propStr.IsEmpty()) {
    PRInt64 prop;
    int     numScanned;
    numScanned = PR_sscanf(NS_ConvertUTF16toUTF8(propStr).get(), "%lld", &prop);
    if (numScanned >= 1) {
      gint64 dur = prop / 1000;
      NS_ENSURE_TRUE(prop < (1 << 30), NS_ERROR_FAILURE);
      *aProp = (gint) (dur & 0xFFFFFFFF);
    }
  }

  return NS_OK;
}


/**
 * Set the property with the name specified by aPropName for the media item
 * specified by aMediaItem with the duration value specified by
 * aProp.
 *
 * \param aMediaItem            Media item for which to set property.
 * \param aPropName             Property name.
 * \param aProp                 Property duration value.
 */

nsresult
sbIPDDevice::SetTrackPropDur(sbIMediaItem* aMediaItem,
                             const char*         aPropName,
                             gint          aProp)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aPropName, "aPropName is null");

  // Function variables.
  nsresult rv;

  // Convert the iPod duration value to a Songbird duration value string.
  PRUint64 duration = ((PRUint64) aProp) * 1000;
  char     durationStr[32];
  PRUint32 printCount;
  printCount = PR_snprintf(durationStr, sizeof(durationStr), "%lld", duration);
  NS_ENSURE_TRUE(printCount > 0, NS_ERROR_UNEXPECTED);

  // Set the property.
  nsAutoString propName;
  nsAutoString prop;
  propName.AssignLiteral(aPropName);
  prop.AssignLiteral(durationStr);
  rv = aMediaItem->SetProperty(propName, prop);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Set the property with the name specified by aPropName within the property
 * array specified by aPropertyArray with the duration value specified by aProp.
 *
 * \param aPropertyArray        Property array for which to set property.
 * \param aPropName             Property name.
 * \param aProp                 Property duration value.
 */

nsresult
sbIPDDevice::SetTrackPropDur(sbIMutablePropertyArray* aPropertyArray,
                             const char*              aPropName,
                             gint                     aProp)
{
  // Validate arguments.
  NS_ASSERTION(aPropertyArray, "aPropertyArray is null");
  NS_ASSERTION(aPropName, "aPropName is null");

  // Function variables.
  nsresult rv;

  // Get the property info.
  nsAutoString              propName;
  nsCOMPtr<sbIPropertyInfo> propertyInfo;
  propName.AssignLiteral(aPropName);
  rv = mPropertyManager->GetPropertyInfo(propName,
                                         getter_AddRefs(propertyInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  /* Convert the iPod duration value to a Songbird duration value string. */
  PRUint64 duration = ((PRUint64) aProp) * 1000;
  char     durationStr[32];
  PRUint32 printCount;
  printCount = PR_snprintf(durationStr, sizeof(durationStr), "%lld", duration);
  NS_ENSURE_TRUE(printCount > 0, NS_ERROR_UNEXPECTED);

  // Convert and validate the property value.
  nsAutoString prop;
  PRBool       valid;
  prop.AssignLiteral(durationStr);
  rv = propertyInfo->Validate(prop, &valid);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(valid, NS_ERROR_INVALID_ARG);

  // Set the property.
  rv = aPropertyArray->AppendProperty(propName, prop);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Get the rating property from 0-5 with the name specified by aPropName from
 * the media item specified by aMediaItem and return the rating value from 0-100
 * in aProp.  If the property value has not been set, leave the contents of
 * aProp unmodified.
 *
 * \param aMediaItem            Media item from which to get property.
 * \param aPropName             Property name.
 * \param aProp                 Property rating value from 0-100.
 */

nsresult
sbIPDDevice::GetTrackPropRating(sbIMediaItem* aMediaItem,
                                const char*   aPropName,
                                guint32*      aProp)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aPropName, "aPropName is null");
  NS_ASSERTION(aProp, "aProp is null");

  // Function variables.
  nsresult rv;

  // Get the property as a string.
  nsAutoString propName;
  nsAutoString propStr;
  propName.AssignLiteral(aPropName);
  rv = aMediaItem->GetProperty(propName, propStr);
  NS_ENSURE_SUCCESS(rv, rv);

  // If present, return the property as an integer scaled from 0-100.
  if (!propStr.IsEmpty()) {
    PRUint32 prop;
    int      numScanned;
    numScanned = PR_sscanf(NS_ConvertUTF16toUTF8(propStr).get(), "%lu", &prop);
    if (numScanned >= 1) {
      guint32 rating = prop * 20;
      *aProp = rating;
    }
  }

  return NS_OK;
}


/**
 * Set the rating property from 0-5 with the name specified by aPropName for the
 * media item specified by aMediaItem with the rating value specified by
 * aProp from 0-100.
 *
 * \param aMediaItem            Media item for which to set property.
 * \param aPropName             Property name.
 * \param aProp                 Property rating value from 0-100.
 */

nsresult
sbIPDDevice::SetTrackPropRating(sbIMediaItem* aMediaItem,
                                const char*   aPropName,
                                guint32       aProp)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aPropName, "aPropName is null");

  // Function variables.
  nsresult rv;

  // Convert the iPod rating value to a Songbird rating value string.
  PRUint32 rating = (aProp + 10) / 20;
  char     ratingStr[32];
  PRUint32 printCount;
  printCount = PR_snprintf(ratingStr, sizeof(ratingStr), "%lu", rating);
  NS_ENSURE_TRUE(printCount > 0, NS_ERROR_UNEXPECTED);

  // Set the property.
  nsAutoString propName;
  nsAutoString prop;
  propName.AssignLiteral(aPropName);
  prop.AssignLiteral(ratingStr);
  rv = aMediaItem->SetProperty(propName, prop);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Set the rating property from 0-5 with the name specified by aPropName within
 * the property array specified by aPropertyArray with the rating value from
 * 0-100 specified by aProp.
 *
 * \param aPropertyArray        Property array for which to set property.
 * \param aPropName             Property name.
 * \param aProp                 Property rating value from 0-100.
 */

nsresult
sbIPDDevice::SetTrackPropRating(sbIMutablePropertyArray* aPropertyArray,
                                const char*              aPropName,
                                guint32                  aProp)
{
  // Validate arguments.
  NS_ASSERTION(aPropertyArray, "aPropertyArray is null");
  NS_ASSERTION(aPropName, "aPropName is null");

  // Function variables.
  nsresult rv;

  // Get the property info.
  nsAutoString              propName;
  nsCOMPtr<sbIPropertyInfo> propertyInfo;
  propName.AssignLiteral(aPropName);
  rv = mPropertyManager->GetPropertyInfo(propName,
                                         getter_AddRefs(propertyInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  // Convert the iPod rating value to a Songbird rating value string.
  PRUint32 rating = (aProp + 10) / 20;
  char     ratingStr[32];
  PRUint32 printCount;
  printCount = PR_snprintf(ratingStr, sizeof(ratingStr), "%lu", rating);
  NS_ENSURE_TRUE(printCount > 0, NS_ERROR_UNEXPECTED);

  // Convert and validate the property value.
  nsAutoString prop;
  PRBool       valid;
  prop.AssignLiteral(ratingStr);
  rv = propertyInfo->Validate(prop, &valid);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(valid, NS_ERROR_INVALID_ARG);

  // Set the property.
  rv = aPropertyArray->AppendProperty(propName, prop);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Get the file type property from the media item specified by aMediaItem and
 * return the string value in aProp.  The returned property string must be
 * deallocated with g_free.
 *
 * \param aMediaItem            Media item from which to get property.
 * \param aProp                 File type string property.
 */

nsresult
sbIPDDevice::GetTrackPropFileType(sbIMediaItem* aMediaItem,
                                  gchar**       aProp)
{
  // Validate parameters.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");
  NS_ASSERTION(aProp, "aProp is null");

  // Function variables.
  nsresult rv;

  // Get the track media URL.
  nsCOMPtr<nsIURI> trackURI;
  rv = aMediaItem->GetContentSrc(getter_AddRefs(trackURI));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIURL> trackURL = do_QueryInterface(trackURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the track media file extension.
  nsCAutoString fileExtension;
  rv = trackURL->GetFileExtension(fileExtension);
  NS_ENSURE_SUCCESS(rv, rv);

  // If present, return property.
  if (!fileExtension.IsEmpty()) {
    gchar* prop = g_strdup(fileExtension.get());
    NS_ENSURE_TRUE(prop, NS_ERROR_OUT_OF_MEMORY);
    *aProp = prop;
  }

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// iPod device track info services.
//
//------------------------------------------------------------------------------


/**
 * Return in aTrackURI the URI for the media file in the iPod track specified by
 * aTrack.
 *
 * \param aTrack                Track for which to get URI.
 * \param aTrackURI             Track media file URI.
 */

nsresult
sbIPDDevice::GetTrackURI(Itdb_Track* aTrack,
                         nsIURI**    aTrackURI)
{
  // Validate parameters.
  NS_ASSERTION(aTrack, "aTrack is null");
  NS_ASSERTION(aTrackURI, "aTrackURI is null");

  // Function variables.
  nsresult rv;

  // Get a track file object.
  nsCOMPtr<nsIFile> trackFile;
  rv = TrackGetFile(aTrack, getter_AddRefs(trackFile));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get a track URI object.
  nsCOMPtr<nsIURI> trackURI;
  rv = mFileProtocolHandler->NewFileURI(trackFile,
                                        getter_AddRefs(trackURI));
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  NS_ADDREF(*aTrackURI = trackURI);

  return NS_OK;
}


/**
 * Return in aTrackFile a file object for the media file in the iPod track
 * specified by aTrack.
 *
 * \param aTrack                Track for which to get media file object.
 * \param aTrackFile            Track media file object.
 */

nsresult
sbIPDDevice::TrackGetFile(Itdb_Track* aTrack,
                          nsIFile**   aTrackFile)
{
  // Validate parameters.
  NS_ASSERTION(aTrack, "aTrack is null");
  NS_ASSERTION(aTrackFile, "aTrackFile is null");

  // Function variables.
  nsresult rv;

  // Get the track file path and set it up for auto-disposal.
  gchar* trackFilePath = itdb_filename_on_ipod(aTrack);
  NS_ENSURE_TRUE(trackFilePath, NS_ERROR_FAILURE);
  sbAutoGMemPtr autoTrackFilePath(trackFilePath);
  nsCAutoString nsTrackFilePath(trackFilePath);

  // Get a track file object.
  nsCOMPtr<nsILocalFile> trackLocalFile =
                           do_CreateInstance("@mozilla.org/file/local;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = trackLocalFile->InitWithNativePath(nsTrackFilePath);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  NS_ADDREF(*aTrackFile = trackLocalFile);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Internal iPod device track services.
//
//------------------------------------------------------------------------------

/**
 * Import the batch of iPod tracks specified by aTrackBatch and aBatchCount into
 * the Songbird device library.
 *
 * This function implements the batch processing using a batch callback.  This
 * function is divided into multiple parts, the main function and the callback
 * functions, as follows:
 *
 *   ImportTrackBatch           Main function.
 *   ImportTrackBatch1          Static batch callback function.
 *   ImportTrackBatch2          Object batch callback function.
 *
 * \param aTrackBatch           Batch of tracks to import.
 * \param aBatchCount           Number of tracks in batch.
 */

class ImportTrackBatchParams : public nsISupports
{
public:
  NS_DECL_ISUPPORTS
  nsRefPtr<sbIPDDevice>       ipdDevice;
  Itdb_Track**                trackBatch;
  PRUint32                    batchCount;
};
NS_IMPL_ISUPPORTS0(ImportTrackBatchParams)

nsresult
sbIPDDevice::ImportTrackBatch(Itdb_Track** aTrackBatch,
                              PRUint32     aBatchCount)
{
  // Validate parameters.
  NS_ASSERTION(aTrackBatch, "aTrackBatch is null");

  // Function variables.
  nsresult rv;

  // Set up the batch callback function.
  nsCOMPtr<sbIMediaListBatchCallback>
    batchCallback = new sbMediaListBatchCallback(ImportTrackBatch1);
  NS_ENSURE_TRUE(batchCallback, NS_ERROR_OUT_OF_MEMORY);

  // Set up the batch callback function parameters.
  nsRefPtr<ImportTrackBatchParams> importTrackBatchParams =
                                     new ImportTrackBatchParams();
  NS_ENSURE_TRUE(importTrackBatchParams, NS_ERROR_OUT_OF_MEMORY);
  importTrackBatchParams->ipdDevice = this;
  importTrackBatchParams->trackBatch = aTrackBatch;
  importTrackBatchParams->batchCount = aBatchCount;

  // Import tracks in batch mode.
  rv = mDeviceLibraryML->RunInBatchMode(batchCallback,
                                        importTrackBatchParams);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* static */
nsresult
sbIPDDevice::ImportTrackBatch1(nsISupports* aUserData)
{
  // Validate parameters.
  NS_ENSURE_ARG_POINTER(aUserData);

  // Function variables.
  nsresult rv;

  // Get the import track batch parameters.
  ImportTrackBatchParams* importTrackBatchParams =
                            static_cast<ImportTrackBatchParams *>(aUserData);
  nsRefPtr<sbIPDDevice> ipdDevice = importTrackBatchParams->ipdDevice;
  Itdb_Track** trackBatch = importTrackBatchParams->trackBatch;
  int batchCount = importTrackBatchParams->batchCount;

  // Import a batch of tracks.
  rv = ipdDevice->ImportTrackBatch2(trackBatch, batchCount);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbIPDDevice::ImportTrackBatch2(Itdb_Track** aTrackBatch,
                               PRUint32     aBatchCount)
{
  // Validate parameters.
  NS_ASSERTION(aTrackBatch, "aTrackBatch is null");

  // Function variables.
  nsresult rv;

  // Create the track info arrays.
  nsTArray<PRUint32>        trackArrayIndexMap(aBatchCount);
  nsCOMPtr<nsIMutableArray> trackURIArray =
                              do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIMutableArray> propsArray =
                              do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Ignore library changes and set up to automatically stop ignoring.
  mLibraryListener->SetIgnoreListener(PR_TRUE);
  sbIPDAutoStopIgnoreLibrary autoStopIgnoreLibrary(mLibraryListener);

  // Get the track URI's and properties.
  PRUint32 trackCount = 0;
  for (PRUint32 i = 0; i < aBatchCount; i++) {
    // Get the track URI.  Skip track on failure.  This can happen if the track
    // file doesn't exist.
    Itdb_Track* track = aTrackBatch[i];
    FIELD_LOG(("1: ImportTrack %s\n", itdb_filename_on_ipod(track)));
    nsCOMPtr<nsIURI> trackURI;
    rv = GetTrackURI(track, getter_AddRefs(trackURI));
    if (NS_FAILED(rv)) {
      FIELD_LOG(("2: ImportTrack failed to get track URI.\n"));
      continue;
    }

    // If track has previously been imported, just update it.
    nsCOMPtr<sbIMediaItem> mediaItem;
    rv = sbDeviceUtils::GetMediaItemByDevicePersistentId
                          (mDeviceLibrary,
                           sbAutoString(track->dbid),
                           getter_AddRefs(mediaItem));
    if (NS_SUCCEEDED(rv)) {
      SetTrackProperties(mediaItem, track);
      rv = mediaItem->SetContentSrc(trackURI);
      NS_ENSURE_SUCCESS(rv, rv);
      continue;
    } else {
      NS_ENSURE_TRUE(rv == NS_ERROR_NOT_AVAILABLE, rv);
    }

    // Map the track array indices.
    trackArrayIndexMap[trackCount] = i;

    // Add the track properties.
    rv = GetTrackProperties(track, propsArray);
    NS_ENSURE_SUCCESS(rv, rv);

    // Add the track URI.
    rv = trackURIArray->AppendElement(trackURI, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    // One more track.
    trackCount++;
  }

  // Create the track media items.
  nsCOMPtr<nsIArray> trackMediaItemArray;
  rv = mDeviceLibrary->BatchCreateMediaItems
                         (trackURIArray,
                          propsArray,
                          PR_TRUE,
                          getter_AddRefs(trackMediaItemArray));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Remove the iPod track specified by aTrack from all iPod playlists.
 *
 * \param aTrack                Track to remove.
 */

void
sbIPDDevice::RemoveTrackFromAllPlaylists(Itdb_Track* aTrack)
{
  // Remove the track from each playlist.
  GList* playlistList = mITDB->playlists;
  while (playlistList) {
    // Get the next playlist.
    Itdb_Playlist* playlist = (Itdb_Playlist *) playlistList->data;
    playlistList = playlistList->next;

    // Remove the track from the playlist.
    itdb_playlist_remove_track(playlist, aTrack);
  }
}


/**
 * Check if the media format of the media item specified by aMediaItem is
 * supported by the device.  Return true if it is; otherwise, return false.
 *
 * \param aMediaItem            Media item to check.
 *
 * \return True if media format is supported.
 */

PRBool
sbIPDDevice::IsMediaSupported(sbIMediaItem* aMediaItem)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");

  // Function variables.
  nsresult rv;

  // Get the media file extension.
  nsCOMPtr<nsIURI> itemURI;
  nsCOMPtr<nsIURL> itemURL;
  nsCAutoString    fileExtension;
  rv = aMediaItem->GetContentSrc(getter_AddRefs(itemURI));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  itemURL = do_QueryInterface(itemURI, &rv);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  rv = itemURL->GetFileExtension(fileExtension);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  // Check the media file extension against all supported extensions.
  PRUint32 i;
  for (i = 0; i < sbIPDSupportedMediaListLength; i++) {
    if (fileExtension.Equals(sbIPDSupportedMediaList[i],
                             CaseInsensitiveCompare)) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}


/**
 * Add the media item specified by aMediaItem to the list of media items that
 * were not transferred to the device because they were in an unsupported
 * format.
 *
 * \param aMediaItem            Unsupported media item.
 */

void
sbIPDDevice::AddUnsupportedMediaItem(sbIMediaItem* aMediaItem)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");

  // Dispatch an unsupported media type event.
  CreateAndDispatchEvent
    (sbIDeviceEvent::EVENT_DEVICE_MEDIA_WRITE_UNSUPPORTED_TYPE,
     sbIPDVariant(NS_GET_IID(sbIMediaItem), aMediaItem).get());
}


