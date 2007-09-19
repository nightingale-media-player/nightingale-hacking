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
#include "sbRemotePlayer.h"

#include <sbClassInfoUtils.h>
#include <sbIMediaList.h>

#include <prlog.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteLibrary:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gLibraryLog = nsnull;
#endif

#undef LOG
#define LOG(args) PR_LOG(gLibraryLog, PR_LOG_WARN, args)

const static char* sPublicWProperties[] =
  {
    "library:scanMediaOnCreation"
  };

const static char* sPublicRProperties[] =
  { // sbIMediaList
    "library_read:name",
    "library_read:type",
    "library_read:length",

    // sbIRemoteLibrary
    "library_read:scanMediaOnCreation",

    // nsIClassInfo
    "classinfo:classDescription",
    "classinfo:contractID",
    "classinfo:classID",
    "classinfo:implementationLanguage",
    "classinfo:flags"
  };

const static char* sPublicMethods[] =
  { // sbIRemoteLibrary
    "library_create:createMediaListFromURL",
    "library_read:getMediaList",
     // different from the ones in sbILibrary
    "library_create:createMediaList",
    "library_write:createMediaItem",

    // sbIMediaList
    "library_read:getItemByGuid",
    "library_read:getItemByIndex",
    "library_read:enumerateAllItems",
    "library_read:enumerateItemsByProperty",
    "library_read:indexOf",
    "library_read:lastIndexOf",
    "library_read:contains",
    "library_write:add",
    "library_write:addAll",
    "library_write:addSome",
    "library_read:getDistinctValuesForProperty",

    // sbILibraryResource
    "library_read:getProperty",
    "library_write:setProperty",
    "library_read:equals"
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

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbRemoteLibrary)

SB_IMPL_SECURITYCHECKEDCOMP_INIT(sbRemoteLibrary)

sbRemoteLibrary::sbRemoteLibrary(sbRemotePlayer* aRemotePlayer) :
  sbRemoteLibraryBase(aRemotePlayer)
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
  nsresult rv = mediaList->CreateView( getter_AddRefs(mediaListView) );
  NS_ENSURE_SUCCESS( rv, rv );

  mRemMediaList = new sbRemoteMediaList( mRemotePlayer,
                                         mediaList,
                                         mediaListView );
  NS_ENSURE_TRUE( mRemMediaList, NS_ERROR_OUT_OF_MEMORY );

  rv = mRemMediaList->Init();
  NS_ENSURE_SUCCESS( rv, rv );

  return rv;
}

