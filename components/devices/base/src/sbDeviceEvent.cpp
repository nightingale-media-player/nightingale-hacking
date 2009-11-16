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

#include "sbDeviceEvent.h"

#include <nsIVariant.h>

#include <sbIDevice.h>
#include "sbIDeviceEventTarget.h"

/* note that we have a "sbDeviceEvent" IID so we can get pointers to this
   concrete class from a COM pointer */
NS_IMPL_THREADSAFE_ISUPPORTS2(sbDeviceEvent,
                              sbDeviceEvent,
                              sbIDeviceEvent)

sbDeviceEvent* sbDeviceEvent::CreateEvent() {
  return new sbDeviceEvent();
}

sbDeviceEvent::sbDeviceEvent()
 : mType(0),
   mWasDispatched(PR_FALSE),
   mDeviceState(sbIDevice::STATE_IDLE),
   mDeviceSubState(sbIDevice::STATE_IDLE)
{
  /* member initializers and constructor code */
}

sbDeviceEvent::~sbDeviceEvent()
{
  /* destructor code */
}

nsresult sbDeviceEvent::InitEvent(PRUint32 aType,
                                  nsIVariant *aData,
                                  nsISupports *aOrigin,
                                  PRUint32 aDeviceState,
                                  PRUint32 aDeviceSubState)
{
  NS_ENSURE_FALSE(mWasDispatched, NS_ERROR_UNEXPECTED);
  mType = aType;
  mData = aData;
  mOrigin = aOrigin;
  mDeviceState = aDeviceState;
  mDeviceSubState = aDeviceSubState;
  return NS_OK;
}

/* readonly attribute PRUint32 type; */
NS_IMETHODIMP sbDeviceEvent::GetType(PRUint32 *aType)
{
  NS_ENSURE_ARG_POINTER(aType);
  *aType = mType;
  return NS_OK;
}

/* readonly attribute nsIVariant data; */
NS_IMETHODIMP sbDeviceEvent::GetData(nsIVariant * *aData)
{
  NS_ENSURE_ARG_POINTER(aData);
  NS_IF_ADDREF(*aData = mData);
  return NS_OK;
}

/* attribute sbIDeviceEventTarget target setter; */
nsresult sbDeviceEvent::SetTarget(sbIDeviceEventTarget *aTarget)
{
  mTarget = aTarget;
  return NS_OK;
}

/* readonly attribute sbIDeviceEventTarget target; */
NS_IMETHODIMP sbDeviceEvent::GetTarget(sbIDeviceEventTarget * *aTarget)
{
  NS_ENSURE_ARG_POINTER(aTarget);
  NS_IF_ADDREF(*aTarget = mTarget);
  return NS_OK;
}

/* readonly attribute nsISupports origin; */
NS_IMETHODIMP sbDeviceEvent::GetOrigin(nsISupports * *aOrigin)
{
  NS_ENSURE_ARG_POINTER(aOrigin);
  NS_IF_ADDREF(*aOrigin = mOrigin);
  return NS_OK;
}

nsresult sbDeviceEvent::CreateEvent(PRUint32 aType,
                                    nsIVariant *aData,
                                    nsISupports *aOrigin,
                                    PRUint32 aDeviceState,
                                    PRUint32 aDeviceSubState,
                                    sbIDeviceEvent **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  nsCOMPtr<sbDeviceEvent> event = new sbDeviceEvent();
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = event->InitEvent(aType,
                                 aData,
                                 aOrigin,
                                 aDeviceState,
                                 aDeviceSubState);
  NS_ENSURE_SUCCESS(rv, rv);
  return CallQueryInterface(event, _retval);
}

NS_IMETHODIMP sbDeviceEvent::GetDeviceState(PRUint32 * aDeviceState)
{
  NS_ENSURE_ARG_POINTER(aDeviceState);

  *aDeviceState = mDeviceState;
}

NS_IMETHODIMP sbDeviceEvent::GetDeviceSubState(PRUint32 * aDeviceSubState)
{
  NS_ENSURE_ARG_POINTER(aDeviceSubState);

  *aDeviceSubState = mDeviceSubState;
}
