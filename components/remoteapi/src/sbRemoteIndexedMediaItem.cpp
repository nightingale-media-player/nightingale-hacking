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

#include "sbRemoteAPIUtils.h"
#include "sbRemoteIndexedMediaItem.h"
#include "sbRemotePlayer.h"

#include <sbClassInfoUtils.h>
#include <sbIMediaItem.h>

#include <nsServiceManagerUtils.h>

const static char* sPublicWProperties[] = { "" };

const static char* sPublicRProperties[] =
{
  // sbIIndexedMediaItem
  "helper:index",
  "helper:mediaItem",

  // nsIClassInfo
  "classinfo:classDescription",
  "classinfo:contractID",
  "classinfo:classID",
  "classinfo:implementationLanguage",
  "classinfo:flags"
};

const static char* sPublicMethods[] = { "" };

NS_IMPL_ISUPPORTS4(sbRemoteIndexedMediaItem,
                   nsIClassInfo,
                   nsISecurityCheckedComponent,
                   sbIIndexedMediaItem,
                   sbISecurityAggregator)

NS_IMPL_CI_INTERFACE_GETTER4(sbRemoteIndexedMediaItem,
                             nsISupports,
                             sbISecurityAggregator,
                             sbIIndexedMediaItem,
                             nsISecurityCheckedComponent)

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbRemoteIndexedMediaItem)

SB_IMPL_SECURITYCHECKEDCOMP_INIT(sbRemoteIndexedMediaItem)

sbRemoteIndexedMediaItem::sbRemoteIndexedMediaItem(sbRemotePlayer* aRemotePlayer,
                                                   sbIIndexedMediaItem* aIndexedMediaItem) :
  mRemotePlayer(aRemotePlayer),
  mIndexedMediaItem(aIndexedMediaItem)
{
  NS_ASSERTION(aRemotePlayer, "Null remote player!");
  NS_ASSERTION(aIndexedMediaItem, "Null media item!");
}

// ---------------------------------------------------------------------------
//
//                          sbISecurityAggregator
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP sbRemoteIndexedMediaItem::GetRemotePlayer(sbIRemotePlayer * *aRemotePlayer)
{
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
//                          sbIIndexedMediaItem
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteIndexedMediaItem::GetIndex(PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  return mIndexedMediaItem->GetIndex(_retval);
}

NS_IMETHODIMP
sbRemoteIndexedMediaItem::GetMediaItem(sbIMediaItem** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediaItem> mediaItem;
  nsresult rv = mIndexedMediaItem->GetMediaItem( getter_AddRefs(mediaItem) );
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> wrappedMediaItem;
  rv =  SB_WrapMediaItem(mRemotePlayer,
                         mediaItem,
                         getter_AddRefs(wrappedMediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = wrappedMediaItem);
  return NS_OK;
}

