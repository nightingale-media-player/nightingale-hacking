/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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
* \file  IntegrationComponent.cpp
* \brief Songbird MediaLibrary Component Factory and Main Entry Point.
*/

#include "nsIGenericFactory.h"

#include "WindowDragger.h"
#include "WindowCloak.h"

#ifdef XP_WIN
#include "WindowMinMax.h"
#include "WindowResizeHook.h"
#include "WindowRegion.h"
#include "GlobalHotkeys.h"
#endif


NS_GENERIC_FACTORY_CONSTRUCTOR(CWindowDragger)
NS_GENERIC_FACTORY_CONSTRUCTOR(CWindowCloak)

#ifdef XP_WIN
NS_GENERIC_FACTORY_CONSTRUCTOR(CWindowMinMax)
NS_GENERIC_FACTORY_CONSTRUCTOR(CWindowResizeHook)
NS_GENERIC_FACTORY_CONSTRUCTOR(CWindowRegion)
NS_GENERIC_FACTORY_CONSTRUCTOR(CGlobalHotkeys)
#endif

static nsModuleComponentInfo sbIntegration[] =
{
  {
    SONGBIRD_WINDOWDRAGGER_CLASSNAME,
    SONGBIRD_WINDOWDRAGGER_CID,
    SONGBIRD_WINDOWDRAGGER_CONTRACTID,
    CWindowDraggerConstructor
  },

  {
    SONGBIRD_WINDOWCLOAK_CLASSNAME,
    SONGBIRD_WINDOWCLOAK_CID,
    SONGBIRD_WINDOWCLOAK_CONTRACTID,
    CWindowCloakConstructor
  },

#ifdef XP_WIN
  {
    SONGBIRD_WINDOWMINMAX_CLASSNAME,
    SONGBIRD_WINDOWMINMAX_CID,
    SONGBIRD_WINDOWMINMAX_CONTRACTID,
    CWindowMinMaxConstructor
  },

  {
    SONGBIRD_WINDOWRESIZEHOOK_CLASSNAME,
    SONGBIRD_WINDOWRESIZEHOOK_CID,
    SONGBIRD_WINDOWRESIZEHOOK_CONTRACTID,
    CWindowResizeHookConstructor
  },

  {
    SONGBIRD_WINDOWREGION_CLASSNAME,
    SONGBIRD_WINDOWREGION_CID,
    SONGBIRD_WINDOWREGION_CONTRACTID,
    CWindowRegionConstructor
  },

  {
    SONGBIRD_GLOBALHOTKEYS_CLASSNAME,
    SONGBIRD_GLOBALHOTKEYS_CID,
    SONGBIRD_GLOBALHOTKEYS_CONTRACTID,
    CGlobalHotkeysConstructor
  },
#endif
};

NS_IMPL_NSGETMODULE("SongbirdIntegrationComponent", sbIntegration)
