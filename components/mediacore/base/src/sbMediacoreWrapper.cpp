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
* \file  sbMediacoreWrapper.cpp
* \brief Songbird Mediacore Wrapper Implementation.
*/
#include "sbMediacoreWrapper.h"

#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>

#include <nsMemory.h>
#include <prlog.h>

#include "sbBaseMediacoreEventTarget.h"

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediacoreWrapper:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gMediacoreWrapper = nsnull;
#define TRACE(args) PR_LOG(gMediacoreWrapper, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMediacoreWrapper, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

NS_IMPL_THREADSAFE_ADDREF(sbMediacoreWrapper)
NS_IMPL_THREADSAFE_RELEASE(sbMediacoreWrapper)

NS_IMPL_QUERY_INTERFACE7_CI(sbMediacoreWrapper,
                            sbIMediacore,
                            sbIMediacorePlaybackControl,
                            sbIMediacoreVolumeControl,
                            sbIMediacoreVotingParticipant,
                            sbIMediacoreEventTarget,
                            nsIDOMEventListener,
                            nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER6(sbMediacoreWrapper,
                             sbIMediacore,
                             sbIMediacorePlaybackControl,
                             sbIMediacoreVolumeControl,
                             sbIMediacoreVotingParticipant,
                             sbIMediacoreEventTarget,
                             nsIClassInfo)

NS_DECL_CLASSINFO(sbMediacoreWrapper)
NS_IMPL_THREADSAFE_CI(sbMediacoreWrapper)

sbMediacoreWrapper::sbMediacoreWrapper()
: mBaseEventTarget(new sbBaseMediacoreEventTarget(this))
{
#ifdef PR_LOGGING
  if (!gMediacoreWrapper)
    gMediacoreWrapper = PR_NewLogModule("sbMediacoreWrapper");
#endif
  TRACE(("sbMediacoreWrapper[0x%x] - Created", this));
}

sbMediacoreWrapper::~sbMediacoreWrapper()
{
  TRACE(("sbMediacoreWrapper[0x%x] - Destroyed", this));
}

nsresult
sbMediacoreWrapper::Init()
{
  TRACE(("sbMediacoreWrapper[0x%x] - Init", this));

  nsresult rv = sbBaseMediacore::InitBaseMediacore();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbBaseMediacorePlaybackControl::OnInitBaseMediacorePlaybackControl();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbBaseMediacoreVolumeControl::OnInitBaseMediacoreVolumeControl();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//
// sbBaseMediacore overrides
//

/*virtual*/ nsresult 
sbMediacoreWrapper::OnInitBaseMediacore()
{
  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnGetCapabilities()
{
  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnShutdown()
{
  return NS_OK;
}

//
// sbBaseMediacorePlaybackControl overrides
//

/*virtual*/ nsresult 
sbMediacoreWrapper::OnInitBaseMediacorePlaybackControl()
{
  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnSetUri(nsIURI *aURI)
{
  // XXXAus: Send 'seturi' event to mDOMWindow. [sync]

  return NS_OK;
}

/*virtual*/ nsresult
sbMediacoreWrapper::OnGetDuration(PRUint64 *aDuration) 
{
  // XXXAus: Send 'getduration' event to mDOMWindow. [sync]

  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnGetPosition(PRUint64 *aPosition)
{
  // XXXAus: Send 'getposition' event to mDOMWindow. [sync]

  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnSetPosition(PRUint64 aPosition)
{
  // XXXAus: Send 'setposition' event to mDOMWindow. [sync]

  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnGetIsPlayingAudio(PRBool *aIsPlayingAudio)
{
  // XXXAus: send 'getisplayingaudio' event to mDOMWindow. [sync]

  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnGetIsPlayingVideo(PRBool *aIsPlayingVideo)
{
  // XXXAus: send 'getisplayingvideo' event to mDOMWindow. [sync]

  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnPlay()
{
  // XXXAus: send 'play' event to mDOMWindow. [sync]
  // XXXAus: on success, send STREAM_START mediacore event.

  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnPause()
{
  // XXXAus: send 'pause' event to mDOMWindow. [sync]
  // XXXAus: on success, send STREAM_PAUSE mediacore event.

  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnStop()
{
  // XXXAus: send 'play' event to mDOMWindow. [sync]
  // XXXAus: on success, send STREAM_START mediacore event.

  return NS_OK;
}

/*virtual*/ nsresult
sbMediacoreWrapper::OnSeek(PRUint64 aPosition, PRUint32 aFlags)
{
  return OnSetPosition(aPosition);
}

//
// sbBaseMediacoreVolumeControl overrides
//

/*virtual*/ nsresult 
sbMediacoreWrapper::OnInitBaseMediacoreVolumeControl()
{
  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnSetMute(PRBool aMute)
{
  // XXXAus: send 'setmute' event to mDOMWindow. [sync]

  return NS_OK;
}
  
/*virtual*/ nsresult 
sbMediacoreWrapper::OnSetVolume(double aVolume)
{
  // XXXAus: send 'setvolume' event to mDOMWindow. [sync]
  
  return NS_OK;
}

//
// sbIMediacoreVotingParticipant
//

NS_IMETHODIMP
sbMediacoreWrapper::VoteWithURI(nsIURI *aURI, PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(_retval);

  // XXXAus: send 'votewithuri' event to mDOMWindow. [sync]
  *_retval = 0;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreWrapper::VoteWithChannel(nsIChannel *aChannel, PRUint32 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

//
// sbIMediacoreWrapper
//

NS_IMETHODIMP
sbMediacoreWrapper::Initialize(const nsAString &aInstanceName, 
                               sbIMediacoreCapabilities *aCapabilities,
                               const nsACString &aChromePageURL)
{
  NS_ENSURE_ARG_POINTER(aCapabilities);

  nsresult rv = SetInstanceName(aInstanceName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetCapabilities(aCapabilities);
  NS_ENSURE_SUCCESS(rv, rv);

  // XXXAus: Open chrome page url, stash DOM window in mDOMWindow
  // XXXAus: Add self as DOM Event Listener?

  return NS_OK;
}

//
// nsIDOMEventListener
//

NS_IMETHODIMP
sbMediacoreWrapper::HandleEvent(nsIDOMEvent *aEvent)
{
  return NS_OK;
}

//
// sbMediacoreWrapper
//
nsresult 
sbMediacoreWrapper::AddSelfDOMListener()
{
  return NS_OK;
}

nsresult 
sbMediacoreWrapper::RemoveSelfDOMListener()
{
  return NS_OK;
}

nsresult 
sbMediacoreWrapper::SendDOMEvent(const nsAString &aEventName, 
                                 const nsAString &aEventData)
{
  return NS_OK;
}
