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

/**
* \file  sbMediacoreEvent.cpp
* \brief Nightingale Mediacore Event Implementation.
*/
#include "sbMediacoreEvent.h"

#include <nsAutoPtr.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediacoreEvent:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo * MediacoreEvent() {
  static PRLogModuleInfo* gMediacoreEvent = PR_NewLogModule("sbMediacoreEvent");
  return gMediacoreEvent;
}

#define TRACE(args) PR_LOG(MediacoreEvent() , PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(MediacoreEvent() , PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

NS_IMPL_THREADSAFE_ISUPPORTS1(sbMediacoreEvent,
                              sbIMediacoreEvent)

sbMediacoreEvent::sbMediacoreEvent()
: mLock(nsnull)
, mType(sbIMediacoreEvent::UNINTIALIZED)
, mDispatched(PR_FALSE)
{
  MOZ_COUNT_CTOR(sbMediacoreEvent);

  TRACE(("sbMediacoreEvent[0x%x] - Created", this));
}

sbMediacoreEvent::~sbMediacoreEvent()
{
  TRACE(("sbMediacoreEvent[0x%x] - Destroyed", this));

  MOZ_COUNT_DTOR(sbMediacoreEvent);

  if(mLock) {
    nsAutoLock::DestroyLock(mLock);
  }
}

nsresult
sbMediacoreEvent::Init(PRUint32 aType,
                       sbIMediacoreError *aError,
                       nsIVariant *aData,
                       sbIMediacore *aOrigin)
{
  TRACE(("sbMediacoreEvent[0x%x] - Init", this));

  mLock = nsAutoLock::NewLock("sbMediacoreEvent::mLock");
  NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);

  mType = aType;
  mError = aError;
  mData = aData;
  mOrigin = aOrigin;

  return NS_OK;
}

nsresult
sbMediacoreEvent::SetTarget(sbIMediacoreEventTarget *aTarget)
{
  TRACE(("sbMediacoreEvent[0x%x] - SetTarget", this));

  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aTarget);

  nsAutoLock lock(mLock);
  mTarget = aTarget;

  return NS_OK;
}

void
sbMediacoreEvent::Dispatch()
{
  TRACE(("sbMediacoreEvent[0x%x] - Dispatch", this));

  nsAutoLock lock(mLock);
  mDispatched = PR_TRUE;
}

PRBool
sbMediacoreEvent::WasDispatched()
{
  TRACE(("sbMediacoreEvent[0x%x] - WasDispatched", this));

  nsAutoLock lock(mLock);
  return mDispatched;
}

NS_IMETHODIMP
sbMediacoreEvent::GetType(PRUint32 *aType)
{
  TRACE(("sbMediacoreEvent[0x%x] - GetType", this));

  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock lock(mLock);

  *aType = mType;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreEvent::GetError(sbIMediacoreError * *aError)
{
  TRACE(("sbMediacoreEvent[0x%x] - GetError", this));

  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock lock(mLock);

  NS_IF_ADDREF(*aError = mError);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreEvent::GetData(nsIVariant * *aData)
{
  TRACE(("sbMediacoreEvent[0x%x] - GetData", this));

  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock lock(mLock);

  NS_IF_ADDREF(*aData = mData);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreEvent::GetOrigin(sbIMediacore * *aOrigin)
{
  TRACE(("sbMediacoreEvent[0x%x] - GetOrigin", this));

  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock lock(mLock);

  NS_IF_ADDREF(*aOrigin = mOrigin);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreEvent::GetTarget(sbIMediacoreEventTarget * *aTarget)
{
  TRACE(("sbMediacoreEvent[0x%x] - GetTarget", this));

  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  nsAutoLock lock(mLock);

  NS_IF_ADDREF(*aTarget = mTarget);

  return NS_OK;
}

nsresult
sbMediacoreEvent::CreateEvent(PRUint32 aType,
                              sbIMediacoreError * aError,
                              nsIVariant *aData,
                              sbIMediacore *aOrigin,
                              sbIMediacoreEvent **retval)
{
  nsresult rv;
  nsRefPtr<sbMediacoreEvent> event = new sbMediacoreEvent();
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  rv = event->Init(aType, aError, aData, aOrigin);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_IF_ADDREF(*retval = event);

  return NS_OK;
}
