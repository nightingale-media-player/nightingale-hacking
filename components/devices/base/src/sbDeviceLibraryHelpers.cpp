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

/*** sbLibraryUpdateListener ***/

#include "sbDeviceLibraryHelpers.h"

#include <nsArrayUtils.h>
#include <nsCOMPtr.h>

#include <sbIDevice.h>
#include <sbIDeviceLibraryMediaSyncSettings.h>
#include <sbIDeviceLibrarySyncSettings.h>
#include <sbIOrderableMediaList.h>
#include <sbIPropertyArray.h>

#include "sbDeviceLibrary.h"

#include <sbDeviceUtils.h>
#include <sbLibraryUtils.h>
#include <sbMediaListBatchCallback.h>
#include <sbStandardProperties.h>

static nsresult GetSyncItemInLibrary(sbIMediaItem*  aMediaItem,
                                     sbILibrary*    aTargetLibrary,
                                     sbIMediaItem** aSyncItem);

// note: this isn't actually threadsafe, but may be used on multiple threads
NS_IMPL_THREADSAFE_ISUPPORTS1(sbLibraryUpdateListener, sbIMediaListListener)

sbLibraryUpdateListener::sbLibraryUpdateListener(sbILibrary * aTargetLibrary,
                                                 bool aManualMode,
                                                 nsIArray * aPlaylistsList,
                                                 bool aIgnorePlaylists,
                                                 sbIDevice * aDevice)
  : mTargetLibrary(aTargetLibrary),
    mPlaylistListener(new sbPlaylistSyncListener(aTargetLibrary,
                                                 aPlaylistsList != nsnull,
                                                 aDevice)),
    mDevice(aDevice),
    mSyncPlaylists(!aPlaylistsList),
    mIgnorePlaylists(aIgnorePlaylists)
{
  SetSyncMode(aManualMode, aPlaylistsList);
}

sbLibraryUpdateListener::~sbLibraryUpdateListener()
{
  if (mPlaylistListener) {
    mPlaylistListener->StopListeningToPlaylists();
    mPlaylistListener = nsnull;
  }
}

void sbLibraryUpdateListener::SetSyncMode(bool aManualMode,
                                          nsIArray * aPlaylistList) {
  mSyncPlaylists = aPlaylistList != nsnull;
  mManualMode = aManualMode;
  mPlaylistListener->SetSyncPlaylists(aPlaylistList);
}

nsresult
sbLibraryUpdateListener::ListenToPlaylist(sbIMediaList * aMainMediaList)
{
  NS_ENSURE_ARG_POINTER(aMainMediaList);
  NS_ENSURE_TRUE(mTargetLibrary, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPlaylistListener, NS_ERROR_NOT_INITIALIZED);

#if DEBUG
  nsCOMPtr<sbILibrary> mediaListLib;
  aMainMediaList->GetLibrary(getter_AddRefs(mediaListLib));
  PRBool debugEquals;
  NS_ASSERTION(NS_SUCCEEDED(mediaListLib->Equals(mTargetLibrary, &debugEquals)) &&  debugEquals,
               "ListToPlaylist should only be passed playlists belonging to its target");
#endif
  nsresult rv = aMainMediaList->AddListener(mPlaylistListener,
                                            PR_FALSE, /* not weak */
                                            0 , /* all */
                                            nsnull /* filter */);
  NS_ENSURE_SUCCESS(rv, rv);

  // Listen to smart playlist rebuild event.
  nsCOMPtr<sbILocalDatabaseSmartMediaList> smartList =
    do_QueryInterface(aMainMediaList, &rv);
  if (NS_SUCCEEDED(rv) && smartList) {
    rv = smartList->AddSmartMediaListListener(mPlaylistListener);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mPlaylistListener->AddMediaList(aMainMediaList);

  return NS_OK;
}

nsresult
sbLibraryUpdateListener::StopListeningToPlaylist(sbIMediaList * aMainMediaList)
{
  NS_ENSURE_ARG_POINTER(aMainMediaList);
  NS_ENSURE_TRUE(mPlaylistListener, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = aMainMediaList->RemoveListener(mPlaylistListener);
  NS_ENSURE_SUCCESS(rv, rv);

  // Listen to smart playlist rebuild event.
  nsCOMPtr<sbILocalDatabaseSmartMediaList> smartList =
    do_QueryInterface(aMainMediaList, &rv);
  if (NS_SUCCEEDED(rv) && smartList) {
    rv = smartList->RemoveSmartMediaListListener(mPlaylistListener);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mPlaylistListener->RemoveMediaList(aMainMediaList);

  return NS_OK;
}

static nsresult ShouldAddMediaItem(sbILibrary *aLibrary,
                                   sbIMediaList *aMediaList,
                                   sbIMediaItem *aMediaItem,
                                   sbIDevice *aDevice,
                                   PRBool *aShouldAdd)
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aShouldAdd);

  nsresult rv;

  *aShouldAdd = PR_FALSE;

  // Should not add hidden item to the device.
  nsString hidden;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                               hidden);
  if (hidden.EqualsLiteral("1"))
    return NS_OK;

  nsCOMPtr<sbIDeviceLibrary> deviceLibrary;
  rv = sbDeviceUtils::GetDeviceLibraryForLibrary(aDevice,
                                                 aLibrary,
                                                 getter_AddRefs(deviceLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceLibrarySyncSettings> syncSettings;
  rv = deviceLibrary->GetSyncSettings(getter_AddRefs(syncSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceLibraryMediaSyncSettings> mediaSyncSettings;
  nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(aMediaItem, &rv);
  // Deal with media item.
  if (NS_FAILED(rv)) {
    nsString contentType;
    rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTTYPE),
                                 contentType);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the corresponding media sync settings based on the content type.
    if (contentType.EqualsLiteral("audio")) {
      rv = syncSettings->GetMediaSettings(sbIDeviceLibrary::MEDIATYPE_AUDIO,
                                          getter_AddRefs(mediaSyncSettings));
      NS_ENSURE_SUCCESS(rv, rv);

    }
    else if (contentType.EqualsLiteral("video")) {
      rv = syncSettings->GetMediaSettings(sbIDeviceLibrary::MEDIATYPE_VIDEO,
                                          getter_AddRefs(mediaSyncSettings));
      NS_ENSURE_SUCCESS(rv, rv);

    }
    else {
      NS_WARNING("Item not in audio or video type?!");
      return NS_OK;
    }

    PRUint32 mgmtType;
    rv = mediaSyncSettings->GetMgmtType(&mgmtType);
    NS_ENSURE_SUCCESS(rv, rv);

    // Sync all means include everything.
    if (mgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_ALL) {
      *aShouldAdd = PR_TRUE;
      return NS_OK;
    }

    // The item should be added if the list that contains the item is
    // selected.
    if (aMediaList &&
        mgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_PLAYLISTS) {
      // The item should be added if the list contains the item is selected.
      PRBool isSelected;
      rv = mediaSyncSettings->GetPlaylistSelected(aMediaList, &isSelected);
      NS_ENSURE_SUCCESS(rv, rv);
      if (isSelected) {
        *aShouldAdd = PR_TRUE;
        return NS_OK;
      }
    }
  }
  // Deal with media list.
  else {
    PRUint16 listContentType;
    rv = sbLibraryUtils::GetMediaListContentType(mediaList, &listContentType);
    // Condition not available for smart playlist
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      *aShouldAdd = PR_TRUE;
      return NS_OK;
    }

    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 i = 0; i < sbIDeviceLibrary::MEDIATYPE_COUNT - 1; ++i) {
      // Map the media type to media list content type. If list type does not
      // match, skip the check directly.
      if (!(listContentType & (i + 1)))
        continue;

      rv = syncSettings->GetMediaSettings(i, getter_AddRefs(mediaSyncSettings));
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 mgmtType;
      rv = mediaSyncSettings->GetMgmtType(&mgmtType);
      NS_ENSURE_SUCCESS(rv, rv);

      // Sync all means include everything.
      if (mgmtType == sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_ALL) {
        *aShouldAdd = PR_TRUE;
        return NS_OK;
      }
      // The list should be added if it is selected.
      else if (mgmtType ==
               sbIDeviceLibraryMediaSyncSettings::SYNC_MGMT_PLAYLISTS) {
        PRBool isSelected;
        rv = mediaSyncSettings->GetPlaylistSelected(mediaList, &isSelected);
        NS_ENSURE_SUCCESS(rv, rv);
        if (isSelected) {
          *aShouldAdd = PR_TRUE;
          return NS_OK;
        }
      }
    }
  }

  return NS_OK;
}

static nsresult ShouldMediaListSync(sbIDevice *aDevice,
                                    sbIMediaList *aMediaList,
                                    PRBool *aShouldSync)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aShouldSync);

  nsresult rv;

  *aShouldSync = PR_FALSE;

  PRUint16 listContentType;
  rv = sbLibraryUtils::GetMediaListContentType(aMediaList, &listContentType);
  // Condition not available for smart playlist
  if (rv == NS_ERROR_NOT_AVAILABLE)
    return NS_OK;

  NS_ENSURE_SUCCESS(rv, rv);

  // Only sync the playlist to the device if device supports the list content
  // type and list is not empty.
  PRBool isEmpty;
  rv = aMediaList->GetIsEmpty(&isEmpty);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!isEmpty && sbDeviceUtils::IsMediaListContentTypeSupported(
                                   aDevice,
                                   listContentType)) {
    *aShouldSync = PR_TRUE;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnItemAdded(sbIMediaList *aMediaList,
                                     sbIMediaItem *aMediaItem,
                                     PRUint32 aIndex,
                                     PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_TRUE(mTargetLibrary, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPlaylistListener, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;

#if DEBUG
  nsCOMPtr<sbIMediaItem> debugDeviceItem;
  GetSyncItemInLibrary(aMediaItem,
                       mTargetLibrary,
                       getter_AddRefs(debugDeviceItem));
  NS_ASSERTION(!debugDeviceItem,
               "sbLibraryUpdateListener::OnItemAdded: Item was already added to the library");
#endif

  nsCOMPtr<sbIMediaList> list = do_QueryInterface(aMediaItem);
  if (!list || !mIgnorePlaylists) {
    PRBool shouldAdd;
    rv = ShouldAddMediaItem(mTargetLibrary, nsnull, aMediaItem,
                            mDevice, &shouldAdd);
    NS_ENSURE_SUCCESS(rv, rv);

    if (shouldAdd) {
      // Add the item if it is not a list.
      if (!list) {
        // Only add media item to the target library if it is supported.
        if (sbDeviceUtils::IsMediaItemSupported(mDevice, aMediaItem)) {
          rv = mTargetLibrary->Add(aMediaItem);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
      else {
        // It's possible to create a new media list and add items to it before
        // OnItemAdded is called, like from a non-main thread.
        PRBool shouldSync;
        rv = ShouldMediaListSync(mDevice, list, &shouldSync);
        NS_ENSURE_SUCCESS(rv, rv);
        if (shouldSync) {
          rv = mTargetLibrary->Add(list);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        rv = ListenToPlaylist(list);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  if (_retval) {
    *_retval = PR_FALSE; /* keep going */
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnBeforeItemRemoved(sbIMediaList *aMediaList,
                                             sbIMediaItem *aMediaItem,
                                             PRUint32 aIndex,
                                             PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_TRUE(mTargetLibrary, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPlaylistListener, NS_ERROR_OUT_OF_MEMORY);

  // If this is a list and we're ignoring lists then just leave
  nsCOMPtr<sbIMediaList> list = do_QueryInterface(aMediaItem);
  if (list && mIgnorePlaylists) {
    return NS_OK;
  }
  nsresult rv;

  // target list could be unavailable.
  if (list) {
    rv = StopListeningToPlaylist(list);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (_retval) {
    *_retval = PR_FALSE; /* keep going */
  }
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnAfterItemRemoved(sbIMediaList *aMediaList,
                                            sbIMediaItem *aMediaItem,
                                            PRUint32 aIndex,
                                            PRBool *_retval)
{
  NS_NOTREACHED("Why are we here?");
  if (_retval) {
    *_retval = PR_TRUE; /* stop */
  }

  return NS_OK;
}

/**
 * Determines if an item was unhidden.
 * \PARAMS aOldProps old property values for the item
 * \PARAMS aNewProps new property values for the item
 * \PARAMS aisUnhidden set to true if the item is now visible
 */
static nsresult
GetHideState(sbIPropertyArray * aOldProps,
             sbIPropertyArray * aNewProps,
             PRBool & aIsHidden,
             PRBool & aIsUnhidden)
{
  nsresult rv;
  nsString oldHidden;
  rv = aOldProps->GetPropertyValue(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                                   oldHidden);
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    oldHidden.SetIsVoid(PR_TRUE);
  }
  else {
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsString newHidden;
  rv = aNewProps->GetPropertyValue(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                                   newHidden);
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    newHidden.SetIsVoid(PR_TRUE);
  }
  else {
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRBool const oldIsHidden = oldHidden.Equals(NS_LITERAL_STRING("1"));
  PRBool const newIsHidden = newHidden.Equals(NS_LITERAL_STRING("1"));

  // If we are not hidden now, but were before return true
  aIsUnhidden = oldIsHidden && !newIsHidden;

  aIsHidden = newIsHidden && !oldIsHidden;

  return NS_OK;
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnItemUpdated(sbIMediaList *aMediaList,
                                       sbIMediaItem *aMediaItem,
                                       sbIPropertyArray *aProperties,
                                       PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aProperties);
  NS_ENSURE_TRUE(mTargetLibrary, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPlaylistListener, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;

  // If this is a list and we're ignoring lists or the list is not simple,
  // or the list should otherwise not be synced, then just leave.
  nsCOMPtr<sbIMediaList> list = do_QueryInterface(aMediaItem);
  if (list) {
    // Check if playlists are being ignored
    if (mIgnorePlaylists)
      return NS_OK;

    // Check if list is not simple
    nsString customType;
    rv = list->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CUSTOMTYPE),
                           customType);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!customType.IsEmpty() &&
        !customType.Equals(NS_LITERAL_STRING("simple"))) {
      return NS_OK;
    }

    PRBool shouldSync;
    rv = ShouldMediaListSync(mDevice, list, &shouldSync);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!shouldSync)
      return NS_OK;
  }

  // the property array here are the old values; we need the new ones
  nsCOMPtr<sbIPropertyArray> newProps;
  rv = aMediaItem->GetProperties(aProperties, getter_AddRefs(newProps));
  NS_ENSURE_SUCCESS(rv, rv);

  // If this item is being unhidden then we'll need to add to the device lib
  PRBool isHidden = PR_FALSE;
  PRBool isUnhidden = PR_FALSE;
  rv = GetHideState(aProperties, newProps, isHidden, isUnhidden);
  NS_ENSURE_SUCCESS(rv, rv);

  // It's no longer hidden so add it
  if (isUnhidden) {
    rv = mTargetLibrary->Add(aMediaItem);
    NS_ENSURE_SUCCESS(rv, rv);
    if (list) {
      rv = ListenToPlaylist(list);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else if (isHidden) {
    if (list) {
      rv = StopListeningToPlaylist(list);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  nsCOMPtr<sbIMediaItem> targetItem;
  rv = GetSyncItemInLibrary(aMediaItem,
                            mTargetLibrary,
                            getter_AddRefs(targetItem));
  NS_ENSURE_SUCCESS(rv, rv);
  if (targetItem) {
    rv = targetItem->SetProperties(newProps);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (_retval) {
    *_retval = PR_FALSE; /* keep going */
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnItemMoved(sbIMediaList *aMediaList,
                                     PRUint32 aFromIndex,
                                     PRUint32 aToIndex,
                                     PRBool *_retval)
{
  NS_NOTREACHED("Why are we here?");

  if (_retval) {
    *_retval = PR_TRUE; /* STOP */
  }
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnListCleared(sbIMediaList *aMediaList,
                                       PRBool aExcludeLists,
                                       PRBool *_retval)
{
  NS_NOTREACHED("Why are we here?");
  if (_retval) {
    *_retval = PR_TRUE; /* stop */
  }
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnBeforeListCleared(sbIMediaList *aMediaList,
                                             PRBool aExcludeLists,
                                             PRBool *_retval)
{
  NS_NOTREACHED("Why are we here?");
  if (_retval) {
    *_retval = PR_TRUE; /* stop */
  }
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnBatchBegin(sbIMediaList *aMediaList)
{
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryUpdateListener::OnBatchEnd(sbIMediaList *aMediaList)
{
  return NS_OK;
}

// sbPlaylistSyncListener class
NS_IMPL_THREADSAFE_ISUPPORTS1(sbPlaylistSyncListener,
                              sbIMediaListListener)

sbPlaylistSyncListener::sbPlaylistSyncListener(sbILibrary * aTargetLibrary,
                                               bool aSyncPlaylists,
                                               sbIDevice * aDevice)
: mTargetLibrary(aTargetLibrary),
  mDevice(aDevice),
  mSyncPlaylists(aSyncPlaylists)
{
  NS_ASSERTION(aTargetLibrary,
               "sbPlaylistSyncListener cannot be given a null aTargetLibrary");

  // Initialize the batch helper table.
  if (!mBatchHelperTable.Init()) {
    NS_WARNING("Out of memory!");
  }
}

sbPlaylistSyncListener::~sbPlaylistSyncListener()
{
  mBatchHelperTable.Clear();
  mListRemovedArray.Clear();
}

void sbPlaylistSyncListener::StopListeningToPlaylists() {
  PRUint32 length = mMediaLists.Count();
  nsresult rv;
  for (PRUint32 index = 0; index < length; ++index) {
    nsCOMPtr<sbIMediaList> list = mMediaLists[index];
    list->RemoveListener(this);

    // Remove the smart playlist listener
    nsCOMPtr<sbILocalDatabaseSmartMediaList> smartList =
      do_QueryInterface(list, &rv);
    if (NS_SUCCEEDED(rv) && smartList) {
      rv = smartList->RemoveSmartMediaListListener(this);
      NS_ENSURE_SUCCESS(rv, /* void */);
    }
  }
  mMediaLists.Clear();
}

nsresult sbPlaylistSyncListener::SetSyncPlaylists(nsIArray * aMediaLists) {
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;

  mMediaLists.Clear();
  mSyncPlaylists = aMediaLists != nsnull;
  if (mSyncPlaylists) {
    PRUint32 length = 0;
    if (aMediaLists) {
      rv = aMediaLists->GetLength(&length);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<sbIMediaList> mediaList;
    for (PRUint32 index = 0; index < length; ++index) {
      mediaList = do_QueryElementAt(aMediaLists, index, &rv);
      if (NS_FAILED(rv)) {
        continue;
      }
      NS_ENSURE_TRUE(mMediaLists.AppendObject(mediaList),
                     NS_ERROR_OUT_OF_MEMORY);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistSyncListener::OnItemAdded(sbIMediaList *aMediaList,
                                    sbIMediaItem *aMediaItem,
                                    PRUint32 aIndex,
                                    PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_TRUE(mTargetLibrary, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;

  nsCOMPtr<sbILibrary> lib = do_QueryInterface(aMediaList);
  if (lib) {
    NS_NOTREACHED("Why are we here?");
    return NS_OK;
  }

  nsCOMPtr<sbIMediaList> list = do_QueryInterface(aMediaItem);
  if (list) {
    // a list being added to a list? we don't care, I think?
  } else {
    // If the device does not support the media item, just leave.
    if (!sbDeviceUtils::IsMediaItemSupported(mDevice, aMediaItem))
      return NS_OK;

    PRBool shouldAdd;
    rv = ShouldAddMediaItem(mTargetLibrary, aMediaList, aMediaItem,
                            mDevice, &shouldAdd);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!shouldAdd)
      return NS_OK;

    nsString hidden;
    rv = aMediaList->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                                 hidden);
    // Ignore hidden playlist. Could be inner playlist for smart playlist.
    // OnRebuild will handle smart playlist.
    if (hidden.EqualsLiteral("1"))
      return NS_OK;

    nsCOMPtr<sbIMediaItem> deviceMediaListAsItem;
    rv = GetSyncItemInLibrary(aMediaList,
                              mTargetLibrary,
                              getter_AddRefs(deviceMediaListAsItem));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaList> deviceMediaList;
    // device media list does not exists. Add the list to the target library.
    if (!deviceMediaListAsItem) {
      if (sbDeviceUtils::ArePlaylistsSupported(mDevice)) {
        // Do not add list content to the library.
        rv = mTargetLibrary->CopyMediaList(NS_LITERAL_STRING("simple"),
                                           aMediaList,
                                           PR_TRUE,
                                           getter_AddRefs(deviceMediaList));
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    else {
      deviceMediaList = do_QueryInterface(deviceMediaListAsItem, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (deviceMediaList) {
      // Null case is handled
      nsCOMPtr<sbIOrderableMediaList> orderedList =
        do_QueryInterface(deviceMediaList);

      PRUint32 length;
      rv = deviceMediaList->GetLength(&length);
      NS_ENSURE_SUCCESS(rv, rv);

      // If this is an ordered list and we're not appending
      if (orderedList && aIndex < length)
        rv = orderedList->InsertBefore(aIndex, aMediaItem);
      else {
        rv = deviceMediaList->Add(aMediaItem);
      }
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      // The device doesn't support playlists, so just add the item to the
      // device library.
      rv = mTargetLibrary->Add(aMediaItem);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if (_retval) {
    *_retval = PR_FALSE; /* don't stop */
  }

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistSyncListener::OnBeforeItemRemoved(sbIMediaList *aMediaList,
                                            sbIMediaItem *aMediaItem,
                                            PRUint32 aIndex,
                                            PRBool *_retval)
{
  if (_retval) {
    *_retval = PR_TRUE; /* stop */
  }
  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistSyncListener::OnAfterItemRemoved(sbIMediaList *aMediaList,
                                           sbIMediaItem *aMediaItem,
                                           PRUint32 aIndex,
                                           PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);

  // If the removed item is not supported by the device, just ignore.
  if (!sbDeviceUtils::IsMediaItemSupported(mDevice, aMediaItem)) {
    return NS_OK;
  }

  // Not in a batch. Rebuild playlist directly.
  if (!mBatchHelperTable.Get(aMediaList, nsnull)) {
    nsresult rv = RemoveItemNotInBatch(aMediaList, aMediaItem, aIndex);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    PRInt32 index;
    PRBool success;

    index = mListRemovedArray.IndexOf(aMediaList);
    // Media list not found in the array. Append.
    if (index < 0) {
      success = mListRemovedArray.AppendObject(aMediaList);
      NS_ENSURE_SUCCESS(success, NS_ERROR_OUT_OF_MEMORY);
    }
  }

  if (_retval) {
    *_retval = PR_FALSE; /* don't stop */
  }

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistSyncListener::OnItemUpdated(sbIMediaList *aMediaList,
                                      sbIMediaItem *aMediaItem,
                                      sbIPropertyArray *aProperties,
                                      PRBool *_retval)
{
  if (_retval) {
    *_retval = PR_TRUE; /* stop */
  }
  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistSyncListener::OnItemMoved(sbIMediaList *aMediaList,
                                    PRUint32 aFromIndex,
                                    PRUint32 aToIndex,
                                    PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_TRUE(mTargetLibrary, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;
  nsCOMPtr<sbIMediaItem> deviceMediaList;
  rv = GetSyncItemInLibrary(aMediaList,
                            mTargetLibrary,
                            getter_AddRefs(deviceMediaList));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIOrderableMediaList> orderedMediaList =
    do_QueryInterface(deviceMediaList);
  if (orderedMediaList) {
    rv = orderedMediaList->MoveBefore(aFromIndex, aToIndex);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (_retval) {
    *_retval = PR_FALSE; /* don't stop */
  }

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistSyncListener::OnBeforeListCleared(sbIMediaList *aMediaList,
                                            PRBool aExcludeLists,
                                            PRBool *_retval)
{
  if (_retval) {
    *_retval = PR_TRUE; /* stop */
  }
  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistSyncListener::OnListCleared(sbIMediaList *aMediaList,
                                      PRBool aExcludeLists,
                                      PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  if (_retval) {
    *_retval = PR_FALSE; /* don't stop */
  }

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistSyncListener::OnBatchBegin(sbIMediaList *aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  sbLibraryBatchHelper *batchHelper;
  // Create a new batch helper for media list if not exist
  if (!mBatchHelperTable.Get(aMediaList, &batchHelper)) {
    batchHelper = new sbLibraryBatchHelper();
    NS_ENSURE_TRUE(batchHelper, NS_ERROR_OUT_OF_MEMORY);
    PRBool success = mBatchHelperTable.Put(aMediaList, batchHelper);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  // Increase the batch depth.
  batchHelper->Begin();

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistSyncListener::OnBatchEnd(sbIMediaList *aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsresult rv;

  sbLibraryBatchHelper *batchHelper;
  PRBool success = mBatchHelperTable.Get(aMediaList,
                                         &batchHelper);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  // Decrease the batch depth.
  batchHelper->End();

  // Rebuild the playlist when the remove batch comes to an end.
  if (!batchHelper->IsActive()) {
    mBatchHelperTable.Remove(aMediaList);

    PRInt32 index = mListRemovedArray.IndexOf(aMediaList);
    // Not found in the array means not remove batch.
    if (index < 0)
      return NS_OK;

    PRBool success = mListRemovedArray.RemoveObjectAt(index);
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
    rv = RebuildPlaylistAfterItemRemoved(aMediaList);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!mBatchHelperTable.Count()) {
    mListRemovedArray.Clear();
  }

  return NS_OK;
}

class AddItemsBatchParams : public nsISupports
{
public:
  NS_DECL_ISUPPORTS
  nsCOMPtr<sbIMediaList>       deviceMediaList;
  nsCOMArray<sbIMediaItem>     mediaItems;
};
NS_IMPL_ISUPPORTS0(AddItemsBatchParams)

nsresult
sbPlaylistSyncListener::RebuildPlaylistAfterItemRemoved(
                                            sbIMediaList *aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_TRUE(mTargetLibrary, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;

  nsCOMPtr<sbIMediaItem> targetListAsItem;
  rv = GetSyncItemInLibrary(aMediaList,
                            mTargetLibrary,
                            getter_AddRefs(targetListAsItem));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> deviceMediaList =
    do_QueryInterface(targetListAsItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Clear the media list first
  rv = deviceMediaList->Clear();
  NS_ENSURE_SUCCESS(rv, rv);

  // Set up the batch callback function parameters.
  nsRefPtr<AddItemsBatchParams> addItemsBatchParams =
                                     new AddItemsBatchParams();
  NS_ENSURE_TRUE(addItemsBatchParams, NS_ERROR_OUT_OF_MEMORY);
  addItemsBatchParams->deviceMediaList = deviceMediaList;

  rv = sbLibraryUtils::GetItemsByProperty(
                          aMediaList,
                          NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                          NS_LITERAL_STRING("0"),
                          addItemsBatchParams->mediaItems);
  NS_ENSURE_SUCCESS(rv, rv);

  // Rebuild the playlist to include all the remaining items in the media list.

  // Set up the batch callback function.
  nsCOMPtr<sbIMediaListBatchCallback>
    batchCallback = new sbMediaListBatchCallback(AddItemsToList);
  NS_ENSURE_TRUE(batchCallback, NS_ERROR_OUT_OF_MEMORY);

  // Add items in batch mode.
  rv = deviceMediaList->RunInBatchMode(batchCallback, addItemsBatchParams);
  NS_ENSURE_SUCCESS(rv, rv);


  return NS_OK;
}

/* static */
nsresult sbPlaylistSyncListener::AddItemsToList(nsISupports* aUserData)
{
  // Validate parameters.
  NS_ENSURE_ARG_POINTER(aUserData);

  // Function variables.
  nsresult rv;

  // Get the add items batch parameters.
  AddItemsBatchParams* addItemsBatchParams =
                         static_cast<AddItemsBatchParams *>(aUserData);
  nsCOMPtr<sbIMediaList> mediaList = addItemsBatchParams->deviceMediaList;

  // Add a batch of items to the list.
  nsCOMPtr<sbIMediaItem> mediaItem;
  for (PRInt32 i = 0; i < addItemsBatchParams->mediaItems.Count(); ++i) {
    mediaItem = addItemsBatchParams->mediaItems.ObjectAt(i);
    NS_ENSURE_TRUE(mediaItem, NS_ERROR_FAILURE);

    rv = mediaList->Add(mediaItem);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbPlaylistSyncListener::RemoveItemNotInBatch(sbIMediaList *aMediaList,
                                             sbIMediaItem *aMediaItem,
                                             PRUint32 aIndex)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_TRUE(mTargetLibrary, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;

  nsCOMPtr<sbIMediaItem> targetListAsItem;
  rv = GetSyncItemInLibrary(aMediaList,
                            mTargetLibrary,
                            getter_AddRefs(targetListAsItem));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> deviceMediaList =
    do_QueryInterface(targetListAsItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> deviceMediaItem;
  rv = GetSyncItemInLibrary(aMediaItem,
                            mTargetLibrary,
                            getter_AddRefs(deviceMediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

#if DEBUG
  nsCOMPtr<sbIMediaItem> debugItem;
  rv = deviceMediaList->GetItemByIndex(aIndex, getter_AddRefs(debugItem));
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool debugEquals;
  NS_ASSERTION(NS_SUCCEEDED(debugItem->Equals(deviceMediaItem,
                                               &debugEquals)) && debugEquals,
                            "Item not the correct item in the playlist");
#endif

  if (mSyncPlaylists) {
    bool exists = false;

    // If there is more than one of this item in the list we'll need to keep it
    PRUint32 itemIndex;
    rv = deviceMediaList->IndexOf(deviceMediaItem, 0, &itemIndex);
    if (NS_SUCCEEDED(rv)) {
      rv = deviceMediaList->IndexOf(deviceMediaItem, itemIndex + 1, &itemIndex);
      exists = NS_SUCCEEDED(rv);
    }
  }

  rv = deviceMediaList->RemoveByIndex(aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 *   Return in aSyncItem the target sync media item in the target sync library
 * specified by aTargetLibrary corresponding to the source sync media item
 * specified by aMediaItem.  If no matching target sync media item can be found,
 * this function returns NS_OK and returns nsnull in aSyncItem.
 *
 * \param aMediaItem            Sync source media item.
 * \param aTargetLibrary        Sync target library.
 * \param aSyncItem             Sync target media item.
 */
static nsresult
GetSyncItemInLibrary(sbIMediaItem*  aMediaItem,
                     sbILibrary*    aTargetLibrary,
                     sbIMediaItem** aSyncItem)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aTargetLibrary);
  NS_ENSURE_ARG_POINTER(aSyncItem);

  // Function variables.
  nsresult rv;

  // Try getting the sync item directly in the target library.
  rv = sbLibraryUtils::GetItemInLibrary(aMediaItem, aTargetLibrary, aSyncItem);
  NS_ENSURE_SUCCESS(rv, rv);
  if (*aSyncItem)
    return NS_OK;

  // Get the source library.
  nsCOMPtr<sbILibrary> sourceLibrary;
  rv = aMediaItem->GetLibrary(getter_AddRefs(sourceLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  // Try getting the sync item using the outer GUID property.
  nsAutoString outerGUID;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_OUTERGUID),
                               outerGUID);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!outerGUID.IsEmpty()) {
    // Get the outer media item.
    nsCOMPtr<sbIMediaItem> outerMediaItem;
    rv = sourceLibrary->GetMediaItem(outerGUID, getter_AddRefs(outerMediaItem));
    NS_ENSURE_SUCCESS(rv, rv);

    // Try getting the sync item from the outer media item.
    rv = sbLibraryUtils::GetItemInLibrary(outerMediaItem,
                                          aTargetLibrary,
                                          aSyncItem);
    NS_ENSURE_SUCCESS(rv, rv);
    if (*aSyncItem)
      return NS_OK;
  }

  // Try getting the sync item using the storage GUID property.
  nsAutoString storageGUID;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_STORAGEGUID),
                               storageGUID);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!storageGUID.IsEmpty()) {
    // Get the storage media item.
    nsCOMPtr<sbIMediaItem> storageMediaItem;
    rv = sourceLibrary->GetMediaItem(storageGUID,
                                     getter_AddRefs(storageMediaItem));
    NS_ENSURE_SUCCESS(rv, rv);

    // Try getting the sync item from the storage media item.
    rv = sbLibraryUtils::GetItemInLibrary(storageMediaItem,
                                          aTargetLibrary,
                                          aSyncItem);
    NS_ENSURE_SUCCESS(rv, rv);
    if (*aSyncItem)
      return NS_OK;
  }

  // Could not find the sync item.
  *aSyncItem = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistSyncListener::OnRebuild(
                          sbILocalDatabaseSmartMediaList *aSmartMediaList)
{
  NS_ENSURE_ARG_POINTER(aSmartMediaList);
  NS_ENSURE_TRUE(mTargetLibrary, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);
  nsresult rv;

  nsCOMPtr<sbIMediaItem> mediaItem = do_QueryInterface(aSmartMediaList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbIMediaItem> targetListAsItem;
  rv = GetSyncItemInLibrary(mediaItem,
                            mTargetLibrary,
                            getter_AddRefs(targetListAsItem));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool shouldSync;
  rv = ShouldMediaListSync(mDevice, aSmartMediaList, &shouldSync);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the item to device library if the list should be synced.
  if (shouldSync && !targetListAsItem) {
    PRBool shouldAdd;
    rv = ShouldAddMediaItem(mTargetLibrary, nsnull, mediaItem,
                            mDevice, &shouldAdd);
    NS_ENSURE_SUCCESS(rv, rv);

    if (shouldAdd) {
      rv = mTargetLibrary->Add(mediaItem);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}
