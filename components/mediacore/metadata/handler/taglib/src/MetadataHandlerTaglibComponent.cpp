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
* \file  MetadataHandlerTaglibComponent.cpp
* \brief Songbird taglib Metadata Handler Component Factory and Main Entry
* Point.
*/

#include "mozilla/ModuleUtils.h"
#include "MetadataHandlerTaglib.h"
#include "TaglibChannelFileIOManager.h"
#include "SeekableChannel.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMetadataHandlerTaglib, Init);
NS_DEFINE_NAMED_CID(SONGBIRD_METADATAHANDLERTAGLIB_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbTagLibChannelFileIOManager, FactoryInit);
NS_DEFINE_NAMED_CID(SONGBIRD_TAGLIBCHANNELFILEIOMANAGER_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbSeekableChannel);
NS_DEFINE_NAMED_CID(SONGBIRD_SEEKABLECHANNEL_CID);

static const mozilla::Module::CIDEntry kMetadataHandlerTaglibCIDs[] = {
  { &kSONGBIRD_METADATAHANDLERTAGLIB_CID, false, NULL, sbMetadataHandlerTaglibConstructor },
  { &kSONGBIRD_TAGLIBCHANNELFILEIOMANAGER_CID, false, NULL, sbTagLibChannelFileIOManagerConstructor },
  { &kSONGBIRD_SEEKABLECHANNEL_CID, false, NULL, sbSeekableChannelConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kMetadataHandlerTaglibContracts[] = {
  { SONGBIRD_METADATAHANDLERTAGLIB_CONTRACTID, &kSONGBIRD_METADATAHANDLERTAGLIB_CID },
  { SONGBIRD_TAGLIBCHANNELFILEIOMANAGER_CONTRACTID, &kSONGBIRD_TAGLIBCHANNELFILEIOMANAGER_CID },
  { SONGBIRD_SEEKABLECHANNEL_CONTRACTID, &kSONGBIRD_SEEKABLECHANNEL_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kMetadataHandlerTaglibCategories[] = {
  { NULL }
};

static const mozilla::Module kMetadataHandlerTaglibModule = {
  mozilla::Module::kVersion,
  kMetadataHandlerTaglibCIDs,
  kMetadataHandlerTaglibContracts,
  kMetadataHandlerTaglibCategories
};

NSMODULE_DEFN(SongbirdMetadataHandlerTaglibComponent) = &kMetadataHandlerTaglibModule;
