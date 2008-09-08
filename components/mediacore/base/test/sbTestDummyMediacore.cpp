/*
 * sbTestDummyMediacore.cpp
 *
 *  Created on: Sep 4, 2008
 *      Author: dbradley
 */

#include "sbTestDummyMediacore.h"
#include <sbBaseMediacoreEventTarget.h>

NS_IMPL_THREADSAFE_ISUPPORTS2(sbTestDummyMediacore, sbIMediacore,
                              sbIMediacoreEventTarget);

sbTestDummyMediacore::sbTestDummyMediacore()
{
  mBaseEventTarget = new sbBaseMediacoreEventTarget(this);
  /* member initializers and constructor code */
}

sbTestDummyMediacore::~sbTestDummyMediacore()
{
  /* destructor code */
}

/* readonly attribute AString instanceName; */
NS_IMETHODIMP sbTestDummyMediacore::GetInstanceName(nsAString & aInstanceName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute sbIMediacoreCapabilities capabilities; */
NS_IMETHODIMP sbTestDummyMediacore::GetCapabilities(sbIMediacoreCapabilities * *aCapabilities)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute sbIMediacoreStatus status; */
NS_IMETHODIMP sbTestDummyMediacore::GetStatus(sbIMediacoreStatus * *aStatus)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute sbIMediacoreSequence currentSequence; */
NS_IMETHODIMP sbTestDummyMediacore::GetCurrentSequence(sbIMediacoreSequence * *aCurrentSequence)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP sbTestDummyMediacore::SetCurrentSequence(sbIMediacoreSequence * aCurrentSequence)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void shutdown (); */
NS_IMETHODIMP sbTestDummyMediacore::Shutdown()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbTestDummyMediacore::DispatchEvent(sbIMediacoreEvent *aEvent,
                                    PRBool aAsync,
                                    PRBool* _retval)
{
  return mBaseEventTarget ? mBaseEventTarget->DispatchEvent(aEvent, aAsync, _retval) : NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
sbTestDummyMediacore::AddListener(sbIMediacoreEventListener *aListener)
{
  return mBaseEventTarget ? mBaseEventTarget->AddListener(aListener) : NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
sbTestDummyMediacore::RemoveListener(sbIMediacoreEventListener *aListener)
{
  return mBaseEventTarget ? mBaseEventTarget->RemoveListener(aListener) : NS_ERROR_NULL_POINTER;

}
