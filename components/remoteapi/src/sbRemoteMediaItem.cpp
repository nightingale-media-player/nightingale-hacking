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

#include "sbRemoteMediaItem.h"
#include "sbRemotePlayer.h"
#include "sbRemoteLibraryResource.h"

#include <prlog.h>
#include <sbClassInfoUtils.h>
#include <sbIPropertyManager.h>
#include <sbILibrary.h>
#include <nsServiceManagerUtils.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteMediaItem:5
 */
#ifdef PR_LOGGING
PRLogModuleInfo* gRemoteMediaItemLog = nsnull;
#endif

#undef LOG
#define LOG(args)  LOG_ITEM(args)

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

SB_IMPL_SECURITYCHECKEDCOMP_INIT_LIBRES(sbRemoteMediaItem,
                                        sbRemoteLibraryResource,
                                        (mRemotePlayer, mMediaItem) )

sbRemoteMediaItem::sbRemoteMediaItem(sbRemotePlayer* aRemotePlayer,
                                     sbIMediaItem* aMediaItem) :
  mRemotePlayer(aRemotePlayer),
  mMediaItem(aMediaItem)
{
  NS_ASSERTION(aRemotePlayer, "Null remote player!");
  NS_ASSERTION(aMediaItem, "Null media item!");

#ifdef PR_LOGGING
  if (!gRemoteMediaItemLog) {
    gRemoteMediaItemLog = PR_NewLogModule("sbRemoteMediaItem");
  }
  LOG_ITEM(("sbRemoteMediaItem::sbRemoteMediaItem()"));
#endif
}

sbRemoteMediaItem::~sbRemoteMediaItem()
{
  LOG_ITEM(("sbRemoteMediaItem::~sbRemoteMediaItem()"));
}

// ---------------------------------------------------------------------------
//
//                          sbISecurityAggregator
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteMediaItem::GetRemotePlayer(sbIRemotePlayer * *aRemotePlayer)
{
  LOG_ITEM(( "sbRemoteMediaItem::GetRemotePlayer()"));
  NS_ENSURE_STATE(mRemotePlayer);
  NS_ENSURE_ARG_POINTER(aRemotePlayer);

  nsresult rv;
  *aRemotePlayer = nsnull;

  nsCOMPtr<sbIRemotePlayer> remotePlayer;

  rv = mRemotePlayer->QueryInterface( NS_GET_IID( sbIRemotePlayer ), getter_AddRefs( remotePlayer ) );
  NS_ENSURE_SUCCESS( rv, rv );

  remotePlayer.swap( *aRemotePlayer );

  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                          sbIWrappedMediaItem
//
// ---------------------------------------------------------------------------

already_AddRefed<sbIMediaItem>
sbRemoteMediaItem::GetMediaItem()
{
  LOG_ITEM(( "sbRemoteMediaItem::GetMediaItem()"));
  sbIMediaItem* item = mMediaItem;
  NS_ADDREF(item);
  return item;
}

