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

#ifndef __SBDEVICEPROPERTIES_H__
#define __SBDEVICEPROPERTIES_H__

#include <sbIDeviceProperties.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <nsIURI.h>
#include <nsIVariant.h>
#include <nsIWritablePropertyBag.h>
#include <nsIWritablePropertyBag2.h>

struct PRLock;

class sbDeviceProperties : public sbIDeviceProperties
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEPROPERTIES

  sbDeviceProperties();

private:
  ~sbDeviceProperties();

protected:
  PRLock * mLock;
  PRPackedBool isInitialized;

  nsCOMPtr<nsIWritablePropertyBag> mProperties;
  nsCOMPtr<nsIWritablePropertyBag2> mProperties2;
  nsCOMPtr<nsIURI> mDeviceLocation;
  nsCOMPtr<nsIURI> mDeviceIcon;
};

#define SONGBIRD_DEVICEPROPERTIES_DESCRIPTION             \
  "Songbird Device Properties Component"
#define SONGBIRD_DEVICEPROPERTIES_CONTRACTID              \
  "@songbirdnest.com/Songbird/Device/DeviceProperties;1"
#define SONGBIRD_DEVICEPROPERTIES_CLASSNAME               \
  "Songbird Device Properties"
#define SONGBIRD_DEVICEPROPERTIES_CID                     \
{ /* 1b7449fe-86d1-4668-85fb-9d75591e48cd */              \
  0x1b7449fe,                                             \
  0x86d1,                                                 \
  0x4668,                                                 \
  {0x85, 0xfb, 0x9d, 0x75, 0x59, 0x1e, 0x48, 0xcd}        \
}

#endif /* __SBDEVICEPROPERTIES_H__ */

