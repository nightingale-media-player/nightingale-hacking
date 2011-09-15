/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
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
 *=END SONGBIRD GPL
 */

#ifndef SBARRAYUTILS_H_
#define SBARRAYUTILS_H_

// Standard includes
#include <algorithm>

// Mozilla includes
#include <nsCOMArray.h>
#include <nsComponentManagerUtils.h>

// Mozilla interfaces
#include <nsIMutableArray.h>
#include <nsISimpleEnumerator.h>

#ifndef SB_THREADSAFE_ARRAY_CONTRACTID
#define SB_THREADSAFE_ARRAY_CONTRACTID \
  "@songbirdnest.com/moz/xpcom/threadsafe-array;1"
#endif /* SB_THREADSAFE_ARRAY_CONTRACTID */

template <class T>
nsresult sbCOMArrayTonsIArray(T & aCOMArray, nsIArray ** aOutArray)
{
  nsresult rv;
  nsCOMPtr<nsIMutableArray> outArray =
    do_CreateInstance(SB_THREADSAFE_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 const length = aCOMArray.Count();
  for (PRInt32 index = 0; index < length; ++index) {
    rv = outArray->AppendElement(aCOMArray.ObjectAt(index), PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = CallQueryInterface(outArray, aOutArray);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

template <class T>
nsresult sbAppendnsCOMArray(T const & aSource,
                            T & aDest,
                            PRUint32 aElementsToCopy = 0)
{
  PRUint32 length = aSource.Count();
  if (aElementsToCopy) {
    length = std::min(length, aElementsToCopy);
  }
  for (PRUint32 index = 0; index < length; ++index) {
    NS_ENSURE_TRUE(aDest.AppendObject(aSource[index]), NS_ERROR_OUT_OF_MEMORY);
  }
  return NS_OK;
}
/**
 * Clones a nsIArray into a new one
 * @param aSrc [in] the array to copy from
 * @param aDest [in|out] the array to append to
 * @param aWeak [in] Whether to store as a weak reference
 * @param aElementsToCopy [in] Allows partial appending of source. 0 means copy
 *                        everything
 */
inline nsresult
sbAppendnsIArray(nsIArray * aSrc,
                 nsIMutableArray * aDest,
                 PRBool aWeak = PR_FALSE,
                 PRUint32 aElementsToCopy = 0)
{
  nsresult rv;

  PRUint32 elementsToCopy = aElementsToCopy;
  if (elementsToCopy == 0) {
    rv = aSrc->GetLength(&elementsToCopy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsISimpleEnumerator> it;
  rv = aSrc->Enumerate(getter_AddRefs(it));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore;
  while (NS_SUCCEEDED(it->HasMoreElements(&hasMore)) &&
         hasMore &&
         elementsToCopy--) {
    nsCOMPtr<nsISupports> supports;
    rv = it->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aDest->AppendElement(supports, aWeak);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
/**
 * Clones a nsIArray into a new one
 * @param aSrc [in] the array to clone
 * @param aClonedArray [out] the newly created array
 * @param aWeak [in] Whether to store as a weak reference
 */
inline nsresult
sbClonensIArray(nsIArray * aSrc, nsIArray ** aClonedArray, PRBool aWeak = PR_FALSE)
{
  NS_ENSURE_ARG_POINTER(aSrc);
  NS_ENSURE_ARG_POINTER(aClonedArray);

  nsresult rv;

  nsCOMPtr<nsIMutableArray> array =
    do_CreateInstance(SB_THREADSAFE_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbAppendnsIArray(aSrc, array, aWeak);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CallQueryInterface(array, aClonedArray);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * Clones a nsIArray into a new nsCOMArray
 * @param aSrc [in] the array to clone
 * @param aDest [inout] the destination array
 * @throws NS_ERROR_NO_INTERFACE if *any* element of the source array does not
 *         QI into the desired interface.
 *         Note that this will modify the destination into an inconsistent state
 */
template<class T>
nsresult
sbClonensIArray(nsIArray * aSrc, nsCOMArray<T> & aDest)
{
  NS_ENSURE_ARG_POINTER(aSrc);
  nsresult rv;
  nsCOMPtr<nsISimpleEnumerator> it;
  rv = aSrc->Enumerate(getter_AddRefs(it));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> supports;
  PRBool hasMore;
  while (NS_SUCCEEDED(rv = it->HasMoreElements(&hasMore)) && hasMore) {
    rv = it->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<T> ptr = do_QueryInterface(supports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool success = aDest.AppendObject(ptr);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

#endif /* SBARRAYUTILS_H_ */
