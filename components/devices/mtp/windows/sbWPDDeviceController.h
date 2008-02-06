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

#ifndef SBWPDDEVICECONTROLLER_H_
#define SBWPDDEVICECONTROLLER_H_

#include "sbBaseDeviceController.h"
#include "sbBaseDeviceEventTarget.h"

#include <sbIDeviceController.h>
#include <sbIDeviceEventTarget.h>

#include <nsHashKeys.h>
#include <nsIClassInfo.h>
#include <nsInterfaceHashtable.h>
#include <nsIObserver.h>

class sbWPDDeviceController : public sbBaseDeviceController,
                              public sbBaseDeviceEventTarget,
                              public sbIDeviceController,
                              public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICECONTROLLER
  NS_DECL_NSICLASSINFO

  sbWPDDeviceController();

protected:
  virtual ~sbWPDDeviceController();

private:
  PRMonitor* mMonitor;
  nsInterfaceHashtable<nsIDHashKey, sbIDevice> mDevices;

};

#define SB_WPDCONTROLLER_CID \
{ 0xcbf1e6b3, 0x8bef, 0x4ce3, { 0x80, 0x83, 0x87, 0x6e, 0x4b, 0x66, 0x7f, 0xba } }

#define SB_WPDCONTROLLER_CONTRACTID \
  "@songbirdnest.com/Songbird/WPDController;1"

#define SB_WPDCONTROLLER_CLASSNAME \
  "WindowsPortableDeviceController"

#define SB_WPDCONTROLLER_DESCRIPTION \
  "Windows Portable Device Controller"

#endif //SBWPDDEVICECONTROLLER_H_