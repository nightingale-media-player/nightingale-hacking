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
#ifndef SBPORTABLEDEVICEPROPERTIESBULKIMPORTCALLBACK_H_
#define SBPORTABLEDEVICEPROPERTIESBULKIMPORTCALLBACK_H_

#include "sbWPDCommon.h"
#include <nsStringAPI.h>

#include "sbPortableDevicePropertiesBulkCallback.h"

class sbPortableDevicePropertiesBulkImportCallback : public sbPortableDevicePropertiesBulkCallback
{
public:

  HRESULT __stdcall onStart(REFGUID aContext);
  HRESULT __stdcall onProgress(REFGUID aContext, IPortableDeviceValuesCollection *aResults);
  HRESULT __stdcall onEnd(REFGUID aContext);

protected:
  sbPortableDevicePropertiesBulkImportCallback(sbWPDDevice * aDevice)
  : sbPortableDevicePropertiesBulkCallback(aDevice)
  {
    
  }

  virtual ~sbPortableDevicePropertiesBulkImportCallback() { };

};

#endif //SBPORTABLEDEVICEPROPERTIESBULKIMPORTCALLBACK_H_
