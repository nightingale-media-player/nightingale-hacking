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

// Local imports.
#include "sbMediaFileManager.h"
#include "sbMediaManagementJob.h"
#include "sbMediaManagementService.h"

// Mozilla imports.
#include "nsIGenericFactory.h"

// Construct and initialize the sbMediaFileManager object.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMediaFileManager, Init);

// Construct the sbMediaManagementJob object.
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaManagementJob)

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMediaManagementService, Init);

static const nsModuleComponentInfo sbMediaManagerComponents[] =
{
  // Media File Manager component info.
  {
    SB_MEDIAFILEMANAGER_DESCRIPTION,
    SB_MEDIAFILEMANAGER_CID,
    SB_MEDIAFILEMANAGER_CONTRACTID,
    sbMediaFileManagerConstructor
  },
  // Media management job component info.
  {
    SB_MEDIAMANAGEMENTJOB_CLASSNAME,
    SB_MEDIAMANAGEMENTJOB_CID,
    SB_MEDIAMANAGEMENTJOB_CONTRACTID,
    sbMediaManagementJobConstructor
  },
  // Media management service component info.
  {
    SB_MEDIAMANAGEMENTSERVICE_CLASSNAME,
    SB_MEDIAMANAGEMENTSERVICE_CID,
    SB_MEDIAMANAGEMENTSERVICE_CONTRACTID,
    sbMediaManagementServiceConstructor,
    sbMediaManagementService::RegisterSelf
  }
};

NS_IMPL_NSGETMODULE(SongbirdMediaManagerModule, sbMediaManagerComponents)
