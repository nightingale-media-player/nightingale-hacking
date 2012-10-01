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

#ifndef SBBASEMEDIACOREEVENTTARGET_H_
#define SBBASEMEDIACOREEVENTTARGET_H_

#include <nsAutoPtr.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsDeque.h>
#include <nsIThread.h>
#include <nsThreadUtils.h>
#include <prmon.h>
#include "sbMediacoreEvent.h"

#include <nsIVariant.h>
#include <sbIMediacoreEventTarget.h>

// Forward declared interfaces

class sbIMediacore;
class sbIMediacoreError;
class sbIMediacoreEvent;
class sbIMediacoreEventListener;

/**
 * Base implementation of a mediacore event target.
 * This class provides a thread safe implementation of an event target. All events are dispatched on
 * the main thread.
 * TODO: Revisit the main thread dispatching this may prove a bottleneck for media cores.
 */
class sbBaseMediacoreEventTarget {
public:
  /**
   * Constructor and destructor
   */
  sbBaseMediacoreEventTarget(sbIMediacoreEventTarget * aTarget);
  virtual ~sbBaseMediacoreEventTarget();
  //sbIMediacoreEventTarget interface methods @see sbIMediacoreEventTarget
  /**
   * See sbIMediacoreEventTarget
   */
  virtual
  nsresult AddListener(sbIMediacoreEventListener * aListener);
  /**
   * See sbIMediacoreEventTarget
   */
  virtual
  nsresult RemoveListener(sbIMediacoreEventListener * aListener);
  /**
   * See sbIMediacoreEventTarget
   */
  virtual
  nsresult DispatchEvent(sbIMediacoreEvent * aEvent,
                         PRBool aAsync, PRBool * aDispatched);

protected:
  nsresult DispatchEventInternal(sbIMediacoreEvent *aEvent, PRBool* _retval);

private:
  sbIMediacoreEventTarget * mTarget;
  nsCOMArray<sbIMediacoreEventListener> mListeners;
  PRMonitor* mMonitor;
  /**
   * Tracks the state of the dispatch. This prevents dispatches to removed
   * events as well as ensuring that newly added events see the event
   * TODO: XXX We probably should evaluate dispatching events to listeners
   * added during the event.
   */
  struct DispatchState {
    // these are indices into mListeners
    PRInt32 index; // the currently processing listener index
    PRInt32 length;  // the number of listeners at the start of this dispatch
  };
  // our stack of states (holds *pointers* to DispatchStates)
  nsDeque mStates;
  friend class RemovalHelper;

  /**
   * Helper classes used to enumerate the list of states and remove any that
   * match
   */
  class RemovalHelper : public nsDequeFunctor {
    public:
      /**
       * Initializes the helper with the index to remove
       */
      RemovalHelper(PRInt32 aIndex)
        : mIndexToRemove(aIndex) {}
      /**
       * function operator that removes the index
       */
      virtual void* operator()(void* aObject)
      {
        sbBaseMediacoreEventTarget::DispatchState *state =
          (sbBaseMediacoreEventTarget::DispatchState*)aObject;
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
  class AsyncDispatchHelper : public nsIRunnable
  {
    NS_DECL_ISUPPORTS
    public:
      /**
       * Initializes the dispatch helper with the target and event to dispatch
       */
      AsyncDispatchHelper(sbIMediacoreEventTarget* aTarget,
                               sbIMediacoreEvent* aEvent)
        : mTarget(aTarget), mEvent(aEvent)
      {
        NS_ASSERTION(aTarget, "sbMediacoreEventTargetAsyncDispatchHelper: no target");
        NS_ASSERTION(aEvent, "sbMediacoreEventTargetAsyncDispatchHelper: no event");
      }
      /**
       * Calls the dispatch event method
       */
      NS_IMETHODIMP Run()
      {
        NS_ASSERTION(NS_IsMainThread(),
                     "sbMediacoreEventTargetAsyncDispatchHelper: not on main thread!");

        /* ignore return value */
        mTarget->DispatchEvent(mEvent,
                               PR_FALSE, /* don't re-async */
                               nsnull); /* ignore retval */
        return NS_OK;
      }
    private:
      nsCOMPtr<sbIMediacoreEventTarget> mTarget;
      nsCOMPtr<sbIMediacoreEvent> mEvent;
  };
};

#endif /* SBBASEMEDIACOREEVENTTARGET_H_ */
