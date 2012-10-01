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

#include <mozilla/ModuleUtils.h>
#include <nsIServiceManager.h>

#include "sbPlaybackHistoryEntry.h"
#include "sbPlaybackHistoryService.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbPlaybackHistoryEntry)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbPlaybackHistoryService, Init);

static const nsModuleComponentInfo components[] =
{
  {
    SB_PLAYBACKHISTORYENTRY_DESCRIPTION,
    SB_PLAYBACKHISTORYENTRY_CID,
    SB_PLAYBACKHISTORYENTRY_CONTRACTID,
    sbPlaybackHistoryEntryConstructor,
    nsnull
  },
  {
    SB_PLAYBACKHISTORYSERVICE_DESCRIPTION,
    SB_PLAYBACKHISTORYSERVICE_CID,
    SB_PLAYBACKHISTORYSERVICE_CONTRACTID,
    sbPlaybackHistoryServiceConstructor,
    sbPlaybackHistoryService::RegisterSelf
  }
};

NS_IMPL_NSGETMODULE(sbPlaybackHistoryComponent, components)

