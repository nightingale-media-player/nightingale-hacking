/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#include "sbProxiedServices.h"
#include <nsServiceManagerUtils.h>
#include <nsIObserverService.h>
#include <nsThreadUtils.h>
#include <sbProxyUtils.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbProxiedServices, sbIProxiedServices)

nsresult
sbProxiedServices::Init()
{
  NS_ASSERTION(NS_IsMainThread(), "Init called off the main thread!");
  nsresult rv;

  nsCOMPtr<nsIObserverService> os =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = os->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the proxies here since we know we're on the main thread
  // XXXsteve Technically we should write a little threadsafe wrapper that
  // has threadsafe addref/release/QI calls.  See:
  // https://bugzilla.mozilla.org/show_bug.cgi?id=418284
  nsCOMPtr<nsIUTF8ConverterService> utf8ConverterService =
    do_GetService("@mozilla.org/intl/utf8converterservice;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                            NS_GET_IID(nsIUTF8ConverterService),
                            utf8ConverterService,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(mUTF8ConverterService));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbProxiedServices::GetUtf8ConverterService(nsIUTF8ConverterService** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(mUTF8ConverterService);

  NS_ADDREF(*_retval = mUTF8ConverterService);
  return NS_OK;
}

NS_IMETHODIMP
sbProxiedServices::Observe(nsISupports* aSubject,
                           const char* aTopic,
                           const PRUnichar* aData)
{
  if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {

    // Null out the handle to the service
    mUTF8ConverterService = nsnull;

    nsCOMPtr<nsIObserverService> os =
      do_GetService("@mozilla.org/observer-service;1");
    if (os) {
      os->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    }
  }

  return NS_OK;
}
