/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbWatchFolder.h"
#include "sbWatchFolderServiceCID.h"
#include "sbWatchFolderService.h"


NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbWatchFolder, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbWatchFolderService, Init)

static nsModuleComponentInfo sbWatchFolder[] =
{
  {
    SB_WATCHFOLDER_CLASSNAME,
    SB_WATCHFOLDER_CID,
    SB_WATCHFOLDER_CONTRACTID,
    sbWatchFolderConstructor,
  },
  {
    SONGBIRD_WATCHFOLDERSERVICE_CLASSNAME,
    SONGBIRD_WATCHFOLDERSERVICE_CID,
    SONGBIRD_WATCHFOLDERSERVICE_CONTRACTID,
    sbWatchFolderServiceConstructor,
    sbWatchFolderService::RegisterSelf
  },
};

NS_IMPL_NSGETMODULE(SongbirdWatchFolderComponent, sbWatchFolder)

