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

#include <sbIDevice.h>
#include <sbIOrderableMediaList.h>
#include <sbIPropertyArray.h>

#include "sbDeviceLibrary.h"

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
                                                 bool aIgnorePlaylists)
  : mTargetLibrary(aTargetLibrary),
    mPlaylistListener(new sbPlaylistSyncListener(aTargetLibrary,
                                                 aPlaylistsList != nsnull)),
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
  mPlaylistList = aPlaylistList;
  mSyncPlaylists = aPlaylistList != nsnull;
  mManualMode = aManualMode;
  mPlaylistListener->SetSyncPlaylists(aPlaylistList);
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

    NS_ASSERTION(mPlaylistList, "Management type is not ALL and mPlaylistList is null");
    // See if the playlist is in the list of playlists to sync
    PRUint32 length;
    rv = mPlaylistList->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 index = 0; index < length; ++index) {
      playlist = do_QueryElementAt(mPlaylistList, index, &rv);
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

  // If this is a list and we're ignoring lists or the list is not a simple
  // list, then just leave
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

  // Initialize the batch helper table.
  if (!mBatchHelperTable.Init()) {
    NS_WARNING("Out of memory!");
  }
}

sbPlaylistSyncListener::~sbPlaylistSyncListener()
{
  mBatchHelperTable.Clear();
  mListRemovedArray.Clear();
  mItemRemovedArray.Clear();
}

void sbPlaylistSyncListener::StopListeningToPlaylists() {
  PRUint32 length = mMediaLists.Count();
  for (PRUint32 index = 0; index < length; ++index) {
    mMediaLists[index]->RemoveListener(this);
  }
  mMediaLists.Clear();
}

nsresult sbPlaylistSyncListener::SetSyncPlaylists(nsIArray * aMediaLists) {
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

    index = mItemRemovedArray.IndexOf(aMediaItem);
    // Media item not found in the array. Append.
    if (index < 0) {
      success = mItemRemovedArray.AppendObject(aMediaItem);
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
                                  mPlaylistList(aPlaylists)
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
                                 mPlaylistList,
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
  nsCOMArray<sbIMediaList> & mPlaylistList;
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
    mItemRemovedArray.Clear();
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

  // If we're sync'ing playlists we may need to remove the items
  if (mSyncPlaylists) {
    nsCOMPtr<sbIMediaItem> deviceMediaItem;
    PRUint32 itemLength = mItemRemovedArray.Count();
    for (PRInt32 i = itemLength - 1; i >= 0; --i) {
      rv = GetSyncItemInLibrary(mItemRemovedArray[i],
                                mTargetLibrary,
                                getter_AddRefs(deviceMediaItem));
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool success = mItemRemovedArray.RemoveObjectAt(i);
      NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

      bool exists = false;

      // If there is still one of this item in the list we'll need to keep it
      PRBool contains;
      rv = deviceMediaList->Contains(deviceMediaItem, &contains);
      NS_ENSURE_SUCCESS(rv, rv);
      exists = contains != PR_FALSE;

      // If no one in the list, check other media lists
      if (!exists) {
        // Check to see if any contains this media item
        rv = IsItemInAnotherPlaylist(mTargetLibrary,
                                     mMediaLists,
                                     deviceMediaList,
                                     deviceMediaItem,
                                     exists);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      // If no list contains this media item, remove it from the device
      if (!exists) {
        mTargetLibrary->Remove(deviceMediaItem);
      }
    }
  }

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
