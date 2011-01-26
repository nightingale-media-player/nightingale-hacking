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
#include <nsThreadUtils.h>
#include <nsILocalFile.h>


//------------------------------------------------------------------------------
//
// iPod device request sbBaseDevice services.
//
//------------------------------------------------------------------------------

/**
 * A request has been added, process the request (or schedule it to be
 * processed)
 */

nsresult
sbIPDDevice::ProcessRequest()
{
  // Operate under the connect lock.
  sbAutoReadLock autoConnectLock(mConnectLock);

  // if we're not connected then do nothing
  if (!mConnected) {
    return NS_OK;
  }

  // Function variables.
  nsresult rv;

  // Dispatch processing of the request added event.
  rv = mReqThread->Dispatch(mReqAddedEvent, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


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
sbIPDDevice::ReqHandleRequestAdded()
{
  // Check for abort.
  NS_ENSURE_FALSE(ReqAbortActive(), NS_ERROR_ABORT);

  // Operate under the connect lock.
  sbAutoReadLock autoConnectLock(mConnectLock);
  NS_ENSURE_TRUE(mConnected, NS_ERROR_NOT_AVAILABLE);

  // Function variables.
  nsresult rv;

  // Prevent re-entrancy.  This can happen if some API waits and runs events on
  // the request thread.  Do nothing if already handling requests.  Otherwise,
  // indicate that requests are being handled and set up to automatically set
  // the handling requests flag to false on exit.  This check is only done on
  // the request thread, so locking is not required.
  if (mIsHandlingRequests)
    return NS_OK;
  mIsHandlingRequests = PR_TRUE;
  sbIPDAutoFalse autoIsHandlingRequests(&mIsHandlingRequests);

  // Start processing of the next request batch and set to automatically
  // complete the current request on exit.
  sbBaseDevice::Batch requestBatch;
  rv = StartNextRequest(requestBatch);
  NS_ENSURE_SUCCESS(rv, rv);
  if (requestBatch.empty())
    return NS_OK;
  sbAutoDeviceCompleteCurrentRequest autoComplete(this);

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

    // If the batch isn't empty, process the batch
    while (!requestBatch.empty()) {
      bool ensuredSpaceForWrite = false;

      // Check for abort.
      if (ReqAbortActive()) {
        rv = RemoveLibraryItems(requestBatch.begin(), requestBatch.end());
        NS_ENSURE_SUCCESS(rv, rv);
        return NS_ERROR_ABORT;
      }

      // Process each request in the batch
      Batch::iterator const end = requestBatch.end();
      Batch::iterator iter = requestBatch.begin();
      while (iter != end) {

        // Check for abort.
        if (ReqAbortActive()) {
          rv = RemoveLibraryItems(iter, end);
          NS_ENSURE_SUCCESS(rv, rv);

          return NS_ERROR_ABORT;
        }

        TransferRequest * request = iter->get();

        {
          nsAutoMonitor autoRequestLock(mRequestLock);
          FIELD_LOG(("Processing request 0x%08x\n", request->type));
          switch(request->type)
          {
            case TransferRequest::REQUEST_MOUNT :
              mIPDStatus->ChangeStatus(STATE_MOUNTING);
              ReqHandleMount(request);
            break;

            case TransferRequest::REQUEST_WRITE : {
              if (!ensuredSpaceForWrite) {

                // Force update of storage statistics before checking for space.
                StatsUpdate(PR_TRUE);

                // Check for space for request.  Assume space is ensured on error
                // and attempt the write.
                rv = EnsureSpaceForWrite(requestBatch);
                NS_ENSURE_SUCCESS(rv, rv);

                ensuredSpaceForWrite = true;

                // Reset the iter and request in case the first item was removed
                iter = requestBatch.begin();
              }

              // Handle request if space has been ensured for it.
              if (iter != end) {
                request = iter->get();
                // Handle request if space has been ensured for it.
                mIPDStatus->ChangeStatus(STATE_COPYING);
                ReqHandleWrite(request);
              }
            }
            break;

            case TransferRequest::REQUEST_DELETE :
              mIPDStatus->ChangeStatus(STATE_DELETING);
              ReqHandleDelete(request);
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
              ReqHandleSetPref(request);
              break;

            default :
              NS_WARNING("Invalid request type.");
              break;
          }
          if (ReqAbortActive()) {
            // If we're aborting iterators will be invalid, need to exit loop
            // to get the next batch if any.
            break;
          }
          // Something may have advanced the iter so need to check for end
          if (iter != end) {
            requestBatch.erase(iter);
            iter = requestBatch.begin();
          }
        }
      }

      // Complete the current request and forget auto-completion.
      CompleteCurrentRequest();
      autoComplete.forget();

      // Start processing of the next request batch and set to automatically
      // complete the current request on exit.
      rv = StartNextRequest(requestBatch);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!requestBatch.empty())
        autoComplete.Set(this);
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
sbIPDDevice::ReqHandleMount(TransferRequest* aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  // Update status and set for auto-failure.
  mIPDStatus->OperationStart(sbIPDStatus::OPERATION_TYPE_MOUNT,
      aRequest->batchIndex, aRequest->batchCount);
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
sbIPDDevice::ReqHandleWrite(TransferRequest* aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Write the item.
  if (aRequest->IsPlaylist())
    ReqHandleWritePlaylistTrack(aRequest);
  else
    ReqHandleWriteTrack(aRequest);
}


/**
 * Handle the write track request specified by aRequest.
 *
 * This function must be called under the connect and request locks.
 *
 * \param aRequest              Request data record.
 */

void
sbIPDDevice::ReqHandleWriteTrack(TransferRequest* aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  // Update operation status and set for auto-completion.
  sbAutoStatusOperationComplete autoOperationStatus(mIPDStatus, NS_ERROR_FAILURE);
  if (aRequest->batchIndex == 1) {
    mIPDStatus->OperationStart(sbIPDStatus::OPERATION_TYPE_WRITE,
                               aRequest->batchIndex,
                               aRequest->batchCount,
                               aRequest->list,
                               aRequest->item);
  }
  if (aRequest->batchIndex == aRequest->batchCount) {
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
sbIPDDevice::ReqHandleWipe(TransferRequest* aRequest)
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
sbIPDDevice::ReqHandleDelete(TransferRequest* aRequest)
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
      ReqHandleDeleteTrack(aRequest);
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
sbIPDDevice::ReqHandleDeleteTrack(TransferRequest* aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  // Update operation status and set for auto-completion.
  sbAutoStatusOperationComplete autoOperationStatus(mIPDStatus, NS_ERROR_FAILURE);
  if (aRequest->batchIndex == 1) {
    mIPDStatus->OperationStart(sbIPDStatus::OPERATION_TYPE_DELETE,
                               aRequest->batchIndex,
                               aRequest->batchCount,
                               aRequest->list,
                               aRequest->item);
  }
  if (aRequest->batchIndex == aRequest->batchCount) {
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
sbIPDDevice::ReqHandleDeletePlaylistTrack(TransferRequest* aRequest)
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
sbIPDDevice::ReqHandleDeletePlaylist(TransferRequest* aRequest)
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
sbIPDDevice::ReqHandleNewPlaylist(TransferRequest* aRequest)
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
sbIPDDevice::ReqHandleUpdate(TransferRequest* aRequest)
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
sbIPDDevice::ReqHandleMovePlaylistTrack(TransferRequest* aRequest)
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
sbIPDDevice::ReqHandleFactoryReset(TransferRequest* aRequest)
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
sbIPDDevice::ReqHandleWritePrefs(TransferRequest* aRequest)
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
sbIPDDevice::ReqHandleSetProperty(TransferRequest* aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  // Process the request.
  SetNamedValueRequest* request = static_cast<SetNamedValueRequest*>(aRequest);
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
sbIPDDevice::ReqHandleSetPref(TransferRequest* aRequest)
{
  // Validate arguments.
  NS_ASSERTION(aRequest, "aRequest is null");

  // Function variables.
  nsresult rv;

  // Process the request.
  SetNamedValueRequest* request = static_cast<SetNamedValueRequest*>(aRequest);
  if (request->name.EqualsLiteral("SyncPartner")) {
    // Get the Songbird sync partner ID.
    nsAutoString sbSyncPartnerID;
    rv = request->value->GetAsAString(sbSyncPartnerID);
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
 * Initialize the iPod device request services.
 */

nsresult
sbIPDDevice::ReqInitialize()
{
  // Create the request lock.
  mRequestLock = nsAutoMonitor::NewMonitor("sbIPDDevice::mRequestLock");
  NS_ENSURE_TRUE(mRequestLock, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}


/**
 * Finalize the iPod device request services.
 */

void
sbIPDDevice::ReqFinalize()
{
  // Dispose of the request lock.
  if (mRequestLock)
    nsAutoMonitor::DestroyMonitor(mRequestLock);
  mRequestLock = nsnull;
}


/**
 * Connect the iPod device request services.
 *
 * This function is called as part of the device connection process, so it does
 * not need to operate under the connect lock.  However, it should avoid
 * accessing "connect lock" fields that are not a part of the request services.
 */

nsresult
sbIPDDevice::ReqConnect()
{
  nsresult rv;

  // Clear the stop request processing flag.
  PR_AtomicSet(&mReqStopProcessing, 0);

  // Create the request added event object.
  rv = sbIPDReqAddedEvent::New(this, getter_AddRefs(mReqAddedEvent));
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the request processing thread.
  rv = NS_NewThread(getter_AddRefs(mReqThread), nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Disconnect the iPod device request services.
 *
 * This function is called as part of the device connection process, so it does
 * not need to operate under the connect lock.  However, it should avoid
 * accessing "connect lock" fields that are not a part of the request services.
 */

void
sbIPDDevice::ReqDisconnect()
{
  // Clear all remaining requests.
  ClearRequests();

  // Remove object references.
  mReqAddedEvent = nsnull;
  mReqThread = nsnull;
}


/**
 * Start processing device requests.
 */

nsresult
sbIPDDevice::ReqProcessingStart()
{
  // Operate under the connect lock.
  sbAutoReadLock autoConnectLock(mConnectLock);
  NS_ENSURE_TRUE(mConnected, NS_ERROR_NOT_AVAILABLE);

  // Process any queued requests.
  ProcessRequest();

  return NS_OK;
}


/**
 * Stop processing device requests.
 */

nsresult
sbIPDDevice::ReqProcessingStop()
{
  // Operate under the connect lock.
  sbAutoReadLock autoConnectLock(mConnectLock);
  NS_ENSURE_TRUE(mConnected, NS_ERROR_NOT_AVAILABLE);

  // Set the stop processing requests flag.
  PR_AtomicSet(&mReqStopProcessing, 1);

  // Shutdown the request processing thread.
  mReqThread->Shutdown();

  return NS_OK;
}


/**
 * Return true if the active request should abort; otherwise, return false.
 *
 * \return PR_TRUE              Active request should abort.
 *         PR_FALSE             Active request should not abort.
 */

PRBool
sbIPDDevice::ReqAbortActive()
{
  // Abort if request processing has been stopped.
  if (PR_AtomicAdd(&mReqStopProcessing, 0))
    return PR_TRUE;

  return IsRequestAbortedOrDeviceDisconnected();
}


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
  nsRefPtr<SetNamedValueRequest> request = new SetNamedValueRequest();
  NS_ENSURE_TRUE(request, NS_ERROR_OUT_OF_MEMORY);
  request->type = aType;
  request->name = aName;
  request->value = aValue;

  // Enqueue the request.
  //XXXeps This can raise a potential deadlock assert when called from the
  //XXXeps request thread.  Not sure what to do about that.
  rv = PushRequest(request);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// iPod device request added event services.
//
//   These services provide an nsIRunnable interface that may be used to
// dispatch and process device request added events.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// iPod device request added event nsISupports services.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbIPDReqAddedEvent, nsIRunnable)


//------------------------------------------------------------------------------
//
// iPod device request added event nsIRunnable services.
//
//------------------------------------------------------------------------------

/**
 * Run the event.
 */

NS_IMETHODIMP
sbIPDReqAddedEvent::Run()
{
  // Dispatch to the device object to handle the event.
  mDevice->ReqHandleRequestAdded();

  return NS_OK;
}


/**
 * Create a new sbIPDReqAddedEvent object for the device specified by aDevice
 * and return it in aEvent.
 *
 * \param aDevice               Device for which to create event.
 * \param aEvent                Created event.
 */

/* static */ nsresult
sbIPDReqAddedEvent::New(sbIPDDevice*  aDevice,
                        nsIRunnable** aEvent)
{
  // Create the event object.
  sbIPDReqAddedEvent* event;
  NS_NEWXPCOM(event, sbIPDReqAddedEvent);
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  // Set the event parameters.
  event->mDevice = aDevice;

  // Return results.
  NS_ADDREF(*aEvent = event);

  return NS_OK;
}


