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

#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>

#include <nsMemory.h>
#include <mozilla/ReentrantMonitor.h>

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

NS_IMPL_THREADSAFE_ISUPPORTS2(sbBaseMediacore, 
                              sbIMediacore,
                              nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER2(sbBaseMediacore,
                             sbIMediacore,
                             nsIClassInfo)

NS_IMPL_THREADSAFE_CI(sbBaseMediacore)

sbBaseMediacore::sbBaseMediacore()
: mMonitor("mMonitor")
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
}

nsresult 
sbBaseMediacore::InitBaseMediacore()
{
  TRACE(("sbBaseMediacore[0x%x] - InitBaseMediacore", this));

  mozilla::ReentrantMonitorAutoEnter monitor(mMonitor);

  return OnInitBaseMediacore();
}

nsresult 
sbBaseMediacore::SetInstanceName(const nsAString &aInstanceName)
{
  TRACE(("sbBaseMediacore[0x%x] - SetInstanceName", this));

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  mInstanceName = aInstanceName;

  return NS_OK;
}

nsresult 
sbBaseMediacore::SetCapabilities(sbIMediacoreCapabilities *aCapabilities)
{
  TRACE(("sbBaseMediacore[0x%x] - SetCapabilities", this));
  NS_ENSURE_ARG_POINTER(aCapabilities);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  mCapabilities = aCapabilities;

  return NS_OK;
}

nsresult 
sbBaseMediacore::SetStatus(sbIMediacoreStatus *aStatus)
{
  TRACE(("sbBaseMediacore[0x%x] - SetStatus", this));
  NS_ENSURE_ARG_POINTER(aStatus);
  
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  mStatus = aStatus;
  
  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacore::GetInstanceName(nsAString & aInstanceName)
{
  TRACE(("sbBaseMediacore[0x%x] - GetInstanceName", this));

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  aInstanceName = mInstanceName;
  
  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacore::GetCapabilities(sbIMediacoreCapabilities * *aCapabilities)
{
  TRACE(("sbBaseMediacore[0x%x] - GetCapabilities", this));
  NS_ENSURE_ARG_POINTER(aCapabilities);

  nsresult rv = OnGetCapabilities();
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  NS_IF_ADDREF(*aCapabilities = mCapabilities);

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacore::GetStatus(sbIMediacoreStatus* *aStatus)
{
  TRACE(("sbBaseMediacore[0x%x] - GetLastStatusEvent", this));
  NS_ENSURE_ARG_POINTER(aStatus);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  NS_IF_ADDREF(*aStatus = mStatus);

  return NS_OK;
}

NS_IMETHODIMP
sbBaseMediacore::GetSequencer(sbIMediacoreSequencer* *aSequencer)
{
  TRACE(("sbBaseMediacore[0x%x] - GetSequencer", this));
  NS_ENSURE_ARG_POINTER(aSequencer);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  NS_IF_ADDREF(*aSequencer = mSequencer);

  return NS_OK;
}

NS_IMETHODIMP
sbBaseMediacore::SetSequencer(sbIMediacoreSequencer *aSequencer)
{
  TRACE(("sbBaseMediacore[0x%x] - GetSequencer", this));
  NS_ENSURE_ARG_POINTER(aSequencer);

  nsresult rv = OnSetSequencer(aSequencer);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  mSequencer = aSequencer;

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacore::Shutdown()
{
  TRACE(("sbBaseMediacore[0x%x] - Shutdown", this));

  return OnShutdown();
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
sbBaseMediacore::OnSetSequencer(sbIMediacoreSequencer *aSequencer)
{
  /** 
   * If your mediacore can handle things like gapless playback and crossfading
   * you can use the sequencer that is passed to you to peek at the item that
   * will follow the current one.
   *
   * If you wish to handle the next item, you can indicate this to the sequencer
   * by calling sbIMediacoreSequencer::requestHandleNextItem.
   *
   * The sequencer is available to you as 'mSequencer' as well if this method
   * returns NS_OK. You should lock sbBaseMediacore::mMonitor when getting the
   * sequencer from mSequencer and putting it in your own local nsCOMPtr. Do not
   * hold the lock longer than you have to. Get your reference, and release the
   * lock.
   */

  return NS_OK;
}

/*virtual*/ nsresult 
sbBaseMediacore::OnShutdown()
{
  /**
   *  This is where you get asked to shutdown all operations.
   *
   *  You should try _REALLY_ hard to make this method always succeed at
   *  whatever cleanup it has to do. Failing to do so would likely cause memory
   *  leaks over time as your core may fail to shutdown multiple times and 
   *  remain alive in the instance cache of the mediacore manager.
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}
