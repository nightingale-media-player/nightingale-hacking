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

#include "sbPortableDeviceEventsCallback.h"
#include <sbIDeviceEvent.h>
#include "sbWPDCommon.h"
#include "sbWPDMarshall.h"

sbPortableDeviceEventsCallback * sbPortableDeviceEventsCallback::New(sbWPDMarshall * marshall,
                                                                     sbIDevice * device,
                                                                     nsAString const & deviceID)
{
  return new sbPortableDeviceEventsCallback(marshall,
                                            device,
                                            deviceID);
}

sbPortableDeviceEventsCallback::~sbPortableDeviceEventsCallback()
{
  NS_ASSERTION(mRefCnt == 0, "sbPortableDeviceEventsCallback being deleted with live references");
}

HRESULT __stdcall sbPortableDeviceEventsCallback::QueryInterface(
    REFIID riid,
    LPVOID* ppvObj)
{
  HRESULT hr = S_OK;
  if (ppvObj == NULL)
  {
    hr = E_INVALIDARG;
    return hr;
  }

  if ((riid == IID_IUnknown) ||
      (riid == IID_IPortableDeviceEventCallback))
  {
    AddRef();
    *ppvObj = this;
  }
  else
  {
    *ppvObj = NULL;
    hr = E_NOINTERFACE;
  }
  return hr;
}

static nsString GetStringProperty(IPortableDeviceValues * pEventParameters,
                                  PROPERTYKEY const & key)
{
  nsString result;
  LPWSTR value = NULL;
  if (SUCCEEDED(pEventParameters->GetStringValue(key, &value))) {
    result = value;
    ::CoTaskMemFree(value);
  }
  return result;
}

HRESULT __stdcall sbPortableDeviceEventsCallback::OnEvent(
    IPortableDeviceValues* pEventParameters)
{
  GUID eventID;
  if (SUCCEEDED(pEventParameters->GetGuidValue(WPD_EVENT_PARAMETER_EVENT_ID,&eventID))) {
    PRUint32 sbEventID = 0;
    // Translate WPD event ID to the Songbird event ID
    if (eventID == WPD_EVENT_DEVICE_REMOVED) {
      mMarshall->RemoveDevice(mDeviceID);
      sbEventID = sbIDeviceEvent::EVENT_DEVICE_REMOVED;
    }
    else if (eventID == WPD_EVENT_OBJECT_ADDED) {
      sbEventID = sbIDeviceEvent::EVENT_DEVICE_MEDIA_INSERTED;
    }
    else if (eventID == WPD_EVENT_OBJECT_REMOVED) {
      sbEventID = sbIDeviceEvent::EVENT_DEVICE_MEDIA_REMOVED;
    }
    else if (eventID == WPD_EVENT_OBJECT_UPDATED) {
      // Not handled for now
    }
    else if (eventID == WPD_EVENT_DEVICE_RESET) {
      sbEventID = sbIDeviceEvent::EVENT_DEVICE_RESET;
    }
    else if (eventID == WPD_EVENT_DEVICE_CAPABILITIES_UPDATED) {
      // Not handled for now
    }
    else if (eventID == WPD_EVENT_STORAGE_FORMAT) {
      // Not handled for now        
    }
    else if (eventID == WPD_EVENT_OBJECT_TRANSFER_REQUESTED) {
      // Not handled for now
    }
    // If we can handle this event, dispatch it
    if (sbEventID > 0) {
      sbWPDCreateAndDispatchEvent(mMarshall,
                             mSBDevice,
                             sbEventID);
    }
  }
  return S_OK;
}
