/*
//
// BEGIN NIGHTINGALE GPL
// 
// This file is part of the Nightingale Media Player.
//
// Copyright(c) 2013 Nightingale Media Player
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
// END NIGHTINGALE GPL
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


NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(sbMetadataManager, sbMetadataManager::GetSingleton);
NS_DEFINE_NAMED_CID(SONGBIRD_METADATAMANAGER_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(sbMetadataChannel);
NS_DEFINE_NAMED_CID(SONGBIRD_METADATACHANNEL_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbFileMetadataService, Init);
NS_DEFINE_NAMED_CID(SONGBIRD_FILEMETADATASERVICE_CID);


static const mozilla::Module::CIDEntry kSongbirdMetadataComponentCIDs[] = {
  { &kSONGBIRD_METADATAMANAGER_CID, false, NULL, sbMetadataManagerConstructor },
  { &kSONGBIRD_METADATACHANNEL_CID, false, NULL, sbMetadataChannelConstructor },
  { &kSONGBIRD_FILEMETADATASERVICE_CID, false, NULL, sbFileMetadataServiceConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kSongbirdMetadataComponentContracts[] = {
  { SONGBIRD_METADATAMANAGER_CONTRACTID, &kSONGBIRD_METADATAMANAGER_CID },
  { SONGBIRD_METADATACHANNEL_CONTRACTID, &kSONGBIRD_METADATACHANNEL_CID },
  { SONGBIRD_FILEMETADATASERVICE_CONTRACTID, &kSONGBIRD_FILEMETADATASERVICE_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kSongbirdMetadataComponentCategoroes[] = {
  { NULL }
};

static const mozilla::Module kSongbirdMetadataComponentModule = {
  mozilla::Module::kVersion,
  kSongbirdMetadataComponentCIDs,
  kSongbirdMetadataComponentContracts,
  kSongbirdMetadataComponentCategoroes
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


NSMODULE_DEFN(SongbirdMetadataComponent) = &kSongbirdMetadataComponentModule;
