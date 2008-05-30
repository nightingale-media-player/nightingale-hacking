/* vim: sw=2 : */
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

#include "sbDeviceStatus.h"

#include <nsComponentManagerUtils.h>
#include <sbIDataRemote.h>
#include <sbIDevice.h>
#include <nsIProxyObjectManager.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceStatus, sbIDeviceStatus)

sbDeviceStatus::sbDeviceStatus()
{
  /* member initializers and constructor code */
  mCurrentState = sbIDevice::STATE_IDLE;
  mCurrentSubState = sbIDevice::STATE_IDLE;
}

sbDeviceStatus::~sbDeviceStatus()
{
  /* destructor code */
}

/* attribute unsigned long currentState; */
NS_IMETHODIMP
sbDeviceStatus::GetCurrentState(PRUint32 *aCurrentState)
{
  NS_ENSURE_ARG_POINTER(aCurrentState);
  *aCurrentState = mCurrentState;
  return NS_OK;
}
NS_IMETHODIMP
sbDeviceStatus::SetCurrentState(PRUint32 aCurrentState)
{
  mCurrentState = aCurrentState;
  return NS_OK;
}

/* attribute unsigned long currentSubState; */
NS_IMETHODIMP
sbDeviceStatus::GetCurrentSubState(PRUint32 *aCurrentSubState)
{
  NS_ENSURE_ARG_POINTER(aCurrentSubState);
  *aCurrentSubState = mCurrentSubState;
  return NS_OK;
}
NS_IMETHODIMP
sbDeviceStatus::SetCurrentSubState(PRUint32 aCurrentSubState)
{
  mCurrentSubState = aCurrentSubState;
  return NS_OK;
}

/* attribute AString stateMessage; */
NS_IMETHODIMP
sbDeviceStatus::GetStateMessage(nsAString & aStateMessage)
{
  return mStatusRemote->GetStringValue(aStateMessage);
}
NS_IMETHODIMP
sbDeviceStatus::SetStateMessage(const nsAString & aStateMessage)
{
  return mStatusRemote->SetStringValue(aStateMessage);
}

/* attribute AString currentOperation; */
NS_IMETHODIMP
sbDeviceStatus::GetCurrentOperation(nsAString & aCurrentOperation)
{
  return mOperationRemote->GetStringValue(aCurrentOperation);
}
NS_IMETHODIMP
sbDeviceStatus::SetCurrentOperation(const nsAString & aCurrentOperation)
{
  return mOperationRemote->SetStringValue(aCurrentOperation);
}

/* attribute double progress; */
NS_IMETHODIMP
sbDeviceStatus::GetProgress(double *aProgress)
{
  PRInt64 curProgress;
  nsresult rv;
  rv = mProgressRemote->GetIntValue(&curProgress);
  NS_ENSURE_SUCCESS(rv, rv);
  
  *aProgress = (curProgress / 100.0 - 0.5);
  return NS_OK;
}
NS_IMETHODIMP
sbDeviceStatus::SetProgress(double aProgress)
{
  return mProgressRemote->SetIntValue(static_cast<PRInt64>(aProgress * 100.0 + 0.5));
}

/* attribute PRUint32 workItemProgress; */
NS_IMETHODIMP
sbDeviceStatus::GetWorkItemProgress(PRInt64 *aWorkItemProgress)
{
  return mWorkCurrentCountRemote->GetIntValue(aWorkItemProgress);
}
NS_IMETHODIMP
sbDeviceStatus::SetWorkItemProgress(PRInt64 aWorkItemProgress)
{
  return mWorkCurrentCountRemote->SetIntValue(aWorkItemProgress);
}

/* attribute PRUint32 workItemProgressEndCount; */
NS_IMETHODIMP
sbDeviceStatus::GetWorkItemProgressEndCount(PRInt64 *aWorkItemProgressEndCount)
{
  return mWorkTotalCountRemote->GetIntValue(aWorkItemProgressEndCount);
}
NS_IMETHODIMP
sbDeviceStatus::SetWorkItemProgressEndCount(PRInt64 aWorkItemProgressEndCount)
{
  return mWorkTotalCountRemote->SetIntValue(aWorkItemProgressEndCount);
}

/* attribute sbIMediaItem mediaItem; */
NS_IMETHODIMP
sbDeviceStatus::GetMediaItem(sbIMediaItem * *aMediaItem)
{
  if (mItem) {
    NS_IF_ADDREF(*aMediaItem = mItem);
  }
  return NS_OK;
}
NS_IMETHODIMP
sbDeviceStatus::SetMediaItem(sbIMediaItem * aMediaItem)
{
  mItem = aMediaItem;
  return NS_OK;
}

/* attribute sbIMediaList mediaList; */
NS_IMETHODIMP
sbDeviceStatus::GetMediaList(sbIMediaList * *aMediaList)
{
  if (mList) {
    NS_IF_ADDREF(*aMediaList = mList);
  }
  return NS_OK;
}
NS_IMETHODIMP
sbDeviceStatus::SetMediaList(sbIMediaList * aMediaList)
{
  mList = aMediaList;
  return NS_OK;
}

NS_IMETHODIMP sbDeviceStatus::Init(const nsAString& aDeviceID)
{
  nsresult rv;
  mDeviceID.Assign(aDeviceID);
  
  NS_NAMED_LITERAL_STRING(STATE, "status.state");
  NS_NAMED_LITERAL_STRING(OPERATION, "status.operation");
  NS_NAMED_LITERAL_STRING(PROGRESS, "status.progress");
  NS_NAMED_LITERAL_STRING(WORK_CURRENT_COUNT, "status.workcount");
  NS_NAMED_LITERAL_STRING(WORK_TOTAL_COUNT, "status.totalcount");

  /* the data remotes need the POM */
  nsCOMPtr<nsIProxyObjectManager> pom =
    do_GetService("@mozilla.org/xpcomproxy;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = GetDataRemote(pom,
                STATE,
                mDeviceID,
                getter_AddRefs(mStatusRemote));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetDataRemote(pom,
                OPERATION,
                mDeviceID,
                getter_AddRefs(mOperationRemote));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetDataRemote(pom,
                PROGRESS,
                mDeviceID,
                getter_AddRefs(mProgressRemote));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetDataRemote(pom,
                WORK_CURRENT_COUNT,
                mDeviceID,
                getter_AddRefs(mWorkCurrentCountRemote));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetDataRemote(pom,
                WORK_TOTAL_COUNT,
                mDeviceID,
                getter_AddRefs(mWorkTotalCountRemote));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
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
  rv = aProxyObjectManager->GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                              NS_GET_IID(sbIDataRemote),
                                              pDataRemote,
                                              nsIProxyObjectManager::INVOKE_ASYNC | 
                                                nsIProxyObjectManager::FORCE_PROXY_CREATION,
                                              appDataRemote);
  NS_ENSURE_SUCCESS(rv, rv);

  return (NS_OK);
}
