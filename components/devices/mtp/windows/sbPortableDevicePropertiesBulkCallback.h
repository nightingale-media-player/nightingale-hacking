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
#ifndef SBPORTABLEDEVICEPROPERTIESBULKCALLBACK_H_
#define SBPORTABLEDEVICEPROPERTIESBULKCALLBACK_H_

#include "sbWPDCommon.h"
#include <nsStringAPI.h>

class sbWPDDevice;

class sbPortableDevicePropertiesBulkCallback : public IPortableDevicePropertiesBulkCallback
{
public:
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
    return InterlockedIncrement((long*) &mRefCnt);
  }
  
  /**
   * Basic Release implementation
   */
  ULONG __stdcall Release()
  {
    ULONG ulRefCount(mRefCnt);

    if ( (ulRefCount = InterlockedDecrement((long*) &mRefCnt)) == 0) {
      delete this;
    }

    return ulRefCount;
  }

  virtual HRESULT onStart(REFGUID aContext) = 0;
  virtual HRESULT onProgress(REFGUID aContext, IPortableDeviceValuesCollection *aResults) = 0;
  virtual HRESULT onEnd(REFGUID aContext) = 0;

protected:
  sbPortableDevicePropertiesBulkCallback(sbWPDDevice * aDevice)
  : mRefCnt(0)
  , mDevice(aDevice)
  {
    NS_ASSERTION(aDevice, "marshall cannot be null");
  }

  ~sbPortableDevicePropertiesBulkCallback();

private:
  ULONG mRefCnt;
  nsRefPtr<sbWPDDevice> mDevice;
};

#endif //SBPORTABLEDEVICEPROPERTIESBULKCALLBACK_H_
