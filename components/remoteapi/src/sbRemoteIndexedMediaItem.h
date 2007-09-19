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

#ifndef __SB_REMOTE_INDEXEDMEDIAITEM_H__
#define __SB_REMOTE_INDEXEDMEDIAITEM_H__

#include "sbRemoteAPI.h"

#include <sbIMediaItem.h>
#include <sbISecurityMixin.h>
#include <sbISecurityAggregator.h>

#include <nsIClassInfo.h>
#include <nsISecurityCheckedComponent.h>
#include <nsCOMPtr.h>

class sbRemotePlayer;

class sbRemoteIndexedMediaItem : public nsIClassInfo,
                                 public nsISecurityCheckedComponent,
                                 public sbISecurityAggregator,
                                 public sbIIndexedMediaItem
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBIINDEXEDMEDIAITEM
  NS_DECL_SBISECURITYAGGREGATOR
  SB_DECL_SECURITYCHECKEDCOMP_INIT

  NS_FORWARD_SAFE_NSISECURITYCHECKEDCOMPONENT(mSecurityMixin);

  sbRemoteIndexedMediaItem(sbRemotePlayer* aRemotePlayer,
                           sbIIndexedMediaItem* aIndexedMediaItem);

protected:

  nsCOMPtr<nsISecurityCheckedComponent> mSecurityMixin;

  nsRefPtr<sbRemotePlayer> mRemotePlayer;
  nsCOMPtr<sbIIndexedMediaItem> mIndexedMediaItem;
};

#endif // __SB_REMOTE_INDEXEDMEDIAITEM_H__
