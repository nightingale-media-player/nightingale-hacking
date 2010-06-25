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
* \file  sbMediacoreFactoryWrapper.cpp
* \brief Songbird Mediacore Factory Wrapper Implementation.
*/
#include "sbMediacoreFactoryWrapper.h"

#include <nsThreadUtils.h>
#include <sbProxiedComponentManager.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediacoreFactoryWrapper:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gMediacoreFactoryWrapper = nsnull;
#define TRACE(args) PR_LOG(gMediacoreFactoryWrapper, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMediacoreFactoryWrapper, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

NS_IMPL_ISUPPORTS_INHERITED2(sbMediacoreFactoryWrapper,
                             sbBaseMediacoreFactory,
                             sbIMediacoreFactoryWrapper,
                             sbIMediacoreFactory);

sbMediacoreFactoryWrapper::sbMediacoreFactoryWrapper()
{
#ifdef PR_LOGGING
  if (!gMediacoreFactoryWrapper)
    gMediacoreFactoryWrapper = PR_NewLogModule("sbMediacoreFactoryWrapper");
#endif

  TRACE(("sbMediacoreFactoryWrapper[0x%x] - Created", this));
}

sbMediacoreFactoryWrapper::~sbMediacoreFactoryWrapper()
{
  TRACE(("sbMediacoreFactoryWrapper[0x%x] - Destroyed", this));
}

nsresult
sbMediacoreFactoryWrapper::Init()
{
  nsresult rv = sbBaseMediacoreFactory::InitBaseMediacoreFactory();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//
// sbIMediacoreFactoryWrapper
//

NS_IMETHODIMP
sbMediacoreFactoryWrapper::Initialize(const nsAString &aFactoryName,
                                      const nsAString &aContractID,
                                      sbIMediacoreCapabilities *aCapabilities,
                                      sbIMediacoreFactoryWrapperListener *aListener)
{
  NS_ENSURE_FALSE(aFactoryName.IsEmpty(), NS_ERROR_INVALID_ARG);
  NS_ENSURE_FALSE(aContractID.IsEmpty(), NS_ERROR_INVALID_ARG);
  NS_ENSURE_ARG_POINTER(aCapabilities);
  NS_ENSURE_ARG_POINTER(aListener);

  SetName(aFactoryName);
  SetContractID(aContractID);

  mCapabilities = aCapabilities;
  
  nsCOMPtr<nsIThread> target;
  nsresult rv = NS_GetMainThread(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = do_GetProxyForObject(target,
                            NS_GET_IID(sbIMediacoreFactoryWrapperListener),
                            aListener,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(mListener));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//
// sbIMediacoreFactory
//

/*virtual*/ nsresult 
sbMediacoreFactoryWrapper::OnInitBaseMediacoreFactory()
{
  return NS_OK;
}

/*virtual*/ nsresult
sbMediacoreFactoryWrapper::OnGetCapabilities(
                             sbIMediacoreCapabilities **aCapabilities)
{
  NS_ENSURE_STATE(mCapabilities);
  NS_ADDREF(*aCapabilities = mCapabilities);
  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreFactoryWrapper::OnCreate(const nsAString &aInstanceName, 
                                    sbIMediacore **_retval)
{
  NS_ENSURE_STATE(mListener);
  nsresult rv = mListener->OnCreate(aInstanceName, _retval);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}
