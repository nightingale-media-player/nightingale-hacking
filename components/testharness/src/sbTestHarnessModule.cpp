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

#include <mozilla/ModuleUtils.h>

#include "sbLeakCanary.h"
#include "sbTestHarnessConsoleListener.h"
#include "sbTestHarnessCID.h"
#include "sbTimingService.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbTestHarnessConsoleListener);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbTimingService, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLeakCanary);

NS_DEFINE_NAMED_CID(SB_TESTHARNESSCONSOLELISTENER_CID);
NS_DEFINE_NAMED_CID(SB_TIMINGSERVICE_CID);
NS_DEFINE_NAMED_CID(SB_LEAKCANARY_CID);


static const mozilla::Module::CIDEntry kTestHarnessConsoleListenerCIDs[] = {
  { &kSB_TESTHARNESSCONSOLELISTENER_CID, false, NULL, sbTestHarnessConsoleListenerConstructor },
  { &kSB_TIMINGSERVICE_CID, true, NULL, sbTimingServiceConstructor },
  { &kSB_LEAKCANARY_CID, false, NULL, sbLeakCanaryConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kTestHarnessConsoleListenerContracts[] = {
  { SB_TESTHARNESSCONSOLELISTENER_CONTRACTID, &kSB_TESTHARNESSCONSOLELISTENER_CID },
  { SB_TIMINGSERVICE_CONTRACTID, &kSB_TIMINGSERVICE_CID },
  { SB_LEAKCANARY_CONTRACTID, &kSB_LEAKCANARY_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kTestHarnessConsoleListenerCategories[] = {
  { "profile-before-change", SB_TIMINGSERVICE_CLASSNAME, SB_TIMINGSERVICE_CONTRACTID },
  { NULL }
};

static const mozilla::Module kTestHarnessConsoleListenerModule = {
  mozilla::Module::kVersion,
  kTestHarnessConsoleListenerCIDs,
  kTestHarnessConsoleListenerContracts,
  kTestHarnessConsoleListenerCategories
};

NSMODULE_DEFN(sbTestHarnessConsoleListenerModule) = &kTestHarnessConsoleListenerModule;
