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

#include "sbRemoteLibrary.h"
#include <sbClassInfoUtils.h>
#include <sbIMediaList.h>

#include <nsComponentManagerUtils.h>
#include <nsIProgrammingLanguage.h>
#include <nsMemory.h>
#include <prlog.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteLibrary:<5|4|3|2|1>
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gLibraryLog = nsnull;
#endif

#define LOG(args) PR_LOG(gLibraryLog, PR_LOG_WARN, args)

static NS_DEFINE_CID(kRemoteLibraryCID, SONGBIRD_REMOTELIBRARY_CID);

const static char* sPublicWProperties[] =
  {
    "library:scanMediaOnCreation"
  };

const static char* sPublicRProperties[] =
  { //
    "library:artists",
    "library:albums",
    "library:name",
    "library:type",
    "library:length",

    // sbIRemoteLibrary
    "library:selection",
    "library:scanMediaOnCreation",
#ifdef DEBUG
    "library:filename",
#endif

    // nsIClassInfo
    "classinfo:classDescription",
    "classinfo:contractID",
    "classinfo:classID",
    "classinfo:implementationLanguage",
    "classinfo:flags"
  };

const static char* sPublicMethods[] =
  { // sbIRemoteLibrary
    "library:connectToMediaLibrary",
    "library:connectToDefaultLibrary",
    "library:createMediaListFromURL",
     // different from the ones in sbILibrary
    "library:createMediaList",
    "library:createMediaItem",

    // sbIMediaList
    "library:getItemByGuid",
    "library:getItemByIndex",
    "library:enumerateAllItems",
    "library:enumerateItemsByProperty",
    "library:indexOf",
    "library:lastIndexOf",
    "library:contains",
    "library:add",
    "library:addAll",
    "library:addSome",
    "library:remove",
    "library:removeByIndex",
    "library:getDistinctValuesForProperty",

    // sbILibraryResource
    "library:getProperty",
    "library:setProperty",
    "library:equals"
  };

NS_IMPL_ISUPPORTS_INHERITED1( sbRemoteLibrary,
                              sbRemoteLibraryBase,
                              nsIClassInfo )

NS_IMPL_CI_INTERFACE_GETTER6( sbRemoteLibrary,
                              sbISecurityAggregator,
                              sbIRemoteLibrary,
                              sbIRemoteMediaList,
                              sbIMediaList,
                              sbIMediaItem,
                              nsISecurityCheckedComponent )

SB_IMPL_CLASSINFO( sbRemoteLibrary,
                   SONGBIRD_REMOTELIBRARY_CONTRACTID,
                   SONGBIRD_REMOTELIBRARY_CLASSNAME,
                   nsIProgrammingLanguage::CPLUSPLUS,
                   0,
                   kRemoteLibraryCID )

SB_IMPL_SECURITYCHECKEDCOMP_INIT(sbRemoteLibrary)

sbRemoteLibrary::sbRemoteLibrary() : sbRemoteLibraryBase()
{
#ifdef PR_LOGGING
  if (!gLibraryLog) {
    gLibraryLog = PR_NewLogModule("sbRemoteLibrary");
  }
  LOG(("sbRemoteLibrary::sbRemoteLibrary()"));
#endif
}

sbRemoteLibrary::~sbRemoteLibrary()
{
  LOG(("sbRemoteLibrary::~sbRemoteLibrary()"));
}


// ---------------------------------------------------------------------------
//
//                            Helper Methods
//
// ---------------------------------------------------------------------------

// create an sbRemoteMediaList object to delegate the calls for
// sbIMediaList, sbIMediaItem, sbIRemoteMedialist
nsresult
sbRemoteLibrary::InitInternalMediaList()
{
  LOG(("sbRemoteLibrary::InitInternalMediaList()"));
  NS_ENSURE_STATE(mLibrary);

  nsCOMPtr<sbIMediaList> mediaList = do_QueryInterface(mLibrary);
  NS_ENSURE_TRUE( mediaList, NS_ERROR_FAILURE );

  nsCOMPtr<sbIMediaListView> mediaListView;
  nsresult rv = mediaList->CreateView(getter_AddRefs(mediaListView));
  NS_ENSURE_SUCCESS(rv, rv);

  mRemMediaList = new sbRemoteMediaList( mediaList, mediaListView );
  NS_ENSURE_TRUE( mRemMediaList, NS_ERROR_OUT_OF_MEMORY );

  rv = mRemMediaList->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

