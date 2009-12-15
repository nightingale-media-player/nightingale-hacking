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

/**
 * \file Listener helpers for sbDeviceLibrary
 */
#ifndef __SBDEVICELIBRARYHELPERS_H__
#define __SBDEVICELIBRARYHELPERS_H__

#include <nsAutoPtr.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>

#include <sbIMediaListListener.h>

class nsIArray;
class sbILibrary;
class sbIMediaList;

class sbDeviceLibrary;

/**
 * Listens to events for a playlist and then performs a sync
 * on the playlist
 */
class sbPlaylistSyncListener : public sbIMediaListListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTLISTENER

  sbPlaylistSyncListener(sbILibrary* aTargetLibrary,
                         bool aSyncPlaylists);

  /**
   * Add the media list to our list of lists
   * TODO: XXX hack to keep the playlists from going away and our listeners from going deaf
   */
  nsresult AddMediaList(sbIMediaList * aList) {
    NS_ASSERTION(mMediaLists.IndexOf(aList) == -1, "Media list already being listened to");
    return mMediaLists.AppendObject(aList);
  }

  void StopListeningToPlaylists();

  nsresult SetSyncPlaylists(bool aManualMode,
                            nsIArray * aMediaLists);
protected:
  virtual ~sbPlaylistSyncListener();

  /**
   * The device owns us and device library owns the device so we're good.
   * non owning pointer
   */
  sbILibrary * mTargetLibrary;
  bool mSyncPlaylists;

  /**
   * TODO: XXX hack to keep the playlists from going away and our listeners from going deaf
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
                          bool aSyncPlaylists,
                          nsIArray * aPlaylistsList,
                          bool aIgnorePlaylists);

  /**
   * Determine if we should listen to a playlist
   * \param aMainList this is the playlist on the main library
   * \param aShouldListen out parameter that denotes whether the
   *        playlist should be listened to
   */
  nsresult ShouldListenToPlaylist(sbIMediaList * aMainList,
                                  PRBool & aShouldListen);

  /**
   * This function adds a listener to the playlist aMainMediaList
   * \param aMainMediaList The media list to listen to, must be in the mTargetLibrary
   * \param aDeviceLibrary the device library
   */
  nsresult ListenToPlaylist(sbIMediaList * aMainMediaList);

  /**
   * Stops listening to the playlists
   */
  void StopListeningToPlaylists()
  {
    mPlaylistListener->StopListeningToPlaylists();
  }

  /**
   * Sets the management type for the listner
   * \param aMgmtType A management type constant
   */
  void SetSyncMode(bool aManualMode,
                   nsIArray * aPlaylistsList);
protected:
  /**
   * The target library holds a reference to us (to unregister), so it is
   * safe to not own a reference
   */
  sbILibrary* mTargetLibrary;
  nsCOMPtr<nsIArray> mPlaylistsList;

  nsRefPtr<sbPlaylistSyncListener> mPlaylistListener;

  bool mManualMode;
  bool const mSyncPlaylists;
  bool const mIgnorePlaylists;
};

#endif
