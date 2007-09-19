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

#include "sbRemotePlaylistClickEvent.h"
#include "sbRemoteAPIUtils.h"
#include "sbRemotePlayer.h"

#include <sbClassInfoUtils.h>

#include <sbIPlaylistClickEvent.h>

#include <nsComponentManagerUtils.h>
#include <nsIProgrammingLanguage.h>
#include <nsMemory.h>
#include <prlog.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemotePlaylistClickEvent:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gRemoteEventLog = nsnull;
#endif

#undef LOG
#define LOG(args) PR_LOG(gRemoteEventLog, PR_LOG_WARN, args)

const static char* sPublicWProperties[] = { "" };

const static char* sPublicRProperties[] =
{
  // sbIPlaylistClickEvent
  "helper:property",
  "helper:item",

  // nsIDOMEventTarget
  // N/A
  
  // nsIDOMMouseEvent
  "helper:button",
  "helper:ctrlKey",
  "helper:altKey",
  "helper:shiftKey",
  "helper:metaKey",

  // nsIClassInfo
  "classinfo:classDescription",
  "classinfo:contractID",
  "classinfo:classID",
  "classinfo:implementationLanguage",
  "classinfo:flags"
};

const static char* sPublicMethods[] =
{ 
  // no applicable methods
  ""
};

NS_IMPL_ISUPPORTS9(sbRemotePlaylistClickEvent,
                   nsIClassInfo,
                   nsIDOMEvent,
                   nsIDOMMouseEvent,
                   nsIDOMNSEvent,
                   nsIDOMUIEvent,
                   nsIPrivateDOMEvent,
                   nsISecurityCheckedComponent,
                   sbIPlaylistClickEvent,
                   sbISecurityAggregator)

NS_IMPL_CI_INTERFACE_GETTER7(sbRemotePlaylistClickEvent,
                             nsISupports,
                             nsIDOMEvent,
                             nsIDOMMouseEvent,
                             nsIDOMUIEvent,
                             nsISecurityCheckedComponent,
                             sbISecurityAggregator,
                             sbIPlaylistClickEvent)

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbRemotePlaylistClickEvent)

SB_IMPL_SECURITYCHECKEDCOMP_INIT(sbRemotePlaylistClickEvent)

sbRemotePlaylistClickEvent::sbRemotePlaylistClickEvent( sbRemotePlayer* aRemotePlayer ) :
  mRemotePlayer(aRemotePlayer)
{
  NS_ASSERTION(aRemotePlayer, "aRemotePlayer is null");
  MOZ_COUNT_CTOR(sbRemotePlaylistClickEvent);
#ifdef PR_LOGGING
  if (!gRemoteEventLog) {
    gRemoteEventLog = PR_NewLogModule("sbRemotePlaylistClickEvent");
  }
  LOG(("sbRemotePlaylistClickEvent::sbRemotePlaylistClickEvent()"));
#endif
}

sbRemotePlaylistClickEvent::~sbRemotePlaylistClickEvent( )
{
  MOZ_COUNT_DTOR(sbRemotePlaylistClickEvent);
}

NS_IMETHODIMP sbRemotePlaylistClickEvent::InitEvent( sbIPlaylistClickEvent* aClickEvent,
                                                     nsIDOMMouseEvent* aMouseEvent )
{
  NS_ENSURE_ARG( aClickEvent );
  NS_ENSURE_ARG( aMouseEvent );
  nsresult rv;
  
  nsCOMPtr<sbIMediaItem> item;
  rv = aClickEvent->GetItem( getter_AddRefs(item) );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SB_WrapMediaItem(mRemotePlayer, item, getter_AddRefs(mWrappedItem));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aClickEvent->GetProperty( mProperty );
  NS_ENSURE_SUCCESS(rv, rv);
  
  mMouseEvent = aMouseEvent;
  mNSEvent = do_QueryInterface( mMouseEvent, &rv );
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

/*** sbIPlaylistClickEvent ***/
/* readonly attribute AString property; */
NS_IMETHODIMP sbRemotePlaylistClickEvent::GetProperty(nsAString & aProperty)
{
  // make sure ::InitEvent() has succeeded properly (mNSEvent is the last thing)
  NS_ENSURE_STATE(mNSEvent);

  aProperty.Assign(mProperty);
  return NS_OK;
}

/* readonly attribute sbIMediaItem item; */
NS_IMETHODIMP sbRemotePlaylistClickEvent::GetItem(sbIMediaItem * *aItem)
{
  NS_ENSURE_ARG(aItem);

  if (!mWrappedItem) {
    *aItem = nsnull;
    return NS_OK;
  }
  NS_IF_ADDREF(*aItem = mWrappedItem.get());
  return (*aItem ? NS_OK : NS_ERROR_FAILURE);
}

/*** nsIPrivateDOMEvent ***/
// this macro forwards a method to the inner mouse event (up to one arg)
#define FORWARD_NSIPRIVATEDOMEVENT(_method, _type, _arg) \
  NS_IMETHODIMP sbRemotePlaylistClickEvent::_method( _type _arg )              \
  {                                                                            \
    nsresult rv;                                                               \
    nsCOMPtr<nsIPrivateDOMEvent> inner( do_QueryInterface(mMouseEvent, &rv) ); \
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
