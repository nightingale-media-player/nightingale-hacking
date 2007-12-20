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

#ifndef __SB_REMOTE_MEDIA_ITEM_STATUS_EVENT_H__
#define __SB_REMOTE_MEDIA_ITEM_STATUS_EVENT_H__

#include "sbRemoteAPI.h"

#include <sbIMediaItemStatusEvent.h>
#include <sbISecurityMixin.h>
#include <sbISecurityAggregator.h>
#include <sbIWrappedMediaItem.h>

#include <nsIClassInfo.h>
#include <nsIDOMEvent.h>
#include <nsIDOMEventTarget.h>
#include <nsIDOMNSEvent.h>
#include <nsIPrivateDOMEvent.h>
#include <nsISecurityCheckedComponent.h>
#include <nsStringGlue.h>
#include <nsCOMPtr.h>

class sbIMediaItemStatusEvent;
class sbRemotePlayer;

class sbRemoteMediaItemStatusEvent : public nsIClassInfo,
                                     public nsISecurityCheckedComponent,
                                     public sbISecurityAggregator,
                                     public sbIMediaItemStatusEvent,
                                     public nsIDOMEvent,
                                     public nsIPrivateDOMEvent,
                                     public nsIDOMNSEvent
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBISECURITYAGGREGATOR
  NS_DECL_SBIMEDIAITEMSTATUSEVENT
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
  sbRemoteMediaItemStatusEvent( sbRemotePlayer* aRemotePlayer );
  ~sbRemoteMediaItemStatusEvent( );
  NS_IMETHOD InitEvent( nsIDOMEvent*, sbIMediaItem*, PRInt32 );

protected:
  nsCOMPtr<nsISecurityCheckedComponent> mSecurityMixin;
  nsRefPtr<sbRemotePlayer> mRemotePlayer;

  // sbIMediaItemStatusEvent
  nsCOMPtr<sbIMediaItem> mWrappedItem;
  PRInt32 mStatus;
  nsCOMPtr<nsIDOMEvent> mEvent;
  nsCOMPtr<nsIDOMNSEvent> mNSEvent;
};

#endif /* __SB_REMOTE_MEDIA_ITEM_STATUS_EVENT_H__ */
