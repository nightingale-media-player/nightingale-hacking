/* vim: set sw=2 :miv */
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
* \file  sbMediacoreManagerModule.cpp
* \brief Songbird Mediacore Manager Component Factory and Main Entry Point.
*/

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <mozilla/ModuleUtils.h>

#include "sbMediacoreManager.h"
#include "sbMediacoreTypeSniffer.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediacoreManager);
NS_DEFINE_NAMED_CID(SB_MEDIACOREMANAGER_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMediacoreTypeSniffer, Init);
NS_DEFINE_NAMED_CID(SB_MEDIACORETYPESNIFFER_CID);

static const mozilla::Module::CIDEntry kSongbirdMediacoreManagerCIDs[] = {
  { &kSB_MEDIACOREMANAGER_CID, false, NULL, sbMediacoreManagerConstructor },
  { &kSB_MEDIACORETYPESNIFFER_CID, false, NULL, sbMediacoreTypeSnifferConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kSongbirdMediacoreManagerContracts[] = {
  { SB_MEDIACOREMANAGER_CONTRACTID, &kSB_MEDIACOREMANAGER_CID },
  { SB_MEDIACORETYPESNIFFER_CONTRACTID, &kSB_MEDIACORETYPESNIFFER_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kSongbirdMediacoreManagerCategoroes[] = {
  { "app-startup", SB_MEDIACOREMANAGER_CLASSNAME, SB_MEDIACOREMANAGER_CONTRACTID },
  // { "app-startup", SB_MEDIACORETYPESNIFFER_CLASSNAME, SB_MEDIACORETYPESNIFFER_CLASSNAME },
  { NULL }
};

static const mozilla::Module kSongbirdMediacoreManagerModule = {
  mozilla::Module::kVersion,
  kSongbirdMediacoreManagerCIDs,
  kSongbirdMediacoreManagerContracts,
  kSongbirdMediacoreManagerCategoroes
};


NSMODULE_DEFN(sbMediacoreManagerComponents) = &kSongbirdMediacoreManagerModule;
