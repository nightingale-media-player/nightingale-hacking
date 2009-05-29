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

#include "sbDeviceFirmwareUpdater.h"

#include <nsAutoLock.h>

#define SB_DEVICE_FIRMWARE_HANLDER_CONTRACTID_ROOT \
  "@songbirdnest.com/Songbird/Device/Firmware/Handler/"

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceFirmwareUpdater, 
                              sbIDeviceFirmwareUpdater)

sbDeviceFirmwareUpdater::sbDeviceFirmwareUpdater()
: mMonitor(nsnull)
{
}

sbDeviceFirmwareUpdater::~sbDeviceFirmwareUpdater()
{
  if(mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
  }
}

nsresult
sbDeviceFirmwareUpdater::Init()
{
  mMonitor = nsAutoMonitor::NewMonitor("sbDeviceFirmwareUpdater::mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::CheckForUpdate(sbIDevice *aDevice, 
                                        sbIDeviceEventListener *aListener)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::DownloadUpdate(sbIDevice *aDevice, 
                                        PRBool aVerifyFirmwareUpdate, 
                                        sbIDeviceEventListener *aListener)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::VerifyUpdate(sbIDevice *aDevice, 
                                      sbIDeviceFirmwareUpdate *aFirmwareUpdate, 
                                      sbIDeviceEventListener *aListener)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aFirmwareUpdate);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::ApplyUpdate(sbIDevice *aDevice, 
                                     sbIDeviceFirmwareUpdate *aFirmwareUpdate, 
                                     sbIDeviceEventListener *aListener)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aFirmwareUpdate);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::VerifyDevice(sbIDevice *aDevice, 
                                      sbIDeviceEventListener *aListener)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::RegisterHandler(sbIDeviceFirmwareHandler *aFirmwareHandler)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aFirmwareHandler);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::UnregisterHandler(sbIDeviceFirmwareHandler *aFirmwareHandler)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aFirmwareHandler);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::HasHandler(sbIDevice *aDevice, 
                                    PRBool *_retval)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(_retval);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdater::GetHandler(sbIDevice *aDevice, 
                                    sbIDeviceFirmwareHandler **_retval)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(_retval);

  return NS_ERROR_NOT_IMPLEMENTED;
}
