/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#include "sbBaseDevice.h"

// Local includes
#include "sbDeviceUtils.h"

// Mozilla includes
#include <nsArrayUtils.h>
#include <nsCOMArray.h>

// Songbird Includes
#include <sbStandardProperties.h>

// Songbird interfaces
#include <sbILibraryChangeset.h>
#include <sbIDeviceLibraryMediaSyncSettings.h>
#include <sbIDeviceLibrarySyncDiff.h>
#include <sbIDeviceLibrarySyncSettings.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>

NS_IMETHODIMP
sbBaseDevice::ImportFromDevice(sbILibrary * aImportToLibrary,
                               sbILibraryChangeset * aImportChangeset)
{
  NS_ENSURE_ARG_POINTER(aImportToLibrary);
  NS_ENSURE_ARG_POINTER(aImportChangeset);

  nsresult rv;

  // Get the list of all changes.
  nsCOMPtr<nsIArray> changeList;
  rv = aImportChangeset->GetChanges(getter_AddRefs(changeList));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 changeCount;
  rv = changeList->GetLength(&changeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // If there aren't any changes, don't bother setting up for the import
  if (changeCount == 0) {
    return NS_OK;
  }
  // Accumulators so we can batch the operations
  nsCOMPtr<nsIMutableArray> mediaItemsToAdd =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIMutableArray> mediaListsToAdd =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIMutableArray> mediaItemsToRemove =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  // Determine if playlists are supported
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIMutableArray> mediaListsToUpdate =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  bool const playlistsSupported = sbDeviceUtils::ArePlaylistsSupported(this);


  // Accumulate changes for later processing to take advantage of batching
  for (PRUint32 i = 0; i < changeCount; i++) {
    if (IsRequestAborted()) {
      return NS_ERROR_ABORT;
    }
    // Get the next change.
    nsCOMPtr<sbILibraryChange> change = do_QueryElementAt(changeList, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Put the change into the appropriate lists.
    PRUint32 operation;
    rv = change->GetOperation(&operation);
    NS_ENSURE_SUCCESS(rv, rv);

    // Is this a media item list
    PRBool itemIsList;
    rv = change->GetItemIsList(&itemIsList);
    NS_ENSURE_SUCCESS(rv, rv);

    // if this is a playlist and they're not supported ignore the change
    if (itemIsList && !playlistsSupported) {
      continue;
    }

    switch (operation) {
      case sbIChangeOperation::ADDED: {
        // Get the source item to add.
        nsCOMPtr<sbIMediaItem> mediaItem;
        rv = change->GetSourceItem(getter_AddRefs(mediaItem));
        NS_ENSURE_SUCCESS(rv, rv);

        if (itemIsList) {
          rv = mediaListsToAdd->AppendElement(change, PR_FALSE);
          NS_ENSURE_SUCCESS(rv, rv);
        } else {
          rv = mediaItemsToAdd->AppendElement(mediaItem, PR_FALSE);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
      break;
      case sbIChangeOperation::MODIFIED: {
        nsCOMPtr<sbIMediaItem> destItem;
        rv = change->GetDestinationItem(getter_AddRefs(destItem));
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<sbIMediaList> destItemAsList =
          do_QueryInterface(destItem);

        if (destItemAsList) {
          // If the change is to a media list, add it to the media list change
          // list.
          PRBool success = mediaListsToUpdate->AppendElement(change, PR_FALSE);
          NS_ENSURE_SUCCESS(success, NS_ERROR_OUT_OF_MEMORY);

        }
        else {
          rv = mediaItemsToRemove->AppendElement(destItem, PR_FALSE);
          NS_ENSURE_SUCCESS(rv, rv);

          nsCOMPtr<sbIMediaItem> sourceItem;
          rv = change->GetSourceItem(getter_AddRefs(sourceItem));

          rv = mediaItemsToAdd->AppendElement(sourceItem, PR_FALSE);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
      break;
      default: {
        NS_WARNING("Unexpected change operation value in sbBaseDevice::Import");
      }
      break;
    }
  }

  // Remove items that need to be removed
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  rv = mediaItemsToRemove->Enumerate(getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aImportToLibrary->RemoveSome(enumerator);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add media lists.
  rv = mediaItemsToAdd->Enumerate(getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aImportToLibrary->AddSome(enumerator);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddMediaLists(aImportToLibrary, mediaListsToAdd);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateMediaLists(mediaListsToUpdate);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
