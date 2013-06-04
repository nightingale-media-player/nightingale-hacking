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

#ifndef __SB_DEVICEFIRMWARESUPPORT_H__
#define __SB_DEVICEFIRMWARESUPPORT_H__

#include <sbIDeviceFirmwareSupport.h>

#include <nsIClassInfo.h>
#include <nsIMutableArray.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <mozilla/Monitor.h>

class sbDeviceFirmwareSupport : public sbIDeviceFirmwareSupport,
                                public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBIDEVICEFIRMWARESUPPORT

  sbDeviceFirmwareSupport();

private:
  virtual ~sbDeviceFirmwareSupport();

protected:
  mozilla::Monitor mMonitor;

  nsString                  mDeviceFriendlyName;
  PRUint32                  mDeviceVendorID;
  nsCOMPtr<nsIMutableArray> mDeviceProductIDs;
};

#define SB_DEVICEFIRMWARESUPPORT_DESCRIPTION               \
  "Songbird Device Firmware Support"
#define SB_DEVICEFIRMWARESUPPORT_CONTRACTID                \
  "@songbirdnest.com/Songbird/Device/Firmware/Support;1"
#define SB_DEVICEFIRMWARESUPPORT_CLASSNAME                 \
  "Songbird Device Firmware Support"
#define SB_DEVICEFIRMWARESUPPORT_CID                       \
/*{c71e74a4-ff35-4f98-bd9c-29d1c45d524e}*/                 \
{ 0xc71e74a4, 0xff35, 0x4f98, { 0xbd, 0x9c, 0x29, 0xd1, 0xc4, 0x5d, 0x52, 0x4e } }

#endif /*__SB_DEVICEFIRMWARESUPPORT_H__*/
