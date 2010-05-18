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

/**
 * \file Listener helpers for sbDeviceLibrary
 */
#ifndef __SBDEVICELIBRARYHELPERS_H__
#define __SBDEVICELIBRARYHELPERS_H__

#include <nsAutoPtr.h>
#include <nsClassHashtable.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>

#include <sbILocalDatabaseSmartMediaList.h>
#include <sbIMediaListListener.h>
#include <sbLibraryUtils.h>

class nsIArray;
class sbILibrary;
class sbIMediaList;

class sbDeviceLibrary;

/**
 * Listens to events for a playlist and then performs a sync
 * on the playlist
 */
class sbPlaylistSyncListener : public sbIMediaListListener,
                               public sbILocalDatabaseSmartMediaListListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTLISTENER
  NS_DECL_SBILOCALDATABASESMARTMEDIALISTLISTENER

  sbPlaylistSyncListener(sbILibrary* aTargetLibrary,
                         bool aSyncPlaylists);

  /**
   * Add the media list to our list of lists
   * TODO: XXX hack to keep the playlists from going away and our listeners
   *           from going deaf
   */
  PRBool AddMediaList(sbIMediaList * aList) {
    if (mMediaLists.IndexOf(aList) == -1)
      return mMediaLists.AppendObject(aList);
    return PR_TRUE;
  }

  /**
   * Remove the media list from our list of lists
   */
  PRBool RemoveMediaList(sbIMediaList * aList) {
    if (mMediaLists.IndexOf(aList) == -1)
      return PR_TRUE;
    return mMediaLists.RemoveObject(aList);
  }

  void StopListeningToPlaylists();

  nsresult SetSyncPlaylists(nsIArray * aMediaLists);
protected:
  virtual ~sbPlaylistSyncListener();

  /**
   * Rebuild playlist after items removed.
   */
  nsresult RebuildPlaylistAfterItemRemoved(sbIMediaList *aMediaList);

  /**
   * Handle function for item removal which is not part of batch.
   */
  nsresult RemoveItemNotInBatch(sbIMediaList *aMediaList,
                                sbIMediaItem *aMediaItem,
                                PRUint32 aIndex);

  /**
   * Static batch callback function to add media items to media list.
   */
  static nsresult AddItemsToList(nsISupports* aUserData);

  /**
   * The device owns us and device library owns the device so we're good.
   * non owning pointer
   */
  sbILibrary * mTargetLibrary;
  bool mSyncPlaylists;

  /**
   * Hash table of sbIMediaList -> sbLibraryBatchHelper for playlist batch
   * removal
   */
  nsClassHashtable<nsISupportsHashKey,
                   sbLibraryBatchHelper> mBatchHelperTable;
  // Array of media lists that the removed media items belong to
  nsCOMArray<sbIMediaList> mListRemovedArray;
  // Array of media items removed
  nsCOMArray<sbIMediaItem> mItemRemovedArray;

  /**
   * TODO: XXX hack to keep the playlists from going away and our listeners
   *           from going deaf
   */
  nsCOMArray<sbIMediaList> mMediaLists;
};

/**
 * Listens to item update events to propagate into a second library
 * The target library must register this listener before going away.
 */
class sbLibraryUpdateListener : public sbIMediaListListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTLISTENER

  sbLibraryUpdateListener(sbILibrary * aTargetLibrary,
                          bool aManualMode,
                          nsIArray * aPlaylistsList,
                          bool aIgnorePlaylists);

  /**
   * This function adds listener to the playlist aMainMediaList
   * \param aMainMediaList The media list to listen to, must be in the
   *        mTargetLibrary
   */
  nsresult ListenToPlaylist(sbIMediaList * aMainMediaList);

  /**
   * This function removes listener from the playlist aMainMediaList
   * \param aMainMediaList The media list to remove listener from, must be
   *        in the mTargetLibrary
   */
  nsresult StopListeningToPlaylist(sbIMediaList * aMainMediaList);

  /**
   * Stops listening to the playlists
   */
  void StopListeningToPlaylists()
  {
    mPlaylistListener->StopListeningToPlaylists();
  }

  /**
   * Sets the management type for the listner
   * \param aManualMode    Whether to set to manual mode
   * \param aPlaylistsList The playlist list to sync to
   */
  void SetSyncMode(bool aManualMode,
                   nsIArray * aPlaylistsList);
protected:
  virtual ~sbLibraryUpdateListener();

  /**
   * The target library holds a reference to us (to unregister), so it is
   * safe to not own a reference
   */
  sbILibrary* mTargetLibrary;

  nsRefPtr<sbPlaylistSyncListener> mPlaylistListener;

  bool mManualMode;
  bool mSyncPlaylists;
  bool mIgnorePlaylists;
};

#endif
