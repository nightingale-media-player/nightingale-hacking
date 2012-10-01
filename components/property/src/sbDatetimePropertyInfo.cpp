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

#include "sbDatetimePropertyInfo.h"
#include "sbStandardOperators.h"

#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>

#include <prtime.h>
#include <prprf.h>

#include <sbLockUtils.h>

static const char *gsFmtRadix10 = "%lld";
static const char *gsSortFmtRadix10 = "%+020lld";

NS_IMPL_ADDREF_INHERITED(sbDatetimePropertyInfo, sbPropertyInfo);
NS_IMPL_RELEASE_INHERITED(sbDatetimePropertyInfo, sbPropertyInfo);

NS_INTERFACE_TABLE_HEAD(sbDatetimePropertyInfo)
NS_INTERFACE_TABLE_BEGIN
NS_INTERFACE_TABLE_ENTRY(sbDatetimePropertyInfo, sbIDatetimePropertyInfo)
NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(sbDatetimePropertyInfo, sbIPropertyInfo, sbIDatetimePropertyInfo)
NS_INTERFACE_TABLE_END
NS_INTERFACE_TABLE_TAIL_INHERITING(sbPropertyInfo)

sbDatetimePropertyInfo::sbDatetimePropertyInfo()
: mTimeTypeLock(nsnull)
, mTimeType(sbIDatetimePropertyInfo::TIMETYPE_UNINITIALIZED)
, mMinMaxDateTimeLock(nsnull)
, mMinDateTime(0)
, mMaxDateTime(LL_MAXINT)
, mAppLocaleLock(nsnull)
, mDateTimeFormatLock(nsnull)
{
  mType = NS_LITERAL_STRING("datetime");

  mTimeTypeLock = PR_NewLock();
  NS_ASSERTION(mTimeTypeLock,
    "sbDatetimePropertyInfo::mTimeTypeLock failed to create lock!");

  mMinMaxDateTimeLock = PR_NewLock();
  NS_ASSERTION(mMinMaxDateTimeLock,
    "sbDatetimePropertyInfo::mMinMaxDateTimeLock failed to create lock!");

  mAppLocaleLock = PR_NewLock();
  NS_ASSERTION(mAppLocaleLock,
    "sbDatetimePropertyInfo::mAppLocaleLock failed to create lock!");

  mDateTimeFormatLock = PR_NewLock();
  NS_ASSERTION(mDateTimeFormatLock,
    "sbDatetimePropertyInfo::mDateTimeFormatLock failed to create lock!");
}

sbDatetimePropertyInfo::~sbDatetimePropertyInfo()
{
  if(mTimeTypeLock) {
    PR_DestroyLock(mTimeTypeLock);
  }
  if(mMinMaxDateTimeLock) {
    PR_DestroyLock(mMinMaxDateTimeLock);
  }
  if(mAppLocaleLock) {
    PR_DestroyLock(mAppLocaleLock);
  }
  if(mDateTimeFormatLock) {
    PR_DestroyLock(mDateTimeFormatLock);
  }
}

nsresult
sbDatetimePropertyInfo::Init() {
  nsresult rv;
  
  rv = sbPropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = InitializeOperators();
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult 
sbDatetimePropertyInfo::InitializeOperators()
{
  nsresult rv;
  nsAutoString op;
  nsRefPtr<sbPropertyOperator> propOp;

  rv = sbPropertyInfo::GetOPERATOR_EQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.equal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetOPERATOR_ONDATE(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.ondate"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_NOTEQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.notequal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetOPERATOR_NOTONDATE(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.notondate"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_GREATER(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.greater"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetOPERATOR_AFTERDATE(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.afterdate"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_GREATEREQUAL(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.greaterequal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetOPERATOR_AFTERORONDATE(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.onafterdate"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_LESS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.less"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetOPERATOR_BEFOREDATE(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.beforedate"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_LESSEQUAL(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.lessequal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetOPERATOR_BEFOREORONDATE(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.onbeforedate"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetOPERATOR_INTHELAST(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.inthelast"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetOPERATOR_NOTINTHELAST(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.notinthelast"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_BETWEEN(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.between"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetOPERATOR_BETWEENDATES(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.betweendates"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP sbDatetimePropertyInfo::Validate(const nsAString & aValue, bool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  PRInt64 value = 0;
  NS_ConvertUTF16toUTF8 narrow(aValue);
  *_retval = PR_TRUE;

  if(PR_sscanf(narrow.get(), gsFmtRadix10, &value) != 1) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  sbSimpleAutoLock lock(mMinMaxDateTimeLock);
  if(value < mMinDateTime ||
     value > mMaxDateTime) {
    *_retval = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP sbDatetimePropertyInfo::Sanitize(const nsAString & aValue, nsAString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbDatetimePropertyInfo::Format(const nsAString & aValue, nsAString & _retval)
{
  PRInt32 timeType = 0;
  PRInt64 value = 0;
  NS_ConvertUTF16toUTF8 narrow(aValue);

  nsresult rv = GetTimeType(&timeType);
  NS_ENSURE_SUCCESS(rv, rv);

  if(PR_sscanf(narrow.get(), gsFmtRadix10, &value) != 1) {
    return NS_ERROR_INVALID_ARG;
  }

  {
    sbSimpleAutoLock lock(mMinMaxDateTimeLock);
    if(value < mMinDateTime ||
       value > mMaxDateTime) {
      return NS_ERROR_INVALID_ARG;
    }
  }

  if(timeType != sbIDatetimePropertyInfo::TIMETYPE_TIMESTAMP) {
    nsAutoString out;
    sbSimpleAutoLock lockLocale(mAppLocaleLock);

    if(!mAppLocale) {
      nsCOMPtr<nsILocaleService> localeService =
        do_GetService("@mozilla.org/intl/nslocaleservice;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = localeService->GetApplicationLocale(getter_AddRefs(mAppLocale));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    sbSimpleAutoLock lockFormatter(mDateTimeFormatLock);
    if(!mDateTimeFormat) {
      mDateTimeFormat = do_CreateInstance(NS_DATETIMEFORMAT_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    switch(mTimeType) {
      case sbIDatetimePropertyInfo::TIMETYPE_TIME:
      {
        PRExplodedTime explodedTime = {0};
        PR_ExplodeTime((PRTime) (value * PR_USEC_PER_MSEC), PR_LocalTimeParameters, &explodedTime);
        rv = mDateTimeFormat->FormatPRExplodedTime(mAppLocale,
          kDateFormatNone,
          kTimeFormatSeconds,
          &explodedTime,
          out);

      }
      break;

      case sbIDatetimePropertyInfo::TIMETYPE_DATE:
      {
        PRExplodedTime explodedTime = {0};
        PR_ExplodeTime((PRTime) (value * PR_USEC_PER_MSEC), PR_LocalTimeParameters, &explodedTime);
        rv = mDateTimeFormat->FormatPRExplodedTime(mAppLocale,
          kDateFormatLong,
          kTimeFormatNone,
          &explodedTime,
          out);

      }
      break;

      case sbIDatetimePropertyInfo::TIMETYPE_DATETIME:
      {
        PRExplodedTime explodedTime = {0};
        PR_ExplodeTime((PRTime) (value * PR_USEC_PER_MSEC), PR_LocalTimeParameters, &explodedTime);
        rv = mDateTimeFormat->FormatPRExplodedTime(mAppLocale,
          kDateFormatShort,
          kTimeFormatNoSeconds,
          &explodedTime,
          out);
      }
      break;
    }

    NS_ENSURE_SUCCESS(rv, rv);
    _retval = out;
  }
  else {
    _retval = aValue;
    CompressWhitespace(_retval);
  }

  return NS_OK;
}

NS_IMETHODIMP sbDatetimePropertyInfo::MakeSearchable(const nsAString & aValue, nsAString & _retval)
{
  nsresult rv;
  PRInt64 value = 0;
  NS_ConvertUTF16toUTF8 narrow(aValue);

  _retval = aValue;
  _retval.StripWhitespace();

  sbSimpleAutoLock lock(mMinMaxDateTimeLock);

  if(PR_sscanf(narrow.get(), gsFmtRadix10, &value) != 1) {
    _retval = EmptyString();
    return NS_ERROR_INVALID_ARG;
  }

  char out[32] = {0};
  if(PR_snprintf(out, 32, gsSortFmtRadix10, value) == (PRUint32)-1) {
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

NS_IMETHODIMP sbDatetimePropertyInfo::GetTimeType(PRInt32 *aTimeType)
{
  NS_ENSURE_ARG_POINTER(aTimeType);

  sbSimpleAutoLock lock(mTimeTypeLock);
  if(mTimeType != sbIDatetimePropertyInfo::TIMETYPE_UNINITIALIZED) {
    *aTimeType = mTimeType;
    return NS_OK;
  }

  return NS_ERROR_NOT_INITIALIZED;
}
NS_IMETHODIMP sbDatetimePropertyInfo::SetTimeType(PRInt32 aTimeType)
{
  NS_ENSURE_ARG(aTimeType > sbDatetimePropertyInfo::TIMETYPE_UNINITIALIZED &&
    aTimeType <= sbDatetimePropertyInfo::TIMETYPE_TIMESTAMP);

  sbSimpleAutoLock lock(mTimeTypeLock);
  if(mTimeType == sbIDatetimePropertyInfo::TIMETYPE_UNINITIALIZED) {
    mTimeType = aTimeType;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbDatetimePropertyInfo::GetMinDateTime(PRInt64 *aMinDateTime)
{
  NS_ENSURE_ARG_POINTER(aMinDateTime);

  sbSimpleAutoLock lock(mMinMaxDateTimeLock);
  *aMinDateTime = mMinDateTime;

  return NS_OK;
}
NS_IMETHODIMP sbDatetimePropertyInfo::SetMinDateTime(PRInt64 aMinDateTime)
{
  NS_ENSURE_ARG(aMinDateTime > -1);

  sbSimpleAutoLock lock(mMinMaxDateTimeLock);
  mMinDateTime = aMinDateTime;

  return NS_OK;
}

NS_IMETHODIMP sbDatetimePropertyInfo::GetMaxDateTime(PRInt64 *aMaxDateTime)
{
  NS_ENSURE_ARG_POINTER(aMaxDateTime);

  sbSimpleAutoLock lock(mMinMaxDateTimeLock);
  *aMaxDateTime = mMaxDateTime;

  return NS_OK;
}
NS_IMETHODIMP sbDatetimePropertyInfo::SetMaxDateTime(PRInt64 aMaxDateTime)
{
  NS_ENSURE_ARG(aMaxDateTime > -1);

  sbSimpleAutoLock lock(mMinMaxDateTimeLock);
  mMaxDateTime = aMaxDateTime;

  return NS_OK;
}

NS_IMETHODIMP sbDatetimePropertyInfo::GetOPERATOR_INTHELAST(nsAString & aOPERATOR_INTHELAST)
{
  aOPERATOR_INTHELAST = NS_LITERAL_STRING(SB_OPERATOR_INTHELAST);
  return NS_OK;
}

NS_IMETHODIMP sbDatetimePropertyInfo::GetOPERATOR_NOTINTHELAST(nsAString & aOPERATOR_NOTINTHELAST)
{
  aOPERATOR_NOTINTHELAST = NS_LITERAL_STRING(SB_OPERATOR_NOTINTHELAST);
  return NS_OK;
}

NS_IMETHODIMP sbDatetimePropertyInfo::GetOPERATOR_ONDATE(nsAString & aOPERATOR_ONDATE)
{
  aOPERATOR_ONDATE = NS_LITERAL_STRING(SB_OPERATOR_ONDATE);
  return NS_OK;
}

NS_IMETHODIMP sbDatetimePropertyInfo::GetOPERATOR_NOTONDATE(nsAString & aOPERATOR_NOTONDATE)
{
  aOPERATOR_NOTONDATE = NS_LITERAL_STRING(SB_OPERATOR_NOTONDATE);
  return NS_OK;
}

NS_IMETHODIMP sbDatetimePropertyInfo::GetOPERATOR_BEFOREDATE(nsAString & aOPERATOR_BEFOREDATE)
{
  aOPERATOR_BEFOREDATE = NS_LITERAL_STRING(SB_OPERATOR_BEFOREDATE);
  return NS_OK;
}

NS_IMETHODIMP sbDatetimePropertyInfo::GetOPERATOR_BEFOREORONDATE(nsAString & aOPERATOR_BEFOREORONDATE)
{
  aOPERATOR_BEFOREORONDATE = NS_LITERAL_STRING(SB_OPERATOR_BEFOREORONDATE);
  return NS_OK;
}

NS_IMETHODIMP sbDatetimePropertyInfo::GetOPERATOR_AFTERDATE(nsAString & aOPERATOR_AFTERDATE)
{
  aOPERATOR_AFTERDATE = NS_LITERAL_STRING(SB_OPERATOR_AFTERDATE);
  return NS_OK;
}

NS_IMETHODIMP sbDatetimePropertyInfo::GetOPERATOR_AFTERORONDATE(nsAString & aOPERATOR_AFTERORONDATE)
{
  aOPERATOR_AFTERORONDATE = NS_LITERAL_STRING(SB_OPERATOR_AFTERORONDATE);
  return NS_OK;
}

NS_IMETHODIMP sbDatetimePropertyInfo::GetOPERATOR_BETWEENDATES(nsAString & aOPERATOR_BETWEENDATES)
{
  aOPERATOR_BETWEENDATES = NS_LITERAL_STRING(SB_OPERATOR_BETWEENDATES);
  return NS_OK;
}
