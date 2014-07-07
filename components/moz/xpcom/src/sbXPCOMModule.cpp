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

#include "sbArray.h"
#include "sbPropertyBag.h"
#include "sbServiceManager.h"
#include <mozilla/ModuleUtils.h>

NS_GENERIC_FACTORY_CONSTRUCTOR(sbArray)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbPropertyBag, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbServiceManager, Initialize)

#define SB_PROPERTYBAG_CLASSNAME "sbPropertyBag"
#define SB_PROPERTYBAG_CID \
   {0x135c8890, 0xd9da, 0x4a1c, \
     {0xb3, 0x45, 0x3b, 0xb2, 0xfe, 0xe8, 0xfa, 0xb5}}
#define SB_PROPERTYBAG_CONTRACTID "@songbirdnest.com/moz/xpcom/sbpropertybag;1"

NS_DEFINE_NAMED_CID(SB_THREADSAFE_ARRAY_CID);
NS_DEFINE_NAMED_CID(SB_PROPERTYBAG_CID);
NS_DEFINE_NAMED_CID(SB_SERVICE_MANAGER_CID);

static const mozilla::Module::CIDEntry kSongbirdXPCOMLibCIDs[] = {
  { &kSB_THREADSAFE_ARRAY_CID, false, NULL, sbArrayConstructor },
  { &kSB_PROPERTYBAG_CID, false, NULL, sbPropertyBagConstructor },
  { &kSB_SERVICE_MANAGER_CID, false, NULL, sbServiceManagerConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kSongbirdXPCOMLibContracts[] = {
  { SB_THREADSAFE_ARRAY_CONTRACTID, &kSB_THREADSAFE_ARRAY_CID },
  { SB_PROPERTYBAG_CONTRACTID, &kSB_PROPERTYBAG_CID },
  { SB_SERVICE_MANAGER_CONTRACTID, &kSB_SERVICE_MANAGER_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kSongbirdXPCOMLibCategories[] = {
  { NULL }
};

static const mozilla::Module kSongbirdXPCOMLibModule = {
  mozilla::Module::kVersion,
  kSongbirdXPCOMLibCIDs,
  kSongbirdXPCOMLibContracts,
  kSongbirdXPCOMLibCategories
};
 
NSMODULE_DEFN(SongbirdXPCOMLib) = &kSongbirdXPCOMLibModule;
