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

#include "sbLocalMediaListBaseEnumerationListener.h"

#include <sbIMediaItem.h>
#include <sbIMediaList.h>

#include <nsComponentManagerUtils.h>

NS_IMPL_ISUPPORTS1(sbLocalMediaListBaseEnumerationListener , 
                   sbIMediaListEnumerationListener)

sbLocalMediaListBaseEnumerationListener ::sbLocalMediaListBaseEnumerationListener ()
: mHasItems(PR_FALSE)
{
}

sbLocalMediaListBaseEnumerationListener ::~sbLocalMediaListBaseEnumerationListener ()
{
}

nsresult 
sbLocalMediaListBaseEnumerationListener::Init()
{
  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIMutableArray> array =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  array.swap(mArray);

  return NS_OK;
}

nsresult
sbLocalMediaListBaseEnumerationListener::GetArray(nsIArray **aArray)
{
  NS_ENSURE_ARG_POINTER(aArray);
  NS_IF_ADDREF(*aArray = mArray);
  return mHasItems ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

nsresult
sbLocalMediaListBaseEnumerationListener::GetArrayLength(PRUint32 *aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);
  nsresult rv = mArray->GetLength(aLength);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
sbLocalMediaListBaseEnumerationListener::SetHasItems(bool aHasItems)
{
  mHasItems = aHasItems;
  return NS_OK;
}

NS_IMETHODIMP 
sbLocalMediaListBaseEnumerationListener::OnEnumerationBegin(sbIMediaList *aMediaList, 
                                                            PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

NS_IMETHODIMP 
sbLocalMediaListBaseEnumerationListener::OnEnumeratedItem(sbIMediaList *aMediaList, 
                                                          sbIMediaItem *aMediaItem, 
                                                          PRUint16 *_retval)
{
  NS_ENSURE_STATE(mArray);
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv = mArray->AppendElement(aMediaItem, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  if(!mHasItems) {
    mHasItems = PR_TRUE;
  }

  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}

NS_IMETHODIMP 
sbLocalMediaListBaseEnumerationListener::OnEnumerationEnd(sbIMediaList *aMediaList, 
                                                          nsresult aStatusCode)
{
  return NS_OK;
}
