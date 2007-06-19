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

#include "sbProgressPropertyInfo.h"

#include <nsAutoLock.h>

#define LOCK_OR_FAIL() \
  NS_ENSURE_TRUE(mLock, NS_ERROR_FAILURE); \
  nsAutoLock _lock(mLock)

static const char* kWhitespace="\b\t\r\n ";

NS_IMPL_ISUPPORTS_INHERITED1(sbProgressPropertyInfo, sbTextPropertyInfo,
                                                     sbIProgressPropertyInfo)

sbProgressPropertyInfo::sbProgressPropertyInfo()
: mLock(nsnull)
{
  mLock = nsAutoLock::NewLock("sbProgressPropertyInfo::mLock");
  NS_ASSERTION(mLock, "Failed to create lock!");
}

sbProgressPropertyInfo::~sbProgressPropertyInfo()
{
  if (mLock)
    nsAutoLock::DestroyLock(mLock);
}

NS_IMETHODIMP
sbProgressPropertyInfo::SetModePropertyName(const nsAString& aModePropertyName)
{
  LOCK_OR_FAIL();
  mModePropertyName.Assign(aModePropertyName);
  return NS_OK;
}

NS_IMETHODIMP
sbProgressPropertyInfo::GetModePropertyName(nsAString& aModePropertyName)
{
  LOCK_OR_FAIL();
  aModePropertyName.Assign(mModePropertyName);
  return NS_OK;
}

NS_IMETHODIMP
sbProgressPropertyInfo::GetDisplayPropertiesForValue(const nsAString& aValue,
                                                     nsAString& _retval)
{
  _retval.Truncate();

  nsAutoString value(aValue);
  value.Trim(kWhitespace);

  nsresult rv;

  // This will fail for empty strings.
  PRInt32 intValue = value.ToInteger(&rv);

  if (NS_FAILED(rv) || intValue == -1) {
    _retval.AssignLiteral("progressNotStarted");
  }
  else if (intValue == 101) {
    _retval.AssignLiteral("progressCompleted");
  }
  
  return NS_OK;
}

NS_IMETHODIMP
sbProgressPropertyInfo::Format(const nsAString& aValue,
                               nsAString& _retval)
{
  if (aValue.EqualsLiteral("-1") ||
      aValue.EqualsLiteral("101")) {
    _retval.Truncate();
  }
  else {
    _retval.Assign(aValue);
  }

  return NS_OK;
}
