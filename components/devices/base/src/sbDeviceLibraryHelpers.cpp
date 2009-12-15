/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// END SONGBIRD GPL
//
*/
/*** sbLibraryUpdateListener ***/

#include "sbDeviceLibraryHelpers.h"

#include <nsArrayUtils.h>
#include <nsCOMPtr.h>

#include <sbIOrderableMediaList.h>
#include <sbIPropertyArray.h>

#include <sbIDevice.h>
#include "sbDeviceLibrary.h"

#include <sbLibraryUtils.h>
#include <sbStandardProperties.h>

static nsresult GetSyncItemInLibrary(sbIMediaItem*  aMediaItem,
                                     sbILibrary*    aTargetLibrary,
                                     sbIMediaItem** aSyncItem);

// note: this isn't actually threadsafe, but may be used on multiple threads
NS_IMPL_THREADSAFE_ISUPPORTS1(sbLibraryUpdateListener, sbIMediaListListener)

sbLibraryUpdateListener::sbLibraryUpdateListener(sbILibrary * aTargetLibrary,
                                                 bool aManualMode,
                                                 bool aSyncPlaylists,
                                                 nsIArray * aPlaylistsList,
                                                 bool aIgnorePlaylists)
  : mTargetLibrary(aTargetLibrary),
    mPlaylistListener(new sbPlaylistSyncListener(aTargetLibrary, aSyncPlaylists)),
    mIgnorePlaylists(aIgnorePlaylists),
    mSyncPlaylists(aSyncPlaylists)
{
  SetSyncMode(aManualMode, aPlaylistsList);
}

void sbLibraryUpdateListener::SetSyncMode(bool aManualMode,
                                          nsIArray * aPlaylistsList) {
  NS_ASSERTION(!mSyncPlaylists && aPlaylistsList != nsnull,
               "Sync Management type isn't playlists but playlists were supplied");
  NS_ASSERTION(mSyncPlaylists && aPlaylistsList == nsnull,
               "Sync Management type is playlists but no playlists were supplied");
  mPlaylistsList = aPlaylistsList;
  mManualMode = aManualMode;
  mPlaylistListener->SetSyncPlaylists(aManualMode, aPlaylistsList);
}

nsresult sbLibraryUpdateListener::ShouldListenToPlaylist(sbIMediaList * aMainList,
                                                         PRBool & aShouldListen)
{
  NS_PRECONDITION(aMainList, "aMainList cannot be null");
  NS_ENSURE_TRUE(mTargetLibrary, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPlaylistListener, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv;
#if DEBUG
  nsCOMPtr<sbILibrary> listLib;
  rv = aMainList->GetLibrary(getter_AddRefs(listLib));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> mainLib;
  rv = GetMainLibrary(getter_AddRefs(mainLib));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool equals;
  NS_ASSERTION(NS_SUCCEEDED(mainLib->Equals(listLib, &equals)) && equals,
               "ShouldListenToPlaylist called with a non-mainlibrary playlist");

#endif

  PRBool listenToChanges = !mManualMode;
  // If we're supposed to be listening and only interested in certain playlists
  // check that list
  if (mSyncPlaylists) {

    listenToChanges = PR_FALSE;

    nsAutoString playlistItemGuid;
    nsCOMPtr<sbIMediaItem> playlist;

    NS_ASSERTION(mPlaylistsList, "Management type is not ALL and mPlaylistsList is null");
    // See if the playlist is in the list of playlists to sync
    PRUint32 length;
    rv = mPlaylistsList->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 index = 0; index < length; ++index) {
      playlist = do_QueryElementAt(mPlaylistsList, index, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      // If we found it, indicate that we want to listen for changes to
      // this playlist
      PRBool equals;
      nsresult rv = aMainList->Equals(playlist, &equals);
      if (NS_SUCCEEDED(rv) && equals) {
        listenToChanges = PR_TRUE;
        break;
      }
    }
  }
  aShouldListen = listenToChanges;
  return NS_OK;
}

nsresult sbLibraryUpdateListener::ListenToPlaylist(sbIMediaList * aMainMediaList)
{
  NS_ENSURE_ARG_POINTER(aMainMediaList);
  NS_ENSURE_TRUE(mTargetLibrary, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPlaylistListener, NS_ERROR_OUT_OF_MEMORY);

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

  mPlaylistListener->AddMediaList(aMainMediaList);

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
  NS_ENSURE_TRUE(mPlaylistListener, NS_ERROR_OUT_OF_MEMORY);

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
    // Ignore if item is hidden. We'll pick it up when it's unhidden
    nsString hidden;
    nsresult rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                                          hidden);
    if (!hidden.EqualsLiteral("1")) {
      rv = mTargetLibrary->Add(aMediaItem);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    // See if this is a playlist being added, if so we need to listen to it
    if (list) {
      PRBool shouldListen;
      rv = ShouldListenToPlaylist(list, shouldListen);
      NS_ENSURE_SUCCESS(rv, rv);

      if (shouldListen) {
        ListenToPlaylist(list);
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
  nsCOMPtr<sbIMediaItem> targetListAsItem;
  rv = GetSyncItemInLibrary(aMediaItem,
                            mTargetLibrary,
                            getter_AddRefs(targetListAsItem));
  NS_ENSURE_SUCCESS(rv, rv);
  if (targetListAsItem) {
    rv = mTargetLibrary->Remove(targetListAsItem);
    NS_ENSURE_SUCCESS(rv, rv);

    if (list) {
      // Remove it, we don't care about errors
      rv = list->RemoveListener(mPlaylistListener);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else
    NS_WARNING("sbLibraryUpdateListener::OnBeforeItemRemoved - Item doesn't exist");

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
            bool & aIsHidden,
            bool & aIsUnhidden)
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

  // If this is a list and we're ignore lists then just leave
  nsCOMPtr<sbIMediaList> list = do_QueryInterface(aMediaItem);
  if (list && mIgnorePlaylists) {
    return NS_OK;
  }
  nsresult rv;

  // the property array here are the old values; we need the new ones
  nsCOMPtr<sbIPropertyArray> newProps;
  rv = aMediaItem->GetProperties(aProperties, getter_AddRefs(newProps));
  NS_ENSURE_SUCCESS(rv, rv);

  // If this item is being unhidden then we'll need to add to the device lib
  bool isHidden;
  bool isUnhidden;
  rv = GetHideState(aProperties, newProps, isHidden, isUnhidden);
  NS_ENSURE_SUCCESS(rv, rv);

  // It's no longer hidden so add it
  if (isUnhidden) {
    rv = mTargetLibrary->Add(aMediaItem);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (isHidden) {
    rv = mTargetLibrary->Remove(aMediaItem);
    NS_ENSURE_SUCCESS(rv, rv);
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

sbPlaylistSyncListener::sbPlaylistSyncListener(sbILibrary* aTargetLibrary,
                                               bool aSyncPlaylists)
: mTargetLibrary(aTargetLibrary),
  mSyncPlaylists(aSyncPlaylists)
{
  NS_ASSERTION(aTargetLibrary,
               "sbPlaylistSyncListener cannot be given a null aTargetLibrary");
}

sbPlaylistSyncListener::~sbPlaylistSyncListener()
{
  StopListeningToPlaylists();
}

void sbPlaylistSyncListener::StopListeningToPlaylists() {
  PRUint32 length = mMediaLists.Count();
  for (PRUint32 index = 0; index < length; ++index) {
    mMediaLists[index]->RemoveListener(this);
  }
  mMediaLists.Clear();
}

nsresult sbPlaylistSyncListener::SetSyncPlaylists(bool aManualMode,
                                                  nsIArray * aMediaLists) {
  nsresult rv;

  PRUint32 length = 0;
  if (aMediaLists) {
    rv = aMediaLists->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mSyncPlaylists = length != 0 && !aManualMode;
  if (mSyncPlaylists) {
    mMediaLists.Clear();
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
    nsCOMPtr<sbIMediaItem> deviceMediaListAsItem;
    rv = GetSyncItemInLibrary(aMediaList,
                              mTargetLibrary,
                              getter_AddRefs(deviceMediaListAsItem));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(deviceMediaListAsItem, NS_ERROR_FAILURE);

    nsCOMPtr<sbIMediaList> deviceMediaList =
      do_QueryInterface(deviceMediaListAsItem, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Null case is handled
    nsCOMPtr<sbIOrderableMediaList> orderedList =
      do_QueryInterface(deviceMediaListAsItem);

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

/**
 * This helper function determines if an item exists in the list of media lists
 * excluding the the media list to be ignored
 * \param aLibrary The library to get to the corresponding playlists from
 * \param aMediaLists The list of playlists to check
 * \param aIgnoreThisMediaList The media list we want to skip, because it's the
 *                             media list we found the item in
 * \param aMediaItem The media item to look for. The item will be on found in
 *                   aLibrary.
 * \param aExistsInAnotherList Out parameter that receives true if the media
 *                             item exists in another playlist
 */
static nsresult
IsItemInAnotherPlaylist(sbILibrary * aLibrary,
                        nsCOMArray<sbIMediaList> & aMediaLists,
                        sbIMediaList * aIgnoreThisMediaList,
                        sbIMediaItem * aMediaItem,
                        bool & aExistsInAnotherList)
{
  nsresult rv;

  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(aIgnoreThisMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);

  // See if the media item is in another list
  aExistsInAnotherList = false;
  PRUint32 const length = aMediaLists.Count();
  for (PRUint32 index = 0; index < length && !aExistsInAnotherList; ++index) {
    // Get the media list
    sbIMediaList * mediaList = aMediaLists.ObjectAt(index);

    // Get the corresponding one in the provided library
    nsCOMPtr<sbIMediaItem> mediaListAsItem;
    rv = GetSyncItemInLibrary(mediaList,
                              aLibrary,
                              getter_AddRefs(mediaListAsItem));
    NS_ENSURE_SUCCESS(rv, rv);

    // Ge the media list interface for the media list now
    nsCOMPtr<sbIMediaList> deviceMediaList =
      do_QueryInterface(mediaListAsItem, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Skip the media list if this is the one we're clearing
    PRBool same;
    rv = aIgnoreThisMediaList->Equals(deviceMediaList, &same);
    if (NS_FAILED(rv)) {
      NS_WARNING("Unable to compare media lists");
      continue;
    }
    if (same) {
      continue;
    }

    // Does the item exist in this media list, if so set flag
    PRBool contains;
    rv = deviceMediaList->Contains(aMediaItem, &contains);
    NS_ENSURE_SUCCESS(rv, rv);

    if (contains) {
      aExistsInAnotherList = true;
    }
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
  NS_ASSERTION(NS_SUCCEEDED(debugItem->Equals(targetListAsItem,
                                               &debugEquals)) && debugEquals,
                            "Item not the correct item in the playlist");
#endif

  // If we're sync'ing playlists we may need to remove the items
  if (mSyncPlaylists) {
    bool exists = false;

    // If there is more than one of this item in the list we'll need to keep it
    PRUint32 itemIndex;
    rv = deviceMediaList->IndexOf(deviceMediaItem, 0, &itemIndex);
    if (NS_SUCCEEDED(rv)) {
      rv = deviceMediaList->IndexOf(deviceMediaItem, itemIndex + 1, &itemIndex);
      exists = NS_SUCCEEDED(rv);
    }

    // If there was only one in the list, check other media lists
    if (!exists) {
      // Check to see if this is the only instance of this item
      rv = IsItemInAnotherPlaylist(mTargetLibrary,
                                   mMediaLists,
                                   deviceMediaList,
                                   deviceMediaItem,
                                   exists);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // If this is the only instance of this media item remove it from the
    // device
    if (!exists) {
      mTargetLibrary->Remove(deviceMediaItem);
    }
  }

  rv = deviceMediaList->RemoveByIndex(aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

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

/**
 * Enumerator used to remove items from a library that exist in a playlist but
 * not in any other sync'd playlist
 */
class sbDeviceSyncPlaylistRemover : public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS;

  /**
   * Initializes the enumerator with the device and playlist collection to read
   */
  sbDeviceSyncPlaylistRemover(sbILibrary * aLibrary,
                              nsCOMArray<sbIMediaList> & aPlaylists) :
                                  mLibrary(aLibrary),
                                  mPlaylists(aPlaylists)
  {
  }
  /**
   * \brief Called when enumeration is about to begin.
   *
   * \param aMediaList - The media list that is being enumerated.
   *
   * \return CONTINUE to continue enumeration, CANCEL to cancel enumeration.
   *         JavaScript callers may omit the return statement entirely to
   *         continue the enumeration.
   */
  /* unsigned short onEnumerationBegin (in sbIMediaList aMediaList); */
  NS_SCRIPTABLE NS_IMETHOD OnEnumerationBegin(sbIMediaList *aMediaList,
                                              PRUint16 *_retval) {
    NS_ENSURE_STATE(mLibrary);
    NS_ENSURE_ARG_POINTER(aMediaList);
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = CONTINUE;
    return NS_OK;
  }

  /**
   * \brief Called once for each item in the enumeration.
   *
   * \param aMediaList - The media list that is being enumerated.
   * \param aMediaItem - The media item.
   *
   * \return CONTINUE to continue enumeration, CANCEL to cancel enumeration.
   *         JavaScript callers may omit the return statement entirely to
   *         continue the enumeration.
   */
  /* unsigned short onEnumeratedItem (in sbIMediaList aMediaList, in sbIMediaItem aMediaItem); */
  NS_SCRIPTABLE NS_IMETHOD OnEnumeratedItem(sbIMediaList *aMediaList,
                                            sbIMediaItem *aMediaItem,
                                            PRUint16 *_retval) {

    NS_ENSURE_STATE(mLibrary);
    NS_ENSURE_ARG_POINTER(aMediaList);
    NS_ENSURE_ARG_POINTER(aMediaItem);
    NS_ENSURE_ARG_POINTER(_retval);

    nsresult rv;

    // See if the media item is in another list
    bool exists = false;
    rv = IsItemInAnotherPlaylist(mLibrary,
                                 mPlaylists,
                                 aMediaList,
                                 aMediaItem,
                                 exists);
    NS_ENSURE_SUCCESS(rv, rv);

    // ok it doesn't exist in another list, so we can remove it
    if (!exists) {
      mLibrary->Remove(aMediaItem);
    }

    *_retval = CONTINUE;
    return NS_OK;
  }
  /**
   * \brief Called when enumeration has completed.
   *
   * \param aMediaList - The media list that is being enumerated.
   * \param aStatusCode - A code to determine if the enumeration was successful.
   */
  /* void onEnumerationEnd (in sbIMediaList aMediaList, in nsresult aStatusCode); */
  NS_SCRIPTABLE NS_IMETHOD OnEnumerationEnd(sbIMediaList *aMediaList,
                                            nsresult aStatusCode) {
    return NS_OK;
  }
private:
  sbILibrary * mLibrary;
  nsCOMArray<sbIMediaList> & mPlaylists;
};

NS_IMPL_ISUPPORTS1(sbDeviceSyncPlaylistRemover,
                   sbIMediaListEnumerationListener);


NS_IMETHODIMP
sbPlaylistSyncListener::OnListCleared(sbIMediaList *aMediaList,
                                      PRBool aExcludeLists,
                                      PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  // Get the media list that is on the device
  nsresult rv;
  nsCOMPtr<sbIMediaItem> targetListAsItem;
  rv = GetSyncItemInLibrary(aMediaList,
                            mTargetLibrary,
                            getter_AddRefs(targetListAsItem));
  NS_ENSURE_SUCCESS(rv, rv);

  if (targetListAsItem) {
    nsCOMPtr<sbIMediaList> targetList = do_QueryInterface(targetListAsItem,
                                                          &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // If we're sync'ing playlists we may need to remove the items
    if (mSyncPlaylists) {
      // Remove the items from the library, which in turn removes them from
      // the playlist
      nsCOMPtr<sbDeviceSyncPlaylistRemover> enumerator =
        new sbDeviceSyncPlaylistRemover(mTargetLibrary, mMediaLists);
      NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

      rv = targetList->EnumerateAllItems(enumerator,
                                         sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Go ahead and clear out the list, there may be things we didn't delete
    // from the library
    rv = targetList->Clear();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (_retval) {
    *_retval = PR_FALSE; /* don't stop */
  }

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistSyncListener::OnBatchBegin(sbIMediaList *aMediaList)
{
  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistSyncListener::OnBatchEnd(sbIMediaList *aMediaList)
{
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
