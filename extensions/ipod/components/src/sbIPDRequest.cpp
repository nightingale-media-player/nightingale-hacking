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
// iPod device request services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbIPDRequest.cpp
 * \brief Songbird iPod Device Request Source.
 */

//------------------------------------------------------------------------------
//
// iPod device request imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIPDDevice.h"
#include "sbIPDLog.h"

// Songbird imports.
#include <sbAutoRWLock.h>
#include <sbIDeviceEvent.h>
#include <sbStandardProperties.h>

// Mozilla imports.
#include <nsArrayUtils.h>
#include <nsThreadUtils.h>
#include <nsILocalFile.h>


//------------------------------------------------------------------------------
//
// iPod device request sbBaseDevice services.
//
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//
// iPod device request handler services.
//
//   All functions in the request handler services, except
// ReqHandleRequestAdded, must be called under the connect and request locks.
//
//------------------------------------------------------------------------------

/**
 * Handle a request added event.
 */

nsresult
sbIPDDevice::ProcessBatch(Batch & aBatch)
{
  nsresult rv;

  // Operate under the connect lock.
  sbAutoReadLock autoConnectLock(mConnectLock);
  NS_ENSURE_TRUE(mConnected, NS_ERROR_NOT_AVAILABLE);

  // Set up to automatically set the state to STATE_IDLE on exit.  Any device
  // operation must change the state to not be idle.  This ensures that the
  // device is not prematurely removed (e.g., ejected) while the device content
  // is being accessed.  This is particularly important since most changes
  // result in the iPod database or prefs files being rewritten.
  sbIPDAutoIdle autoIdle(this);

  // Set up to auto-flush the iPod database files before returning to the idle
  // state.
  {
    sbIPDAutoDBFlush autoDBFlush(this);


    if (aBatch.empty()) {
      return NS_OK;
    }

    PRUint32 batchType = aBatch.RequestType();

    if (batchType == TransferRequest::REQUEST_WRITE) {

      // Force update of storage statistics before checking for space.
      StatsUpdate(PR_TRUE);

      PRUint32 deviceState;
      rv = GetState(&deviceState);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    Batch batches[3];
    SBWriteRequestSplitBatches(aBatch,
                               batches[0],
                               batches[1],
                               batches[2]);

    for (PRUint32 batchIndex = 0;
         batchIndex < NS_ARRAY_LENGTH(batches);
         ++batchIndex) {
      Batch & batch = batches[batchIndex];
      const Batch::const_iterator end = batch.end();
      for (Batch::const_iterator iter = batch.begin();
           iter != end;
           ++iter) {
        // Check for abort.
        if (IsRequestAborted()) {
          return NS_ERROR_ABORT;
        }

        TransferRequest * request = static_cast<TransferRequest *>(*iter);


        PRUint32 type = request->GetType();

        FIELD_LOG(("Processing request 0x%08x\n", type));
        switch(type)
        {
          case TransferRequest::REQUEST_MOUNT :
            mIPDStatus->ChangeStatus(STATE_MOUNTING);
            ReqHandleMount(request);
          break;

          case TransferRequest::REQUEST_WRITE : {
            // Handle request if space has been ensured for it.
            mIPDStatus->ChangeStatus(STATE_COPYING);
            ReqHandleWrite(request, batch.CountableItems());
          }
          break;

          case TransferRequest::REQUEST_DELETE :
            mIPDStatus->ChangeStatus(STATE_DELETING);
            ReqHandleDelete(request, batch.CountableItems());
            break;

          case TransferRequest::REQUEST_WIPE :
            mIPDStatus->ChangeStatus(STATE_DELETING);
            ReqHandleWipe(request);
            break;

          case TransferRequest::REQUEST_NEW_PLAYLIST :
            mIPDStatus->ChangeStatus(STATE_BUSY);
            ReqHandleNewPlaylist(request);
            break;

          case TransferRequest::REQUEST_UPDATE :
            mIPDStatus->ChangeStatus(STATE_UPDATING);
            ReqHandleUpdate(request);
            break;

          case TransferRequest::REQUEST_MOVE :
            mIPDStatus->ChangeStatus(STATE_BUSY);
            ReqHandleMovePlaylistTrack(request);
            break;

          case TransferRequest::REQUEST_SYNC :
            mIPDStatus->ChangeStatus(STATE_SYNCING);
            HandleSyncRequest(request);
            break;

          case sbIDevice::REQUEST_FACTORY_RESET :
            mIPDStatus->ChangeStatus(STATE_BUSY);
            ReqHandleFactoryReset(request);
            break;

          case REQUEST_WRITE_PREFS :
            mIPDStatus->ChangeStatus(STATE_BUSY);
            ReqHandleWritePrefs(request);
            break;

          case REQUEST_SET_PROPERTY :
            mIPDStatus->ChangeStatus(STATE_BUSY);
            ReqHandleSetProperty(request);
            break;

          case REQUEST_SET_PREF :
            ReqHandleSetPref(static_cast<SetNamedValueRequest*>(request));
            break;

          default :
            NS_WARNING("Invalid request type.");
            break;
        }
        if (IsRequestAborted()) {
          // If we're aborting iterators will be invalid, need to exit loop
          // to get the next batch if any.
          break;
        }
        request->SetIsProcessed(true);
      }
    }
  }

  return NS_OK;
}


/**
 * Handle the mount request specified by aRequest.
 *
 * This function must be called under the connect and request locks.
 *
 * \param aRequest              Request data record.
 */

void
sbIPDDevice::ReqHandleMount(TransferRequest * aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  const PRUint32 batchIndex = aRequest->GetBatchIndex() + 1;

  // Update status and set for auto-failure.
  mIPDStatus->OperationStart(sbIPDStatus::OPERATION_TYPE_MOUNT,
                             aRequest,
                             batchIndex);

  sbAutoStatusOperationFailure autoStatus(mIPDStatus);

  // Force early updating of statistics to get some initial statistics for UI.
  StatsUpdate(PR_TRUE);

  // Ignore library changes and set up to automatically stop ignoring.
  {
    mLibraryListener->SetIgnoreListener(PR_TRUE);
    sbIPDAutoStopIgnoreLibrary autoStopIgnoreLibrary(mLibraryListener);

    // Process the on-the-go playlists.  Do this before importing the iPod
    // database because processing will change the on-the-go playlist names.
    rv = ProcessOTGPlaylists();
    NS_ENSURE_SUCCESS(rv, /* void */);

    // Import the iPod database.
    rv = ImportDatabase();
    NS_ENSURE_SUCCESS(rv, /* void */);

    // Show the device library.
    rv = mDeviceLibrary->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                                     NS_LITERAL_STRING("0"));
    NS_ENSURE_SUCCESS(rv, /* void */);
  }

  // Update status and clear auto-failure.
  mIPDStatus->OperationComplete(NS_OK);
  autoStatus.forget();

  // Indicate that the device is now ready.
  //XXXeps should have some variable that can be read by code that misses this
  //XXXeps event (e.g., some JS that registers its handlers after mounting
  //XXXeps completes).
  CreateAndDispatchEvent
    (sbIDeviceEvent::EVENT_DEVICE_READY,
     sbIPDVariant(NS_ISUPPORTS_CAST(sbIDevice*, this)).get());

  // Force updating of statistics.
  StatsUpdate(PR_TRUE);

  // Initiate auto-synchronization from the device.
  //XXXeps should do this via a request so that wait on quit works.
  SyncFromDevice();
}


/**
 * Handle the write item request specified by aRequest.
 *
 * This function must be called under the connect and request locks.
 *
 * \param aRequest              Request data record.
 */

void
sbIPDDevice::ReqHandleWrite(TransferRequest * aRequest, PRUint32 aBatchCount)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Write the item.
  if (aRequest->IsPlaylist())
    ReqHandleWritePlaylistTrack(aRequest);
  else
    ReqHandleWriteTrack(aRequest, aBatchCount);
}


/**
 * Handle the write track request specified by aRequest.
 *
 * This function must be called under the connect and request locks.
 *
 * \param aRequest              Request data record.
 */

void
sbIPDDevice::ReqHandleWriteTrack(TransferRequest * aRequest,
                                 PRUint32 aBatchCount)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  const PRUint32 batchIndex = aRequest->GetBatchIndex() + 1;

  // Update operation status and set for auto-completion.
  sbAutoStatusOperationComplete autoOperationStatus(mIPDStatus, NS_ERROR_FAILURE);
  if (batchIndex == 1) {
    mIPDStatus->OperationStart(sbIPDStatus::OPERATION_TYPE_WRITE, aRequest,
                               aBatchCount);
  }
  if (batchIndex == aBatchCount) {
    autoOperationStatus.Set(mIPDStatus, NS_OK);
  }

  sbAutoStatusItemFailure autoItemStatus(mIPDStatus);

  // Remove unsupported media items and report errors.
  PRBool supported;
  rv = sbBaseDevice::SupportsMediaItem(aRequest->item,
                                       nsnull,
                                       PR_TRUE,
                                       &supported);
  NS_ENSURE_SUCCESS(rv, /* void */);
  if (!supported) {
    rv = DeleteItem(aRequest->list, aRequest->item);
    NS_ENSURE_SUCCESS(rv, /* void */);
    return;
  }

  // Upload track.
  rv = UploadTrack(aRequest->item);
  NS_ENSURE_SUCCESS(rv, /* void */);

  // Update status and clear auto-failure.
  autoItemStatus.forget();
  mIPDStatus->ItemComplete(NS_OK);
}


/**
 * Handle the write playlist track request specified by aRequest.
 *
 * This function must be called under the connect and request locks.
 *
 * \param aRequest              Request data record.
 */

void
sbIPDDevice::ReqHandleWritePlaylistTrack(TransferRequest* aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  // Log progress.
  FIELD_LOG(("Add track to playlist at index %d.\n", aRequest->index));

  // Add the track to the playlist.
  rv = PlaylistAddTrack(aRequest->list, aRequest->item, aRequest->index);
  NS_ENSURE_SUCCESS(rv, /* void */);
}


/**
 * Handle the wipe request specified by aRequest.
 *
 * This function must be called under the connect and request locks.
 *
 * \param aRequest              Request data record.
 */

void
sbIPDDevice::ReqHandleWipe(TransferRequest * aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  // if this is a library we're wiping the device
  nsCOMPtr<sbILibrary> library = do_QueryInterface(aRequest->item, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = WipeDevice();
    NS_ENSURE_SUCCESS(rv, /* void */);
  }
  else {
    // Determine if the item to delete is a playlist.
    nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(aRequest->item, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = PlaylistWipe(mediaList);
      NS_ENSURE_SUCCESS(rv, /* void */);
    }
    else {
      NS_WARNING("Unexpected data provided to REQUEST_WIPE");
    }
  }
}

/**
 * Handle the delete item request specified by aRequest.
 *
 * This function must be called under the connect and request locks.
 *
 * \param aRequest              Request data record.
 */

void
sbIPDDevice::ReqHandleDelete(TransferRequest* aRequest,
                             PRUint32 aBatchCount)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  // Determine if the item to delete is a playlist.
  PRBool deletePlaylist;
  nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(aRequest->item, &rv);
  if (NS_SUCCEEDED(rv))
    deletePlaylist = PR_TRUE;
  else
    deletePlaylist = PR_FALSE;

  // Delete the item.
  if (deletePlaylist) {
    ReqHandleDeletePlaylist(aRequest);
  } else {
    if (aRequest->IsPlaylist())
      ReqHandleDeletePlaylistTrack(aRequest);
    else
      ReqHandleDeleteTrack(aRequest, aBatchCount);
  }
}


/**
 * Handle the delete track request specified by aRequest.
 *
 * This function must be called under the connect and request locks.
 *
 * \param aRequest              Request data record.
 */

void
sbIPDDevice::ReqHandleDeleteTrack(TransferRequest * aRequest,
                                  PRUint32 aBatchCount)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  const PRUint32 batchIndex = aRequest->GetBatchIndex() + 1;


  // Update operation status and set for auto-completion.
  sbAutoStatusOperationComplete autoOperationStatus(mIPDStatus, NS_ERROR_FAILURE);
  if (batchIndex == 1) {
    mIPDStatus->OperationStart(sbIPDStatus::OPERATION_TYPE_DELETE, aRequest,
                               aBatchCount);
  }
  if (batchIndex == aBatchCount) {
    autoOperationStatus.Set(mIPDStatus, NS_OK);
  }

  sbAutoStatusItemFailure autoItemStatus(mIPDStatus);

  // Delete track.
  rv = DeleteTrack(aRequest->item);
  NS_ENSURE_SUCCESS(rv, /* void */);

  // Update status and clear auto-failure.
  autoItemStatus.forget();
  mIPDStatus->ItemComplete(NS_OK);
}


/**
 * Handle the delete playlist track request specified by aRequest.
 *
 * This function must be called under the connect and request locks.
 *
 * \param aRequest              Request data record.
 */

void
sbIPDDevice::ReqHandleDeletePlaylistTrack(TransferRequest * aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  // Log progress.
  FIELD_LOG(("Delete track from playlist at index %d.\n", aRequest->index));

  // Delete the track from the playlist.
  rv = PlaylistRemoveTrack(aRequest->list, aRequest->item, aRequest->index);
  NS_ENSURE_SUCCESS(rv, /* void */);
}


/**
 * Handle the delete playlist request specified by aRequest.
 *
 * This function must be called under the connect and request locks.
 *
 * \param aRequest              Request data record.
 */

void
sbIPDDevice::ReqHandleDeletePlaylist(TransferRequest * aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  // Log progress.
  FIELD_LOG(("Deleting playlist.\n"));

  // Delete the playlist from the iPod.
  nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(aRequest->item, &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);
  rv = PlaylistDelete(mediaList);
  NS_ENSURE_SUCCESS(rv, /* void */);
}


/**
 * Handle the new playlist request specified by aRequest.
 *
 * This function must be called under the connect and request locks.
 *
 * \param aRequest              Request data record.
 */

void
sbIPDDevice::ReqHandleNewPlaylist(TransferRequest * aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  // Log progress.
  FIELD_LOG(("Creating playlist.\n"));

  // Add the playlist to the iPod.
  Itdb_Playlist*         playlist;
  nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(aRequest->item, &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);
  rv = PlaylistAdd(mediaList, &playlist);
  NS_ENSURE_SUCCESS(rv, /* void */);
}


/**
 * Handle the update request specified by aRequest.
 *
 * This function must be called under the connect and request locks.
 *
 * \param aRequest              Request data record.
 */

void
sbIPDDevice::ReqHandleUpdate(TransferRequest * aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  // Get the request parameters.
  nsCOMPtr<sbIMediaItem> mediaItem = aRequest->item;
  nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(mediaItem);

  // Do nothing if the item to update no longer exists.
  nsCOMPtr<sbILibrary> library;
  PRBool               exists;
  rv = mediaItem->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, /* void */);
  rv = library->Contains(mediaItem, &exists);
  NS_ENSURE_SUCCESS(rv, /* void */);
  if (!exists)
    return;

  // Log progress.
  FIELD_LOG(("Updating item.\n"));

  // Update the item.
  if (mediaList)
    rv = PlaylistUpdateProperties(mediaList);
  else
    rv = TrackUpdateProperties(mediaItem);
  NS_ENSURE_SUCCESS(rv, /* void */);
}


/**
 * Handle the move playlist track request specified by aRequest.
 *
 * This function must be called under the connect and request locks.
 *
 * \param aRequest              Request data record.
 */

void
sbIPDDevice::ReqHandleMovePlaylistTrack(TransferRequest * aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  // Log progress.
  FIELD_LOG(("Move playlist track from index %d to index %d.\n",
      aRequest->index,
      aRequest->otherIndex));

  // Validate request parameters.
  NS_ENSURE_TRUE(aRequest->list, /* void */);

  // Move the track in the playlist.
  rv = PlaylistMoveTrack(aRequest->list,
                         aRequest->index,
                         aRequest->otherIndex);
  NS_ENSURE_SUCCESS(rv, /* void */);
}


/**
 * Handle the factory reset request specified by aRequest.
 *
 * This function must be called under the connect and request locks.
 *
 * \param aRequest              Request data record.
 */

void
sbIPDDevice::ReqHandleFactoryReset(TransferRequest * aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  // Log progress.
  FIELD_LOG(("Factory reset.\n"));

  nsCOMPtr<nsILocalFile> mountDir =
    do_CreateInstance("@mozilla.org/file/local;1");
  mountDir->InitWithPath(mMountPath);

  nsCOMPtr<nsISimpleEnumerator> files;
  rv = mountDir->GetDirectoryEntries(getter_AddRefs(files));
  NS_ENSURE_SUCCESS(rv, /* void */);

  PRBool hasMore;
  while (NS_SUCCEEDED(files->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsIFile> file;
    rv = files->GetNext(getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, /* void */);

#ifdef DEBUG
    nsString path;
    file->GetPath(path);

    FIELD_LOG(("sbIPDDevice::ReqHandleFactoryReset deleting directory %s\n",
         NS_LossyConvertUTF16toASCII(path).get()));
#endif

    rv = file->Remove(PR_TRUE);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Unable to delete directory");
  }

  /* eject! */
  Eject();
}


/**
 * Handle the write preferences request specified by aRequest.
 *
 * This function must be called under the connect and request locks.
 *
 * \param aRequest              Request data record.
 */

void
sbIPDDevice::ReqHandleWritePrefs(TransferRequest * aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  GError* gError = NULL;
  PRBool  success;

  // Operate under the preference lock.
  //XXXeps this can sometimes cause a potential deadlock assert because the
  //XXXeps main thread can hold the pref lock, then the request thread can
  //XXXeps acquire the request lock, then try to acquire the pref lock.  This
  //XXXeps doesn't lead to an actual dead lock, just an assert.
  nsAutoLock autoPrefLock(mPrefLock);

  // Write the iPod preferences.
  if (mIPodPrefsDirty) {
    success = itdb_prefs_write(mITDB->device, mIPodPrefs, &gError);
    if (gError) {
      if (gError->message)
        FIELD_LOG(("%s", gError->message));
      g_error_free(gError);
      gError = NULL;
    }
    if (!success)
      NS_WARNING("Failed to write the iPod preferences file.");
  }
  mIPodPrefsDirty = PR_FALSE;

  // Write the sync playlist list.
  if (mSyncPlaylistListDirty) {
    success = itdb_update_playlists_write(mITDB->device,
                                          mSyncPlaylistList.Elements(),
                                          mSyncPlaylistList.Length(),
                                          &gError);
    if (gError) {
      if (gError->message)
        FIELD_LOG(("%s", gError->message));
      g_error_free(gError);
      gError = nsnull;
    }
    if (!success)
      NS_WARNING("Failed to write the iPod sync playlist file.");
  }
  mSyncPlaylistListDirty = PR_FALSE;
}


/**
 * Handle the set property request specified by aRequest.
 *
 * This function must be called under the connect and request locks.
 *
 * \param aRequest              Request data record.
 */

void
sbIPDDevice::ReqHandleSetProperty(TransferRequest * aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  SetNamedValueRequest* request = static_cast<SetNamedValueRequest*>(aRequest);

  // Process the request.
  if (request->name.EqualsLiteral("FriendlyName")) {
    // Get the friendly name.
    nsAutoString friendlyName;
    rv = request->value->GetAsAString(friendlyName);
    NS_ENSURE_SUCCESS(rv, /* void */);

    // Set the master playlist name to the friendly name.
    gchar* masterPlaylistName =
             g_strdup(NS_ConvertUTF16toUTF8(friendlyName).get());
    NS_ENSURE_TRUE(masterPlaylistName, /* void */);
    if (mMasterPlaylist->name)
      g_free(mMasterPlaylist->name);
    mMasterPlaylist->name = masterPlaylistName;

    // Mark the iPod database as dirty.
    mITDBDirty = PR_TRUE;
  }
}


/**
 * Handle the set preference request specified by aRequest.
 *
 * This function must be called under the connect and request locks.
 *
 * \param aRequest              Request data record.
 */

void
sbIPDDevice::ReqHandleSetPref(SetNamedValueRequest * aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  // Process the request.

  if (aRequest->name.EqualsLiteral("SyncPartner")) {
    // Get the Songbird sync partner ID.
    nsAutoString sbSyncPartnerID;
    rv = aRequest->value->GetAsAString(sbSyncPartnerID);
    NS_ENSURE_SUCCESS(rv, /* void */);

    // Get the iPod sync partner ID mapped to the Songbird sync partner ID.
    // Create one if none mapped.
    guint64 iPodSyncPartnerID;
    rv = IDMapGet(sbSyncPartnerID, &iPodSyncPartnerID);
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      iPodSyncPartnerID = (((guint64) g_random_int()) << 32) |
                          g_random_int();
      IDMapAdd(sbSyncPartnerID, iPodSyncPartnerID);
    } else {
      NS_ENSURE_SUCCESS(rv, /* void */);
    }

    // Set the iPod linked library ID preference under the preference lock.
    {
      nsAutoLock autoPrefLock(mPrefLock);
      mIPodPrefs->music_lib_link_id = iPodSyncPartnerID;
      mIPodPrefsDirty = PR_TRUE;
    }

    // Write the preferences.
    rv = PushRequest(REQUEST_WRITE_PREFS);
    NS_ENSURE_SUCCESS(rv, /* void */);
  }
}


//------------------------------------------------------------------------------
//
// iPod device request services.
//
//------------------------------------------------------------------------------

/**
 * Enqueue a request of type specified by aType to set the named value specified
 * by aName to the value specified by aValue.
 *
 * \param aType                 Type of request.
 * \param aName                 Name of value to set.
 * \param aValue                Value to set.
 */

nsresult
sbIPDDevice::ReqPushSetNamedValue(int              aType,
                                  const nsAString& aName,
                                  nsIVariant*      aValue)
{
  // Validate arguments.
  NS_ASSERTION(aValue, "aValue is null");

  // Function variables.
  nsresult rv;

  // Set up the set named value request.
  SetNamedValueRequest * request = new SetNamedValueRequest(aType);
  NS_ENSURE_TRUE(request, NS_ERROR_OUT_OF_MEMORY);
  request->name = aName;
  request->value = aValue;

  // Enqueue the request.
  //XXXeps This can raise a potential deadlock assert when called from the
  //XXXeps request thread.  Not sure what to do about that.
  rv = mRequestThreadQueue->PushRequest(request);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


