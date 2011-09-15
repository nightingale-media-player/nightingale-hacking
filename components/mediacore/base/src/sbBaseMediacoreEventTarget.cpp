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

#include "sbBaseMediacoreEventTarget.h"

#include <nsAutoLock.h>
#include <nsComponentManagerUtils.h>

#include <sbIMediacore.h>
#include <sbIMediacoreError.h>
#include <sbIMediacoreEventListener.h>

#include <sbMediacoreEvent.h>
#include <sbProxiedComponentManager.h>

/* ctor / dtor */
sbBaseMediacoreEventTarget::sbBaseMediacoreEventTarget(sbIMediacoreEventTarget * aTarget)
  : mTarget(aTarget),
    mMonitor(nsAutoMonitor::NewMonitor("sbBaseMediacoreEventTarget::mMonitor"))
{
}
sbBaseMediacoreEventTarget::~sbBaseMediacoreEventTarget()
{
  nsAutoMonitor::DestroyMonitor(mMonitor);
}


/* boolean dispatchEvent (in sbIMediacoreEvent aEvent, [optional] PRBool aAsync); */
nsresult
sbBaseMediacoreEventTarget::DispatchEvent(sbIMediacoreEvent *aEvent,
                                          PRBool aAsync,
                                          PRBool* _retval)
{
  nsresult rv;

  // Note: in the async case, we need to make a new runnable, because
  // DispatchEvent has an out param, and XPCOM proxies can't deal with that.
  if (aAsync) {
    nsRefPtr<AsyncDispatchHelper> dispatchHelper =
      new AsyncDispatchHelper(static_cast<sbIMediacoreEventTarget*>(mTarget), aEvent);
    NS_ENSURE_TRUE(dispatchHelper, NS_ERROR_OUT_OF_MEMORY);
    rv = NS_DispatchToMainThread(dispatchHelper, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }
  if (!NS_IsMainThread()) {
    // we need to proxy to the main thread
    nsCOMPtr<sbIMediacoreEventTarget> proxiedSelf;
    { /* scope the monitor */
      NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
      nsAutoMonitor mon(mMonitor);
      rv = do_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                NS_GET_IID(sbIMediacoreEventTarget),
                                mTarget,
                                NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                getter_AddRefs(proxiedSelf));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    // don't have a return value if dispatching asynchronously
    // (since the variable is likely to be dead by that point)
    return proxiedSelf->DispatchEvent(aEvent, PR_FALSE, _retval);
  }

  return DispatchEventInternal(aEvent, _retval);
}

/* Dispatch an event, assuming we're already on the main thread */
nsresult
sbBaseMediacoreEventTarget::DispatchEventInternal(sbIMediacoreEvent *aEvent,
                                                  PRBool* _retval)
{
  DispatchState state;
  state.length = mListeners.Count();

  nsresult rv;

  // make sure the event has not already been dispatched
  nsCOMPtr<sbMediacoreEvent> event = do_QueryInterface(aEvent, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_FALSE(event->WasDispatched(), NS_ERROR_ALREADY_INITIALIZED);

  // set the event target
  rv = event->SetTarget(mTarget);
  NS_ENSURE_SUCCESS(rv, rv);

  // store the state into our state stack, so if any listener removes a
  // listener we get updated
  mStates.Push(&state);

  // note that the return value doesn't exist if we're an async proxy
  if (_retval)
    *_retval = PR_FALSE;

  for (state.index = 0; state.index < state.length; ++state.index) {
    rv = mListeners[state.index]->OnMediacoreEvent(aEvent);
    /* the return value is only checked on debug builds */
    #if DEBUG
      if (NS_FAILED(rv)) {
        NS_WARNING("Mediacore event listener returned error");
      }
    #endif
    if (_retval)
      *_retval = PR_TRUE;
  }

  // pop the stored state, to ensure we don't end up with a pointer to a stack
  // variable dangling
  mStates.Pop();

  return NS_OK;
}

  /* void addEventListener (in sbIMediacoreEventListener aListener); */
nsresult
sbBaseMediacoreEventTarget::AddListener(sbIMediacoreEventListener *aListener)
{
  nsresult rv;

  // we need to access the listeners from the main thread only
  if (!NS_IsMainThread()) {
    // we need to proxy to the main thread
    // because we can't just use a monitor (that'll deadlock if we're in the
    // middle of a listener, then got proxied onto a second thread)
    nsCOMPtr<sbIMediacoreEventTarget> proxiedSelf;
    { /* scope the monitor */
      NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
      nsAutoMonitor mon(mMonitor);
      rv = do_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                NS_GET_IID(sbIMediacoreEventTarget),
                                mTarget,
                                NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                getter_AddRefs(proxiedSelf));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // XXX Mook: consider wrapping the listener in a proxy

    return proxiedSelf->AddListener(aListener);
  }

  PRInt32 index = mListeners.IndexOf(aListener);
  if (index >= 0) {
    // the listener already exists, do not re-add
    return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;
  }
  PRBool succeeded = mListeners.AppendObject(aListener);
  return succeeded ? NS_OK : NS_ERROR_FAILURE;
}

  /* void removeEventListener (in sbIMediacoreEventListener aListener); */
nsresult
sbBaseMediacoreEventTarget::RemoveListener(sbIMediacoreEventListener *aListener)
{
  nsresult rv;

  // we need to access the listeners from the main thread only
  if (!NS_IsMainThread()) {
    // we need to proxy to the main thread
    nsCOMPtr<sbIMediacoreEventTarget> proxiedSelf;
    { /* scope the monitor */
      NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
      nsAutoMonitor mon(mMonitor);
      rv = do_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                NS_GET_IID(sbIMediacoreEventTarget),
                                mTarget,
                                NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                getter_AddRefs(proxiedSelf));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return proxiedSelf->RemoveListener(aListener);
  }

  // XXX Mook: if we wrapped listeners, watch for equality!
  PRInt32 indexToRemove = mListeners.IndexOf(aListener);
  if (indexToRemove < 0) {
    // err, no such listener
    return NS_OK;
  }

  // remove the listener
  PRBool succeeded = mListeners.RemoveObjectAt(indexToRemove);
  NS_ENSURE_TRUE(succeeded, NS_ERROR_FAILURE);

  // fix up the stack to account for the removed listener
  // (decrease the stored length of the listener array)
  RemovalHelper helper(indexToRemove);
  mStates.ForEach(helper);

  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(sbBaseMediacoreEventTarget::AsyncDispatchHelper, 
                              nsIRunnable);
