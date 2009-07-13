/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://www.songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
// 
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
//=END SONGBIRD GPL
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// iPod device module services.
//
//   These services provide support for registering the iPod device component
// module.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/** 
 * \file  sbIPDModule.cpp
 * \brief Songbird iPod Component Factory and Main Entry Point.
 */


//------------------------------------------------------------------------------
//
// iPod device module imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIPDController.h"
#include "sbIPDMarshall.h"

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsICategoryManager.h>
#include <nsIClassInfoImpl.h>
#include <nsIGenericFactory.h>
#include <nsServiceManagerUtils.h>


//------------------------------------------------------------------------------
//
// iPod device module marshall services.
//
//------------------------------------------------------------------------------

// Marshall defs.
NS_GENERIC_FACTORY_CONSTRUCTOR(sbIPDMarshall)
NS_DECL_CLASSINFO(sbIPDMarshall)


/**
 * \brief Register the device marshall component.
 */

static NS_METHOD
sbIPDMarshallRegisterSelf(nsIComponentManager*         aCompMgr,
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

  // Add self to the device marshall category.
  rv = categoryManager->AddCategoryEntry(SB_DEVICE_MARSHALL_CATEGORY,
                                         SB_IPDMARSHALL_DESCRIPTION,
                                         SB_IPDMARSHALL_CONTRACTID,
                                         PR_TRUE,
                                         PR_TRUE,
                                         nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * \brief Unregister the device marshall component.
 */

static NS_METHOD
sbIPDMarshallUnregisterSelf(nsIComponentManager*         aCompMgr,
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

  // Delete self from the device marshall category.
  rv = categoryManager->DeleteCategoryEntry(SB_DEVICE_MARSHALL_CATEGORY,
                                            SB_IPDMARSHALL_DESCRIPTION,
                                            PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// iPod device module controller services.
//
//------------------------------------------------------------------------------

// Controller defs.
NS_GENERIC_FACTORY_CONSTRUCTOR(sbIPDController)
NS_DECL_CLASSINFO(sbIPDController)
SB_DEVICE_CONTROLLER_REGISTERSELF(sbIPDController);


//------------------------------------------------------------------------------
//
// iPod device module registration services.
//
//------------------------------------------------------------------------------

// Module component information.
static nsModuleComponentInfo sbIPDComponents[] =
{
  // Marshall component info.
  {
    SB_IPDMARSHALL_CLASSNAME,
    SB_IPDMARSHALL_CID,
    SB_IPDMARSHALL_CONTRACTID,
    sbIPDMarshallConstructor,
    sbIPDMarshallRegisterSelf,
    sbIPDMarshallUnregisterSelf,
    nsnull,
    NS_CI_INTERFACE_GETTER_NAME(sbIPDMarshall),
    nsnull,
    &NS_CLASSINFO_NAME(sbIPDMarshall)
  },

  // Controller component info.
  {
    SB_IPDCONTROLLER_CLASSNAME,
    SB_IPDCONTROLLER_CID,
    SB_IPDCONTROLLER_CONTRACTID,
    sbIPDControllerConstructor,
    sbIPDControllerRegisterSelf,
    sbIPDControllerUnregisterSelf,
    nsnull,
    NS_CI_INTERFACE_GETTER_NAME(sbIPDController),
    nsnull,
    &NS_CLASSINFO_NAME(sbIPDController)
  }
};

// NSGetModule
NS_IMPL_NSGETMODULE(sbIPDModule, sbIPDComponents)

