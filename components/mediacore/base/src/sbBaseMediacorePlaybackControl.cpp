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

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbBaseMediacorePlaybackControl:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gBaseMediacorePlaybackControl = nsnull;
#define TRACE(args) PR_LOG(gBaseMediacorePlaybackControl , PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gBaseMediacorePlaybackControl , PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

NS_IMPL_THREADSAFE_ISUPPORTS1(sbBaseMediacorePlaybackControl, 
                              sbIMediacorePlaybackControl)

sbBaseMediacorePlaybackControl::sbBaseMediacorePlaybackControl()
: mLock(nsnull)
, mPosition(0)
, mDuration(0)
{
  MOZ_COUNT_CTOR(sbBaseMediacorePlaybackControl);

#ifdef PR_LOGGING
  if (!gBaseMediacorePlaybackControl)
    gBaseMediacorePlaybackControl= PR_NewLogModule("sbBaseMediacorePlaybackControl");
#endif

  TRACE(("sbBaseMediacorePlaybackControl[0x%x] - Created", this));
}

sbBaseMediacorePlaybackControl::~sbBaseMediacorePlaybackControl()
{
  TRACE(("sbBaseMediacorePlaybackControl[0x%x] - Destroyed", this));

  MOZ_COUNT_DTOR(sbBaseMediacorePlaybackControl);

  if(mLock) {
    nsAutoLock::DestroyLock(mLock);
  }
}

nsresult 
sbBaseMediacorePlaybackControl::InitBaseMediacorePlaybackControl()
{
  TRACE(("sbBaseMediacorePlaybackControl[0x%x] - InitBaseMediacorePlaybackControl", this));

  mLock = nsAutoLock::NewLock("sbBaseMediacorePlaybackControl::mLock");
  NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);

  return OnInitBaseMediacorePlaybackControl();
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::GetUri(nsIURI * *aUri)
{
  TRACE(("sbBaseMediacorePlaybackControl[0x%x] - GetUri", this));
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aUri);

  nsAutoLock lock(mLock);
  NS_IF_ADDREF(*aUri = mUri);  

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::SetUri(nsIURI * aUri)
{
  TRACE(("sbBaseMediacorePlaybackControl[0x%x] - SetUri", this));
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aUri);

  nsresult rv = OnSetUri(aUri);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoLock lock(mLock);
  mUri = aUri;  

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::GetPosition(PRUint64 *aPosition)
{
  TRACE(("sbBaseMediacorePlaybackControl[0x%x] - GetPosition", this));
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aPosition);

  nsAutoLock lock(mLock);
  *aPosition = mPosition;

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::SetPosition(PRUint64 aPosition)
{
  TRACE(("sbBaseMediacorePlaybackControl[0x%x] - SetPosition", this));
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = OnSetPosition(aPosition);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoLock lock(mLock);
  mPosition = aPosition;

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::GetDuration(PRUint64 *aDuration)
{
  TRACE(("sbBaseMediacorePlaybackControl[0x%x] - GetDuration", this));
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDuration);

  nsAutoLock lock(mLock);
  *aDuration = mDuration;  

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::Play()
{
  TRACE(("sbBaseMediacorePlaybackControl[0x%x] - Play", this));
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  return OnPlay();
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::Pause()
{
  TRACE(("sbBaseMediacorePlaybackControl[0x%x] - Pause", this));
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  return OnPause();
}

NS_IMETHODIMP 
sbBaseMediacorePlaybackControl::Stop()
{
  TRACE(("sbBaseMediacorePlaybackControl[0x%x] - Stop", this));
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
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
sbBaseMediacorePlaybackControl::OnSetPosition(PRUint64 aPosition)
{
  /**
   *  This is where you'll want to seek to aPosition in the currently 
   *  set source.
   *
   *  The position is cached in 
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
   *  Upon stopping, you are responsible for rewinding / setting the position
   *  to the beginning. Yes, you should also fire an event after you've reset
   *  the position :)
   *
   *  See sbIMediacoreEvent for more information.
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}
