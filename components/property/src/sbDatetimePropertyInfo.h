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

#ifndef __SBDATETIMEPROPERTYINFO_H__
#define __SBDATETIMEPROPERTYINFO_H__

#include <sbIPropertyManager.h>
#include "sbPropertyInfo.h"

#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <nsDateTimeFormatCID.h>
#include <nsIDateTimeFormat.h>
#include <nsILocale.h>
#include <nsILocaleService.h>
#include <nsAutoLock.h>

class sbDatetimePropertyInfo : public sbPropertyInfo,
                               public sbIDatetimePropertyInfo

{
public:
  NS_DECL_ISUPPORTS
  NS_FORWARD_SBIPROPERTYINFO_NOVALIDATE_NOFORMAT(sbPropertyInfo::)
  NS_DECL_SBIDATETIMEPROPERTYINFO

  sbDatetimePropertyInfo();
  virtual ~sbDatetimePropertyInfo();

  NS_IMETHOD Validate(const nsAString & aValue, PRBool *_retval);
  NS_IMETHOD Format(const nsAString & aValue, nsAString & _retval);
  NS_IMETHOD MakeSortable(const nsAString & aValue, nsAString & _retval);
  
protected:
  PRBool  mDurationInversed;
  PRBool  mDurationDisplayMillisec;

  PRLock* mTimeTypeLock;
  PRInt32 mTimeType;

  PRLock* mMinMaxTimeLock;
  PRInt64 mMinTime;
  PRInt64 mMaxTime;

  PRLock* mAppLocaleLock;
  nsCOMPtr<nsILocale> mAppLocale;
  
  PRLock* mDateTimeFormatLock;
  nsCOMPtr<nsIDateTimeFormat> mDateTimeFormat;

};

#endif /* __SBDATETIMEPROPERTYINFO_H__ */