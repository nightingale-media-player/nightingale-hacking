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
#include "sbRemotePlayer.h"

#include <prlog.h>
#include <sbClassInfoUtils.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteMediaList:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gRemoteMediaListLog = nsnull;
#endif

#undef LOG
#define LOG(args) PR_LOG(gRemoteMediaListLog, PR_LOG_WARN, args)

const static char* sPublicWProperties[] =
{
  // sbIMediaList
  "library_write:name"
};

const static char* sPublicRProperties[] =
{
  // sbILibraryResource
  "library_read:guid",
  "library_read:created",
  "library_read:updated",

  // sbIMediaItem
  // omitting library since we don't want the user to get back
  // to the original library
  "library_read:isMutable",
  "library_read:mediaCreated",
  "library_read:mediaUpdated",
  "library_read:contentLength",
  "library_read:contentType",

  // sbIMediaList
  "library_read:name",
  "library_read:type",
  "library_read:length",
  "library_read:isEmpty",

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
  "library_read:getProperty",
  "library_write:setProperty",
  "library_read:equals",

  // sbIMediaItem
  // none applicable

  // sbIMediaList
  // omitting createView, listeners, and batching
  "library_read:getItemByGuid",
  "library_read:getItemByIndex",
  "library_read:enumerateAllItems",
  "library_read:enumerateItemsByProperty",
  "library_read:indexOf",
  "library_read:lastIndexOf",
  "library_read:contains",
  "library_write:add",
  "library_write:addAll",
  "library_write:remove",
  "library_write:removeByIndex",
  "library_write:clear",
  "library_read:getDistinctValuesForProperty",

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

sbRemoteMediaList::sbRemoteMediaList( sbRemotePlayer* aRemotePlayer,
                                      sbIMediaList* aMediaList,
                                      sbIMediaListView* aMediaListView ) :
  sbRemoteMediaListBase( aRemotePlayer, aMediaList, aMediaListView )
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

