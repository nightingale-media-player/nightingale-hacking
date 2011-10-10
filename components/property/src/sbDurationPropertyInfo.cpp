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

#include "sbDurationPropertyInfo.h"
#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>

#include <prtime.h>
#include <prprf.h>

#include <sbLockUtils.h>

static const char *gsFmtRadix10 = "%lld";
static const char *gsSortFmtRadix10 = "%+020lld";
static const char *gsFmtMilliseconds10 = "%04d";

NS_IMPL_ADDREF_INHERITED(sbDurationPropertyInfo, sbPropertyInfo);
NS_IMPL_RELEASE_INHERITED(sbDurationPropertyInfo, sbPropertyInfo);

NS_INTERFACE_TABLE_HEAD(sbDurationPropertyInfo)
NS_INTERFACE_TABLE_BEGIN
NS_INTERFACE_TABLE_ENTRY(sbDurationPropertyInfo, sbIDurationPropertyInfo)
NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(sbDurationPropertyInfo, sbIPropertyInfo, sbIDurationPropertyInfo)
NS_INTERFACE_TABLE_END
NS_INTERFACE_TABLE_TAIL_INHERITING(sbPropertyInfo)

sbDurationPropertyInfo::sbDurationPropertyInfo()
: mDurationInversed(PR_FALSE)
, mDurationDisplayMillisec(PR_FALSE)
, mMinMaxDurationLock(nsnull)
, mMinDuration(0)
, mMaxDuration(LL_MAXINT)
, mAppLocaleLock(nsnull)
, mDateTimeFormatLock(nsnull)
{
  mType = NS_LITERAL_STRING("duration");

  mMinMaxDurationLock = PR_NewLock();
  NS_ASSERTION(mMinMaxDurationLock,
    "sbDurationPropertyInfo::mMinMaxDurationLock failed to create lock!");

  mAppLocaleLock = PR_NewLock();
  NS_ASSERTION(mAppLocaleLock,
    "sbDurationPropertyInfo::mAppLocaleLock failed to create lock!");

  mDateTimeFormatLock = PR_NewLock();
  NS_ASSERTION(mDateTimeFormatLock,
    "sbDurationPropertyInfo::mDateTimeFormatLock failed to create lock!");
}

sbDurationPropertyInfo::~sbDurationPropertyInfo()
{
  if(mMinMaxDurationLock) {
    PR_DestroyLock(mMinMaxDurationLock);
  }
  if(mAppLocaleLock) {
    PR_DestroyLock(mAppLocaleLock);
  }
  if(mDateTimeFormatLock) {
    PR_DestroyLock(mDateTimeFormatLock);
  }
}

nsresult sbDurationPropertyInfo::Init()
{
  nsresult rv;

  rv = sbPropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitializeOperators();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDurationPropertyInfo::InitializeOperators()
{
  nsresult rv;
  nsAutoString op;
  nsRefPtr<sbPropertyOperator> propOp;

  rv = sbPropertyInfo::GetOPERATOR_EQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.duration.equal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_NOTEQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.duration.notequal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_GREATER(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.duration.greater"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_GREATEREQUAL(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.duration.greaterequal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_LESS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.duration.less"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_LESSEQUAL(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.duration.lessequal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_BETWEEN(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.duration.between"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP sbDurationPropertyInfo::Validate(const nsAString & aValue, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  PRInt64 value = 0;
  NS_ConvertUTF16toUTF8 narrow(aValue);
  *_retval = PR_TRUE;

  if(PR_sscanf(narrow.get(), gsFmtRadix10, &value) != 1) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  sbSimpleAutoLock lock(mMinMaxDurationLock);
  if(value < mMinDuration ||
     value > mMaxDuration) {
    *_retval = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP sbDurationPropertyInfo::Sanitize(const nsAString & aValue, nsAString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbDurationPropertyInfo::Format(const nsAString & aValue, nsAString & _retval)
{
  PRInt64 value = 0;
  NS_ConvertUTF16toUTF8 narrow(aValue);

  if(PR_sscanf(narrow.get(), gsFmtRadix10, &value) != 1) {
    return NS_ERROR_INVALID_ARG;
  }

/*
  XXXlone> There is fundamentally no reason for Format to validate the range of
  its input. it does make sense to validate that the value is a number, but we
  want to let people format values that are lower than MinDuration and longer
  than MaxDuration. Ultimately, we should review Format, MakeSortable and
  makeSearchable in all sbIPropertyInfo classes, and go through the steps needed
  to make them all 'permissive', ie, accepting any value that can be interpreted
  as the right type, but regardless of what the value is and whether or not it
  matches the constraints on the property (eg, min/max).
  
  {
    sbSimpleAutoLock lock(mMinMaxDurationLock);
    if(value < mMinDuration ||
       value > mMaxDuration) {
      return NS_ERROR_INVALID_ARG;
    }
  }
*/

  nsAutoString out;
  sbSimpleAutoLock lockLocale(mAppLocaleLock);
  
  nsresult rv;

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

  PRExplodedTime referenceTime = {0}, explodedTime = {0};
  PR_ExplodeTime((PRTime) 0, PR_GMTParameters, &referenceTime);
  PR_ExplodeTime((PRTime)value, PR_GMTParameters, &explodedTime);

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

  _retval = out;

  return NS_OK;
}

NS_IMETHODIMP sbDurationPropertyInfo::MakeSearchable(const nsAString & aValue, nsAString & _retval)
{
  nsresult rv;
  PRInt64 value = 0;
  NS_ConvertUTF16toUTF8 narrow(aValue);

  _retval = aValue;
  _retval.StripWhitespace();

  sbSimpleAutoLock lock(mMinMaxDurationLock);

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

NS_IMETHODIMP sbDurationPropertyInfo::GetMinDuration(PRInt64 *aMinDuration)
{
  NS_ENSURE_ARG_POINTER(aMinDuration);

  sbSimpleAutoLock lock(mMinMaxDurationLock);
  *aMinDuration = mMinDuration;

  return NS_OK;
}
NS_IMETHODIMP sbDurationPropertyInfo::SetMinDuration(PRInt64 aMinDuration)
{
  NS_ENSURE_ARG(aMinDuration > -1);

  sbSimpleAutoLock lock(mMinMaxDurationLock);
  mMinDuration = aMinDuration;

  return NS_OK;
}

NS_IMETHODIMP sbDurationPropertyInfo::GetMaxDuration(PRInt64 *aMaxDuration)
{
  NS_ENSURE_ARG_POINTER(aMaxDuration);

  sbSimpleAutoLock lock(mMinMaxDurationLock);
  *aMaxDuration = mMaxDuration;

  return NS_OK;
}
NS_IMETHODIMP sbDurationPropertyInfo::SetMaxDuration(PRInt64 aMaxDuration)
{
  NS_ENSURE_ARG(aMaxDuration > -1);

  sbSimpleAutoLock lock(mMinMaxDurationLock);
  mMaxDuration = aMaxDuration;

  return NS_OK;
}

NS_IMETHODIMP sbDurationPropertyInfo::GetDurationInverse(PRBool *aDurationInverse)
{
  NS_ENSURE_ARG_POINTER(aDurationInverse);
  *aDurationInverse = mDurationInversed;
  return NS_OK;
}
NS_IMETHODIMP sbDurationPropertyInfo::SetDurationInverse(PRBool aDurationInverse)
{
  mDurationInversed = aDurationInverse;
  return NS_OK;
}

NS_IMETHODIMP sbDurationPropertyInfo::GetDurationWithMilliseconds(PRBool *aDurationWithMilliseconds)
{
  NS_ENSURE_ARG_POINTER(aDurationWithMilliseconds);
  *aDurationWithMilliseconds = mDurationDisplayMillisec;
  return NS_OK;
}
NS_IMETHODIMP sbDurationPropertyInfo::SetDurationWithMilliseconds(PRBool aDurationWithMilliseconds)
{
  mDurationDisplayMillisec = aDurationWithMilliseconds;
  return NS_OK;
}
