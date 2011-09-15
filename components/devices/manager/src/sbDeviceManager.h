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

#include "sbIDeviceManager.h"
#include "sbIDeviceControllerRegistrar.h"
#include "sbIDeviceRegistrar.h"
#include "sbIDeviceEventTarget.h"
#include "sbBaseDeviceEventTarget.h"

#include <prmon.h>
#include <nsHashKeys.h>
#include <nsIClassInfo.h>
#include <nsInterfaceHashtable.h>
#include <nsIObserver.h>

#include <sbWeakReference.h>

#include "sbIDevice.h"
#include "sbIDeviceController.h"
#include "sbIDeviceMarshall.h"

class sbDeviceManager : public sbBaseDeviceEventTarget,
                        public sbIDeviceManager2,
                        public sbIDeviceControllerRegistrar,
                        public sbIDeviceRegistrar,
                        public nsIClassInfo,
                        public nsIObserver,
                        public sbSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEMANAGER2
  NS_DECL_SBIDEVICECONTROLLERREGISTRAR
  NS_DECL_SBIDEVICEREGISTRAR
  NS_DECL_NSICLASSINFO
  NS_DECL_NSIOBSERVER
  sbDeviceManager();

private:
  ~sbDeviceManager();

protected:
  // copies the given hash table into the given mutable array
  template<class T>
  static NS_HIDDEN_(PLDHashOperator) EnumerateIntoArray(const nsID& aKey,
                                                        T* aData,
                                                        void* aArray);
  
  // helpers
  nsresult Init();
  nsresult BeginMarshallMonitoring();
  nsresult PrepareShutdown();
  nsresult FinalShutdown();
  nsresult QuitApplicationRequested(PRBool *aShouldQuit);
  nsresult QuitApplicationGranted();
  nsresult RemoveAllDevices();

protected:
  PRMonitor* mMonitor;
  nsInterfaceHashtableMT<nsIDHashKey, sbIDeviceController> mControllers;
  nsInterfaceHashtableMT<nsIDHashKey, sbIDevice> mDevices;
  nsInterfaceHashtableMT<nsIDHashKey, sbIDeviceMarshall> mMarshalls;
  PRBool mHasAllowedShutdown;
};

#define SONGBIRD_DEVICEMANAGER2_DESCRIPTION                \
  "Songbird DeviceManager2 Service"
#define SONGBIRD_DEVICEMANAGER2_CONTRACTID                 \
  "@songbirdnest.com/Songbird/DeviceManager;2"
#define SONGBIRD_DEVICEMANAGER2_CLASSNAME                  \
  "Songbird Device Manager 2"
#define SONGBIRD_DEVICEMANAGER2_CID                        \
{ /* {F1B2417E-2515-481d-B03A-DA3D5B7F62FA} */             \
  0xf1b2417e, 0x2515, 0x481d,                              \
  { 0xb0, 0x3a, 0xda, 0x3d, 0x5b, 0x7f, 0x62, 0xfa } }
