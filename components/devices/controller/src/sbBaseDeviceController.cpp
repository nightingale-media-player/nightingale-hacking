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

#include <nsAutoLock.h>
#include "sbBaseDeviceController.h"

sbBaseDeviceController::sbBaseDeviceController()
: mMonitor(nsnull) {
}

sbBaseDeviceController::~sbBaseDeviceController() {
  if(mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
  }
}

nsresult sbBaseDeviceController::Init() {
  mMonitor = 
    nsAutoMonitor::NewMonitor("sbBaseDeviceController.mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  PRBool succeeded = mDevices.Init();
  NS_ENSURE_TRUE(succeeded, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult sbBaseDeviceController::GetControllerIDInternal(nsID &aID) {
  nsAutoMonitor mon(mMonitor);
  aID = mControllerID;
  return NS_OK;
}
nsresult sbBaseDeviceController::SetControllerIDInternal(const nsID &aID) {
  nsAutoMonitor mon(mMonitor);
  mControllerID = aID;
  return NS_OK;
}

nsresult sbBaseDeviceController::GetControllerNameInternal(nsString &aName) {
  nsAutoMonitor mon(mMonitor);
  aName = mControllerName ;
  return NS_OK;
}
nsresult sbBaseDeviceController::SetControllerNameInternal(const nsString &aName) {
  nsAutoMonitor mon(mMonitor);
  mControllerName = aName;
  return NS_OK;
}

nsresult sbBaseDeviceController::GetMarshallIDInternal(nsID &aID) {
  nsAutoMonitor mon(mMonitor);
  aID = mMarshallID;
  return NS_OK;
}
nsresult sbBaseDeviceController::SetMarshallIDInternal(const nsID &aID) {
  nsAutoMonitor mon(mMonitor);
  mMarshallID = aID;
  return NS_OK;
}

nsresult sbBaseDeviceController::AddDeviceInternal(sbIDevice *aDevice) {
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;
  nsID* id;
  rv = aDevice->GetId(&id);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_ARG_POINTER(id);

  nsAutoMonitor mon(mMonitor);
  
  PRBool succeeded = mDevices.Put(*id, aDevice);
  NS_Free(id);
  
  return succeeded ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}
nsresult sbBaseDeviceController::RemoveDeviceInternal(sbIDevice *aDevice) {
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;
  nsID* id;
  rv = aDevice->GetId(&id);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_ARG_POINTER(id);

  nsAutoMonitor mon(mMonitor);

  mDevices.Remove(*id);
  NS_Free(id);

  return NS_OK;
}
