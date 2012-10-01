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

#include "sbClipboardHelper.h"
#include <mozilla/ModuleUtils.h>

NS_GENERIC_FACTORY_CONSTRUCTOR(sbClipboardHelper)
NS_DEFINE_NAMED_CID(SONGBIRD_CLIPBOARD_HELPER_CID);

static const mozilla::Module::CIDEntry kClipboardHelperCIDs[] = {
  { &kSONGBIRD_CLIPBOARD_HELPER_CID, false, NULL, sbClipboardHelperConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kClipboardHelperContracts[] = {
  { SONGBIRD_CLIPBOARD_HELPER_CONTRACTID, &kSONGBIRD_CLIPBOARD_HELPER_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kClipboardHelperCategories[] = {
  { NULL }
};

static const mozilla::Module kClipboardHelperModule = {
  mozilla::Module::kVersion,
  kClipboardHelperCIDs,
  kClipboardHelperContracts,
  kClipboardHelperCategories
};

NSMODULE_DEFN(SongbirdClipboardHelper) = &kClipboardHelperModule;
