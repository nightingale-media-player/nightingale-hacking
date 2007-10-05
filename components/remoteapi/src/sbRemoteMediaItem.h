/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

#ifndef __SB_REMOTE_MEDIAITEM_H__
#define __SB_REMOTE_MEDIAITEM_H__

#include "sbRemoteAPI.h"
#include "sbRemoteForwardingMacros.h"

#include <sbIMediaItem.h>
#include <sbILibraryResource.h>
#include <sbISecurityMixin.h>
#include <sbISecurityAggregator.h>
#include <sbIWrappedMediaItem.h>

#include <nsIClassInfo.h>
#include <nsISecurityCheckedComponent.h>
#include <nsStringGlue.h>
#include <nsCOMPtr.h>

class sbRemotePlayer;

class sbRemoteMediaItem : public nsIClassInfo,
                          public nsISecurityCheckedComponent,
                          public sbISecurityAggregator,
                          public sbIMediaItem,
                          public sbIWrappedMediaItem
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBISECURITYAGGREGATOR
  SB_DECL_SECURITYCHECKEDCOMP_INIT

  NS_FORWARD_SAFE_SBIMEDIAITEM(mMediaItem);
  NS_FORWARD_SAFE_NSISECURITYCHECKEDCOMPONENT(mSecurityMixin);
  NS_FORWARD_SAFE_SBILIBRARYRESOURCE_NO_SETGETPROPERTY_SETPROPERTIES(mMediaItem);

  NS_IMETHOD GetProperty(const nsAString& aName, nsAString& _retval);
  NS_IMETHOD SetProperty(const nsAString& aName, const nsAString& aValue);
  NS_IMETHOD SetProperties(sbIPropertyArray* aProperties);

  // sbIWrappedMediaItem interface
  virtual already_AddRefed<sbIMediaItem> GetMediaItem();

  sbRemoteMediaItem( sbRemotePlayer* aRemotePlayer,
                     sbIMediaItem* aMediaItem );

protected:

  nsCOMPtr<nsISecurityCheckedComponent> mSecurityMixin;

  nsRefPtr<sbRemotePlayer> mRemotePlayer;
  nsCOMPtr<sbIMediaItem> mMediaItem;
};

#endif // __SB_REMOTE_MEDIAITEM_H__
