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

#ifndef __SB_DEVICEFIRMWAREUPDATE_H__
#define __SB_DEVICEFIRMWAREUPDATE_H__

#include <sbIDeviceFirmwareUpdate.h>

#include <nsIClassInfo.h>
#include <nsIFile.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <prmon.h>

class sbDeviceFirmwareUpdate : public sbIDeviceFirmwareUpdate,
                               public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBIDEVICEFIRMWAREUPDATE

  sbDeviceFirmwareUpdate();

private:
  virtual ~sbDeviceFirmwareUpdate();

protected:
  PRMonitor* mMonitor;

  nsCOMPtr<nsIFile> mFirmwareImageFile;
  nsString          mFirmwareReadableVersion;
  PRUint32          mFirmwareVersion;
};

#define SB_DEVICEFIRMWAREUPDATE_DESCRIPTION                \
  "Nightingale Device Firmware Update"
#define SB_DEVICEFIRMWAREUPDATE_CONTRACTID                 \
  "@getnightingale.com/Nightingale/Device/Firmware/Update;1"
#define SB_DEVICEFIRMWAREUPDATE_CLASSNAME                  \
  "Nightingale Device Firmware Update"
#define SB_DEVICEFIRMWAREUPDATE_CID                        \
{ /* {c5dc91bb-1b27-431e-8f93-0112022a9548} */             \
  0xc5dc91bb, 0x1b27, 0x431e,                              \
  { 0x8f, 0x93, 0x1, 0x12, 0x2, 0x2a, 0x95, 0x48 } }

#endif /*__SB_DEVICEFIRMWAREUPDATE_H__*/
