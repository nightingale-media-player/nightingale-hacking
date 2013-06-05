/*
 * BEGIN NIGHTINGALE GPL
 * 
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2013
 * http://getnightingale.com
 * 
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 * 
 * Software distributed under the License is distributed 
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 * express or implied. See the GPL for the specific language 
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this 
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc., 
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * 
 * END NIGHTINGALE GPL
 */

#include <mozilla/ModuleUtils.h>

#include "sbMediaSniffer.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaSniffer);
NS_DEFINE_NAMED_CID(SONGBIRD_MEDIASNIFFER_CID);

static const mozilla::Module::CIDEntry kMediaSnifferCIDs[] = {
  { &kSONGBIRD_MEDIASNIFFER_CID, false, NULL, sbMediaSnifferConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kMediaSnifferContracts[] = {
  { SONGBIRD_MEDIASNIFFER_CONTRACTID, &kSONGBIRD_MEDIASNIFFER_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kMediaSnifferCategories[] = {
  { NULL }
};

static const mozilla::Module kMediaSnifferModule = {
  mozilla::Module::kVersion,
  kMediaSnifferCIDs,
  kMediaSnifferContracts,
  kMediaSnifferCategories
};

NSMODULE_DEFN(sbMediaSniffer) = &kMediaSnifferModule;
