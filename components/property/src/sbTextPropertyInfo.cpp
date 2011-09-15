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

#include "sbTextPropertyInfo.h"

#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsUnicharUtils.h>
#include <nsMemory.h>

#include <sbIStringTransform.h>

#include <sbLockUtils.h>
#include <sbStringUtils.h>

#include <locale>     // for collation
#include <prmem.h>
#include <errno.h>

NS_IMPL_ADDREF_INHERITED(sbTextPropertyInfo, sbPropertyInfo);
NS_IMPL_RELEASE_INHERITED(sbTextPropertyInfo, sbPropertyInfo);

NS_INTERFACE_TABLE_HEAD(sbTextPropertyInfo)
NS_INTERFACE_TABLE_BEGIN
NS_INTERFACE_TABLE_ENTRY(sbTextPropertyInfo, sbITextPropertyInfo)
NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(sbTextPropertyInfo, sbIPropertyInfo, sbITextPropertyInfo)
NS_INTERFACE_TABLE_END
NS_INTERFACE_TABLE_TAIL_INHERITING(sbPropertyInfo)

sbTextPropertyInfo::sbTextPropertyInfo()
: mMinMaxLock(nsnull)
, mMinLen(0)
, mMaxLen(0)
, mEnforceLowercaseLock(nsnull)
, mEnforceLowercase(PR_FALSE)
, mNoCompressWhitespaceLock(nsnull)
, mNoCompressWhitespace(PR_FALSE)
{
  mType = NS_LITERAL_STRING("text");

  mMinMaxLock = PR_NewLock();
  NS_ASSERTION(mMinMaxLock,
    "sbTextPropertyInfo::mMinMaxLock failed to create lock!");

  mEnforceLowercaseLock = PR_NewLock();
  NS_ASSERTION(mEnforceLowercaseLock,
    "sbTextPropertyInfo::mEnforceLowercaseLock failed to create lock!");

  mNoCompressWhitespaceLock = PR_NewLock();
  NS_ASSERTION(mNoCompressWhitespaceLock,
    "sbTextPropertyInfo::mNoCompressWhitespaceLock failed to create lock!");
}

sbTextPropertyInfo::~sbTextPropertyInfo()
{
  if(mMinMaxLock) {
    PR_DestroyLock(mMinMaxLock);
  }

  if(mEnforceLowercaseLock) {
    PR_DestroyLock(mEnforceLowercaseLock);
  }

  if(mNoCompressWhitespaceLock) {
    PR_DestroyLock(mNoCompressWhitespaceLock);
  }
}

nsresult
sbTextPropertyInfo::Init()
{
  nsresult rv;

  rv = sbPropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitializeOperators();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbTextPropertyInfo::InitializeOperators()
{
  nsresult rv;
  nsAutoString op;
  nsRefPtr<sbPropertyOperator> propOp;

  rv = sbPropertyInfo::GetOPERATOR_CONTAINS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.contains"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_NOTCONTAINS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.not_contain"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_EQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.is"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_NOTEQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.is_not"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_BEGINSWITH(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.starts"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_NOTBEGINSWITH(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.not_start"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_ENDSWITH(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.ends"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_NOTENDSWITH(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.not_end"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP sbTextPropertyInfo::Validate(const nsAString & aValue, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  PRUint32 len = aValue.Length();
  sbSimpleAutoLock lock(mMinMaxLock);

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
  PRBool isTrim = PR_FALSE;

  _retval = aValue;

  //Don't compress/strip the leading whitespace if requested
  {
    sbSimpleAutoLock lock(mNoCompressWhitespaceLock);
    if (!mNoCompressWhitespace) {
      isTrim = PR_TRUE;
    }
  }
  SB_CompressWhitespace(_retval, isTrim, PR_TRUE);

  PRUint32 len = aValue.Length();

  {
    sbSimpleAutoLock lock(mMinMaxLock);

    // If a minimum length is specified and is not respected there's nothing
    // we can do about it, so we reject it.
    if(mMinLen && len < mMinLen) {
      _retval = EmptyString();
      return NS_ERROR_INVALID_ARG;
    }

    // If a maximum length is specified and we exceed it, we cut the string to the
    // maximum length.
    if(mMaxLen && len > mMaxLen) {
      _retval.SetLength(mMaxLen);
    }
  }

  // Enforce lowercase if that is requested.
  {
    sbSimpleAutoLock lock(mEnforceLowercaseLock);
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

// text property info needs to compute local-specific collation data instead
// of relying on the ancestor's default implementation (which just calls
// MakeSearchable), so that proper sort order are achieved.
NS_IMETHODIMP sbTextPropertyInfo::MakeSortable(const nsAString & aValue, nsAString & _retval)
{
  // first compress whitespaces
  nsAutoString val;
  val = aValue;
  CompressWhitespace(val);
  ToLowerCase(val);

  nsresult rv;

  nsCOMPtr<sbIStringTransform> stringTransform =
    do_CreateInstance(SB_STRINGTRANSFORM_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString outVal;

  // lone> note that if we ever decide to remove the non-alphanum chars from
  // more than the leading part of the string, we must still not remove those
  // chars from the entire string. Ie, we can only extend the filtering to the
  // trailing part of the string, but not the whole string. the reason for this
  // is that we do not want to remove the "," in "Beatles, The", or it will not
  // be recognized by the articles removal code since the pattern is "*, The".
  rv = stringTransform->
         NormalizeString(EmptyString(),
                         sbIStringTransform::TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                         sbIStringTransform::TRANSFORM_IGNORE_LEADING |
                         sbIStringTransform::TRANSFORM_IGNORE_KEEPNUMBERSYMBOLS,
                         val,
                         outVal);
  NS_ENSURE_SUCCESS(rv, rv);

  // don't allow normalization to produce an empty sortable string
  if (!outVal.IsEmpty()) {
    val = outVal;
  }

  rv = stringTransform->RemoveArticles(val, EmptyString(), outVal);
  NS_ENSURE_SUCCESS(rv, rv);

  _retval = outVal;

  // all done
  return NS_OK;
}

NS_IMETHODIMP sbTextPropertyInfo::MakeSearchable(const nsAString & aValue, nsAString & _retval)
{
  nsresult rv;
  PRBool valid = PR_FALSE;

  _retval = aValue;
  CompressWhitespace(_retval);
  ToLowerCase(_retval);

  nsCOMPtr<sbIStringTransform> stringTransform =
    do_CreateInstance(SB_STRINGTRANSFORM_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString outVal;
  rv = stringTransform->NormalizeString(EmptyString(),
                                        sbIStringTransform::TRANSFORM_IGNORE_NONSPACE,
                                        _retval, outVal);
  NS_ENSURE_SUCCESS(rv, rv);

  _retval = outVal;

  PRUint32 len = aValue.Length();

  PR_Lock(mMinMaxLock);

  // If a minimum length is specified and is not respected there's nothing
  // we can do about it, so we reject it.
  if(mMinLen && len < mMinLen) {
    PR_Unlock(mMinMaxLock);
    _retval = EmptyString();
    return NS_ERROR_INVALID_ARG;
  }

  // If a maximum length is specified and we exceed it, we cut the string to the
  // maximum length.
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
  sbSimpleAutoLock lock(mMinMaxLock);
  *aMinLength = mMinLen;
  return NS_OK;
}
NS_IMETHODIMP sbTextPropertyInfo::SetMinLength(PRUint32 aMinLength)
{
  sbSimpleAutoLock lock(mMinMaxLock);
  mMinLen = aMinLength;
  return NS_OK;
}

NS_IMETHODIMP sbTextPropertyInfo::GetMaxLength(PRUint32 *aMaxLength)
{
  NS_ENSURE_ARG_POINTER(aMaxLength);
  sbSimpleAutoLock lock(mMinMaxLock);
  *aMaxLength = mMaxLen;
  return NS_OK;
}
NS_IMETHODIMP sbTextPropertyInfo::SetMaxLength(PRUint32 aMaxLength)
{
  sbSimpleAutoLock lock(mMinMaxLock);
  mMaxLen = aMaxLength;
  return NS_OK;
}

NS_IMETHODIMP sbTextPropertyInfo::GetEnforceLowercase(PRBool *aEnforceLowercase)
{
  NS_ENSURE_ARG_POINTER(aEnforceLowercase);
  sbSimpleAutoLock lock(mEnforceLowercaseLock);
  *aEnforceLowercase = mEnforceLowercase;
  return NS_OK;
}
NS_IMETHODIMP sbTextPropertyInfo::SetEnforceLowercase(PRBool aEnforceLowercase)
{
  sbSimpleAutoLock lock(mEnforceLowercaseLock);
  mEnforceLowercase = aEnforceLowercase;
  return NS_OK;
}

NS_IMETHODIMP sbTextPropertyInfo::GetNoCompressWhitespace(PRBool *aNoCompressWhitespace)
{
  NS_ENSURE_ARG_POINTER(aNoCompressWhitespace);
  sbSimpleAutoLock lock(mNoCompressWhitespaceLock);
  *aNoCompressWhitespace = mNoCompressWhitespace;
  return NS_OK;
}
NS_IMETHODIMP sbTextPropertyInfo::SetNoCompressWhitespace(PRBool aNoCompressWhitespace)
{
  sbSimpleAutoLock lock(mNoCompressWhitespaceLock);
  mNoCompressWhitespace = aNoCompressWhitespace;
  return NS_OK;
}
