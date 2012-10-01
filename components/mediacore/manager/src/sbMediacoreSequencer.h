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

#include <sbIMediacoreSequencer.h>

#include <nsIClassInfo.h>
#include <nsIMutableArray.h>
#include <nsIStringEnumerator.h>
#include <nsITimer.h>
#include <nsIURI.h>
#include <nsIWeakReference.h>

#include <nsCOMPtr.h>
#include <nsHashKeys.h>
#include <nsTHashtable.h>
#include <prmon.h>

#include <sbIDataRemote.h>
#include <sbIMediacoreEventListener.h>
#include <sbIMediacoreManager.h>
#include <sbIMediacoreStatus.h>
#include <sbIMediacoreSequenceGenerator.h>
#include <sbIMediaListListener.h>
#include <sbIMediaListView.h>
#include <sbIPropertyManager.h>
#include <sbIMediaItemController.h>

#include <vector>
#include <map>

class nsAutoMonitor;

class sbMediacoreSequencer : public sbIMediacoreSequencer,
                             public sbIMediacoreEventListener,
                             public sbIMediacoreStatus,
                             public sbIMediaListListener,
                             public sbIMediaListViewListener,
                             public sbIMediaItemControllerListener,
                             public nsIClassInfo,
                             public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS

  NS_DECL_SBIMEDIACORESEQUENCER
  NS_DECL_SBIMEDIACOREEVENTLISTENER
  NS_DECL_SBIMEDIACORESTATUS
  NS_DECL_SBIMEDIALISTLISTENER
  NS_DECL_SBIMEDIALISTVIEWLISTENER
  NS_DECL_SBIMEDIAITEMCONTROLLERLISTENER
  NS_DECL_NSICLASSINFO
  NS_DECL_NSITIMERCALLBACK

  sbMediacoreSequencer();

  typedef std::vector<PRUint32> sequence_t;
  typedef std::map<PRUint32, PRUint32> sequencemap_t;

  nsresult Init();

  // Sequence Processor (timer driven)
  nsresult StartSequenceProcessor();
  nsresult StopSequenceProcessor();

  // DataRemotes
  nsresult BindDataRemotes();
  nsresult UnbindDataRemotes();

  // Faceplate Playback Status DataRemotes
  nsresult UpdatePlayStateDataRemotes();
  nsresult UpdatePositionDataRemotes(PRUint64 aPosition);
  nsresult UpdateDurationDataRemotes(PRUint64 aDuration);
  nsresult UpdateURLDataRemotes(nsIURI *aURI);
  nsresult UpdateShuffleDataRemote(PRUint32 aMode);
  nsresult UpdateRepeatDataRemote(PRUint32 aRepeatMode);
  nsresult ResetPlayingVideoDataRemote();

  // Volume & Mute
  nsresult HandleVolumeChangeEvent(sbIMediacoreEvent *aEvent);
  nsresult UpdateVolumeDataRemote(PRFloat64 aVolume);
  nsresult HandleMuteChangeEvent(sbIMediacoreEvent *aEvent);
  nsresult UpdateMuteDataRemote(bool aMuted);

  // Metadata Event & DataRemote
  nsresult HandleMetadataEvent(sbIMediacoreEvent *aEvent);
  nsresult SetMetadataDataRemote(const nsAString &aId,
                                 const nsAString &aValue);
  nsresult SetMetadataDataRemotesFromItem(
          sbIMediaItem *aItem,
          sbIPropertyArray *aPropertiesChanged = nsnull);
  nsresult ResetMetadataDataRemotes();

  nsresult UpdateCurrentItemDuration(PRUint64 aDuration);

  nsresult ResetPlayerControlDataRemotes();

  // Error Event
  nsresult HandleErrorEvent(sbIMediacoreEvent *aEvent);

  // Sequence management
  nsresult RecalculateSequence(PRInt64 *aViewPosition = nsnull);

  // Fetching of items, item manipulation.
  nsresult GetItem(const sequence_t &aSequence,
                   PRUint32 aPosition,
                   sbIMediaItem **aItem);

  // Setup for playback
  nsresult ProcessNewPosition();
  nsresult Setup(nsIURI *aURI = nsnull);
  nsresult CoreHandleNextSetup();
  bool   HandleAbort();

  // Set view with optional view position
  nsresult SetViewWithViewPosition(sbIMediaListView *aView,
                                   PRInt64 *aViewPosition = nsnull);

  // Timer handlers
  nsresult HandleSequencerTimer(nsITimer *aTimer);
  nsresult HandleDelayedCheckTimer(nsITimer *aTimer);

  nsresult StartWatchingView();
  nsresult StopWatchingView();

  nsresult DelayedCheck();
  nsresult UpdateItemUIDIndex();

protected:
  virtual ~sbMediacoreSequencer();

  nsresult DispatchMediacoreEvent(sbIMediacoreEvent *aEvent,
                                  bool aAsync = PR_FALSE);

  nsresult StartPlayback();

  bool   CheckPropertiesInfluenceView(sbIPropertyArray *aProperties);

  bool IsPropertyInPropertyArray(sbIPropertyArray *aPropArray,
                                   const nsAString &aPropName);


  /**
   * Update the "lastPosition" property on the item, to support resuming
   * playback from where things left off
   */
  nsresult UpdateLastPositionProperty(sbIMediaItem* aItem,
                                      nsIVariant*   aData);

  /**
   * Asynchronous callback to seek
   */
  nsresult SeekCallback(PRUint64 aPosition);

  /**
   * Helper function to cleanly stop the playback. The monitor must be locked
   * when this function is called.
   */
  nsresult StopPlaybackHelper(nsAutoMonitor& aMonitor);

  /**
   * Checks for a mediaitemcontroller for the current item, and
   * return true or false depending on whether the item should
   * be skipped by the sequencer
   */
  nsresult ValidateMediaItemControllerPlayback(bool aFromUserAction, 
                                               PRInt32 aOnHoldStatus,
                                               bool *_proceed);

protected:
  PRMonitor *mMonitor;

  PRUint32                       mStatus;
  PRPackedBool                   mIsWaitingForPlayback;
  PRPackedBool                   mSeenPlaying;
  PRPackedBool                   mNextTriggeredByStreamEnd;
  PRPackedBool                   mStopTriggeredBySequencer;
  PRPackedBool                   mCoreWillHandleNext;
  PRPackedBool                   mPositionInvalidated;
  PRUint32                       mErrorCount;

  PRPackedBool                   mCanAbort;
  PRPackedBool                   mShouldAbort;

  PRUint32                       mChainIndex;
  nsCOMPtr<nsIArray>             mChain;

  nsCOMPtr<sbIMediacore>                mCore;
  nsCOMPtr<sbIMediacorePlaybackControl> mPlaybackControl;

  PRUint32                       mMode;
  PRUint32                       mRepeatMode;

  nsCOMPtr<sbIMediaListView>     mView;
  sequence_t                     mSequence;
  sequencemap_t                  mViewIndexToSequenceIndex;
  PRUint32                       mPosition;
  PRUint32                       mViewPosition;

  nsCOMPtr<sbIMediacoreSequenceGenerator> mCustomGenerator;
  nsCOMPtr<sbIMediacoreSequenceGenerator> mShuffleGenerator;

  nsCOMPtr<nsIWeakReference> mMediacoreManager;

  nsCOMPtr<sbIPropertyManager> mPropertyManager;

  // Data Remotes
  nsCOMPtr<sbIDataRemote> mDataRemoteFaceplateBuffering;
  nsCOMPtr<sbIDataRemote> mDataRemoteFaceplatePaused;
  nsCOMPtr<sbIDataRemote> mDataRemoteFaceplatePlaying;
  nsCOMPtr<sbIDataRemote> mDataRemoteFaceplatePlayingVideo;
  nsCOMPtr<sbIDataRemote> mDataRemoteFaceplateSeenPlaying;
  nsCOMPtr<sbIDataRemote> mDataRemoteFaceplateURL;
  nsCOMPtr<sbIDataRemote> mDataRemoteFaceplateVolume;
  nsCOMPtr<sbIDataRemote> mDataRemoteFaceplateMute;

  nsCOMPtr<sbIDataRemote> mDataRemoteFaceplateRemainingTime;

  nsCOMPtr<sbIDataRemote> mDataRemoteMetadataAlbum;
  nsCOMPtr<sbIDataRemote> mDataRemoteMetadataArtist;
  nsCOMPtr<sbIDataRemote> mDataRemoteMetadataTitle;
  nsCOMPtr<sbIDataRemote> mDataRemoteMetadataGenre;

  nsCOMPtr<sbIDataRemote> mDataRemoteMetadataDuration;
  nsCOMPtr<sbIDataRemote> mDataRemoteMetadataDurationStr;
  nsCOMPtr<sbIDataRemote> mDataRemoteMetadataPosition;
  nsCOMPtr<sbIDataRemote> mDataRemoteMetadataPositionStr;

  nsCOMPtr<sbIDataRemote> mDataRemoteMetadataURL;
  nsCOMPtr<sbIDataRemote> mDataRemoteMetadataImageURL;

  nsCOMPtr<sbIDataRemote> mDataRemotePlaylistShuffle;
  nsCOMPtr<sbIDataRemote> mDataRemotePlaylistRepeat;

  nsCOMPtr<sbIDataRemote> mDataRemotePlaylistShuffleDisabled;
  nsCOMPtr<sbIDataRemote> mDataRemotePlaylistRepeatDisabled;
  nsCOMPtr<sbIDataRemote> mDataRemotePlaylistPreviousDisabled;
  nsCOMPtr<sbIDataRemote> mDataRemotePlaylistNextDisabled;

  nsCOMPtr<nsITimer> mSequenceProcessorTimer;

  // MediaListListener and ViewListener data.
  nsCOMPtr<nsITimer> mDelayedCheckTimer;
  nsCOMPtr<sbIMediaList> mViewList;

  nsString mCurrentItemUID;
  PRUint32 mCurrentItemIndex;
  nsCOMPtr<sbIMediaItem> mCurrentItem;

  PRInt32 mListBatchCount;
  PRInt32 mLibraryBatchCount;
  PRInt32 mSmartRebuildDetectBatchCount;

  PRPackedBool mResetPosition;
  PRPackedBool mNoRecalculate;
  PRPackedBool mViewIsLibrary;
  PRPackedBool mNeedSearchPlayingItem;
  PRPackedBool mNeedsRecalculate;
  PRPackedBool mWatchingView;
  PRPackedBool mResumePlaybackPosition;
  PRPackedBool mValidationComplete;
  PRUint32     mOnHoldStatus;
  enum {
    ONHOLD_NOTONHOLD = 0,
    ONHOLD_PLAYVIEW,
    ONHOLD_NEXT,
    ONHOLD_PREVIOUS
  };
  nsCOMPtr<sbIMediaItem> mValidatingItem;
  bool mValidationFromUserAction;
};

class sbScopedBoolToggle
{
public:
  explicit
  sbScopedBoolToggle(PRPackedBool *aBool, bool aValue = PR_TRUE) {
    mBool = aBool;
    *mBool = aValue;
  }
  ~sbScopedBoolToggle() {
    if(mBool) {
      *mBool = !(*mBool);
    }
  }

private:
  PRPackedBool *mBool;
};

