/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2010 POTI, Inc.
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

#ifndef __SB_MOCKDEVICEFIRMWAREHANDLER_H__
#define __SB_MOCKDEVICEFIRMWAREHANDLER_H__

#include "sbBaseDeviceFirmwareHandler.h"
#include <sbPIMockFirmwareHandlerURLService.h>
#include <nsIStreamListener.h>


class sbMockDeviceFirmwareHandler : public sbBaseDeviceFirmwareHandler,
                                    public nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER

  sbMockDeviceFirmwareHandler();

  virtual nsresult OnInit();
  virtual nsresult OnGetCurrentFirmwareVersion(PRUint32 *aCurrentFirmwareVersion);
  virtual nsresult OnGetCurrentFirmwareReadableVersion(nsAString &aCurrentFirmwareReadableVersion);
  virtual nsresult OnGetRecoveryMode(bool *aRecoveryMode);

  virtual nsresult OnGetDeviceModelNumber(nsAString &aModelNumber);
  virtual nsresult OnGetDeviceModelVersion(nsAString &aModelVersion);
  virtual nsresult OnGetDeviceVendor(nsAString &aDeviceVendor);

  virtual nsresult OnCanUpdate(sbIDevice *aDevice,
                               PRUint32 aDeviceVendorID,
                               PRUint32 aDeviceProductID,
                               bool *_retval);
  virtual nsresult OnRebind(sbIDevice *aDevice,
                          sbIDeviceEventListener *aListener,
                          bool *_retval);
  virtual nsresult OnCancel();
  virtual nsresult OnRefreshInfo();
  virtual nsresult OnUpdate(sbIDeviceFirmwareUpdate *aFirmwareUpdate);
  virtual nsresult OnRecover(sbIDeviceFirmwareUpdate *aFirmwareUpdate);
  virtual nsresult OnVerifyDevice();
  virtual nsresult OnVerifyUpdate(sbIDeviceFirmwareUpdate *aFirmwareUpdate);
  virtual nsresult OnHttpRequestCompleted();

private:
  virtual ~sbMockDeviceFirmwareHandler();

protected:
  nsresult HandleRefreshInfoRequest();
  nsCOMPtr<sbPIMockFirmwareHandlerURLService> mHandlerURLService;
  PRInt32 mComplete;
};

#endif /*__SB_MOCKDEVICEFIRMWAREHANDLER_H__*/
