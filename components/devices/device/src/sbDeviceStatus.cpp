#include "sbDeviceStatus.h"
#include <nsComponentManagerUtils.h>
#include <sbIDataRemote.h>
#include <nsIProxyObjectManager.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>

NS_IMPL_THREADSAFE_ISUPPORTS0(sbDeviceStatus)

already_AddRefed<sbDeviceStatus> sbDeviceStatus::New(nsAString const & deviceID)
{
  return new sbDeviceStatus(deviceID);
}

sbDeviceStatus::sbDeviceStatus(nsAString const & deviceID) : 
  mDeviceID(deviceID)
{
  /* we need to call init on the main thread, because data remotes are not
     threadsafe (because it eventually hits observer service) */
  // of course, this means we need to hold a ref to ourselves...
  // it ends up on ::New()
  NS_ADDREF(this);
  nsCOMPtr<nsIRunnable> event =
    NS_NEW_RUNNABLE_METHOD(sbDeviceStatus, this, Init);
  NS_DispatchToMainThread(event, NS_DISPATCH_SYNC);
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

void sbDeviceStatus::Init()
{
  NS_NAMED_LITERAL_STRING(STATE, "status.state");
  NS_NAMED_LITERAL_STRING(OPERATION, "status.operation");
  NS_NAMED_LITERAL_STRING(PROGRESS, "status.progress");
  NS_NAMED_LITERAL_STRING(WORK_CURRENT_COUNT, "status.workcount");
  NS_NAMED_LITERAL_STRING(WORK_TOTAL_COUNT, "status.totalcount");

  /* the data remotes need the POM */
  nsCOMPtr<nsIProxyObjectManager> pom =
    do_GetService("@mozilla.org/xpcomproxy;1");
  
  
  
  GetDataRemote(pom,
                STATE,
                mDeviceID,
                getter_AddRefs(mStatusRemote));
  GetDataRemote(pom,
                OPERATION,
                mDeviceID,
                getter_AddRefs(mOperationRemote));
  GetDataRemote(pom,
                PROGRESS,
                mDeviceID,
                getter_AddRefs(mProgressRemote));
  GetDataRemote(pom,
                WORK_CURRENT_COUNT,
                mDeviceID,
                getter_AddRefs(mWorkCurrentCountRemote));
  GetDataRemote(pom,
                WORK_TOTAL_COUNT,
                mDeviceID,
                getter_AddRefs(mWorkTotalCountRemote));
}
nsresult sbDeviceStatus::GetDataRemote(nsIProxyObjectManager* aProxyObjectManager,
                                       const nsAString& aDataRemoteName,
                                       const nsAString& aDataRemotePrefix,
                                       void** appDataRemote)
{
  nsAutoString                fullDataRemoteName;
  nsCOMPtr<sbIDataRemote>     pDataRemote;
  nsString                    nullString;
  nsresult                    rv;

  nullString.SetIsVoid(PR_TRUE);

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
  rv = pDataRemote->Init(fullDataRemoteName, nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  /* Make a proxy for the status data remote. */
  rv = aProxyObjectManager->GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                                              NS_GET_IID(sbIDataRemote),
                                              pDataRemote,
                                              nsIProxyObjectManager::INVOKE_ASYNC | 
                                                nsIProxyObjectManager::FORCE_PROXY_CREATION,
                                              appDataRemote);
  NS_ENSURE_SUCCESS(rv, rv);

  return (NS_OK);
}
