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

#include "sbRemoteAPIService.h"
#include "sbRemotePlayer.h"
#include "sbRemotePlayerFactory.h"
#include "sbSecurityMixin.h"
#include "sbRemoteSecurityEvent.h"

#include <mozilla/ModuleUtils.h>
#include <nsIClassInfoImpl.h>

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbRemoteAPIService, Init);

NS_GENERIC_FACTORY_CONSTRUCTOR(sbRemotePlayerFactory);

NS_GENERIC_FACTORY_CONSTRUCTOR(sbSecurityMixin);
NS_DECL_CI_INTERFACE_GETTER(sbSecurityMixin);

NS_GENERIC_FACTORY_CONSTRUCTOR(sbRemoteSecurityEvent);
NS_DECL_CI_INTERFACE_GETTER(sbRemoteSecurityEvent);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbRemotePlayer, Init);
NS_DECL_CI_INTERFACE_GETTER(sbRemotePlayer);

NS_DEFINE_NAMED_CID(SONGBIRD_REMOTEAPI_SERVICE_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_REMOTEPLAYERFACTORY_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_REMOTEPLAYER_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_SECURITYMIXIN_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_SECURITYEVENT_CID);


static const mozilla::Module::CIDEntry kRemoteAPICIDs[] = {
  { &kSONGBIRD_REMOTEAPI_SERVICE_CID, false, NULL, sbRemoteAPIServiceConstructor },
  { &kSONGBIRD_REMOTEPLAYERFACTORY_CID, false, NULL, sbRemotePlayerFactoryConstructor },
  { &kSONGBIRD_REMOTEPLAYER_CID, false, NULL, sbRemotePlayerConstructor },
  { &kSONGBIRD_SECURITYMIXIN_CID, false, NULL, sbSecurityMixinConstructor },
  { &kSONGBIRD_SECURITYEVENT_CID, false, NULL, sbRemoteSecurityEventConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kRemoteAPIContracts[] = {
  { SONGBIRD_REMOTEAPI_SERVICE_CONTRACTID, &kSONGBIRD_REMOTEAPI_SERVICE_CID },
  { SONGBIRD_REMOTEPLAYERFACTORY_CONTRACTID, &kSONGBIRD_REMOTEPLAYERFACTORY_CID },
  { SONGBIRD_REMOTEPLAYER_CONTRACTID, &kSONGBIRD_REMOTEPLAYER_CID },
  { SONGBIRD_SECURITYMIXIN_CONTRACTID, &kSONGBIRD_SECURITYMIXIN_CID },
  { SONGBIRD_SECURITYEVENT_CONTRACTID, &kSONGBIRD_SECURITYEVENT_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kRemoteAPICategories[] = {
  // { "profile-after-change", SONGBIRD_REMOTEAPI_SERVICE_CLASSNAME, SONGBIRD_REMOTEAPI_SERVICE_CONTRACTID },
  { NULL }
};

static const mozilla::Module kRemoteAPIModule = {
  mozilla::Module::kVersion,
  kRemoteAPICIDs,
  kRemoteAPIContracts,
  kRemoteAPICategories
};

NSMODULE_DEFN(sbRemoteAPIComponent) = &kRemoteAPIModule;
