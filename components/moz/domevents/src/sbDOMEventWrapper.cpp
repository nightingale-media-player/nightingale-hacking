/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2007 POTI, Inc.
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

#include "sbDOMEventWrapper.h"

NS_IMPL_ISUPPORTS4(sbDOMEventWrapper,
                   sbIDOMEventWrapper,
                   nsIDOMEvent,
                   nsIDOMNSEvent,
                   nsIPrivateDOMEvent)

sbDOMEventWrapper::sbDOMEventWrapper()
{
  /* member initializers and constructor code */
}

sbDOMEventWrapper::~sbDOMEventWrapper()
{
  /* destructor code */
}

/* void init (in nsIDOMEvent aBaseEvent, in nsISupports aData); */
NS_IMETHODIMP sbDOMEventWrapper::Init(nsIDOMEvent *aBaseEvent, nsISupports *aData)
{
  NS_ENSURE_ARG_POINTER(aBaseEvent);
  mDOMEvent = aBaseEvent;
  mNSEvent = do_QueryInterface(aBaseEvent);
  mData = aData; // may be null
  return NS_OK;
}

/* readonly attribute nsISupports data; */
NS_IMETHODIMP sbDOMEventWrapper::GetData(nsISupports * *aData)
{
  NS_ENSURE_ARG_POINTER(aData);
  NS_IF_ADDREF(*aData = mData);
  return NS_OK;
}

/*** nsIPrivateDOMEvent ***/
// this macro forwards a method to the inner mouse event (up to one arg)
#define FORWARD_NSIPRIVATEDOMEVENT(_method, _type, _arg) \
  NS_IMETHODIMP sbDOMEventWrapper::_method( _type _arg )                       \
  {                                                                            \
    nsresult rv;                                                               \
    nsCOMPtr<nsIPrivateDOMEvent> inner( do_QueryInterface(mDOMEvent, &rv) );   \
    NS_ENSURE_SUCCESS(rv, rv);                                                 \
    return inner->_method( _arg );                                             \
  }

FORWARD_NSIPRIVATEDOMEVENT(DuplicatePrivateData, , )
FORWARD_NSIPRIVATEDOMEVENT(SetTarget, nsIDOMEventTarget*, aTarget)
FORWARD_NSIPRIVATEDOMEVENT(SetCurrentTarget, nsIDOMEventTarget*, aTarget)
FORWARD_NSIPRIVATEDOMEVENT(SetOriginalTarget, nsIDOMEventTarget*, aTarget)
FORWARD_NSIPRIVATEDOMEVENT(IsDispatchStopped, PRBool*, aIsDispatchPrevented)
FORWARD_NSIPRIVATEDOMEVENT(GetInternalNSEvent, nsEvent**, aNSEvent)
FORWARD_NSIPRIVATEDOMEVENT(HasOriginalTarget, PRBool*, aResult)
FORWARD_NSIPRIVATEDOMEVENT(SetTrusted, PRBool, aTrusted)
