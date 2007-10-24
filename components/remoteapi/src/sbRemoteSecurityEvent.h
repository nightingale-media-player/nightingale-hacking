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

#ifndef __SB_REMOTE_SECURITY_EVENT_H__
#define __SB_REMOTE_SECURITY_EVENT_H__

#include "sbRemoteAPI.h"

#include <sbISecurityMixin.h>
#include <sbISecurityAggregator.h>
#include <sbIRemoteSecurityEvent.h>

#include <nsIClassInfo.h>
#include <nsIDOMEvent.h>
#include <nsIDOMEventTarget.h>
#include <nsIDOMNSEvent.h>
#include <nsIPrivateDOMEvent.h>
#include <nsISecurityCheckedComponent.h>
#include <nsStringGlue.h>
#include <nsCOMPtr.h>

#define SONGBIRD_SECURITYEVENT_CONTRACTID                 \
  "@songbirdnest.com/remoteapi/security-event;1"
#define SONGBIRD_SECURITYEVENT_CLASSNAME                  \
  "Songbird Remote Security Event"
#define SONGBIRD_SECURITYEVENT_CID                        \
{ /* 17097f88-2b36-4bbc-a74b-a3199a3e8c43 */              \
  0x17097f88,                                             \
  0x2b36,                                                 \
  0x4bbc,                                                 \
  { 0xa7, 0x4b, 0xa3, 0x19, 0x9a, 0x3e, 0x8c, 0x43 }      \
}

class sbIRemoteSecurityEvent;
class sbRemotePlayer;

class sbRemoteSecurityEvent : public nsIClassInfo,
                              public nsISecurityCheckedComponent,
                              public sbISecurityAggregator,
                              public sbIRemoteSecurityEvent,
                              public sbIMutableRemoteSecurityEvent,
                              public nsIDOMEvent,
                              public nsIPrivateDOMEvent,
                              public nsIDOMNSEvent
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBISECURITYAGGREGATOR
  NS_DECL_SBIREMOTESECURITYEVENT
  NS_DECL_SBIMUTABLEREMOTESECURITYEVENT
  SB_DECL_SECURITYCHECKEDCOMP_INIT

  NS_FORWARD_SAFE_NSISECURITYCHECKEDCOMPONENT(mSecurityMixin);
  NS_FORWARD_SAFE_NSIDOMEVENT(mEvent);
  NS_FORWARD_SAFE_NSIDOMNSEVENT(mNSEvent);

/** nsIPrivateDOMEvent - this is non-XPCOM so no forward macro **/
  NS_IMETHOD DuplicatePrivateData();
  NS_IMETHOD SetTarget(nsIDOMEventTarget* aTarget);
  NS_IMETHOD SetCurrentTarget(nsIDOMEventTarget* aTarget);
  NS_IMETHOD SetOriginalTarget(nsIDOMEventTarget* aTarget);
  NS_IMETHOD IsDispatchStopped(PRBool* aIsDispatchPrevented);
  NS_IMETHOD GetInternalNSEvent(nsEvent** aNSEvent);
  NS_IMETHOD HasOriginalTarget(PRBool* aResult);
  NS_IMETHOD SetTrusted(PRBool aTrusted);

public:
  sbRemoteSecurityEvent( );
  ~sbRemoteSecurityEvent( );

  NS_IMETHOD InitEvent( nsIDOMEvent*, 
                        nsIURI *,
                        const nsAString &, 
                        const nsAString &,
                        PRBool );

protected:
  nsCOMPtr<nsISecurityCheckedComponent> mSecurityMixin;

  nsCOMPtr<nsIDOMEvent> mEvent;
  nsCOMPtr<nsIDOMNSEvent> mNSEvent;
  
  // this is the actual event payload.
  nsCOMPtr<nsIURI> mScopeURI;
  PRBool           mHasAccess;
  nsString         mCategory;
  nsString         mCategoryID;
};

#endif /* __SB_REMOTE_SECURITY_EVENT_H__ */
