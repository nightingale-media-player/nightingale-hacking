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

#include <sbILibraryResource.h>

#include "sbRemotePlayer.h"
#include "sbRemoteSiteMediaList.h"
#include "sbRemoteSiteLibraryResource.h"
#include <sbClassInfoUtils.h>
#include <prlog.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteMediaList:5
 *   LOG_LIST defined in sbRemoteMediaListBase.h/.cpp
 */
#undef LOG
#define LOG(args) LOG_LIST(args)

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
                              sbRemoteMediaList,
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

SB_IMPL_SECURITYCHECKEDCOMP_INIT_LIBRES(sbRemoteSiteMediaList,
                                        sbRemoteSiteLibraryResource,
                                        (mRemotePlayer, mMediaList) )

sbRemoteSiteMediaList::sbRemoteSiteMediaList( sbRemotePlayer* aRemotePlayer,
                                              sbIMediaList* aMediaList,
                                              sbIMediaListView* aMediaListView ) :
  sbRemoteMediaList( aRemotePlayer, aMediaList, aMediaListView )
{
  LOG_LIST(("sbRemoteSiteMediaList::sbRemoteSiteMediaList()"));
}

sbRemoteSiteMediaList::~sbRemoteSiteMediaList()
{
  LOG_LIST(("sbRemoteSiteMediaList::~sbRemoteSiteMediaList()"));
}

