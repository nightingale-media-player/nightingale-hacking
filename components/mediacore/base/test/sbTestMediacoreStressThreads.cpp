/* vim: set sw=2 :miv */
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

#include "sbTestMediacoreStressThreads.h"

#include <nsAutoLock.h>
#include <nsComponentManagerUtils.h>
#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsIGenericFactory.h>
#include <sbBaseMediacoreEventTarget.h>

#include <sbIMediacoreEvent.h>
#include <sbIMediacoreEventTarget.h>
#include "sbTestMediacoreStressThreads.h"

NS_IMPL_THREADSAFE_ISUPPORTS4(sbTestMediacoreStressThreads,
                              nsIRunnable,
                              sbIMediacore,
                              sbIMediacoreEventListener,
                              sbIMediacoreEventTarget)

sbTestMediacoreStressThreads::sbTestMediacoreStressThreads()
 : mCounter(-999),
   mMonitor(nsnull)
{
  mBaseEventTarget = new sbBaseMediacoreEventTarget(this);
  /* member initializers and constructor code */
}

sbTestMediacoreStressThreads::~sbTestMediacoreStressThreads()
{
  if(mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
  }
  /* destructor code */
}

/* void run (); */
NS_IMETHODIMP sbTestMediacoreStressThreads::Run()
{
  NS_ENSURE_FALSE(mMonitor, NS_ERROR_ALREADY_INITIALIZED);

  mMonitor = nsAutoMonitor::NewMonitor(__FILE__);
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;

  mBaseEventTarget = new sbBaseMediacoreEventTarget(this);

  rv = mBaseEventTarget->AddListener(static_cast<sbIMediacoreEventListener*>(this));
  NS_ENSURE_SUCCESS(rv, rv);

  // spin up a *ton* of threads...
  mCounter = 0;
  for (int i = 0; i < 100; ++i) {
    nsAutoMonitor mon(mMonitor);
    nsCOMPtr<nsIRunnable> event =
      NS_NEW_RUNNABLE_METHOD(sbTestMediacoreStressThreads, this, OnEvent);
    NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<nsIThread> thread;
    ++mCounter;
    rv = NS_NewThread(getter_AddRefs(thread), event);
    NS_ENSURE_SUCCESS(rv, rv);

    mThreads.AppendObject(thread);
  }

  // and wait for them
  while (mThreads.Count()) {
    nsCOMPtr<nsIThread> thread = mThreads[0];
    PRBool succeeded = mThreads.RemoveObjectAt(0);
    NS_ENSURE_TRUE(succeeded, NS_ERROR_FAILURE);
    rv = thread->Shutdown();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mBaseEventTarget->RemoveListener(static_cast<sbIMediacoreEventListener*>(this));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(mCounter == 0, NS_ERROR_FAILURE);

  mBaseEventTarget = nsnull;

  return NS_OK;
}

/* void onMediacoreEvent (in sbMediacoreEvent aEvent); */
NS_IMETHODIMP sbTestMediacoreStressThreads::OnMediacoreEvent(sbIMediacoreEvent *aEvent)
{
  nsAutoMonitor mon(mMonitor);
  --mCounter;
  if (!NS_IsMainThread()) {
    NS_WARNING("Not on main thread!");
    mCounter = -2000; // some random error value
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

void sbTestMediacoreStressThreads::OnEvent()
{
  nsresult rv;

  nsCOMPtr<sbIMediacore> core = do_QueryInterface(NS_ISUPPORTS_CAST(sbIMediacore*, this), &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);

  nsCOMPtr<sbIMediacoreEvent> event;
  rv = sbMediacoreEvent::CreateEvent(sbIMediacoreEvent::URI_CHANGE,
                                     nsnull,
                                     nsnull,
                                     core,
                                     getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, /* void */);

  rv = mBaseEventTarget->DispatchEvent(event, PR_FALSE, nsnull);
  NS_ENSURE_SUCCESS(rv, /* void */);
}

/* readonly attribute AString instanceName; */
NS_IMETHODIMP sbTestMediacoreStressThreads::GetInstanceName(nsAString & aInstanceName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute sbIMediacoreCapabilities capabilities; */
NS_IMETHODIMP sbTestMediacoreStressThreads::GetCapabilities(sbIMediacoreCapabilities * *aCapabilities)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute sbIMediacoreStatus status; */
NS_IMETHODIMP sbTestMediacoreStressThreads::GetStatus(sbIMediacoreStatus * *aStatus)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute sbIMediacoreSequencer sequencer; */
NS_IMETHODIMP sbTestMediacoreStressThreads::GetSequencer(sbIMediacoreSequencer* *aSequencer)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbTestMediacoreStressThreads::SetSequencer(sbIMediacoreSequencer *aSequencer)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void shutdown (); */
NS_IMETHODIMP sbTestMediacoreStressThreads::Shutdown()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbTestMediacoreStressThreads::DispatchEvent(sbIMediacoreEvent *aEvent,
                                            PRBool aAsync,
                                            PRBool* _retval)
{
  return mBaseEventTarget ? mBaseEventTarget->DispatchEvent(aEvent, aAsync, _retval) : NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
sbTestMediacoreStressThreads::AddListener(sbIMediacoreEventListener *aListener)
{
  return mBaseEventTarget ? mBaseEventTarget->AddListener(aListener) : NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
sbTestMediacoreStressThreads::RemoveListener(sbIMediacoreEventListener *aListener)
{
  return mBaseEventTarget ? mBaseEventTarget->RemoveListener(aListener) : NS_ERROR_NULL_POINTER;

}
