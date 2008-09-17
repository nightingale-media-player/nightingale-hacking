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

#include <nsISupportsPrimitives.h>
#include <nsIURL.h>
#include <nsIWeakReferenceUtils.h>

#include <nsAutoLock.h>
#include <nsArrayUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <nsTArray.h>

#include <sbIMediacore.h>
#include <sbIMediacoreEvent.h>
#include <sbIMediacoreEventTarget.h>
#include <sbIMediacorePlaybackControl.h>
#include <sbIMediacoreStatus.h>
#include <sbIMediacoreVoting.h>
#include <sbIMediacoreVotingChain.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>

#include <sbTArrayStringEnumerator.h>

//------------------------------------------------------------------------------
//  sbMediacoreSequencer
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbMediacoreSequencer, 
                              sbIMediacoreSequencer)

sbMediacoreSequencer::sbMediacoreSequencer()
: mStatus(sbIMediacoreStatus::STATUS_UNKNOWN)
, mIsWaitingForPlayback(PR_FALSE)
, mMode(sbIMediacoreSequencer::MODE_FORWARD)
, mRepeatMode(sbIMediacoreSequencer::MODE_REPEAT_NONE)
{
}

sbMediacoreSequencer::~sbMediacoreSequencer()
{
  if(mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
  }
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

  // XXXAus: Bind DataRemotes !

  return NS_OK;
}

nsresult 
sbMediacoreSequencer::RecalculateSequence()
{
  nsAutoMonitor mon(mMonitor);

  mSequence.clear();

  nsCOMPtr<sbIMediaList> list;
  nsresult rv = mView->GetMediaList(getter_AddRefs(list));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length = 0;
  rv = list->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  switch(mMode) {
    case sbIMediacoreSequencer::MODE_FORWARD:
    {
      // Generate forward sequence
      for(PRUint32 i = 0; i < length; ++i) {
        mSequence.push_back(i);
      }
    }
    break;
    case sbIMediacoreSequencer::MODE_REVERSE:
    {
      // Generate reverse sequence
      for(PRUint32 i = length - 1; i >= 0; --i) {
        mSequence.push_back(i);
      }
    }
    break;
    case sbIMediacoreSequencer::MODE_SHUFFLE:
    {
      // XXXAus: Use shuffle generator.
    }
    break;
    case sbIMediacoreSequencer::MODE_CUSTOM:
    {
      // XXXAus: Use custom generator.
    }
    break;

    default:
      return NS_ERROR_UNEXPECTED;
  }

  // Reset position to 0 when recalculating the sequence.
  mPosition = 0;

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

  nsAutoMonitor mon(mMonitor);
  if(mView != aView) {
    mView = aView;

    nsresult rv = RecalculateSequence();
    NS_ENSURE_SUCCESS(rv, rv);
  }

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
sbMediacoreSequencer::Play()
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);

  // No sequence, no error, but return right away.
  if(!mSequence.size()) {
    return NS_OK;
  }

  // We're already actively playing the sequence
  if(mStatus == sbIMediacoreStatus::STATUS_PLAYING ||
     mStatus == sbIMediacoreStatus::STATUS_BUFFERING ||
     mStatus == sbIMediacoreStatus::STATUS_PAUSED) {
    return NS_OK;
  }

  nsCOMPtr<sbIMediaItem> item;
  nsresult rv = GetItem(mSequence, mPosition, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

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

  nsCOMPtr<sbIMediacorePlaybackControl> playbackControl = 
    do_QueryElementAt(chain, mChainIndex, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mPlaybackControl = playbackControl;

  // Remove listener from old core.
  if(mCore) {
    nsCOMPtr<sbIMediacoreEventTarget> eventTarget = 
      do_QueryInterface(mCore, &rv);
    
    rv = eventTarget->RemoveListener(this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mCore = do_QueryElementAt(chain, mChainIndex, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add listener to new core.
  nsCOMPtr<sbIMediacoreEventTarget> eventTarget = do_QueryInterface(mCore, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = eventTarget->AddListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  // XXXAus: Call Stop before setting new uri if core was playing?

  rv = mPlaybackControl->SetUri(uri);
  NS_ENSURE_SUCCESS(rv, rv);
  
  mStatus = sbIMediacoreStatus::STATUS_BUFFERING;
  mIsWaitingForPlayback = PR_TRUE;

  rv = mPlaybackControl->Play();

#if defined(DEBUG)
  nsCString spec;
  rv = uri->GetSpec(spec);
  if(NS_SUCCEEDED(rv)) {
    printf("[sbMediacoreSequencer] -- Attempting to play %s\n", 
           spec.BeginReading());
  }
#endif

  if(NS_FAILED(rv)) {
    mStatus = sbIMediacoreStatus::STATUS_STOPPED;
    mIsWaitingForPlayback = PR_FALSE;

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
    hasNext = PR_TRUE;

    if(mMode == sbIMediacoreSequencer::MODE_SHUFFLE ||
       mMode == sbIMediacoreSequencer::MODE_CUSTOM) {
      nsresult rv = RecalculateSequence();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else if(mMode == sbIMediacoreSequencer::MODE_REPEAT_ONE) {
    hasNext = PR_TRUE;
  }
  else if(mPosition + 1 < length) {
    ++mPosition;
    hasNext = PR_TRUE;
  }

  // No next track, not an error.
  if(!hasNext) {
    // XXXAus: Send End of Sequence Event?
    return NS_OK;
  }

  nsCOMPtr<sbIMediaItem> item;
  nsresult rv = GetItem(mSequence, mPosition, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

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

  nsCOMPtr<sbIMediacorePlaybackControl> playbackControl = 
    do_QueryElementAt(chain, mChainIndex, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mPlaybackControl = playbackControl;

  // Remove listener from previous core.
  if(mCore) {
    nsCOMPtr<sbIMediacoreEventTarget> eventTarget = 
      do_QueryInterface(mCore, &rv);

    rv = eventTarget->RemoveListener(this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mCore = do_QueryElementAt(chain, mChainIndex, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add listener to new core.
  nsCOMPtr<sbIMediacoreEventTarget> eventTarget = do_QueryInterface(mCore, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = eventTarget->AddListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  // XXXAus: Call Stop before setting new uri if core was playing?

  rv = mPlaybackControl->SetUri(uri);
  NS_ENSURE_SUCCESS(rv, rv);

#if defined(DEBUG)
  nsCString spec;
  rv = uri->GetSpec(spec);
  if(NS_SUCCEEDED(rv)) {
    printf("[sbMediacoreSequencer] -- Attempting to set next uri to %s\n", 
      spec.BeginReading());
  }
#endif

  if(mStatus == sbIMediacoreStatus::STATUS_PLAYING ||
     mStatus == sbIMediacoreStatus::STATUS_BUFFERING) {
    
    mStatus = sbIMediacoreStatus::STATUS_BUFFERING;
    mIsWaitingForPlayback = PR_TRUE;

    rv = mPlaybackControl->Play();

    if(NS_FAILED(rv)) {
      mStatus = sbIMediacoreStatus::STATUS_STOPPED;
      mIsWaitingForPlayback = PR_FALSE;

      return rv;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreSequencer::Previous()
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  return NS_ERROR_NOT_IMPLEMENTED;
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
    }
    break;

    case sbIMediacoreEvent::STREAM_END: {
      if(mStatus == sbIMediacoreStatus::STATUS_PLAYING &&
         !mIsWaitingForPlayback) {
        rv = Next();
        NS_ENSURE_SUCCESS(rv, rv);
#if defined(DEBUG)
        printf("[sbMediacoreSequencer] - Was playing, stream ended, attempting to go to next track in sequence.\n");
#endif
      }
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
      // XXXAus: Update metadata dataremotes
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
