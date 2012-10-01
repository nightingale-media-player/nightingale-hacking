/* vim: set sw=2 :miv */
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

#include "sbIDeviceStatus.h"

#include <nsStringAPI.h>
#include <nsCOMPtr.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>

class sbIDataRemote;
class nsIProxyObjectManager;

class sbDeviceStatus : public sbIDeviceStatus
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICESTATUS
  
  sbDeviceStatus();

private:
  ~sbDeviceStatus();

  nsString mDeviceID;
  PRUint32 mCurrentState;
  PRUint32 mCurrentSubState;
  nsCOMPtr<sbIDataRemote> mStatusRemote;
  nsCOMPtr<sbIDataRemote> mOperationRemote;
  nsCOMPtr<sbIDataRemote> mProgressRemote;
  nsCOMPtr<sbIDataRemote> mWorkCurrentTypeRemote;
  nsCOMPtr<sbIDataRemote> mWorkCurrentCountRemote;
  nsCOMPtr<sbIDataRemote> mWorkTotalCountRemote;
  nsCOMPtr<sbIMediaItem> mItem;
  nsCOMPtr<sbIMediaList> mList;
  PRTime mTimestamp;
  PRInt64 mCurrentProgress;
  bool mNewBatch;
  
  nsresult GetDataRemote(nsIProxyObjectManager* aProxyObjectManager,
                         const nsAString& aDataRemoteName,
                         const nsAString& aDataRemotePrefix,
                         void** appDataRemote);

};

#define SONGBIRD_DEVICESTATUS_DESCRIPTION             \
  "Songbird Device Status Component"
#define SONGBIRD_DEVICESTATUS_CONTRACTID              \
  "@songbirdnest.com/Songbird/Device/DeviceStatus;1"
#define SONGBIRD_DEVICESTATUS_CLASSNAME               \
  "Songbird Device Status"
#define SONGBIRD_DEVICESTATUS_CID                     \
{ /* 7b2026c4-9193-4cbd-818a-0d 07 ab ae c8 54 */              \
  0x7b2026c4,                                             \
  0x9193,                                                 \
  0x4cbd,                                                 \
  {0x81, 0x8a, 0x0d, 0x07, 0xab, 0xae, 0xc8, 0x54}        \
}
