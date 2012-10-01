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
* \file  sbDeviceFirmwareModule.cpp
* \brief Songbird Device Firmware Updater Component Factory and Main Entry Point.
*/

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <mozilla/ModuleUtils.h>

#include "sbDeviceFirmwareSupport.h"
#include "sbDeviceFirmwareUpdate.h"
#include "sbDeviceFirmwareUpdater.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceFirmwareSupport);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceFirmwareUpdate);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbDeviceFirmwareUpdater, Init);

static NS_METHOD
sbDeviceFirmwareUpdaterRegisterSelf(nsIComponentManager* aCompMgr,
                                    nsIFile* aPath,
                                    const char* registryLocation,
                                    const char* componentType,
                                    const nsModuleComponentInfo* info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = categoryManager->
         AddCategoryEntry(APPSTARTUP_CATEGORY,
                          SB_DEVICEFIRMWAREUPDATER_DESCRIPTION,
                          "service," SB_DEVICEFIRMWAREUPDATER_CONTRACTID,
                          PR_TRUE, PR_TRUE, nsnull);
  return rv;
}

static NS_METHOD
sbDeviceFirmwareUpdaterUnregisterSelf(nsIComponentManager* aCompMgr,
                                      nsIFile* aPath,
                                      const char* registryLocation,
                                      const nsModuleComponentInfo* info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = categoryManager->DeleteCategoryEntry(APPSTARTUP_CATEGORY,
                                            SB_DEVICEFIRMWAREUPDATER_DESCRIPTION,
                                            PR_TRUE);

  return rv;
}

static nsModuleComponentInfo sbDeviceFirmwareUpdaterComponents[] =
{
  {
    SB_DEVICEFIRMWARESUPPORT_CLASSNAME,
    SB_DEVICEFIRMWARESUPPORT_CID,
    SB_DEVICEFIRMWARESUPPORT_CONTRACTID,
    sbDeviceFirmwareSupportConstructor
  },

  {
    SB_DEVICEFIRMWAREUPDATE_CLASSNAME,
    SB_DEVICEFIRMWAREUPDATE_CID,
    SB_DEVICEFIRMWAREUPDATE_CONTRACTID,
    sbDeviceFirmwareUpdateConstructor
  },

  {
    SB_DEVICEFIRMWAREUPDATER_CLASSNAME,
    SB_DEVICEFIRMWAREUPDATER_CID,
    SB_DEVICEFIRMWAREUPDATER_CONTRACTID,
    sbDeviceFirmwareUpdaterConstructor,
    sbDeviceFirmwareUpdaterRegisterSelf,
    sbDeviceFirmwareUpdaterUnregisterSelf
  }
};

NS_IMPL_NSGETMODULE(SongbirdDeviceFirmwareUpdater, sbDeviceFirmwareUpdaterComponents)
