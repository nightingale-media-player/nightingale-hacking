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
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbRemoteIndexedMediaItem> indexedMediaItem =
    new sbRemoteIndexedMediaItem(mRemotePlayer, item);
  NS_ENSURE_TRUE(indexedMediaItem, NS_ERROR_OUT_OF_MEMORY);

  rv = indexedMediaItem->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF( *_retval = NS_ISUPPORTS_CAST(sbIIndexedMediaItem*,
                                          indexedMediaItem) );
  return NS_OK;
}

