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
#ifndef SBPORTABLEDEVICEEVENTSCALLBACK_H_
#define SBPORTABLEDEVICEEVENTSCALLBACK_H_

#include "sbWPDCommon.h"
#include <nsStringAPI.h>
#include <nsCOMPtr.h>
#include <sbIDevice.h>

class sbWPDMarshall;

/******************************************************************************
 * This class is the listner for device events. It listens on a per device
 * basis. There is one listener per device.
 */
class sbPortableDeviceEventsCallback : public IPortableDeviceEventCallback
{
public:
  /**
   * Initialize the listener with the marshaller object and device
   */
  static sbPortableDeviceEventsCallback * New(sbWPDMarshall * marshall,
                                              sbIDevice * device,
                                              nsAString const & deviceID);  
  /**
   * Basic QI implementation
   */
  HRESULT __stdcall QueryInterface(
      REFIID riid,
      LPVOID* ppvObj);
  /**
   * Basic AddRef implementation
   */
  ULONG __stdcall AddRef()
  {
    InterlockedIncrement((long*) &mRefCnt);
    return mRefCnt;
  }
  /**
   * Basic Release implementation
   */
  ULONG __stdcall Release()
  {
    ULONG ulRefCount = mRefCnt - 1;

    if (InterlockedDecrement((long*) &mRefCnt) == 0) {
      delete this;
      return 0;
    }
    return ulRefCount;
  }
  /**
   * Man event implementation. This dispatches into the Songbird device manager
   */
  HRESULT __stdcall OnEvent(IPortableDeviceValues* pEventParameters);
protected:
  /**
   * Initialize the listener with the marshaller object and device
   */
  sbPortableDeviceEventsCallback(sbWPDMarshall * marshall,
                                 sbIDevice * device,
                                 nsAString const & deviceID) :
    mRefCnt(0),
    mMarshall(marshall),
    mSBDevice(device),
    mDeviceID(deviceID)
  {
    NS_ASSERTION(marshall, "marshall cannot be null");
    NS_ASSERTION(device, "device cannot be null");
  }
  /**
   * Release our reference to the marshaller and the device (implict)
   */
  ~sbPortableDeviceEventsCallback();
private:
  ULONG mRefCnt;
  // We need access to the concrete class so we'll
  // manage the release this pointer.
  sbWPDMarshall * mMarshall;
  nsCOMPtr<sbIDevice> mSBDevice;
  nsString mDeviceID;
};

#endif /*SBPORTABLEDEVICEEVENTSCALLBACK_H_*/
