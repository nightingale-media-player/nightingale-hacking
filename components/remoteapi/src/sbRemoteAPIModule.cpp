/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
 */

#include "sbRemoteAPIService.h"
#include "sbRemotePlayer.h"
#include "sbRemotePlayerFactory.h"
#include "sbSecurityMixin.h"
#include "sbRemoteSecurityEvent.h"

#include <nsIClassInfoImpl.h>

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbRemoteAPIService, Init)

NS_GENERIC_FACTORY_CONSTRUCTOR(sbRemotePlayerFactory)

NS_GENERIC_FACTORY_CONSTRUCTOR(sbSecurityMixin)
NS_DECL_CI_INTERFACE_GETTER(sbSecurityMixin)

NS_GENERIC_FACTORY_CONSTRUCTOR(sbRemoteSecurityEvent)
NS_DECL_CI_INTERFACE_GETTER(sbRemoteSecurityEvent)

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbRemotePlayer, Init)
NS_DECL_CI_INTERFACE_GETTER(sbRemotePlayer)

// fill out data struct to register with component system
static const nsModuleComponentInfo components[] =
{
  {
    NIGHTINGALE_REMOTEAPI_SERVICE_CLASSNAME,
    NIGHTINGALE_REMOTEAPI_SERVICE_CID,
    NIGHTINGALE_REMOTEAPI_SERVICE_CONTRACTID,
    sbRemoteAPIServiceConstructor
  },
  {
    NIGHTINGALE_REMOTEPLAYERFACTORY_CLASSNAME,
    NIGHTINGALE_REMOTEPLAYERFACTORY_CID,
    NIGHTINGALE_REMOTEPLAYERFACTORY_CONTRACTID,
    sbRemotePlayerFactoryConstructor
  },
  {
    NIGHTINGALE_REMOTEPLAYER_CLASSNAME,
    NIGHTINGALE_REMOTEPLAYER_CID,
    NIGHTINGALE_REMOTEPLAYER_CONTRACTID,
    sbRemotePlayerConstructor,
    sbRemotePlayer::Register,
    sbRemotePlayer::Unregister,
    NULL,
    NS_CI_INTERFACE_GETTER_NAME(sbRemotePlayer)
  },
  {
    NIGHTINGALE_SECURITYMIXIN_CLASSNAME,
    NIGHTINGALE_SECURITYMIXIN_CID,
    NIGHTINGALE_SECURITYMIXIN_CONTRACTID,
    sbSecurityMixinConstructor,
    NULL,
    NULL,
    NULL,
    NS_CI_INTERFACE_GETTER_NAME(sbSecurityMixin)
  },
  {
    NIGHTINGALE_SECURITYEVENT_CLASSNAME,
    NIGHTINGALE_SECURITYEVENT_CID,
    NIGHTINGALE_SECURITYEVENT_CONTRACTID,
    sbRemoteSecurityEventConstructor,
    NULL,
    NULL,
    NULL,
    NS_CI_INTERFACE_GETTER_NAME(sbRemoteSecurityEvent)
  }
};

// create the module info struct that is used to regsiter
NS_IMPL_NSGETMODULE(NightingaleRemoteAPIModule, components)

