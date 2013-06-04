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

/**
 * \file sbPlayQueueServiceComponent.cpp
 * \brief Songbird Play Queue Service component factory.
 */

#include "sbPlayQueueService.h"

#include <mozilla/ModuleUtils.h>

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbPlayQueueService, Init);

NS_DEFINE_NAMED_CID(SB_PLAYQUEUESERVICE_CID);

static const mozilla::Module::CIDEntry kPlayQueueServiceCIDs[] = {
  { &kSB_PLAYQUEUESERVICE_CID, false, NULL, sbPlayQueueServiceConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kPlayQueueServiceContracts[] = {
  { SB_PLAYQUEUESERVICE_CONTRACTID, &kSB_PLAYQUEUESERVICE_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kPlayQueueServiceCategories[] = {
  { "app-startup", SB_PLAYQUEUESERVICE_CONTRACTID },
  { NULL }
};

static const mozilla::Module kPlayQueueServiceModule = {
  mozilla::Module::kVersion,
  kPlayQueueServiceCIDs,
  kPlayQueueServiceContracts,
  kPlayQueueServiceCategories
};

NSMODULE_DEFN(sbPlayQueueServiceComponent) = &kPlayQueueServiceModule;
