/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2011 POTI, Inc.
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

#include "sbBaseDevice.h"
#include "sbIMockDevice.h"
#include <sbIDeviceProperties.h>
#include <vector>
#include <sbDeviceStatusHelper.h>

class sbDeviceContent;

class sbMockDevice : public sbBaseDevice,
                     public sbIMockDevice
{
public:
  sbMockDevice();
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICE
  NS_DECL_SBIMOCKDEVICE

public:
  nsresult ProcessBatch(Batch & aBatch);

protected:
  PRBool mIsConnected;

  nsCOMPtr<sbDeviceContent> mContent;
  nsCOMPtr<sbIDeviceProperties> mProperties;
  std::vector<nsRefPtr<sbRequestItem> > mBatch;

private:
  virtual ~sbMockDevice();
  /**
   * Performs the disconnect
   */
  virtual nsresult DeviceSpecificDisconnect();
};
