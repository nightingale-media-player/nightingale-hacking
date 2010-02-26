/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2010 POTI, Inc.
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
* \file  IntegrationComponent.cpp
* \brief Songbird MediaLibrary Component Factory and Main Entry Point.
*/

#include "nsIGenericFactory.h"

#include "WindowCloak.h"

#include "sbNativeWindowManagerCID.h"

#ifdef XP_MACOSX
#include "macosx/sbNativeWindowManager.h"
#include "macosx/sbMacAppDelegate.h"
#include "macosx/sbMacWindowMoveService.h"
#include "macosx/sbScreenSaverSuppressor.h"
#endif

#ifdef MOZ_WIDGET_GTK2 
#include "linux/sbNativeWindowManager.h"
#include "linux/sbGtkWindowMoveService.h"
#include "linux/sbScreenSaverSuppressor.h"
#endif

#ifdef XP_WIN
#include "win32/sbNativeWindowManager.h"
#include "win32/sbScreenSaverSuppressor.h"
#include "win32/sbWindowMoveService.h"
#include "GlobalHotkeys.h"
#include "WindowMinMax.h"
#include "WindowResizeHook.h"
#include "WindowRegion.h"
#endif


#define SB_WINDOWMOVE_SERVICE_CONTRACTID \
  "@songbirdnest.com/integration/window-move-resize-service;1"
#define SB_WINDOWMOVE_SERVICE_CLASSNAME \
  "Songbird Window Move/Resize Service"
#define SB_WINDOWMOVE_SERVICE_CID \
  {0x4f8fecc6, 0x1dd2, 0x11b2, {0x90, 0x3a, 0xf3, 0x47, 0x1b, 0xfd, 0x3a, 0x60}}



NS_GENERIC_FACTORY_CONSTRUCTOR(sbWindowCloak)

NS_GENERIC_FACTORY_CONSTRUCTOR(sbNativeWindowManager)

#ifdef XP_WIN
NS_GENERIC_FACTORY_CONSTRUCTOR(CWindowMinMax)
NS_GENERIC_FACTORY_CONSTRUCTOR(CWindowResizeHook)
NS_GENERIC_FACTORY_CONSTRUCTOR(CWindowRegion)
NS_GENERIC_FACTORY_CONSTRUCTOR(CGlobalHotkeys)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbScreenSaverSuppressor, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbWindowMoveService, Init);
#endif

#ifdef XP_MACOSX
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMacAppDelegateManager, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMacWindowMoveService, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbScreenSaverSuppressor, Init);
#endif

#ifdef MOZ_WIDGET_GTK2
NS_GENERIC_FACTORY_CONSTRUCTOR(sbGtkWindowMoveService)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbScreenSaverSuppressor, Init);
#endif

static nsModuleComponentInfo sbIntegration[] =
{  
  {
    SONGBIRD_WINDOWCLOAK_CLASSNAME,
    SONGBIRD_WINDOWCLOAK_CID,
    SONGBIRD_WINDOWCLOAK_CONTRACTID,
    sbWindowCloakConstructor
  },

  {
    SONGBIRD_NATIVEWINDOWMANAGER_CLASSNAME,
    SONGBIRD_NATIVEWINDOWMANAGER_CID,
    SONGBIRD_NATIVEWINDOWMANAGER_CONTRACTID,
    sbNativeWindowManagerConstructor
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

  {
    SB_BASE_SCREEN_SAVER_SUPPRESSOR_CLASSNAME,
    SB_BASE_SCREEN_SAVER_SUPPRESSOR_CID,
    SB_BASE_SCREEN_SAVER_SUPPRESSOR_CONTRACTID,
    sbScreenSaverSuppressorConstructor,
    sbScreenSaverSuppressor::RegisterSelf
  },

  {
    SB_WINDOWMOVE_SERVICE_CLASSNAME,
    SB_WINDOWMOVE_SERVICE_CID,
    SB_WINDOWMOVE_SERVICE_CONTRACTID,
    sbWindowMoveServiceConstructor
  },
#endif
#ifdef XP_MACOSX
  {
    SONGBIRD_MACAPPDELEGATEMANAGER_CLASSNAME,
    SONGBIRD_MACAPPDELEGATEMANAGER_CID,
    SONGBIRD_MACAPPDELEGATEMANAGER_CONTRACTID,
    sbMacAppDelegateManagerConstructor,
    sbMacAppDelegateManager::RegisterSelf
  },
  {
    SB_WINDOWMOVE_SERVICE_CLASSNAME,
    SB_WINDOWMOVE_SERVICE_CID,
    SB_WINDOWMOVE_SERVICE_CONTRACTID,
    sbMacWindowMoveServiceConstructor
  },
  {
    SB_BASE_SCREEN_SAVER_SUPPRESSOR_CLASSNAME,
    SB_BASE_SCREEN_SAVER_SUPPRESSOR_CID,
    SB_BASE_SCREEN_SAVER_SUPPRESSOR_CONTRACTID,
    sbScreenSaverSuppressorConstructor,
    sbScreenSaverSuppressor::RegisterSelf
  },
#endif
#ifdef MOZ_WIDGET_GTK2
  {
    SB_WINDOWMOVE_SERVICE_CLASSNAME,
    SB_WINDOWMOVE_SERVICE_CID,
    SB_WINDOWMOVE_SERVICE_CONTRACTID,
    sbGtkWindowMoveServiceConstructor
  },
  {
    SB_BASE_SCREEN_SAVER_SUPPRESSOR_CLASSNAME,
    SB_BASE_SCREEN_SAVER_SUPPRESSOR_CID,
    SB_BASE_SCREEN_SAVER_SUPPRESSOR_CONTRACTID,
    sbScreenSaverSuppressorConstructor,
    sbScreenSaverSuppressor::RegisterSelf
  },
#endif
};

NS_IMPL_NSGETMODULE(SongbirdIntegrationComponent, sbIntegration)
