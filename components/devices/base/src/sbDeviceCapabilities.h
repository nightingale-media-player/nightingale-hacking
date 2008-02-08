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

#ifndef __SBDEVICECAPABILITIES_H__
#define __SBDEVICECAPABILITIES_H__

#include <sbIDeviceCapabilities.h>

#include <nsCOMPtr.h>
#include <nsClassHashtable.h>
#include <nsTArray.h>
#include "nsMemory.h"

class sbDeviceCapabilities : public sbIDeviceCapabilities
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICECAPABILITIES

  sbDeviceCapabilities();

private:
  ~sbDeviceCapabilities();

protected:
  PRBool isInitialized;

  nsTArray<PRUint32> mFunctionTypes;
  nsClassHashtable<nsUint32HashKey, nsTArray<PRUint32> > mContentTypes;
  nsClassHashtable<nsUint32HashKey, nsTArray<PRUint32> > mSupportedFormats;
  nsTArray<PRUint32> mSupportedEvents;
};

#define SONGBIRD_DEVICECAPABILITIES_DESCRIPTION             \
  "Songbird Device Capabilities Component"
#define SONGBIRD_DEVICECAPABILITIES_CONTRACTID              \
  "@songbirdnest.com/Songbird/Device/DeviceCapabilities;1"
#define SONGBIRD_DEVICECAPABILITIES_CLASSNAME               \
  "Songbird Device Capabilities"
#define SONGBIRD_DEVICECAPABILITIES_CID                     \
{ /* 54d42a87-9031-4928-991e-e66f4916a90b */              \
  0x54d42a87,                                             \
  0x9031,                                                 \
  0x4928,                                                 \
  {0x99, 0x1e, 0xe6, 0x6f, 0x49, 0x16, 0xa9, 0x0b}        \
}

#endif /* __SBDEVICECAPABILITIES_H__ */

