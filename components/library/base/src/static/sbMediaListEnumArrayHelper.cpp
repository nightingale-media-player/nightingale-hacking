/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END NIGHTINGALE GPL
 */

#include "sbMediaListEnumArrayHelper.h"

#include <nsComponentManagerUtils.h>
#include <sbIMediaList.h>
#include <sbIMediaItem.h>


NS_IMPL_ISUPPORTS1(sbMediaListEnumArrayHelper, sbIMediaListEnumerationListener)

sbMediaListEnumArrayHelper * sbMediaListEnumArrayHelper::New(nsIArray * aArray)
{
  sbMediaListEnumArrayHelper * helper = new sbMediaListEnumArrayHelper();
  nsresult rv = helper->Init(aArray);
  NS_ENSURE_SUCCESS(rv, nsnull);
  return helper;
}

sbMediaListEnumArrayHelper::sbMediaListEnumArrayHelper()
{
}

sbMediaListEnumArrayHelper::~sbMediaListEnumArrayHelper()
{
}

nsresult
sbMediaListEnumArrayHelper::Init(nsIArray * aArray)
{
  nsresult rv;
  if (aArray) {
    mItemsArray = do_QueryInterface(aArray, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    mItemsArray =
      do_CreateInstance("@getnightingale.com/moz/xpcom/threadsafe-array;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

NS_IMETHODIMP
sbMediaListEnumArrayHelper::GetMediaItemsArray(nsIArray **aOutArray)
{
  NS_ENSURE_ARG_POINTER(aOutArray);
  NS_IF_ADDREF(*aOutArray = mItemsArray);
  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIMediaListEnumerationListener

NS_IMETHODIMP
sbMediaListEnumArrayHelper::OnEnumerationBegin(sbIMediaList *aMediaList,
                                               PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

NS_IMETHODIMP
sbMediaListEnumArrayHelper::OnEnumeratedItem(sbIMediaList *aMediaList,
                                             sbIMediaItem *aMediaItem,
                                             PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  nsresult rv;
  if (!mItemsArray) {
    rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mItemsArray->AppendElement(aMediaItem, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP
sbMediaListEnumArrayHelper::OnEnumerationEnd(sbIMediaList *aMediaList,
                                             nsresult aStatusCode)
{
  return NS_OK;
}

