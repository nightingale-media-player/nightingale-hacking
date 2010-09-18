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

#ifndef __SB_PLAYQUEUESERVICE_H__
#define __SB_PLAYQUEUESERVICE_H__

/**
 * \file sbPlayQueueService.h
 * \brief Songbird Play Queue Service Component Definition.
 */

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsTArray.h>
#include <nsIObserver.h>
#include <nsIGenericFactory.h>
#include <nsIWeakReference.h>
#include <nsTHashtable.h>
#include <nsHashKeys.h>

#include <sbILibrary.h>
#include <sbIMediacoreEvent.h>
#include <sbIMediacoreEventListener.h>
#include <sbIMediacoreManager.h>
#include <sbIMediaListListener.h>
#include <sbIPlayQueueService.h>
#include <sbLibraryUtils.h>

#include "sbPlayQueueLibraryListener.h"

class sbIMediaItem;
class sbIMediaList;

class sbPlayQueueService : public sbIPlayQueueService,
                           public sbIMediaListListener,
                           public sbIMediacoreEventListener,
                           public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIPLAYQUEUESERVICE
  NS_DECL_SBIMEDIALISTLISTENER
  NS_DECL_SBIMEDIACOREEVENTLISTENER
  NS_DECL_NSIOBSERVER

  sbPlayQueueService();
  virtual ~sbPlayQueueService();

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo* aInfo);

  nsresult Init();

private:

  void Finalize();

  /**
   * \brief The underlying play queue media list.
   */
  nsCOMPtr<sbIMediaList> mMediaList;

  /**
   * \brief The play queue library.
   */
  nsCOMPtr<sbILibrary> mLibrary;

  /**
   * \brief The index into mMediaList of the 'current' item in the play queue.
   *        This should only be set through SetIndex() so that listeners are
   *        updated.
   */
  PRUint32 mIndex;

  /**
   * \brief True if the service is fully initialized and Finalize() has not been
   *        called yet.
   */
  PRBool mInitialized;

  /**
   * \brief A helper index to help track changes to mIndex during batch
   *        additions and removals to/from mMediaList.
   */
  PRUint32 mBatchRemovalIndex;

  /**
   * \brief Keeps track of whether or not all items in the queue were 'history'
   *        items at the beginning of a batch.
   */
  PRBool mBatchBeginAllHistory;

  /**
   * \brief When true, the sbIMediaListListener notifications are ignored.
   */
  PRBool mIgnoreListListener;

  /**
   * \brief True if the sequencer is on a view of mMediaList. False if the
   *        sequencer is on any other view or not on a view.
   */
  PRBool mSequencerOnQueue;

  /** \brief True of the sequencer is on a view of mMediaList AND the sequencer is
   *         in a playing or paused state.
   */
  PRBool mSequencerPlayingOrPaused;

  /**
   * \brief Helper for batch operations on mMediaList.
   */
  sbLibraryBatchHelper mBatchHelper;

  /**
   * \brief An array to keep track of media item guids that have been removed
   *        from mMediaList but have not been removed from mLibrary.
   */
  nsTArray<nsString> mRemovedItemGUIDs;

  /**
   * \brief The listener for mLibrary.
   */
  nsRefPtr<sbPlayQueueLibraryListener> mLibraryListener;

  /**
   * \brief Weak reference to the mediacore manager.
   */
  nsCOMPtr<nsIWeakReference> mWeakMediacoreManager;

  /**
   * \brief Insert aMediaItem into mMediaList directly before aInsertBeforeIndex 
   */
  nsresult QueueNextInternal(sbIMediaItem* aMediaItem,
                             PRUint32      aInsertBeforeIndex);

  /**
   * \brief Insert all contents of aMediaList into mMediaList directly before
   *        aInsertBeforeIndex.
   */
  nsresult QueueNextInternal(sbIMediaList* aMediaList,
                             PRUint32      aInsertBeforeIndex);

  /**
   * \brief Add aMediaItem to the end of the queue.
   */
  nsresult QueueLastInternal(sbIMediaItem* aMediaItem);

  /**
   * \brief Add all contents of aMediaList to the end of the queue.
   */
  nsresult QueueLastInternal(sbIMediaList* aMediaList);

  nsresult RefreshIndexFromView();

  /**
   * \brief Creates a mediaList for the queue and sets the new list's guid as a
   *        property on the play queue library. This should be the only
   *        mediaList in the play queue library, and this method should only be
   *        called if the queue's mediaList does not exist.
   */
  nsresult CreateMediaList();

  /**
   * \brief Initialization procedure to get the reference to the queue library.
   */
  nsresult InitLibrary();

  /**
   * \brief Initialization procedure to get the reference to the mediaList.
   */
  nsresult InitMediaList();

  /**
   * \brief Sets mIndex to the index of the currently playing track if the
   *        sequencer is on a view of mMediaList. Otherwise, does nothing.
   */
  nsresult SetIndexToPlayingTrack();

  /**
   * \brief Event handler for sbIMediacoreEvent::VIEW_CHANGE. Sets
   *        mSequencerOnQueue based on the new mediaListView.
   */
  nsresult OnViewChange(sbIMediacoreEvent* aEvent);

  /**
   * \brief Event handler for sbIMediacoreEvent::TRACK_CHANGE. This method
   *        should only be called if the sequencer is on a view of mMediaList.
   */
  nsresult OnTrackChange(sbIMediacoreEvent* aEvent);

  /**
   * \brief Event handler for sbIMediacoreEvent::TRACK_INDEX_CHANGE. This method
   *        should only be called if the sequencer is on a view of mMediaList.
   */
  nsresult OnTrackIndexChange(sbIMediacoreEvent* aEvent);

  /**
   * \brief Listeners to be updated when the index changes.
   */
  nsTHashtable<nsISupportsHashKey> mListeners;

  // This callback is meant to be used with mListeners.
  // aUserData should be a sbIPlayQueueServiceListener pointer.
  static PLDHashOperator PR_CALLBACK
    OnIndexUpdatedCallback(nsISupportsHashKey* aKey,
                            void* aUserData);
};

#define SB_PLAYQUEUESERVICE_CONTRACTID                                         \
  "@songbirdnest.com/Songbird/playqueue/service;1"
#define SB_PLAYQUEUESERVICE_CLASSNAME                                          \
  "Songbird Play Queue Service"
#define SB_PLAYQUEUESERVICE_CID                                                \
  { 0xe6407d63, 0x33d2, 0x402e,                                                \
    { 0xa1, 0x66, 0x3d, 0x6b, 0xc7, 0x85, 0xa4, 0x8e } }

#endif /* __SB_PLAYQUEUESERVICE_H__ */
