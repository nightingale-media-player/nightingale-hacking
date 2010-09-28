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

#include "sbMediacoreSequencer.h"

#include <nsIClassInfoImpl.h>
#include <nsIDOMWindow.h>
#include <nsIDOMXULElement.h>
#include <nsIPrefBranch.h>
#include <nsIPrefService.h>
#include <nsIProgrammingLanguage.h>
#include <nsIPropertyBag2.h>
#include <nsISupportsPrimitives.h>
#include <nsIURL.h>
#include <nsIVariant.h>
#include <nsIWeakReferenceUtils.h>
#include <nsIWindowWatcher.h>

#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsArrayUtils.h>
#include <nsComponentManagerUtils.h>

#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <nsTArray.h>

#include <prtime.h>

#include <sbICascadeFilterSet.h>
#include <sbIFilterableMediaListView.h>
#include <sbILibrary.h>
#include <sbILibraryConstraints.h>
#include <sbIMediacore.h>
#include <sbIMediacoreEvent.h>
#include <sbIMediacoreEventTarget.h>
#include <sbIMediacorePlaybackControl.h>
#include <sbIMediacoreStatus.h>
#include <sbIMediacoreVideoWindow.h>
#include <sbIMediacoreVoting.h>
#include <sbIMediacoreVotingChain.h>
#include <sbIMediaItem.h>
#include <sbIMediaItemController.h>
#include <sbIMediaList.h>
#include <sbIPrompter.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyInfo.h>
#include <sbISortableMediaListView.h>
#include <sbIWindowWatcher.h>
#include <sbIMediacoreErrorHandler.h>

#include <sbBaseMediacoreVolumeControl.h>
#include <sbMediacoreError.h>
#include <sbMediacoreEvent.h>
#include <sbMemoryUtils.h>
#include <sbProxiedComponentManager.h>
#include <sbStandardProperties.h>
#include <sbStringBundle.h>
#include <sbStringUtils.h>
#include <sbTArrayStringEnumerator.h>
#include <sbThreadUtils.h>
#include <sbVariantUtils.h>

#include "sbMediacoreDataRemotes.h"
#include "sbMediacoreShuffleSequenceGenerator.h"

// Time in ms between dataremote updates
#define MEDIACORE_UPDATE_NOTIFICATION_DELAY 500

// Time in ms to wait before deferred media list check
#define MEDIACORE_CHECK_DELAY 100

// Number of errors after which to stop flipping to next track
#define MEDIACORE_MAX_SUBSEQUENT_ERRORS 20

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediacoreSequencer:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gMediacoreSequencerLog = nsnull;
#define TRACE(args) PR_LOG(gMediacoreSequencerLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMediacoreSequencerLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

inline nsresult
EmitMillisecondsToTimeString(PRUint64 aValue,
                             nsAString &aString,
                             PRBool aRemainingTime = PR_FALSE)
{
  PRUint64 seconds = aValue / 1000;
  PRUint64 minutes = seconds / 60;
  PRUint64 hours = minutes / 60;

  seconds = seconds %60;
  minutes = minutes % 60;

  NS_NAMED_LITERAL_STRING(strZero, "0");
  NS_NAMED_LITERAL_STRING(strCol, ":");
  nsString stringValue;

  if(hours > 0) {
    AppendInt(stringValue, hours);
    stringValue += strCol;
  }

  if(hours > 0 && minutes < 10) {
    stringValue += strZero;
  }

  AppendInt(stringValue, minutes);
  stringValue += strCol;

  if(seconds < 10) {
    stringValue += strZero;
  }

  AppendInt(stringValue, seconds);

  aString.Truncate();

  if(aRemainingTime) {
    aString.Assign(NS_LITERAL_STRING("-"));
  }

  aString.Append(stringValue);

  return NS_OK;
}

//------------------------------------------------------------------------------
//  sbMediacoreSequencer
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ADDREF(sbMediacoreSequencer)
NS_IMPL_THREADSAFE_RELEASE(sbMediacoreSequencer)

NS_IMPL_QUERY_INTERFACE7_CI(sbMediacoreSequencer,
                            sbIMediacoreSequencer,
                            sbIMediacoreStatus,
                            sbIMediaListListener,
                            sbIMediaListViewListener,
                            sbIMediaItemControllerListener,
                            nsIClassInfo,
                            nsITimerCallback)

NS_IMPL_CI_INTERFACE_GETTER2(sbMediacoreSequencer,
                             sbIMediacoreSequencer,
                             sbIMediacoreStatus)

NS_DECL_CLASSINFO(sbMediacoreSequencer)
NS_IMPL_THREADSAFE_CI(sbMediacoreSequencer)

sbMediacoreSequencer::sbMediacoreSequencer()
: mMonitor(nsnull)
, mStatus(sbIMediacoreStatus::STATUS_STOPPED)
, mIsWaitingForPlayback(PR_FALSE)
, mSeenPlaying(PR_FALSE)
, mNextTriggeredByStreamEnd(PR_FALSE)
, mStopTriggeredBySequencer(PR_FALSE)
, mCoreWillHandleNext(PR_FALSE)
, mPositionInvalidated(PR_FALSE)
, mCanAbort(PR_FALSE)
, mShouldAbort(PR_FALSE)
, mMode(sbIMediacoreSequencer::MODE_FORWARD)
, mRepeatMode(sbIMediacoreSequencer::MODE_REPEAT_NONE)
, mErrorCount(0)
, mPosition(0)
, mViewPosition(0)
, mCurrentItemIndex(0)
, mListBatchCount(0)
, mLibraryBatchCount(0)
, mSmartRebuildDetectBatchCount(0)
, mResetPosition(PR_FALSE)
, mNoRecalculate(PR_FALSE)
, mViewIsLibrary(PR_FALSE)
, mNeedSearchPlayingItem(PR_FALSE)
, mNeedsRecalculate(PR_FALSE)
, mWatchingView(PR_FALSE)
, mResumePlaybackPosition(PR_TRUE)
, mOnHoldStatus(ONHOLD_NOTONHOLD)
, mValidationComplete(PR_FALSE)
{

#ifdef PR_LOGGING
  if (!gMediacoreSequencerLog)
    gMediacoreSequencerLog = PR_NewLogModule("sbMediacoreSequencer");
#endif
}

sbMediacoreSequencer::~sbMediacoreSequencer()
{
  if(mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
  }

  UnbindDataRemotes();
}

nsresult
sbMediacoreSequencer::Init()
{
  NS_ASSERTION(NS_IsMainThread(),
               "sbMediacoreSequencer::Init expected to be on the main thread");
  mMonitor = nsAutoMonitor::NewMonitor("sbMediacoreSequencer::mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsISupportsWeakReference> weakRef =
    do_GetService(SB_MEDIACOREMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = weakRef->GetWeakReference(getter_AddRefs(mMediacoreManager));
  NS_ENSURE_SUCCESS(rv, rv);

  mSequenceProcessorTimer =
    do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = BindDataRemotes();
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbMediacoreShuffleSequenceGenerator> generator;
  NS_NEWXPCOM(generator, sbMediacoreShuffleSequenceGenerator);
  NS_ENSURE_TRUE(generator, NS_ERROR_OUT_OF_MEMORY);

  rv = generator->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mShuffleGenerator = do_QueryInterface(generator, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool shuffle = PR_FALSE;
  rv = mDataRemotePlaylistShuffle->GetBoolValue(&shuffle);
  NS_ENSURE_SUCCESS(rv, rv);

  if(shuffle) {
    mMode = sbIMediacoreSequencer::MODE_SHUFFLE;
  }

  PRInt64 repeatMode = 0;
  rv = mDataRemotePlaylistRepeat->GetIntValue(&repeatMode);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_ARG_RANGE(repeatMode, 0, 2);

  mRepeatMode = repeatMode;

  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool doResume;
  rv = prefs->GetBoolPref("songbird.mediacore.resumePlaybackPosition",
                          &doResume);
  if (NS_SUCCEEDED(rv)) {
    // only set the member we actually check if the pref was retrieved
    mResumePlaybackPosition = doResume ? PR_TRUE : PR_FALSE;
  }

  return NS_OK;
}

nsresult
sbMediacoreSequencer::StartSequenceProcessor()
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mSequenceProcessorTimer, NS_ERROR_NOT_INITIALIZED);

  nsresult rv =
    mSequenceProcessorTimer->InitWithCallback(this,
                                              MEDIACORE_UPDATE_NOTIFICATION_DELAY,
                                              nsITimer::TYPE_REPEATING_SLACK);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = StartWatchingView();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreSequencer::StopSequenceProcessor()
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mSequenceProcessorTimer, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = mSequenceProcessorTimer->Cancel();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdatePositionDataRemotes(0);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateDurationDataRemotes(0);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = StopWatchingView();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreSequencer::BindDataRemotes()
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  //
  // Faceplate DataRemotes
  //

  nsString nullString;
  nullString.SetIsVoid(PR_TRUE);

  // Faceplate Buffering
  mDataRemoteFaceplateBuffering =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplateBuffering->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_FACEPLATE_BUFFERING),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplateBuffering->SetBoolValue(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Faceplate Paused
  mDataRemoteFaceplatePaused =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplatePaused->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_FACEPLATE_PAUSED),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplatePaused->SetBoolValue(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Faceplate Playing
  mDataRemoteFaceplatePlaying =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplatePlaying->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_FACEPLATE_PLAYING),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplatePlaying->SetBoolValue(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Faceplate Playing Video
  mDataRemoteFaceplatePlayingVideo =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplatePlayingVideo->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_FACEPLATE_PLAYINGVIDEO),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplatePlayingVideo->SetBoolValue(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Faceplate Seen Playing
  mDataRemoteFaceplateSeenPlaying =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplateSeenPlaying->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_FACEPLATE_SEENPLAYING),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplateSeenPlaying->SetBoolValue(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  //Faceplate Play URL
  mDataRemoteFaceplateURL =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplateURL->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_FACEPLATE_PLAY_URL),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplateURL->SetStringValue(EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  // Faceplate show remaining time
  mDataRemoteFaceplateRemainingTime =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplateRemainingTime->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_FACEPLATE_SHOWREMAINING),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString showRemainingTime;
  rv = mDataRemoteFaceplateRemainingTime->GetStringValue(showRemainingTime);
  NS_ENSURE_SUCCESS(rv, rv);

  if(showRemainingTime.IsEmpty()) {
    rv = mDataRemoteFaceplateRemainingTime->SetBoolValue(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Faceplate Volume
  mDataRemoteFaceplateVolume =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplateVolume->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_FACEPLATE_VOLUME),
    nullString);

  // Faceplate Mute
  mDataRemoteFaceplateMute =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplateMute->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_FACEPLATE_MUTE),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  //
  // Metadata DataRemotes
  //

  // Metadata Album
  mDataRemoteMetadataAlbum =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataAlbum->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_METADATA_ALBUM),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataAlbum->SetStringValue(EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  // Metadata Artist
  mDataRemoteMetadataArtist =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataArtist->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_METADATA_ARTIST),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataArtist->SetStringValue(EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  // Metadata Genre
  mDataRemoteMetadataGenre =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataGenre->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_METADATA_GENRE),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);


  rv = mDataRemoteMetadataGenre->SetStringValue(EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  // Metadata Title
  mDataRemoteMetadataTitle =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataTitle->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_METADATA_TITLE),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataTitle->SetStringValue(EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  // Metadata Duration
  mDataRemoteMetadataDuration =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataDuration->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_METADATA_LENGTH),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataDuration->SetIntValue(0);
  NS_ENSURE_SUCCESS(rv, rv);

  // Metadata Duration Str
  mDataRemoteMetadataDurationStr =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataDurationStr->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_METADATA_LENGTH_STR),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataDurationStr->SetStringValue(NS_LITERAL_STRING("0:00"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Metadata Position
  mDataRemoteMetadataPosition =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataPosition->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_METADATA_POSITION),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataPosition->SetIntValue(0);
  NS_ENSURE_SUCCESS(rv, rv);

  // Metadata Position Str
  mDataRemoteMetadataPositionStr =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataPositionStr->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_METADATA_POSITION_STR),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv =
    mDataRemoteMetadataPositionStr->SetStringValue(NS_LITERAL_STRING("0:00"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Metadata URL
  mDataRemoteMetadataURL =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataURL->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_METADATA_URL),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataURL->SetStringValue(EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  // Metadata image URL
  mDataRemoteMetadataImageURL =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataImageURL->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_METADATA_IMAGEURL),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataImageURL->SetStringValue(EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  // Playlist Shuffle
  mDataRemotePlaylistShuffle =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemotePlaylistShuffle->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_PLAYLIST_SHUFFLE),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString shuffle;
  rv = mDataRemotePlaylistShuffle->GetStringValue(shuffle);
  NS_ENSURE_SUCCESS(rv, rv);

  if(shuffle.IsEmpty()) {
    rv = mDataRemotePlaylistShuffle->SetBoolValue(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }


  // Playlist Repeat
  mDataRemotePlaylistRepeat =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemotePlaylistRepeat->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_PLAYLIST_REPEAT),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString repeat;
  rv = mDataRemotePlaylistRepeat->GetStringValue(repeat);
  NS_ENSURE_SUCCESS(rv, rv);

  if(shuffle.IsEmpty()) {
    rv = mDataRemotePlaylistRepeat->SetIntValue(
            sbIMediacoreSequencer::MODE_REPEAT_NONE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbMediacoreSequencer::UnbindDataRemotes()
{
  //
  // Faceplate DataRemotes
  //
  nsresult rv;

  if (mDataRemoteFaceplateBuffering) {
    rv = mDataRemoteFaceplateBuffering->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteFaceplatePlaying) {
    rv = mDataRemoteFaceplatePlaying->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteFaceplatePaused) {
    rv = mDataRemoteFaceplatePaused->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteFaceplateSeenPlaying) {
    rv = mDataRemoteFaceplateSeenPlaying->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteFaceplateURL) {
    rv = mDataRemoteFaceplateURL->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteFaceplateVolume) {
    rv = mDataRemoteFaceplateVolume->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteFaceplateMute) {
    rv = mDataRemoteFaceplateMute->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  //
  // Metadata DataRemotes
  //

  if (mDataRemoteMetadataAlbum) {
    rv = mDataRemoteMetadataAlbum->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteMetadataArtist) {
    rv = mDataRemoteMetadataArtist->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteMetadataGenre) {
    rv = mDataRemoteMetadataGenre->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteMetadataTitle) {
    rv = mDataRemoteMetadataTitle->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteMetadataDuration) {
    rv = mDataRemoteMetadataDuration->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteMetadataDurationStr) {
    rv = mDataRemoteMetadataDurationStr->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteMetadataPosition) {
    rv = mDataRemoteMetadataPosition->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteMetadataPositionStr) {
    rv = mDataRemoteMetadataPositionStr->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteMetadataURL) {
    rv = mDataRemoteMetadataURL->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteMetadataImageURL) {
    rv = mDataRemoteMetadataImageURL->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  //
  // Playlist DataRemotes
  //

  if (mDataRemotePlaylistShuffle) {
    rv = mDataRemotePlaylistShuffle->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemotePlaylistRepeat) {
    rv = mDataRemotePlaylistRepeat->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteFaceplateRemainingTime) {
    rv = mDataRemoteFaceplateRemainingTime->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbMediacoreSequencer::UpdatePlayStateDataRemotes()
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);

  PRBool paused = PR_FALSE;
  PRBool playing = PR_FALSE;

  if(mStatus == sbIMediacoreStatus::STATUS_BUFFERING ||
     mStatus == sbIMediacoreStatus::STATUS_PLAYING) {
    playing = PR_TRUE;
  }
  else if(mStatus == sbIMediacoreStatus::STATUS_PAUSED) {
    paused = PR_TRUE;
  }

  nsresult rv = mDataRemoteFaceplatePaused->SetBoolValue(paused);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplatePlaying->SetBoolValue(playing);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreSequencer::UpdatePositionDataRemotes(PRUint64 aPosition)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsString str;
  nsresult rv = EmitMillisecondsToTimeString(aPosition, str);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoMonitor mon(mMonitor);

  rv = mDataRemoteMetadataPosition->SetIntValue(aPosition);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataPositionStr->SetStringValue(str);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreSequencer::UpdateDurationDataRemotes(PRUint64 aDuration)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  if(!mPlaybackControl) {
    return NS_OK;
  }

  PRUint64 duration = aDuration;
  nsresult rv = mDataRemoteMetadataDuration->SetIntValue(duration);
  NS_ENSURE_SUCCESS(rv, rv);

  // show remaining time only affects the text based data remote!
  PRBool showRemainingTime = PR_FALSE;
  rv = mDataRemoteFaceplateRemainingTime->GetBoolValue(&showRemainingTime);
  NS_ENSURE_SUCCESS(rv, rv);

  if(showRemainingTime) {
    PRUint64 position = 0;

    rv = mPlaybackControl->GetPosition(&position);
    if(NS_FAILED(rv)) {
      position = 0;
    }

    duration = duration > position ? duration - position : 0;
  }

  nsString str;
  rv = EmitMillisecondsToTimeString(duration, str, showRemainingTime);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoMonitor mon(mMonitor);

  rv = mDataRemoteMetadataDurationStr->SetStringValue(str);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreSequencer::UpdateURLDataRemotes(nsIURI *aURI)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aURI);

  nsCString spec;
  nsresult rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoMonitor mon(mMonitor);

  NS_ConvertUTF8toUTF16 wideSpec(spec);
  rv = mDataRemoteFaceplateURL->SetStringValue(wideSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataURL->SetStringValue(wideSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreSequencer::UpdateShuffleDataRemote(PRUint32 aMode)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  PRBool shuffle = PR_FALSE;
  if(aMode == sbIMediacoreSequencer::MODE_SHUFFLE) {
    shuffle = PR_TRUE;
  }

  nsAutoMonitor mon(mMonitor);

  nsresult rv = mDataRemotePlaylistShuffle->SetBoolValue(shuffle);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreSequencer::UpdateRepeatDataRemote(PRUint32 aRepeatMode)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);

  nsresult rv = mDataRemotePlaylistRepeat->SetIntValue(aRepeatMode);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreSequencer::ResetPlayingVideoDataRemote()
{
  
  PRBool isPlayingVideo;
  nsresult rv = mDataRemoteFaceplatePlayingVideo->GetBoolValue(&isPlayingVideo);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isPlayingVideo) {
    rv = UpdateLastPositionProperty(mCurrentItem, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDataRemoteFaceplatePlayingVideo->SetBoolValue(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbMediacoreSequencer::HandleVolumeChangeEvent(sbIMediacoreEvent *aEvent)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aEvent);

  nsCOMPtr<nsIVariant> variant;
  nsresult rv = aEvent->GetData(getter_AddRefs(variant));
  NS_ENSURE_SUCCESS(rv, rv);

  PRFloat64 volume;
  rv = variant->GetAsDouble(&volume);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateVolumeDataRemote(volume);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreSequencer::UpdateVolumeDataRemote(PRFloat64 aVolume)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);

  nsCString volume;
  SB_ConvertFloatVolToJSStringValue(aVolume, volume);

  NS_ConvertUTF8toUTF16 volumeStr(volume);
  nsresult rv = mDataRemoteFaceplateVolume->SetStringValue(volumeStr);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreSequencer::HandleMuteChangeEvent(sbIMediacoreEvent *aEvent)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aEvent);

  nsCOMPtr<nsIVariant> variant;
  nsresult rv = aEvent->GetData(getter_AddRefs(variant));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool muted = PR_FALSE;
  rv = variant->GetAsBool(&muted);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateMuteDataRemote(muted);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreSequencer::UpdateMuteDataRemote(PRBool aMuted)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);

  nsresult rv = mDataRemoteFaceplateMute->SetBoolValue(aMuted);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreSequencer::HandleMetadataEvent(sbIMediacoreEvent *aEvent)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aEvent);

  nsCOMPtr<nsIVariant> variant;
  nsresult rv = aEvent->GetData(getter_AddRefs(variant));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> supports;
  rv = variant->GetAsISupports(getter_AddRefs(supports));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyArray> propertyArray =
    do_QueryInterface(supports, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length = 0;
  rv = propertyArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIProperty> property;
  for(PRUint32 current = 0; current < length; ++current) {
    rv = propertyArray->GetPropertyAt(current, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString id, value;
    rv = property->GetId(id);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->GetValue(value);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = SetMetadataDataRemote(id, value);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbMediacoreSequencer::SetMetadataDataRemote(const nsAString &aId,
                                            const nsAString &aValue)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  LOG(("[sbMediacoreSequencer] - SetMetadataDataRemote: Id '%s', Value '%s'",
         NS_ConvertUTF16toUTF8(aId).BeginReading(),
         NS_ConvertUTF16toUTF8(aValue).BeginReading()));

  if(!mCurrentItem) {
    return NS_OK;
  }

  //
  // For local files,  we want to keep the existing property rather than
  // overriding it here if we successfully imported metadata in the first
  // place. As a proxy for "successfully imported metadata", we check if
  // the artist is non-empty.
  //
  // We allow overriding for non-file URIs so that streams that update
  // their metadata periodically can continue to do so.
  //
  nsString artistName;
  nsresult rv = mCurrentItem->GetProperty(
          NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME), artistName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  rv = mCurrentItem->GetContentSrc(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString scheme;
  rv = uri->GetScheme(scheme);
  NS_ENSURE_SUCCESS(rv, rv);

  if (scheme.EqualsLiteral("file") && !artistName.IsEmpty())
    return NS_OK;

  nsCOMPtr<sbIDataRemote> remote;
  if(aId.EqualsLiteral(SB_PROPERTY_ALBUMNAME)) {
    remote = mDataRemoteMetadataAlbum;
  }
  else if(aId.EqualsLiteral(SB_PROPERTY_ARTISTNAME)) {
    remote = mDataRemoteMetadataArtist;
  }
  else if(aId.EqualsLiteral(SB_PROPERTY_GENRE)) {
    remote = mDataRemoteMetadataGenre;
  }
  else if(aId.EqualsLiteral(SB_PROPERTY_TRACKNAME)) {
    remote = mDataRemoteMetadataTitle;
  }
  else if(aId.EqualsLiteral(SB_PROPERTY_PRIMARYIMAGEURL)) {
    remote = mDataRemoteMetadataImageURL;
  }

  if(remote) {
    nsresult rv = remote->SetStringValue(aValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

PRBool
sbMediacoreSequencer::IsPropertyInPropertyArray(sbIPropertyArray *aPropArray,
                                                const nsAString &aPropName)
{
  PRUint32 length = 0;
  nsresult rv = aPropArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  nsCOMPtr<sbIProperty> property;
  for(PRUint32 current = 0; current < length; ++current) {
    rv = aPropArray->GetPropertyAt(current, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    nsString id;
    rv = property->GetId(id);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    if(id.Equals(aPropName)) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

/* Set the metadata data remotes from the properties on aItem.
 * If aPropertiesChanges is non-null, only set the data remotes that rely on
 * the changed properties.
 */
nsresult
sbMediacoreSequencer::SetMetadataDataRemotesFromItem(
        sbIMediaItem *aItem,
        sbIPropertyArray *aPropertiesChanged)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aItem);

  nsString albumName, artistName, genre, trackName, imageURL;
  nsresult rv;

  if(!aPropertiesChanged || 
     IsPropertyInPropertyArray(aPropertiesChanged,
                               NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME)))
  {
    rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                            albumName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDataRemoteMetadataAlbum->SetStringValue(albumName);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if(!aPropertiesChanged || 
     IsPropertyInPropertyArray(aPropertiesChanged,
                               NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME)))
  {
    rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME),
                            artistName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDataRemoteMetadataArtist->SetStringValue(artistName);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if(!aPropertiesChanged || 
     IsPropertyInPropertyArray(aPropertiesChanged,
                               NS_LITERAL_STRING(SB_PROPERTY_GENRE)))
  {
    rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_GENRE), genre);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDataRemoteMetadataGenre->SetStringValue(genre);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if(!aPropertiesChanged || 
     IsPropertyInPropertyArray(aPropertiesChanged,
                               NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME)))
  {
    rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME),
                            trackName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDataRemoteMetadataTitle->SetStringValue(trackName);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if(!aPropertiesChanged || 
     IsPropertyInPropertyArray(aPropertiesChanged,
                               NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL)))
  {
    rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL),
                            imageURL);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDataRemoteMetadataImageURL->SetStringValue(imageURL);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbMediacoreSequencer::ResetMetadataDataRemotes() {
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = mDataRemoteMetadataAlbum->SetStringValue(EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataArtist->SetStringValue(EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataGenre->SetStringValue(EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataTitle->SetStringValue(EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataImageURL->SetStringValue(EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdatePositionDataRemotes(0);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateDurationDataRemotes(0);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreSequencer::UpdateCurrentItemDuration(PRUint64 aDuration)
{
  if(mCurrentItem) {
    NS_NAMED_LITERAL_STRING(PROPERTY_DURATION, SB_PROPERTY_DURATION);
    nsString strDuration;
    nsresult rv = mCurrentItem->GetProperty(PROPERTY_DURATION,
                                            strDuration);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint64 itemDuration = 0;

    // if there was a duration set, convert it.
    if (!strDuration.IsEmpty()) {
      itemDuration = nsString_ToUint64(strDuration, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    itemDuration /= PR_USEC_PER_MSEC;

    if(aDuration && itemDuration != aDuration) {
      sbScopedBoolToggle doNotRecalculate(&mNoRecalculate);
      sbAutoString strNewDuration(aDuration * PR_USEC_PER_MSEC);
      rv = mCurrentItem->SetProperty(PROPERTY_DURATION, strNewDuration);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}

nsresult
sbMediacoreSequencer::StopPlaybackHelper(nsAutoMonitor& aMonitor)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;

  if(mStatus == sbIMediacoreStatus::STATUS_PLAYING ||
     mStatus == sbIMediacoreStatus::STATUS_PAUSED ||
     mStatus == sbIMediacoreStatus::STATUS_BUFFERING) {
    // Grip.
    nsCOMPtr<sbIMediacorePlaybackControl> playbackControl = mPlaybackControl;
    aMonitor.Exit();
    rv = playbackControl->Stop();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Stop failed at end of sequence.");
    aMonitor.Enter();
  }

  mStatus = sbIMediacoreStatus::STATUS_STOPPED;

  rv = StopSequenceProcessor();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdatePlayStateDataRemotes();
  NS_ENSURE_SUCCESS(rv, rv);

  if(mSeenPlaying) {
    mSeenPlaying = PR_FALSE;

    rv = mDataRemoteFaceplateSeenPlaying->SetBoolValue(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbMediacoreSequencer::HandleErrorEvent(sbIMediacoreEvent *aEvent)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aEvent);

  nsAutoMonitor mon(mMonitor);
  mErrorCount++;

  if(mIsWaitingForPlayback) {
    mIsWaitingForPlayback = PR_FALSE;
  }

  nsresult rv;
  if(mErrorCount >= MEDIACORE_MAX_SUBSEQUENT_ERRORS) {
    // Too many subsequent errors, stop
    rv = StopPlaybackHelper(mon);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    if (mCoreWillHandleNext) {
      // The core has requested handling the next item, then given us an error.
      // Assume the error is about the next item - thus, we should skip to two
      // items ahead.
      // This call to Next() (while mCoreWillHandleNext is set) will just cause
      // the position tracking to update - we won't try to play it again this
      // time.
      rv = Next(PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);
    }      
      
    mCoreWillHandleNext = PR_FALSE;

    nsCOMPtr<sbIMediaItem> mediaItem;
    rv = GetCurrentItem(getter_AddRefs(mediaItem));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString contentType;
    rv = mediaItem->GetContentType(contentType);
    NS_ENSURE_SUCCESS(rv, rv);

    // Pause the sequencer if playback error happens to video.
    if (!contentType.Equals(NS_LITERAL_STRING("video"))) {
      rv = Next(PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = StopPlaybackHelper(mon);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  mon.Exit();

  nsCOMPtr<sbIMediacoreError> error;
  rv = aEvent->GetError(getter_AddRefs(error));
  NS_ENSURE_SUCCESS(rv, rv);

  // If there's an error object, we'll show the contents of it to the user
  if(error) {
    nsCOMPtr<sbIMediacoreErrorHandler> errorHandler =
      do_GetService("@songbirdnest.com/Songbird/MediacoreErrorHandler;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = errorHandler->ProcessError(error);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbMediacoreSequencer::RecalculateSequence(PRInt64 *aViewPosition /*= nsnull*/)
{
  LOG(("[%s] Recalculating Sequence", __FUNCTION__));

  nsAutoMonitor mon(mMonitor);

  if(!mView) {
    return NS_OK;
  }

  mSequence.clear();
  mViewIndexToSequenceIndex.clear();

  PRUint32 length = 0;
  nsresult rv = mView->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  mPosition = 0;
  mSequence.reserve(length);

  // ensure view position is inside the bounds of the view.
  if(aViewPosition &&
     ((*aViewPosition >= length) ||
      (*aViewPosition < sbIMediacoreSequencer::AUTO_PICK_INDEX))) {
    *aViewPosition = 0;
  }

  switch(mMode) {
    case sbIMediacoreSequencer::MODE_FORWARD:
    {
      // Generate forward sequence
      for(PRUint32 i = 0; i < length; ++i) {
        mSequence.push_back(i);
        mViewIndexToSequenceIndex[i] = i;
      }

      if(aViewPosition &&
         *aViewPosition != sbIMediacoreSequencer::AUTO_PICK_INDEX) {
        mPosition = *aViewPosition;
      }
    }
    break;
    case sbIMediacoreSequencer::MODE_REVERSE:
    {
      // Generate reverse sequence
      PRUint32 j = 0;
      for(PRUint32 i = length - 1; i >= 0; --i, ++j) {
        mSequence.push_back(i);
        mViewIndexToSequenceIndex[j] = j;
      }

      if(aViewPosition &&
         *aViewPosition != sbIMediacoreSequencer::AUTO_PICK_INDEX) {
        mPosition = length - *aViewPosition;
      }
    }
    break;
    case sbIMediacoreSequencer::MODE_SHUFFLE:
    {
      NS_ENSURE_TRUE(mShuffleGenerator, NS_ERROR_UNEXPECTED);

      PRUint32 sequenceLength = 0;
      PRUint32 *sequence = nsnull;

      rv = mShuffleGenerator->OnGenerateSequence(mView,
                                                 &sequenceLength,
                                                 &sequence);
      NS_ENSURE_SUCCESS(rv, rv);

      for(PRUint32 i = 0; i < sequenceLength; ++i) {
        mSequence.push_back(sequence[i]);
        mViewIndexToSequenceIndex[sequence[i]] = i;

        if(aViewPosition &&
           *aViewPosition != sbIMediacoreSequencer::AUTO_PICK_INDEX &&
           *aViewPosition == sequence[i]) {
          // Swap the first position item with the item that was selected by the
          // user to play first.
          PRUint32 viewIndex = mSequence[0];
          mSequence[0] = mSequence[i];
          mSequence[i] = viewIndex;

          PRUint32 sequenceIndex = mViewIndexToSequenceIndex[viewIndex];
          mViewIndexToSequenceIndex[viewIndex] = mViewIndexToSequenceIndex[mSequence[0]];
          mViewIndexToSequenceIndex[mSequence[0]] = sequenceIndex;
        }
      }

      NS_Free(sequence);
    }
    break;
    case sbIMediacoreSequencer::MODE_CUSTOM:
    {
      NS_ENSURE_TRUE(mCustomGenerator, NS_ERROR_UNEXPECTED);

      PRUint32 sequenceLength = 0;
      PRUint32 *sequence = nsnull;

      rv = mCustomGenerator->OnGenerateSequence(mView,
                                                &sequenceLength,
                                                &sequence);
      NS_ENSURE_SUCCESS(rv, rv);

      for(PRUint32 i = 0; i < sequenceLength; ++i) {
        mSequence.push_back(sequence[i]);
        mViewIndexToSequenceIndex[sequence[i]] = i;

        if(aViewPosition &&
           *aViewPosition != sbIMediacoreSequencer::AUTO_PICK_INDEX &&
           *aViewPosition == sequence[i]) {
          // Match the sequence position to the item that is selected
          mPosition = i;
        }
      }

      NS_Free(sequence);
    }
    break;
  }

  if(mSequence.size()) {
    mViewPosition = mSequence[mPosition];
  }
  else {
    mViewPosition = 0;
  }

  // Fire sequence changed event
  nsCOMPtr<nsIArray> newSequence;
  rv = GetCurrentSequence(getter_AddRefs(newSequence));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> variant = sbNewVariant(newSequence).get();
  NS_ENSURE_TRUE(variant, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<sbIMediacoreEvent> event;
  rv = sbMediacoreEvent::CreateEvent(sbIMediacoreEvent::SEQUENCE_CHANGE,
                                     nsnull,
                                     variant,
                                     mCore,
                                     getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DispatchMediacoreEvent(event);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreSequencer::GetItem(const sequence_t &aSequence,
                              PRUint32 aPosition,
                              sbIMediaItem **aItem)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aItem);

  nsAutoMonitor mon(mMonitor);

  PRUint32 length = mSequence.size();
  NS_ENSURE_TRUE(aPosition < length, NS_ERROR_INVALID_ARG);

  nsCOMPtr<sbIMediaItem> item;
  nsresult rv = mView->GetItemByIndex(aSequence[aPosition],
                                      getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  item.forget(aItem);

  return NS_OK;
}

nsresult
sbMediacoreSequencer::ProcessNewPosition()
{
  nsAutoMonitor mon(mMonitor);

  nsresult rv = ResetMetadataDataRemotes();
  NS_ENSURE_SUCCESS(rv, rv);

  // if the current core requested handling of the next item, we have nothing
  // to do here but reset the flag.
  if(mCoreWillHandleNext) {
    mon.Exit();

    rv = CoreHandleNextSetup();
    if(rv == NS_ERROR_ABORT) {
      NS_WARNING("Someone aborted playback of the next track.");
      return NS_OK;
    }
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  mon.Exit();

  rv = Setup();
  if(rv == NS_ERROR_ABORT) {
    NS_WARNING("Someone aborted playback of the next track.");
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  mon.Enter();

  if(mStatus == sbIMediacoreStatus::STATUS_PLAYING ||
     mStatus == sbIMediacoreStatus::STATUS_BUFFERING) {

    mStatus = sbIMediacoreStatus::STATUS_BUFFERING;
    mIsWaitingForPlayback = PR_TRUE;

    rv = UpdatePlayStateDataRemotes();
    NS_ENSURE_SUCCESS(rv, rv);

    mon.Exit();

    rv = StartPlayback();
  }
  else if(mStatus == sbIMediacoreStatus::STATUS_PAUSED) {
    mon.Exit();
    rv = mPlaybackControl->Pause();
  }

  if(NS_FAILED(rv)) {
    mon.Enter();
    mStatus = sbIMediacoreStatus::STATUS_STOPPED;
    mIsWaitingForPlayback = PR_FALSE;

    rv = UpdatePlayStateDataRemotes();
    NS_ENSURE_SUCCESS(rv, rv);

    return rv;
  }

  return NS_OK;
}

nsresult
sbMediacoreSequencer::Setup(nsIURI *aURI /*= nsnull*/)
{
  nsAutoMonitor mon(mMonitor);

  nsCOMPtr<nsIURI> uri;
  nsCOMPtr<sbIMediaItem> item;
  nsCOMPtr<sbIMediaItem> lastItem = mCurrentItem;

  nsresult rv = NS_ERROR_UNEXPECTED;

  if(!aURI) {
    rv = GetItem(mSequence, mPosition, getter_AddRefs(item));
    NS_ENSURE_SUCCESS(rv, rv);

    mCurrentItemIndex = mSequence[mPosition];

    rv = mView->GetViewItemUIDForIndex(mCurrentItemIndex, mCurrentItemUID);
    NS_ENSURE_SUCCESS(rv, rv);

    mCurrentItem = item;

    rv = item->GetContentSrc(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    uri = aURI;

    mCurrentItem = nsnull;
    mCurrentItemIndex = 0;
  }

  nsCOMPtr<sbIMediacoreVoting> voting =
    do_QueryReferent(mMediacoreManager, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(voting, NS_ERROR_UNEXPECTED);

  nsCOMPtr<sbIMediacoreVotingChain> votingChain;
  rv = voting->VoteWithURI(uri, getter_AddRefs(votingChain));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool validChain = PR_FALSE;
  rv = votingChain->GetValid(&validChain);
  NS_ENSURE_SUCCESS(rv, rv);

  // Not a valid chain, we'll have to skip this item.
  if(!validChain) {
    // XXXAus: Skip item later, for now fail.
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIArray> chain;
  rv = votingChain->GetMediacoreChain(getter_AddRefs(chain));
  NS_ENSURE_SUCCESS(rv, rv);

  mChain = chain;
  mChainIndex = 0;

  // Remove listener from old core.
  if(mCore) {
    // But only remove it if we're not going to use the same core
    // to play again.
    nsCOMPtr<sbIMediacore> nextCore =
      do_QueryElementAt(chain, mChainIndex, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if(mCore != nextCore) {
      nsCOMPtr<sbIMediacoreEventTarget> eventTarget =
        do_QueryInterface(mCore, &rv);

      rv = eventTarget->RemoveListener(this);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if(mStatus == sbIMediacoreStatus::STATUS_BUFFERING ||
       mStatus == sbIMediacoreStatus::STATUS_PLAYING ||
       mStatus == sbIMediacoreStatus::STATUS_PAUSED) {

      if(mCore == nextCore) {
        // Remember the fact that we called stop so we ignore the stop
        // event coming from the core.
        mStopTriggeredBySequencer = PR_TRUE;
      }

      // Grip.
      nsCOMPtr<sbIMediacorePlaybackControl> playbackControl = mPlaybackControl;
      mon.Exit();

      // If we played an Item, update it. If we played an url (no Item in Library), we skip this part.
      if (lastItem){
        rv = UpdateLastPositionProperty(lastItem, nsnull);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      rv = playbackControl->Stop();
      NS_ASSERTION(NS_SUCCEEDED(rv),
        "Stop returned failure. Attempting to recover.");

      mon.Enter();
    }
  }

  nsCOMPtr<sbIMediacorePlaybackControl> playbackControl =
    do_QueryElementAt(chain, mChainIndex, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mPlaybackControl = playbackControl;

  mCore = do_QueryElementAt(chain, mChainIndex, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Fire before track change event
  if(item) {
    nsCOMPtr<nsIVariant> variant = sbNewVariant(item).get();
    NS_ENSURE_TRUE(variant, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<sbIMediacoreEvent> event;
    rv = sbMediacoreEvent::CreateEvent(sbIMediacoreEvent::BEFORE_TRACK_CHANGE,
                                       nsnull,
                                       variant,
                                       mCore,
                                       getter_AddRefs(event));
    NS_ENSURE_SUCCESS(rv, rv);

    sbScopedBoolToggle canAbort(&mCanAbort);

    rv = DispatchMediacoreEvent(event);
    NS_ENSURE_SUCCESS(rv, rv);

    mon.Exit();

    // Process any pending abort requests
    if(HandleAbort()) {
      return NS_ERROR_ABORT;
    }

    mon.Enter();
  }

  // Add listener to new core.
  nsCOMPtr<sbIMediacoreEventTarget> eventTarget =
    do_QueryInterface(mCore, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = eventTarget->AddListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPlaybackControl->SetUri(uri);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef PR_LOGGING
  {
    nsCString spec;
    rv = uri->GetSpec(spec);
    LOG(("[sbMediacoreSequencer] -- Attempting to play %s",
          spec.BeginReading()));
  }
#endif

  nsCOMPtr<sbPIMediacoreManager> privateMediacoreManager =
    do_QueryReferent(mMediacoreManager, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = privateMediacoreManager->SetPrimaryCore(mCore);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mCore->SetSequencer(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = StartSequenceProcessor();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateURLDataRemotes(uri);
  NS_ENSURE_SUCCESS(rv, rv);

  if(item) {
    rv = SetMetadataDataRemotesFromItem(item);
    NS_ENSURE_SUCCESS(rv, rv);

    // Fire track change event
    nsCOMPtr<nsIVariant> variant = sbNewVariant(item).get();
    NS_ENSURE_TRUE(variant, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<sbIMediacoreEvent> event;
    rv = sbMediacoreEvent::CreateEvent(sbIMediacoreEvent::TRACK_CHANGE,
                                       nsnull,
                                       variant,
                                       mCore,
                                       getter_AddRefs(event));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = DispatchMediacoreEvent(event, PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbMediacoreSequencer::CoreHandleNextSetup()
{
  nsAutoMonitor mon(mMonitor);

  mCoreWillHandleNext = PR_FALSE;

  // If the current position is valid, get the item at the current position.
  // Otherwise, get the current item without modifying the sequencer state.
  nsresult rv;
  nsCOMPtr<sbIMediaItem> item;
  if (!mPositionInvalidated) {
    rv = GetItem(mSequence, mPosition, getter_AddRefs(item));
    NS_ENSURE_SUCCESS(rv, rv);

    mCurrentItemIndex = mSequence[mPosition];

    rv = mView->GetViewItemUIDForIndex(mCurrentItemIndex, mCurrentItemUID);
    NS_ENSURE_SUCCESS(rv, rv);

    mCurrentItem = item;
  } else {
    item = mCurrentItem;
  }

  nsCOMPtr<nsIURI> uri;
  rv = item->GetContentSrc(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> variant = sbNewVariant(item).get();
  NS_ENSURE_TRUE(variant, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<sbIMediacoreEvent> event;
  rv = sbMediacoreEvent::CreateEvent(sbIMediacoreEvent::BEFORE_TRACK_CHANGE,
                                     nsnull,
                                     variant,
                                     mCore,
                                     getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  // Additional scope to handle 'canAbort' status
  {
    sbScopedBoolToggle canAbort(&mCanAbort);

    rv = DispatchMediacoreEvent(event);
    NS_ENSURE_SUCCESS(rv, rv);

    mon.Exit();

    // Process any pending abort requests
    if(HandleAbort()) {
      return NS_ERROR_ABORT;
    }

    mon.Enter();
  }

  rv = UpdateURLDataRemotes(uri);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetMetadataDataRemotesFromItem(item);
  NS_ENSURE_SUCCESS(rv, rv);

  // Fire track change event
  variant = sbNewVariant(item).get();
  rv = sbMediacoreEvent::CreateEvent(sbIMediacoreEvent::TRACK_CHANGE,
                                     nsnull,
                                     variant,
                                     mCore,
                                     getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DispatchMediacoreEvent(event);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

PRBool
sbMediacoreSequencer::HandleAbort()
{
  nsAutoMonitor mon(mMonitor);

  if(mShouldAbort) {
    mShouldAbort = PR_FALSE;

    mon.Exit();

    nsresult rv = Stop();
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    return PR_TRUE;
  }

  return PR_FALSE;
}

nsresult
sbMediacoreSequencer::SetViewWithViewPosition(sbIMediaListView *aView,
                                              PRInt64 *aViewPosition /* = nsnull */)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aView);

  nsAutoMonitor mon(mMonitor);

  // Regardless of what happens here, we'll have a valid position and view
  // position after the method returns, so reset the invalidated position flag.
  mPositionInvalidated = PR_FALSE;

  PRUint32 viewLength = 0;
  nsresult rv = aView->GetLength(&viewLength);
  NS_ENSURE_SUCCESS(rv, rv);

  if(mView != aView ||
     mSequence.size() != viewLength) {

    // Fire before view change event
    nsCOMPtr<nsIVariant> variant = sbNewVariant(aView).get();
    NS_ENSURE_TRUE(variant, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<sbIMediacoreEvent> event;
    rv =
      sbMediacoreEvent::CreateEvent(sbIMediacoreEvent::BEFORE_VIEW_CHANGE,
                                    nsnull,
                                    variant,
                                    mCore,
                                    getter_AddRefs(event));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = DispatchMediacoreEvent(event);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = StopWatchingView();
    NS_ENSURE_SUCCESS(rv, rv);

    mView = aView;

    rv = StartWatchingView();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = RecalculateSequence(aViewPosition);
    NS_ENSURE_SUCCESS(rv, rv);

    // Fire view change event
    rv = sbMediacoreEvent::CreateEvent(sbIMediacoreEvent::VIEW_CHANGE,
                                       nsnull,
                                       variant,
                                       mCore,
                                       getter_AddRefs(event));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = DispatchMediacoreEvent(event);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if(aViewPosition &&
          *aViewPosition >= 0 &&
          mViewPosition != *aViewPosition &&
          mViewIndexToSequenceIndex.size() > *aViewPosition) {
    // We check to see if the view position is different than the current view
    // position before setting the new view position.
    mPosition = mViewIndexToSequenceIndex[*aViewPosition];
    mViewPosition = mSequence[mPosition];
  }

  return NS_OK;
}

nsresult
sbMediacoreSequencer::StartWatchingView()
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);

  // No view, we're probably playing single items
  if(!mView) {
    return NS_OK;
  }

  if(mWatchingView) {
    return NS_OK;
  }

  nsresult rv = mView->AddListener(this, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mView->GetMediaList(getter_AddRefs(mViewList));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> library = do_QueryInterface(mViewList, &rv);
  mViewIsLibrary = NS_SUCCEEDED(rv) ? PR_TRUE : PR_FALSE;

  rv = mViewList->AddListener(this,
                              PR_FALSE,
                              sbIMediaList::LISTENER_FLAGS_ITEMADDED |
                              sbIMediaList::LISTENER_FLAGS_ITEMUPDATED |
                              sbIMediaList::LISTENER_FLAGS_ITEMMOVED |
                              sbIMediaList::LISTENER_FLAGS_AFTERITEMREMOVED |
                              sbIMediaList::LISTENER_FLAGS_BATCHBEGIN |
                              sbIMediaList::LISTENER_FLAGS_BATCHEND |
                              sbIMediaList::LISTENER_FLAGS_LISTCLEARED,
                              nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  if(!mViewIsLibrary) {
    nsCOMPtr<sbIMediaItem> item = do_QueryInterface(mViewList, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = item->GetLibrary(getter_AddRefs(library));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaList> list = do_QueryInterface(library, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = list->AddListener(this,
                           PR_FALSE,
                           sbIMediaList::LISTENER_FLAGS_AFTERITEMREMOVED |
                           sbIMediaList::LISTENER_FLAGS_ITEMUPDATED |
                           sbIMediaList::LISTENER_FLAGS_BATCHBEGIN |
                           sbIMediaList::LISTENER_FLAGS_BATCHEND |
                           sbIMediaList::LISTENER_FLAGS_LISTCLEARED,
                           nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mWatchingView = PR_TRUE;

  return NS_OK;
}

nsresult
sbMediacoreSequencer::StopWatchingView()
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);

  // No view, we're probably playing single items
  if(!mView) {
    return NS_OK;
  }

  if(!mWatchingView) {
    return NS_OK;
  }

  nsresult rv = NS_ERROR_UNEXPECTED;

  if(mDelayedCheckTimer) {
    rv = HandleDelayedCheckTimer(mDelayedCheckTimer);
    NS_ENSURE_SUCCESS(rv, rv);

    if(mDelayedCheckTimer) {
      rv = mDelayedCheckTimer->Cancel();
      NS_ENSURE_SUCCESS(rv, rv);

      mDelayedCheckTimer = nsnull;
    }
  }

  rv = mViewList->RemoveListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mView->RemoveListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  if(!mViewIsLibrary) {
    nsCOMPtr<sbIMediaItem> item = do_QueryInterface(mViewList, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbILibrary> library;
    rv = item->GetLibrary(getter_AddRefs(library));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaList> list = do_QueryInterface(library, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = list->RemoveListener(this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mWatchingView = PR_FALSE;

  mListBatchCount = 0;
  mLibraryBatchCount = 0;
  mSmartRebuildDetectBatchCount = 0;

  mViewIsLibrary = PR_FALSE;
  mNeedSearchPlayingItem = PR_FALSE;
  mNeedsRecalculate = PR_FALSE;

  return NS_OK;
}

nsresult
sbMediacoreSequencer::DelayedCheck()
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);

  nsresult rv = NS_ERROR_UNEXPECTED;
  if(mDelayedCheckTimer) {
    rv = mDelayedCheckTimer->Cancel();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    mDelayedCheckTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mDelayedCheckTimer->InitWithCallback(
          this, MEDIACORE_CHECK_DELAY, nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreSequencer::UpdateItemUIDIndex()
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_STATE(mView);
  NS_ENSURE_STATE(mCurrentItem);

  nsAutoMonitor mon(mMonitor);

  if(mNoRecalculate) {
    mNeedsRecalculate = PR_FALSE;
    return NS_OK;
  }

  nsString previousItemUID = mCurrentItemUID;
  PRUint32 previousItemIndex = mCurrentItemIndex;

  nsresult rv = NS_ERROR_UNEXPECTED;

  if(!mNeedSearchPlayingItem) {
    rv = mView->GetIndexForViewItemUID(mCurrentItemUID,
                                       &mCurrentItemIndex);
  }
  else {
    // Item not there anymore (by UID), just try and get
    // the item index by current item. We have to do this
    // for smart playlists.
    rv = mView->GetIndexForItem(mCurrentItem, &mCurrentItemIndex);

    if(NS_SUCCEEDED(rv)) {
      // Grab the new item uid.
      rv = mView->GetViewItemUIDForIndex(mCurrentItemIndex, mCurrentItemUID);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // Ok, looks like we'll have to regenerate the sequence and start playing
  // from the new sequence only after the current item is done playing.
  mPositionInvalidated = NS_FAILED(rv) ? PR_TRUE: PR_FALSE;

  // If we should reset the position when the position is invalid,
  // do so now. Resetting the position is only necessary for filter,
  // search and xxx changes.
  if(mPositionInvalidated && mResetPosition) {
    mCurrentItemIndex = 0;
  }

  if(mCurrentItemIndex != previousItemIndex ||
     mCurrentItemUID != previousItemUID ||
     mNeedsRecalculate) {

    mNeedsRecalculate = PR_FALSE;

    PRInt64 currentItemIndex = sbIMediacoreSequencer::AUTO_PICK_INDEX;
    if (!mPositionInvalidated)
      currentItemIndex = mCurrentItemIndex;
    rv = RecalculateSequence(&currentItemIndex);
    NS_ENSURE_SUCCESS(rv, rv);

    if(!mPositionInvalidated) {
      nsCOMPtr<nsIVariant> variant = sbNewVariant(mCurrentItem).get();
      NS_ENSURE_TRUE(variant, NS_ERROR_OUT_OF_MEMORY);

      nsCOMPtr<sbIMediacoreEvent> event;
      rv = sbMediacoreEvent::CreateEvent(sbIMediacoreEvent::TRACK_INDEX_CHANGE,
        nsnull,
        variant,
        mCore,
        getter_AddRefs(event));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = DispatchMediacoreEvent(event);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}


nsresult
sbMediacoreSequencer::DispatchMediacoreEvent(sbIMediacoreEvent *aEvent,
                                             PRBool aAsync /*= PR_FALSE*/)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aEvent);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<sbIMediacoreEventTarget> target =
    do_QueryReferent(mMediacoreManager, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool dispatched = PR_FALSE;
  rv = target->DispatchEvent(aEvent, aAsync, &dispatched);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult
sbMediacoreSequencer::StartPlayback()
{
  nsresult rv;

  // Get the URI scheme for the media to playback
  nsCOMPtr<nsIURI> uri;
  rv = mPlaybackControl->GetUri(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString scheme;
  rv = uri->GetScheme(scheme);
  NS_ENSURE_SUCCESS(rv, rv);

  // If attempting to play from an MTP device, dispatch an error event and do
  // nothing more
  if (scheme.Equals("x-mtp"))
  {
    // Create a mediacore error
    nsCOMPtr<sbMediacoreError> error;
    NS_NEWXPCOM(error, sbMediacoreError);
    NS_ENSURE_TRUE(error, NS_ERROR_OUT_OF_MEMORY);

    // Get the error message
    error->Init
      (0, sbStringBundle().Get("mediacore.device_media.error.text"));

    // Create the error event
    nsCOMPtr<sbIMediacoreEvent> event;
    rv = sbMediacoreEvent::CreateEvent(sbIMediacoreEvent::ERROR_EVENT,
                                       error,
                                       nsnull,
                                       mCore,
                                       getter_AddRefs(event));
    NS_ENSURE_SUCCESS(rv, rv);

    // Dispatch the event
    nsCOMPtr<sbIMediacoreEventTarget> target = do_QueryInterface(mCore, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool dispatched;
    rv = target->DispatchEvent(event, PR_TRUE, &dispatched);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  // Start playback
  rv = mPlaybackControl->Play();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreSequencer::UpdateLastPositionProperty(sbIMediaItem* aItem,
                                                 nsIVariant* aData)
{
  TRACE(("%s: item(%p)", __FUNCTION__, aItem));
  NS_ENSURE_ARG_POINTER(aItem);

  nsresult rv;

  PRBool hasVideo;
  rv = mDataRemoteFaceplatePlayingVideo->GetBoolValue(&hasVideo);
  if (NS_FAILED(rv) || !hasVideo) {
    // we only track last position for video
    return NS_OK;
  }

  PRUint64 position, duration;

  if (aData) {
    nsCOMPtr<nsISupports> supports;
    nsIID *iid = nsnull;
    rv = aData->GetAsInterface(&iid, getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIPropertyBag2> bag = do_QueryInterface(supports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = bag->GetPropertyAsUint64(NS_LITERAL_STRING("position"), &position);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = bag->GetPropertyAsUint64(NS_LITERAL_STRING("duration"), &duration);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIURI> expectedURI;
    rv = bag->GetPropertyAsInterface(NS_LITERAL_STRING("uri"),
                                     NS_GET_IID(nsIURI),
                                     getter_AddRefs(expectedURI));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCString expectedSpec;
    nsString actualSpec;
    rv = expectedURI->GetSpec(expectedSpec);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL), actualSpec);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!expectedSpec.Equals(NS_ConvertUTF16toUTF8(actualSpec))) {
      // this isn't a stop, it's a track change; since we're way too late now,
      // don't set anything.  We would have caught it from
      // sbMediacoreSequencer::Setup() anyway.
      return NS_OK;
    }
  }
  else {
    rv = mPlaybackControl->GetPosition(&position);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mPlaybackControl->GetDuration(&duration);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  TRACE(("%s: position = %llu / %llu status %08x",
         __FUNCTION__,  position, duration, mStatus));

  #ifdef PR_LOGGING
  do {
    nsString actualSpec;
    rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL), actualSpec);
    if (NS_FAILED(rv)) break;
    TRACE(("%s[%p]: actual uri %s",
           __FUNCTION__, this,
           NS_ConvertUTF16toUTF8(actualSpec).get()));
  } while(0);
  #endif

  if (position == 0 || duration == 0) {
    // this is invalid, probably means things have already stopped
    return NS_OK;
  }

  /* the number of milliseconds before the end of the track where we will
   * consider this track have to finished playing (so the next play of this
   * track will not resume where we left off)
   */
  const PRUint64 TIME_BEFORE_END = 10 * PR_MSEC_PER_SEC;
  NS_NAMED_LITERAL_STRING(PROPERTY_LAST_POSITION, SB_PROPERTY_LASTPLAYPOSITION);

  if (position + TIME_BEFORE_END >= duration) {
    // this is near the end, just clear the remembered position
    rv = aItem->SetProperty(PROPERTY_LAST_POSITION, SBVoidString());
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    sbAutoString strPosition(position);
    rv = aItem->SetProperty(PROPERTY_LAST_POSITION, strPosition);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
//  sbIMediacoreSequencer
//------------------------------------------------------------------------------

NS_IMETHODIMP
sbMediacoreSequencer::GetMode(PRUint32 *aMode)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aMode);

  nsAutoMonitor mon(mMonitor);
  *aMode = mMode;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::SetMode(PRUint32 aMode)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  PRBool validMode = PR_FALSE;
  switch(aMode) {
    case sbIMediacoreSequencer::MODE_FORWARD:
    case sbIMediacoreSequencer::MODE_REVERSE:
    case sbIMediacoreSequencer::MODE_SHUFFLE:
    case sbIMediacoreSequencer::MODE_CUSTOM:
      validMode = PR_TRUE;
    break;
  }
  NS_ENSURE_TRUE(validMode, NS_ERROR_INVALID_ARG);

  nsAutoMonitor mon(mMonitor);

  if(mMode != aMode) {
    mMode = aMode;

    PRInt64 viewPosition = mViewPosition;
    nsresult rv = RecalculateSequence(&viewPosition);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = UpdateShuffleDataRemote(aMode);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::GetRepeatMode(PRUint32 *aRepeatMode)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aRepeatMode);

  nsAutoMonitor mon(mMonitor);
  *aRepeatMode = mRepeatMode;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::SetRepeatMode(PRUint32 aRepeatMode)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  PRBool validMode = PR_FALSE;
  switch(aRepeatMode) {
    case sbIMediacoreSequencer::MODE_REPEAT_NONE:
    case sbIMediacoreSequencer::MODE_REPEAT_ONE:
    case sbIMediacoreSequencer::MODE_REPEAT_ALL:
      validMode = PR_TRUE;
    break;
  }
  NS_ENSURE_TRUE(validMode, NS_ERROR_INVALID_ARG);

  nsAutoMonitor mon(mMonitor);
  mRepeatMode = aRepeatMode;

  nsresult rv = UpdateRepeatDataRemote(aRepeatMode);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::GetView(sbIMediaListView * *aView)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aView);

  nsAutoMonitor mon(mMonitor);
  NS_IF_ADDREF(*aView = mView);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::SetView(sbIMediaListView * aView)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aView);

  return SetViewWithViewPosition(aView);
}

NS_IMETHODIMP
sbMediacoreSequencer::GetViewPosition(PRUint32 *aViewPosition)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aViewPosition);

  nsAutoMonitor mon(mMonitor);

  // Position currently not valid, return with error.
  if(mPositionInvalidated) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  *aViewPosition = mViewPosition;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::GetCurrentItem(sbIMediaItem **aItem)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aItem);

  if (!NS_IsMainThread()) {
    // this method accesses the view, which isn't threadsafe.
    // proxy to the main thread to avoid hilarious crashes.
    nsresult rv;
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    nsCOMPtr<sbIMediacoreSequencer> proxiedSeq;
    rv = do_GetProxyForObject(mainThread,
                              NS_GET_IID(sbIMediacoreSequencer),
                              NS_ISUPPORTS_CAST(sbIMediacoreSequencer*, this),
                              NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                              getter_AddRefs(proxiedSeq));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = proxiedSeq->GetCurrentItem(aItem);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }
  // by default, if there's no current item, this doesn't throw, it returns a
  // null item instead.
  *aItem = nsnull;

  if(!mView) {
    return NS_OK;
  }

  PRUint32 index = 0;
  nsresult rv = mView->GetIndexForViewItemUID(mCurrentItemUID, &index);
  NS_ENSURE_SUCCESS(rv, NS_OK);

  rv = mView->GetItemByIndex(index, aItem);
  NS_ENSURE_SUCCESS(rv, NS_OK);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::GetNextItem(sbIMediaItem **aItem)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aItem);

  nsAutoMonitor mon(mMonitor);

  if(mRepeatMode == sbIMediacoreSequencer::MODE_REPEAT_ONE) {
    NS_IF_ADDREF(*aItem = mCurrentItem);
    return NS_OK;
  }

  // by default, if there's no current item, this doesn't throw, it returns a
  // null item instead.
  *aItem = nsnull;

  PRUint32 nextPosition = mPositionInvalidated ? mPosition : mPosition + 1;

  if(!mView ||
     nextPosition >= mSequence.size()) {
    return NS_OK;
  }

  nsresult rv = mView->GetItemByIndex(mSequence[nextPosition], aItem);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::GetCurrentSequence(nsIArray * *aCurrentSequence)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aCurrentSequence);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIMutableArray> array =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  sequence_t::const_iterator it = mSequence.begin();
  for(; it != mSequence.end(); ++it) {
    nsCOMPtr<nsISupportsPRUint32> index =
      do_CreateInstance("@mozilla.org/supports-PRUint32;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = index->SetData((*it));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = array->AppendElement(index, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF(*aCurrentSequence = array);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::GetSequencePosition(PRUint32 *aSequencePosition)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aSequencePosition);

  nsAutoMonitor mon(mMonitor);

  // Position currently not valid, return with error.
  if(mPositionInvalidated) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  *aSequencePosition = mPosition;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::SetSequencePosition(PRUint32 aSequencePosition)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbMediacoreSequencer::PlayView(sbIMediaListView *aView,
                               PRInt64 aItemIndex,
                               PRBool aNotFromUserAction)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aView);

  nsresult rv = SetViewWithViewPosition(aView, &aItemIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool proceed;
  rv = ValidateMediaItemControllerPlayback(!aNotFromUserAction, 
                                           ONHOLD_PLAYVIEW, 
                                           &proceed);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (!proceed) {
    return NS_OK;
  }

  rv = Play();
  NS_ENSURE_SUCCESS(rv, rv);

  // Fire EXPLICIT_TRACK_CHANGE when PlayView() is called.
  nsCOMPtr<sbIMediacoreEvent> event;
  rv = sbMediacoreEvent::CreateEvent(sbIMediacoreEvent::EXPLICIT_TRACK_CHANGE,
                                     nsnull,
                                     nsnull,
                                     mCore,
                                     getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DispatchMediacoreEvent(event);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::PlayURL(nsIURI *aURI)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aURI);

  nsAutoMonitor mon(mMonitor);

  mStatus = sbIMediacoreStatus::STATUS_BUFFERING;
  mErrorCount = 0;
  mIsWaitingForPlayback = PR_TRUE;

  nsresult rv = ResetMetadataDataRemotes();
  NS_ENSURE_SUCCESS(rv, rv);

  // Reset the playing-video dataremote in case the user was previously
  // playing video
  rv = ResetPlayingVideoDataRemote();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = Setup(aURI);
  if(rv == NS_ERROR_ABORT) {
    NS_WARNING("Someone aborted playback of the next track.");
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdatePlayStateDataRemotes();
  NS_ENSURE_SUCCESS(rv, rv);

  mon.Exit();

  rv = StartPlayback();

  if(NS_FAILED(rv)) {
    mon.Enter();

    mStatus = sbIMediacoreStatus::STATUS_STOPPED;
    mIsWaitingForPlayback = PR_FALSE;

    rv = UpdatePlayStateDataRemotes();
    NS_ENSURE_SUCCESS(rv, rv);

    return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::Play()
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);

  // No sequence, no error, but return right away.
  if(!mSequence.size()) {
    return NS_OK;
  }

  mStatus = sbIMediacoreStatus::STATUS_BUFFERING;
  mErrorCount = 0;
  mIsWaitingForPlayback = PR_TRUE;

  // Always reset this data remote, otherwise the video window may get into
  // an unexpected state and not get shown ever again.
  nsresult rv = ResetPlayingVideoDataRemote();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ResetMetadataDataRemotes();
  NS_ENSURE_SUCCESS(rv, rv);

  // never call setup holding the monitor!
  mon.Exit();

  rv = Setup();
  if(rv == NS_ERROR_ABORT) {
    NS_WARNING("Someone aborted playback of the next track.");
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  mon.Enter();

  rv = UpdatePlayStateDataRemotes();
  NS_ENSURE_SUCCESS(rv, rv);

  mon.Exit();

  rv = StartPlayback();

  if(NS_FAILED(rv)) {
    mon.Enter();

    mStatus = sbIMediacoreStatus::STATUS_STOPPED;
    mIsWaitingForPlayback = PR_FALSE;

    rv = UpdatePlayStateDataRemotes();
    NS_ENSURE_SUCCESS(rv, rv);

    return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::Stop() {
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);

  mStatus = sbIMediacoreStatus::STATUS_STOPPED;

  nsresult rv = StopSequenceProcessor();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdatePlayStateDataRemotes();
  NS_ENSURE_SUCCESS(rv, rv);

  if(mPlaybackControl) {
    // Grip.
    nsCOMPtr<sbIMediacorePlaybackControl> playbackControl = mPlaybackControl;
    mon.Exit();

    rv = playbackControl->Stop();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Couldn't stop core.");

    mon.Enter();
  }

  if(mSeenPlaying) {
    rv = mDataRemoteFaceplateSeenPlaying->SetBoolValue(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mSeenPlaying = PR_FALSE;

  nsCOMPtr<sbIMediacoreEvent> event;
  rv = sbMediacoreEvent::CreateEvent(sbIMediacoreEvent::SEQUENCE_END,
                                     nsnull,
                                     nsnull,
                                     mCore,
                                     getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DispatchMediacoreEvent(event);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::OnValidatePlaybackComplete(sbIMediaItem *aItem, 
                                                 PRInt32 aResult) {
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsAutoMonitor mon(mMonitor);
  
  mValidationComplete = PR_TRUE;

  if (aItem == mValidatingItem) {
    switch (aResult) {
      case sbIMediaItemControllerListener::VALIDATEPLAYBACKCOMPLETE_PROCEED:
        if (mOnHoldStatus == ONHOLD_PLAYVIEW ||
            !mNextTriggeredByStreamEnd) {

          if (mOnHoldStatus == ONHOLD_PLAYVIEW) {
            rv = Play();
            NS_ENSURE_SUCCESS(rv, rv);
          } 

          nsCOMPtr<sbIMediacoreEvent> event;
          rv = sbMediacoreEvent::
            CreateEvent(sbIMediacoreEvent::EXPLICIT_TRACK_CHANGE,
                        nsnull,
                        nsnull,
                        mCore,
                        getter_AddRefs(event));
          NS_ENSURE_SUCCESS(rv, rv);

          rv = DispatchMediacoreEvent(event);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        
        mon.Exit();

        if (mOnHoldStatus != ONHOLD_PLAYVIEW) {
          rv = ProcessNewPosition();
          NS_ENSURE_SUCCESS(rv, rv);
        }
        return NS_OK;
      case sbIMediaItemControllerListener::VALIDATEPLAYBACKCOMPLETE_SKIP:
        // go to the next track in the view
        if (mOnHoldStatus == ONHOLD_PLAYVIEW) {
          mStatus = sbIMediacoreStatus::STATUS_BUFFERING;
          mErrorCount = 0;
          mIsWaitingForPlayback = PR_TRUE;
          mOnHoldStatus = ONHOLD_PLAYVIEW;
        }
        if (mOnHoldStatus == ONHOLD_PREVIOUS)
          return Previous(PR_TRUE);
        return Next(PR_TRUE);
    }
  } 
  // else, a previous validation is done, but the user has triggered a new 
  // playback event that cancelled the one that caused vaidation. Either a
  // new validation is in progress (in which case we'll get its completed
  // notification later with the correct mediaitem), or validation wasn't
  // needed anymore (in which case it is safe to ignore the event)
  return NS_OK;
}

nsresult 
sbMediacoreSequencer::ValidateMediaItemControllerPlayback(PRBool aFromUserAction,
                                                          PRInt32 aOnHoldStatus,
                                                          PRBool *_proceed)
{
  NS_ENSURE_TRUE(NS_IsMainThread(), NS_ERROR_UNEXPECTED);
  NS_ENSURE_ARG_POINTER(_proceed);

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  nsAutoMonitor mon(mMonitor);

  // get the item controller
  nsCOMPtr<sbIMediaItem> mediaItem;
  nsresult rv = mView->GetItemByIndex(mSequence[mPosition], getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItemController> mediaItemController;
  rv = mediaItem->GetItemController(getter_AddRefs(mediaItemController));
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (mediaItemController) {
    // Call validatePlayback on the controller. Set _proceed to false so that our
    // caller doesn't do any more work. The validatePlayback call may synchronously
    // call completion callback and thus this won't always cause playback to pause.
    mOnHoldStatus = aOnHoldStatus;
    mValidatingItem = mediaItem;
    mValidationComplete = PR_FALSE;
    rv = mediaItemController->ValidatePlayback(mediaItem, aFromUserAction, this);
    *_proceed = PR_FALSE;
    return rv;
  }
  mOnHoldStatus = ONHOLD_NOTONHOLD;
  mValidatingItem.forget();
  *_proceed = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::Next(PRBool aNotFromUserAction)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsAutoMonitor mon(mMonitor);

  // No sequence, no error, return early.
  if(!mSequence.size()) {
    return NS_OK;
  }

  PRBool hasNext = PR_FALSE;
  PRUint32 length = mSequence.size();

  if((mRepeatMode == sbIMediacoreSequencer::MODE_REPEAT_ONE) &&
     mNextTriggeredByStreamEnd) {
    // Repeat the item only if the next action was the result of the stream
    // ending.  Don't touch sequencer state.  Don't even clear
    // mPositionInvalidated.  Doing so can cause problems if the view changed.
    hasNext = PR_TRUE;
  }
  else if(mPositionInvalidated) {
    // The position of the currently playing item is invalid because we had to
    // regenerate a sequence that didn't include the currently playing item.
    // The next item should be at the current position instead of the current
    // position + 1.
    mViewPosition = mSequence[mPosition];
    hasNext = PR_TRUE;
    mPositionInvalidated = PR_FALSE;
  }
  else if(mRepeatMode == sbIMediacoreSequencer::MODE_REPEAT_ALL &&
     mPosition + 1 >= length) {
    mPosition = 0;
    mViewPosition = mSequence[mPosition];
    hasNext = PR_TRUE;

    if(mMode == sbIMediacoreSequencer::MODE_SHUFFLE ||
       mMode == sbIMediacoreSequencer::MODE_CUSTOM) {
      nsresult rv = RecalculateSequence();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else if(mRepeatMode == sbIMediacoreSequencer::MODE_REPEAT_ONE) {
    hasNext = PR_TRUE;

    // The next action was not the result of the stream ending, so act like
    // repeat all.
    if(mPosition + 1 >= length) {
      mPosition = 0;
    }
    else if(mPosition + 1 < length) {
      ++mPosition;
    }
    mViewPosition = mSequence[mPosition];
  }
  else if(mPosition + 1 < length) {
    ++mPosition;
    mViewPosition = mSequence[mPosition];
    hasNext = PR_TRUE;
  }

  // No next track, not an error.
  if(!hasNext) {
    // No next track and playing, stop.
    if(mStatus == sbIMediacoreStatus::STATUS_PLAYING ||
       mStatus == sbIMediacoreStatus::STATUS_PAUSED ||
       mStatus == sbIMediacoreStatus::STATUS_BUFFERING) {
      // Grip.
      nsCOMPtr<sbIMediacorePlaybackControl> playbackControl = mPlaybackControl;
      mon.Exit();
      if (playbackControl) {
        rv = playbackControl->Stop();
        NS_ASSERTION(NS_SUCCEEDED(rv), "Stop failed at end of sequence.");
      }
      mon.Enter();
    }

    mStatus = sbIMediacoreStatus::STATUS_STOPPED;

    rv = StopSequenceProcessor();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = UpdatePlayStateDataRemotes();
    NS_ENSURE_SUCCESS(rv, rv);

    if(mSeenPlaying) {
      mSeenPlaying = PR_FALSE;

      rv = mDataRemoteFaceplateSeenPlaying->SetBoolValue(PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<sbIMediacoreEvent> event;
    rv = sbMediacoreEvent::CreateEvent(sbIMediacoreEvent::SEQUENCE_END,
                                       nsnull,
                                       nsnull,
                                       mCore,
                                       getter_AddRefs(event));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = DispatchMediacoreEvent(event);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }
  
  PRBool proceed;
  rv = ValidateMediaItemControllerPlayback(!aNotFromUserAction, ONHOLD_NEXT, &proceed);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (!proceed) {
    return NS_OK;
  }

  // Fire EXPLICIT_TRACK_CHANGE when Next() was not triggered by the stream 
  // ending.
  if(!mNextTriggeredByStreamEnd) {
    nsCOMPtr<sbIMediacoreEvent> event;
    rv = sbMediacoreEvent::
      CreateEvent(sbIMediacoreEvent::EXPLICIT_TRACK_CHANGE,
                  nsnull,
                  nsnull,
                  mCore,
                  getter_AddRefs(event));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = DispatchMediacoreEvent(event);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mon.Exit();

  rv = ProcessNewPosition();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::Previous(PRBool aNotFromUserAction)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsAutoMonitor mon(mMonitor);

  // No sequence, no error, return early.
  if(!mSequence.size()) {
    return NS_OK;
  }

  PRBool hasNext = PR_FALSE;
  PRUint32 length = mSequence.size();
  PRInt64 position = mPosition;

  if(mRepeatMode == sbIMediacoreSequencer::MODE_REPEAT_ALL &&
     position - 1 < 0) {
      mPosition = length - 1;
      mViewPosition = mSequence[mPosition];
      hasNext = PR_TRUE;

      if(mMode == sbIMediacoreSequencer::MODE_SHUFFLE ||
         mMode == sbIMediacoreSequencer::MODE_CUSTOM) {
          nsresult rv = RecalculateSequence();
          NS_ENSURE_SUCCESS(rv, rv);
      }
  }
  else if(mRepeatMode == sbIMediacoreSequencer::MODE_REPEAT_ONE) {
    hasNext = PR_TRUE;

    if(!mNextTriggeredByStreamEnd) {
      if(position - 1 < 0) {
        mPosition = length - 1;
      }
      else if(position - 1 >= 0) {
        --mPosition;
      }
      mViewPosition = mSequence[mPosition];
    }
  }
  else if(position - 1 >= 0) {
    --mPosition;
    mViewPosition = mSequence[mPosition];
    hasNext = PR_TRUE;
  }

  // No next track, not an error.
  if(!hasNext) {
    // No next track and playing, stop.
    if(mStatus == sbIMediacoreStatus::STATUS_PLAYING ||
       mStatus == sbIMediacoreStatus::STATUS_PAUSED ||
       mStatus == sbIMediacoreStatus::STATUS_BUFFERING) {
      // Grip.
      nsCOMPtr<sbIMediacorePlaybackControl> playbackControl = mPlaybackControl;
      mon.Exit();
      rv = playbackControl->Stop();
      NS_ASSERTION(NS_SUCCEEDED(rv), "Stop failed at end of sequence.");
      mon.Enter();
    }

    mStatus = sbIMediacoreStatus::STATUS_STOPPED;

    rv = StopSequenceProcessor();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = UpdatePlayStateDataRemotes();
    NS_ENSURE_SUCCESS(rv, rv);

    if(mSeenPlaying) {
      mSeenPlaying = PR_FALSE;

      rv = mDataRemoteFaceplateSeenPlaying->SetBoolValue(PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<sbIMediacoreEvent> event;
    rv = sbMediacoreEvent::CreateEvent(sbIMediacoreEvent::SEQUENCE_END,
                                       nsnull,
                                       nsnull,
                                       mCore,
                                       getter_AddRefs(event));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = DispatchMediacoreEvent(event);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  PRBool proceed;
  rv = ValidateMediaItemControllerPlayback(!aNotFromUserAction, ONHOLD_PREVIOUS, &proceed);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (!proceed) {
    return NS_OK;
  }

  // Fire EXPLICIT_TRACK_CHANGE when Previous() was not triggered by the stream
  // ending.
  if(!mNextTriggeredByStreamEnd) {
    nsCOMPtr<sbIMediacoreEvent> event;
    rv = sbMediacoreEvent::
      CreateEvent(sbIMediacoreEvent::EXPLICIT_TRACK_CHANGE,
                  nsnull,
                  nsnull,
                  mCore,
                  getter_AddRefs(event));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = DispatchMediacoreEvent(event);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mon.Exit();

  rv = ProcessNewPosition();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::RequestHandleNextItem(sbIMediacore *aMediacore)
{
  // lone> Note that this method does not perform mediaitem trackType 
  // service validation. This is only used for gapless playback (one 
  // core to the same core), so the assumption here is that validated
  // playback on one track for a service gives access to all tracks
  // from that service. This is good enough for now but may not be anymore
  // at some point in the future. Fixing this will imply making cores
  // aware of the ability of the sequencer to set itself 'on hold'. When
  // this method (RequestHandleNextItem) is called, cores need to be able
  // to receive a "hold on, i don't know what track is going to be next yet"
  // answer, and call back into the sequencer later (after some event)
  // for the actual handling of the gapless next event. This means the
  // core could potentially run to the end of the stream before the
  // event has occured (slow login), and that could also have
  // implications.
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aMediacore);

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsAutoMonitor mon(mMonitor);
  if(mIsWaitingForPlayback) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  mon.Exit();

  nsCOMPtr<sbIMediaItem> item;
  rv = GetNextItem(getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  rv = item->GetContentSrc(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  mon.Enter();
  NS_ENSURE_TRUE(mCore == aMediacore, NS_ERROR_INVALID_ARG);

  nsCOMPtr<sbIMediacoreVoting> voting =
    do_QueryReferent(mMediacoreManager, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(voting, NS_ERROR_UNEXPECTED);

  mon.Exit();

  nsCOMPtr<sbIMediacoreVotingChain> votingChain;
  rv = voting->VoteWithURI(uri, getter_AddRefs(votingChain));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool validChain = PR_FALSE;
  rv = votingChain->GetValid(&validChain);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(validChain, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIArray> chain;
  rv = votingChain->GetMediacoreChain(getter_AddRefs(chain));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediacore> core = do_QueryElementAt(chain, 0, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mon.Enter();

  NS_ENSURE_TRUE(core == mCore, NS_ERROR_INVALID_ARG);
  mCoreWillHandleNext = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::Abort()
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);

  // If we can't abort right now, just return NS_OK.
  NS_ENSURE_TRUE(mCanAbort, NS_OK);

  mShouldAbort = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::GetCustomGenerator(
                        sbIMediacoreSequenceGenerator * *aCustomGenerator)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aCustomGenerator);

  nsAutoMonitor mon(mMonitor);
  NS_IF_ADDREF(*aCustomGenerator = mCustomGenerator);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::SetCustomGenerator(
                        sbIMediacoreSequenceGenerator * aCustomGenerator)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aCustomGenerator);

  nsAutoMonitor mon(mMonitor);
  if(mCustomGenerator != aCustomGenerator) {
    mCustomGenerator = aCustomGenerator;

    // Custom generator changed and mode is custom, recalculate sequence
    if(mMode == sbIMediacoreSequencer::MODE_CUSTOM) {
      PRInt64 viewPosition = mViewPosition;
      nsresult rv = RecalculateSequence(&viewPosition);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
//  sbIMediacoreEventListener
//------------------------------------------------------------------------------

/* void onMediacoreEvent (in sbIMediacoreEvent aEvent); */
NS_IMETHODIMP
sbMediacoreSequencer::OnMediacoreEvent(sbIMediacoreEvent *aEvent)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aEvent);

  nsCOMPtr<sbIMediacore> core;
  nsresult rv = aEvent->GetOrigin(getter_AddRefs(core));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 eventType = 0;
  rv = aEvent->GetType(&eventType);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef PR_LOGGING
  nsString coreName;
  rv = core->GetInstanceName(coreName);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("[sbMediacoreSequencer] - Event from core '%s', type %08x",
        NS_ConvertUTF16toUTF8(coreName).BeginReading(), eventType));
#endif

  nsAutoMonitor mon(mMonitor);
  if(mCore != core) {
    return NS_OK;
  }

  if(!(eventType == sbIMediacoreEvent::STREAM_STOP &&
       mStopTriggeredBySequencer)) {
    nsCOMPtr<sbIMediacoreEventTarget> target =
      do_QueryReferent(mMediacoreManager, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool dispatched;
    rv = target->DispatchEvent(aEvent, PR_TRUE, &dispatched);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  mon.Exit();

  switch(eventType) {

    // Stream Events
    case sbIMediacoreEvent::STREAM_START: {
      mon.Enter();

      if(mStatus == sbIMediacoreStatus::STATUS_BUFFERING &&
         mIsWaitingForPlayback) {
        mErrorCount = 0;
        mIsWaitingForPlayback = PR_FALSE;
        mStopTriggeredBySequencer = PR_FALSE;
        mStatus = sbIMediacoreStatus::STATUS_PLAYING;

        LOG(("[sbMediacoreSequencer] - Was waiting for playback, playback now started."));
      }

      if(mStatus == sbIMediacoreStatus::STATUS_PAUSED) {
         mStatus = sbIMediacoreStatus::STATUS_PLAYING;
      }

      if(mCoreWillHandleNext) {
        sbScopedBoolToggle toggle(&mNextTriggeredByStreamEnd);
        nsresult rv = Next(PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      mon.Exit();

      rv = UpdatePlayStateDataRemotes();
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mDataRemoteFaceplateBuffering->SetBoolValue(PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);

      if(!mSeenPlaying) {
        mSeenPlaying = PR_TRUE;

        rv = mDataRemoteFaceplateSeenPlaying->SetBoolValue(PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      // we may wish to restore the video position
      if (mResumePlaybackPosition) {
        NS_ENSURE_STATE(mCurrentItem);
        NS_NAMED_LITERAL_STRING(PROPERTY_LAST_POSITION,
                                SB_PROPERTY_LASTPLAYPOSITION);
        nsString lastPositionStr;
        rv = mCurrentItem->GetProperty(PROPERTY_LAST_POSITION, lastPositionStr);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = mCurrentItem->SetProperty(PROPERTY_LAST_POSITION, SBVoidString());
        NS_ENSURE_SUCCESS(rv, rv);
        PRUint64 lastPosition = nsString_ToUint64(lastPositionStr);
        if (lastPosition > 0) {
          TRACE(("%s[%p] - seeking to %llu (%s)",
                 __FUNCTION__, this, lastPosition,
                 NS_LossyConvertUTF16toASCII(lastPositionStr).get()));
          nsRefPtr<sbRunnableMethod1<sbMediacoreSequencer, nsresult, PRUint64> > runnable;
          rv = sbRunnableMethod1<sbMediacoreSequencer, nsresult, PRUint64>
                                ::New(getter_AddRefs(runnable),
                                      this,
                                      &sbMediacoreSequencer::SeekCallback,
                                      NS_ERROR_FAILURE,
                                      lastPosition);
          NS_ENSURE_SUCCESS(rv, rv);
          rv = NS_DispatchToCurrentThread(runnable);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }

    }
    break;

    case sbIMediacoreEvent::STREAM_PAUSE: {

      mon.Enter();
      mStatus = sbIMediacoreStatus::STATUS_PAUSED;
      mStopTriggeredBySequencer = PR_FALSE;
      mon.Exit();

      rv = UpdatePlayStateDataRemotes();
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mDataRemoteFaceplateBuffering->SetBoolValue(PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    case sbIMediacoreEvent::STREAM_END: {
      rv = mDataRemoteFaceplatePlayingVideo->SetBoolValue(PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);

      mon.Enter();
      /* Track done, continue on to the next, if possible. */
      if(mStatus == sbIMediacoreStatus::STATUS_PLAYING &&
         !mIsWaitingForPlayback) {

        mCoreWillHandleNext = PR_FALSE;

        sbScopedBoolToggle toggle(&mNextTriggeredByStreamEnd);
        rv = Next(PR_TRUE);

        if(NS_FAILED(rv) ||
           mSequence.empty()) {
          mon.Exit();
          Stop();
          mon.Enter();
        }

        LOG(("[sbMediacoreSequencer] - Was playing, stream ended, attempting to go to next track in sequence."));
      }
      mon.Exit();
    }
    break;

    case sbIMediacoreEvent::STREAM_BEFORE_STOP: {
      nsCOMPtr<nsIVariant> data;
      rv = aEvent->GetData(getter_AddRefs(data));
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_STATE(data);
      rv = UpdateLastPositionProperty(mCurrentItem, data);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    case sbIMediacoreEvent::STREAM_STOP: {
      mon.Enter();

      if(!mStopTriggeredBySequencer) {
        LOG(("[sbMediacoreSequencer] - Hard stop requested."));
        /* If we're explicitly stopped, don't continue to the next track,
        * just clean up... */
        mStatus = sbIMediacoreStatus::STATUS_STOPPED;
        mon.Exit();

        rv = StopSequenceProcessor();
        NS_ENSURE_SUCCESS(rv, rv);

        rv = UpdatePlayStateDataRemotes();
        NS_ENSURE_SUCCESS(rv, rv);

        rv = ResetMetadataDataRemotes();
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mDataRemoteFaceplatePlayingVideo->SetBoolValue(PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);

        mon.Enter();
        if(mSeenPlaying) {
          mSeenPlaying = PR_FALSE;
          mon.Exit();

          rv = mDataRemoteFaceplateSeenPlaying->SetBoolValue(PR_FALSE);
          NS_ENSURE_SUCCESS(rv, rv);

          mon.Enter();
        }
      }
      else {
        mStopTriggeredBySequencer = PR_FALSE;
      }
      mon.Exit();
    }
    break;

    case sbIMediacoreEvent::STREAM_HAS_VIDEO: {
      rv = mDataRemoteFaceplatePlayingVideo->SetBoolValue(PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    case sbIMediacoreEvent::BUFFERING: {
      PRBool buffering = PR_FALSE;

      rv = mDataRemoteFaceplateBuffering->GetBoolValue(&buffering);
      NS_ENSURE_SUCCESS(rv, rv);

      if(!buffering) {
        rv = mDataRemoteFaceplateBuffering->SetBoolValue(PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      if(!mSeenPlaying) {
        mSeenPlaying = PR_TRUE;

        rv = mDataRemoteFaceplateSeenPlaying->SetBoolValue(PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    break;

    // Error Events
    case sbIMediacoreEvent::ERROR_EVENT: {
      mon.Enter();
      mStopTriggeredBySequencer = PR_FALSE;
      mon.Exit();

      rv = HandleErrorEvent(aEvent);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    // Misc events
    case sbIMediacoreEvent::DURATION_CHANGE: {
    }
    break;

    case sbIMediacoreEvent::METADATA_CHANGE: {
      rv = HandleMetadataEvent(aEvent);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    case sbIMediacoreEvent::MUTE_CHANGE: {
      rv = HandleMuteChangeEvent(aEvent);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    case sbIMediacoreEvent::VOLUME_CHANGE: {
      rv = HandleVolumeChangeEvent(aEvent);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    default:;
  }

  return NS_OK;
}

nsresult
sbMediacoreSequencer::SeekCallback(PRUint64 aPosition)
{
  TRACE(("%s[%p](%llu)", __FUNCTION__, this, aPosition));
  nsresult rv;
  #ifdef PR_LOGGING
  {
    nsString url;
    rv = mDataRemoteMetadataURL->GetStringValue(url);
    TRACE(("%s[%p] - url is %s", __FUNCTION__, this,
           NS_SUCCEEDED(rv) ? NS_LossyConvertUTF16toASCII(url).get()
                            : "<none>"));
  }
  #endif
  NS_ENSURE_STATE(mPlaybackControl);
  rv = mPlaybackControl->SetPosition(aPosition);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

// -----------------------------------------------------------------------------
// sbIMediacoreStatus
// -----------------------------------------------------------------------------
NS_IMETHODIMP
sbMediacoreSequencer::GetState(PRUint32 *aState)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);
  *aState = mStatus;

  return NS_OK;
}

// -----------------------------------------------------------------------------
// sbIMediaListListener
// -----------------------------------------------------------------------------
NS_IMETHODIMP
sbMediacoreSequencer::OnItemAdded(sbIMediaList *aMediaList,
                                  sbIMediaItem *aMediaItem,
                                  PRUint32 aIndex,
                                  PRBool *_retval)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsAutoMonitor mon(mMonitor);

  // 2nd part of smart playlist rebuild detection: are we adding
  // items in the same batch as we cleared the list in ?

  if (aMediaList == mViewList && mListBatchCount) {
    if (mSmartRebuildDetectBatchCount == mListBatchCount) {
      // Our playing list is a smart playlist, and it is being rebuilt,
      // so make a note that we need to try to find the old playitem in
      // the new list (this will update the now playing icon in
      // tree views, and ensure that currentIndex returns the new index).
      // The 1st part of the detection has already scheduled a check, but
      // our search will occur before the check happens, so if the old
      // playing item no longer exists, playback will correctly stop.
      mNeedSearchPlayingItem = PR_TRUE;
    }
    else {
      mNeedsRecalculate = PR_TRUE;
    }
  }
  else {
    mNeedsRecalculate = PR_TRUE;

    nsresult rv = UpdateItemUIDIndex();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::OnBeforeItemRemoved(sbIMediaList *aMediaList,
                                          sbIMediaItem *aMediaItem,
                                          PRUint32 aIndex,
                                          PRBool *_retval)
{
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::OnAfterItemRemoved(sbIMediaList *aMediaList,
                                         sbIMediaItem *aMediaItem,
                                         PRUint32 aIndex,
                                         PRBool *_retval)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsAutoMonitor mon(mMonitor);

  PRBool listEvent = (aMediaList == mViewList);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<sbILibrary> library = do_QueryInterface(aMediaList, &rv);

  PRBool libraryEvent = PR_FALSE;
  if(!mViewIsLibrary && NS_SUCCEEDED(rv)) {
    libraryEvent = PR_TRUE;
  }

  // if this is an event on the library...
  if (libraryEvent) {

    // and the item is not our list, get more events if in a batch, or
    // just discard event if not in a batch.
    nsCOMPtr<sbIMediaItem> item = do_QueryInterface(mViewList, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (aMediaItem != item) {
      *_retval = PR_FALSE;
      return NS_OK;
    } else {
      if(mPlaybackControl) {
        // Grip.
        nsCOMPtr<sbIMediacorePlaybackControl> playbackControl = mPlaybackControl;
        mon.Exit();

        // if the item is our list, stop playback now and shutdown watcher
        rv = playbackControl->Stop();
        NS_ENSURE_SUCCESS(rv, rv);

        mon.Enter();
      }

      mStatus = sbIMediacoreStatus::STATUS_STOPPED;

      rv = StopSequenceProcessor();
      NS_ENSURE_SUCCESS(rv, rv);

      rv = UpdatePlayStateDataRemotes();
      NS_ENSURE_SUCCESS(rv, rv);

      rv = StopWatchingView();
      NS_ENSURE_SUCCESS(rv, rv);

      // return value does not actually matter since shutdown removes our
      // listener
      *_retval = PR_FALSE;
      return NS_OK;
    }
  }
  // if this is a track being removed from our list, and we're in a batch,
  // don't get anymore events for this batch, we'll do the check when it
  // ends
  if (listEvent && mListBatchCount > 0) {
    // remember that we need to do a check when batch ends
    mNeedSearchPlayingItem = PR_TRUE;
    *_retval = PR_TRUE;

    return NS_OK;
  }

  // we have to delay the check for currentIndex, because its invalidation
  // relies on a medialistlistener (in the view) which may occur after
  // ours has been issued, in which case it will still be valid now.
  rv = DelayedCheck();
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::OnItemUpdated(sbIMediaList *aMediaList,
                                    sbIMediaItem *aMediaItem,
                                    sbIPropertyArray *aProperties,
                                    PRBool *_retval)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsAutoMonitor mon(mMonitor);

  nsCOMPtr<sbIMediaItem> item;
  nsresult rv = GetCurrentItem(getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  if(aMediaItem == item) {
    rv = SetMetadataDataRemotesFromItem(item, aProperties);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if(!mSmartRebuildDetectBatchCount) {
    // Not only do we need to recalculate the sequence but we
    // also have to search for the playing item after a delay
    // to allow the metadata batch editor to settle down. UGH!

    // First check to see if the metadata actually has any effect on the view
    // If the modified metadata is not present in the view's cascade filter set
    // or sort array there is no need to recalculate the sequence
    if (!CheckPropertiesInfluenceView(aProperties)) {
      return NS_OK;
    }

    mNeedsRecalculate = PR_TRUE;
    mNeedSearchPlayingItem = PR_TRUE;

    // We must wait if there is a batch because rebuilding would
    // end up picking the wrong item.
    if(!mListBatchCount && !mLibraryBatchCount) {
      rv = DelayedCheck();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Suppress this notification for the remainder of the batch
    *_retval = PR_TRUE;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::OnItemMoved(sbIMediaList *aMediaList,
                                  PRUint32 aFromIndex,
                                  PRUint32 aToIndex,
                                  PRBool *_retval)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsAutoMonitor mon(mMonitor);

  if (aMediaList == mViewList && mListBatchCount) {
    if (mSmartRebuildDetectBatchCount == mListBatchCount) {
      mNeedSearchPlayingItem = PR_TRUE;
    }
    else {
      mNeedsRecalculate = PR_TRUE;
    }
  }
  else {
    mNeedsRecalculate = PR_TRUE;

    nsresult rv = UpdateItemUIDIndex();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::OnBeforeListCleared(sbIMediaList* aMediaList,
                                          PRBool aExcludeLists,
                                          PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  // Don't care
  *aNoMoreForBatch = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::OnListCleared(sbIMediaList *aMediaList,
                                    PRBool aExcludeLists,
                                    PRBool *_retval)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsAutoMonitor mon(mMonitor);

  // list or library cleared, playing item must have gone away, however
  // the item might be coming back immediately, in which case we want to
  // keep playing it, so delay the check.
  nsresult rv = DelayedCheck();
  NS_ENSURE_SUCCESS(rv, rv);

  // 1st part of smart playlist rebuild detection: is the event
  // occurring on our list and inside a batch ? if 2nd part never
  // happens, it could be a smart playlist rebuild that now has no
  // content, we don't care about that, the item will simply not be
  // found, and playback will correctly stop.
  if(mListBatchCount > 0 && aMediaList == mViewList) {
    mSmartRebuildDetectBatchCount = mListBatchCount;
  }

  *_retval = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::OnBatchBegin(sbIMediaList *aMediaList)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsAutoMonitor mon(mMonitor);

  if(aMediaList == mViewList) {
    mListBatchCount++;
  }
  else {
    mLibraryBatchCount++;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::OnBatchEnd(sbIMediaList *aMediaList)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsAutoMonitor mon(mMonitor);

  PRInt32 listBatchCount = mListBatchCount;

  if(aMediaList == mViewList && mListBatchCount > 0) {
    mListBatchCount--;
  }
  else if(mLibraryBatchCount > 0) {
    mLibraryBatchCount--;
  }
  else {
    mNeedsRecalculate = PR_TRUE;
  }

  if(mListBatchCount == 0 || mLibraryBatchCount == 0) {

    if(mNeedSearchPlayingItem) {
      rv = DelayedCheck();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else if(mNeedsRecalculate) {
      rv = UpdateItemUIDIndex();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if(mSmartRebuildDetectBatchCount == listBatchCount) {
    mSmartRebuildDetectBatchCount = 0;
  }

  return NS_OK;
}

PRBool
sbMediacoreSequencer::CheckPropertiesInfluenceView(sbIPropertyArray *aProperties)
{
  PRUint32 propertyCount = 0;
  nsresult rv = aProperties->GetLength(&propertyCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mPropertyManager) {
    mPropertyManager =
      do_GetService("@songbirdnest.com/Songbird/Properties/PropertyManager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbICascadeFilterSet>  cfs;
  PRUint16                       filterCount = 0;
  nsCOMPtr<sbILibraryConstraint> constraint;
  PRUint32                       constraintGroupCount = 0;

  nsCOMPtr<sbIFilterableMediaListView>
    filterableView = do_QueryInterface(mView);
  if (filterableView) {
    rv = filterableView->GetFilterConstraint(getter_AddRefs(constraint));
    NS_ENSURE_SUCCESS(rv, rv);
    if (constraint) {
      rv = constraint->GetGroupCount(&constraintGroupCount);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else {
    rv = mView->GetCascadeFilterSet(getter_AddRefs(cfs));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = cfs->GetLength(&filterCount);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbISortableMediaListView> sortableView;
  sortableView = do_QueryInterface(mView, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyArray> primarySortArray;
  rv = sortableView->GetCurrentSort(getter_AddRefs(primarySortArray));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 primarySortCount = 0;
  rv = primarySortArray->GetLength(&primarySortCount);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIProperty> property;
  for(PRUint32 propIndex = 0; propIndex < propertyCount; ++propIndex) {
    rv = aProperties->GetPropertyAt(propIndex, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString id;
    rv = property->GetId(id);
    NS_ENSURE_SUCCESS(rv, rv);

    // Check the filter constraint
    if (constraint) {
      for (PRUint32 groupIndex = 0;
           groupIndex < constraintGroupCount;
           ++groupIndex) {
        nsCOMPtr<sbILibraryConstraintGroup> group;
        rv = constraint->GetGroup(groupIndex, getter_AddRefs(group));
        NS_ENSURE_SUCCESS(rv, rv);

        PRBool hasProperty;
        rv = group->HasProperty(id, &hasProperty);
        NS_ENSURE_SUCCESS(rv, rv);

        if (hasProperty)
          return PR_TRUE;
      }
    }

    // Check the cascade filter set
    if (cfs) {
      for (PRUint16 filterIndex = 0; filterIndex < filterCount; ++filterIndex) {
        nsString filter;
        rv = cfs->GetProperty(filterIndex, filter);
        NS_ENSURE_SUCCESS(rv, rv);

        if (id.Equals(filter)) {
          // The property is present in the cascade filter set
          return PR_TRUE;
        }
      }
    }

    // Check the primary sort array
    nsCOMPtr<sbIProperty> sortProperty;
    for (PRUint32 sortIndex = 0; sortIndex < primarySortCount; ++sortIndex) {
      rv = primarySortArray->GetPropertyAt(sortIndex, getter_AddRefs(sortProperty));
      NS_ENSURE_SUCCESS(rv, rv);

      nsString SortId;
      rv = sortProperty->GetId(SortId);
      NS_ENSURE_SUCCESS(rv, rv);

      if (id.Equals(SortId)) {
        // The property is present in the primary sort
        return PR_TRUE;
      }
    }

    // Check secondary sorting
    nsCOMPtr<sbIPropertyInfo> propertyInfo;
    rv = mPropertyManager->GetPropertyInfo(id, getter_AddRefs(propertyInfo));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIPropertyArray> secondarySortArray;
    rv = propertyInfo->GetSecondarySort(getter_AddRefs(secondarySortArray));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!secondarySortArray) {
      // This property doesn't have secondary sorting
      continue;
    }

    PRUint32 secondarySortCount = 0;
    rv = secondarySortArray->GetLength(&secondarySortCount);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 sortIndex = 0; sortIndex < secondarySortCount; ++sortIndex) {
      rv = secondarySortArray->GetPropertyAt(sortIndex, getter_AddRefs(sortProperty));
      NS_ENSURE_SUCCESS(rv, rv);

      nsString SortId;
      rv = sortProperty->GetId(SortId);
      NS_ENSURE_SUCCESS(rv, rv);

      if (id.Equals(SortId)) {
        // The property is present in the secondary sort
        return PR_TRUE;
      }
    }
  }

  // No properties were present in the cascade filter set, primary sort array or
  // secondary sort array
  return PR_FALSE;
}


// -----------------------------------------------------------------------------
// sbIMediaListViewListener
// -----------------------------------------------------------------------------
NS_IMETHODIMP
sbMediacoreSequencer::OnFilterChanged(sbIMediaListView *aChangedView)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);

  sbScopedBoolToggle reset(&mResetPosition);
  mNeedsRecalculate = PR_TRUE;

  nsresult rv = UpdateItemUIDIndex();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::OnSearchChanged(sbIMediaListView *aChangedView)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);

  sbScopedBoolToggle reset(&mResetPosition);
  mNeedsRecalculate = PR_TRUE;

  nsresult rv = UpdateItemUIDIndex();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::OnSortChanged(sbIMediaListView *aChangedView)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);

  mNeedsRecalculate = PR_TRUE;

  nsresult rv = UpdateItemUIDIndex();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


// -----------------------------------------------------------------------------
// nsITimerCallback
// -----------------------------------------------------------------------------

NS_IMETHODIMP
sbMediacoreSequencer::Notify(nsITimer *timer)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(timer);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsAutoMonitor mon(mMonitor);

  if(timer == mSequenceProcessorTimer) {
    rv = HandleSequencerTimer(timer);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if(timer == mDelayedCheckTimer) {
    rv = HandleDelayedCheckTimer(timer);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbMediacoreSequencer::HandleSequencerTimer(nsITimer *aTimer)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aTimer);

  nsresult rv = NS_ERROR_UNEXPECTED;
  PRUint64 position = 0;

  // Update the position in the position data remote.
  if(mStatus == sbIMediacoreStatus::STATUS_PLAYING ||
     mStatus == sbIMediacoreStatus::STATUS_PAUSED) {
    rv = mPlaybackControl->GetPosition(&position);
    if(NS_SUCCEEDED(rv)) {
      rv = UpdatePositionDataRemotes(position);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if(mStatus == sbIMediacoreStatus::STATUS_PLAYING ||
     mStatus == sbIMediacoreStatus::STATUS_BUFFERING ||
     mStatus == sbIMediacoreStatus::STATUS_PAUSED) {

    PRUint64 duration = 0;
    rv = mPlaybackControl->GetDuration(&duration);
    if(NS_SUCCEEDED(rv)) {
      rv = UpdateDurationDataRemotes(duration);
      NS_ENSURE_SUCCESS(rv, rv);

      // only update the duration if we've played at least 5% of the
      // the track. This is to allow the duration to stabilize for
      // vbr files without proper headers.
      if((position > 0) && position > ((duration * 5) / 100)) {
        // only updates if there is a current item and the duration
        // reported by the core is different than the one the item
        // currently has.
        rv = UpdateCurrentItemDuration(duration);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  return NS_OK;
}

nsresult
sbMediacoreSequencer::HandleDelayedCheckTimer(nsITimer *aTimer)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_STATE(mDelayedCheckTimer);

  nsAutoMonitor mon(mMonitor);
  mDelayedCheckTimer = nsnull;

  PRUint32 viewLength = 0;
  nsresult rv = mView->GetLength(&viewLength);
  NS_ENSURE_SUCCESS(rv, rv);

  if(mSequence.size() != viewLength) {
    mNeedsRecalculate = PR_TRUE;
  }

  rv = UpdateItemUIDIndex();
  NS_ENSURE_SUCCESS(rv, rv);

  mNeedSearchPlayingItem = PR_FALSE;

  return NS_OK;
}
