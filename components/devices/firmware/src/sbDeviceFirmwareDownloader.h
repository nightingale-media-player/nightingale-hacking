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

#ifndef __SB_DEVICEFIRMWAREDOWNLOADER_H__
#define __SB_DEVICEFIRMWAREDOWNLOADER_H__

#include <nsIFile.h>
#include <nsITimer.h>
#include <nsIURI.h>
#include <nsIXMLHttpRequest.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <prmon.h>

#include <sbIDevice.h>
#include <sbIDeviceEvent.h>
#include <sbIDeviceEventListener.h>
#include <sbIDeviceFirmwareHandler.h>
#include <sbIDeviceManager.h>
#include <sbIFileDownloader.h>

class sbDeviceFirmwareDownloader : public sbIFileDownloaderListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIFILEDOWNLOADERLISTENER

  sbDeviceFirmwareDownloader();

  nsresult Init(sbIDevice *aDevice, 
                sbIDeviceEventListener *aListener,
                sbIDeviceFirmwareHandler *aHandler);

  nsresult CreateCacheDirForDevice();
  nsresult GetCacheDirForDevice(nsIFile **aCacheDir);
  PRBool   IsAlreadyInCache();
  nsresult GetCachedFile(nsIFile **aFile);
 
  nsresult Start();
  nsresult Cancel();

  nsresult HandleProgress();
  nsresult HandleComplete();

  nsresult CreateDeviceEvent(PRUint32 aType, 
                             nsIVariant *aData, 
                             sbIDeviceEvent **aEvent);
  nsresult SendDeviceEvent(sbIDeviceEvent *aEvent, PRBool aAsync = PR_TRUE);
  nsresult SendDeviceEvent(PRUint32 aType,
                           nsIVariant *aData,
                           PRBool aAsync = PR_TRUE);
protected:
  virtual ~sbDeviceFirmwareDownloader();

private:
  nsCOMPtr<nsIFile>                   mCacheDir;
  nsCOMPtr<nsIFile>                   mDeviceCacheDir;
  nsCOMPtr<sbIDevice>                 mDevice;
  nsCOMPtr<sbIDeviceEventListener>    mListener;
  nsCOMPtr<sbIDeviceFirmwareHandler>  mHandler;
  nsCOMPtr<sbIFileDownloader>         mDownloader;
};

#endif /*__SB_DEVICEFIRMWAREDOWNLOADER_H__*/
