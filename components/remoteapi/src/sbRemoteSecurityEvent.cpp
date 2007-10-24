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

#include "sbRemoteSecurityEvent.h"
#include "sbRemoteAPIUtils.h"
#include "sbRemotePlayer.h"

#include <sbClassInfoUtils.h>

#include <sbIRemoteSecurityEvent.h>

#include <nsComponentManagerUtils.h>
#include <nsIDOMDocumentEvent.h>
#include <nsIProgrammingLanguage.h>
#include <nsMemory.h>
#include <prlog.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteSecurityEvent:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gRemoteEventLog = nsnull;
#endif

#undef LOG
#define LOG(args) PR_LOG(gRemoteEventLog, PR_LOG_WARN, args)

const static char* sPublicWProperties[] = { "" };

const static char* sPublicRProperties[] =
{
  // sbIRemoteSecurityEvent
  "helper:siteScope",
  "helper:category",
  "helper:categoryID",
  "helper:hasAccess",

  // nsIDOMEvent
  "helper:type",

  // nsIClassInfo
  "classinfo:classDescription",
  "classinfo:contractID",
  "classinfo:classID",
  "classinfo:implementationLanguage",
  "classinfo:flags"
};

const static char* sPublicMethods[] = { 
  ""
};

NS_IMPL_ISUPPORTS8(sbRemoteSecurityEvent,
                   nsIClassInfo,
                   nsIDOMEvent,
                   nsIDOMNSEvent,
                   nsIPrivateDOMEvent,
                   nsISecurityCheckedComponent,
                   sbIRemoteSecurityEvent,
                   sbIMutableRemoteSecurityEvent,
                   sbISecurityAggregator)

NS_IMPL_CI_INTERFACE_GETTER5(sbRemoteSecurityEvent,
                             nsISupports,
                             nsIDOMEvent,
                             nsISecurityCheckedComponent,
                             sbISecurityAggregator,
                             sbIRemoteSecurityEvent)

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbRemoteSecurityEvent)

SB_IMPL_SECURITYCHECKEDCOMP_INIT(sbRemoteSecurityEvent)

sbRemoteSecurityEvent::sbRemoteSecurityEvent() :
  mHasAccess(PR_FALSE)
{
  MOZ_COUNT_CTOR(sbRemoteSecurityEvent);
#ifdef PR_LOGGING
  if (!gRemoteEventLog) {
    gRemoteEventLog = PR_NewLogModule("sbRemoteSecurityEvent");
  }
  LOG(("sbRemoteMediaItemStatusEvent::sbRemoteMediaItemStatusEvent()"));
#endif
}

sbRemoteSecurityEvent::~sbRemoteSecurityEvent( )
{
  MOZ_COUNT_DTOR(sbRemoteSecurityEvent);
}

// ---------------------------------------------------------------------------
//
//                          sbISecurityAggregator
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP 
sbRemoteSecurityEvent::GetRemotePlayer(sbIRemotePlayer * *aRemotePlayer)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

// ---------------------------------------------------------------------------
//
//                          sbIRemoteSecurityEvent
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP sbRemoteSecurityEvent::InitEvent( nsIDOMEvent* aEvent,
                                                nsIURI *aScopeURI,
                                                const nsAString& aSecurityCategory,
                                                const nsAString& aSecurityCategoryID,
                                                PRBool aHasAccessToCategory )
{
  NS_ENSURE_ARG_POINTER( aEvent );
  NS_ENSURE_ARG_POINTER( aScopeURI );

  nsresult rv;

  mEvent = aEvent;

  mNSEvent = do_QueryInterface( mEvent, &rv );
  NS_ENSURE_SUCCESS( rv, rv );

  mHasAccess = aHasAccessToCategory;
  mCategory  = aSecurityCategory;
  mCategoryID = aSecurityCategoryID;

  mScopeURI = aScopeURI;

  return NS_OK;
}

/*** sbRemoteSecurityEvent ***/
/* attribute nsIURI siteScope; */
NS_IMETHODIMP sbRemoteSecurityEvent::GetSiteScope(nsIURI * *aSiteScope)
{
  NS_ENSURE_ARG_POINTER(aSiteScope);

  *aSiteScope = mScopeURI;
  NS_ADDREF( *aSiteScope );

  return NS_OK;
}

/* attribute AString category; */
NS_IMETHODIMP sbRemoteSecurityEvent::GetCategory(nsAString & aCategory)
{
  aCategory = mCategory;
  return NS_OK;
}

/* attribute AString categoryID */
NS_IMETHODIMP sbRemoteSecurityEvent::GetCategoryID(nsAString & aCategoryID)
{
  aCategoryID = mCategoryID;
  return NS_OK;
}

/* attribute boolean hasAccess; */
NS_IMETHODIMP sbRemoteSecurityEvent::GetHasAccess(PRBool *aHasAccess)
{
  NS_ENSURE_ARG_POINTER(aHasAccess);
  *aHasAccess = mHasAccess;
  return NS_OK;
}

/* void Init (in nsIDOMDocument aDoc, in AString aCategory, in AString aCategoryID, in boolean aHasAccess); */
NS_IMETHODIMP 
sbRemoteSecurityEvent::InitSecurityEvent(nsIDOMDocument *aDoc,
                                         nsIURI *aSiteScope, 
                                         const nsAString & aCategory, 
                                         const nsAString & aCategoryID, 
                                         PRBool aHasAccess)
{
  NS_ENSURE_ARG_POINTER(aSiteScope);
  
  sbRemoteSecurityEvent::Init();

  nsresult rv;

  //change interfaces to create the event
  nsCOMPtr<nsIDOMDocumentEvent>
    docEvent( do_QueryInterface( aDoc, &rv ) );
  NS_ENSURE_SUCCESS( rv , rv );

  //create the event
  nsCOMPtr<nsIDOMEvent> event;
  docEvent->CreateEvent( RAPI_EVENT_CLASS, getter_AddRefs(event) );
  NS_ENSURE_STATE(event);

  rv = event->InitEvent( aHasAccess ? SB_EVENT_RAPI_PERMISSION_CHANGED : SB_EVENT_RAPI_PERMISSION_DENIED, 
                         PR_TRUE, 
                         PR_TRUE );
  NS_ENSURE_SUCCESS( rv , rv );

  //use the document for a target.
  nsCOMPtr<nsIDOMEventTarget>
    eventTarget( do_QueryInterface( aDoc, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );

  //make the event untrusted
  nsCOMPtr<nsIPrivateDOMEvent> privEvt( do_QueryInterface( event, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );
  privEvt->SetTrusted(PR_TRUE);

  return sbRemoteSecurityEvent::InitEvent(event, 
                                          aSiteScope, 
                                          aCategory, 
                                          aCategoryID, 
                                          aHasAccess);
}

/*** nsIPrivateDOMEvent ***/
// this macro forwards a method to the inner event (up to one arg)
#define FORWARD_NSIPRIVATEDOMEVENT(_method, _type, _arg) \
  NS_IMETHODIMP sbRemoteSecurityEvent::_method( _type _arg )                   \
  {                                                                            \
    nsresult rv;                                                               \
    nsCOMPtr<nsIPrivateDOMEvent> inner( do_QueryInterface(mEvent, &rv) );      \
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
