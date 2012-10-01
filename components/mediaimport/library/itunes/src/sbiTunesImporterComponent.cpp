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
* \file  sbiTUnesImporterComponent.cpp
* \brief Songbird iTunes Importer component
*/

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <mozilla/ModuleUtils.h>

#include "sbiTunesXMLParser.h"
#include "sbiTunesImporter.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbiTunesXMLParser)

NS_GENERIC_FACTORY_CONSTRUCTOR(sbiTunesImporter)

#define SB_LIBRARY_IMPORTER_CATEGORY "library-importer"

// Registration functions for becoming a startup observer
static NS_METHOD sbiTunesImporterRegisterSelf(nsIComponentManager* aCompMgr,
                                              nsIFile* aPath,
                                              const char* registryLocation,
                                              const char* componentType,
                                              const nsModuleComponentInfo* info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager =
      do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = categoryManager->AddCategoryEntry(SB_LIBRARY_IMPORTER_CATEGORY,
                                         SBITUNESIMPORTER_CLASSNAME,
                                         SBITUNESIMPORTER_CONTRACTID,
                                         PR_TRUE, PR_TRUE, nsnull);

  return rv;
}

static NS_METHOD sbiTunesImporterUnregisterSelf(nsIComponentManager* aCompMgr,
                                                nsIFile* aPath,
                                                const char* registryLocation,
                                                const nsModuleComponentInfo* info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager =
      do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = categoryManager->DeleteCategoryEntry(SB_LIBRARY_IMPORTER_CATEGORY,
                                            SBITUNESIMPORTER_CLASSNAME,
                                            PR_TRUE);
  return rv;
}

static nsModuleComponentInfo sbiTunesImporter[] =
{
  {
    SBITUNESXMLPARSER_CLASSNAME,
    SBITUNESXMLPARSER_CID,
    SBITUNESXMLPARSER_CONTRACTID,
    sbiTunesXMLParserConstructor
  },
  {
    SBITUNESIMPORTER_CLASSNAME,
    SBITUNESIMPORTER_CID,
    SBITUNESIMPORTER_CONTRACTID,
    sbiTunesImporterConstructor,
    sbiTunesImporterRegisterSelf,
    sbiTunesImporterUnregisterSelf
  },
};

NS_IMPL_NSGETMODULE(SongbirdiTunesImporterComponent, sbiTunesImporter)
