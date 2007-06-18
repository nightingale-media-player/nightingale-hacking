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

#include "sbNumberPropertyInfo.h"
#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <prprf.h>

#include <sbTArrayStringEnumerator.h>

/*static inline*/
PRBool IsValidRadix(PRUint32 aRadix) 
{
  if(aRadix == 8 ||
     aRadix == 10 ||
     aRadix == 16) {
    return PR_TRUE;
  }
     
  return PR_FALSE;
}

/*static data for static inline function*/
static const char *gsFmtRadix8 = "%llo";
static const char *gsFmtRadix10 = "%lld";
static const char *gsFmtRadix16 = "%llX";

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
  }

  return fmt;
}

/*static data for static inline function*/
static const char *gsSortFmtRadix8 = "%022llo";
static const char *gsSortFmtRadix10 = "%+020lld";
static const char *gsSortFmtRadix16 = "%016llX";

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
  }

  return fmt;
}

NS_IMPL_ISUPPORTS_INHERITED1(sbNumberPropertyInfo, sbPropertyInfo,
                                                   sbINumberPropertyInfo)

sbNumberPropertyInfo::sbNumberPropertyInfo()
: mMinMaxValueLock(nsnull)
, mMinValue(LL_MININT)
, mMaxValue(LL_MAXINT)
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

  InitializeOperators();
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

void sbNumberPropertyInfo::InitializeOperators()
{
  nsAutoString op;
  nsRefPtr<sbPropertyOperator> propOp;
  
  sbPropertyInfo::GetOPERATOR_GREATER(op);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.int.greater"));
  mOperators.AppendObject(propOp);

  sbPropertyInfo::GetOPERATOR_GREATEREQUAL(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.int.greaterequal"));
  mOperators.AppendObject(propOp);

  sbPropertyInfo::GetOPERATOR_LESS(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.int.less"));
  mOperators.AppendObject(propOp);

  sbPropertyInfo::GetOPERATOR_LESSEQUAL(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.int.lessequal"));
  mOperators.AppendObject(propOp);

  sbPropertyInfo::GetOPERATOR_EQUALS(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.int.equal"));
  mOperators.AppendObject(propOp);

  sbPropertyInfo::GetOPERATOR_NOTEQUALS(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.int.notequal"));
  mOperators.AppendObject(propOp);

  sbPropertyInfo::GetOPERATOR_BETWEEN(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.int.between"));
  mOperators.AppendObject(propOp);

  return;
}

NS_IMETHODIMP sbNumberPropertyInfo::Validate(const nsAString & aValue, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  
  PRInt64 value = 0;
  NS_ConvertUTF16toUTF8 narrow(aValue);
  
  nsAutoLock lockRadix(mRadixLock);
  const char *fmt = GetFmtFromRadix(mRadix);
  
  nsAutoLock lockMinMax(mMinMaxValueLock);
  *_retval = PR_TRUE;

  if(PR_sscanf(narrow.get(), fmt, &value) != 1) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  if(value < mMinValue ||
     value > mMaxValue) {
     *_retval = PR_FALSE;
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
  NS_ConvertUTF16toUTF8 narrow(aValue);

  _retval = aValue;
  _retval.StripWhitespace();

  nsAutoLock lockRadix(mRadixLock);
  const char *fmt = GetFmtFromRadix(mRadix);

  nsAutoLock lockMinMax(mMinMaxValueLock);
  if(PR_sscanf(narrow.get(), fmt, &value) != 1) {
    _retval = EmptyString();
    return NS_ERROR_INVALID_ARG;
  }

  char out[32] = {0};
  PR_snprintf(out, 32, fmt, value);
  
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

NS_IMETHODIMP sbNumberPropertyInfo::MakeSortable(const nsAString & aValue, nsAString & _retval)
{
  nsresult rv;
  PRInt64 value = 0;
  NS_ConvertUTF16toUTF8 narrow(aValue);

  _retval = aValue;
  _retval.StripWhitespace();

  nsAutoLock lockRadix(mRadixLock);
  const char *fmt = GetFmtFromRadix(mRadix);

  nsAutoLock lockMinMax(mMinMaxValueLock);
  if(PR_sscanf(narrow.get(), fmt, &value) != 1) {
    _retval = EmptyString();
    return NS_ERROR_INVALID_ARG;
  }

  char out[32] = {0};
  const char *sortableFmt = GetSortableFmtFromRadix(mRadix);
  if(PR_snprintf(out, 32, sortableFmt, value) == -1) {
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
  nsAutoLock lock(mMinMaxValueLock);
  *aMinValue = mMinValue;
  return NS_OK;
}
NS_IMETHODIMP sbNumberPropertyInfo::SetMinValue(PRInt64 aMinValue)
{
  nsAutoLock lock(mMinMaxValueLock);

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
  nsAutoLock lock(mMinMaxValueLock);
  *aMaxValue = mMaxValue;
  return NS_OK;
}
NS_IMETHODIMP sbNumberPropertyInfo::SetMaxValue(PRInt64 aMaxValue)
{
  nsAutoLock lock(mMinMaxValueLock);

  if(!mHasSetMaxValue) {
    mMaxValue = aMaxValue;
    mHasSetMaxValue = PR_TRUE;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbNumberPropertyInfo::GetRadix(PRUint32 *aRadix)
{
  NS_ENSURE_ARG_POINTER(aRadix);

  nsAutoLock lock(mRadixLock);
  *aRadix = mRadix;
  return NS_OK;
}
NS_IMETHODIMP sbNumberPropertyInfo::SetRadix(PRUint32 aRadix)
{
  NS_ENSURE_TRUE(IsValidRadix(aRadix), NS_ERROR_INVALID_ARG);
  nsAutoLock lock(mRadixLock);
  mRadix = aRadix;
  return NS_OK;
}
