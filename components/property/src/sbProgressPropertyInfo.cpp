
#include "sbProgressPropertyInfo.h"

#include <nsAutoLock.h>

#define LOCK_OR_FAIL() \
  NS_ENSURE_TRUE(mLock, NS_ERROR_FAILURE); \
  nsAutoLock _lock(mLock)

NS_IMPL_ISUPPORTS_INHERITED1(sbProgressPropertyInfo, sbNumberPropertyInfo,
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
