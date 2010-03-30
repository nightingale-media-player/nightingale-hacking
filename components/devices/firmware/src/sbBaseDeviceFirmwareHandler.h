/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
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
 *=END SONGBIRD GPL
 */

#ifndef __SB_BASEDEVICEFIRMWAREHANDLER_H__
#define __SB_BASEDEVICEFIRMWAREHANDLER_H__

#include <sbIDeviceFirmwareHandler.h>

#include <nsIMutableArray.h>
#include <nsITimer.h>
#include <nsIURI.h>
#include <nsIXMLHttpRequest.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <prmon.h>

#include <sbIDevice.h>
#include <sbIDeviceEvent.h>
#include <sbIDeviceEventListener.h>
#include <sbIDeviceManager.h>

class sbBaseDeviceFirmwareHandler : public sbIDeviceFirmwareHandler,
                                    public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_SBIDEVICEFIRMWAREHANDLER

  typedef enum {
    HANDLER_IDLE = 0,
    HANDLER_REFRESHING_INFO,
    HANDLER_UPDATING_DEVICE,
    HANDLER_X
  } handlerstate_t;

  sbBaseDeviceFirmwareHandler();

  nsresult Init();
  
  /**
   * \brief Append an entry to the list of supported devices.
   */
  nsresult AppendSupportedDevice(const nsAString &aDeviceFriendlyName,
                                 const PRUint32 aDeviceVendorID,
                                 const PRUint32 aDeviceProductID);

  nsresult AppendSupportedDeviceProductID(const nsAString &aDeviceFriendlyName,
                                          const PRUint32 aDeviceProductID);

  /**
   * \brief Create an nsIURI from a spec string (e.g. http://some.url.com/path)
   *        in a thread-safe manner
   * \param aURISpec - The spec string
   * \param aURI - The pointer which will hold the new URI
   */
  nsresult CreateProxiedURI(const nsACString &aURISpec, 
                            nsIURI **aURI);

  /**
   * \brief Send an HTTP request
   *
   * \param aMethod - The HTTP Method to use, currently we support POST and GET
   * \param aUrl - The URL to send the request to, as a string.
   * \param aUsername [optional] - HTTP authentication username.
   * \param aPassword [optional] - HTTP authentication password.
   * \param aContentType [optional] - Content type to use for request
   * \param aRequestBody [optional] - Request body, if any 
   *                                  (string or input stream)
   *
   * \note The request is always sent asynchronously.
   *       The 'OnHttpRequestCompleted' will get called when the 
   *       request completed. If there is a pending request, 
   *       calling this method will fail.
   * \throw NS_ERROR_ABORT when there is already a request pending.
   */
  nsresult SendHttpRequest(const nsACString &aMethod, 
                           const nsACString &aUrl,
                           const nsAString &aUsername = EmptyString(),
                           const nsAString &aPassword = EmptyString(),
                           const nsACString &aContentType = EmptyCString(),
                           nsIVariant *aRequestBody = nsnull);


  /**
   * \brief Abort an HTTP request
   */
  nsresult AbortHttpRequest();

  /**
   * \brief Create a device event
   */
  nsresult CreateDeviceEvent(PRUint32 aType, 
                             nsIVariant *aData, 
                             sbIDeviceEvent **aEvent);

  /**
   * \brief Send a device event
   */
  nsresult SendDeviceEvent(sbIDeviceEvent *aEvent, PRBool aAsync = PR_TRUE);

  /**
   * \brief Send a device event
   */
  nsresult SendDeviceEvent(PRUint32 aType,
                           nsIVariant *aData,
                           PRBool aAsync = PR_TRUE);

  /**
   * \brief Set internal state
   */
  handlerstate_t GetState();

  /**
   * \brief Get internal state
   */
  nsresult SetState(handlerstate_t aState);

  /**
   * \brief Check nsresult value, send error event if 
   *        nsresult value is an error.
   * \param aResult - The result to check
   * \param aEventType - The event to send in the case where aResult
   *                     is an error.
   */
  nsresult CheckForError(const nsresult &aResult, 
                         PRUint32 aEventType,
                         nsIVariant *aData = nsnull);

  // override me, see cpp file for implementation notes
  virtual nsresult OnInit();

  // override me, see cpp file for implementation notes
  virtual nsresult OnGetCurrentFirmwareVersion(PRUint32 *aCurrentFirmwareVersion);
  // override me, see cpp file for implementation notes
  virtual nsresult OnGetCurrentFirmwareReadableVersion(nsAString &aCurrentFirmwareReadableVersion);
  // override me, see cpp file for implementation notes
  virtual nsresult OnGetRecoveryMode(PRBool *aRecoveryMode);

  // override me, see cpp file for implementation notes
  virtual nsresult OnGetDeviceModelNumber(nsAString &aModelNumber);
  // override me, see cpp file for implementation notes
  virtual nsresult OnGetDeviceModelVersion(nsAString &aModelVersion);
  // override me, see cpp file for implementation notes
  virtual nsresult OnGetDeviceVendor(nsAString &aDeviceVendor);

  // override me, see cpp file for implementation notes
  virtual nsresult OnGetSupportedDevices(nsISimpleEnumerator **aSupportedDevices);

  // override me, see cpp file for implementation notes
  virtual nsresult OnCanUpdate(sbIDevice *aDevice, 
                               PRUint32 aDeviceVendorID,
                               PRUint32 aDeviceProductID,
                               PRBool *_retval);
  // override me, see cpp file for implementation notes
  virtual nsresult OnRebind(sbIDevice *aDevice,
                            sbIDeviceEventListener *aListener,
                            PRBool *_retval);
  // override me, see cpp file for implementation notes
  virtual nsresult OnCancel();
  // override me, see cpp file for implementation notes
  virtual nsresult OnRefreshInfo();
  // override me, see cpp file for implementation notes
  virtual nsresult OnUpdate(sbIDeviceFirmwareUpdate *aFirmwareUpdate);
  // override me, see cpp file for implementation notes
  virtual nsresult OnRecover(sbIDeviceFirmwareUpdate *aFirmwareUpdate);
  // override me, see cpp file for implementation notes
  virtual nsresult OnVerifyDevice();
  // override me, see cpp file for implementation notes
  virtual nsresult OnVerifyUpdate(sbIDeviceFirmwareUpdate *aFirmwareUpdate);
  // override me, see cpp file for implementation notes
  virtual nsresult OnHttpRequestCompleted();

protected:
  virtual ~sbBaseDeviceFirmwareHandler();

  PRMonitor* mMonitor;

  nsCOMPtr<sbIDevice> mDevice;
  nsCOMPtr<sbIDeviceEventListener> mListener;
  nsCOMPtr<sbIDeviceEventListener> mProxiedListener;

  handlerstate_t mHandlerState;
  PRUint32 mFirmwareVersion;
  PRUint32 mDefaultFirmwareVersion;

  nsString mContractId;
  nsString mReadableFirmwareVersion;
  nsString mDefaultReadableFirmwareVersion;

  nsCOMPtr<nsIURI> mDefaultFirmwareLocation;
  nsCOMPtr<nsIURI> mFirmwareLocation;
  nsCOMPtr<nsIURI> mDefaultReleaseNotesLocation;
  nsCOMPtr<nsIURI> mReleaseNotesLocation;
  nsCOMPtr<nsIURI> mResetInstructionsLocation;
  nsString         mRecoveryModeKeyCombination;
  nsCOMPtr<nsIURI> mSupportLocation;
  nsCOMPtr<nsIURI> mRegisterLocation;

  PRPackedBool mNeedsRecoveryMode;
  PRPackedBool mRecoveryMode;

  nsCOMPtr<nsIXMLHttpRequest> mXMLHttpRequest;
  nsCOMPtr<nsITimer>          mXMLHttpRequestTimer;

  nsCOMPtr<nsIMutableArray>   mSupportedDevices;
};

#endif /*__SB_BASEDEVICEFIRMWAREHANDLER_H__*/
