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

#include "nsIGenericFactory.h"
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>
#include <nsServiceManagerUtils.h>

#include "sbStringTransform.h"
#include <sbIStringTransform.h>

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbStringTransform, Init);

/**
 * Register the Songbird string transform service component.
 */

static NS_METHOD
sbStringTransformRegister(nsIComponentManager*         aCompMgr,
                          nsIFile*                     aPath,
                          const char*                  aLoaderStr,
                          const char*                  aType,
                          const nsModuleComponentInfo* aInfo)
{
  nsresult rv;

  // Get the category manager.
  nsCOMPtr<nsICategoryManager> categoryManager =
                                 do_GetService(NS_CATEGORYMANAGER_CONTRACTID,
                                               &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add self to the app-startup category.
  rv = categoryManager->AddCategoryEntry
                          ("app-startup",
                           SB_STRINGTRANSFORM_CLASSNAME,
                           "service," SB_STRINGTRANSFORM_CONTRACTID,
                           PR_TRUE,
                           PR_TRUE,
                           nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Unregister the Songbird string transform service component.
 */

static NS_METHOD
sbStringTransformUnregister(nsIComponentManager*         aCompMgr,
                            nsIFile*                     aPath,
                            const char*                  aLoaderStr,
                            const nsModuleComponentInfo* aInfo)
{
  nsresult rv;

  // Get the category manager.
  nsCOMPtr<nsICategoryManager> categoryManager =
                                 do_GetService(NS_CATEGORYMANAGER_CONTRACTID,
                                               &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Delete self from the app-startup category.
  rv = categoryManager->DeleteCategoryEntry("app-startup",
                                            SB_STRINGTRANSFORM_CLASSNAME,
                                            PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


static const nsModuleComponentInfo components[] =
{
	{
    SB_STRINGTRANSFORM_DESCRIPTION,
    SB_STRINGTRANSFORM_CID,
    SB_STRINGTRANSFORM_CONTRACTID,
    sbStringTransformConstructor,
    sbStringTransformRegister,
    sbStringTransformUnregister
	},
};

NS_IMPL_NSGETMODULE(SongbirdIntlModule, components)
