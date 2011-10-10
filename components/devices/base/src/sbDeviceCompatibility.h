/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#ifndef __SBDEVICECOMPATIBILITY_H__
#define __SBDEVICECOMPATIBILITY_H__

#include <sbIDeviceCompatibility.h>

class sbDeviceCompatibility : public sbIDeviceCompatibility
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICECOMPATIBILITY

  sbDeviceCompatibility();
  virtual ~sbDeviceCompatibility();

  nsresult Init(PRUint32 aCompatibility, 
                PRUint32 aUserPreference);

private:
  PRBool    mInitialized;
  PRUint32  mCompatibility;
  PRUint32  mUserPreference;

};

#endif /* __SBDEVICECOMPATIBILITY_H__ */

