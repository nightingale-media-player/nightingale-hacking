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

#include "nsIGenericFactory.h"
#include "MetadataManager.h"
#include "MetadataValues.h"
#include "MetadataChannel.h"
#include "MetadataBackscanner.h"
#include "MetadataJobManager.h"
#include "MetadataJob.h"
#include "prlog.h"

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(sbMetadataManager, sbMetadataManager::GetSingleton)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMetadataValues)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMetadataChannel)
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(sbMetadataBackscanner, sbMetadataBackscanner::GetSingleton)
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(sbMetadataJobManager, sbMetadataJobManager::GetSingleton)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMetadataJob, FactoryInit)

static nsModuleComponentInfo sbMetadataManagerComponent[] =
{
  {
    SONGBIRD_METADATAMANAGER_CLASSNAME,
    SONGBIRD_METADATAMANAGER_CID,
    SONGBIRD_METADATAMANAGER_CONTRACTID,
    sbMetadataManagerConstructor
  },

  {
    SONGBIRD_METADATAVALUES_CLASSNAME,
    SONGBIRD_METADATAVALUES_CID,
    SONGBIRD_METADATAVALUES_CONTRACTID,
    sbMetadataValuesConstructor
  },

  {
    SONGBIRD_METADATACHANNEL_CLASSNAME,
    SONGBIRD_METADATACHANNEL_CID,
    SONGBIRD_METADATACHANNEL_CONTRACTID,
    sbMetadataChannelConstructor
  },

  {
    SONGBIRD_METADATABACKSCANNER_CLASSNAME,
    SONGBIRD_METADATABACKSCANNER_CID,
    SONGBIRD_METADATABACKSCANNER_CONTRACTID,
    sbMetadataBackscannerConstructor
  },

  {
    SONGBIRD_METADATAJOBMANAGER_CLASSNAME,
    SONGBIRD_METADATAJOBMANAGER_CID,
    SONGBIRD_METADATAJOBMANAGER_CONTRACTID,
    sbMetadataJobManagerConstructor
  },

  {
    SONGBIRD_METADATAJOB_CLASSNAME,
    SONGBIRD_METADATAJOB_CID,
    SONGBIRD_METADATAJOB_CONTRACTID,
    sbMetadataJobConstructor
  }
};

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
  
  NS_IF_RELEASE(gBackscanner);
  gBackscanner = nsnull;
  
  NS_IF_RELEASE(gMetadataJobManager);
  gMetadataJobManager = nsnull;
}

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(SongbirdMetadataManagerComponent,
                                   sbMetadataManagerComponent,
                                   sbMetadataManagerComponentConstructor,
                                   sbMetadataManagerComponentDestructor)

