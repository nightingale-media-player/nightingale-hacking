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

#include "sbTextPropertyInfo.h"

#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsUnicharUtils.h>
#include <nsMemory.h>

NS_IMPL_ADDREF_INHERITED(sbTextPropertyInfo, sbPropertyInfo);
NS_IMPL_RELEASE_INHERITED(sbTextPropertyInfo, sbPropertyInfo);

NS_INTERFACE_TABLE_HEAD(sbTextPropertyInfo)
NS_INTERFACE_TABLE_BEGIN
NS_INTERFACE_TABLE_ENTRY(sbTextPropertyInfo, sbITextPropertyInfo)
NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(sbTextPropertyInfo, sbIPropertyInfo, sbITextPropertyInfo)
NS_INTERFACE_TABLE_ENTRY(sbTextPropertyInfo, sbIRemotePropertyInfo)
NS_INTERFACE_TABLE_END
NS_INTERFACE_TABLE_TAIL_INHERITING(sbPropertyInfo)

sbTextPropertyInfo::sbTextPropertyInfo()
: mMinMaxLock(nsnull)
, mMinLen(0)
, mMaxLen(0)
, mEnforceLowercaseLock(nsnull)
, mEnforceLowercase(PR_FALSE)
{
  mType = NS_LITERAL_STRING("text");

  mMinMaxLock = PR_NewLock();
  NS_ASSERTION(mMinMaxLock, 
    "sbTextPropertyInfo::mMinMaxLock failed to create lock!");

  mEnforceLowercaseLock = PR_NewLock();
  NS_ASSERTION(mEnforceLowercaseLock, 
    "sbTextPropertyInfo::mMinMaxLock failed to create lock!");

  InitializeOperators();
}

sbTextPropertyInfo::~sbTextPropertyInfo()
{
  if(mMinMaxLock) {
    PR_DestroyLock(mMinMaxLock);
  }

  if(mEnforceLowercaseLock) {
    PR_DestroyLock(mEnforceLowercaseLock);
  }
}

void sbTextPropertyInfo::InitializeOperators()
{
  nsAutoString op;
  nsRefPtr<sbPropertyOperator> propOp;

  sbPropertyInfo::GetOPERATOR_CONTAINS(op);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.contains"));
  mOperators.AppendObject(propOp);

  sbPropertyInfo::GetOPERATOR_BEGINSWITH(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.starts"));
  mOperators.AppendObject(propOp);

  sbPropertyInfo::GetOPERATOR_ENDSWITH(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.ends"));
  mOperators.AppendObject(propOp);

  sbPropertyInfo::GetOPERATOR_EQUALS(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.is"));
  mOperators.AppendObject(propOp);

  sbPropertyInfo::GetOPERATOR_NOTEQUALS(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.is_not"));
  mOperators.AppendObject(propOp);

  return;
}

NS_IMETHODIMP sbTextPropertyInfo::Validate(const nsAString & aValue, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  
  PRInt32 len = aValue.Length();
  nsAutoLock lock(mMinMaxLock);

  *_retval = PR_TRUE;

  if(mMinLen && len < mMinLen) {
    *_retval = PR_FALSE;
  }

  if(mMaxLen && len > mMaxLen) {
    *_retval = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP sbTextPropertyInfo::Sanitize(const nsAString & aValue, nsAString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbTextPropertyInfo::Format(const nsAString & aValue, nsAString & _retval)
{
  nsresult rv;
  PRBool valid = PR_FALSE;

  _retval = aValue;

  CompressWhitespace(_retval);
  PRInt32 len = aValue.Length();

  {
    nsAutoLock lock(mMinMaxLock);

    //If a minimum length is specified and is not respected there's nothing
    //we can do about it, so we reject it.
    if(mMinLen && len < mMinLen) {
      _retval = EmptyString();
      return NS_ERROR_INVALID_ARG;
    }

    //If a maximum length is specified and we exceed it, we cut the string to the
    //maximum length.
    if(mMaxLen && len > mMaxLen) {
      _retval.SetLength(mMaxLen);
    }
  }

  //Enforce lowercase if that is requested.
  {
    nsAutoLock lock(mEnforceLowercaseLock);
    if(mEnforceLowercase) {
      ToLowerCase(_retval);
    }
  }

  rv = Validate(_retval, &valid);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if(!valid) {
    rv = NS_ERROR_FAILURE;
    _retval = EmptyString();
  }

  return rv;
}

NS_IMETHODIMP sbTextPropertyInfo::MakeSortable(const nsAString & aValue, nsAString & _retval)
{
  nsresult rv;
  PRBool valid = PR_FALSE;

  _retval = aValue;
  
  CompressWhitespace(_retval);
  ToLowerCase(_retval);

  PRInt32 len = aValue.Length();
  
  PR_Lock(mMinMaxLock);

  //If a minimum length is specified and is not respected there's nothing
  //we can do about it, so we reject it.
  if(mMinLen && len < mMinLen) {
    PR_Unlock(mMinMaxLock);
    _retval = EmptyString();
    return NS_ERROR_INVALID_ARG;
  }

  //If a maximum length is specified and we exceed it, we cut the string to the
  //maximum length.
  if(mMaxLen && len > mMaxLen) {
    _retval.SetLength(mMaxLen);
  }

  PR_Unlock(mMinMaxLock);

  rv = Validate(_retval, &valid);
  if(!valid) {
    rv = NS_ERROR_FAILURE;
    _retval = EmptyString();
  }

  return rv;
}

NS_IMETHODIMP sbTextPropertyInfo::GetMinLength(PRUint32 *aMinLength)
{
  NS_ENSURE_ARG_POINTER(aMinLength);
  nsAutoLock lock(mMinMaxLock);
  *aMinLength = mMinLen;
  return NS_OK;
}
NS_IMETHODIMP sbTextPropertyInfo::SetMinLength(PRUint32 aMinLength)
{
  nsAutoLock lock(mMinMaxLock);
  mMinLen = aMinLength;
  return NS_OK;
}

NS_IMETHODIMP sbTextPropertyInfo::GetMaxLength(PRUint32 *aMaxLength)
{
  NS_ENSURE_ARG_POINTER(aMaxLength);
  nsAutoLock lock(mMinMaxLock);
  *aMaxLength = mMaxLen;
  return NS_OK;
}
NS_IMETHODIMP sbTextPropertyInfo::SetMaxLength(PRUint32 aMaxLength)
{
  nsAutoLock lock(mMinMaxLock);
  mMaxLen = aMaxLength;
  return NS_OK;
}

NS_IMETHODIMP sbTextPropertyInfo::GetEnforceLowercase(PRBool *aEnforceLowercase)
{
  NS_ENSURE_ARG_POINTER(aEnforceLowercase);
  nsAutoLock lock(mEnforceLowercaseLock);
  *aEnforceLowercase = mEnforceLowercase;
  return NS_OK;
}
NS_IMETHODIMP sbTextPropertyInfo::SetEnforceLowercase(PRBool aEnforceLowercase)
{
  nsAutoLock lock(mEnforceLowercaseLock);
  mEnforceLowercase = aEnforceLowercase;
  return NS_OK;
}
