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
* \file  sbBaseMediacoreFactory.cpp
* \brief Songbird Base Mediacore Factory Implementation.
*/
#include "sbBaseMediacoreFactory.h"

#include <sbIMediacore.h>
#include <mozilla/ReentrantMonitor.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbBaseMediacoreFactory:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gBaseMediacoreFactory = nsnull;
#define TRACE(args) PR_LOG(gBaseMediacoreFactory, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gBaseMediacoreFactory, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

NS_IMPL_THREADSAFE_ISUPPORTS1(sbBaseMediacoreFactory, 
                              sbIMediacoreFactory)

sbBaseMediacoreFactory::sbBaseMediacoreFactory()
: mMonitor(nsnull)
{
  MOZ_COUNT_CTOR(sbBaseMediacoreFactory);

#ifdef PR_LOGGING
  if (!gBaseMediacoreFactory)
    gBaseMediacoreFactory= PR_NewLogModule("sbBaseMediacoreFactory");
#endif

  TRACE(("sbBaseMediacoreFactory[0x%x] - Created", this));
}

sbBaseMediacoreFactory::~sbBaseMediacoreFactory()
{
  TRACE(("sbBaseMediacoreFactory[0x%x] - Destroyed", this));
  MOZ_COUNT_DTOR(sbBaseMediacoreFactory);
}

nsresult 
sbBaseMediacoreFactory::InitBaseMediacoreFactory()
{
  TRACE(("sbBaseMediacoreFactory[0x%x] - InitBaseMediacoreFactory", this));

  return OnInitBaseMediacoreFactory();
}

nsresult 
sbBaseMediacoreFactory::SetContractID(const nsAString &aContractID)
{
  TRACE(("sbBaseMediacoreFactory[0x%x] - Init", this));

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  mContractID = aContractID;

  return NS_OK;
}

nsresult 
sbBaseMediacoreFactory::SetName(const nsAString &aName)
{
  TRACE(("sbBaseMediacoreFactory[0x%x] - Init", this));

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  mName = aName;

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacoreFactory::GetContractID(nsAString & aContractID)
{
  TRACE(("sbBaseMediacoreFactory[0x%x] - Init", this));
  
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  aContractID = mContractID;

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacoreFactory::GetName(nsAString & aName)
{
  TRACE(("sbBaseMediacoreFactory[0x%x] - Init", this));

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  aName = mName;

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacoreFactory::GetCapabilities(
                          sbIMediacoreCapabilities * *aCapabilities)
{
  TRACE(("sbBaseMediacoreFactory[0x%x] - GetCapabilities", this));
  NS_ENSURE_ARG_POINTER(aCapabilities);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  return OnGetCapabilities(aCapabilities);
}

NS_IMETHODIMP 
sbBaseMediacoreFactory::Create(const nsAString & aInstanceName, 
                               sbIMediacore **_retval)
{
  TRACE(("sbBaseMediacoreFactory[0x%x] - Create", this));
  NS_ENSURE_ARG_POINTER(_retval);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

  nsresult rv = OnCreate(aInstanceName, _retval);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(*_retval, NS_ERROR_UNEXPECTED);

  NS_ADDREF(*_retval);
  
  return NS_OK;
}

/*virtual*/ nsresult 
sbBaseMediacoreFactory::OnInitBaseMediacoreFactory()
{
  /**
   * You'll probably want to set the contract id and the name of the factory 
   * at this point.
   *
   * rv = SetName(NS_LITERAL_STRING("Sample Mediacore Factory"));
   * NS_ENSURE_SUCCESS(rv, rv);
   *
   * rv = SetContractID("@example.com/Example/Mediacore/Factory;1");
   * NS_ENSURE_SUCCESS(rv, rv);
   */  

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult
sbBaseMediacoreFactory::OnGetCapabilities(
                          sbIMediacoreCapabilities **aCapabilities)
{
  /**
   *  You'll probably want to call a static method on your media core
   *  implementation here. 
   * 
   *  This method would return a static list of capabilities. It's important 
   *  to return a fairly accurate list because the capabilities will be used 
   *  to determine which core is worth initializing when attempting to perform
   *  a playback or transcoding operation.
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult 
sbBaseMediacoreFactory::OnCreate(const nsAString &aInstanceName, 
                                 sbIMediacore **_retval)
{
  /**
   *  You should now create a new core with the requested instance
   *  name and return it in *_retval. Do _not_ add a reference to the object.
   *  The reference will be added for you!
   */
  
  return NS_ERROR_NOT_IMPLEMENTED;
}
