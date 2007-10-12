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

#include "sbPropertyInfo.h"
#include "sbStandardOperators.h"

#include <nsISimpleEnumerator.h>

#include <nsArrayEnumerator.h>

#include <sbLockUtils.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbPropertyOperator, sbIPropertyOperator)

sbPropertyOperator::sbPropertyOperator()
: mLock(nsnull)
, mInitialized(PR_FALSE)
{
  mLock = PR_NewLock();
  NS_ASSERTION(mLock, "sbPropertyOperator::mLock failed to create lock!");
}

sbPropertyOperator::sbPropertyOperator(const nsAString& aOperator,
                                       const nsAString& aOperatorReadable)
: mLock(nsnull)
, mInitialized(PR_TRUE)
, mOperator(aOperator)
, mOperatorReadable(aOperatorReadable)
{
  mLock = PR_NewLock();
  NS_ASSERTION(mLock, "sbPropertyOperator::mLock failed to create lock!");
}

sbPropertyOperator::~sbPropertyOperator()
{
  if(mLock) {
    PR_DestroyLock(mLock);
  }
}

NS_IMETHODIMP sbPropertyOperator::GetOperator(nsAString & aOperator)
{
  sbSimpleAutoLock lock(mLock);
  aOperator = mOperator;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyOperator::GetOperatorReadable(nsAString & aOperatorReadable)
{
  sbSimpleAutoLock lock(mLock);
  aOperatorReadable = mOperatorReadable;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyOperator::Init(const nsAString & aOperator, const nsAString & aOperatorReadable)
{
  sbSimpleAutoLock lock(mLock);
  NS_ENSURE_TRUE(mInitialized == PR_FALSE, NS_ERROR_ALREADY_INITIALIZED);

  mOperator = aOperator;
  mOperatorReadable = aOperatorReadable;

  mInitialized = PR_TRUE;

  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(sbPropertyInfo, sbIPropertyInfo)

sbPropertyInfo::sbPropertyInfo()
: mNullSort(sbIPropertyInfo::SORT_NULL_SMALL)
, mSortProfileLock(nsnull)
, mNameLock(nsnull)
, mTypeLock(nsnull)
, mDisplayNameLock(nsnull)
, mUserViewableLock(nsnull)
, mUserViewable(PR_TRUE)
, mUserEditableLock(nsnull)
, mUserEditable(PR_TRUE)
, mUnitsLock(nsnull)
, mOperatorsLock(nsnull)
, mRemoteReadableLock(nsnull)
, mRemoteWritableLock(nsnull)
{
  mSortProfileLock = PR_NewLock();
  NS_ASSERTION(mSortProfileLock,
    "sbPropertyInfo::mSortProfileLock failed to create lock!");

  mNameLock = PR_NewLock();
  NS_ASSERTION(mNameLock,
    "sbPropertyInfo::mNameLock failed to create lock!");

  mTypeLock = PR_NewLock();
  NS_ASSERTION(mTypeLock,
    "sbPropertyInfo::mTypeLock failed to create lock!");

  mDisplayNameLock = PR_NewLock();
  NS_ASSERTION(mDisplayNameLock,
    "sbPropertyInfo::mDisplayNameLock failed to create lock!");

  mUserViewableLock = PR_NewLock();
  NS_ASSERTION(mUserViewableLock,
    "sbPropertyInfo::mUserViewableLock failed to create lock!");

  mUserEditableLock = PR_NewLock();
  NS_ASSERTION(mUserEditableLock,
    "sbPropertyInfo::mUserEditableLock failed to create lock!");

  mUnitsLock = PR_NewLock();
  NS_ASSERTION(mUnitsLock,
    "sbPropertyInfo::mUnitsLock failed to create lock!");

  mOperatorsLock = PR_NewLock();
  NS_ASSERTION(mOperatorsLock,
    "sbPropertyInfo::mOperatorsLock failed to create lock!");

  mRemoteReadableLock = PR_NewLock();
  NS_ASSERTION(mRemoteReadableLock,
    "sbPropertyInfo::mRemoteReadableLock failed to create lock!");

  mRemoteWritableLock = PR_NewLock();
  NS_ASSERTION(mRemoteWritableLock,
    "sbPropertyInfo::mRemoteWritableLock failed to create lock!");
}

sbPropertyInfo::~sbPropertyInfo()
{
  if(mSortProfileLock) {
    PR_DestroyLock(mSortProfileLock);
  }

  if(mNameLock) {
    PR_DestroyLock(mNameLock);
  }

  if(mTypeLock) {
    PR_DestroyLock(mTypeLock);
  }

  if(mDisplayNameLock) {
    PR_DestroyLock(mDisplayNameLock);
  }

  if(mUserViewableLock) {
    PR_DestroyLock(mUserViewableLock);
  }

  if(mUserEditableLock) {
    PR_DestroyLock(mUserEditableLock);
  }

  if(mUnitsLock) {
    PR_DestroyLock(mUnitsLock);
  }

  if(mOperatorsLock) {
    PR_DestroyLock(mOperatorsLock);
  }

  if(mRemoteReadableLock) {
    PR_DestroyLock(mRemoteReadableLock);
  }

  if(mRemoteWritableLock) {
    PR_DestroyLock(mRemoteWritableLock);
  }
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_EQUALS(nsAString & aOPERATOR_EQUALS)
{
  aOPERATOR_EQUALS = NS_LITERAL_STRING(SB_OPERATOR_EQUALS);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_NOTEQUALS(nsAString & aOPERATOR_NOTEQUALS)
{
  aOPERATOR_NOTEQUALS = NS_LITERAL_STRING(SB_OPERATOR_NOTEQUALS);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_GREATER(nsAString & aOPERATOR_GREATER)
{
  aOPERATOR_GREATER = NS_LITERAL_STRING(SB_OPERATOR_GREATER);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_GREATEREQUAL(nsAString & aOPERATOR_GREATEREQUAL)
{
  aOPERATOR_GREATEREQUAL = NS_LITERAL_STRING(SB_OPERATOR_GREATEREQUAL);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_LESS(nsAString & aOPERATOR_LESS)
{
  aOPERATOR_LESS = NS_LITERAL_STRING(SB_OPERATOR_LESS);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_LESSEQUAL(nsAString & aOPERATOR_LESSEQUAL)
{
  aOPERATOR_LESSEQUAL = NS_LITERAL_STRING(SB_OPERATOR_LESSEQUAL);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_CONTAINS(nsAString & aOPERATOR_CONTAINS)
{
  aOPERATOR_CONTAINS = NS_LITERAL_STRING(SB_OPERATOR_CONTAINS);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_BEGINSWITH(nsAString & aOPERATOR_BEGINSWITH)
{
  aOPERATOR_BEGINSWITH = NS_LITERAL_STRING(SB_OPERATOR_BEGINSWITH);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_ENDSWITH(nsAString & aOPERATOR_ENDSWITH)
{
  aOPERATOR_ENDSWITH = NS_LITERAL_STRING(SB_OPERATOR_ENDSWITH);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_BETWEEN(nsAString & aOPERATOR_BETWEEN)
{
  aOPERATOR_BETWEEN = NS_LITERAL_STRING(SB_OPERATOR_BETWEEN);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::SetNullSort(PRUint32 aNullSort)
{
  mNullSort = aNullSort;
  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::GetNullSort(PRUint32 *aNullSort)
{
  NS_ENSURE_ARG_POINTER(aNullSort);
  *aNullSort = mNullSort;
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::SetSortProfile(sbIPropertyArray * aSortProfile)
{
  NS_ENSURE_ARG_POINTER(aSortProfile);

  sbSimpleAutoLock lock(mSortProfileLock);
  mSortProfile = aSortProfile;

  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::GetSortProfile(sbIPropertyArray * *aSortProfile)
{
  NS_ENSURE_ARG_POINTER(aSortProfile);

  sbSimpleAutoLock lock(mSortProfileLock);
  *aSortProfile = mSortProfile;
  NS_IF_ADDREF(*aSortProfile);

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetName(nsAString & aName)
{
  sbSimpleAutoLock lock(mNameLock);
  aName = mName;
  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::SetName(const nsAString &aName)
{
  sbSimpleAutoLock lock(mNameLock);

  if(mName.IsEmpty()) {
    mName = aName;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbPropertyInfo::GetType(nsAString & aType)
{
  sbSimpleAutoLock lock(mTypeLock);
  aType = mType;
  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::SetType(const nsAString &aType)
{
  sbSimpleAutoLock lock(mTypeLock);

  if(mType.IsEmpty()) {
    mType = aType;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbPropertyInfo::GetDisplayName(nsAString & aDisplayName)
{
  sbSimpleAutoLock lock(mDisplayNameLock);

  if(mDisplayName.IsEmpty()) {
    sbSimpleAutoLock lock(mNameLock);
    aDisplayName = mName;
  }
  else {
    aDisplayName = mDisplayName;
  }

  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::SetDisplayName(const nsAString &aDisplayName)
{
  sbSimpleAutoLock lock(mDisplayNameLock);

  if(mDisplayName.IsEmpty()) {
    mDisplayName = aDisplayName;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbPropertyInfo::GetUserViewable(PRBool *aUserViewable)
{
  NS_ENSURE_ARG_POINTER(aUserViewable);

  sbSimpleAutoLock lock(mUserViewableLock);
  *aUserViewable = mUserViewable;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::SetUserViewable(PRBool aUserViewable)
{
  sbSimpleAutoLock lock(mUserViewableLock);
  mUserViewable = aUserViewable;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetUserEditable(PRBool *aUserEditable)
{
  NS_ENSURE_ARG_POINTER(aUserEditable);

  sbSimpleAutoLock lock(mUserEditableLock);
  *aUserEditable = mUserEditable;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::SetUserEditable(PRBool aUserEditable)
{
  sbSimpleAutoLock lock(mUserEditableLock);
  mUserEditable = aUserEditable;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetUnits(nsAString & aUnits)
{
  sbSimpleAutoLock lock(mUnitsLock);
  aUnits = mUnits;
  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::SetUnits(const nsAString & aUnits)
{
  sbSimpleAutoLock lock(mUnitsLock);

  if(mUnits.IsEmpty()) {
    mUnits = aUnits;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbPropertyInfo::GetOperators(nsISimpleEnumerator * *aOperators)
{
  NS_ENSURE_ARG_POINTER(aOperators);

  sbSimpleAutoLock lock(mOperatorsLock);
  return NS_NewArrayEnumerator(aOperators, mOperators);
}
NS_IMETHODIMP sbPropertyInfo::SetOperators(nsISimpleEnumerator * aOperators)
{
  NS_ENSURE_ARG_POINTER(aOperators);

  sbSimpleAutoLock lock(mOperatorsLock);
  mOperators.Clear();

  PRBool hasMore = PR_FALSE;
  nsCOMPtr<nsISupports> object;

  while( NS_SUCCEEDED(aOperators->HasMoreElements(&hasMore)) &&
         hasMore  &&
         NS_SUCCEEDED(aOperators->GetNext(getter_AddRefs(object)))) {
    nsresult rv;
    nsCOMPtr<sbIPropertyOperator> po = do_QueryInterface(object, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool success = mOperators.AppendObject(po);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOperator(const nsAString & aOperator,
                                          sbIPropertyOperator * *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  PRUint32 length = mOperators.Count();
  for (PRUint32 i = 0; i < length; i++) {
    nsAutoString op;
    nsresult rv = mOperators[i]->GetOperator(op);
    NS_ENSURE_SUCCESS(rv, rv);
    if (op.Equals(aOperator)) {
      NS_ADDREF(*_retval = mOperators[i]);
      return NS_OK;
    }
  }

  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::Validate(const nsAString & aValue, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbPropertyInfo::Sanitize(const nsAString & aValue, nsAString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbPropertyInfo::Format(const nsAString & aValue, nsAString & _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbPropertyInfo::MakeSortable(const nsAString & aValue, nsAString & _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbPropertyInfo::GetRemoteReadable(PRBool *aRemoteReadable)
{
  NS_ENSURE_ARG_POINTER(aRemoteReadable);

  sbSimpleAutoLock lock(mRemoteReadableLock);
  *aRemoteReadable = mRemoteReadable;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::SetRemoteReadable(PRBool aRemoteReadable)
{
  sbSimpleAutoLock lock(mRemoteReadableLock);
  mRemoteReadable = aRemoteReadable;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetRemoteWritable(PRBool *aRemoteWritable)
{
  NS_ENSURE_ARG_POINTER(aRemoteWritable);

  sbSimpleAutoLock lock(mRemoteWritableLock);
  *aRemoteWritable = mRemoteWritable;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::SetRemoteWritable(PRBool aRemoteWritable)
{
  sbSimpleAutoLock lock(mRemoteWritableLock);
  mRemoteWritable = aRemoteWritable;

  return NS_OK;
}
