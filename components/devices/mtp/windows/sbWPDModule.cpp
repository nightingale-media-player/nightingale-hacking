/* vim: set sw=2 :miv */
/*
 //
 // BEGIN SONGBIRD GPL
 // 
 // This file is part of the Songbird web player.
 //
 // Copyright(c) 2005-2008 POTI, Inc.
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
 * \file  sbDeviceMarshallModule.cpp
 * \brief Songbird Device Marshall Component Factory and Main Entry Point.
 */

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>
#include "sbWPDMarshall.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbWPDMarshall);

// Registration functions for becoming a startup observer
static NS_METHOD sbDeviceMarshallRegisterSelf(nsIComponentManager* aCompMgr,
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
      AddCategoryEntry(SB_DEVICE_MARSHALL_CATEGORY,
                       SB_WPDMARSHALL_DESCRIPTION,
                       SB_WPDMARSHALL_CONTRACTID,
                       PR_TRUE, PR_TRUE, nsnull);

  return rv;
}

static NS_METHOD sbDeviceMarshallUnregisterSelf(nsIComponentManager* aCompMgr,
                                                nsIFile* aPath,
                                                const char* registryLocation,
                                                const nsModuleComponentInfo* info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager =
      do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = categoryManager->DeleteCategoryEntry(SB_DEVICE_MARSHALL_CATEGORY,
                                            SB_WPDMARSHALL_DESCRIPTION,
                                            PR_TRUE);
  return rv;
}

static nsModuleComponentInfo sbDeviceMarshallComponents[] = 
{
  {
    SB_WPDMARSHALL_CLASSNAME,
    SB_WPDMARSHALL_CID,
    SB_WPDMARSHALL_CONTRACTID,
    sbWPDMarshallConstructor,
    sbDeviceMarshallRegisterSelf,
    sbDeviceMarshallUnregisterSelf
  }
};

NS_IMPL_NSGETMODULE(SongbirdDeviceMarshall, sbDeviceMarshallComponents)
