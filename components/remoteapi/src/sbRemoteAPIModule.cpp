/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

#include "sbRemoteAPIService.h"
#include "sbRemotePlayer.h"
#include "sbSecurityMixin.h"
#include "sbRemoteSecurityEvent.h"

#include <nsIClassInfoImpl.h>

#define SONGBIRD_REMOTEAPI_MODULENAME "Songbird Remote API Module"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbRemoteAPIService, Init)

NS_GENERIC_FACTORY_CONSTRUCTOR(sbSecurityMixin)
NS_DECL_CI_INTERFACE_GETTER(sbSecurityMixin)

NS_GENERIC_FACTORY_CONSTRUCTOR(sbRemoteSecurityEvent)
NS_DECL_CI_INTERFACE_GETTER(sbRemoteSecurityEvent)

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(sbRemotePlayer, sbRemotePlayer::GetInstance)
NS_DECL_CI_INTERFACE_GETTER(sbRemotePlayer)

// fill out data struct to register with component system
static const nsModuleComponentInfo components[] =
{
  {
    SONGBIRD_REMOTEAPI_SERVICE_CLASSNAME,
    SONGBIRD_REMOTEAPI_SERVICE_CID,
    SONGBIRD_REMOTEAPI_SERVICE_CONTRACTID,
    sbRemoteAPIServiceConstructor
  },
  {
    SONGBIRD_REMOTEPLAYER_CLASSNAME,
    SONGBIRD_REMOTEPLAYER_CID,
    SONGBIRD_REMOTEPLAYER_CONTRACTID,
    sbRemotePlayerConstructor,
    sbRemotePlayer::Register,
    sbRemotePlayer::Unregister,
    NULL,
    NS_CI_INTERFACE_GETTER_NAME(sbRemotePlayer)
  },
  {
    SONGBIRD_SECURITYMIXIN_CLASSNAME,
    SONGBIRD_SECURITYMIXIN_CID,
    SONGBIRD_SECURITYMIXIN_CONTRACTID,
    sbSecurityMixinConstructor,
    NULL,
    NULL,
    NULL,
    NS_CI_INTERFACE_GETTER_NAME(sbSecurityMixin)
  },
  {
    SONGBIRD_SECURITYEVENT_CLASSNAME,
    SONGBIRD_SECURITYEVENT_CID,
    SONGBIRD_SECURITYEVENT_CONTRACTID,
    sbRemoteSecurityEventConstructor,
    NULL,
    NULL,
    NULL,
    NS_CI_INTERFACE_GETTER_NAME(sbRemoteSecurityEvent)
  }
};

// create the module info struct that is used to regsiter
NS_IMPL_NSGETMODULE(SONGBIRD_REMOTEAPI_MODULENAME, components)

