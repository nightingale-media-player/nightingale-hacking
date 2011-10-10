/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END NIGHTINGALE GPL
 */

#ifndef __SB_DEVICEFIRMWAREUPDATER_H__
#define __SB_DEVICEFIRMWAREUPDATER_H__

#include <sbIDeviceFirmwareUpdater.h>

#include <nsIEventTarget.h>
#include <nsIObserver.h>
#include <nsIRunnable.h>

#include <sbIDeviceEventListener.h>
#include <sbIDeviceFirmwareHandler.h>
#include <sbIDeviceFirmwareUpdate.h>
#include <sbIFileDownloader.h>

#include <nsClassHashtable.h>
#include <nsCOMPtr.h>
#include <nsHashKeys.h>
#include <nsInterfaceHashtable.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <prmon.h>

class sbDeviceFirmwareHandlerStatus;
class sbDeviceFirmwareDownloader;

class sbDeviceFirmwareUpdater : public sbIDeviceFirmwareUpdater,
                                public sbIDeviceEventListener,
                                public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_SBIDEVICEFIRMWAREUPDATER
  NS_DECL_SBIDEVICEEVENTLISTENER

  sbDeviceFirmwareUpdater();

  nsresult Init();
  nsresult Shutdown();

  already_AddRefed<sbIDeviceFirmwareHandler> 
    GetRunningHandler(sbIDevice *aDevice);

  already_AddRefed<sbIDeviceFirmwareHandler>
    GetRunningHandler(sbIDevice *aDevice, 
                      PRUint32 aVendorID,
                      PRUint32 aProductID,
                      sbIDeviceEventListener *aListener,
                      PRBool aCreate);

  nsresult PutRunningHandler(sbIDevice *aDevice, 
                             sbIDeviceFirmwareHandler *aHandler);

  sbDeviceFirmwareHandlerStatus* 
    GetHandlerStatus(sbIDeviceFirmwareHandler *aHandler);

  nsresult RequiresRecoveryMode(sbIDevice *aDevice,
                                sbIDeviceFirmwareHandler *aHandler);

  nsresult GetCachedFirmwareUpdate(sbIDevice *aDevice,
                                   sbIDeviceFirmwareUpdate **aUpdate);

private:
  virtual ~sbDeviceFirmwareUpdater();

protected:
  template<class T>
  static NS_HIDDEN_(PLDHashOperator)
    EnumerateIntoArrayISupportsKey(nsISupports *aKey,
                                   T* aData,
                                   void* aArray);

protected:
  PRMonitor*    mMonitor;
  PRPackedBool  mIsShutdown;

  typedef nsTArray<nsCString> firmwarehandlers_t;
  firmwarehandlers_t mFirmwareHandlers;

  typedef 
    nsInterfaceHashtableMT<nsISupportsHashKey, 
                           sbIDeviceFirmwareHandler>  runninghandlers_t;
  typedef 
    nsClassHashtableMT<nsISupportsHashKey,
                       sbDeviceFirmwareHandlerStatus> handlerstatus_t;
  typedef
    nsInterfaceHashtableMT<nsISupportsHashKey,
                           sbIFileDownloaderListener> downloaders_t;

  /* Hash table of sbIDevice -> sbIDeviceFirmwareHandler for firmware update
     processes in progress */
  runninghandlers_t mRunningHandlers;
  /* Hash table of sbIDevice -> sbIDeviceFirmwareHandler
     for device instances where we expect to resume firmware
     updating later. Here, the sbIDevice key is the original device (not the
     recovery mode device instance) */
  runninghandlers_t mRecoveryModeHandlers;
  /* Hash table of sbIDeviceFirmwareHandler -> sbIDeviceFirmwareHandlerStatus */
  handlerstatus_t   mHandlerStatus;
  /* Hash table of sbIDevice -> sbIFileDownloaderListener */
  downloaders_t     mDownloaders;

  nsCOMPtr<nsIEventTarget> mThreadPool;
};

class sbDeviceFirmwareHandlerStatus
{
public:
  typedef enum  {
    STATUS_NONE = 0,
    STATUS_WAITING_FOR_START,
    STATUS_RUNNING,
    STATUS_FINISHED
  } handlerstatus_t;

  typedef enum {
    OP_NONE = 0,
    OP_REFRESH,
    OP_DOWNLOAD,
    OP_UPDATE,
    OP_RECOVERY
  } handleroperation_t;

  sbDeviceFirmwareHandlerStatus();
  ~sbDeviceFirmwareHandlerStatus();

  nsresult Init();

  nsresult GetOperation(handleroperation_t *aOperation);
  nsresult SetOperation(handleroperation_t aOperation);

  nsresult GetStatus(handlerstatus_t *aStatus);
  nsresult SetStatus(handlerstatus_t aStatus);

private:
  PRMonitor* mMonitor;
  
  handleroperation_t mOperation;
  handlerstatus_t    mStatus;
};

class sbDeviceFirmwareUpdaterRunner : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  sbDeviceFirmwareUpdaterRunner();

  nsresult Init(sbIDevice *aDevice,
                sbIDeviceFirmwareUpdate *aUpdate, 
                sbIDeviceFirmwareHandler *aHandler,
                PRBool aRecovery = PR_FALSE);

private:
  virtual ~sbDeviceFirmwareUpdaterRunner();

protected:
  nsCOMPtr<sbIDevice>                 mDevice;
  nsCOMPtr<sbIDeviceFirmwareUpdate>   mFirmwareUpdate;
  nsCOMPtr<sbIDeviceFirmwareHandler>  mHandler;

  PRPackedBool mRecovery;
};

#define SB_DEVICEFIRMWAREUPDATER_DESCRIPTION               \
  "Nightingale Device Firmware Updater"
#define SB_DEVICEFIRMWAREUPDATER_CONTRACTID                \
  "@getnightingale.com/Nightingale/Device/Firmware/Updater;1"
#define SB_DEVICEFIRMWAREUPDATER_CLASSNAME                 \
  "Nightingale Device Firmware Updater"
#define SB_DEVICEFIRMWAREUPDATER_CID                       \
{ /* {9a84d24f-b02b-42bc-a2cb-b4792023aa70} */             \
  0x9a84d24f, 0xb02b, 0x42bc,                              \
  { 0xa2, 0xcb, 0xb4, 0x79, 0x20, 0x23, 0xaa, 0x70 } }

#endif /*__SB_DEVICEFIRMWAREUPDATER_H__*/
