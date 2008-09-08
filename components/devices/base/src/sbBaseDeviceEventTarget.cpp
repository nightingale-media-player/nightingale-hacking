/* vim: set sw=2 :miv */
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

#include "sbBaseDeviceEventTarget.h"

#include <nsIThread.h>
#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsDeque.h>
#include <nsThreadUtils.h>

#include "sbIDeviceEventListener.h"
#include "sbDeviceEvent.h"
#include "sbProxyUtils.h"

class sbDeviceEventTargetRemovalHelper : public nsDequeFunctor {
  public:
    sbDeviceEventTargetRemovalHelper(PRInt32 aIndex)
      : mIndexToRemove(aIndex) {}
    virtual void* operator()(void* aObject)
    {
      sbBaseDeviceEventTarget::DispatchState *state =
        (sbBaseDeviceEventTarget::DispatchState*)aObject;
      if (state->length > mIndexToRemove) {
        --state->length;
      }
      if (state->index >= mIndexToRemove) {
        --state->index;
      }
      /* no return value (it's only useful for FirstThat, not ForEach) */
      return nsnull;
    }
  protected:
    PRInt32 mIndexToRemove;
};

// helper class for async event dispatch, because XPCOM proxies can't deal
// with having out-params.  Note that we discard the result anyway.
class sbDETAsyncDispatchHelper : public nsIRunnable
{
  NS_DECL_ISUPPORTS
  public:
    sbDETAsyncDispatchHelper(sbIDeviceEventTarget* aTarget,
                             sbIDeviceEvent* aEvent)
      : mTarget(aTarget), mEvent(aEvent)
    {
      NS_ASSERTION(aTarget, "sbDeviceEventTargetAsyncDispatchHelper: no target");
      NS_ASSERTION(aEvent, "sbDeviceEventTargetAsyncDispatchHelper: no event");
    }
    NS_IMETHODIMP Run()
    {
      NS_ASSERTION(NS_IsMainThread(),
                   "sbDeviceEventTargetAsyncDispatchHelper: not on main thread!");

      /* ignore return value */
      mTarget->DispatchEvent(mEvent,
                             PR_FALSE, /* don't re-async */
                             nsnull); /* ignore retval */
      return NS_OK;
    }
  private:
    nsCOMPtr<sbIDeviceEventTarget> mTarget;
    nsCOMPtr<sbIDeviceEvent> mEvent;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(sbDETAsyncDispatchHelper, nsIRunnable);

sbBaseDeviceEventTarget::sbBaseDeviceEventTarget()
{
  /* member initializers and constructor code */
  mMonitor = nsAutoMonitor::NewMonitor(__FILE__);
  NS_ASSERTION(mMonitor, "Failed to create monitor");
}

sbBaseDeviceEventTarget::~sbBaseDeviceEventTarget()
{
  /* destructor code */
}

/* boolean dispatchEvent (in sbIDeviceEvent aEvent, [optional] PRBool aAsync); */
NS_IMETHODIMP sbBaseDeviceEventTarget::DispatchEvent(sbIDeviceEvent *aEvent,
                                                     PRBool aAsync,
                                                     PRBool* _retval)
{
  nsresult rv;

  // Note: in the async case, we need to make a new runnable, because
  // DispatchEvent has an out param, and XPCOM proxies can't deal with that.
  if (aAsync) {
    nsRefPtr<sbDETAsyncDispatchHelper> dispatchHelper =
      new sbDETAsyncDispatchHelper(static_cast<sbIDeviceEventTarget*>(this), aEvent);
    NS_ENSURE_TRUE(dispatchHelper, NS_ERROR_OUT_OF_MEMORY);
    rv = NS_DispatchToMainThread(dispatchHelper, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }
  if (!NS_IsMainThread()) {
    // we need to proxy to the main thread
    nsCOMPtr<sbIDeviceEventTarget> proxiedSelf;
    { /* scope the monitor */
      NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
      nsAutoMonitor mon(mMonitor);
      rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                NS_GET_IID(sbIDeviceEventTarget),
                                this,
                                NS_PROXY_SYNC,
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
nsresult sbBaseDeviceEventTarget::DispatchEventInternal(sbIDeviceEvent *aEvent,
                                                        PRBool* _retval)
{
  DispatchState state;
  state.length = mListeners.Count();

  nsresult rv;

  // make sure the event has not already been dispatched
  nsCOMPtr<sbDeviceEvent> event = do_QueryInterface(aEvent, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_FALSE(event->WasDispatched(), NS_ERROR_ALREADY_INITIALIZED);

  // set the event target
  rv = event->SetTarget(this);
  NS_ENSURE_SUCCESS(rv, rv);

  // store thes state into our state stack, so if any listener removes a
  // listener we get updated
  mStates.Push(&state);

  // note that the return value doesn't exist if we're an async proxy
  if (_retval)
    *_retval = PR_FALSE;

  for (state.index = 0; state.index < state.length; ++state.index) {
    rv = mListeners[state.index]->OnDeviceEvent(aEvent);
    /* the return value is only checked on debug builds */
    #if DEBUG
      if (NS_FAILED(rv)) {
        NS_WARNING("Device event listener returned error");
      }
    #endif
    if (_retval)
      *_retval = PR_TRUE;
  }

  // pop the stored state, to ensure we don't end up with a pointer to a stack
  // variable dangling
  mStates.Pop();

  // bubble this event upwards
  if (!mParentEventTarget) {
    // no other event target, just return early
    return NS_OK;
  }

  nsCOMPtr<sbIDeviceEventTarget> parentEventTarget =
    do_QueryReferent(mParentEventTarget, &rv);
  if (NS_FAILED(rv) || !parentEventTarget) {
    // the parent's gone, we don't care all that much
    return NS_OK;
  }

  // always dispatch as sync, since if we wanted to be async, we're already on
  // that path and the caller's already gone.
  rv = parentEventTarget->DispatchEvent(aEvent, PR_TRUE, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* void addEventListener (in sbIDeviceEventListener aListener); */
NS_IMETHODIMP sbBaseDeviceEventTarget::AddEventListener(sbIDeviceEventListener *aListener)
{
  nsresult rv;

  // we need to access the listeners from the main thread only
  if (!NS_IsMainThread()) {
    // we need to proxy to the main thread
    // because we can't just use a monitor (that'll deadlock if we're in the
    // middle of a listener, then got proxied onto a second thread)
    nsCOMPtr<sbIDeviceEventTarget> proxiedSelf;
    { /* scope the monitor */
      NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
      nsAutoMonitor mon(mMonitor);
      rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                NS_GET_IID(sbIDeviceEventTarget),
                                this,
                                NS_PROXY_SYNC,
                                getter_AddRefs(proxiedSelf));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // XXX Mook: consider wrapping the listener in a proxy

    return proxiedSelf->AddEventListener(aListener);
  }

  PRInt32 index = mListeners.IndexOf(aListener);
  if (index >= 0) {
    // the listener already exists, do not re-add
    return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;
  }
  PRBool succeeded = mListeners.AppendObject(aListener);
  return succeeded ? NS_OK : NS_ERROR_FAILURE;
}

/* void removeEventListener (in sbIDeviceEventListener aListener); */
NS_IMETHODIMP sbBaseDeviceEventTarget::RemoveEventListener(sbIDeviceEventListener *aListener)
{
  nsresult rv;

  // we need to access the listeners from the main thread only
  if (!NS_IsMainThread()) {
    // we need to proxy to the main thread
    nsCOMPtr<sbIDeviceEventTarget> proxiedSelf;
    { /* scope the monitor */
      NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
      nsAutoMonitor mon(mMonitor);
      rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                NS_GET_IID(sbIDeviceEventTarget),
                                this,
                                NS_PROXY_SYNC,
                                getter_AddRefs(proxiedSelf));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return proxiedSelf->RemoveEventListener(aListener);
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
  sbDeviceEventTargetRemovalHelper helper(indexToRemove);
  mStates.ForEach(helper);

  return NS_OK;
}
