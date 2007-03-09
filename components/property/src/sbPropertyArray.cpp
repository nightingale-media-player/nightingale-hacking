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

#include "sbPropertyArray.h"

#include <nsArrayEnumerator.h>
#include <nsComponentManagerUtils.h>
#include <nsCOMPtr.h>
#include <nsIProperty.h>
#include <nsISimpleEnumerator.h>

#include "sbSimpleProperty.h"

NS_IMPL_ISUPPORTS3(sbPropertyArray, sbIPropertyArray,
                                    nsIMutableArray,
                                    nsIArray)

/**
 * See nsIArray
 */
NS_IMETHODIMP
sbPropertyArray::GetLength(PRUint32* aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);

  *aLength = (PRUint32)mArray.Count();
  return NS_OK;
}

/**
 * See nsIArray
 */
NS_IMETHODIMP
sbPropertyArray::QueryElementAt(PRUint32 aIndex,
                                const nsIID& aIID,
                                void** _retval)
{
  NS_ENSURE_ARG_MAX(aIndex, PR_MAX(0, mArray.Count() - 1));
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<nsISupports> element = mArray.ObjectAt(aIndex);
  NS_ENSURE_STATE(element);

  return element->QueryInterface(aIID, _retval);
}

/**
 * See nsIArray
 */
NS_IMETHODIMP
sbPropertyArray::IndexOf(PRUint32 aStartIndex,
                         nsISupports* aElement,
                         PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(aElement);
  NS_ENSURE_ARG_POINTER(_retval);
  
  // nsCOMArray doesn't provide any way for us to start searching for an element
  // at an offset, so warn about the usage. Hopefully no one will use this
  // method anyway ;)
  if (aStartIndex) {
    NS_WARNING("sbPropertyArray::IndexOf ignores aStartIndex parameter!");
  }

  nsresult rv;
  nsCOMPtr<nsIProperty> property = do_QueryInterface(aElement, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 index = mArray.IndexOf(property);
  NS_ENSURE_TRUE(index >= 0, NS_ERROR_NOT_AVAILABLE);

  *_retval = (PRUint32)index;
  return NS_OK;
}

/**
 * See nsIArray
 */
NS_IMETHODIMP
sbPropertyArray::Enumerate(nsISimpleEnumerator** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  return NS_NewArrayEnumerator(_retval, mArray);
}

/**
 * See nsIMutableArray
 */
NS_IMETHODIMP
sbPropertyArray::AppendElement(nsISupports* aElement,
                               PRBool aWeak)
{
  NS_ENSURE_ARG_POINTER(aElement);

  // No support for weak references here
  NS_ENSURE_FALSE(aWeak, NS_ERROR_FAILURE);

  nsresult rv;
  nsCOMPtr<nsIProperty> property = do_QueryInterface(aElement, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mArray.AppendObject(property);
  NS_ENSURE_STATE(success);

  return NS_OK;
}

/**
 * See nsIMutableArray
 */
NS_IMETHODIMP
sbPropertyArray::RemoveElementAt(PRUint32 aIndex)
{
  NS_ENSURE_ARG_MAX(aIndex, PR_MAX(0, mArray.Count() - 1));

  PRBool success = mArray.RemoveObjectAt(aIndex);
  NS_ENSURE_STATE(success);
  return NS_OK;
}

/**
 * See nsIMutableArray
 */
NS_IMETHODIMP
sbPropertyArray::InsertElementAt(nsISupports* aElement,
                                 PRUint32 aIndex,
                                 PRBool aWeak)
{
  NS_ENSURE_ARG_POINTER(aElement);
  NS_ENSURE_ARG_MAX(aIndex, mArray.Count());

  // No support for weak references here
  NS_ENSURE_FALSE(aWeak, NS_ERROR_FAILURE);

  nsresult rv;
  nsCOMPtr<nsIProperty> property = do_QueryInterface(aElement, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mArray.InsertObjectAt(property, aIndex);
  NS_ENSURE_STATE(success);

  return NS_OK;
}

/**
 * See nsIMutableArray
 */
NS_IMETHODIMP
sbPropertyArray::ReplaceElementAt(nsISupports* aElement,
                                  PRUint32 aIndex,
                                  PRBool aWeak)
{
  NS_ENSURE_ARG_POINTER(aElement);
  NS_ENSURE_ARG_MAX(aIndex, PR_MAX(0, mArray.Count() - 1));

  // No support for weak references here
  NS_ENSURE_FALSE(aWeak, NS_ERROR_NOT_IMPLEMENTED);

  nsresult rv;
  nsCOMPtr<nsIProperty> property = do_QueryInterface(aElement, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mArray.ReplaceObjectAt(property, aIndex);
  NS_ENSURE_STATE(success);

  return NS_OK;
}

/**
 * See nsIMutableArray
 */
NS_IMETHODIMP
sbPropertyArray::Clear()
{
  mArray.Clear();
  return NS_OK;
}

/**
 * See sbIPropertyArray
 */
NS_IMETHODIMP
sbPropertyArray::AppendProperty(const nsAString& aName,
                                const nsAString& aValue)
{
  NS_ENSURE_TRUE(!aName.IsEmpty(), NS_ERROR_INVALID_ARG);

  nsresult rv;
  nsCOMPtr<nsIWritableVariant> variant =
    do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = variant->SetAsAString(aValue);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIProperty> property = new sbSimpleProperty(aName, variant);
  NS_ENSURE_TRUE(property, NS_ERROR_OUT_OF_MEMORY);

  PRBool success = mArray.AppendObject(property);
  NS_ENSURE_STATE(success);

  return NS_OK;
}

/**
 * See sbIPropertyArray
 */
NS_IMETHODIMP
sbPropertyArray::GetPropertyAt(PRUint32 aIndex,
                               nsIProperty** _retval)
{
  NS_ENSURE_ARG_MAX(aIndex, PR_MAX(0, mArray.Count() - 1));
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<nsIProperty> property = mArray.ObjectAt(aIndex);
  NS_ENSURE_STATE(property);

  NS_ADDREF(*_retval = property);
  return NS_OK;
}
