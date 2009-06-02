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

#include <sbIDeviceFirmwareHandler.h>

#include <nsIURI.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <prmon.h>

class sbBaseDeviceFirmwareHandler : public sbIDeviceFirmwareHandler
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEFIRMWAREHANDLER

  sbBaseDeviceFirmwareHandler();

  nsresult Init();
  
  /**
   * \brief Create an nsIURI from a spec string (e.g. http://some.url.com/path)
   *        in a thread-safe manner
   * \param aURISpec - The spec string
   * \param aURI - The pointer which will hold the new URI
   */
  nsresult CreateProxiedURI(const nsACString &aURISpec, 
                            nsIURI **aURI);

  // override me, see cpp file for implementation notes
  virtual nsresult OnInit();
  // override me, see cpp file for implementation notes
  virtual nsresult OnCanUpdate(sbIDevice *aDevice, 
                               sbIDeviceEventListener *aListener, 
                               PRBool *_retval);
  // override me, see cpp file for implementation notes
  virtual nsresult OnRefreshInfo(sbIDevice *aDevice, 
                                 sbIDeviceEventListener *aListener);
  // override me, see cpp file for implementation notes
  virtual nsresult OnUpdate(sbIDevice *aDevice, 
                            sbIDeviceFirmwareUpdate *aFirmwareUpdate, 
                            sbIDeviceEventListener *aListener);
  // override me, see cpp file for implementation notes
  virtual nsresult OnVerifyDevice(sbIDevice *aDevice, 
                                  sbIDeviceEventListener *aListener);
  // override me, see cpp file for implementation notes
  virtual nsresult OnVerifyUpdate(sbIDevice *aDevice, 
                                  sbIDeviceFirmwareUpdate *aFirmwareUpdate, 
                                  sbIDeviceEventListener *aListener);

private:
  virtual ~sbBaseDeviceFirmwareHandler();

protected:
  PRMonitor* mMonitor;

  PRUint32 mFirmwareVersion;

  nsString mContractId;
  nsString mReadableFirmwareVersion;

  nsCOMPtr<nsIURI> mFirmwareLocation;
  nsCOMPtr<nsIURI> mReleaseNotesLocation;
  nsCOMPtr<nsIURI> mResetInstructionsLocation;

};