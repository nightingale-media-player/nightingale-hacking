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

#include "sbRemoteMediaItemStatusEvent.h"
#include "sbRemoteAPIUtils.h"
#include "sbRemotePlayer.h"

#include <sbClassInfoUtils.h>

#include <sbIMediaItemStatusEvent.h>

#include <nsComponentManagerUtils.h>
#include <nsIProgrammingLanguage.h>
#include <nsMemory.h>
#include <prlog.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteMediaItemStatusEvent:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gRemoteEventLog = nsnull;
#endif

#undef LOG
#define LOG(args) PR_LOG(gRemoteEventLog, PR_LOG_WARN, args)

const static char* sPublicWProperties[] = { "" };

const static char* sPublicRProperties[] =
{
  // sbIMediaItemStatusEvent
  "helper:item",
  "helper:status",

  // nsIDOMEventTarget
  // N/A
  
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

NS_IMPL_ISUPPORTS7(sbRemoteMediaItemStatusEvent,
                   nsIClassInfo,
                   nsIDOMEvent,
                   nsIDOMNSEvent,
                   nsIPrivateDOMEvent,
                   nsISecurityCheckedComponent,
                   sbIMediaItemStatusEvent,
                   sbISecurityAggregator)

NS_IMPL_CI_INTERFACE_GETTER5(sbRemoteMediaItemStatusEvent,
                             nsISupports,
                             nsIDOMEvent,
                             nsISecurityCheckedComponent,
                             sbISecurityAggregator,
                             sbIMediaItemStatusEvent)

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbRemoteMediaItemStatusEvent)

SB_IMPL_SECURITYCHECKEDCOMP_INIT(sbRemoteMediaItemStatusEvent)

sbRemoteMediaItemStatusEvent::sbRemoteMediaItemStatusEvent( sbRemotePlayer* aRemotePlayer ) :
  mRemotePlayer(aRemotePlayer),
  mStatus( NS_OK )
{
  NS_ASSERTION(aRemotePlayer, "aRemotePlayer is null");
  MOZ_COUNT_CTOR(sbRemoteMediaItemStatusEvent);
#ifdef PR_LOGGING
  if (!gRemoteEventLog) {
    gRemoteEventLog = PR_NewLogModule("sbRemoteMediaItemStatusEvent");
  }
  LOG(("sbRemoteMediaItemStatusEvent::sbRemoteMediaItemStatusEvent()"));
#endif
}

sbRemoteMediaItemStatusEvent::~sbRemoteMediaItemStatusEvent( )
{
  MOZ_COUNT_DTOR(sbRemoteMediaItemStatusEvent);
}

NS_IMETHODIMP sbRemoteMediaItemStatusEvent::InitEvent( nsIDOMEvent* aEvent,
                                                       sbIMediaItem* aMediaItem,
                                                       PRInt32 aStatus )
{
  NS_ENSURE_ARG( aEvent );
  NS_ENSURE_ARG( aMediaItem );
  nsresult rv;
  
  mStatus = aStatus;

  rv = SB_WrapMediaItem(mRemotePlayer, aMediaItem, getter_AddRefs(mWrappedItem));
  NS_ENSURE_SUCCESS(rv, rv);
  
  mEvent = aEvent;
  mNSEvent = do_QueryInterface( mEvent, &rv );
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

/*** sbIMediaItemStatusEvent ***/
/* readonly attribute sbIMediaItem item; */
NS_IMETHODIMP sbRemoteMediaItemStatusEvent::GetItem(sbIMediaItem * *aItem)
{
  NS_ENSURE_ARG(aItem);

  if (!mWrappedItem) {
    *aItem = nsnull;
    return NS_OK;
  }
  NS_IF_ADDREF(*aItem = mWrappedItem.get());
  return (*aItem ? NS_OK : NS_ERROR_FAILURE);
}

/* readonly attribute long status; */
NS_IMETHODIMP sbRemoteMediaItemStatusEvent::GetStatus(PRInt32 *aStatus)
{
  NS_ENSURE_ARG(aStatus);

  *aStatus = mStatus;
  return NS_OK;
}

/*** nsIPrivateDOMEvent ***/
// this macro forwards a method to the inner event (up to one arg)
#define FORWARD_NSIPRIVATEDOMEVENT(_method, _type, _arg) \
  NS_IMETHODIMP sbRemoteMediaItemStatusEvent::_method( _type _arg )            \
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
