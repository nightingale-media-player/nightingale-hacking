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

/** 
* \file  sbINativeWindowManager.h
* \brief Native window manager interface implementation
*/

#ifndef __SB_NATIVE_WINDOW_MANAGER_H__
#define __SB_NATIVE_WINDOW_MANAGER_H__

#include "sbINativeWindowManager.h"

#define SONGBIRD_NATIVEWINDOWMANAGER_CONTRACTID                  \
  "@songbirdnest.com/integration/native-window-manager;1"
#define SONGBIRD_NATIVEWINDOWMANAGER_CLASSNAME                   \
  "Songbird native window manager interface"
#define SONGBIRD_NATIVEWINDOWMANAGER_CID                         \
{ /* 3ec4c167-4dee-45c5-b7f1-5091cc8192e8 */              \
  0x3ec4c167,                                             \
  0x4dee,                                                 \
  0x45c5,                                                 \
  {0xb7, 0xf1, 0x50, 0x91, 0xcc, 0x81, 0x92, 0x8e}        \
}

class sbNativeWindowManager : public sbINativeWindowManager
{
public:
  sbNativeWindowManager() { };
  virtual ~sbNativeWindowManager() { };

  NS_DECL_ISUPPORTS
  NS_DECL_SBINATIVEWINDOWMANAGER
};

#endif // __SB_NATIVE_WINDOW_MANAGER_H__

