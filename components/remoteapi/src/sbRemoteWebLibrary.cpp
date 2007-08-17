/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

#include "sbRemoteWebLibrary.h"

#include <prlog.h>
#include <sbClassInfoUtils.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteWebLibrary:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gWebLibraryLog = nsnull;
#endif

#define LOG(args) PR_LOG( gWebLibraryLog, PR_LOG_WARN, args )

const static char* sPublicWProperties[] = { "" };

const static char* sPublicRProperties[] =
  { // sbIMediaList
    "site:name",
    "site:type",
    "site:length",

    // sbIRemoteLibrary
    "site:scanMediaOnCreation",

    // nsIClassInfo
    "classinfo:classDescription",
    "classinfo:contractID",
    "classinfo:classID",
    "classinfo:implementationLanguage",
    "classinfo:flags"
  };

const static char* sPublicMethods[] =
  { // sbIMediaList
    "site:getItemByGuid",
    "site:getItemByIndex",
    "site:enumerateAllItems",
    "site:enumerateItemsByProperty",
    "site:indexOf",
    "site:lastIndexOf",
    "site:contains",
    "site:getDistinctValuesForProperty",

    // sbILibraryResource
    "site:getProperty",
    "site:equals"
  };

NS_IMPL_ISUPPORTS_INHERITED1( sbRemoteWebLibrary,
                              sbRemoteLibrary,
                              nsIClassInfo )

NS_IMPL_CI_INTERFACE_GETTER10( sbRemoteWebLibrary,
                               nsISupports,
                               sbISecurityAggregator,
                               sbIRemoteLibrary,
                               sbIRemoteMediaList,
                               sbIWrappedMediaList,
                               sbIWrappedMediaItem,
                               sbIMediaList,
                               sbIMediaItem,
                               sbILibraryResource,
                               nsISecurityCheckedComponent )

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbRemoteWebLibrary)

SB_IMPL_SECURITYCHECKEDCOMP_INIT(sbRemoteWebLibrary)

sbRemoteWebLibrary::sbRemoteWebLibrary() : sbRemoteLibrary()
{
#ifdef PR_LOGGING
  if (!gWebLibraryLog) {
    gWebLibraryLog = PR_NewLogModule("sbRemoteWebLibrary");
  }
  LOG(("sbRemoteWebLibrary::sbRemoteWebLibrary()"));
#endif
}

sbRemoteWebLibrary::~sbRemoteWebLibrary()
{
  LOG(("sbRemoteWebLibrary::~sbRemoteWebLibrary()"));
}

nsresult
sbRemoteWebLibrary::InitInternalMediaList()
{
  LOG(("sbRemoteWebLibrary::InitInternalMediaList()"));
  NS_ASSERTION( mLibrary, "EEK! Initing internals without a mLibrary" );

  nsCOMPtr<sbIMediaList> mediaList( do_QueryInterface(mLibrary) );
  NS_ENSURE_TRUE( mediaList, NS_ERROR_FAILURE );

  nsCOMPtr<sbIMediaListView> mediaListView;
  nsresult rv = mediaList->CreateView( getter_AddRefs(mediaListView) );
  NS_ENSURE_SUCCESS( rv, rv );

  // XXXredfive this needs to become a sbRemoteWebMediaList I think
  mRemMediaList = new sbRemoteMediaList( mediaList, mediaListView );
  NS_ENSURE_TRUE( mRemMediaList, NS_ERROR_OUT_OF_MEMORY );

  rv = mRemMediaList->Init();
  NS_ENSURE_SUCCESS( rv, rv );

  return rv;
}

