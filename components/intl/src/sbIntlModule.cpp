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

#include <nsICategoryManager.h>
#include <mozilla/ModuleUtils.h>
#include <nsServiceManagerUtils.h>

#include "sbStringTransform.h"
#include <sbIStringTransform.h>

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbStringTransform, Init);
NS_DEFINE_NAMED_CID(SB_STRINGTRANSFORM_CID);

static const mozilla::Module::CIDEntry kStringTransformCIDs[] = {
  { &kSB_STRINGTRANSFORM_CID, false, NULL, sbStringTransformConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kStringTransformContracts[] = {
  { SB_STRINGTRANSFORM_CONTRACTID, &kSB_STRINGTRANSFORM_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kStringTransformCategories[] = {
  // { "app-startup", SB_STRINGTRANSFORM_CLASSNAME, SB_STRINGTRANSFORM_CONTRACTID },
  { NULL }
};

static const mozilla::Module kStringTransformModule = {
  mozilla::Module::kVersion,
  kStringTransformCIDs,
  kStringTransformContracts,
  kStringTransformCategories
};

NSMODULE_DEFN(sbStringTransform) = &kStringTransformModule;
