/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#include "sbNumberPropertyInfo.h"

#include <float.h>
#include <nsAutoPtr.h>
#include <prprf.h>

#include <sbLockUtils.h>
#include <sbTArrayStringEnumerator.h>

/*static inline*/
PRBool IsValidRadix(PRUint32 aRadix)
{
  if(aRadix == 8 ||
     aRadix == 10 ||
     aRadix == 16 ||
     aRadix == 0) {
    return PR_TRUE;
  }

  return PR_FALSE;
}

/*static data for static inline function*/
static const char *gsFmtRadix8 = "%llo";
static const char *gsFmtRadix10 = "%lld";
static const char *gsFmtRadix16 = "%llX";
static const char *gsFmtFloat = "%lg";

/*static inline*/
const char *GetFmtFromRadix(PRUint32 aRadix)
{
  const char *fmt = nsnull;

  switch(aRadix) {
    case sbINumberPropertyInfo::RADIX_8:
      fmt = gsFmtRadix8;
    break;

    case sbINumberPropertyInfo::RADIX_10:
      fmt = gsFmtRadix10;
    break;

    case sbINumberPropertyInfo::RADIX_16:
      fmt = gsFmtRadix16;
    break;

    case sbINumberPropertyInfo::FLOAT:
      fmt = gsFmtFloat;
    break;
  }

  return fmt;
}

/*static data for static inline function*/
static const char *gsSortFmtRadix8  = "%022llo";
static const char *gsSortFmtRadix10 = "%+020lld";
static const char *gsSortFmtRadix16 = "%016llX";
static const char *gsSortFmtFloat   = "%+046.16lf";

/*static inline*/
const char *GetSortableFmtFromRadix(PRUint32 aRadix)
{
  const char *fmt = nsnull;

  switch(aRadix) {
    case sbINumberPropertyInfo::RADIX_8:
      fmt = gsSortFmtRadix8;
      break;

    case sbINumberPropertyInfo::RADIX_10:
      fmt = gsSortFmtRadix10;
      break;

    case sbINumberPropertyInfo::RADIX_16:
      fmt = gsSortFmtRadix16;
      break;

    case sbINumberPropertyInfo::FLOAT:
      fmt = gsSortFmtFloat;
      break;
  }

  return fmt;
}

NS_IMPL_ADDREF_INHERITED(sbNumberPropertyInfo, sbPropertyInfo);
NS_IMPL_RELEASE_INHERITED(sbNumberPropertyInfo, sbPropertyInfo);

NS_INTERFACE_TABLE_HEAD(sbNumberPropertyInfo)
NS_INTERFACE_TABLE_BEGIN
NS_INTERFACE_TABLE_ENTRY(sbNumberPropertyInfo, sbINumberPropertyInfo)
NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(sbNumberPropertyInfo, sbIPropertyInfo, sbINumberPropertyInfo)
NS_INTERFACE_TABLE_END
NS_INTERFACE_TABLE_TAIL_INHERITING(sbPropertyInfo)

sbNumberPropertyInfo::sbNumberPropertyInfo()
: mMinMaxValueLock(nsnull)
, mMinValue(LL_MININT)
, mMaxValue(LL_MAXINT)
, mMinFloatValue(DBL_MIN)
, mMaxFloatValue(DBL_MAX)
, mHasSetMinValue(PR_FALSE)
, mHasSetMaxValue(PR_FALSE)
, mRadix(sbINumberPropertyInfo::RADIX_10)
{
  mType = NS_LITERAL_STRING("number");

  mMinMaxValueLock = PR_NewLock();
  NS_ASSERTION(mMinMaxValueLock,
    "sbNumberPropertyInfo::mMinMaxValueLock failed to create lock!");

  mRadixLock = PR_NewLock();
  NS_ASSERTION(mRadixLock,
    "sbNumberPropertyInfo::mRadixLock failed to create lock!");
}

sbNumberPropertyInfo::~sbNumberPropertyInfo()
{
  if(mMinMaxValueLock) {
    PR_DestroyLock(mMinMaxValueLock);
  }
  if(mRadixLock) {
    PR_DestroyLock(mRadixLock);
  }
}

nsresult
sbNumberPropertyInfo::Init() {
  nsresult rv;

  rv = sbPropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitializeOperators();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult 
sbNumberPropertyInfo::InitializeOperators()
{
  nsresult rv;
  nsAutoString op;
  nsRefPtr<sbPropertyOperator> propOp;

  rv = sbPropertyInfo::GetOPERATOR_EQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.int.equal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_NOTEQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.int.notequal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_GREATER(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.int.greater"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_GREATEREQUAL(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.int.greaterequal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_LESS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.int.less"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_LESSEQUAL(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.int.lessequal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_BETWEEN(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.int.between"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP sbNumberPropertyInfo::Validate(const nsAString & aValue, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = PR_TRUE;
  if (aValue.IsVoid()) {
     return NS_OK;
  }

  PRInt64 value = 0;
  PRFloat64 floatValue = 0;

  NS_ConvertUTF16toUTF8 narrow(aValue);

  sbSimpleAutoLock lockRadix(mRadixLock);
  const char *fmt = GetFmtFromRadix(mRadix);
  
  // Add a string parsing specifier, to catch extra characters behind 
  // the number. Limit string parsing to 16 chars, we just want to check
  // if something's there anyway, we don't want to do anything with the 
  // actual string
  nsAutoString ext_fmt;
  ext_fmt.AssignLiteral(fmt);
  ext_fmt += NS_LITERAL_STRING("%16s");
  const char remainder[17]="";
  PRInt32 r = 0;
  if(mRadix) {
    r = PR_sscanf(narrow.get(), 
                  NS_LossyConvertUTF16toASCII(ext_fmt).get(), 
                  &value, 
                  remainder);
  }
  else {
    r = PR_sscanf(narrow.get(), 
                  NS_LossyConvertUTF16toASCII(ext_fmt).get(), 
                  &floatValue, 
                  remainder);
  }

  if (r < 1) {
    // We got less than one parameter (the number) that parsed correctly,
    // the value is not valid.
    *_retval = PR_FALSE;
  }

  // Otherwise, check whether we got something in the string. Can't rely on the
  // return value from PR_sscanf here because a number by itself will still parse
  // an empty trailing string successfully
  if (*remainder != 0) {
    // We got extra characters, the value is not valid.
    *_retval = PR_FALSE;
  }

  // This part is specific to integer values. Floats need to be checked against
  // different min/max values.
  sbSimpleAutoLock lockMinMax(mMinMaxValueLock);
  if(mRadix) {
    // Now check min & max constraints
    if(value < mMinValue ||
       value > mMaxValue) {
        // Value is above or below limits, not valid.
        *_retval = PR_FALSE;
    }
  }
  else {
    // Now check min & max constraints
    if(floatValue < mMinFloatValue ||
       floatValue > mMaxFloatValue) {
        // Value is above or below limits, not valid.
        *_retval = PR_FALSE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP sbNumberPropertyInfo::Sanitize(const nsAString & aValue, nsAString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbNumberPropertyInfo::Format(const nsAString & aValue, nsAString & _retval)
{
  PRInt64 value = 0;
  PRFloat64 floatValue = 0;

  NS_ConvertUTF16toUTF8 narrow(aValue);

  _retval = aValue;
  _retval.StripWhitespace();

  sbSimpleAutoLock lockRadix(mRadixLock);
  const char *fmt = GetFmtFromRadix(mRadix);

  sbSimpleAutoLock lockMinMax(mMinMaxValueLock);
  if(mRadix) {
    if(PR_sscanf(narrow.get(), fmt, &value) != 1) {
      _retval = EmptyString();
      return NS_ERROR_INVALID_ARG;
    }
  }
  else {
    if(PR_sscanf(narrow.get(), fmt, &floatValue) != 1) {
      _retval = EmptyString();
      return NS_ERROR_INVALID_ARG;
    }
  }

  char out[64] = {0};
  if(mRadix) {
    PR_snprintf(out, 64, fmt, value);
  }
  else {
    PR_snprintf(out, 64, fmt, floatValue);
  }

  NS_ConvertUTF8toUTF16 wide(out);
  _retval = EmptyString();

  if(fmt == gsFmtRadix16) {
    _retval.AssignLiteral("0x");
  }
  else if(fmt == gsFmtRadix8) {
    _retval.AssignLiteral("0");
  }

  _retval.Append(wide);

  return NS_OK;
}

NS_IMETHODIMP sbNumberPropertyInfo::MakeSearchable(const nsAString & aValue, nsAString & _retval)
{
  nsresult rv;
  _retval = aValue;
  if (aValue.IsVoid()) {
     return NS_OK;
  }

  PRInt64 value = 0;
  PRFloat64 floatValue = 0;

  NS_ConvertUTF16toUTF8 narrow(aValue);

  _retval.StripWhitespace();

  sbSimpleAutoLock lockRadix(mRadixLock);
  const char *fmt = GetFmtFromRadix(mRadix);

  PRInt32 parsedCount = 0;
  sbSimpleAutoLock lockMinMax(mMinMaxValueLock);
  if(mRadix) {
    parsedCount = PR_sscanf(narrow.get(), fmt, &value);
  }
  else {
    parsedCount = PR_sscanf(narrow.get(), fmt, &floatValue);
  }

  if(parsedCount != 1) {
    _retval = EmptyString();
    return NS_ERROR_INVALID_ARG;
  }


  PRUint32 outputLength = 0;
  char out[64] = {0};
  const char *sortableFmt = GetSortableFmtFromRadix(mRadix);

  if(mRadix) {
    outputLength = PR_snprintf(out, 64, sortableFmt, value);
  }
  else {
    outputLength = PR_snprintf(out, 64, sortableFmt, floatValue);
  }

  if(outputLength == -1) {
    rv = NS_ERROR_FAILURE;
    _retval = EmptyString();
  }
  else {
    NS_ConvertUTF8toUTF16 wide(out);
    rv = NS_OK;
    _retval = wide;
  }

  return rv;
}

NS_IMETHODIMP sbNumberPropertyInfo::GetMinValue(PRInt64 *aMinValue)
{
  NS_ENSURE_ARG_POINTER(aMinValue);
  sbSimpleAutoLock lock(mMinMaxValueLock);
  *aMinValue = mMinValue;
  return NS_OK;
}
NS_IMETHODIMP sbNumberPropertyInfo::SetMinValue(PRInt64 aMinValue)
{
  sbSimpleAutoLock lock(mMinMaxValueLock);

  if(!mHasSetMinValue) {
    mMinValue = aMinValue;
    mHasSetMinValue = PR_TRUE;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbNumberPropertyInfo::GetMaxValue(PRInt64 *aMaxValue)
{
  NS_ENSURE_ARG_POINTER(aMaxValue);
  sbSimpleAutoLock lock(mMinMaxValueLock);
  *aMaxValue = mMaxValue;
  return NS_OK;
}
NS_IMETHODIMP sbNumberPropertyInfo::SetMaxValue(PRInt64 aMaxValue)
{
  sbSimpleAutoLock lock(mMinMaxValueLock);

  if(!mHasSetMaxValue) {
    mMaxValue = aMaxValue;
    mHasSetMaxValue = PR_TRUE;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbNumberPropertyInfo::GetMinFloatValue(PRFloat64 *aMinFloatValue)
{
  NS_ENSURE_ARG_POINTER(aMinFloatValue);
  sbSimpleAutoLock lock(mMinMaxValueLock);
  *aMinFloatValue = mMinFloatValue;
  return NS_OK;
}
NS_IMETHODIMP sbNumberPropertyInfo::SetMinFloatValue(PRFloat64 aMinFloatValue)
{
  sbSimpleAutoLock lock(mMinMaxValueLock);

  if(!mHasSetMinValue) {
    mMinFloatValue = aMinFloatValue;
    mHasSetMinValue = PR_TRUE;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbNumberPropertyInfo::GetMaxFloatValue(PRFloat64 *aMaxFloatValue)
{
  NS_ENSURE_ARG_POINTER(aMaxFloatValue);
  sbSimpleAutoLock lock(mMinMaxValueLock);
  *aMaxFloatValue = mMaxFloatValue;
  return NS_OK;
}
NS_IMETHODIMP sbNumberPropertyInfo::SetMaxFloatValue(PRFloat64 aMaxFloatValue)
{
  sbSimpleAutoLock lock(mMinMaxValueLock);

  if(!mHasSetMaxValue) {
    mMaxFloatValue = aMaxFloatValue;
    mHasSetMaxValue = PR_TRUE;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbNumberPropertyInfo::GetRadix(PRUint32 *aRadix)
{
  NS_ENSURE_ARG_POINTER(aRadix);

  sbSimpleAutoLock lock(mRadixLock);
  *aRadix = mRadix;
  return NS_OK;
}
NS_IMETHODIMP sbNumberPropertyInfo::SetRadix(PRUint32 aRadix)
{
  NS_ENSURE_TRUE(IsValidRadix(aRadix), NS_ERROR_INVALID_ARG);
  sbSimpleAutoLock lock(mRadixLock);
  mRadix = aRadix;
  return NS_OK;
}
