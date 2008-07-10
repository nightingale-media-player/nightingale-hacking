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
* \file  MetadataManagerComponent.cpp
* \brief Songbird Metadata Manager Component Factory and Main Entry Point.
*/
#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>

#include "sbMetadataManager.h"
#include "sbMetadataChannel.h"
#include "MetadataJobManager.h"
#include "MetadataJob.h"
#include "prlog.h"

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(sbMetadataManager, sbMetadataManager::GetSingleton)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMetadataChannel)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMetadataJobManager)



// Registration functions for becoming a startup observer
static NS_METHOD
sbMetadataJobManagerRegisterSelf(nsIComponentManager* aCompMgr,
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
                          SONGBIRD_METADATAJOBMANAGER_DESCRIPTION,
                          "service," SONGBIRD_METADATAJOBMANAGER_CONTRACTID,
                          PR_TRUE, PR_TRUE, nsnull);
  return rv;
}

static NS_METHOD
sbMetadataJobManagerUnregisterSelf(nsIComponentManager* aCompMgr,
                                   nsIFile* aPath,
                                   const char* registryLocation,
                                   const nsModuleComponentInfo* info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = categoryManager->DeleteCategoryEntry(APPSTARTUP_CATEGORY,
                                            SONGBIRD_METADATAJOBMANAGER_DESCRIPTION,
                                            PR_TRUE);

  return rv;
}



static nsModuleComponentInfo sbMetadataManagerComponent[] =
{
  {
    SONGBIRD_METADATAMANAGER_CLASSNAME,
    SONGBIRD_METADATAMANAGER_CID,
    SONGBIRD_METADATAMANAGER_CONTRACTID,
    sbMetadataManagerConstructor
  },

  {
    SONGBIRD_METADATACHANNEL_CLASSNAME,
    SONGBIRD_METADATACHANNEL_CID,
    SONGBIRD_METADATACHANNEL_CONTRACTID,
    sbMetadataChannelConstructor
  },

  {
    SONGBIRD_METADATAJOBMANAGER_CLASSNAME,
    SONGBIRD_METADATAJOBMANAGER_CID,
    SONGBIRD_METADATAJOBMANAGER_CONTRACTID,
    sbMetadataJobManagerConstructor,
    sbMetadataJobManagerRegisterSelf,
    sbMetadataJobManagerUnregisterSelf
  }
};



// TODO Consider changing the following:

#ifdef PR_LOGGING
PRLogModuleInfo *gMetadataLog;
#endif

PR_STATIC_CALLBACK(nsresult)
sbMetadataManagerComponentConstructor(nsIModule *self)
{
#ifdef PR_LOGGING
  gMetadataLog = PR_NewLogModule("metadata");
#endif
  return NS_OK;
}

// When everything else shuts down, delete it.
PR_STATIC_CALLBACK(void)
sbMetadataManagerComponentDestructor(nsIModule* module)
{
  NS_IF_RELEASE(gMetadataManager);
  gMetadataManager = nsnull;
}

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(SongbirdMetadataManagerComponent,
                                   sbMetadataManagerComponent,
                                   sbMetadataManagerComponentConstructor,
                                   sbMetadataManagerComponentDestructor)

