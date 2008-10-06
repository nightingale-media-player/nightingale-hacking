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

#include "sbMediacoreSequencer.h"

#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsISupportsPrimitives.h>
#include <nsIURL.h>
#include <nsIVariant.h>
#include <nsIWeakReferenceUtils.h>

#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsArrayUtils.h>
#include <nsComponentManagerUtils.h>

#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <nsTArray.h>

#include <prtime.h>

#include <sbILibrary.h>
#include <sbIMediacore.h>
#include <sbIMediacoreEvent.h>
#include <sbIMediacoreEventTarget.h>
#include <sbIMediacorePlaybackControl.h>
#include <sbIMediacoreStatus.h>
#include <sbIMediacoreVoting.h>
#include <sbIMediacoreVotingChain.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbIPropertyArray.h>

#include <sbMediacoreEvent.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>
#include <sbTArrayStringEnumerator.h>
#include <sbVariantUtils.h>

#include "sbMediacoreDataRemotes.h"
#include "sbMediacoreShuffleSequenceGenerator.h"

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

NS_IMPL_QUERY_INTERFACE6_CI(sbMediacoreSequencer, 
                            sbIMediacoreSequencer,
                            sbIMediacoreStatus,
                            sbIMediaListListener,
                            sbIMediaListViewListener,
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
, mMode(sbIMediacoreSequencer::MODE_FORWARD)
, mRepeatMode(sbIMediacoreSequencer::MODE_REPEAT_NONE)
, mPosition(0)
, mViewPosition(0)
, mCurrentItemIndex(0)
, mListBatchCount(0)
, mLibraryBatchCount(0)
, mSmartRebuildDetectBatchCount(0)
, mNeedCheck(PR_FALSE)
, mViewIsLibrary(PR_FALSE)
, mNeedSearchPlayingItem(PR_FALSE)
{
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

  return NS_OK;
}

nsresult 
sbMediacoreSequencer::StartSequenceProcessor()
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mSequenceProcessorTimer, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = 
    mSequenceProcessorTimer->InitWithCallback(this, 
                                              100, 
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

  rv = mDataRemoteFaceplateBuffering->SetIntValue(0);
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

  // Playlist Shuffle
  mDataRemotePlaylistShuffle = 
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemotePlaylistShuffle->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_PLAYLIST_SHUFFLE),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  // Playlist Repeat
  mDataRemotePlaylistRepeat = 
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemotePlaylistRepeat->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_PLAYLIST_REPEAT),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult 
sbMediacoreSequencer::UnbindDataRemotes()
{
  //
  // Faceplate DataRemotes
  //
  nsresult rv = mDataRemoteFaceplateBuffering->Unbind();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplatePlaying->Unbind();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplatePaused->Unbind();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplateSeenPlaying->Unbind();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplateURL->Unbind();
  NS_ENSURE_SUCCESS(rv, rv);

  //
  // Metadata DataRemotes
  //

  rv = mDataRemoteMetadataAlbum->Unbind();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataArtist->Unbind();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataGenre->Unbind();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataTitle->Unbind();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataDuration->Unbind();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataPosition->Unbind();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataPositionStr->Unbind();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataURL->Unbind();
  NS_ENSURE_SUCCESS(rv, rv);

  //
  // Playlist DataRemotes
  //

  rv = mDataRemotePlaylistShuffle->Unbind();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemotePlaylistRepeat->Unbind();
  NS_ENSURE_SUCCESS(rv, rv);

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
    NS_ENSURE_SUCCESS(rv, rv);

    duration -= position;
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

#if defined(DEBUG)
  {
    NS_ConvertUTF16toUTF8 id(aId);
    NS_ConvertUTF16toUTF8 value(aValue);

    printf("[sbMediacoreSequencer] - SetMetadataDataRemote\n\tId: %s\n\tValue: %s\n\n",
           id.BeginReading(),
           value.BeginReading());
  }
#endif

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
  
  if(remote) {
    nsresult rv = remote->SetStringValue(aValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult 
sbMediacoreSequencer::SetMetadataDataRemotesFromItem(sbIMediaItem *aItem)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aItem);

  nsString albumName, artistName, genre, trackName;
  nsresult rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME), 
                                   albumName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME), 
                          artistName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_GENRE), genre);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME), trackName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataAlbum->SetStringValue(albumName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataArtist->SetStringValue(artistName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataGenre->SetStringValue(genre);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteMetadataTitle->SetStringValue(trackName);
  NS_ENSURE_SUCCESS(rv, rv);

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

  return NS_OK;
}

nsresult 
sbMediacoreSequencer::RecalculateSequence(PRUint32 *aViewPosition /*= nsnull*/)
{
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
  mViewIndexToSequenceIndex.reserve(length);

  switch(mMode) {
    case sbIMediacoreSequencer::MODE_FORWARD:
    {
      // Generate forward sequence
      for(PRUint32 i = 0; i < length; ++i) {
        mSequence.push_back(i);
        mViewIndexToSequenceIndex.push_back(i);
      }

      if(aViewPosition) {
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
        mViewIndexToSequenceIndex.push_back(j);
      }

      if(aViewPosition) {
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

        if(aViewPosition &&
           *aViewPosition == sequence[i]) {
          // Swap the first position item with the item that was selected by the
          // user to play first.
          PRUint32 index = mSequence[0];
          mSequence[0] = mSequence[i];
          mSequence[i] = index;
        }
      }

      delete [] sequence;
    }
    break;
    case sbIMediacoreSequencer::MODE_CUSTOM:
    {
      // XXXAus: Use custom generator.
      // XXXAus: Match view position if present.
    }
    break;
  }

  mViewPosition = mSequence[mPosition];

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
  nsresult rv = ResetMetadataDataRemotes();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = Setup();
  NS_ENSURE_SUCCESS(rv, rv);

  if(mStatus == sbIMediacoreStatus::STATUS_PLAYING ||
     mStatus == sbIMediacoreStatus::STATUS_BUFFERING) {

    mStatus = sbIMediacoreStatus::STATUS_BUFFERING;
    mIsWaitingForPlayback = PR_TRUE;

    rv = UpdatePlayStateDataRemotes();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mPlaybackControl->Play();
  }
  else if(mStatus == sbIMediacoreStatus::STATUS_PAUSED) {
    rv = mPlaybackControl->Pause();
  }

  if(NS_FAILED(rv)) {
    mStatus = sbIMediacoreStatus::STATUS_STOPPED;
    mIsWaitingForPlayback = PR_FALSE;

    rv = UpdatePlayStateDataRemotes();
    NS_ENSURE_SUCCESS(rv, rv);

    return rv;
  }

  return NS_OK;
}

nsresult 
sbMediacoreSequencer::Setup()
{
  nsCOMPtr<sbIMediaItem> item;
  nsresult rv = GetItem(mSequence, mPosition, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentItemIndex = mSequence[mPosition];

  rv = mView->GetViewItemUIDForIndex(mCurrentItemIndex, mCurrentItemUID);
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentItem = item;

  nsCOMPtr<nsIURI> uri;
  rv = item->GetContentSrc(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

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
    nsCOMPtr<sbIMediacoreEventTarget> eventTarget = 
      do_QueryInterface(mCore, &rv);

    rv = eventTarget->RemoveListener(this);
    NS_ENSURE_SUCCESS(rv, rv);

    if(mStatus == sbIMediacoreStatus::STATUS_BUFFERING ||
       mStatus == sbIMediacoreStatus::STATUS_PLAYING ||
       mStatus == sbIMediacoreStatus::STATUS_PAUSED) {

      // Also stop the current core.
      rv = mPlaybackControl->Stop();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  nsCOMPtr<sbIMediacorePlaybackControl> playbackControl = 
    do_QueryElementAt(chain, mChainIndex, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mPlaybackControl = playbackControl;

  mCore = do_QueryElementAt(chain, mChainIndex, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add listener to new core.
  nsCOMPtr<sbIMediacoreEventTarget> eventTarget = 
    do_QueryInterface(mCore, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Fire before track change event 
  nsCOMPtr<nsIVariant> variant = sbNewVariant(item).get();
  NS_ENSURE_TRUE(variant, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<sbIMediacoreEvent> event;
  rv = sbMediacoreEvent::CreateEvent(sbIMediacoreEvent::BEFORE_TRACK_CHANGE, 
                                     nsnull, 
                                     variant, 
                                     mCore, 
                                     getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DispatchMediacoreEvent(event);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = eventTarget->AddListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPlaybackControl->SetUri(uri);
  NS_ENSURE_SUCCESS(rv, rv);

#if defined(DEBUG)
  {
    nsCString spec;
    rv = uri->GetSpec(spec);
    if(NS_SUCCEEDED(rv)) {
      printf("[sbMediacoreSequencer] -- Attempting to play %s\n", 
        spec.BeginReading());

    }
  }
#endif

  nsCOMPtr<sbPIMediacoreManager> privateMediacoreManager = 
    do_QueryReferent(mMediacoreManager, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = privateMediacoreManager->SetPrimaryCore(mCore);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = StartSequenceProcessor();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateURLDataRemotes(uri);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetMetadataDataRemotesFromItem(item);
  NS_ENSURE_SUCCESS(rv, rv);

  // Fire track change event
  variant = sbNewVariant(item);
  NS_ENSURE_TRUE(variant, NS_ERROR_OUT_OF_MEMORY);

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

nsresult 
sbMediacoreSequencer::SetViewWithViewPosition(sbIMediaListView *aView, 
                                              PRUint32 *aViewPosition /* = nsnull */)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aView);

  nsAutoMonitor mon(mMonitor);
  if(mView != aView) {

    // Fire before view change event 
    nsCOMPtr<nsIVariant> variant = sbNewVariant(aView).get();
    NS_ENSURE_TRUE(variant, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<sbIMediacoreEvent> event;
    nsresult rv = 
      sbMediacoreEvent::CreateEvent(sbIMediacoreEvent::BEFORE_VIEW_CHANGE, 
                                    nsnull, 
                                    variant, 
                                    mCore, 
                                    getter_AddRefs(event));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = DispatchMediacoreEvent(event);
    NS_ENSURE_SUCCESS(rv, rv);

    mView = aView;

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
  NS_ENSURE_STATE(mView);

  nsAutoMonitor mon(mMonitor);

  nsresult rv = mView->GetMediaList(getter_AddRefs(mViewList));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> library = do_QueryInterface(mViewList, &rv);
  mViewIsLibrary = NS_SUCCEEDED(rv) ? PR_TRUE : PR_FALSE;

  rv = mViewList->AddListener(this, 
                              PR_FALSE, 
                              sbIMediaList::LISTENER_FLAGS_ITEMADDED |
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
                           sbIMediaList::LISTENER_FLAGS_BATCHBEGIN |
                           sbIMediaList::LISTENER_FLAGS_BATCHEND |
                           sbIMediaList::LISTENER_FLAGS_LISTCLEARED,
                           nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult 
sbMediacoreSequencer::StopWatchingView()
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_STATE(mView);

  nsAutoMonitor mon(mMonitor);
  
  nsresult rv = NS_ERROR_UNEXPECTED;

  if(mDelayedCheckTimer) {
    rv = HandleDelayedCheckTimer(mDelayedCheckTimer);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDelayedCheckTimer->Cancel();
    NS_ENSURE_SUCCESS(rv, rv);

    mDelayedCheckTimer = nsnull;
  }

  rv = mViewList->RemoveListener(this);
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

  rv = mDelayedCheckTimer->InitWithCallback(this, 100, nsITimer::TYPE_ONE_SHOT);
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
  
  nsString previousItemUID = mCurrentItemUID;
  PRUint32 previousItemIndex = mCurrentItemIndex;

  nsresult rv = mView->GetIndexForItem(mCurrentItem, &mCurrentItemIndex);

  if(NS_FAILED(rv)) {
    mCurrentItemIndex = 0;
    mCurrentItemUID = EmptyString();
  }
  else {
    rv = mView->GetViewItemUIDForIndex(mCurrentItemIndex, mCurrentItemUID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if(mCurrentItemIndex != previousItemIndex || 
     mCurrentItemUID != previousItemUID) {

    rv = RecalculateSequence(&mCurrentItemIndex);
    NS_ENSURE_SUCCESS(rv, rv);

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

    nsresult rv = RecalculateSequence();
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
  *aViewPosition = mViewPosition;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreSequencer::GetCurrentItem(sbIMediaItem **aItem) 
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aItem);

  // by default, if there's no current item, this doesn't throw, it returns a
  // null item instead.
  *aItem = nsnull;

  if(!mView) {
    return NS_OK;
  }

  nsresult rv = mView->GetItemByIndex(mViewPosition, aItem);
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

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreSequencer::GetSequencePosition(PRUint32 *aSequencePosition)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aSequencePosition);

  nsAutoMonitor mon(mMonitor);
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
                               PRUint32 aItemIndex)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aView);

  nsAutoMonitor mon(mMonitor);
  
  nsresult rv = SetViewWithViewPosition(aView, &aItemIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = Play();
  NS_ENSURE_SUCCESS(rv, rv);
  
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
  mIsWaitingForPlayback = PR_TRUE;

  nsresult rv = ResetMetadataDataRemotes();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = Setup();
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = UpdatePlayStateDataRemotes();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPlaybackControl->Play();

  if(NS_FAILED(rv)) {
    mStatus = sbIMediacoreStatus::STATUS_STOPPED;
    mIsWaitingForPlayback = PR_FALSE;

    rv = UpdatePlayStateDataRemotes();
    NS_ENSURE_SUCCESS(rv, rv);

    return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreSequencer::Next()
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);

  // No sequence, no error, return early.
  if(!mSequence.size()) {
    return NS_OK;
  }
  
  PRBool hasNext = PR_FALSE;
  PRUint32 length = mSequence.size();

  if(mRepeatMode == sbIMediacoreSequencer::MODE_REPEAT_ALL &&
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

    if(!mNextTriggeredByStreamEnd) {
      if(mPosition + 1 >= length) {
        mPosition = 0;
      }
      else if(mPosition + 1 < length) {
        ++mPosition;
      }
      mViewPosition = mSequence[mPosition];
    }
  }
  else if(mPosition + 1 < length) {
    ++mPosition;
    mViewPosition = mSequence[mPosition];
    hasNext = PR_TRUE;
  }

  // No next track, not an error.
  if(!hasNext) {
    nsresult rv = NS_ERROR_UNEXPECTED;

    // No next track and playing, stop.
    if(mStatus == sbIMediacoreStatus::STATUS_PLAYING ||
       mStatus == sbIMediacoreStatus::STATUS_PAUSED ||
       mStatus == sbIMediacoreStatus::STATUS_BUFFERING) {
      rv = mPlaybackControl->Stop();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    mStatus = sbIMediacoreStatus::STATUS_STOPPED;

    rv = StopSequenceProcessor();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = UpdatePlayStateDataRemotes();
    NS_ENSURE_SUCCESS(rv, rv);

    // XXXAus: Send End of Sequence Event?

    return NS_OK;
  }

  nsresult rv = ProcessNewPosition();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreSequencer::Previous()
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

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

    if(mNextTriggeredByStreamEnd) {
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
    nsresult rv = NS_ERROR_UNEXPECTED;

    // No next track and playing, stop.
    if(mStatus == sbIMediacoreStatus::STATUS_PLAYING ||
       mStatus == sbIMediacoreStatus::STATUS_PAUSED ||
       mStatus == sbIMediacoreStatus::STATUS_BUFFERING) {
      rv = mPlaybackControl->Stop();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    mStatus = sbIMediacoreStatus::STATUS_STOPPED;

    rv = StopSequenceProcessor();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = UpdatePlayStateDataRemotes();
    NS_ENSURE_SUCCESS(rv, rv);

    // XXXAus: Send End of Sequence Event?

    return NS_OK;
  }

  nsresult rv = ProcessNewPosition();
  NS_ENSURE_SUCCESS(rv, rv);

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
      nsresult rv = RecalculateSequence();
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

#if defined(DEBUG)
  nsString eventInstanceName;
  rv = core->GetInstanceName(eventInstanceName);
  NS_ENSURE_SUCCESS(rv, rv);
  
  NS_ConvertUTF16toUTF8 name(eventInstanceName);
  printf("\n[sbMediacoreSequencer] - Event - BEGIN\n");
  printf("[sbMediacoreSequencer] - Instance: %s\n", name.BeginReading());
  printf("[sbMediacoreSequencer] - Event Type: %d\n\n", eventType);
#endif

  if(mCore != core) {
    return NS_OK;
  }

  nsAutoMonitor mon(mMonitor);
  nsCOMPtr<sbIMediacoreEventTarget> target = 
    do_QueryReferent(mMediacoreManager, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool dispatched;
  rv = target->DispatchEvent(aEvent, PR_TRUE, &dispatched);
  NS_ENSURE_SUCCESS(rv, rv);

  switch(eventType) {
    // Stream Events
    case sbIMediacoreEvent::STREAM_START: {
      if(mStatus == sbIMediacoreStatus::STATUS_BUFFERING &&
         mIsWaitingForPlayback) {
        mIsWaitingForPlayback = PR_FALSE;
        mStatus = sbIMediacoreStatus::STATUS_PLAYING;

#if defined(DEBUG)
        printf("[sbMediacoreSequencer] - Was waiting for playback, playback now started.\n");
#endif
      }

      if(mStatus == sbIMediacoreStatus::STATUS_PAUSED) {
         mStatus = sbIMediacoreStatus::STATUS_PLAYING;
      }

      rv = UpdatePlayStateDataRemotes();
      NS_ENSURE_SUCCESS(rv, rv);

      if(!mSeenPlaying) {
        mSeenPlaying = PR_TRUE;

        rv = mDataRemoteFaceplateSeenPlaying->SetBoolValue(PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    break;

    case sbIMediacoreEvent::STREAM_PAUSE: {
      mStatus = sbIMediacoreStatus::STATUS_PAUSED;

      rv = UpdatePlayStateDataRemotes();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    case sbIMediacoreEvent::STREAM_END: {
      /* Track done, continue on to the next, if possible. */
      if(mStatus == sbIMediacoreStatus::STATUS_PLAYING &&
         !mIsWaitingForPlayback) {
        
        sbScopedBoolToggle toggle(&mNextTriggeredByStreamEnd);
        
        if(mMode != sbIMediacoreSequencer::MODE_REVERSE) {
          rv = Next();
        }
        else {
          rv = Previous();
        }

        if(NS_FAILED(rv)) {
          mStatus = sbIMediacoreStatus::STATUS_STOPPED;

          rv = StopSequenceProcessor();
          NS_ENSURE_SUCCESS(rv, rv);

          rv = UpdatePlayStateDataRemotes();
          NS_ENSURE_SUCCESS(rv, rv);
        }
#if defined(DEBUG)
        printf("[sbMediacoreSequencer] - Was playing, stream ended, attempting to go to next track in sequence.\n");
#endif
      }
    }
    break;
    case sbIMediacoreEvent::STREAM_STOP: {
      /* If we're explicitly stopped, don't continue to the next track,
       * just clean up... */
      mStatus = sbIMediacoreStatus::STATUS_STOPPED;

      rv = StopSequenceProcessor();
      NS_ENSURE_SUCCESS(rv, rv);

      rv = UpdatePlayStateDataRemotes();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    // Error Events
    case sbIMediacoreEvent::ERROR_EVENT: {
      if(mIsWaitingForPlayback) {
        mIsWaitingForPlayback = PR_FALSE;

        // XXXAus: Error while trying to play, try next core in chain if there 
        //         is one. If not, proceed to the next item.
      }
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
      // XXXAus: Update mute state dataremote
    }
    break;

    case sbIMediacoreEvent::VOLUME_CHANGE: {
      // XXXAus: Update volume dataremote
    }
    break;

    default:;
  }

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

  if (aMediaList == mViewList && 
      mSmartRebuildDetectBatchCount == mListBatchCount) {
      // Our playing list is a smart playlist, and it is being rebuilt,
      // so make a note that we need to try to find the old playitem in
      // the new list (this will update the now playing icon in 
      // tree views, and ensure that currentIndex returns the new index).
      // The 1st part of the detection has already scheduled a check, but
      // our search will occur before the check happens, so if the old
      // playing item no longer exists, playback will correctly stop.
      mNeedSearchPlayingItem = true;
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreSequencer::OnBeforeItemRemoved(sbIMediaList *aMediaList, 
                                          sbIMediaItem *aMediaItem, 
                                          PRUint32 aIndex, 
                                          PRBool *_retval)
{
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
      // if the item is our list, stop playback now and shutdown watcher
      rv = mPlaybackControl->Stop();
      NS_ENSURE_SUCCESS(rv, rv);

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
    mNeedCheck = PR_TRUE;
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
    rv = SetMetadataDataRemotesFromItem(item);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreSequencer::OnItemMoved(sbIMediaList *aMediaList, 
                                  PRUint32 aFromIndex, 
                                  PRUint32 aToIndex, 
                                  PRBool *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreSequencer::OnListCleared(sbIMediaList *aMediaList, 
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

  if(aMediaList == mViewList) {
    mListBatchCount--;
  }
  else {
    mLibraryBatchCount--;
  }

  if(mListBatchCount == 0 || mLibraryBatchCount == 0) {
    
    if(mNeedCheck) {
      rv = DelayedCheck();
      NS_ENSURE_SUCCESS(rv, rv);
      
      mNeedCheck = PR_FALSE;
    }

    if(mNeedSearchPlayingItem) {
      rv = UpdateItemUIDIndex();
      NS_ENSURE_SUCCESS(rv, rv);

      mNeedSearchPlayingItem = PR_FALSE;
    }

  }

  return NS_OK;
}


// -----------------------------------------------------------------------------
// sbIMediaListViewListener
// -----------------------------------------------------------------------------
NS_IMETHODIMP 
sbMediacoreSequencer::OnFilterChanged(sbIMediaListView *aChangedView)
{
  // XXXAus: Not used currently, leaving stubbed.
  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreSequencer::OnSearchChanged(sbIMediaListView *aChangedView)
{
  // XXXAus: Not used currently, leaving stubbed.
  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreSequencer::OnSortChanged(sbIMediaListView *aChangedView)
{
  // XXXAus: Not used currently, leaving stubbed.
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
  nsAutoMonitor mon(mMonitor);

  // Update the position in the position data remote.
  if(mStatus == sbIMediacoreStatus::STATUS_PLAYING ||
     mStatus == sbIMediacoreStatus::STATUS_PAUSED) {

    PRUint64 position = 0;
    rv = mPlaybackControl->GetPosition(&position);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = UpdatePositionDataRemotes(position);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if(mStatus == sbIMediacoreStatus::STATUS_PLAYING ||
     mStatus == sbIMediacoreStatus::STATUS_BUFFERING ||
     mStatus == sbIMediacoreStatus::STATUS_PAUSED) {

    PRUint64 duration = 0;
    rv = mPlaybackControl->GetDuration(&duration);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = UpdateDurationDataRemotes(duration);
    NS_ENSURE_SUCCESS(rv, rv);
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

  PRUint32 index = 0;
  nsresult rv = mView->GetIndexForViewItemUID(mCurrentItemUID, &index);
  if(NS_FAILED(rv)) {
    // if the item is our list, stop playback now and shutdown watcher
    rv = mPlaybackControl->Stop();
    NS_ENSURE_SUCCESS(rv, rv);

    mStatus = sbIMediacoreStatus::STATUS_STOPPED;

    rv = StopSequenceProcessor();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = UpdatePlayStateDataRemotes();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = StopWatchingView();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
