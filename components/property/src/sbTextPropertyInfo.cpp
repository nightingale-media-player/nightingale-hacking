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

#include "sbTextPropertyInfo.h"

#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsUnicharUtils.h>
#include <nsMemory.h>

#include <sbIStringTransform.h>

#include <sbLockUtils.h>

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

  PRInt32 len = aValue.Length();
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

  _retval = aValue;

  //Don't compress/strip the whitespace if requested
  {
    sbSimpleAutoLock lock(mNoCompressWhitespaceLock);
    if (!mNoCompressWhitespace) {
      CompressWhitespace(_retval);
    }
  }

  PRInt32 len = aValue.Length();

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

// XXXlone> jemalloc is missing _realloc_dbg (http://bugzilla.songbirdnest.com/show_bug.cgi?id=14615)
// respin is coming, but until then, you can uncomment these lines to succesfuly
// build in debug mode

#ifdef DEBUG
void *
_realloc_dbg(void *userdata, size_t s, int blocktype, const char* r1, int r2)
{
        return realloc(userdata, s);
} 
#endif 

// text property info needs to compute local-specific collation data instead
// of relying on the ancestor's default implementation (which just calls
// MakeSearchable), so that proper sort order are achieved.
NS_IMETHODIMP sbTextPropertyInfo::MakeSortable(const nsAString & aValue, nsAString & _retval)
{
  // first compress whitespaces
  nsAutoString val;
  val = aValue;
  CompressWhitespace(val);

  nsresult rv;
  
  nsCOMPtr<sbIStringTransform> stringTransform = 
    do_CreateInstance(SB_STRINGTRANSFORM_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString outVal;
  rv = stringTransform->RemoveArticles(val, EmptyString(), outVal);
  NS_ENSURE_SUCCESS(rv, rv);

  val = outVal;
  
  // we might want to limit the number of characters we are sorting on ? just
  // so that we do not generate humongus collation data blocks ? disable for
  // now.
  // if (val.Length() > 256)
  //     val.SetLength()
  
  // read the current locale for collate and ctype
  const char *oldlocale_collate = setlocale(LC_COLLATE, NULL);
  const char *oldlocale_ctype = setlocale(LC_CTYPE, NULL);

  // loading locale "" loads the user defined locale
  setlocale(LC_COLLATE, "");
  // this sets mbstowcs and wcstombs' encoding. en_US is ignored because
  // we're only setting LC_CTYPE.
  setlocale(LC_CTYPE, "en_US.UTF-8");
  
  wchar_t *input;
  
#ifdef WIN32
  input = (wchar_t *)val.BeginReading();
#else
  // convert from 4-bytes chars to 2-bytes chars
  nsCString input_utf8 = NS_ConvertUTF16toUTF8(val);
  PRUint32 wc32size = mbstowcs(NULL, 
                               input_utf8.BeginReading(), 
                               input_utf8.Length());
  input = (wchar_t *)PR_Malloc((wc32size + 1) * 4);
  if (!input) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mbstowcs(input, input_utf8.BeginReading(), input_utf8.Length() + 1);
#endif

  // ask how many characters we need to hold the collation data
  PRUint32 xfrm_size = wcsxfrm(NULL, input, 0);
  if (xfrm_size < 0) {
    // uncollatable characters, fail.
#ifdef __STDC_ISO_10646__
    PR_Free(input);
#endif
    return NS_ERROR_FAILURE;
  }
  
  PRUint32 buffer_size;

// glibc uses 4-bytes wchar_t, but mozilla is compiled with 2-bytes wchar_t, 
// we need to apply conversions. we add 1 char to hold the terminating null.
#ifndef WIN32
  buffer_size = (xfrm_size + 1) * 4;
#else
  buffer_size = (xfrm_size + 1) * sizeof(wchar_t);
#endif

  // allocate enough chars to hold the collation data
  wchar_t *xform_data = (wchar_t *)PR_Malloc(buffer_size);
  if (!xform_data) {
#ifndef WIN32
    PR_Free(input);
#endif
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // apply the proper collation algorithm, depending on the user's locale.
  
  // note that it is impossible to use the proper sort for *all* languages at
  // the same time, because what is proper depends on the original language from
  // which the string came. for instance, hungarian artists should have their
  // accented vowels sorted the same as non-accented ones, but a french artist 
  // should have the last accent determine that order. because we cannot
  // possibly guess the origin locale for the string, the only thing we can do
  // is use the user's current locale on all strings.
  
  // that being said, many language-specific letters (such as the german eszett)
  // have one one way of being properly sorted (in this instance, it must sort
  // with the same weight as 'ss', and are collated that way no matter what
  // locale is being used.
  
  // also note that if a user changes his locale at any point, the order of
  // a sort will remain the same (it is stored in the db as collation data)
  // until the sort values are regenerated. this could lead to inconsistent sort
  // orders if some sort values have been regenerated (eg, because their track's
  // metadata changed), but not others. perhaps we should have some ui somewhere
  // that lets the user perform a complete sort data regeneration ? (more
  // probably this should be part of the power tools extension, though)
  wcsxfrm(xform_data, input, xfrm_size + 1); // +1 for terminating null
  
#ifdef WIN32
  // if 2-bytes wchar_t, simply cast, then assign to the return value
  _retval = (PRUnichar *)xform_data;
#else
  // we're done with the input
  PR_Free(input);
  // convert back from 4-bytes chars to 2-bytes chars
  int mbsize = wcstombs(NULL, xform_data, xfrm_size);
  char *xform_data_mbs = (char *)PR_Malloc(mbsize + 1);
  wcstombs(xform_data_mbs, xform_data, xfrm_size + 1);
  _retval = NS_ConvertUTF8toUTF16(xform_data_mbs, mbsize);
  PR_Free(xform_data_mbs);
#endif

  // free the collation data buffer
  PR_Free(xform_data);
  
  // restore the previous locale settings
  setlocale(LC_COLLATE, oldlocale_collate);
  setlocale(LC_CTYPE, oldlocale_ctype);

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

  PRInt32 len = aValue.Length();

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
