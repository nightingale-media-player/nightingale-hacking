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

#include "sbRemoteMediaList.h"

#include <prlog.h>
#include <sbClassInfoUtils.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteMediaList:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gRemoteMediaListLog = nsnull;
#endif

#define LOG(args) PR_LOG(gRemoteMediaListLog, PR_LOG_WARN, args)

const static char* sPublicWProperties[] =
{
  // sbIMediaList
  "library:name"
};

const static char* sPublicRProperties[] =
{
  // sbILibraryResource
  "library:guid",
  "library:created",
  "library:updated",

  // sbIMediaItem
  // omitting library since we don't want the user to get back
  // to the original library
  "library:isMutable",
  "library:mediaCreated",
  "library:mediaUpdated",
  "library:contentLength",
  "library:contentType",

  // sbIMediaList
  "library:name",
  "library:type",
  "library:length",
  "library:isEmpty",

  // sbIRemoteMediaList
  "library:selection",

  // nsIClassInfo
  "classinfo:classDescription",
  "classinfo:contractID",
  "classinfo:classID",
  "classinfo:implementationLanguage",
  "classinfo:flags"
};

const static char* sPublicMethods[] =
{ 
  // sbILibraryResource
  "library:getProperty",
  "library:setProperty",
  "library:equals",

  // sbIMediaItem
  // none applicable

  // sbIMediaList
  // omitting createView, listeners, and batching
  "library:getItemByGuid",
  "library:getItemByIndex",
  "library:enumerateAllItems",
  "library:enumerateItemsByProperty",
  "library:indexOf",
  "library:lastIndexOf",
  "library:contains",
  "library:add",
  "library:addAll",
  "library:remove",
  "library:removeByIndex",
  "library:clear",
  "library:getDistinctValuesForProperty",

  // sbIRemoteMediaList
  "internal:getView"
};

NS_IMPL_ISUPPORTS_INHERITED1( sbRemoteMediaList,
                              sbRemoteMediaListBase,
                              nsIClassInfo )

NS_IMPL_CI_INTERFACE_GETTER7( sbRemoteMediaList,
                              nsISupports,
                              sbISecurityAggregator,
                              sbIRemoteMediaList,
                              sbIMediaList,
                              sbIMediaItem,
                              sbILibraryResource,
                              nsISecurityCheckedComponent )

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbRemoteMediaList)

SB_IMPL_SECURITYCHECKEDCOMP_INIT(sbRemoteMediaList)

sbRemoteMediaList::sbRemoteMediaList( sbIMediaList* aMediaList,
                                      sbIMediaListView* aMediaListView ) :
  sbRemoteMediaListBase( aMediaList, aMediaListView )
{
#ifdef PR_LOGGING
  if (!gRemoteMediaListLog) {
    gRemoteMediaListLog = PR_NewLogModule("sbRemoteMediaList");
  }
  LOG(("sbRemoteMediaList::sbRemoteMediaList()"));
#endif
}

sbRemoteMediaList::~sbRemoteMediaList()
{
  LOG(("sbRemoteMediaList::~sbRemoteMediaList()"));
}

