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
#include "sbRemoteWrappingSimpleEnumerator.h"
#include "sbRemoteIndexedMediaItem.h"

#include <sbClassInfoUtils.h>
#include <sbIMediaItem.h>

#include <nsAutoPtr.h>
#include <nsStringGlue.h>

const static char* sPublicWProperties[] =
{
  // Need this empty string to make VC happy
  ""
};

const static char* sPublicRProperties[] =
{
  // Need this empty string to make VC happy
  ""
};

const static char* sPublicMethods[] =
{
  // internal for helper classes like this
  // nsISimpleEnumerator
  "helper:hasMoreElements",
  "helper:getNext"
};

NS_IMPL_ISUPPORTS4(sbRemoteWrappingSimpleEnumerator,
                   nsIClassInfo,
                   nsISecurityCheckedComponent,
                   sbISecurityAggregator,
                   nsISimpleEnumerator)

NS_IMPL_CI_INTERFACE_GETTER3(sbRemoteWrappingSimpleEnumerator,
                             nsISecurityCheckedComponent,
                             nsISimpleEnumerator,
                             sbISecurityAggregator)

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbRemoteWrappingSimpleEnumerator)

SB_IMPL_SECURITYCHECKEDCOMP_INIT(sbRemoteWrappingSimpleEnumerator)

// ---------------------------------------------------------------------------
//
//                          sbISecurityAggregator
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP 
sbRemoteWrappingSimpleEnumerator::GetRemotePlayer(sbIRemotePlayer * *aRemotePlayer)
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
//                            nsISimpleEnumerator
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteWrappingSimpleEnumerator::HasMoreElements(PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  return mWrapped->HasMoreElements(_retval);
}

NS_IMETHODIMP
sbRemoteWrappingSimpleEnumerator::GetNext(nsISupports** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<nsISupports> supports;
  nsresult rv = mWrapped->GetNext(getter_AddRefs(supports));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIIndexedMediaItem> item = do_QueryInterface(supports, &rv);
  if (NS_SUCCEEDED(rv)) {
    // we have an IndexedMediaItem, we're handling a selection enumeration
    nsRefPtr<sbRemoteIndexedMediaItem> indexedMediaItem =
      new sbRemoteIndexedMediaItem(mRemotePlayer, item);
    NS_ENSURE_TRUE(indexedMediaItem, NS_ERROR_OUT_OF_MEMORY);

    rv = indexedMediaItem->Init();
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ADDREF( *_retval = NS_ISUPPORTS_CAST(sbIIndexedMediaItem*,
                                            indexedMediaItem) );
  }
  else {
    nsCOMPtr<sbIMediaItem> item = do_QueryInterface(supports, &rv);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<sbIMediaItem> remoteItem;
      rv = SB_WrapMediaItem(mRemotePlayer, item, getter_AddRefs(remoteItem));
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ADDREF( *_retval = NS_ISUPPORTS_CAST(sbIMediaItem*,
                                              remoteItem) );
    }
  }

  return NS_OK;
}

