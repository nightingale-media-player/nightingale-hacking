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
#include "sbMediaExportITunesAgentService.h"
#include "sbIMediaExportAgentService.h"

#define SB_MEDIAEXPORTAGENTSERVICE_CID                    \
{ 0x625f3a7c, 0x9f5c, 0x4d43, { 0x82, 0xc5, 0xdb, 0x30, 0xe6, 0x7b, 0x60, 0x25 } }

NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaExportITunesAgentService);
NS_DEFINE_NAMED_CID(SB_MEDIAEXPORTAGENTSERVICE_CID);


static const mozilla::Module::CIDEntry kMediaExportITunesAgentCIDs[] = {
  { &kSB_MEDIAEXPORTAGENTSERVICE_CID, false, NULL, sbMediaExportITunesAgentServiceConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kMediaExportITunesAgentContracts[] = {
  { SB_MEDIAEXPORTAGENTSERVICE_CONTRACTID, &kSB_MEDIAEXPORTAGENTSERVICE_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kMediaExportITunesAgentCategories[] = {
  // { "app-startup", SB_MEDIAEXPORTAGENTSERVICE_CLASSNAME, SB_MEDIAEXPORTAGENTSERVICE_CONTRACTID },
  { NULL }
};

static const mozilla::Module kMediaExportITunesAgentModule = {
  mozilla::Module::kVersion,
  kMediaExportITunesAgentCIDs,
  kMediaExportITunesAgentContracts,
  kMediaExportITunesAgentCategories
};

NSMODULE_DEFN(sbMediaExportITunesAgent) = &kMediaExportITunesAgentModule;

