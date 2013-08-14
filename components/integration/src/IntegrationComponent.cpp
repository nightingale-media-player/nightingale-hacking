/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
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
 *=END SONGBIRD GPL
 */

/** 
* \file  IntegrationComponent.cpp
* \brief Songbird MediaLibrary Component Factory and Main Entry Point.
*/

#include "mozilla/ModuleUtils.h"

#include "WindowCloak.h"

#include "sbNativeWindowManagerCID.h"

#ifdef XP_MACOSX
#include "macosx/sbNativeWindowManager.h"
#include "macosx/sbMacAppDelegate.h"
#include "macosx/sbMacWindowMoveService.h"
#include "macosx/sbMacWindowTitlebarService.h"
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
  "@songbirdnest.com/integration/window-move-resize-service;1"
#define SB_WINDOWMOVE_SERVICE_CLASSNAME \
  "Songbird Window Move/Resize Service"
#define SB_WINDOWMOVE_SERVICE_CID \
  {0x4f8fecc6, 0x1dd2, 0x11b2, {0x90, 0x3a, 0xf3, 0x47, 0x1b, 0xfd, 0x3a, 0x60}}

NS_GENERIC_FACTORY_CONSTRUCTOR(sbWindowCloak);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbNativeWindowManager);
NS_DEFINE_NAMED_CID(SONGBIRD_WINDOWCLOAK_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_NATIVEWINDOWMANAGER_CID);

#ifdef XP_WIN
NS_GENERIC_FACTORY_CONSTRUCTOR(CWindowMinMax);
NS_GENERIC_FACTORY_CONSTRUCTOR(CWindowResizeHook);
NS_GENERIC_FACTORY_CONSTRUCTOR(CWindowRegion);
NS_GENERIC_FACTORY_CONSTRUCTOR(CGlobalHotkeys);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbWindowChromeService);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbKnownFolderManager, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbScreenSaverSuppressor, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbWindowMoveService, Init);
NS_DEFINE_NAMED_CID(SONGBIRD_KNOWN_FOLDER_MANAGER_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_WINDOW_CHROME_SERVICE_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_WINDOWMINMAX_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_WINDOWRESIZEHOOK_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_WINDOWREGION_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_GLOBALHOTKEYS_CID);
NS_DEFINE_NAMED_CID(SB_BASE_SCREEN_SAVER_SUPPRESSOR_CID);
NS_DEFINE_NAMED_CID(SB_WINDOWMOVE_SERVICE_CID);
#endif

#ifdef XP_MACOSX
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMacAppDelegateManager, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMacWindowMoveService, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMacWindowTitlebarService, Initialize);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbScreenSaverSuppressor, Init);
#endif

#ifdef MOZ_WIDGET_GTK2
NS_GENERIC_FACTORY_CONSTRUCTOR(sbGtkWindowMoveService);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbScreenSaverSuppressor, Init);
NS_DEFINE_NAMED_CID(SB_WINDOWMOVE_SERVICE_CID);
NS_DEFINE_NAMED_CID(SB_BASE_SCREEN_SAVER_SUPPRESSOR_CID);
#endif


#ifdef XP_WIN
  static const mozilla::Module::CIDEntry kIntegrationCIDs[] = {
    { &kSONGBIRD_WINDOWCLOAK_CID, false, NULL, sbWindowCloakConstructor },
    { &kSONGBIRD_NATIVEWINDOWMANAGER_CID, false, NULL, sbNativeWindowManagerConstructor },
    { &kSONGBIRD_KNOWN_FOLDER_MANAGER_CID, false, NULL, sbKnownFolderManagerConstructor },
    { &kSONGBIRD_WINDOW_CHROME_SERVICE_CID, false, NULL, sbWindowChromeServiceConstructor },
    { &kSONGBIRD_WINDOWMINMAX_CID, false, NULL, CWindowMinMaxConstructor },
    { &kSONGBIRD_WINDOWRESIZEHOOK_CID, false, NULL, CWindowResizeHookConstructor },
    { &kSONGBIRD_WINDOWREGION_CID, false, NULL, CWindowRegionConstructor },
    { &kSONGBIRD_GLOBALHOTKEYS_CID, false, NULL, CGlobalHotkeysConstructor },
    { &kSB_BASE_SCREEN_SAVER_SUPPRESSOR_CID, false, NULL, sbScreenSaverSuppressorConstructor },
    { &kSB_WINDOWMOVE_SERVICE_CID, false, NULL, sbWindowMoveServiceConstructor },
    { NULL }
  };

  static const mozilla::Module::ContractIDEntry kIntegrationContracts[] = {
    { SONGBIRD_WINDOWCLOAK_CONTRACTID, &kSONGBIRD_WINDOWCLOAK_CID },
    { SONGBIRD_NATIVEWINDOWMANAGER_CONTRACTID, &kSONGBIRD_NATIVEWINDOWMANAGER_CID },
    { SONGBIRD_KNOWN_FOLDER_MANAGER_CONTRACTID, &kSONGBIRD_KNOWN_FOLDER_MANAGER_CID },
    { SONGBIRD_WINDOW_CHROME_SERVICE_CONTRACTID, &kSONGBIRD_WINDOW_CHROME_SERVICE_CID },
    { SONGBIRD_WINDOWMINMAX_CONTRACTID, &kSONGBIRD_WINDOWMINMAX_CID },
    { SONGBIRD_WINDOWRESIZEHOOK_CONTRACTID, &kSONGBIRD_WINDOWRESIZEHOOK_CID },
    { SONGBIRD_WINDOWREGION_CONTRACTID, &kSONGBIRD_WINDOWREGION_CID },
    { SONGBIRD_GLOBALHOTKEYS_CONTRACTID, &kSONGBIRD_GLOBALHOTKEYS_CID },
    { SB_BASE_SCREEN_SAVER_SUPPRESSOR_CONTRACTID, &kSB_BASE_SCREEN_SAVER_SUPPRESSOR_CID },
    { SB_WINDOWMOVE_SERVICE_CONTRACTID, &kSB_WINDOWMOVE_SERVICE_CID },
    { NULL }
  };

  static const mozilla::Module::CategoryEntry kIntegrationCategories[] = {
    { NULL }
  };
#endif
#ifdef XP_MACOSX
  static const mozilla::Module::CIDEntry kIntegrationCIDs[] = {
    { &kSONGBIRD_WINDOWCLOAK_CID, false, NULL, sbWindowCloakConstructor },
    { &kSONGBIRD_NATIVEWINDOWMANAGER_CID, false, NULL, sbNativeWindowManagerConstructor },
    { &kSONGBIRD_MACAPPDELEGATEMANAGER_CID, false, NULL, sbMacAppDelegateManagerConstructor },
    { &kSB_MAC_WINDOW_TITLEBAR_SERVICE_CID, false, NULL, sbMacWindowTitlebarServiceConstructor },
    { &kSB_BASE_SCREEN_SAVER_SUPPRESSOR_CID, false, NULL, sbScreenSaverSuppressorConstructor },
    { &kSB_WINDOWMOVE_SERVICE_CID, false, NULL, sbMacWindowMoveServiceConstructor },
    { NULL }
  };
  static const mozilla::Module::ContractIDEntry kIntegrationContracts[] = {
    { SONGBIRD_WINDOWCLOAK_CONTRACTID, &kSONGBIRD_WINDOWCLOAK_CID },
    { SONGBIRD_NATIVEWINDOWMANAGER_CONTRACTID, &kSONGBIRD_NATIVEWINDOWMANAGER_CID },
    { SONGBIRD_MACAPPDELEGATEMANAGER_CONTRACTID, &kSONGBIRD_MACAPPDELEGATEMANAGER_CID },
    { SB_MAC_WINDOW_TITLEBAR_SERVICE_CONTRACTID, &kSB_MAC_WINDOW_TITLEBAR_SERVICE_CID },
    { SB_BASE_SCREEN_SAVER_SUPPRESSOR_CONTRACTID, &kSB_BASE_SCREEN_SAVER_SUPPRESSOR_CID },
    { SB_WINDOWMOVE_SERVICE_CONTRACTID, &kSB_WINDOWMOVE_SERVICE_CID },
    { NULL }
  };

  static const mozilla::Module::CategoryEntry kIntegrationCategories[] = {
    { NULL }
  };
#endif
#ifdef MOZ_WIDGET_GTK2
  static const mozilla::Module::CIDEntry kIntegrationCIDs[] = {
    { &kSONGBIRD_WINDOWCLOAK_CID, false, NULL, sbWindowCloakConstructor },
    { &kSONGBIRD_NATIVEWINDOWMANAGER_CID, false, NULL, sbNativeWindowManagerConstructor },
    { &kSB_BASE_SCREEN_SAVER_SUPPRESSOR_CID, true, NULL, sbScreenSaverSuppressorConstructor },
    { &kSB_WINDOWMOVE_SERVICE_CID, false, NULL, sbGtkWindowMoveServiceConstructor },
    { NULL }
  };

  static const mozilla::Module::ContractIDEntry kIntegrationContracts[] = {
    { SONGBIRD_WINDOWCLOAK_CONTRACTID, &kSONGBIRD_WINDOWCLOAK_CID },
    { SONGBIRD_NATIVEWINDOWMANAGER_CONTRACTID, &kSONGBIRD_NATIVEWINDOWMANAGER_CID },
    { SB_BASE_SCREEN_SAVER_SUPPRESSOR_CONTRACTID, &kSB_BASE_SCREEN_SAVER_SUPPRESSOR_CID },
    { SB_WINDOWMOVE_SERVICE_CONTRACTID, &kSB_WINDOWMOVE_SERVICE_CID },
    { NULL }
  };

  static const mozilla::Module::CategoryEntry kIntegrationCategories[] = {
    { NULL }
  };
#endif

static const mozilla::Module kIntegrationModule = {
  mozilla::Module::kVersion,
  kIntegrationCIDs,
  kIntegrationContracts,
  kIntegrationCategories
};

NSMODULE_DEFN(SongbirdIntegrationComponent) = &kIntegrationModule;
