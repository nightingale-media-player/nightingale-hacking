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
* \file  sbBaseMediacore.cpp
* \brief Songbird Base Mediacore Implementation.
*/
#include "sbBaseMediacore.h"

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbBaseMediacore:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gBaseMediacore = nsnull;
#define TRACE(args) PR_LOG(gBaseMediacore, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gBaseMediacore, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

NS_IMPL_THREADSAFE_ISUPPORTS1(sbBaseMediacore, 
                              sbIMediacore)

sbBaseMediacore::sbBaseMediacore()
: mLock(nsnull)
{
  MOZ_COUNT_CTOR(sbBaseMediacore);

#ifdef PR_LOGGING
  if (!gBaseMediacore)
    gBaseMediacore = PR_NewLogModule("sbBaseMediacore");
#endif

  TRACE(("sbBaseMediacore[0x%x] - Created", this));
}

sbBaseMediacore::~sbBaseMediacore()
{
  TRACE(("sbBaseMediacore[0x%x] - Destroyed", this));
  MOZ_COUNT_DTOR(sbBaseMediacore);

  if (mLock) {
    nsAutoLock::DestroyLock(mLock);
  }
}

nsresult 
sbBaseMediacore::InitBaseMediacore()
{
  TRACE(("sbBaseMediacore[0x%x] - InitBaseMediacore", this));

  mLock = nsAutoLock::NewLock("sbBaseMediacore::mLock");
  NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);

  return OnInitBaseMediacore();
}

nsresult 
sbBaseMediacore::SetInstanceName(const nsAString &aInstanceName)
{
  TRACE(("sbBaseMediacore[0x%x] - SetInstanceName", this));
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  mInstanceName = aInstanceName;

  return NS_OK;
}

nsresult 
sbBaseMediacore::SetCapabilities(sbIMediacoreCapabilities *aCapabilities)
{
  TRACE(("sbBaseMediacore[0x%x] - SetCapabilities", this));
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aCapabilities);

  nsAutoLock lock(mLock);
  mCapabilities = aCapabilities;

  return NS_OK;
}

nsresult 
sbBaseMediacore::SetStatus(sbIMediacoreStatus *aStatus)
{
  TRACE(("sbBaseMediacore[0x%x] - SetStatus", this));
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aStatus);
  
  nsAutoLock lock(mLock);
  mStatus = aStatus;
  
  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacore::GetInstanceName(nsAString & aInstanceName)
{
  TRACE(("sbBaseMediacore[0x%x] - GetInstanceName", this));
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  aInstanceName = mInstanceName;
  
  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacore::GetCapabilities(sbIMediacoreCapabilities * *aCapabilities)
{
  TRACE(("sbBaseMediacore[0x%x] - GetCapabilities", this));
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aCapabilities);

  nsresult rv = OnGetCapabilities();
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoLock lock(mLock);
  NS_IF_ADDREF(*aCapabilities = mCapabilities);

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacore::GetStatus(sbIMediacoreStatus* *aStatus)
{
  TRACE(("sbBaseMediacore[0x%x] - GetLastStatusEvent", this));
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aStatus);

  nsAutoLock lock(mLock);
  NS_IF_ADDREF(*aStatus = mStatus);

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacore::GetCurrentSequence(sbIMediacoreSequence * *aCurrentSequence)
{
  TRACE(("sbBaseMediacore[0x%x] - GetCurrentSequence", this));
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aCurrentSequence);

  nsAutoLock lock(mLock);
  NS_IF_ADDREF(*aCurrentSequence = mCurrentSequence);

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacore::SetCurrentSequence(sbIMediacoreSequence * aCurrentSequence)
{
  TRACE(("sbBaseMediacore[0x%x] - SetCurrentSequence", this));
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aCurrentSequence);

  nsAutoLock lock(mLock);
  mCurrentSequence = aCurrentSequence;

  nsresult rv = OnSetCurrentSequence(aCurrentSequence);

  return NS_OK;
}

/*virtual*/ nsresult 
sbBaseMediacore::OnInitBaseMediacore()
{
  /**
   *  You'll probably want to initialiaze all essential bits of your mediacore
   *  here.
   *
   *  Don't worry about the instance name, it will be set by the factory
   *  that created you after Init() is called.
   */  

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult 
sbBaseMediacore::OnGetCapabilities()
{
  /**
   *  If you feel like your status is out of data, this is good time to update
   *  it as someone has just requested to get the current status.
   *
   *  Once you have your new status, you should set it using SetStatus(). This 
   *  will update the cached version of the status object and return it to
   *  the callee.
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult 
sbBaseMediacore::OnSetCurrentSequence(sbIMediacoreSequence *aCurrentSequence)
{
  /**
   *  If you get a new sequence you may want to do something about it. Like peek 
   *  at the next item for gapless playback, set up buffering, or what not.
   *
   *  If you don't need to do anything, just return NS_OK;
   *
   *  Please note that when you are called in this method, mLock is already 
   *  acquired. Do not attempt to acquire it!
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}