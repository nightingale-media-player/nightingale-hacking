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

#include <sbClassInfoUtils.h>
#include <sbIMediaItem.h>

#include <nsComponentManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsIProgrammingLanguage.h>
#include <nsIScriptNameSpaceManager.h>
#include <nsIScriptSecurityManager.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <prlog.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteMediaItem:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gRemoteMediaItemLog = nsnull;
#define LOG(args)   if (gRemoteMediaItemLog) PR_LOG(gRemoteMediaItemLog, PR_LOG_WARN, args)
#else
#define LOG(args)   /* nothing */
#endif

// prefixes for theses strings can be:
// metadata:
// library:
// classinfo:
// controls:
// binding:
// internal:
const static char* sPublicWProperties[] =
{
  // sbIMediaItem
  "library:mediaCreated",
  "library:mediaUpdated",
  "library:contentSrc",
  "library:contentLength",
  "library:contentType"
};

const static char* sPublicRProperties[] =
{
  // sbILibraryResource
  "library:guid",
  "library:created",
  "library:updated",

  // sbIMediaItem
  // XXXsteve I've omitted library since we don't want the user to get back
  // to the original library
  "library:isMutable",
  "library:mediaCreated",
  "library:mediaUpdated",
  "library:contentLength",
  "library:contentType",

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
  "library:equals"

  // sbIMediaItem
  // XXXsteve None of the media item methods are suitable for the remote api
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

