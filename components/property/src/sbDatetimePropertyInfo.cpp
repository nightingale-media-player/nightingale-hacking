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

#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>

#include <prtime.h>
#include <prprf.h>

#include <sbLockUtils.h>

static const char *gsFmtRadix10 = "%lld";
static const char *gsSortFmtRadix10 = "%+020lld";
static const char *gsFmtMilliseconds10 = "%04d";

NS_IMPL_ADDREF_INHERITED(sbDatetimePropertyInfo, sbPropertyInfo);
NS_IMPL_RELEASE_INHERITED(sbDatetimePropertyInfo, sbPropertyInfo);

NS_INTERFACE_TABLE_HEAD(sbDatetimePropertyInfo)
NS_INTERFACE_TABLE_BEGIN
NS_INTERFACE_TABLE_ENTRY(sbDatetimePropertyInfo, sbIDatetimePropertyInfo)
NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(sbDatetimePropertyInfo, sbIPropertyInfo, sbIDatetimePropertyInfo)
NS_INTERFACE_TABLE_END
NS_INTERFACE_TABLE_TAIL_INHERITING(sbPropertyInfo)

sbDatetimePropertyInfo::sbDatetimePropertyInfo()
: mDurationInversed(PR_FALSE)
, mDurationDisplayMillisec(PR_FALSE)
, mTimeTypeLock(nsnull)
, mTimeType(sbIDatetimePropertyInfo::TIMETYPE_UNINITIALIZED)
, mMinMaxTimeLock(nsnull)
, mMinTime(0)
, mMaxTime(LL_MAXINT)
, mAppLocaleLock(nsnull)
, mDateTimeFormatLock(nsnull)
{
  mType = NS_LITERAL_STRING("datetime");

  mTimeTypeLock = PR_NewLock();
  NS_ASSERTION(mTimeTypeLock,
    "sbDatetimePropertyInfo::mTimeTypeLock failed to create lock!");

  mMinMaxTimeLock = PR_NewLock();
  NS_ASSERTION(mMinMaxTimeLock,
    "sbDatetimePropertyInfo::mMinMaxTimeLock failed to create lock!");

  mAppLocaleLock = PR_NewLock();
  NS_ASSERTION(mAppLocaleLock,
    "sbDatetimePropertyInfo::mAppLocaleLock failed to create lock!");

  mDateTimeFormatLock = PR_NewLock();
  NS_ASSERTION(mDateTimeFormatLock,
    "sbDatetimePropertyInfo::mDateTimeFormatLock failed to create lock!");

  InitializeOperators();
}

sbDatetimePropertyInfo::~sbDatetimePropertyInfo()
{
  if(mTimeTypeLock) {
    PR_DestroyLock(mTimeTypeLock);
  }
  if(mMinMaxTimeLock) {
    PR_DestroyLock(mMinMaxTimeLock);
  }
  if(mAppLocaleLock) {
    PR_DestroyLock(mAppLocaleLock);
  }
  if(mDateTimeFormatLock) {
    PR_DestroyLock(mDateTimeFormatLock);
  }
}

void sbDatetimePropertyInfo::InitializeOperators()
{
  nsAutoString op;
  nsRefPtr<sbPropertyOperator> propOp;

  sbPropertyInfo::GetOPERATOR_EQUALS(op);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.on"));
  mOperators.AppendObject(propOp);

  sbPropertyInfo::GetOPERATOR_NOTEQUALS(op);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.noton"));
  mOperators.AppendObject(propOp);

  sbPropertyInfo::GetOPERATOR_LESSEQUAL(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.onbefore"));
  mOperators.AppendObject(propOp);

  sbPropertyInfo::GetOPERATOR_GREATEREQUAL(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.onafter"));
  mOperators.AppendObject(propOp);

  sbPropertyInfo::GetOPERATOR_LESS(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.before"));
  mOperators.AppendObject(propOp);

  sbPropertyInfo::GetOPERATOR_GREATER(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.after"));
  mOperators.AppendObject(propOp);

  sbPropertyInfo::GetOPERATOR_BETWEEN(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.date.between"));
  mOperators.AppendObject(propOp);

  return;
}

NS_IMETHODIMP sbDatetimePropertyInfo::Validate(const nsAString & aValue, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  PRInt64 value = 0;
  NS_ConvertUTF16toUTF8 narrow(aValue);
  *_retval = PR_TRUE;

  if(PR_sscanf(narrow.get(), gsFmtRadix10, &value) != 1) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  sbSimpleAutoLock lock(mMinMaxTimeLock);
  if(value < mMinTime ||
     value > mMaxTime) {
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
    sbSimpleAutoLock lock(mMinMaxTimeLock);
    if(value < mMinTime ||
       value > mMaxTime) {
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
        PR_ExplodeTime((PRTime) (value * 1000), PR_LocalTimeParameters, &explodedTime);
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
        PR_ExplodeTime((PRTime) (value * 1000), PR_LocalTimeParameters, &explodedTime);
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
        PR_ExplodeTime((PRTime) (value * 1000), PR_LocalTimeParameters, &explodedTime);
        rv = mDateTimeFormat->FormatPRExplodedTime(mAppLocale,
          kDateFormatShort,
          kTimeFormatNoSeconds,
          &explodedTime,
          out);
      }
      break;

      case sbIDatetimePropertyInfo::TIMETYPE_DURATION:
      {
        PRExplodedTime referenceTime = {0}, explodedTime = {0};
        PR_ExplodeTime((PRTime) 0, PR_GMTParameters, &referenceTime);
        PR_ExplodeTime((PRTime) value, PR_GMTParameters, &explodedTime);

        PRInt32 delta = explodedTime.tm_year - referenceTime.tm_year;
        if(delta) {
          out.AppendInt(delta);
          out.AppendLiteral("Y");
        }

        delta = explodedTime.tm_month - referenceTime.tm_month;
        if(delta) {
          out.AppendInt(delta);
          out.AppendLiteral("M");
        }

        delta = explodedTime.tm_mday - referenceTime.tm_mday;
        if(delta) {
          out.AppendInt(delta);
          out.AppendLiteral("D ");
        }

        PRInt32 hours = explodedTime.tm_hour - referenceTime.tm_hour;
        if(hours) {
          out.AppendInt(hours);
          out.AppendLiteral(":");
        }

        PRInt32 mins = explodedTime.tm_min - referenceTime.tm_min;
        if(hours && mins < 10 ) {
          out.AppendLiteral("0");
        }

        out.AppendInt(mins);
        out.AppendLiteral(":");

        PRInt32 secs = explodedTime.tm_sec - referenceTime.tm_sec;
        if(secs < 10) {
          out.AppendLiteral("0");
        }

        out.AppendInt(secs);

        if(mDurationDisplayMillisec) {
          delta = (explodedTime.tm_usec - referenceTime.tm_usec)
            / PR_USEC_PER_MSEC;

          char c[32] = {0};
          PR_snprintf(c, 32, gsFmtMilliseconds10, delta);

          out.AppendLiteral(".");
          out.Append(NS_ConvertUTF8toUTF16(c));
        }
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

NS_IMETHODIMP sbDatetimePropertyInfo::MakeSortable(const nsAString & aValue, nsAString & _retval)
{
  nsresult rv;
  PRInt64 value = 0;
  NS_ConvertUTF16toUTF8 narrow(aValue);

  _retval = aValue;
  _retval.StripWhitespace();

  sbSimpleAutoLock lock(mMinMaxTimeLock);

  if(PR_sscanf(narrow.get(), gsFmtRadix10, &value) != 1) {
    _retval = EmptyString();
    return NS_ERROR_INVALID_ARG;
  }

  char out[32] = {0};
  if(PR_snprintf(out, 32, gsSortFmtRadix10, value) == -1) {
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

NS_IMETHODIMP sbDatetimePropertyInfo::GetMinTime(PRInt64 *aMinTime)
{
  NS_ENSURE_ARG_POINTER(aMinTime);

  sbSimpleAutoLock lock(mMinMaxTimeLock);
  *aMinTime = mMinTime;

  return NS_OK;
}
NS_IMETHODIMP sbDatetimePropertyInfo::SetMinTime(PRInt64 aMinTime)
{
  NS_ENSURE_ARG(aMinTime > -1);

  sbSimpleAutoLock lock(mMinMaxTimeLock);
  mMinTime = aMinTime;

  return NS_OK;
}

NS_IMETHODIMP sbDatetimePropertyInfo::GetMaxTime(PRInt64 *aMaxTime)
{
  NS_ENSURE_ARG_POINTER(aMaxTime);

  sbSimpleAutoLock lock(mMinMaxTimeLock);
  *aMaxTime = mMaxTime;

  return NS_OK;
}
NS_IMETHODIMP sbDatetimePropertyInfo::SetMaxTime(PRInt64 aMaxTime)
{
  NS_ENSURE_ARG(aMaxTime > -1);

  sbSimpleAutoLock lock(mMinMaxTimeLock);
  mMaxTime = aMaxTime;

  return NS_OK;
}

NS_IMETHODIMP sbDatetimePropertyInfo::GetDurationInverse(PRBool *aDurationInverse)
{
  NS_ENSURE_ARG_POINTER(aDurationInverse);
  *aDurationInverse = mDurationInversed;
  return NS_OK;
}
NS_IMETHODIMP sbDatetimePropertyInfo::SetDurationInverse(PRBool aDurationInverse)
{
  mDurationInversed = aDurationInverse;
  return NS_OK;
}

NS_IMETHODIMP sbDatetimePropertyInfo::GetDurationWithMilliseconds(PRBool *aDurationWithMilliseconds)
{
  NS_ENSURE_ARG_POINTER(aDurationWithMilliseconds);
  *aDurationWithMilliseconds = mDurationDisplayMillisec;
  return NS_OK;
}
NS_IMETHODIMP sbDatetimePropertyInfo::SetDurationWithMilliseconds(PRBool aDurationWithMilliseconds)
{
  mDurationDisplayMillisec = aDurationWithMilliseconds;
  return NS_OK;
}
