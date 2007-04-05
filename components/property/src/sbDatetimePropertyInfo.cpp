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

#include "sbDatetimePropertyInfo.h"

NS_IMPL_ISUPPORTS_INHERITED2(sbDatetimePropertyInfo,
                             sbPropertyInfo,
                             sbIPropertyInfo,
                             sbIDatetimePropertyInfo)

sbDatetimePropertyInfo::sbDatetimePropertyInfo()
: mTimeTypeLock(nsnull)
, mTimeType(sbIDatetimePropertyInfo::TIMETYPE_UNINITIALIZED)
, mMinMaxTimeLock(nsnull)
, mMinTime(-1)
, mMaxTime(-1)
{
  mType = NS_LITERAL_STRING("datetime");

  mTimeTypeLock = PR_NewLock();
  NS_ASSERTION(mTimeTypeLock, 
    "sbDatetimePropertyInfo::mTimeTypeLock failed to create lock!");

  mMinMaxTimeLock = PR_NewLock();
  NS_ASSERTION(mMinMaxTimeLock,
    "sbDatetimePropertyInfo::mMinMaxTimeLock failed to create lock!");
}

sbDatetimePropertyInfo::~sbDatetimePropertyInfo()
{
  if(mTimeTypeLock) {
    PR_DestroyLock(mTimeTypeLock);
  }
  if(mMinMaxTimeLock) {
    PR_DestroyLock(mMinMaxTimeLock);
  }
}

NS_IMETHODIMP sbDatetimePropertyInfo::Validate(const nsAString & aValue, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbDatetimePropertyInfo::Format(const nsAString & aValue, nsAString & _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbDatetimePropertyInfo::MakeSortable(const nsAString & aValue, nsAString & _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbDatetimePropertyInfo::GetTimeType(PRInt32 *aTimeType)
{
  NS_ENSURE_ARG_POINTER(aTimeType);
  
  nsAutoLock lock(mTimeTypeLock);
  if(mTimeType != sbIDatetimePropertyInfo::TIMETYPE_UNINITIALIZED) {
    *aTimeType = mTimeType;
    return NS_OK;
  }

  return NS_ERROR_NOT_INITIALIZED;
}
NS_IMETHODIMP sbDatetimePropertyInfo::SetTimeType(PRInt32 aTimeType)
{
  NS_ENSURE_ARG(aTimeType > 0 && 
    aTimeType <= sbDatetimePropertyInfo::TIMETYPE_DATETIME);

  nsAutoLock lock(mTimeTypeLock);
  if(mTimeType == sbIDatetimePropertyInfo::TIMETYPE_UNINITIALIZED) {
    mTimeType = aTimeType;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbDatetimePropertyInfo::GetMinTime(PRInt64 *aMinTime)
{
  NS_ENSURE_ARG_POINTER(aMinTime);
  
  nsAutoLock lock(mMinMaxTimeLock);
  if(mMinTime != -1) {
    *aMinTime = mMinTime;
    return NS_OK;
  }
  
  return NS_ERROR_NOT_INITIALIZED;
}
NS_IMETHODIMP sbDatetimePropertyInfo::SetMinTime(PRInt64 aMinTime)
{
  NS_ENSURE_ARG(aMinTime > -1);

  nsAutoLock lock(mMinMaxTimeLock);
  if(mMinTime == -1) {
    mMinTime = aMinTime;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbDatetimePropertyInfo::GetMaxTime(PRInt64 *aMaxTime)
{
  NS_ENSURE_ARG_POINTER(aMaxTime);

  nsAutoLock lock(mMinMaxTimeLock);
  if(mMaxTime != -1) {
    *aMaxTime = mMaxTime;
    return NS_OK;
  }

  return NS_ERROR_NOT_INITIALIZED;

}
NS_IMETHODIMP sbDatetimePropertyInfo::SetMaxTime(PRInt64 aMaxTime)
{
  NS_ENSURE_ARG(aMaxTime > -1);

  nsAutoLock lock(mMinMaxTimeLock);
  if(mMaxTime == -1) {
    mMaxTime = aMaxTime;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}