/* vim: set sw=2 :miv */
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

#include "sbBaseDeviceController.h"

#include <mozilla/ReentrantMonitor.h>
#include <nsComponentManagerUtils.h>

#include <sbDebugUtils.h>

template<class T>
PLDHashOperator sbBaseDeviceController::EnumerateIntoArray(const nsID& aKey,
                                                           T* aData,
                                                           void* aArray)
{
  nsIMutableArray *array = (nsIMutableArray*)aArray;
  nsresult rv;
  nsCOMPtr<nsISupports> supports = do_QueryInterface(aData, &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  rv = array->AppendElement(aData, false);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

template<class T>
PLDHashOperator sbBaseDeviceController::EnumerateConnectAll(const nsID& aKey,
                                                            T* aData,
                                                            void* aArray)
{
  nsIMutableArray *array = (nsIMutableArray*)aArray;
  nsresult rv;
  nsCOMPtr<nsISupports> supports = do_QueryInterface(aData, &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  rv = aData->Connect();
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  rv = array->AppendElement(aData, false);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

template<class T>
PLDHashOperator sbBaseDeviceController::EnumerateDisconnectAll(const nsID& aKey,
                                                               T* aData,
                                                               void* aArray)
{
  nsIMutableArray *array = (nsIMutableArray*)aArray;
  nsresult rv;
  nsCOMPtr<nsISupports> supports = do_QueryInterface(aData, &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  rv = aData->Disconnect();
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  rv = array->AppendElement(aData, false);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

sbBaseDeviceController::sbBaseDeviceController()
  : mMonitor("mMonitor")
{
  PRBool SB_UNUSED_IN_RELEASE(succeeded) = mDevices.Init();
  NS_ASSERTION(succeeded, "Failed to initialize hashtable");
}

sbBaseDeviceController::~sbBaseDeviceController()
{

}

nsresult 
sbBaseDeviceController::GetControllerIdInternal(nsID &aID)
{
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  aID = mControllerID;
  return NS_OK;
}

nsresult 
sbBaseDeviceController::SetControllerIdInternal(const nsID &aID)
{
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  mControllerID = aID;
  return NS_OK;
}

nsresult 
sbBaseDeviceController::GetControllerNameInternal(nsAString &aName)
{
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  aName = mControllerName;
  return NS_OK;
}

nsresult 
sbBaseDeviceController::SetControllerNameInternal(const nsAString &aName)
{
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  mControllerName = aName;
  return NS_OK;
}

nsresult 
sbBaseDeviceController::GetMarshallIdInternal(nsID &aID)
{
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  aID = mMarshallID;
  return NS_OK;
}

nsresult 
sbBaseDeviceController::SetMarshallIdInternal(const nsID &aID)
{
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  mMarshallID = aID;
  return NS_OK;
}

nsresult 
sbBaseDeviceController::AddDeviceInternal(sbIDevice *aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;
  nsID* id;
  rv = aDevice->GetId(&id);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_ARG_POINTER(id);

  PRBool succeeded;

  {
    mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
    succeeded = mDevices.Put(*id, aDevice);
  }

  NS_Free(id);

  return succeeded ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult 
sbBaseDeviceController::RemoveDeviceInternal(sbIDevice *aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;
  nsID* id;
  rv = aDevice->GetId(&id);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_ARG_POINTER(id);

  {
    mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
    mDevices.Remove(*id);
  }

  NS_Free(id);

  return NS_OK;
}

nsresult
sbBaseDeviceController::GetDeviceInternal(const nsID * aID,
                                          sbIDevice* *aDevice)
{
  NS_ENSURE_ARG_POINTER(aID);
  NS_ENSURE_ARG_POINTER(aDevice);

  PRBool succeeded;

  {
    mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
    succeeded = mDevices.Get(*aID, aDevice);
  }

  return succeeded ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

nsresult 
sbBaseDeviceController::GetDevicesInternal(nsIArray* *aDevices)
{
  NS_ENSURE_ARG_POINTER(aDevices);

  nsresult rv;
  nsCOMPtr<nsIMutableArray> array = 
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 count;

  {
    mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

    count = mDevices.EnumerateRead(sbBaseDeviceController::EnumerateIntoArray,
                                   array.get());
  }

  // we can't trust the count returned from EnumerateRead because that won't
  // tell us about erroring on the last element
  rv = array->GetLength(&count);
  NS_ENSURE_SUCCESS(rv, rv);
  if (count < mDevices.Count()) {
    return NS_ERROR_FAILURE;
  }

  return CallQueryInterface(array, aDevices);
}

nsresult 
sbBaseDeviceController::ControlsDeviceInternal(sbIDevice *aDevice, 
                                               PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(_retval);

  nsID *id = nsnull;
  nsresult rv = aDevice->GetId(&id);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevice> device;
  rv = GetDeviceInternal(id, getter_AddRefs(device));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_Free(id);
  *_retval = (device != nsnull) ? PR_TRUE : PR_FALSE;

  return NS_OK;
}

nsresult 
sbBaseDeviceController::ConnectDevicesInternal()
{
  nsresult rv;
  PRUint32 count;
  nsCOMPtr<nsIMutableArray> array;

  {
    mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

    array =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    count = mDevices.EnumerateRead(sbBaseDeviceController::EnumerateConnectAll,
                                   array.get());

  }

  // we can't trust the count returned from EnumerateRead because that won't
  // tell us about erroring on the last element
  rv = array->GetLength(&count);
  NS_ENSURE_SUCCESS(rv, rv);
  if (count < mDevices.Count()) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult 
sbBaseDeviceController::DisconnectDevicesInternal()
{
  nsresult rv;
  PRUint32 count;
  nsCOMPtr<nsIMutableArray> array;

  {
    mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

    array =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    count = mDevices.EnumerateRead(sbBaseDeviceController::EnumerateDisconnectAll,
                                   array.get());
  }

  // we can't trust the count returned from EnumerateRead because that won't
  // tell us about erroring on the last element
  rv = array->GetLength(&count);
  NS_ENSURE_SUCCESS(rv, rv);
  if (count < mDevices.Count()) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult 
sbBaseDeviceController::ReleaseDeviceInternal(sbIDevice *aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsID *id = nsnull;
  nsresult rv = aDevice->GetId(&id);
  NS_ENSURE_SUCCESS(rv, rv);

  aDevice->Disconnect();

  {
    mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
    mDevices.Remove(*id);
  }

  NS_Free(id);
  
  return NS_OK;
}

nsresult 
sbBaseDeviceController::ReleaseDevicesInternal()
{
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  mDevices.Clear();
  return NS_OK;
}
