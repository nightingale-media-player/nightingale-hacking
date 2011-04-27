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
  nsCOMPtr<nsIMutableArray> mediaListsToAdd =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  nsCOMArray<sbILibraryChange> mediaItemsToUpdate;
  nsCOMArray<sbILibraryChange> mediaListsToUpdate;

  // Determine if playlists are supported
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
        nsCOMPtr<sbIMediaItem> sourceItem;
        rv = change->GetSourceItem(getter_AddRefs(sourceItem));

        PRBool isList;
        change->GetItemIsList(&isList);
        if (isList) {
          // If the change is to a media list, add it to the media list change
          // list.
          PRBool success = mediaListsToUpdate.AppendObject(change);
          NS_ENSURE_SUCCESS(success, NS_ERROR_OUT_OF_MEMORY);

        }
        // Update the item properties.
        rv = SyncUpdateProperties(change);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      break;
      default: {
        NS_WARNING("Unexpected change operation value in sbBaseDevice::Import");
      }
      break;
    }
  }

  // Add media lists.
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  rv = mediaItemsToAdd->Enumerate(getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aImportToLibrary->AddSome(enumerator);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add items.
  rv = ImportNewMediaLists(aImportToLibrary, mediaListsToAdd);
  NS_ENSURE_SUCCESS(rv, rv);

  // Sync the media lists.
  rv = ImportMediaLists(mediaListsToUpdate);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseDevice::ImportMediaLists(nsCOMArray<sbILibraryChange>& aMediaListChangeList)
{
  nsresult rv;

  // Just replace the destination media lists content with the sources
  // TODO: See if just applying changes is more efficient than clearing and
  // addAll

  // Sync each media list.
  PRInt32 count = aMediaListChangeList.Count();
  for (PRInt32 i = 0; i < count; i++) {
    // Get the media list change.
    nsCOMPtr<sbILibraryChange> change = aMediaListChangeList[i];

    // Get the destination media list item and library.
    nsCOMPtr<sbIMediaItem>     devMediaListItem;
    rv = change->GetSourceItem(getter_AddRefs(devMediaListItem));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaList> devMediaList =
        do_QueryInterface(devMediaListItem, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the destination media list item and library.
    nsCOMPtr<sbIMediaItem> mainMediaListItem;
    rv = change->GetDestinationItem(getter_AddRefs(mainMediaListItem));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaList> mainMediaList =
        do_QueryInterface(mainMediaListItem, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mainMediaList->Clear();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = CopyChangedMediaItemsToMediaList(change, mainMediaList);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbBaseDevice::ImportNewMediaLists(sbILibrary * aImportToLibrary,
                                  nsIArray * aNewMediaListsChanges)
{
  NS_ENSURE_ARG_POINTER(aNewMediaListsChanges);

  nsresult rv;

  PRUint32 length;
  rv = aNewMediaListsChanges->GetLength(&length);
  for (PRUint32 index = 0; index < length; ++index) {
    nsCOMPtr<sbILibraryChange> change =
        do_QueryElementAt(aNewMediaListsChanges, index, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
#ifdef DEBUG
    PRBool isList;
    rv = change->GetItemIsList(&isList);
    NS_ASSERTION(NS_SUCCEEDED(rv) && isList,
                 "Non-list change passed to ImportNewMediaLists");
#endif

    nsCOMPtr<sbIMediaItem> devMediaItem;
    rv = change->GetSourceItem(getter_AddRefs(devMediaItem));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaList> devMediaList = do_QueryInterface(devMediaItem, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIPropertyArray> properties;
    rv = devMediaList->GetProperties(nsnull, getter_AddRefs(properties));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaList> newMediaList;
    rv = aImportToLibrary->CreateMediaList(NS_LITERAL_STRING("simple"),
                                           properties,
                                           getter_AddRefs(newMediaList));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = sbLibraryUtils::LinkCopy(newMediaList, devMediaItem);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = CopyChangedMediaItemsToMediaList(change, newMediaList);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
