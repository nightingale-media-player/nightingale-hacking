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

#include "sbMockDevice.h"

#include <nsIPrefBranch.h>
#include <nsIPrefService.h>
#include <nsIVariant.h>
#include <nsIWritablePropertyBag2.h>

#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsISupportsUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOMCIDInternal.h>

/* for an actual device, you would probably want to actually sort the prefs on
 * the device itself (and not the mozilla prefs system).  And even if you do end
 * up wanting to store things in the prefs system for some odd reason, you would
 * want to have a unique id per instance of the device and not a hard-coded one.
 */
#define DEVICE_PREF_BRANCH \
  "songbird.devices.mock.00000000-0000-0000-c000-000000000046."

NS_IMPL_THREADSAFE_ADDREF(sbMockDevice)
NS_IMPL_THREADSAFE_RELEASE(sbMockDevice)
NS_INTERFACE_MAP_BEGIN(sbMockDevice)
  NS_INTERFACE_MAP_ENTRY(sbIMockDevice)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(sbIDevice, sbBaseDevice)
  NS_INTERFACE_MAP_ENTRY(sbIDeviceEventTarget)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, sbIDeviceEventTarget)
NS_INTERFACE_MAP_END

sbMockDevice::sbMockDevice()
 : mIsConnected(PR_FALSE)
{
  /* member initializers and constructor code */
}

sbMockDevice::~sbMockDevice()
{
  /* destructor code */
}

/* readonly attribute AString name; */
NS_IMETHODIMP sbMockDevice::GetName(nsAString & aName)
{
  aName.AssignLiteral("Bob's Mock Device");
  return NS_OK;
}

/* readonly attribute nsIDPtr controllerId; */
NS_IMETHODIMP sbMockDevice::GetControllerId(nsID * *aControllerId)
{
  NS_ENSURE_ARG_POINTER(aControllerId);
  
  *aControllerId = (nsID*)NS_Alloc(sizeof(nsID));
  NS_ENSURE_TRUE(*aControllerId, NS_ERROR_OUT_OF_MEMORY);
  **aControllerId = NS_GET_IID(nsISupports);
  return NS_OK;
}

/* readonly attribute nsIDPtr id; */
NS_IMETHODIMP sbMockDevice::GetId(nsID * *aId)
{
  /* note: this is a bad example, since you probably want a different ID per
     instance of a device... and not a constant ;) */
  NS_ENSURE_ARG_POINTER(aId);
  
  *aId = (nsID*)NS_Alloc(sizeof(nsID));
  NS_ENSURE_TRUE(*aId, NS_ERROR_OUT_OF_MEMORY);
  **aId = NS_GET_IID(sbIDevice);
  return NS_OK;
}

/* void connect (); */
NS_IMETHODIMP sbMockDevice::Connect()
{
  NS_ENSURE_STATE(!mIsConnected);
  mIsConnected = PR_TRUE;
  return NS_OK;
}

/* void disconnect (); */
NS_IMETHODIMP sbMockDevice::Disconnect()
{
  NS_ENSURE_STATE(mIsConnected);
  mIsConnected = PR_FALSE;
  return NS_OK;
}

/* readonly attribute boolean connected; */
NS_IMETHODIMP sbMockDevice::GetConnected(PRBool *aConnected)
{
  NS_ENSURE_ARG_POINTER(aConnected);
  *aConnected = mIsConnected;
  return NS_OK;
}

/* readonly attribute boolean threaded; */
NS_IMETHODIMP sbMockDevice::GetThreaded(PRBool *aThreaded)
{
  NS_ENSURE_ARG_POINTER(aThreaded);
  *aThreaded = PR_FALSE;
  return NS_OK;
}

/* nsIVariant getPreference (in AString aPrefName); */
NS_IMETHODIMP sbMockDevice::GetPreference(const nsAString & aPrefName, nsIVariant **_retval)
{
  /* what, you expect a mock device to be actually useful? */
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv;

  nsCOMPtr<nsIPrefService> prefRoot =
    do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefRoot->GetBranch(DEVICE_PREF_BRANCH, getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, rv);
  
  NS_LossyConvertUTF16toASCII prefNameC(aPrefName);
  
  PRInt32 prefType;
  rv = prefBranch->GetPrefType(prefNameC.get(),
                               &prefType);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIWritableVariant> result =
    do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  switch(prefType) {
    case nsIPrefBranch::PREF_INVALID: {
      rv = result->SetAsVoid();
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
    case nsIPrefBranch::PREF_STRING: {
      char *value = nsnull;
      rv = prefBranch->GetCharPref(prefNameC.get(), &value);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = result->SetAsString(value);
      NS_Free(value);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
    case nsIPrefBranch::PREF_INT: {
      PRInt32 value;
      rv = prefBranch->GetIntPref(prefNameC.get(), &value);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = result->SetAsInt32(value);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
    case nsIPrefBranch::PREF_BOOL: {
      PRBool value;
      rv = prefBranch->GetBoolPref(prefNameC.get(), &value);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = result->SetAsBool(value);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
    default: {
      /* wtf? */
      return NS_ERROR_UNEXPECTED;
    }
  }
  
  return CallQueryInterface(result, _retval);
}

/* void setPreference (in AString aPrefName, in nsIVariant aPrefValue); */
NS_IMETHODIMP sbMockDevice::SetPreference(const nsAString & aPrefName, nsIVariant *aPrefValue)
{
  NS_ENSURE_ARG_POINTER(aPrefValue);
  nsresult rv;

  nsCOMPtr<nsIPrefService> prefRoot =
    do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefRoot->GetBranch(DEVICE_PREF_BRANCH, getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, rv);
  
  NS_LossyConvertUTF16toASCII prefNameC(aPrefName);
  
  PRUint16 prefType;
  rv = aPrefValue->GetDataType(&prefType);
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRInt32 oldPrefType;
  rv = prefBranch->GetPrefType(prefNameC.get(), &oldPrefType);
  NS_ENSURE_SUCCESS(rv, rv);
  
  switch(prefType) {
    case nsIDataType::VTYPE_INT8:
    case nsIDataType::VTYPE_INT16:
    case nsIDataType::VTYPE_INT32:
    case nsIDataType::VTYPE_INT64:
    case nsIDataType::VTYPE_UINT8:
    case nsIDataType::VTYPE_UINT16:
    case nsIDataType::VTYPE_UINT32:
    case nsIDataType::VTYPE_UINT64:
    {
      if (oldPrefType != nsIPrefBranch::PREF_INVALID &&
          oldPrefType != nsIPrefBranch::PREF_INT) {
        rv = prefBranch->ClearUserPref(prefNameC.get());
        NS_ENSURE_SUCCESS(rv, rv);
      }
      PRInt32 value;
      rv = aPrefValue->GetAsInt32(&value);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = prefBranch->SetIntPref(prefNameC.get(), value);
      NS_ENSURE_SUCCESS(rv, rv);
  
      /* special case for state */
      if (aPrefName.Equals(NS_LITERAL_STRING("state"))) {
        mState = value;
      }

      break;
    }
    case nsIDataType::VTYPE_BOOL:
    {
      if (oldPrefType != nsIPrefBranch::PREF_INVALID &&
          oldPrefType != nsIPrefBranch::PREF_BOOL) {
        rv = prefBranch->ClearUserPref(prefNameC.get());
        NS_ENSURE_SUCCESS(rv, rv);
      }
      PRBool value;
      rv = aPrefValue->GetAsBool(&value);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = prefBranch->SetBoolPref(prefNameC.get(), value);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
    default:
    {
      if (oldPrefType != nsIPrefBranch::PREF_INVALID &&
          oldPrefType != nsIPrefBranch::PREF_STRING) {
        rv = prefBranch->ClearUserPref(prefNameC.get());
        NS_ENSURE_SUCCESS(rv, rv);
      }
      nsCString value;
      rv = aPrefValue->GetAsACString(value);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = prefBranch->SetCharPref(prefNameC.get(), value.get());
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
  }

  return NS_OK;
}

/* readonly attribute sbIDeviceCapabilities capabilities; */
NS_IMETHODIMP sbMockDevice::GetCapabilities(sbIDeviceCapabilities * *aCapabilities)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute sbIDeviceContent content; */
NS_IMETHODIMP sbMockDevice::GetContent(sbIDeviceContent * *aContent)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIPropertyBag2 parameters; */
NS_IMETHODIMP sbMockDevice::GetParameters(nsIPropertyBag2 * *aParameters)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbMockDevice::GetProperties(sbIDeviceProperties * *theProperties)
{
  return NS_ERROR_NOT_IMPLEMENTED;  
}

NS_IMETHODIMP sbMockDevice::SubmitRequest(PRUint32 aRequest, nsIPropertyBag2 *aRequestParameters)
{
  nsRefPtr<TransferRequest> transferRequest;
  nsresult rv = CreateTransferRequest(aRequest,
                                      aRequestParameters,
                                      getter_AddRefs(transferRequest));
  NS_ENSURE_SUCCESS(rv, rv);
  
  return PushRequest(transferRequest);
}

nsresult sbMockDevice::ProcessRequest()
{
  /* don't process, let the js deal with it */
  return NS_OK;
}

NS_IMETHODIMP sbMockDevice::CancelRequests()
{
  nsID* id;
  nsresult rv = GetId(&id);
  NS_ENSURE_SUCCESS(rv, rv);
  
  char idString[NSID_LENGTH];
  id->ToProvidedString(idString);
  NS_Free(id);
  
  return ClearRequests(NS_ConvertASCIItoUTF16(idString));
}

NS_IMETHODIMP sbMockDevice::Eject()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbMockDevice::GetState(PRUint32 *aState)
{
  return sbBaseDevice::GetState(aState);
}
/****************************** sbIMockDevice ******************************/

#define SET_PROP(type, name) \
  rv = bag->SetPropertyAs ## type(NS_LITERAL_STRING(#name), request->name); \
  NS_ENSURE_SUCCESS(rv, rv);

/* nsIPropertyBag2 PopRequest (); */
NS_IMETHODIMP sbMockDevice::PopRequest(nsIPropertyBag2 **_retval)
{
  // while it's easier to reuse PeekRequest, that sort of defeats the purpose
  // of testing.
  NS_ENSURE_ARG_POINTER(_retval);
  
  nsRefPtr<TransferRequest> request;
  nsresult rv = sbBaseDevice::PopRequest(getter_AddRefs(request));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(request, NS_ERROR_FAILURE);
  
  nsCOMPtr<nsIWritablePropertyBag2> bag =
    do_CreateInstance(NS_HASH_PROPERTY_BAG_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  SET_PROP(Interface, item);
  SET_PROP(Interface, list);
  SET_PROP(Interface, data);
  SET_PROP(Uint32, index);
  SET_PROP(Uint32, otherIndex);
  SET_PROP(Uint32, batchCount);
  SET_PROP(Uint32, batchIndex);
  SET_PROP(Uint32, itemTransferID);
  SET_PROP(Int32, priority);
  
  return CallQueryInterface(bag, _retval);
}

/* nsIPropertyBag2 PeekRequest (); */
NS_IMETHODIMP sbMockDevice::PeekRequest(nsIPropertyBag2 **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  
  nsRefPtr<TransferRequest> request;
  nsresult rv = sbBaseDevice::PeekRequest(getter_AddRefs(request));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(request, NS_ERROR_FAILURE);
  
  nsCOMPtr<nsIWritablePropertyBag2> bag =
    do_CreateInstance(NS_HASH_PROPERTY_BAG_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  SET_PROP(Interface, item);
  SET_PROP(Interface, list);
  SET_PROP(Interface, data);
  SET_PROP(Uint32, index);
  SET_PROP(Uint32, otherIndex);
  SET_PROP(Uint32, batchCount);
  SET_PROP(Uint32, batchIndex);
  SET_PROP(Uint32, itemTransferID);
  SET_PROP(Int32, priority);
  
  return CallQueryInterface(bag, _retval);
}
