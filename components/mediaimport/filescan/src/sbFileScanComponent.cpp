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
* \file  FileScanComponent.cpp
* \brief Songbird FileScan Component Factory and Main Entry Point.
*/

#include <mozilla/ModuleUtils.h>

#include "sbFileScan.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbFileScan);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbFileScanQuery);

NS_DEFINE_NAMED_CID(SONGBIRD_FILESCAN_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_FILESCANQUERY_CID);

static const mozilla::Module::CIDEntry kFileScanCIDs[] = {
  { &kSONGBIRD_FILESCAN_CID, false, NULL, sbFileScanConstructor },
  { &kSONGBIRD_FILESCANQUERY_CID, false, NULL, sbFileScanQueryConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kFileScanContracts[] = {
  { SONGBIRD_FILESCAN_CONTRACTID, &kSONGBIRD_FILESCAN_CID },
  { SONGBIRD_FILESCANQUERY_CONTRACTID, &kSONGBIRD_FILESCANQUERY_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kFileScanCategories[] = {
  { NULL }
};

static const mozilla::Module kFileScanModule = {
  mozilla::Module::kVersion,
  kFileScanCIDs,
  kFileScanContracts,
  kFileScanCategories
};

NSMODULE_DEFN(sbFileScan) = &kFileScanModule;
