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

#ifndef __SB_REMOTE_COMMANDS_H__
#define __SB_REMOTE_COMMANDS_H__

#include "sbRemoteAPI.h"
#include <sbIPlaylistCommands.h>
#include <sbIRemoteCommands.h>
#include <sbIRemotePlayer.h>
#include <sbISecurityAggregator.h>
#include <sbISecurityMixin.h>

#include <nsIClassInfo.h>
#include <nsIGenericFactory.h>
#include <nsISecurityCheckedComponent.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsCOMPtr.h>
#include <map>

class nsIDOMNode;
class nsIWeakReference;

struct sbCommand {
  nsString type;
  nsString id;
  nsString name;
  nsString tooltip;
};

class sbRemoteCommands : public sbIRemoteCommands,
                         public nsIClassInfo,
                         public nsISecurityCheckedComponent,
                         public sbIPlaylistCommands,
                         public sbISecurityAggregator
{
public:
  NS_DECL_SBIREMOTECOMMANDS
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBISECURITYAGGREGATOR
  NS_DECL_SBIPLAYLISTCOMMANDS

  NS_FORWARD_SAFE_NSISECURITYCHECKEDCOMPONENT(mSecurityMixin)

  SB_DECL_SECURITYCHECKEDCOMP_INIT

  sbRemoteCommands();

protected:
  virtual ~sbRemoteCommands();
  void DoCommandsUpdated();

  // sbIPlaylistCommands stuff
  nsCOMPtr<nsIWeakReference> mWeakOwner;
  nsCOMPtr<sbIPlaylistCommandsContext> mContext;
  nsTArray<sbCommand> mCommands;
  std::map<nsString, nsString> mProperties;

  // SecurityCheckedComponent stuff
  nsCOMPtr<nsISecurityCheckedComponent> mSecurityMixin;
};

#endif // __SB_REMOTE_COMMANDS_H__
