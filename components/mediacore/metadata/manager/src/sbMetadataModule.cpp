/*
//
// BEGIN NIGHTINGALE GPL
// 
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

/**
 * \file  sbMetadataModule.cpp
 * \brief Nightingale Metadata Component Factory and Main Entry Point.
 */

#include <nsIGenericFactory.h>
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
    NIGHTINGALE_METADATAMANAGER_CLASSNAME,
    NIGHTINGALE_METADATAMANAGER_CID,
    NIGHTINGALE_METADATAMANAGER_CONTRACTID,
    sbMetadataManagerConstructor
  },

  {
    NIGHTINGALE_METADATACHANNEL_CLASSNAME,
    NIGHTINGALE_METADATACHANNEL_CID,
    NIGHTINGALE_METADATACHANNEL_CONTRACTID,
    sbMetadataChannelConstructor
  },

  {
    NIGHTINGALE_FILEMETADATASERVICE_CLASSNAME,
    NIGHTINGALE_FILEMETADATASERVICE_CID,
    NIGHTINGALE_FILEMETADATASERVICE_CONTRACTID,
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

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(NightingaleMetadataComponent,
                                   components,
                                   InitMetadata,
                                   DestroyModule)

