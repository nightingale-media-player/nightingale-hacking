
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
