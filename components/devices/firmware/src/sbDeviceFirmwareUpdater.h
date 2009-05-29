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

#include <sbIDeviceFirmwareUpdater.h>

#include <nsStringGlue.h>
#include <prmon.h>

class sbDeviceFirmwareUpdater : public sbIDeviceFirmwareUpdater
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEFIRMWAREUPDATER

  sbDeviceFirmwareUpdater();

  nsresult Init();

private:
  virtual ~sbDeviceFirmwareUpdater();

protected:
  PRMonitor* mMonitor;

};

#define SB_DEVICEFIRMWAREUPDATER_DESCRIPTION               \
  "Songbird Device Firmware Updater"
#define SB_DEVICEFIRMWAREUPDATER_CONTRACTID                \
  "@songbirdnest.com/Songbird/Device/Firmware/Updater;1"
#define SB_DEVICEFIRMWAREUPDATER_CLASSNAME                 \
  "Songbird Device Firmware Updater"
#define SB_DEVICEFIRMWAREUPDATER_CID                       \
{ /* {9a84d24f-b02b-42bc-a2cb-b4792023aa70} */             \
  0x9a84d24f, 0xb02b, 0x42bc,                              \
  { 0xa2, 0xcb, 0xb4, 0x79, 0x20, 0x23, 0xaa, 0x70 } }
