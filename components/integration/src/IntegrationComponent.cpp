/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END NIGHTINGALE GPL
 */

/** 
* \file  IntegrationComponent.cpp
* \brief Nightingale MediaLibrary Component Factory and Main Entry Point.
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
#include "win32/sbKnownFolderManager.h"
#include "win32/sbNativeWindowManager.h"
#include "win32/sbScreenSaverSuppressor.h"
#include "win32/sbWindowMoveService.h"
#include "GlobalHotkeys.h"
#include "WindowMinMax.h"
#include "WindowResizeHook.h"
#include "WindowRegion.h"
#include "sbWindowChromeService.h"
#endif


#define SB_WINDOWMOVE_SERVICE_CONTRACTID \
  "@getnightingale.com/integration/window-move-resize-service;1"
#define SB_WINDOWMOVE_SERVICE_CLASSNAME \
  "Nightingale Window Move/Resize Service"
#define SB_WINDOWMOVE_SERVICE_CID \
  {0x4f8fecc6, 0x1dd2, 0x11b2, {0x90, 0x3a, 0xf3, 0x47, 0x1b, 0xfd, 0x3a, 0x60}}



NS_GENERIC_FACTORY_CONSTRUCTOR(sbWindowCloak)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbNativeWindowManager)

#ifdef XP_WIN
NS_GENERIC_FACTORY_CONSTRUCTOR(CWindowMinMax)
NS_GENERIC_FACTORY_CONSTRUCTOR(CWindowResizeHook)
NS_GENERIC_FACTORY_CONSTRUCTOR(CWindowRegion)
NS_GENERIC_FACTORY_CONSTRUCTOR(CGlobalHotkeys)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbWindowChromeService)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbKnownFolderManager, Init);
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
    NIGHTINGALE_WINDOWCLOAK_CLASSNAME,
    NIGHTINGALE_WINDOWCLOAK_CID,
    NIGHTINGALE_WINDOWCLOAK_CONTRACTID,
    sbWindowCloakConstructor
  },

  {
    NIGHTINGALE_NATIVEWINDOWMANAGER_CLASSNAME,
    NIGHTINGALE_NATIVEWINDOWMANAGER_CID,
    NIGHTINGALE_NATIVEWINDOWMANAGER_CONTRACTID,
    sbNativeWindowManagerConstructor
  },

#ifdef XP_WIN
  {
    NIGHTINGALE_KNOWN_FOLDER_MANAGER_CLASSNAME,
    NIGHTINGALE_KNOWN_FOLDER_MANAGER_CID,
    NIGHTINGALE_KNOWN_FOLDER_MANAGER_CONTRACTID,
    sbKnownFolderManagerConstructor
  },

  {
    NIGHTINGALE_WINDOW_CHROME_SERVICE_CLASSNAME,
    NIGHTINGALE_WINDOW_CHROME_SERVICE_CID,
    NIGHTINGALE_WINDOW_CHROME_SERVICE_CONTRACTID,
    sbWindowChromeServiceConstructor
  },

  {
    NIGHTINGALE_WINDOWMINMAX_CLASSNAME,
    NIGHTINGALE_WINDOWMINMAX_CID,
    NIGHTINGALE_WINDOWMINMAX_CONTRACTID,
    CWindowMinMaxConstructor
  },

  {
    NIGHTINGALE_WINDOWRESIZEHOOK_CLASSNAME,
    NIGHTINGALE_WINDOWRESIZEHOOK_CID,
    NIGHTINGALE_WINDOWRESIZEHOOK_CONTRACTID,
    CWindowResizeHookConstructor
  },

  {
    NIGHTINGALE_WINDOWREGION_CLASSNAME,
    NIGHTINGALE_WINDOWREGION_CID,
    NIGHTINGALE_WINDOWREGION_CONTRACTID,
    CWindowRegionConstructor
  },

  {
    NIGHTINGALE_GLOBALHOTKEYS_CLASSNAME,
    NIGHTINGALE_GLOBALHOTKEYS_CID,
    NIGHTINGALE_GLOBALHOTKEYS_CONTRACTID,
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
    NIGHTINGALE_MACAPPDELEGATEMANAGER_CLASSNAME,
    NIGHTINGALE_MACAPPDELEGATEMANAGER_CID,
    NIGHTINGALE_MACAPPDELEGATEMANAGER_CONTRACTID,
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

NS_IMPL_NSGETMODULE(NightingaleIntegrationComponent, sbIntegration)
