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
* \file  MetadataHandlerWMAComponent.cpp
* \brief Songbird WMA Metadata Handler Component Factory and Main Entry Point.
*/

#include <mozilla/ModuleUtils.h>
#include "MetadataHandlerWMA.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbMetadataHandlerWMA);
NS_DEFINE_NAMED_CID(SONGBIRD_METADATAHANDLERWMA_CID);

static const mozilla::Module::CIDEntry kMetadataHandlerWMACIDs[] = {
  { &kSONGBIRD_METADATAHANDLERWMA_CID, false, NULL, sbMetadataHandlerWMAConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kMetadataHandlerWMAContracts[] = {
  { SONGBIRD_METADATAHANDLERWMA_CONTRACTID, &kSONGBIRD_METADATAHANDLERWMA_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kMetadataHandlerWMACategories[] = {
  { NULL }
};

static const mozilla::Module kMetadataHandlerWMAModule = {
    mozilla::Module::kVersion,
    kMetadataHandlerWMACIDs,
    kMetadataHandlerWMAContracts,
    kMetadataHandlerWMACategories
};

NSMODULE_DEFN(sbMetadataHandlerWMAComponent) = &kMetadataHandlerWMAModule;
