/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef _SB_JOB_UTILS_H_
#define _SB_JOB_UTILS_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird job utility defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbJobUtils.h
 * \brief Songbird Job Utility Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird job imported services.
//
//------------------------------------------------------------------------------

// Mozilla interface imports.
#include <nsISimpleEnumerator.h>
#include <nsIStringEnumerator.h>
#include <nsISupportsPrimitives.h>

// Songbird imports.
#include <sbMemoryUtils.h>


//------------------------------------------------------------------------------
//
// Songbird job services.
//
//------------------------------------------------------------------------------

//
// sbAutoJobCancel              Wrapper to auto-cancel a job.
//

SB_AUTO_NULL_CLASS(sbAutoJobCancel, sbIJobCancelable*, mValue->Cancel());

//
// sbJobErrorEnumerator         Wrapper class for job errors
//
template <class Interface>
class sbJobErrorEnumerator : public nsIStringEnumerator,
                             public nsISimpleEnumerator
{
public:
  typedef nsTArray<nsCOMPtr<Interface> > ArrayType;
  // default constructor
  explicit sbJobErrorEnumerator() : mIndex(0){}
  // construct an enumerator from a copy of an array
  sbJobErrorEnumerator(ArrayType& aArray)
    : mArray(aArray), mIndex(0) {}

  /**
   * Adopt the elements from another array; this will clear out the contents
   * of the other array.
   * @param aArray
   * @return
   */
  PRBool Adopt(ArrayType& aArray)
  {
    mIndex = 0;
    mArray.Clear();
    return mArray.SwapElements(aArray);
  }

  ///// nsIStringEnumerator
  NS_IMETHODIMP HasMore(PRBool *_retval)
  {
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = mIndex < mArray.Length();
    return NS_OK;
  }

  NS_IMETHODIMP GetNext(nsAString & _retval)
  {
    nsresult rv;

    if (!(mIndex < mArray.Length())) {
      return NS_ERROR_FAILURE;
    }
    nsCOMPtr<Interface> next(mArray[mIndex]);
    NS_ENSURE_TRUE(next, NS_ERROR_FAILURE);

    nsCOMPtr<nsISupportsString> string = do_QueryInterface(next, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = string->GetData(_retval);
    NS_ENSURE_SUCCESS(rv, rv);

    ++mIndex;
    return NS_OK;
  }

  ///// nsISimpleEnumerator
  NS_IMETHODIMP HasMoreElements(PRBool *_retval)
  {
    return HasMore(_retval);
  }
  NS_IMETHODIMP GetNext(nsISupports** _retval)
  {
    nsresult rv;

    if (!(mIndex < mArray.Length())) {
      return NS_ERROR_FAILURE;
    }
    nsCOMPtr<Interface> next(mArray[mIndex]);
    NS_ENSURE_TRUE(next, NS_ERROR_FAILURE);

    rv = CallQueryInterface(next, _retval);
    NS_ENSURE_SUCCESS(rv, rv);

    ++mIndex;
    return NS_OK;
  }

  ///// nsISupports
  NS_IMETHODIMP_(nsrefcnt) AddRef(void)
  {
    NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "illegal refcnt");
    nsrefcnt count;
    count = PR_AtomicIncrement((PRInt32*)&mRefCnt);
    NS_LOG_ADDREF(this, count, "sbJobErrorEnumerator<>", sizeof(*this));
    return count;
  }

  NS_IMETHODIMP_(nsrefcnt) Release(void)
  {
    nsrefcnt count;
    NS_PRECONDITION(0 != mRefCnt, "dup release");
    count = PR_AtomicDecrement((PRInt32 *)&mRefCnt);
    NS_LOG_RELEASE(this, count, "sbJobErrorEnumerator<>");
    if (0 == count) {
      mRefCnt = 1; /* stabilize */
      NS_DELETEXPCOM(this);
    }
    return count;
  }

  NS_IMETHODIMP QueryInterface(REFNSIID aIID, void** aInstancePtr)
  {
    NS_ASSERTION(aInstancePtr,
                 "QueryInterface requires a non-NULL destination!");
    nsresult rv = NS_ERROR_FAILURE;
    NS_INTERFACE_TABLE2(sbJobErrorEnumerator<Interface>,
                        nsISimpleEnumerator,
                        nsIStringEnumerator)
    NS_INTERFACE_TABLE_TAIL

protected:
  nsAutoRefCnt mRefCnt;
  ArrayType mArray;
  PRUint32 mIndex;
};

#endif /* _SB_JOB_UTILS_H_ */

