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

#include "sbRemoteSiteMediaList.h"
#include <sbClassInfoUtils.h>

#include <prlog.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteSiteMediaList:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gRemoteMediaListLog = nsnull;
#endif

#define LOG(args) PR_LOG(gRemoteMediaListLog, PR_LOG_WARN, args)

const static char* sPublicWProperties[] =
{
  // sbIMediaList
  "site:name"
};

const static char* sPublicRProperties[] =
{
  // sbILibraryResource
  "site:guid",
  "site:created",
  "site:updated",

  // sbIMediaItem
  // omitting library since we don't want the user to get back
  // to the original library
  "site:isMutable",
  "site:mediaCreated",
  "site:mediaUpdated",
  "site:contentLength",
  "site:contentType",

  // sbIMediaList
  "site:name",
  "site:type",
  "site:length",
  "site:isEmpty",

  // sbIRemoteMediaList
  "site:selection",

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
  "site:getProperty",
  "site:setProperty",
  "site:equals",

  // sbIMediaItem
  // none applicable

  // sbIMediaList
  // omitting createView, listeners, and batching
  "site:getItemByGuid",
  "site:getItemByIndex",
  "site:enumerateAllItems",
  "site:enumerateItemsByProperty",
  "site:indexOf",
  "site:lastIndexOf",
  "site:contains",
  "site:add",
  "site:addAll",
  "site:remove",
  "site:removeByIndex",
  "site:clear",
  "site:getDistinctValuesForProperty",

  // sbIRemoteMediaList
  "internal:getView"
};

NS_IMPL_ISUPPORTS_INHERITED1( sbRemoteSiteMediaList,
                              sbRemoteMediaListBase,
                              nsIClassInfo )

NS_IMPL_CI_INTERFACE_GETTER7( sbRemoteSiteMediaList,
                              nsISupports,
                              sbISecurityAggregator,
                              sbIRemoteMediaList,
                              sbIMediaList,
                              sbIMediaItem,
                              sbILibraryResource,
                              nsISecurityCheckedComponent )

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbRemoteSiteMediaList)

SB_IMPL_SECURITYCHECKEDCOMP_INIT(sbRemoteSiteMediaList)

sbRemoteSiteMediaList::sbRemoteSiteMediaList( sbIMediaList* aMediaList,
                                              sbIMediaListView* aMediaListView ) :
  sbRemoteMediaListBase( aMediaList, aMediaListView )
{
#ifdef PR_LOGGING
  if (!gRemoteMediaListLog) {
    gRemoteMediaListLog = PR_NewLogModule("sbRemoteSiteMediaList");
  }
  LOG(("sbRemoteSiteMediaList::sbRemoteSiteMediaList()"));
#endif
}

sbRemoteSiteMediaList::~sbRemoteSiteMediaList()
{
  LOG(("sbRemoteSiteMediaList::~sbRemoteSiteMediaList()"));
}

