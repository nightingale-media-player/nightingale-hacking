/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbCDDeviceMarshall.h"
#include "sbCDDeviceDefines.h"

#include <sbICDDeviceService.h>
#include <sbIDeviceController.h>
#include <sbIDeviceManager.h>
#include <sbIDeviceEvent.h>
#include <sbIDeviceEventTarget.h>

#include <nsComponentManagerUtils.h>
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsServiceManagerUtils.h>
#include <nsMemory.h>


NS_IMPL_THREADSAFE_ADDREF(sbCDDeviceMarshall)
NS_IMPL_THREADSAFE_RELEASE(sbCDDeviceMarshall)
NS_IMPL_QUERY_INTERFACE2_CI(sbCDDeviceMarshall,
                            sbIDeviceMarshall,
                            sbICDDeviceListener)
NS_IMPL_CI_INTERFACE_GETTER2(sbCDDeviceMarshall,
                             sbIDeviceMarshall,
                             sbICDDeviceListener)

// nsIClassInfo implementation.
NS_DECL_CLASSINFO(sbCDDeviceMarshall)
NS_IMPL_THREADSAFE_CI(sbCDDeviceMarshall)

sbCDDeviceMarshall::sbCDDeviceMarshall()
  : sbBaseDeviceMarshall(NS_LITERAL_CSTRING(SB_DEVICE_CONTROLLER_CATEGORY))
{
}

sbCDDeviceMarshall::~sbCDDeviceMarshall()
{
}

nsresult
sbCDDeviceMarshall::Init()
{
  nsresult rv;
  nsCOMPtr<sbIDeviceManager2> deviceMgr =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // TODO: Check to see if there is a implementation for the cd device service.

  return NS_OK;
}

nsresult
sbCDDeviceMarshall::CreateAndDispatchDeviceManagerEvent(PRUint32 aType,
                                                        nsIVariant *aData,
                                                        nsISupports *aOrigin,
                                                        PRBool aAsync)
{
  nsresult rv;

  // Get the device manager.
  nsCOMPtr<sbIDeviceManager2> manager =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Use the device manager as the event target.
  nsCOMPtr<sbIDeviceEventTarget> eventTarget = do_QueryInterface(manager, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the event.
  nsCOMPtr<sbIDeviceEvent> event;
  rv = manager->CreateEvent(aType, aData, aOrigin, getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  // Dispatch the event.
  PRBool dispatched;
  rv = eventTarget->DispatchEvent(event, aAsync, &dispatched);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIDeviceMarshall

NS_IMETHODIMP
sbCDDeviceMarshall::LoadControllers(sbIDeviceControllerRegistrar *aRegistrar)
{
  NS_ENSURE_ARG_POINTER(aRegistrar);

  RegisterControllers(aRegistrar);

  return NS_OK;
}

NS_IMETHODIMP
sbCDDeviceMarshall::BeginMonitoring()
{
  //
  // XXXkreeger Write Me (use the mock cd device for now).
  //
  return NS_OK;
}

NS_IMETHODIMP
sbCDDeviceMarshall::StopMonitoring()
{
  //
  // XXXkreeger Write Me (use the mock cd device for now).
  //

  // Remove any device references.

  return NS_OK;
}

NS_IMETHODIMP
sbCDDeviceMarshall::GetId(nsID **aId)
{
  NS_ENSURE_ARG_POINTER(aId);

  nsID *pid = static_cast<nsID *>(NS_Alloc(sizeof(nsID)));
  NS_ENSURE_TRUE(pid, NS_ERROR_OUT_OF_MEMORY);
  *pid = mMarshallCID;

  *aId = pid;
  return NS_OK;
}

NS_IMETHODIMP
sbCDDeviceMarshall::GetName(nsAString & aName)
{
  aName.Assign(NS_LITERAL_STRING(SB_CDDEVICE_MARSHALL_NAME));
  return NS_OK;
}

//------------------------------------------------------------------------------
// sbICDDeviceListener

NS_IMETHODIMP
sbCDDeviceMarshall::OnDeviceRemoved(sbICDDevice *aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDeviceMarshall::OnMediaInserted(sbICDDevice *aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDeviceMarshall::OnMediaEjected(sbICDDevice *aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  return NS_OK;
}

