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
#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsIProgrammingLanguage.h>
#include <nsISimpleEnumerator.h>
#include <nsMemory.h>

#include "sbSimpleProperty.h"

NS_IMPL_THREADSAFE_ADDREF(sbPropertyArray)
NS_IMPL_THREADSAFE_RELEASE(sbPropertyArray)

NS_INTERFACE_MAP_BEGIN(sbPropertyArray)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIMutableArray)
  NS_INTERFACE_MAP_ENTRY(sbIPropertyArray)
  NS_INTERFACE_MAP_ENTRY(sbIMutablePropertyArray)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIArray, nsIMutableArray)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY(nsIMutableArray)
NS_INTERFACE_MAP_END

NS_IMPL_CI_INTERFACE_GETTER5(sbPropertyArray, nsIArray,
                                              nsIMutableArray,
                                              sbIPropertyArray,
                                              sbIMutablePropertyArray,
                                              nsIClassInfo)
sbPropertyArray::sbPropertyArray()
{
  MOZ_COUNT_CTOR(sbPropertyArray);
}

sbPropertyArray::~sbPropertyArray()
{
  MOZ_COUNT_DTOR(sbPropertyArray);
  if(mArrayLock) {
    nsAutoLock::DestroyLock(mArrayLock);
  }
}

nsresult
sbPropertyArray::Init()
{
  mArrayLock = nsAutoLock::NewLock("sbPropertyArray::mArrayLock");
  NS_ENSURE_TRUE(mArrayLock, NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

/**
 * See nsIArray
 */
NS_IMETHODIMP
sbPropertyArray::GetLength(PRUint32* aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);

  nsAutoLock lock(mArrayLock);
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

  nsAutoLock lock(mArrayLock);
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
  nsCOMPtr<sbIProperty> property = do_QueryInterface(aElement, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoLock lock(mArrayLock);
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

  nsresult rv;
  nsCOMPtr<nsIMutableArray> array =
    do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

  nsAutoLock lock(mArrayLock);
  PRUint32 length = mArray.Count();
  for (PRUint32 i = 0; i < length; i++) {
    rv = array->AppendElement(mArray.ObjectAt(i), PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_NewArrayEnumerator(_retval, array);
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
  nsCOMPtr<sbIProperty> property = do_QueryInterface(aElement, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoLock lock(mArrayLock);
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

  nsAutoLock lock(mArrayLock);
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
  nsCOMPtr<sbIProperty> property = do_QueryInterface(aElement, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoLock lock(mArrayLock);
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
  nsCOMPtr<sbIProperty> property = do_QueryInterface(aElement, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoLock lock(mArrayLock);
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
  nsAutoLock lock(mArrayLock);
  mArray.Clear();
  return NS_OK;
}

/**
 * See sbIMutablePropertyArray
 */
NS_IMETHODIMP
sbPropertyArray::AppendProperty(const nsAString& aName,
                                const nsAString& aValue)
{
  NS_ENSURE_TRUE(!aName.IsEmpty(), NS_ERROR_INVALID_ARG);

  nsCOMPtr<sbIProperty> property = new sbSimpleProperty(aName, aValue);
  NS_ENSURE_TRUE(property, NS_ERROR_OUT_OF_MEMORY);

  nsAutoLock lock(mArrayLock);
  PRBool success = mArray.AppendObject(property);
  NS_ENSURE_STATE(success);

  return NS_OK;
}

/**
 * See sbIPropertyArray
 */
NS_IMETHODIMP
sbPropertyArray::GetPropertyAt(PRUint32 aIndex,
                               sbIProperty** _retval)
{
  NS_ENSURE_ARG_MAX(aIndex, PR_MAX(0, mArray.Count() - 1));
  NS_ENSURE_ARG_POINTER(_retval);

  nsAutoLock lock(mArrayLock);
  nsCOMPtr<sbIProperty> property = mArray.ObjectAt(aIndex);
  NS_ENSURE_STATE(property);

  NS_ADDREF(*_retval = property);
  return NS_OK;
}

/**
 * See sbIPropertyArray
 */
NS_IMETHODIMP
sbPropertyArray::GetPropertyValue(const nsAString& aName,
                                  nsAString& _retval)
{
  nsresult rv;

  nsAutoLock lock(mArrayLock);
  PRUint32 length = mArray.Count();
  for (PRUint32 i = 0; i < length; i++) {
    nsCOMPtr<sbIProperty> property = mArray.ObjectAt(i);
    NS_ENSURE_STATE(property);

    nsAutoString propertyName;
    rv = property->GetName(propertyName);
    NS_ENSURE_SUCCESS(rv, rv);

    if (propertyName.Equals(aName)) {
      rv = property->GetValue(_retval);
      NS_ENSURE_SUCCESS(rv, rv);
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
sbPropertyArray::ToString(nsAString& _retval)
{
  nsresult rv;

  nsAutoLock lock(mArrayLock);

  nsAutoString buff;
  buff.AssignLiteral("[");

  PRUint32 length = mArray.Count();
  for (PRUint32 i = 0; i < length; i++) {
    nsCOMPtr<sbIProperty> property = mArray.ObjectAt(i);
    NS_ENSURE_STATE(property);

    nsAutoString propertyName;
    rv = property->GetName(propertyName);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString value;
    rv = property->GetValue(value);
    NS_ENSURE_SUCCESS(rv, rv);

    buff.AppendLiteral("'");
    buff.Append(propertyName);
    buff.AppendLiteral("' => ");

    buff.AppendLiteral("'");
    buff.Append(value);
    buff.AppendLiteral("'");

    if (i + 1 < length) {
      buff.AppendLiteral(", ");
    }
  }

  buff.AppendLiteral("]");
  _retval = buff;

  return NS_OK;
}

// nsIClassInfo
NS_IMETHODIMP
sbPropertyArray::GetInterfaces(PRUint32* count, nsIID*** array)
{
  return NS_CI_INTERFACE_GETTER_NAME(sbPropertyArray)(count, array);
}

NS_IMETHODIMP
sbPropertyArray::GetHelperForLanguage(PRUint32 language,
                                      nsISupports** _retval)
{
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbPropertyArray::GetContractID(char** aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbPropertyArray::GetClassDescription(char** aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbPropertyArray::GetClassID(nsCID** aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbPropertyArray::GetImplementationLanguage(PRUint32* aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
sbPropertyArray::GetFlags(PRUint32 *aFlags)
{
  *aFlags = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbPropertyArray::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  return NS_ERROR_NOT_AVAILABLE;
}

