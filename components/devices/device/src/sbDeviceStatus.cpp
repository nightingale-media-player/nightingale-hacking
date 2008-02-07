#include "sbDeviceStatus.h"
#include <nsComponentManagerUtils.h>
#include <sbIDataRemote.h>
#include <nsIProxyObjectManager.h>
#include <nsServiceManagerUtils.h>

sbDeviceStatus::sbDeviceStatus(nsAString const & deviceID) : 
  mDeviceID(deviceID)
{
  static nsString const STATE(NS_LITERAL_STRING("status.state"));
  static nsString const OPERATION(NS_LITERAL_STRING("status.operation"));
  static nsString const PROGRESS(NS_LITERAL_STRING("status.progress"));
  static nsString const WORK_CURRENT_COUNT(NS_LITERAL_STRING("status.workcount"));
  static nsString const WORK_TOTAL_COUNT(NS_LITERAL_STRING("status.totalcount"));
  
  nsresult rv = GetDataRemote(STATE,
                              mDeviceID,
                              getter_AddRefs(mStatusRemote));
  GetDataRemote(OPERATION,
                mDeviceID,
                getter_AddRefs(mOperationRemote));
  GetDataRemote(PROGRESS,
                mDeviceID,
                getter_AddRefs(mProgressRemote));
  GetDataRemote(WORK_CURRENT_COUNT,
                mDeviceID,
                getter_AddRefs(mWorkCurrentCountRemote));
  GetDataRemote(WORK_TOTAL_COUNT,
                mDeviceID,
                getter_AddRefs(mWorkTotalCountRemote));
  mProxyObjectManager = do_GetService("@mozilla.org/xpcomproxy;1", &rv);
}

sbDeviceStatus::~sbDeviceStatus()
{
}

nsresult sbDeviceStatus::StateMessage(nsString const & msg)
{
  return mStatusRemote->SetStringValue(msg);
}

nsresult sbDeviceStatus::CurrentOperation(nsString const & operationMessage)
{
  return mOperationRemote->SetStringValue(operationMessage);
}

nsresult sbDeviceStatus::Progress(double percent)
{
  return mProgressRemote->SetIntValue(static_cast<PRInt64>(percent * 100.0 + 0.5));
}

nsresult sbDeviceStatus::WorkItemProgress(PRUint32 current)
{
  return mWorkCurrentCountRemote->SetIntValue(current);
}

nsresult sbDeviceStatus::WorkItemProgressEndCount(PRUint32 count)
{
  return mWorkTotalCountRemote->SetIntValue(count);
}

nsresult sbDeviceStatus::GetDataRemote(const nsAString& aDataRemoteName,
                                       const nsAString& aDataRemotePrefix,
                                       void** appDataRemote)
{
  NS_PRECONDITION(mProxyObjectManager, "Proxy manager must be created");
  nsAutoString                fullDataRemoteName;
  nsCOMPtr<sbIDataRemote>     pDataRemote;
  nsString                    *pNullNSString = NULL;
  nsString                    &nullNSStringRef = *pNullNSString;
  nsresult                    rv;

  /* Get the full data remote name. */
  if (!aDataRemotePrefix.IsEmpty())
  {
      fullDataRemoteName.Assign(aDataRemotePrefix);
      fullDataRemoteName.AppendLiteral(".");
  }
  fullDataRemoteName.Append(aDataRemoteName);

  /* Get the data remote. */
  pDataRemote = do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1",
                                  &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = pDataRemote->Init(fullDataRemoteName, nullNSStringRef);
  NS_ENSURE_SUCCESS(rv, rv);

  /* Make a proxy for the status data remote. */
  rv = mProxyObjectManager->GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                                              NS_GET_IID(sbIDataRemote),
                                              pDataRemote,
                                              nsIProxyObjectManager::INVOKE_ASYNC | 
                                                nsIProxyObjectManager::FORCE_PROXY_CREATION,
                                              appDataRemote);
  NS_ENSURE_SUCCESS(rv, rv);

  return (NS_OK);
}
