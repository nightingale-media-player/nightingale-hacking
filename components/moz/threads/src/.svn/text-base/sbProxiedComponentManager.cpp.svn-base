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

#include "sbProxiedComponentManager.h"

#include <nsAutoPtr.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbProxiedComponentManagerRunnable, nsIRunnable)

NS_IMETHODIMP
sbProxiedComponentManagerRunnable::Run()
{
  NS_ASSERTION(NS_IsMainThread(), "Not main thread!");

  nsCOMPtr<nsIProxyObjectManager> pom =
    do_GetService(NS_XPCOMPROXY_CONTRACTID, &mResult);
  if (NS_FAILED(mResult)) {
    return NS_OK;
  }

  nsCOMPtr<nsISupports> supports;
  if (mIsService) {
    if (mContractID) {
      supports = do_GetService(mContractID, &mResult);
    }
    else {
      supports = do_GetService(mCID, &mResult);
    }
  }
  else {
    if (mContractID) {
      supports = do_CreateInstance(mContractID, &mResult);
    }
    else {
      supports = do_CreateInstance(mCID, &mResult);
    }
  }

  if (NS_FAILED(mResult)) {
    return NS_OK;
  }

  mResult = pom->GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                   mIID,
                                   supports,
                                   NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                   getter_AddRefs(mSupports));

  return NS_OK;
}

nsresult
sbCreateProxiedComponent::operator()(const nsIID& aIID, void** aInstancePtr) const
{
  nsRefPtr<sbProxiedComponentManagerRunnable> runnable =
    new sbProxiedComponentManagerRunnable(mIsService, mCID, mContractID, aIID);
  if (!runnable) {
    *aInstancePtr = 0;
    if (mErrorPtr)
      *mErrorPtr = NS_ERROR_OUT_OF_MEMORY;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = NS_DispatchToMainThread(runnable, NS_DISPATCH_SYNC);
  if (NS_FAILED(rv)) {
    *aInstancePtr = 0;
    if (mErrorPtr)
      *mErrorPtr = rv;
    return rv;
  }

  if (NS_FAILED(runnable->mResult)) {
    *aInstancePtr = 0;
    if (mErrorPtr)
      *mErrorPtr = runnable->mResult;
    return runnable->mResult;
  }

  runnable->mSupports.forget(reinterpret_cast<nsISupports**>(aInstancePtr));

  if (mErrorPtr)
    *mErrorPtr = runnable->mResult;

  return NS_OK;
}

nsresult
sbMainThreadQueryInterface::operator()(const  nsIID& aIID,
                                       void** aInstancePtr) const
{
  nsresult rv;

  // Get the main thread QI'd object.
  if (NS_IsMainThread()) {
    // Already on main thread.  Just QI.
    rv = mSupports->QueryInterface(aIID, aInstancePtr);
  }
  else {
    // Not on main thread.  Get a proxy.
    nsCOMPtr<nsIThread> mainThread;
    rv = NS_GetMainThread(getter_AddRefs(mainThread));
    if (NS_SUCCEEDED(rv)) {
      rv = do_GetProxyForObject(mainThread,
                                aIID,
                                mSupports,
                                NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                aInstancePtr);
    }
  }

  // Return null on failure.
  if (NS_FAILED(rv))
    *aInstancePtr = nsnull;

  // Return result.
  if (mResult)
    *mResult = rv;

  return rv;
}

