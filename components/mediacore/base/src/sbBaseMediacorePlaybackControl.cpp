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
* \file  sbBaseMediacorePlaybackControl.cpp
* \brief Songbird Base Mediacore Playback Control Implementation.
*/
#include "sbBaseMediacorePlaybackControl.h"

#include <nsIWritablePropertyBag2.h>

#include <nsComponentManagerUtils.h>

#include "sbMediacoreEvent.h"
#include "sbVariantUtils.h"

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbBaseMediacorePlaybackControl:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gBaseMediacorePlaybackControl = nsnull;
#define TRACE(args) PR_LOG(gBaseMediacorePlaybackControl , PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gBaseMediacorePlaybackControl , PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

NS_IMPL_THREADSAFE_ISUPPORTS1(sbBaseMediacorePlaybackControl, 
                              sbIMediacorePlaybackControl)

sbBaseMediacorePlaybackControl::sbBaseMediacorePlaybackControl()
: mMonitor(nsnull)
, mPosition(0)
, mDuration(0)
{
  MOZ_COUNT_CTOR(sbBaseMediacorePlaybackControl);

#ifdef PR_LOGGING
  if (!gBaseMediacorePlaybackControl)
    gBaseMediacorePlaybackControl= PR_NewLogModule("sbBaseMediacorePlaybackControl");
#endif

  TRACE(("%s[%p]", __FUNCTION__, this));
}

sbBaseMediacorePlaybackControl::~sbBaseMediacorePlaybackControl()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  MOZ_COUNT_DTOR(sbBaseMediacorePlaybackControl);
}

nsresult 
sbBaseMediacorePlaybackControl::InitBaseMediacorePlaybackControl()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  return OnInitBaseMediacorePlaybackControl();
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::GetUri(nsIURI * *aUri)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aUri);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  NS_IF_ADDREF(*aUri = mUri);  

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::SetUri(nsIURI * aUri)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aUri);

  nsresult rv = OnSetUri(aUri);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  mUri = aUri;  

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::GetPosition(PRUint64 *aPosition)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aPosition);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  nsresult rv = OnGetPosition(aPosition);
  
  if(NS_FAILED(rv)) {
    *aPosition = 0;
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::SetPosition(PRUint64 aPosition)
{
  TRACE(("%s[%p] (%llu)", __FUNCTION__, this, aPosition));

  nsresult rv = OnSetPosition(aPosition);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  mPosition = aPosition;

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::Seek(PRUint64 aPosition, PRUint32 aFlags)
{
  TRACE(("%s[%p] (%llu, %llu)", __FUNCTION__, this, aPosition, aFlags));

  nsresult rv = OnSeek(aPosition, aFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  mPosition = aPosition;

  return NS_OK;
}

NS_IMETHODIMP
sbBaseMediacorePlaybackControl::GetDuration(PRUint64 *aDuration)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aDuration);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  nsresult rv = OnGetDuration(aDuration);

  if(NS_FAILED(rv)) {
    *aDuration = 0;
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::GetIsPlayingAudio(PRBool *aIsPlayingAudio)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aIsPlayingAudio);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  nsresult rv = OnGetIsPlayingAudio(aIsPlayingAudio);

  if(NS_FAILED(rv)) {
    *aIsPlayingAudio = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::GetIsPlayingVideo(PRBool *aIsPlayingVideo)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aIsPlayingVideo);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  nsresult rv = OnGetIsPlayingVideo(aIsPlayingVideo);

  if(NS_FAILED(rv)) {
    *aIsPlayingVideo = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::Play()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsresult rv;
  rv = DispatchPlaybackControlEvent(sbIMediacoreEvent::STREAM_BEFORE_START);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  return OnPlay();
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::Pause()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsresult rv;
  rv = DispatchPlaybackControlEvent(sbIMediacoreEvent::STREAM_BEFORE_PAUSE);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  return OnPause();
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::Stop()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsresult rv;
  rv = DispatchPlaybackControlEvent(sbIMediacoreEvent::STREAM_BEFORE_STOP);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  return OnStop();
}

/*virtual*/ nsresult 
sbBaseMediacorePlaybackControl::OnInitBaseMediacorePlaybackControl()
{
  /**
   *  You'll probably want to initialize the playback mechanism for your 
   *  mediacore implementation here.
   */  

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult 
sbBaseMediacorePlaybackControl::OnSetUri(nsIURI *aURI)
{
  /**
   *  This is where you'll probably want to set the source for playback.
   *
   *  The uri is cached in mUri if this method succeeds.
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult
sbBaseMediacorePlaybackControl::OnGetDuration(PRUint64 *aDuration) 
{
  /**
   *  This is where you'll want to get the current duration (in milliseconds).
   */
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult 
sbBaseMediacorePlaybackControl::OnGetPosition(PRUint64 *aPosition)
{
  /**
   *  This is where you'll want to get the current position (in milliseconds).
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult 
sbBaseMediacorePlaybackControl::OnSetPosition(PRUint64 aPosition)
{
  /**
   *  This is where you'll want to seek to aPosition in the currently 
   *  set source. The position is in milliseconds.
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult 
sbBaseMediacorePlaybackControl::OnGetIsPlayingAudio(PRBool *aIsPlayingAudio)
{
  /**
   *  This is where you'll want to report on whether you're playing audio.
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult 
sbBaseMediacorePlaybackControl::OnGetIsPlayingVideo(PRBool *aIsPlayingVideo)
{
  /**
   *  This is where you'll want to report on whether you're playing video.
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult 
sbBaseMediacorePlaybackControl::OnPlay()
{
  /**
   *  This is where you will actually want to start playing back the current
   *  source.
   *
   *  This method is called with the lock protecting base class data locked. 
   *  It is safe to get and set position and duration at this time.
   *
   *  You are responsible for firing any error events for errors that occur. 
   *
   *  You are also responsible for firing any success events.
   *
   *  See sbIMediacoreEvent for more information.
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult 
sbBaseMediacorePlaybackControl::OnPause()
{
  /**
   *  This is where you will actually want to pause playback for the current 
   *  source.
   *
   *  This method is called with the lock protecting base class data locked. 
   *  It is safe to get and set position and duration at this time.
   *
   *  You are responsible for firing any error events for errors that occur. 
   *
   *  You are also responsible for firing any success events.
   *
   *  See sbIMediacoreEvent for more information.
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult 
sbBaseMediacorePlaybackControl::OnStop()
{
  /**
   *  This is where you will actually want to stop playback for the current 
   *  source.
   *
   *  This method is called with the lock protecting base class data locked. 
   *  It is safe to get and set position and duration at this time.
   *
   *  You are responsible for firing any error events for errors that occur. 
   *
   *  You are also responsible for firing any success events.
   *
   *  You are not responsible for the STREAM_BEFORE_STOP event.
   *
   *  Upon stopping, you are responsible for rewinding / setting the position
   *  to the beginning. Yes, you should also fire an event after you've reset
   *  the position :)
   *
   *  See sbIMediacoreEvent for more information.
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult
sbBaseMediacorePlaybackControl::OnSeek(PRUint64 aPosition, PRUint32 aFlags)
{
  /**
   *  This is where you'll want to seek to aPosition in the currently
   *  set source. The position is in milliseconds.  The flags map to
   *  sbIMediacorePlaybackControl.SEEK_FLAG_*
   *
   *  Typically, the case where the flag is SEEK_FLAG_NORMAL will be the same
   *  codepath as OnSetPosition().
   *
   *  If let unimplemented, this will just default to being the same as if
   *  the user has set the position attribute.
   */

  return OnSetPosition(aPosition);
}

nsresult
sbBaseMediacorePlaybackControl::DispatchPlaybackControlEvent(PRUint32 aType)
{
  TRACE(("%s[%p] (%08x)", __FUNCTION__, this, aType));

  nsresult rv;

  // it's not fatal if this fails
  nsCOMPtr<sbIMediacore> core =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIMediacorePlaybackControl*, this));

  nsCOMPtr<nsIWritablePropertyBag2> bag =
    do_CreateInstance("@mozilla.org/hash-property-bag;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  switch (aType) {
    case sbIMediacoreEvent::STREAM_BEFORE_PAUSE:
    case sbIMediacoreEvent::STREAM_BEFORE_STOP:
    {
      PRUint64 number;
      rv = GetPosition(&number);
      if (NS_SUCCEEDED(rv)) {
        rv = bag->SetPropertyAsUint64(NS_LITERAL_STRING("position"), number);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      rv = GetDuration(&number);
      if (NS_SUCCEEDED(rv)) {
        rv = bag->SetPropertyAsUint64(NS_LITERAL_STRING("duration"), number);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      rv = bag->SetPropertyAsInterface(NS_LITERAL_STRING("uri"), mUri);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  nsCOMPtr<nsIVariant> data = do_QueryInterface(sbNewVariant(bag), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediacoreEvent> event;
  rv = sbMediacoreEvent::CreateEvent(aType,  // type
                                     nsnull, // error
                                     data,   // data
                                     core,   // origin
                                     getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DispatchEvent(event, PR_TRUE, nsnull);
  /* ignore the result */

  return NS_OK;
}
