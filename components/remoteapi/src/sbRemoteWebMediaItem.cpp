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

#include "sbRemoteWebMediaItem.h"
#include "sbRemotePlayer.h"

#include <prlog.h>
#include <sbClassInfoUtils.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteWebMediaItem:5
 *   LOG_ITEM defined in sbRemoteMediaItem.h/.cpp
 */
#undef LOG
#define LOG(args) LOG_ITEM(args)

const static char* sPublicWProperties[] = {""};

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
  "site:equals"

  // sbIMediaItem
  // none applicable
};

NS_IMPL_ISUPPORTS_INHERITED1( sbRemoteWebMediaItem,
                              sbRemoteMediaItem,
                              nsIClassInfo )

NS_IMPL_CI_INTERFACE_GETTER5( sbRemoteWebMediaItem,
                              nsISupports,
                              sbISecurityAggregator,
                              sbIMediaItem,
                              sbILibraryResource,
                              nsISecurityCheckedComponent )

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbRemoteWebMediaItem)

SB_IMPL_SECURITYCHECKEDCOMP_INIT(sbRemoteWebMediaItem)

sbRemoteWebMediaItem::sbRemoteWebMediaItem( sbRemotePlayer* aRemotePlayer,
                                            sbIMediaItem* aMediaItem ) :
  sbRemoteMediaItem(aRemotePlayer, aMediaItem)
{
  LOG_ITEM(("sbRemoteWebMediaItem::sbRemoteWebMediaItem()"));
}

sbRemoteWebMediaItem::~sbRemoteWebMediaItem()
{
  LOG_ITEM(("sbRemoteWebMediaItem::~sbRemoteWebMediaItem()"));
}

