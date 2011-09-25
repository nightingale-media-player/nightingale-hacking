/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

// Local imports.
#include "sbMediaFileManager.h"
#include "sbMediaManagementJob.h"
#include "sbMediaManagementService.h"

// Mozilla imports.
#include <nsICategoryManager.h>
#include <nsIClassInfoImpl.h>
#include <nsIGenericFactory.h>

// Nightingale imports.
#include <sbStandardProperties.h>

// Construct and initialize the sbMediaFileManager object.
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaFileManager)

// Construct the sbMediaManagementJob object.
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaManagementJob)
NS_DECL_CLASSINFO(sbMediaManagementJob)

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMediaManagementService, Init)

static NS_METHOD
sbMediaManagementServiceRegisterSelf(nsIComponentManager *aCompMgr,
                                     nsIFile *aPath,
                                     const char *aLoaderStr,
                                     const char *aType,
                                     const nsModuleComponentInfo *aInfo)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman =
    do_GetService("@mozilla.org/categorymanager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  char* previousEntry;
  rv = catman->AddCategoryEntry("app-startup",
                                SB_MEDIAMANAGEMENTSERVICE_CLASSNAME,
                                "service," SB_MEDIAMANAGEMENTSERVICE_CONTRACTID,
                                PR_TRUE,
                                PR_TRUE,
                                &previousEntry);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (previousEntry) {
    NS_Free(previousEntry);
  }

  // register the properties we want to show up in the prefs dropdowns
  static const char* kDefaultFileNameProperties[] = {
    SB_PROPERTY_TRACKNAME,
    SB_PROPERTY_ALBUMNAME,
    SB_PROPERTY_ARTISTNAME,
    SB_PROPERTY_GENRE,
    SB_PROPERTY_TRACKNUMBER,
    SB_PROPERTY_YEAR,
    SB_PROPERTY_DISCNUMBER,
    SB_PROPERTY_TOTALDISCS,
    SB_PROPERTY_TOTALTRACKS,
    SB_PROPERTY_ISPARTOFCOMPILATION,
    SB_PROPERTY_PRODUCERNAME,
    SB_PROPERTY_COMPOSERNAME,
    SB_PROPERTY_CONDUCTORNAME,
    SB_PROPERTY_LYRICISTNAME,
    SB_PROPERTY_RECORDLABELNAME,
    SB_PROPERTY_BITRATE,
    SB_PROPERTY_SAMPLERATE,
    SB_PROPERTY_KEY,
    SB_PROPERTY_LANGUAGE,
    SB_PROPERTY_COPYRIGHT,
    SB_PROPERTY_SUBTITLE,
    SB_PROPERTY_METADATAUUID,
    SB_PROPERTY_SOFTWAREVENDOR,
    SB_PROPERTY_DESTINATION,
    SB_PROPERTY_ORIGINPAGE,
    SB_PROPERTY_ORIGINPAGETITLE,
    SB_PROPERTY_MEDIALISTNAME,
    SB_PROPERTY_AVAILABILITY,
    SB_PROPERTY_ALBUMARTISTNAME,
  };
  for (int i = 0; i < NS_ARRAY_LENGTH(kDefaultFileNameProperties); ++i) {
    char* previousEntry;
    rv = catman->AddCategoryEntry(SB_CATEGORY_MEDIA_MANAGER_FILE_NAME_PROPERTIES,
                                  kDefaultFileNameProperties[i],
                                  "",
                                  PR_TRUE,
                                  PR_TRUE,
                                  &previousEntry);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_Free(previousEntry);
  }
  
  static const char* kDefaultDirectoryNameProperties[] = {
    SB_PROPERTY_ALBUMNAME,
    SB_PROPERTY_ARTISTNAME,
    SB_PROPERTY_GENRE,
    SB_PROPERTY_YEAR,
    SB_PROPERTY_DISCNUMBER,
    SB_PROPERTY_TOTALDISCS,
    SB_PROPERTY_TOTALTRACKS,
    SB_PROPERTY_ISPARTOFCOMPILATION,
    SB_PROPERTY_PRODUCERNAME,
    SB_PROPERTY_COMPOSERNAME,
    SB_PROPERTY_CONDUCTORNAME,
    SB_PROPERTY_LYRICISTNAME,
    SB_PROPERTY_RECORDLABELNAME,
    SB_PROPERTY_BITRATE,
    SB_PROPERTY_SAMPLERATE,
    SB_PROPERTY_KEY,
    SB_PROPERTY_LANGUAGE,
    SB_PROPERTY_COPYRIGHT,
    SB_PROPERTY_SUBTITLE,
    SB_PROPERTY_DESTINATION,
    SB_PROPERTY_ORIGINPAGE,
    SB_PROPERTY_ORIGINPAGETITLE,
    SB_PROPERTY_MEDIALISTNAME,
    SB_PROPERTY_AVAILABILITY,
    SB_PROPERTY_ALBUMARTISTNAME,
  };
  for (int i = 0; i < NS_ARRAY_LENGTH(kDefaultDirectoryNameProperties); ++i) {
    char* previousEntry;
    rv = catman->AddCategoryEntry(SB_CATEGORY_MEDIA_MANAGER_DIRECTORY_NAME_PROPERTIES,
                                  kDefaultDirectoryNameProperties[i],
                                  "",
                                  PR_TRUE,
                                  PR_TRUE,
                                  &previousEntry);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_Free(previousEntry);
  }
  
  return NS_OK;
}

static const nsModuleComponentInfo sbMediaManagerComponents[] =
{
  // Media File Manager component info.
  {
    SB_MEDIAFILEMANAGER_DESCRIPTION,
    SB_MEDIAFILEMANAGER_CID,
    SB_MEDIAFILEMANAGER_CONTRACTID,
    sbMediaFileManagerConstructor
  },
  // Media management job component info.
  {
    SB_MEDIAMANAGEMENTJOB_CLASSNAME,
    SB_MEDIAMANAGEMENTJOB_CID,
    SB_MEDIAMANAGEMENTJOB_CONTRACTID,
    sbMediaManagementJobConstructor,
    nsnull, // RegisterSelf
    nsnull, // UnregisterSelf,
    nsnull, // factory destructor
    NS_CI_INTERFACE_GETTER_NAME(sbMediaManagementJob),
    nsnull, // language helper
    &NS_CLASSINFO_NAME(sbMediaManagementJob),  /* global class-info pointer */
    nsIClassInfo::THREADSAFE
  },
  // Media management service component info.
  {
    SB_MEDIAMANAGEMENTSERVICE_CLASSNAME,
    SB_MEDIAMANAGEMENTSERVICE_CID,
    SB_MEDIAMANAGEMENTSERVICE_CONTRACTID,
    sbMediaManagementServiceConstructor,
    sbMediaManagementServiceRegisterSelf
  }
};

NS_IMPL_NSGETMODULE(NightingaleMediaManagerModule, sbMediaManagerComponents)
