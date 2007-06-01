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
#include "sbRemoteLibrary.h"

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
  // nsISimpleEnumerator
  "library:hasMoreElements",
  "library:getNext"
};

NS_IMPL_ISUPPORTS4(sbRemoteWrappingSimpleEnumerator,
                   nsIClassInfo,
                   nsISecurityCheckedComponent,
                   sbISecurityAggregator,
                   nsISimpleEnumerator)

NS_IMPL_CI_INTERFACE_GETTER3( sbRemoteWrappingSimpleEnumerator,
                              nsISecurityCheckedComponent,
                              nsISimpleEnumerator,
                              sbISecurityAggregator )

SB_IMPL_CLASSINFO_INTERFACES_ONLY(sbRemoteWrappingSimpleEnumerator)

nsresult
sbRemoteWrappingSimpleEnumerator::Init()
{
  nsresult rv;

  nsCOMPtr<sbISecurityMixin> mixin =
    do_CreateInstance("@songbirdnest.com/remoteapi/security-mixin;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the list of IIDs to pass to the security mixin
  nsIID ** iids;
  PRUint32 iidCount;
  rv = GetInterfaces(&iidCount, &iids);
  NS_ENSURE_SUCCESS(rv, rv);

  // initialize our mixin with approved interfaces, methods, properties
  rv = mixin->Init( (sbISecurityAggregator*)this,
                    (const nsIID**)iids, iidCount,
                    sPublicMethods, NS_ARRAY_LENGTH(sPublicMethods),
                    sPublicRProperties,NS_ARRAY_LENGTH(sPublicRProperties),
                    sPublicWProperties, NS_ARRAY_LENGTH(sPublicWProperties) );
  NS_ENSURE_SUCCESS(rv, rv);

  mSecurityMixin = do_QueryInterface(mixin, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

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

  nsCOMPtr<sbIMediaItem> mediaItem = do_QueryInterface(supports, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> wrappedMediaItem;
  rv = SB_WrapMediaItem(mediaItem, getter_AddRefs(wrappedMediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = wrappedMediaItem);
  return NS_OK;
}

