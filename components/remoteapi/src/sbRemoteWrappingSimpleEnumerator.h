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

#ifndef __SB_REMOTE_WRAPPINGSIMPLEENUMERATOR_H__
#define __SB_REMOTE_WRAPPINGSIMPLEENUMERATOR_H__

#include "sbRemotePlayer.h"

#include <nsISimpleEnumerator.h>
#include <sbISecurityMixin.h>
#include <sbISecurityAggregator.h>

#include <nsIClassInfo.h>
#include <nsISecurityCheckedComponent.h>
#include <nsStringGlue.h>
#include <nsCOMPtr.h>

class sbRemoteWrappingSimpleEnumerator : public nsIClassInfo,
                                         public nsISecurityCheckedComponent,
                                         public sbISecurityAggregator,
                                         public nsISimpleEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBISECURITYAGGREGATOR
  NS_DECL_NSISIMPLEENUMERATOR

  NS_FORWARD_SAFE_NSISECURITYCHECKEDCOMPONENT(mSecurityMixin)

  sbRemoteWrappingSimpleEnumerator(sbRemotePlayer* aRemotePlayer,
                                   nsISimpleEnumerator* aWrapped) :
    mRemotePlayer(aRemotePlayer),
    mWrapped(aWrapped)
    {
      NS_ASSERTION(aRemotePlayer, "aRemotePlayer is null");
      NS_ASSERTION(aWrapped, "aWrapped is null");
    };

  nsresult Init();

protected:
  nsCOMPtr<nsISecurityCheckedComponent> mSecurityMixin;
  nsRefPtr<sbRemotePlayer> mRemotePlayer;
  nsCOMPtr<nsISimpleEnumerator> mWrapped;
};

#endif // __SB_REMOTE_WRAPPINGSIMPLEENUMERATOR_H__
