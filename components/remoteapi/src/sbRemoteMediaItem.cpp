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

#include "sbRemoteMediaItem.h"

#include <prlog.h>
#include <sbClassInfoUtils.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteMediaItem:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gRemoteMediaItemLog = nsnull;
#endif

#undef LOG
#define LOG(args) PR_LOG(gRemoteMediaItemLog, PR_LOG_WARN, args)

const static char* sPublicWProperties[] = { "" };

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
  "library_read:equals"

  // sbIMediaItem
  // none applicable
};

NS_IMPL_ISUPPORTS6(sbRemoteMediaItem,
                   nsIClassInfo,
                   nsISecurityCheckedComponent,
                   sbISecurityAggregator,
                   sbIMediaItem,
                   sbIWrappedMediaItem,
                   sbILibraryResource)

NS_IMPL_CI_INTERFACE_GETTER5(sbRemoteMediaItem,
                             nsISupports,
                             sbISecurityAggregator,
                             sbIMediaItem,
                             sbILibraryResource,
                             nsISecurityCheckedComponent)

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbRemoteMediaItem)

SB_IMPL_SECURITYCHECKEDCOMP_INIT(sbRemoteMediaItem)

sbRemoteMediaItem::sbRemoteMediaItem(sbIMediaItem* aMediaItem) :
  mMediaItem(aMediaItem)
{
  NS_ASSERTION(aMediaItem, "Null media list!");

#ifdef PR_LOGGING
  if (!gRemoteMediaItemLog) {
    gRemoteMediaItemLog = PR_NewLogModule("sbRemoteMediaItem");
  }
  LOG(("sbRemoteMediaItem::sbRemoteMediaItem()"));
#endif
}

NS_IMETHODIMP_(already_AddRefed<sbIMediaItem>)
sbRemoteMediaItem::GetMediaItem()
{
  sbIMediaItem* item = mMediaItem;
  NS_ADDREF(item);
  return item;
}

