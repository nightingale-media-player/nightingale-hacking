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

/**
 * \file  sbMetadataModule.cpp
 * \brief Songbird Metadata Component Factory and Main Entry Point.
 */

#include <mozilla/ModuleUtils.h>
#include "sbMetadataManager.h"
#include "sbMetadataChannel.h"
#include "sbFileMetadataService.h"
#include "prlog.h"

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(sbMetadataManager, sbMetadataManager::GetSingleton)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMetadataChannel)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbFileMetadataService, Init)

static nsModuleComponentInfo components[] =
{
  {
    SONGBIRD_METADATAMANAGER_CLASSNAME,
    SONGBIRD_METADATAMANAGER_CID,
    SONGBIRD_METADATAMANAGER_CONTRACTID,
    sbMetadataManagerConstructor
  },

  {
    SONGBIRD_METADATACHANNEL_CLASSNAME,
    SONGBIRD_METADATACHANNEL_CID,
    SONGBIRD_METADATACHANNEL_CONTRACTID,
    sbMetadataChannelConstructor
  },

  {
    SONGBIRD_FILEMETADATASERVICE_CLASSNAME,
    SONGBIRD_FILEMETADATASERVICE_CID,
    SONGBIRD_FILEMETADATASERVICE_CONTRACTID,
    sbFileMetadataServiceConstructor
  }
};

// Set up logging
#if defined( PR_LOGGING )
PRLogModuleInfo *gMetadataLog;
PR_STATIC_CALLBACK(nsresult)
InitMetadata(nsIModule *self)
{
  gMetadataLog = PR_NewLogModule("metadata");
  return NS_OK;
}
#else
#define InitMetadata nsnull
#endif

PR_STATIC_CALLBACK(void)
DestroyModule(nsIModule* self)
{
  sbMetadataManager::DestroySingleton();
}

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(SongbirdMetadataComponent,
                                   components,
                                   InitMetadata,
                                   DestroyModule)

